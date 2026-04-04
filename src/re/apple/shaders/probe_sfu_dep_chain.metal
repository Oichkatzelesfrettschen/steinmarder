#include <metal_stdlib>
using namespace metal;

// H1b: AGX SFU / ALU single dep-chain latency probes.
//
// Designed to measure TRUE dep-chain latency (one op per iter on the critical path),
// NOT SFU throughput as in the 8-chain H1a probes.
//
// METHODOLOGY — isolating SFU op latency via subtraction:
//   Each SFU probe chains: [feedback FMA] → [SFU op] → [feedback FMA] → ...
//   L(SFU op) = T(SFU probe) - T(fma_baseline probe)  where both use identical FMA params.
//
//   EXP2: a = exp2(fma(a, -0.5f, 0.75f))       → L = T(probe_exp2_dep) - T(probe_fma_dep)
//   LOG2: a = log2(fma(a, a, 1.0f))             → L = T(probe_log2_dep) - T(probe_fma_sq_dep)
//   FMAX: a = fmax(fma(a,0.9f,0.1f), fma(a,0.1f,0.9f))   → L = T - T(probe_fma_dep)
//   SEL:  a = select(fma(a,0.1f,0.9f), fma(a,0.9f,0.1f), fma(a,0.9f,0.1f) > fma(a,0.1f,0.9f))
//
// SNR GUIDANCE:
//   Dispatch baseline C ≈ 3,153,956 ns. For reliable measurement, need net signal >> C.
//   At 4-cycle (3.07 ns) EXP2 latency, need shader_iters ≥ 3,000,000 for >3× SNR.
//   Recommended: --shader-iters 3000000 --iters 30
//
// RANGE STABILITY (verified by fixed-point analysis):
//   EXP2 probe: a* ≈ 1.135, convergence slope ≈ -0.39 → stable in ~20 iters
//   LOG2 probe: a* ≈ 0.77 (log2(a*²+1)=a*), convergence slope < 1 → stable in ~15 iters
//   FMAX probe: a* = 1.0 (crossover point), slope = 0.9 or 0.1 → stable in ~100 iters
//   SEL probe:  a* = 1.0 (same crossover), identical structure to FMAX
//
// Build:
//   xcrun -sdk macosx metal -c -O2 -o probe_sfu_dep_chain.air probe_sfu_dep_chain.metal
//   xcrun -sdk macosx metallib probe_sfu_dep_chain.air -o probe_sfu_dep_chain.metallib

// --- Loop-only baseline ---
// Measures pure loop overhead (counter increment + compare + back-branch + XOR dep).
// Subtract this from any probe's ns_per_iter to get the net compute time.
// Run this in the SAME metal_probe_host invocation as your probe kernels to ensure
// identical GPU clock state.
kernel void probe_loop_only(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    uint dummy = gid;
    for (uint iter = 0u; iter < n_iters; iter++) {
        dummy ^= iter;
    }
    out[gid] = float(dummy);
}

// --- EXP2 single dep-chain ---
// Critical path: FMA(a, -0.5, 0.75) → EXP2 → back to a
// ops_per_iter = 1 EXP2 + 1 FMA on critical path
// Fixed point: exp2(-a*/2 + 0.75) = a*, a* ≈ 1.135. Range: [0.9, 1.3].
kernel void probe_exp2_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        a = exp2(fma(a, -0.5f, 0.75f));
    }
    out[gid] = a;
}

// --- LOG2 single dep-chain ---
// Critical path: FMA(a, a, 1.0) = a²+1 → LOG2 → back to a
// ops_per_iter = 1 LOG2 + 1 FMA on critical path
// a²+1 ≥ 1 for all a, so log2(a²+1) ≥ 0. Fixed point: log2(a*²+1) = a*, a* ≈ 0.77.
kernel void probe_log2_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.8f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        a = log2(fma(a, a, 1.0f));
    }
    out[gid] = a;
}

// --- FMA baseline (matches EXP2 probe FMA parameters) ---
// Critical path: FMA(a, -0.5, 0.75) → back to a. No SFU op.
// Subtract this from probe_exp2_dep to isolate EXP2 latency.
// Fixed point: a* = a*(-0.5) + 0.75 → 1.5a* = 0.75 → a* = 0.5.
kernel void probe_fma_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        a = fma(a, -0.5f, 0.75f);
    }
    out[gid] = a;
}

// --- FMA-square baseline (matches LOG2 probe FMA parameters) ---
// Critical path: FMA(a, a, 1.0) = a²+1 → back to a. No SFU op.
// Subtract this from probe_log2_dep to isolate LOG2 latency.
// Diverges without clamping, so we square then immediately take sqrt via bit-cast trick.
// Actually: a²+1 grows. Clamp by scaling: a = sqrt(a²+1) ≡ hypot, but that's SQRT (SFU!).
// Instead: just measure, knowing a diverges after ~50 iters (Inf). The early iters give
// valid timing data once the dep chain is warm (values reach Inf then NaN after overflow,
// but the pipeline still ticks at L(FMA) per cycle — Inf/NaN don't stall AGX ALUs).
kernel void probe_fma_sq_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.8f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        a = fma(a, a, 1.0f);
    }
    out[gid] = a;
}

// --- FMAX single dep-chain ---
// Both FMA args depend on a and are computed in parallel.
// Critical path: a → FMA_x || FMA_y (parallel) → FMAX → a
// Per iter: max(L(FMA_x), L(FMA_y)) + L(FMAX) = L(FMA) + L(FMAX)
// DCE prevention: compiler cannot prove x > y or y > x for all a since crossover at a=1.
// Convergence: x = a*0.9+0.1, y = a*0.1+0.9. x>y iff a>1. fmax(x,y) → a*=1.0.
// Subtract probe_fma_dep to isolate FMAX latency.
kernel void probe_fmax_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            float x = fma(a, 0.9f, 0.1f);
            float y = fma(a, 0.1f, 0.9f);
            a = fmax(x, y);
        }
    }
    out[gid] = a;
}

// --- FMIN single dep-chain ---
// Identical structure to FMAX — measures FMIN latency via same subtraction method.
// fmin(x,y) where x=a*0.9+0.1, y=a*0.1+0.9 → converges to a*=1.0 (crossover).
kernel void probe_fmin_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 1.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            float x = fma(a, 0.9f, 0.1f);
            float y = fma(a, 0.1f, 0.9f);
            a = fmin(x, y);
        }
    }
    out[gid] = a;
}

// --- SELECT (FCMPSEL) single dep-chain ---
// Metal select(false_val, true_val, condition) — compiles to FCMPSEL on AGX ISA.
// Same FMA structure as FMAX: both args computed in parallel, then select.
// Critical path: L(FMA) + L(SELECT).
// Condition: x > y (same crossover at a=1 as FMAX probe — compiler cannot constant-fold).
// Subtract probe_fma_dep to isolate SELECT latency.
// If SELECT_lat ≈ FMAX_lat: SELECT and FMAX share the same hardware execution unit.
kernel void probe_select_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            float x = fma(a, 0.9f, 0.1f);
            float y = fma(a, 0.1f, 0.9f);
            a = select(y, x, x > y);  // select(false_val, true_val, cond) — picks x when x>y
        }
    }
    out[gid] = a;
}

// ============================================================
// H1b v2: 4-op UNROLLED dep-chain probes (better SNR).
//
// Problem with single-op-per-iter: loop overhead (~3 cycles/iter) ≈ op latency (2-6 cyc).
// Signal fraction at 100K iters ≈ 2-6% — within run-to-run noise floor.
//
// Fix: unroll 4 dep-chain ops per inner iteration.
//   Critical path per iter = 4 × (L_FMA + L_SFU) >> loop overhead.
//   At 4-cycle EXP2: 4 × 6.2 = 24.8 cycles >> 3-cycle loop overhead.
//   Net signal = 4× larger → SNR ≈ 20-30% of dispatch time at 100K iters.
//
// L(SFU op) = (T(probe_exp2_dep4) - T(probe_fma4_dep)) / (4 × 100K)
//
// ops_per_iter = 4 for all kernels below.
// ============================================================

// --- EXP2 dep chain × 4 per iter ---
// Critical path: 4 × [FMA(-0.5,0.75) → EXP2] in series.
// Range: a converges to a* ≈ 1.135 after ~50 iters; stays bounded forever after.
kernel void probe_exp2_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = exp2(fma(a, -0.5f, 0.75f));
            a = exp2(fma(a, -0.5f, 0.75f));
            a = exp2(fma(a, -0.5f, 0.75f));
            a = exp2(fma(a, -0.5f, 0.75f));
        }
    }
    out[gid] = a;
}

// --- LOG2 dep chain × 4 per iter ---
// Critical path: 4 × [FMA(a,a,1) → LOG2] in series.
kernel void probe_log2_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.8f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = log2(fma(a, a, 1.0f));
            a = log2(fma(a, a, 1.0f));
            a = log2(fma(a, a, 1.0f));
            a = log2(fma(a, a, 1.0f));
        }
    }
    out[gid] = a;
}

// --- FMA dep chain × 4 per iter (baseline for EXP2/LOG2/FMAX/SELECT subtraction) ---
// Same FMA params as EXP2 probe: fma(a, -0.5, 0.75). No SFU op.
// Critical path: 4 × FMA(-0.5, 0.75) = a converges to 0.5 in ~50 iters.
// CONTAMINATION WARNING: a* = 0.5 (exact power of 2); fma(0.5, -0.5, 0.75) = 0.5 exactly.
// AGX appears to fast-path the identical-output FMA at the fixed point, producing
// a 2-3× lower step time than the true 2.20 cyc latency. Use probe_fma4_osc_dep instead.
kernel void probe_fma4_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = fma(a, -0.5f, 0.75f);
            a = fma(a, -0.5f, 0.75f);
            a = fma(a, -0.5f, 0.75f);
            a = fma(a, -0.5f, 0.75f);
        }
    }
    out[gid] = a;
}

// --- FMA dep chain × 4 per iter (oscillating fixed point — clean baseline) ---
//
// fma(a, 0.7, 0.2): fixed point a* = 0.2 / (1-0.7) = 2/3 ≈ 0.6666...
// 2/3 is NOT exactly representable in IEEE 754 single precision.
// The iteration oscillates between adjacent floats:
//   0x3F2AAAAB (0.6666666865) → 0x3F2AAAAA (0.6666666269) → 0x3F2AAAAB → ...
// Because the result ALWAYS DIFFERS from the input by 1 ULP, AGX cannot apply
// any same-value forwarding optimization. The dep chain always has true latency.
//
// Use this as the FMA baseline for all dep-chain subtraction measurements.
kernel void probe_fma4_osc_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = fma(a, 0.7f, 0.2f);
            a = fma(a, 0.7f, 0.2f);
            a = fma(a, 0.7f, 0.2f);
            a = fma(a, 0.7f, 0.2f);
        }
    }
    out[gid] = a;
}

// --- FMA-sq dep chain × 4 per iter (baseline for LOG2 subtraction) ---
kernel void probe_fma_sq4_dep(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.8f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = fma(a, a, 1.0f);
            a = fma(a, a, 1.0f);
            a = fma(a, a, 1.0f);
            a = fma(a, a, 1.0f);
        }
    }
    out[gid] = a;
}

// --- FMAX dep chain × 4 per iter ---
// Each step: two FMAs in parallel, then FMAX. Critical path: 4 × [FMA || FMA → FMAX].
// Per step: L_FMA + L_FMAX. Total per iter: 4 × (L_FMA + L_FMAX).
kernel void probe_fmax_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = fmax(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
            a = fmax(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
            a = fmax(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
            a = fmax(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
        }
    }
    out[gid] = a;
}

// --- FMIN dep chain × 4 per iter ---
// Identical structure to probe_fmax_dep4; measures FMIN latency.
// Each step: two FMAs in parallel, then FMIN. Critical path: 4 × [FMA || FMA → FMIN].
// fmin(x,y) where x=a*0.9+0.1, y=a*0.1+0.9 → converges to a*=1.0 (crossover at a=1).
// Subtract probe_fma09_dep4 to isolate FMIN latency; compare with probe_fmax_dep4 to confirm
// FMIN and FMAX share the same hardware execution unit (same latency expected).
kernel void probe_fmin_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = fmin(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
            a = fmin(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
            a = fmin(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
            a = fmin(fma(a, 0.9f, 0.1f), fma(a, 0.1f, 0.9f));
        }
    }
    out[gid] = a;
}

// --- FMA-only dep chain × 4 per iter (matches critical FMA in fmax/select probes) ---
// Dep chain through fma(a, 0.9, 0.1) alone (the dominant FMA in the FMAX/SELECT structure).
// Subtract from probe_fmax_dep4 / probe_select_dep4 to isolate FMAX/SELECT latency.
kernel void probe_fma09_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = fma(a, 0.9f, 0.1f);
            a = fma(a, 0.9f, 0.1f);
            a = fma(a, 0.9f, 0.1f);
            a = fma(a, 0.9f, 0.1f);
        }
    }
    out[gid] = a;
}

// --- SELECT (FCMPSEL) dep chain × 4 per iter ---
// Same FMA structure as FMAX dep chain × 4. fcmp + select instead of fmax.
kernel void probe_select_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            float x, y;
            x = fma(a, 0.9f, 0.1f); y = fma(a, 0.1f, 0.9f); a = select(y, x, x > y);
            x = fma(a, 0.9f, 0.1f); y = fma(a, 0.1f, 0.9f); a = select(y, x, x > y);
            x = fma(a, 0.9f, 0.1f); y = fma(a, 0.1f, 0.9f); a = select(y, x, x > y);
            x = fma(a, 0.9f, 0.1f); y = fma(a, 0.1f, 0.9f); a = select(y, x, x > y);
        }
    }
    out[gid] = a;
}

// --- LOG2 clean dep chain × 4 per iter (v2 — stable input range) ---
//
// Problem with probe_log2_dep4 (v1): a = log2(fma(a,a,1)) converges to a=0 after ~2 outer
// iters. Then all 98K remaining iters compute log2(1.0)=0.0, a possibly special-cased path.
//
// Fix: use fma(a, 0.5, 1.5) → argument always in [1.0, 2.0] → log2 output always in [0,1].
//   Fixed point: a* = log2(a*/2 + 1.5) = 1.0 (since log2(0.5+1.5)=log2(2)=1). Stable.
//   |slope at a*=1|: 0.5/(2×ln2) = 0.361. Convergence in ~30 iters; then a=1.0 fixed point.
//   log2(1.5+0.5×1.0) = log2(2.0) = 1.0 — argument = 2.0 (NOT special case 1.0).
//   With gid × 1e-6 perturbation: argument ≈ 2.0 ± tiny epsilon. Normal range throughout.
//
// FMA baseline: probe_fma4_dep (same 4 × FMA structure, same latency).
// L(LOG2 dep step) = (T(log2_v2_dep4) - T(fma4_dep)) / (4 × 100K)
kernel void probe_log2_v2_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = log2(fma(a, 0.5f, 1.5f));
            a = log2(fma(a, 0.5f, 1.5f));
            a = log2(fma(a, 0.5f, 1.5f));
            a = log2(fma(a, 0.5f, 1.5f));
        }
    }
    out[gid] = a;
}

// --- SQRT clean dep chain × 4 per iter ---
// SQRT SFU TP was previously measured as 12.80 cyc from 8-chain probe.
// Single dep-chain (not SFU-saturating) measures true dep-chain LATENCY.
// Input: a = sqrt(a * 0.5f + 0.75f). Range: argument in [0.75, 1.25], output in [0.866, 1.118].
// Fixed point: a* = sqrt(a*/2 + 0.75). a*²= a*/2+0.75 → a*²-a*/2-0.75=0 → a*=(0.5+sqrt(3.25))/2 ≈ 1.151.
kernel void probe_sqrt_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.8f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = sqrt(fma(a, 0.5f, 0.75f));
            a = sqrt(fma(a, 0.5f, 0.75f));
            a = sqrt(fma(a, 0.5f, 0.75f));
            a = sqrt(fma(a, 0.5f, 0.75f));
        }
    }
    out[gid] = a;
}

// --- LOG2 clean dep chain × 4 per iter (v3 — arg ≉ power of 2) ---
//
// v2 fixed point: a*=1.0, arg=fma(1.0, 0.5, 1.5)=2.0 (exact power of 2).
// Hypothesis for 37% discrepancy vs Philip: AGX SFU has a slow path for args that
// are exact powers of 2 (or macOS 26 SDK polynomial is longer than Philip's SDK).
//
// v3 fix: use fma(a, 0.4, 1.5). Fixed point:
//   a* = log2(0.4×a* + 1.5)  →  numerically: a*≈0.894, arg=0.4×0.894+1.5≈1.858
//   1.858 is NOT a power of 2 (between 1.5 and 2.0). Convergence: slope≈0.311 at a*.
//   arg stays in [1.5, 2.0] throughout iteration; valid, non-degenerate range.
//
// If v3 ≈ Philip (4.31 cyc), v2 discrepancy was the arg=2.0 slow path.
// If v3 still shows ~5.9 cyc, the discrepancy is real (SDK-level algorithm change).
kernel void probe_log2_v3_dep4(
    device float* out           [[buffer(0)]],
    constant uint& n_iters      [[buffer(1)]],
    uint gid                    [[thread_position_in_grid]])
{
    float a = 0.5f + float(gid) * 1.0e-6f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a = log2(fma(a, 0.4f, 1.5f));
            a = log2(fma(a, 0.4f, 1.5f));
            a = log2(fma(a, 0.4f, 1.5f));
            a = log2(fma(a, 0.4f, 1.5f));
        }
    }
    out[gid] = a;
}
