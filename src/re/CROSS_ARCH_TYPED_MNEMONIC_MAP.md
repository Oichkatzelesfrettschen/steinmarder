# Cross-Architecture Typed Mnemonic Map
## SASS (Ada SM89) → Metal/AGX (M1 AGXG13G) → r600 TeraScale-2 (VLIW5)

**Purpose**: Use the steinmarder SASS mnemonic corpus + Apple neural typed matrix measurements
to inform what numeric types and patterns to use when writing GLSL/SPIR-V for r600.
Each entry shows what hardware exists, what the compiler does, measured cost, and the build decision.

**Data sources**:
- SASS: `results/mnemonic_census.csv`, `SM89_SASS_INSTRUCTION_REFERENCE.md`
- Metal/AGX: `results/typed_metal_suite_20260401_apple_sync/metal_type_surface_matrix.csv`
- Neural proxy: `results/typed_neural_frontier_expand_20260401/neural_timing_matrix.csv`
- r600: `results/x130e_terascale/LATENCY_PROBE_RESULTS.md`, `ISA_SLOT_ANALYSIS.md`, `NUMERIC_PACKING_RESEARCH.md`

---

## FP32 — The Universal Workhorse

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `FFMA` 3–4 cy latency, `FADD`/`FMUL` 3–4 cy; tensor: `HMMA.16816.F32` (66 cy/tile, 256 FMAs) |
| **Metal/AGX** | `float` — `native_surface`, 38.6 ns/elem (cpu_wall dispatch baseline) |
| **AGX AIR** | `@air.fma.f32` direct (confirmed, 64 instances in corpus) |
| **r600** | `MULADD_IEEE` / `ADD` / `MUL` in VLIW5 ALU slot; **1 cycle via PV forwarding** |
| **r600 slot** | x/y/z/w ALU, or t-slot if trans variant needed |
| **Build decision** | **Always preferred.** FP32 MULADD_IEEE fills VLIW5 slot efficiently. Never substitute a reduced-precision type hoping for HW speedup — r600 has no sub-FP32 ALU pipeline. |

---

## FP16 (Half Precision)

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `HADD2` 4.54 cy (paired); `HFMA2` paired; 2× FP16 ops per instruction vs FP32 |
| **Metal/AGX** | `half` — `native_surface`, 45.7 ns/elem; AIR uses `half`-typed arithmetic directly |
| **r600** | **NO NATIVE FP16 PIPELINE.** GLSL `mediump float` → compiled as FP32 on r600 |
| **r600 ISA** | No `MULADD_IEEE_F16` or `ADD_F16`; all FP lands in FP32 pipeline |
| **Build decision** | **Never use `mediump`/`lowp` expecting speedup.** Same cost as FP32 on r600. If you need packed FP16 in memory (bandwidth), store as `uvec2` and unpack with `unpackHalf2x16` — that gives BW reduction but ALU is still FP32. |
| **Cross-arch note** | SASS 2× FP16 throughput advantage **does not exist** on TeraScale-2. Shaders optimized for `HADD2` density need full rewrite for VLIW5 packing. |

---

## BF16 (Brain Float 16)

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `HFMA2.BF16_V2` — same throughput as FP16; `F2FP.BF16.F32.PACK_AB` convert |
| **Metal/AGX** | `bfloat` — `native_surface`, 47.3 ns/elem; AIR shows bfloat-typed arithmetic (compiler keeps BF16 through pipeline) |
| **r600** | **NOT SUPPORTED.** No BF16 ops in r600 ISA. Must truncate FP32 mantissa in software |
| **Emulation cost** | ~3 VLIW5 ops: `AND` (mask mantissa to 16 bits) + `BFI_INT` (clear low 16) + `MOV` |
| **Build decision** | **Avoid BF16 on r600.** Emulation costs 3× ALU budget for no accuracy gain. Use FP32. Only consider for memory bandwidth reduction in uniform buffers (pack BF16 pairs into a uint32). |

---

## FP64 (Double Precision) — r600 UNIQUE ADVANTAGE

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `DFMA` 36–57 instances confirmed; `DADD`/`DMUL` native; ~6–8× slower throughput vs FP32 |
| **Metal/AGX** | **UNSUPPORTED** — `'double' is not supported in Metal` compilation error |
| **r600** | **NATIVE HARDWARE FP64.** `ADD_64`, `MUL_64`, `FMA_64` in ALU clause; uses 2 consecutive VLIW5 slots for 64-bit operands |
| **r600 latency** | Confirmed via probe: FP64 ADD_64 1 cycle (same PV forwarding path as FP32, measured in LATENCY_PROBE_RESULTS.md) |
| **Build decision** | **r600 has FP64 hardware that Metal/M1 lacks.** Use `double` in GLSL for precision-critical code (matrix inversion, physics simulation, Dekker double-single). Cost: 2 VLIW5 slots per op, but still 1 cycle latency via PV chain. VLIW5 slot utilization goes from 4→2 effective per bundle — factor in scheduler efficiency. |
| **Dekker analysis** | See `NUMERIC_PACKING_RESEARCH.md`: Dekker double-single (3-op FP32) vs native FP64 (2-slot): Dekker wins if FP64 breaks VLIW5 packing on inner loop; FP64 wins for isolated high-precision ops. |

---

## Long Double / FP80 / FP128 — Platform Taxonomy

| Dimension | Detail |
|-----------|--------|
| **x86/x64** | `long double` = 80-bit extended (x87 fp80), 64-bit mantissa; hardware in x87 FPU |
| **AArch64 (Apple M-series)** | `long double` = `binary64` (same as `double`). `__LDBL_MANT_DIG__ = 53`. Confirmed: `fadd_dep_ld_aarch64` = 0.949 ns vs `fadd_dep_f64` = 0.949 ns (delta 0.001 ns). |
| **`__float128` / `_Float128`** | **ABSENT from Apple Clang on AArch64.** Compiler error: "not supported on this target". No 128-bit FP hardware; no integer-ALU softfp path. |
| **fp80 (x87)** | **x86-exclusive ISA.** No AArch64 SIMD equivalent. All extended precision on AArch64 routes through NEON FPU. |
| **r600** | Uses 2 VLIW5 slots for FP64 (= 64-bit mantissa, closest native analog to x87 fp80). No fp80/fp128 hardware. |
| **Build decision** | **Do not assume `long double` gives extra precision on AArch64/Apple.** It is exactly `binary64`. Code relying on fp80 for numerical stability (compensated summation, catastrophic-cancellation avoidance) must use a software double-double or explicit NEON vectorisation instead. |

---

## Double-Double (DD) and NEON Wide-Precision Fastpaths

The AArch64/Apple M-series reveals a set of fastpaths for software-emulated extended precision.
All extended-precision routes pass through the NEON FPU — there is no integer-ALU softfp penalty.

### Emulated-type fastpath ladder (measured on Apple M-series P-core, 2026-04-02)

| Level | Probe | ns/value | Precision | Mechanism |
|-------|-------|----------|-----------|-----------|
| 1 | `dd_twosum_dep` | 1.348 | 106-bit | Knuth 2-sum, 4 FP ops/step, scalar |
| 2 | `dd_twosum_fma_dep` | 1.347 | 106-bit | FMA 2-sum, same critical-path length |
| 3 | `f64x2_vadd_dep` | 0.473 | 53-bit | NEON 2×f64 per instruction |
| 4 | `f32x4_vfma_dep` | 0.237 | 24-bit | NEON 4×f32 FMA, 4.2× faster than f64 |
| **5** | **`f64x2_dd_twosum_dep`** | **0.520** | **106-bit** | **NEON vectorised double-double — BREAKTHROUGH** |

**Level 5**: 106-bit precision at **0.520 ns/value** — **1.91× more throughput than scalar float64** (0.949 ns).
Two independent DD pairs fit in one NEON Q-register (`float64x2_t hi, lo`).
The M-series OOO engine overlaps the 4-op dep chain to ~3.3 cycles (expected 4).
The NEON register is fp128-equivalent for this workload at lower cost than plain float64.

### FMA neutrality finding

`dd_twosum_fma_dep` (Level 2) is **identical** to scalar dep chain (Level 1): 1.347 vs 1.348 ns.
The critical path `hi→s→fma_inner→e→lo` is still 4 cycles regardless of FMA use.
FMA advantage only emerges with:
- Independent accumulator pairs (breaks the dep chain; reveals throughput)
- Veltkamp product splitting (~8 FP ops/step) where FMA eliminates one rounding step

### AGX FMA pipeline saturation (Metal, 2026-04-02)

With 32 FMA ops/iter × 100k GPU iters, 4 independent `probe_ffma_tput` chains gain
only **1.038×** over the dep-chain version (2.853 vs 2.960 ns/step). At 32 FP32 ops
per inner iteration, the AGX SIMD-group FP pipeline is already saturated.

Compare with the DD throughput probe (8 ops/iter): 4 independent chains gain **3.749×**
because 8 ops/iter uses only ~25% of AGX FP capacity.

**Throughput knee**: between 8 and 32 FP ops/inner-iter, additional independent chains
stop providing ILP gains on AGX. Target ≤16 ops/inner-iter when writing Metal compute
kernels that need to benefit from parallel dep-chain scheduling.

### Cross-architecture comparison

| Architecture | Extended precision path | Cost estimate |
|---|---|---|
| **AArch64/Apple M-series** | NEON vectorised DD (`float64x2_t`) | **0.520 ns / 106-bit value** |
| **AArch64/Apple M-series** | Scalar double-double (Knuth 2-sum) | 1.348 ns/value |
| **x86/x64** | Native `long double` (fp80 x87) | ~1–2 ns hardware (µarch-dependent) |
| **SASS Ada SM89** | `DFMA` double precision | ~3–4 ns/DFMA (38–57 cycle latency) |
| **r600 TeraScale-2** | Native FP64 (`ADD_64`, `FMA_64`) | 1 cycle PV chain, 2 VLIW5 slots |
| **r600 TeraScale-2** | Dekker double-single (FP32-pair) | **1.375 cycles/step** (VLIW5 packs parallel sub-ops) |

### r600 Dekker double-single (FP32-pair) — VLIW5 ISA analysis (2026-04-02)

r600 can implement double-single (FP32-pair, ~43-bit mantissa) in VLIW5 ALU:
```
# Knuth 2-sum in VLIW5 (4 ops, 4 slots):
ADD_IEEE  t.x, a, b          # s = a + b            (1 cycle PV)
ADD_IEEE  t.y, a, NEG(t.x)   # e1 = a - s           (wait for t.x)
ADD_IEEE  t.z, b, NEG(t.x)   # e2 = b - s           (wait for t.x, parallel with e1)
ADD_IEEE  out, t.y, t.z      # err = e1 + e2
```

**VLIW5 throughput finding (from ISA disassembly of Rusticl OpenCL kernel)**:
- Inner loop ISA: 8 Dekker steps compiled to **11 VLIW5 bundles** = **1.375 cycles/step**
- Scalar FP32 add dep chain: 8 steps to **8-9 VLIW5 bundles** = **1.0-1.125 cycles/step**
- Dekker overhead on VLIW5: **37.5%** (not the 4× expected from scalar hardware)
- This is because `e1` and `e2` from the same step are independent and pack into the same bundle,
  and later steps' `s=hi+delta` can coexist with the current step's `v=s-hi` in another slot.
- ISA sizes: fp32_add_dep = 68 dwords, f32_dekker_dep = 122 dwords (verified, no DCE)

Native FP64 `ADD_64` costs 2 VLIW5 slots at 1 cycle per step.

**Decision**: use native FP64 for isolated precision ops; Dekker FP32-pair only if native FP64
breaks inner-loop slot packing. See `NUMERIC_PACKING_RESEARCH.md`.

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `DFMA` native; software DD via `DFMA` chains for compensated sum |
| **Metal/AGX** | FP32 `float (hi, lo)` DD shaders measured: dep chain 3.968 ns/step, 4-chain tput 1.058 ns/step, 3.749× speedup. FMA probe (32 FMA ops/iter): dep=2.960 ns, tput=2.853 ns, speedup=1.038× (pipeline-saturated) |
| **AArch64 fastpath** | Level-5 NEON DD (`float64x2_t`): 0.520 ns/value, 1.91× scalar f64 throughput. 4-chain tput: 0.280 ns/step = 0.140 ns/value, 7.1× scalar f64 |
| **r600** | Native FP64 (`ADD_64`/`FMA_64`) OR Dekker FP32-pair (3–4 ALU ops) |
| **Build decision** | **r600**: prefer native FP64. **AArch64/Apple**: `float64x2_t` DD pairs give maximum extended-precision throughput. **Metal**: FP32 DD measured; 4-chain tput achieves 3.749× (approaching 4× theoretical). Use 4 independent `float (hi, lo)` pairs in MSL kernels to unlock peak AGX DD bandwidth. |

---

## INT32 / UINT32

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `IMAD` (INT32 MAD, base form); `IADD3`; `IMAD.WIDE` for INT64 result |
| **Metal/AGX** | `int`/`uint` — `native_surface`, 44.6 ns/elem; AIR shows direct int-typed arithmetic |
| **r600** | `MULLO_INT`/`MULHI_INT`/`ADD_INT`/`AND_INT`/`OR_INT` — all 1 cycle via PV forwarding |
| **r600 ISA** | Full 32-bit integer ALU in VLIW5 x/y/z/w slots |
| **Build decision** | **Use freely.** INT32 is the native integer width on r600. No sub-width benefit available. |

---

## INT16 / UINT16

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | No dedicated 16-bit int ops; uses `PRMT` (byte permute) + `IMAD` |
| **Metal/AGX** | `short`/`ushort` — **compiler_lowered** to i32; 47.6 ns/elem (same cost as INT32); AIR widens ops |
| **r600** | No dedicated 16-bit int ALU ops. GLSL `ivec2` of packed shorts compiled as INT32. 16-bit loads via TEX work for bandwidth. |
| **Build decision** | **Use INT32 for ALU; INT16 only for memory bandwidth.** Store packed `uint16_t` in buffers to halve BW, then unpack with `BFE_UINT` (1 cycle) before ALU. Same pattern as `PRMT`+`IMAD` on SASS. |

---

## INT8 / UINT8 — r600 UNIQUE CAPABILITY

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `IDP.4A.U8.U8` (packed 4×uint8 dot product, 4.53 cy); `PRMT` for extraction; `I2F.S8` convert |
| **Metal/AGX** | `char`/`uchar` — **compiler_lowered** to i32; 46–47 ns/elem (same as INT32); AIR widens to i32 |
| **r600** | **`UBYTE_FLT`** — unpacks 4×UINT8 from one INT32 into 4 FP32 values, **1 cycle**; `BFE_UINT` bit-field extract with 8-bit width, 1 cycle; `MULLO_INT` on results |
| **r600 cost** | 1 `UBYTE_FLT` + 4 `MULADD_IEEE` per INT8×4 = 5 VLIW5 slots, packs into ~2 VLIW5 bundles |
| **SASS comparison** | SASS `IDP.4A` does 4 multiplies + 1 add in 4.53 cy (tensor path); r600 needs 5 ALU ops ~2 cycles — competitive for non-tensor use |
| **Build decision** | **Use INT8 packing for lookup tables, palette indexing, neural activations < 256 values.** `UBYTE_FLT` is a 1-cycle unpacker that SASS/Metal don't expose so cheaply in shader code. Pack 4 INT8 values into one INT32 uniform. Throughput-limited by MULADD_IEEE after unpack. |
| **INT4 extension** | Use `BFE_UINT(src, bitoffset, 4)` — 1 cycle — for INT4 nibble extraction. Pack 8 INT4 per INT32. This maps to SASS `PRMT` nibble pattern. |

---

## TF32 (TensorFloat-32, 10-bit mantissa)

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `HMMA.1688.F32.TF32` — single instruction (Ada), 66.66 cy/tile; `HMMA.1684.F32.TF32` — 2-instruction Ampere form |
| **Metal/AGX** | `host_proxy` (software mantissa truncation); 45.2 ns/elem; no native TF32 in Metal/MSL |
| **pytorch_proxy_mps TF32** | 4.1–14.1 ms for 1024–2048 GEMM (proxy through FP32 MPS) |
| **r600** | **NO TF32 HARDWARE.** Emulation: `BFE_UINT` to zero mantissa bits 0–12 + `BFI_INT` reinsert = 3 ops per value |
| **Build decision** | **Skip TF32 on r600.** Pure FP32 (`MULADD_IEEE`) is cheaper and more accurate. TF32 emulation costs 3× ALU ops. Only relevant if you need SASS-compatible determinism (not a scenario here). |

---

## FP8 E4M3 / E5M2

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `F2FP.F16.E4M3.UNPACK_B` / `F2FP.SATFINITE.E4M3.F32.PACK_AB_MERGE_C` — Ada-only hardware |
| **Metal/AGX** | `host_proxy` (software minifloat); 58–60 ns/elem — **50% slower than FP32** due to software decode |
| **pytorch_proxy_mps FP8** | 2.2–13.7 ms for GEMM (similar to FP16 proxy through FP32 MPS) |
| **r600** | **NO FP8 HARDWARE.** Software decode: `BFE_UINT`(sign,1) + `BFE_UINT`(exp,4–5) + `BFE_UINT`(mant,3) + MULADD for reassembly = 7+ ops per value |
| **Build decision** | **Do not use FP8 on r600.** Software decode overhead kills any memory bandwidth benefit. FP8 only makes sense with Ada tensor core hardware. |

---

## MX Formats (MXFP8/6/4, MXINT8) — Blackwell-era

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | **No MX hardware on Ada.** Blackwell (SM100+) introduces native MX. Ada uses software emulation. |
| **Metal/AGX** | All `host_proxy`; MPS proxy (FP32): MXFP8 3.4 ns/elem, MXFP4 3.7 ns/elem — fast via batching |
| **pytorch_proxy_mps MXFP8** | 3.4–17.7 ms GEMM 1024–2048 — faster than CPU proxy (3.4× speedup for 2048) |
| **r600** | **NO MX HARDWARE.** MXFP8 decode = per-group scale read + FP8 decode per value = expensive |
| **Build decision** | **Skip all MX formats on r600.** Applicable only to Blackwell inference chips. Not in scope for TeraScale-2 compute. |

---

## INT64

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `IMAD.WIDE` — INT64 result via 2×INT32 carry chain (2.59 cy) |
| **Metal/AGX shift/XOR** | `long`/`ulong` — AIR emits genuine `shl i64`, `xor i64` (verified `llvm-dis`). **AGX backend lowers to i32 pairs.** Xorshift dep-chain: INT64 = **2.512×** INT32 (42.2 vs 16.8 ms/400 dispatches). Cross-word carry in `shl i64 val, k` is the bottleneck. |
| **Metal/AGX multiply** | `long`/`ulong` — INT64 mul dep-chain (LLVM-folded to 1 mul/iter): INT64 = **1.337×** INT32 (4.38 vs 3.28 ms/400 dispatches). Likely widened 32×32→64 multiplier path, not pure i32-pair lowering. |
| **r600** | Uses 2×INT32 VLIW5 ops for 64-bit carries. `MULLO_INT`+`MULHI_INT`+`ADD_INT`+`ADDC_UINT` = carry chain |
| **Build decision** | **Avoid INT64 shift/XOR on AGX and r600 in inner loops.** AGX costs 2.5× i32 for shift/XOR; r600 requires explicit 4-op carry chains. For 64-bit multiply on AGX, cost is only 1.34× — acceptable if needed. On r600, INT32 preferred for everything. |

---

## PRMT / Byte Permute

| Dimension | Detail |
|-----------|--------|
| **SASS (Ada)** | `PRMT` / `UPRMT` — 4-byte permute, used for INT8 packing/unpacking |
| **Metal/AGX** | No direct `prmt` in AIR; uses vector shuffle intrinsics or compiler-managed lane ops |
| **r600** | `BFE_UINT`/`BFI_INT` (bit field extract/insert) — 1 cycle; `AND_INT`+`LSHL_INT` for manual byte extraction |
| **Build decision** | r600's equivalent is `BFE_UINT(src, offset, width)` with constant offset/width → 1 cycle. For runtime-variable extraction, use `LSHR_INT` + `AND_INT` (2 cycles). GLSL `bitfieldExtract(value, offset, bits)` → lowers to `BFE_UINT` when offset/width are literal. |

---

## PRMT-to-BFE Mapping for r600

```
SASS               r600 VLIW5 op                  GLSL spelling           Cycles
────────────────────────────────────────────────────────────────────────────────
PRMT byte0         BFE_UINT(src, 0, 8)             bitfieldExtract(s,0,8)  1
PRMT byte1         BFE_UINT(src, 8, 8)             bitfieldExtract(s,8,8)  1
PRMT byte2         BFE_UINT(src,16, 8)             bitfieldExtract(s,16,8) 1
PRMT byte3         BFE_UINT(src,24, 8)             bitfieldExtract(s,24,8) 1
PRMT nibble_lo     BFE_UINT(src, 0, 4)             bitfieldExtract(s,0,4)  1
PRMT nibble_hi     BFE_UINT(src, 4, 4)             bitfieldExtract(s,4,4)  1
I2F.S8             UBYTE_FLT (for packed 4×U8)     unpackUnorm4x8()        1
                   or INT_TO_FLT (single U8)        intBitsToFloat(i)       1
F2F.F16.F32 pack   bitfieldInsert(hi,lo,0,16)      packHalf2x16()          1 (BFI_INT)
HADD2 (FP16 add)   ADD_IEEE (FP32 on r600)         (plain +)               1
```

---

## Clause-Level Patterns: Avoiding TEX Stalls in Blur

The effect2d blur shader is the slowest vkmark scene (424 FPS / 2.36ms).
It performs a 9-tap convolution: 9 texture fetches + 9 MULADD operations.

**TeraScale-2 clause model** (vs SASS scoreboard):
- SASS: TEX/LDST hide behind scoreboard — ALU continues while TEX in flight
- r600 VLIW5: TEX clause **suspends the ALU until TEX results return** — no hiding
- Implication: for N texture fetches, cost = `N × TEX_latency + N × MULADD`

**TEX latency** (currently unmeasured — highest priority gap):
- L1 hit: estimated 2–4 cycles (similar to LDS latency from related measurements)
- L2 miss to VRAM: estimated 50–200 cycles (DDR3 GART: ~7.3 cyc, but includes GART TLB overhead)
- APU advantage: GPU reads from system DDR3 (no separate VRAM copy cost)

**Blur optimization hypothesis** (requires TEX probe to confirm):
1. Combine all 9 taps into one TEX clause — minimize TEX→ALU transitions
2. After TEX clause returns all 9 samples, all 9 MULADD ops run in consecutive ALU clause
3. Current SFN may be splitting into smaller TEX/ALU alternations — needs ISA verification

**Action**: See `TEX_LATENCY_PROBE.frag` (to be written, Task #41).

---

## Cross-Architecture GEMM Throughput Reference

From `neural_timing_matrix.csv` (expand run, 2026-04-01), GEMM 1024×1024:

| Format | pytorch_cpu (ms) | pytorch_mps (ms) | Ratio (MPS/CPU) | r600 path |
|--------|-----------------|-----------------|-----------------|-----------|
| float32 | 2.85 | 4.24 | 1.49× slower | MULADD_IEEE (use) |
| float16 | 1161 | 5.60 | 207× faster | FP32 on r600 |
| bfloat16 | 1131 | 5.84 | 194× faster | FP32 on r600 |
| int8 | 60.0 | 8.43 | 7.1× faster | BFE_UINT+MULLO_INT |
| uint8 | 64.2 | 8.39 | 7.6× faster | UBYTE_FLT path |
| tf32 (proxy) | 3.66 | 4.10 | 1.12× slower | skip on r600 |
| fp8_e4m3 (proxy) | 3.36 | 2.24 | 1.50× faster | skip on r600 |
| mxfp8 (proxy) | 4.16 | 3.39 | 1.23× faster | skip on r600 |
| mxint8 (proxy) | 3.40 | 3.20 | 1.06× faster | skip on r600 |

**Key insight for r600**: The MPS FP16/BF16 speedup (200×) comes from Apple's matrix engine.
r600 has no matrix unit. For GEMM on r600: FP32 MULADD_IEEE is always optimal.
INT8 packing (UBYTE_FLT) is the only r600 format with meaningful packing benefit.

---

## Priority Action Items

### P1 — TEX Latency Probes (Task #41)
Write GLSL fragment shader probes:
```glsl
// latency_tex_l1_hit.frag: same texcoord repeated N times
// Measure: N × TEX_latency with warm L1
// Expected: 2–8 cycles/fetch (L1)

// latency_tex_l2_miss.frag: stride > 4KB between taps
// Measure: N × TEX_latency with cold L1
// Expected: 50–200 cycles/fetch (L2/VRAM)
```
Deploy via `piglit shader_runner` + `R600_DEBUG` ISA verification.
This data directly explains the blur scene bottleneck.

### P2 — Blur ISA Analysis
With TEX latency measured, count clause boundaries in blur ISA:
```bash
R600_DEBUG=ps vkmark --winsys headless -b effect2d:kernel=blur 2>&1 | \
  grep -E 'ALU|TEX|clause|VEC|TRANS' | head -100
```
Verify: are 9 TEX fetches in one clause, or split?

### P3 — SFN Scheduler for TEX Batching
If TEX fetches are split, find in `sfn_scheduler.cpp` where TEX clause gets cut.
Target: emit all 9 blur taps in one TEX clause, then all MADs in one ALU clause.

### P4 — INT8 Packing Probe
Demonstrate UBYTE_FLT speedup vs naive INT8 GLSL in a simple kernel:
```glsl
// pack 4 weights into one uniform uint
uint packed_weights = (w0 << 0) | (w1 << 8) | (w2 << 16) | (w3 << 24);
// unpack in shader via bitfieldExtract — becomes UBYTE_FLT or BFE_UINT
```

---

## See Also
- `BUILD_DECISION_MATRIX.md` — compiler-vs-manual pattern decisions
- `NUMERIC_PACKING_RESEARCH.md` — Dekker double-single, INT8 unpack
- `ISA_SLOT_ANALYSIS.md` — VLIW5 slot utilization histogram
- `SM89_SASS_INSTRUCTION_REFERENCE.md` — full SASS mnemonic corpus with latencies
- `APPLE_WIDE_PRECISION_FINDINGS.md` — canonical writeup for the DD/NEON fastpath ladder, FMA neutrality finding, and fp80/fp128/long_double taxonomy
- `results/typed_metal_suite_20260401_apple_sync/metal_type_surface_matrix.csv` — AGX type classification
- `results/typed_neural_frontier_expand_20260401/neural_timing_matrix.csv` — proxy GEMM timings
