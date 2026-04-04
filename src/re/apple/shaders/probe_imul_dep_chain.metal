#include <metal_stdlib>
using namespace metal;

// H1c: AGX Integer Multiply Dep-Chain Latency Probes
//
// Goal: measure dep-chain latency of:
//   (a) IMUL32     — 32×32→32 truncated multiply  (reference)
//   (b) IMUL32×64  — 32×32→64 widening multiply    (H1c target)
//
// Philip Turner's metal-benchmarks: IMUL(32×32=64) adj_lat ≈ 8–9 cycles on M1.
//
// PROBLEM WITH PRIOR INT64 PROBES (probe_int64_mul_lat.metal):
//   `acc *= 1664525L` — constant multiplier. LLVM strength-reduces constant-multiply to
//   a shift+add sequence on some targets, replacing the HW multiply with cheaper ops.
//   Result: measured latency underestimates true IMUL latency.
//
// FIX: both operands must be RUNTIME-VARIABLE (derived from acc each iteration).
//   Step: acc = ulong(uint(acc)) * ulong(uint(acc >> 16) | 1u)
//   - First operand: uint(acc) — lower 32 bits of acc. Changes every iteration.
//   - Second operand: uint(acc >> 16) | 1u — bits [47:16] of acc, forced odd.
//     Changes every iteration. LLVM cannot constant-fold or strength-reduce this.
//   The `| 1u` ensures the second operand is always non-zero and odd (full-range mul).
//
// STRUCTURE: 4-op unrolled dep-chain, 100K outer iters (matches H1b SNR budget).
//
// MEASUREMENT METHOD: SFU-SFU delta (same as H1b LOG2 vs EXP2):
//   Run probe_imul32_dep4 and probe_imul32x64_dep4 interleaved (--interleave flag).
//   Both have identical critical-path overhead (shift + OR before mul).
//   Delta eliminates loop overhead and the 2-cycle shift+OR pre-cost.
//   L(IMUL32×64) = L(IMUL32) + Δcyc(64−32)
//   L(IMUL32) can be pinned via FMA calibration: run probe_fma4_osc_dep from
//   probe_sfu_dep_chain.metallib interleaved with probe_imul32_dep4.
//
// OUTPUT BUFFER: device uint* (matches host's width × sizeof(uint32_t) allocation).
//   For the 64-bit probe, fold 64-bit result to 32: out[gid] = uint(acc) ^ uint(acc>>32).
//   This forces LLVM to track all 64 bits — it cannot reduce to 32-bit arithmetic.
//
// Build:
//   xcrun -sdk macosx metal -c -O2 -o probe_imul_dep_chain.air probe_imul_dep_chain.metal
//   xcrun -sdk macosx metallib probe_imul_dep_chain.air -o probe_imul_dep_chain.metallib
//
// H1c FINDINGS (2026-04-03):
//   probe_imul32_dep4:        step ≈ 5.2 cyc  (1_pre + IMUL32 4 cyc) ✓ Philip=4.03
//   probe_imul32x64_dep4 v1:  step ≈ 17.7 cyc (2_pre + IMUL64 16 cyc) ← TRUNC→ZEXT path
//   probe_imul32x64_v2_dep4:  step ≈ 7.2 cyc  (Δ=+2.1 vs IMUL32)
//     → dep chain critical path: hi32(r) → OR → next mul
//     → implied IMULHI32 adj_lat ≈ 6 cyc (+ 1 OR = 7 total)
//
// probe_imulhi32_dep4 (added): same structure but hi32(r) feeds a (not b),
//   so no OR pre-cost. Expected step = 6 cyc if IMULHI32 = 6 cyc.
//
// Run (example — interleave with FMA reference from probe_sfu_dep_chain.metallib):
//   Merge both .air into one .metallib for interleaved measurement:
//   xcrun -sdk macosx metallib probe_sfu_dep_chain.air probe_imul_dep_chain.air \
//       -o probe_imul_combined.metallib
//   ./metal_probe_host --metallib probe_imul_combined.metallib \
//       --kernel probe_fma4_osc_dep --kernel probe_imul32_dep4 \
//       --kernel probe_imul32x64_dep4 \
//       --width 1024 --iters 200 --shader-iters 100000 --interleave

// =====================================================================
// REFERENCE: 32×32→32 truncated multiply dep-chain × 4 per iter
// =====================================================================
//
// Critical path per step:
//   acc → [shift >> 16 → OR | 1u] (2 cycles) → IMUL32 ← [acc, identity] → new acc
//   The "identity" (passing acc as first operand) is free (same register read).
//   Step latency = L(SHIFT) + L(OR) + L(IMUL32) = 2 + L(IMUL32) cycles.
//
// Fixed-point behavior:
//   The dep chain does not have a traditional fixed point — it cycles through many
//   different 32-bit values. No degenerate convergence issues.
//
// Initialization: bit-mixed from gid so non-zero high bits appear immediately.
//   uint(gid+1) * 0x9E3779B9 — Fibonacci hashing constant (odd, good bit spread).
kernel void probe_imul32_dep4(
    device uint* out           [[buffer(0)]],
    constant uint& n_iters     [[buffer(1)]],
    uint gid                   [[thread_position_in_grid]])
{
    // Fibonacci hash for initial bit mixing — ensures high bits of acc are non-zero
    // so (acc >> 16) is non-trivial from iteration 1.
    uint acc = (gid + 1u) * 0x9E3779B9u;

    for (uint iter = 0u; iter < n_iters; iter++) {
        // Both operands are runtime-variable — LLVM cannot strength-reduce.
        // uint * uint → 32×32→32 multiply (AIR: mul i32).
        acc = acc * ((acc >> 16u) | 1u);
        acc = acc * ((acc >> 16u) | 1u);
        acc = acc * ((acc >> 16u) | 1u);
        acc = acc * ((acc >> 16u) | 1u);
    }
    out[gid] = acc;
}

// =====================================================================
// TARGET (v1 — EMITS IMUL64, NOT hardware widening): 32×32→64 dep-chain × 4
// =====================================================================
//
// DISCOVERY: This probe DOES NOT measure IMUL(32×32=64) hardware path.
//
// Problem: acc is typed ulong. LLVM emits:
//   trunc i64 acc to i32 → zext i32 to i64  (for each operand)
//   mul nuw i64
// The AGX backend does NOT recognize TRUNC→ZEXT as a widening multiply.
// It emits IMUL(64×64=64) at 16 cycles instead of IMUL(32×32=64) at 8 cycles.
//
// Evidence: measured step = 18.02 cycles at 1.252 GHz.
//   Expected widening path: 2_pre + 8_mul = 10 cycles.
//   Observed:               2_pre + 16_mul = 18 cycles ← IMUL64 confirmed.
//
// Left here for reference (measures IMUL(64×64=64) dep-chain adj_lat = 16 cyc).
// Use probe_imul32x64_v2_dep4 below for the hardware widening path.
kernel void probe_imul32x64_dep4(
    device uint* out           [[buffer(0)]],
    constant uint& n_iters     [[buffer(1)]],
    uint gid                   [[thread_position_in_grid]])
{
    ulong acc = (ulong)(gid + 1u) * 0x9E3779B97F4A7C15uL;

    for (uint iter = 0u; iter < n_iters; iter++) {
        // AIR: trunc i64 → zext i32 → mul nuw i64 — emits IMUL64 on AGX, NOT widening.
        acc = ulong(uint(acc)) * ulong(uint(acc >> 16u) | 1u);
        acc = ulong(uint(acc)) * ulong(uint(acc >> 16u) | 1u);
        acc = ulong(uint(acc)) * ulong(uint(acc >> 16u) | 1u);
        acc = ulong(uint(acc)) * ulong(uint(acc >> 16u) | 1u);
    }
    out[gid] = uint(acc) ^ uint(acc >> 32u);
}

// =====================================================================
// TARGET (v2 — NATIVE uint operands): 32×32→64 widening dep-chain × 4
// =====================================================================
//
// Fix for the TRUNC→ZEXT problem in probe_imul32x64_dep4:
//   Use natively-typed `uint a, b` accumulators. LLVM sees `zext i32 to i64`
//   directly (no intervening trunc), which the AGX backend recognizes as the
//   hardware IMUL(32×32=64) widening multiply (8 cycles adj_lat, Philip Turner).
//
// Dep-chain structure:
//   r = ulong(a) * ulong(b | 1u)   ← widening mul: zext(a) × zext(b_odd)
//   a = uint(r)                     ← lo 32 bits of result feed next a
//   b = uint(r >> 32u)              ← hi 32 bits of result feed next b
//
// Critical path per step:
//   r → [r >> 32 (1 cyc)] → b → [b | 1u (1 cyc)] → IMUL32×64 ← [a=uint(r)] → new r
//   Step latency = 2_pre + L(IMUL32×64) cycles.
//   Expected: 2 + 8 = 10 cycles → 7.99 ns/step at 1.252 GHz.
//
// LLVM analysis:
//   a and b are natively uint (i32). `ulong(a)` → zext i32 to i64 (no trunc).
//   `ulong(b | 1u)` → (or i32 b, 1) → zext i32 to i64 (no trunc).
//   mul nuw i64 of two zero-extended i32 values → AGX: IMUL(32×32=64) hardware.
//   `uint(r)` → trunc i64 to i32 → assigned to a (i32 native, no live-i64 chain).
//   `uint(r >> 32u)` → lshr i64 r, 32 → trunc to i32 → assigned to b.
//   LLVM must preserve full 64-bit r (out[gid] = a ^ b reads both halves).
//
// Initialization: Fibonacci hash for both a and b (different constants to ensure
//   non-trivial high bits in both halves from the first iteration).
//   a = (gid+1) * 0x9E3779B9 (32-bit Fibonacci constant)
//   b = a ^ 0xDEADBEEF (bit-mixed; always odd after |1u, so no zero product)
//
// Interleave reference: probe_imul32_dep4 (step ≈ 5 cyc, same pre_cost = 1 cyc).
//   Δ(v2 − imul32) eliminates the shared pre_cost.
//   L(IMUL32×64) = L(IMUL32) + Δcyc(v2 − imul32)
//   Expected Δ = 8 − 4 = 4 cycles → Δstep = 4/1.252GHz ≈ 3.19 ns/step.
//   (Pre_cost differs: v2 has 2-cycle pre [r>>32 + |1u], imul32 has 1-cycle pre [>>16+|1u].
//    So actual Δstep includes +1 cycle overhead: Δcyc_measured ≈ 5.)
kernel void probe_imul32x64_v2_dep4(
    device uint* out           [[buffer(0)]],
    constant uint& n_iters     [[buffer(1)]],
    uint gid                   [[thread_position_in_grid]])
{
    // Fibonacci-hash initial values: ensure non-zero high bits in both a and b.
    uint a = (gid + 1u) * 0x9E3779B9u;
    uint b = a ^ 0xDEADBEEFu;

    for (uint iter = 0u; iter < n_iters; iter++) {
        // Step 0: r0 = a * (b|1). Operands are native uint → direct zext → widening mul.
        ulong r0 = ulong(a) * ulong(b | 1u);
        a = uint(r0);
        b = uint(r0 >> 32u);

        // Step 1: dep on r0 via a and b.
        ulong r1 = ulong(a) * ulong(b | 1u);
        a = uint(r1);
        b = uint(r1 >> 32u);

        // Step 2: dep on r1.
        ulong r2 = ulong(a) * ulong(b | 1u);
        a = uint(r2);
        b = uint(r2 >> 32u);

        // Step 3: dep on r2.
        ulong r3 = ulong(a) * ulong(b | 1u);
        a = uint(r3);
        b = uint(r3 >> 32u);
    }
    // XOR both halves: forces LLVM to track full 64-bit products throughout loop.
    out[gid] = a ^ b;
}

// =====================================================================
// PROBE: IMULHI32 isolation — hi32 feeds `a` (no OR pre-cost)
// =====================================================================
//
// v2 dep chain went through b = uint(r >> 32), which required `b | 1u` (1 OR cycle)
// before feeding the next multiply's second operand. This added 1 cycle pre-cost.
// Measured v2 step ≈ 7.2 cyc → implied: IMULHI32(6 cyc) + OR(1 cyc) = 7.
//
// This probe: hi32 of r feeds `a` (first operand), NOT `b`.
// `a` feeds the next mul via `ulong(a)` = direct zext i32 → i64, FREE (no OR).
// `b` = lo32 of r, needs `b | 1u` (1 OR cycle), but b is ready at 4 cycles (IMUL32),
//   so OR completes at 5 cycles — within the 6-cycle IMULHI32 window for `a`.
// Critical path: hi32 ready at T+6 → zext (free) → mul starts at T+6 (no extra pre).
//
// Expected step:
//   IMULHI32 adj_lat = 6 cyc → step = 6 (0 pre on a-path; b-path OR at T+5 < T+6)
//   IMULHI32 adj_lat = 8 cyc (Philip) → step = 8
//   IMULHI32 adj_lat = 7 cyc → step = 7
//
// This probe disambiguates v2's 7.2-cycle result:
//   If imulhi32 step ≈ 6: confirms IMULHI32 = 6 cyc, and v2's +1 cycle = OR pre-cost.
//   If imulhi32 step ≈ 7: same as v2; OR is pipelined, effective IMULHI32 = 7 cyc.
//   If imulhi32 step ≈ 8: IMULHI32 = 8 cyc (matches Philip); v2 was something else.
//
// Dep chain per step:
//   a (native uint phi, hi32 of prev r) → zext i32 to i64 [free] ← first operand
//   b (native uint phi, lo32 of prev r) → or i32, 1u [1 cyc] → zext [free] ← second operand
//   mul: zext(a) × zext(b|1) → r (64-bit)
//   a_next = uint(r >> 32u)  [lshr hi-reg = free; trunc = free]  ← back to a phi
//   b_next = uint(r)         [lo-reg read = free; trunc = free]   ← back to b phi
//
// Critical path (loop-carried, per step):
//   {a,b} → mul [IMULHI32 adj_lat via a path; IMUL32+OR via b path]
//         → r_hi available at T + max(IMULHI32, IMUL32+1_OR) = T + max(6, 5) = T+6
//   a_next: from r_hi at T+6, free (hi-reg read + trunc = free) → T+6
//   b_next: from r_lo at T+4, free (lo-reg read + trunc = free) → T+4
//   Next mul: a_next at T+6 (critical), b_next at T+4 → b|1 at T+5, mul waits for a at T+6
//   Step = 6 cycles (if IMULHI32 = 6).
kernel void probe_imulhi32_dep4(
    device uint* out           [[buffer(0)]],
    constant uint& n_iters     [[buffer(1)]],
    uint gid                   [[thread_position_in_grid]])
{
    // a starts as hi32 of init product; b as lo32. Both non-zero from Fibonacci hash.
    uint a_init = (gid + 1u) * 0x9E3779B9u;
    uint b_init = a_init ^ 0xDEADBEEFu;
    ulong r_init = ulong(a_init) * ulong(b_init | 1u);
    uint a = uint(r_init >> 32u);   // start with hi32 in a
    uint b = uint(r_init);          // start with lo32 in b

    for (uint iter = 0u; iter < n_iters; iter++) {
        // Step 0: a = hi32(prev r), b = lo32(prev r).
        // a feeds first operand (direct zext, no OR pre-cost).
        // b feeds second operand (needs b|1, 1 OR cycle — but b ready at T+4, OR at T+5 < T+6).
        ulong r0 = ulong(a) * ulong(b | 1u);
        a = uint(r0 >> 32u);    // new a = hi32 of product (IMULHI32 path)
        b = uint(r0);           // new b = lo32 of product (IMUL32 path, fast)

        ulong r1 = ulong(a) * ulong(b | 1u);
        a = uint(r1 >> 32u);
        b = uint(r1);

        ulong r2 = ulong(a) * ulong(b | 1u);
        a = uint(r2 >> 32u);
        b = uint(r2);

        ulong r3 = ulong(a) * ulong(b | 1u);
        a = uint(r3 >> 32u);
        b = uint(r3);
    }
    out[gid] = a ^ b;
}
