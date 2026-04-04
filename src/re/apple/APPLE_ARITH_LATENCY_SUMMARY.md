# Apple Silicon Arithmetic Latency Reference Card

**Platform**: Apple M1, AArch64 + Metal AGX GPU  
**Measured**: 2026-04-02/03 via steinmarder dep-chain probe harnesses (B4/B5/C3 updated 2026-04-03)  
**RE cross-reference**: [`AGX_RE_SOURCES.md`](AGX_RE_SOURCES.md) (dougallj/applegpu, philipturner/metal-benchmarks)  
**Full data**: [`APPLE_WIDE_PRECISION_FINDINGS.md`](APPLE_WIDE_PRECISION_FINDINGS.md) · [`APPLE_CROSS_DOMAIN_PERF_ATLAS.md`](APPLE_CROSS_DOMAIN_PERF_ATLAS.md)  
**CSV**: [`tables/table_apple_type_timings.csv`](tables/table_apple_type_timings.csv)

---

## CPU (AArch64 / NEON, dep-chain latency)

| Op | Type | ns/op | ~cycles @3.2GHz | Notes |
|----|------|-------|-----------------|-------|
| int ADD | u8/u16/u32/u64 | **0.315–0.333** | ~1 | Fastest ALU primitive; 3× faster than FP add |
| CLZ | u64 | **0.645** | ~2 | Count-leading-zeros; sub-cycle effective |
| nibble unpack | u4/i4 | **0.316** | ~1 | add+and / add+sbfx patterns |
| load/store chain | u64 | **0.500** | ~1.6 | L1-hit load-use forwarding |
| f32 fadd | f32 | **0.945** | ~3 | CPU FP baseline |
| f64 fadd | f64 | **0.945** | ~3 | = f32; M-series FPU same speed for both |
| f16 fadd (FEAT_FP16) | f16 | **0.945–0.980** | ~3 | Zero latency penalty vs f32 (scalar) |
| long double | f64 alias | **0.949** | ~3 | = binary64 on AArch64/Apple |
| int MUL | u32/u64 | **0.944–0.979** | ~3 | = FP add tier (dedicated multiply pipeline) |
| madd / msub | u64 | **0.958** | ~3 | Fused multiply-add (integer) |
| umulh / smull | u64/i32 | **0.944–0.964** | ~3 | Wide multiply |
| CRC32 (FEAT_CRC32) | u32 | **0.998** | ~3.2 | Hardware CRC instruction |
| f16 fmul (scalar) | f16 | **1.306** | ~4.2 | Slightly slower than f16 fadd |
| DD two-sum dep | f64×2 | **1.348** | ~4.3 | Knuth two-sum scalar; 106-bit |
| atomic add relaxed | u64 | **1.912** | ~6.1 | Serial uncontended atomic dep-chain |
| shuffle cross-lane | u64 | **1.345** | ~4.3 | NEON across-lane operation |
| fmadd dep-chain | f64 | **1.260** | ~4.0 | FMA 1.33× slower than fadd (deeper pipeline) |
| sqrt (FSQRT) | f64 | **4.870** | ~15.6 | Hardware; 5.15× fadd |
| sin/cos | f64 | **20.30** | ~65 | libm software |
| log | f64 | **14.497** | ~46 | libm software (~15 cycles) |
| exp | f64 | **14.879** | ~48 | libm software (~15 cycles) |

## CPU (NEON SIMD, dep-chain and throughput)

| Op | Mode | ns/op unit | ns/value | Notes |
|----|------|------------|---------|-------|
| f32x4 FMA (NEON) | dep-chain | 0.951/vec | **0.237**/f32 | 4-wide FMA; 4.2× faster than scalar f64 |
| f64x2 vadd | dep-chain | 0.964/vec | **0.482**/f64 | 2× f64 in one NEON op |
| f64x2 DD two-sum | dep-chain | 1.044/vec | **0.522**/DD-val | 106-bit precision at 0.52 ns/value |
| f64x2 DD throughput | independent | 0.289/vec | **0.144**/DD-val | **FASTEST**: 106-bit at 0.144 ns/value (6.6× scalar f64!) |
| f64x2 DD FMA throughput | independent | 0.290/vec | **0.145**/DD-val | FMA neutral in throughput mode |
| 4-pair scalar DD throughput | independent | 0.530/4-pairs | **0.132**/pair | OOO overlaps 4 Knuth two-sum chains |
| fmla f16x8 | dep-chain | 1.264/vec | 0.158/f16 | NEON 8-wide fp16 FMA |

## Metal AGX GPU (dep-chain latency, 30 dispatches × 100K inner iters)

| Op | Type | ns/op | ~GPU cycles @1.3GHz | Notes |
|----|------|-------|---------------------|-------|
| f32 fmul | dep-chain | **1.347** | ~1.75 | Fastest GPU arithmetic |
| f32 fadd | dep-chain | **1.695** | ~2.20 | philip: 2.20 cycles ✓ exact |
| f32 fma | dep-chain | **1.993** | ~2.59 | Slower than fadd; half-rate issue unit |
| f16 fadd | dep-chain | **2.101** | ~2.73 | fp16 widening: +0.4 ns over fp32 |
| f16 fmul | dep-chain | **2.018** | ~2.62 | All fp16 ops converge ~2.0 ns |
| f16 fma | dep-chain | **2.006** | ~2.61 | Widening masks op-type differences |
| DS-multiply step | dep-chain | **2.072** | ~2.69 | FMA-bottlenecked (not 3 serial ops) |
| int32 imul | dep-chain | **3.376** | ~4.39 | Separate int multiply pipeline; philip: ~4.02 |
| DD genuine (float32×2) | dep-chain | **3.348** | ~4.35 | fadd-chain bottleneck; 2.49× CPU DD |
| Veltkamp split step | dep-chain | **1.468**/FP-op | ~1.91 | 7 FP ops/step, each ~fadd; GPU 2.16× slower than CPU |
| RECIP (reciprocal) | dep-chain | **8.605** | ~11 | Special unit; ~4× fadd; `air.arcp.f32` hardware approx |
| RSQRT fast (`rsqrt(x)`) | dep-chain | **9.405** | ~12 | `air.fast_rsqrt.f32` hardware approx; T=L=12 cyc (NON-pipelineable on M1) |
| RSQRT precise (`metal::precise::rsqrt`) | dep-chain | **29.2 ns** | ~38 | `air.rsqrt.f32` — Newton-Raphson multi-step firmware; 3.1× slower than fast |

## Metal AGX GPU (throughput, 8 independent accumulators)

| Op | ns/8-op-slot | ILP gain over dep | ~GPU TP cycles |
|----|-------------|-------------------|----------------|
| f32 fmul | **0.751** | 1.79× | ~1.00/op |
| f32 fadd | **1.090** | 1.55× | ~1.42/op |
| f32 fma | **1.764** | 1.13× | ~2.29/op ← register pressure at 8 acc |
| f16 fadd (8-acc) | **1.228** | 1.71× (vs dep) | ~1.57/op | 12.7% slower than f32 fadd TP |
| f16 fmul (8-acc) | **0.836** | 2.41× (vs dep) | ~1.07/op | 11.3% slower than f32 fmul TP |
| f16 fma (8-acc) | **0.736** | 2.72× (vs dep) | ~0.94/op | Sub-cycle; suspect fast-math or measurement artifact |
| RECIP (8-acc TP) | **6.269** | 1.37× (vs dep) | ~8/op | Needs wider chain to saturate; philip: 6-cycle TP (may be M2/M3) |
| RSQRT fast (8-acc TP) | **9.393** | 1.00× (= dep!) | ~12/op | `air.fast_rsqrt.f32`; T=L=12 cyc — **non-pipelineable** on M1 (structural hazard) |
| RSQRT precise (16-op ILP) | **29.2 ns** | ~1.0× | ~38 cyc | `air.rsqrt.f32`; iter-derived inputs; register pressure likely inflates slightly |

**Register pressure note**: 16 accumulators causes register spill on M1 AGX — both f32 fadd (3.184 ns, 4.07 cycles) and f32 fma (4.851 ns, 6.19 cycles) degrade sharply at 16-acc. 8 accumulators is the register-optimal point. FMA throughput anomaly (only 1.13× ILP gain at 8-acc) is register pressure, not a half-rate FMA unit — confirmed by AIR dump showing 16 independent `@air.fma.f32` calls at 16-acc, and by equal degradation of fadd at 16-acc.

## Metal AGX GPU (memory and synchronization)

| Op | ns | ~GPU cycles | Notes |
|----|-----|-------------|-------|
| Warm L1 global load (sequential) | **10.07** | ~13 | Per-thread, no contention; 8KB L1 fits; pipelined sequential |
| L1 dep-chain pointer-chase (4KB) | **49.33** | ~64 | Serialized dep-chain through `device uint*`; 5× sequential L1 — AGX wait-instruction overhead |
| L2 dep-chain pointer-chase (16KB) | **70.87** | ~92 | 16KB > L1 (8KB), step 8KB; clear L1→L2 boundary |
| L2/L3 boundary pointer-chase (64KB) | **83.81** | ~109 | Approaching L2/core capacity (96KB = 768KB/8 cores) |
| L3 dep-chain pointer-chase (256KB) | **85.22** | ~111 | >96KB L2/core; plateau ~110 cyc (64KB–512KB all same tier) |
| LDS store→load (volatile) | **21.8**/access | ~28 | Per access; L1 is FASTER than LDS! |
| threadgroup_barrier | **25.44** | ~33 | Barrier-only isolation |
| simdgroup_matmul 8×8 (SGMM f32) | **24.235** | ~31.5 | 512 MADs/call; 0.024 ns/FP-op effective; 71× scalar fadd; T=L=~32 cyc (NON-pipelineable) |
| simd_prefix_inclusive_sum | **23.720** | ~30.8 | 2.32× single shuffle; log2(32) stages in 2.15× naive serial; faster than simd_sum! |
| simd_prefix_exclusive_sum | **22.275** | ~29.0 | 1.73 ns cheaper than inclusive (no final self-element add) |
| simd_broadcast (lane 0 → all) | **10.690** | ~13.9 | ≈ L1 global load! All permutations cost same |
| simd_shuffle_down (delta=1) | **10.162** | ~13.2 | Same tier as broadcast and L1 load |
| simd_shuffle_xor (mask=1) | **10.215** | ~13.3 | Adjacent-lane butterfly step = shuffle_down latency |
| simdgroup_sum (32-lane) | **28.31** | ~36.8 | 2.77× single shuffle; reduction broadcast step adds ~6 cyc vs prefix scan |
| Threadgroup atomic_fetch_add | **89.6** | ~115 | Serial dep-chain; 47× CPU atomic |
| Global atomic_fetch_add | **143.8** | ~184 | 60% slower than TG atomic |

---

## Cross-domain comparison at a glance

| Op | CPU (ns) | GPU dep (ns) | GPU tp (ns) | Winner |
|----|---------|-------------|------------|--------|
| FP32 fadd | 0.945 | 1.695 | 1.090 | **CPU** (dep) or **GPU tp** @1024+ threads |
| FP32 fmul | 0.945 | 1.347 | 0.751 | **GPU tp** beats CPU scalar for independent work |
| FP32 fma | 1.260 | 1.993 | 1.764 | **CPU** (FMA slow on both; CPU less penalized) |
| FP16 fadd | 0.945 | 2.101 | TBD | **CPU** 2.2× faster for dep chains |
| INT32 mul | 0.944 | 3.376 | — | **CPU** 3.57× faster |
| INT ADD | 0.319 | — | — | **CPU** only; 3× faster than CPU FP add |
| DD two-sum | 1.348 | 3.348 | — | **CPU** 2.49× faster for serial DD |
| NEON DD tp | 0.144/val | — | — | **CPU NEON** only; fastest 106-bit precision |
| Atomic add | 1.912 | 89.6 (TG) | — | **CPU** 47× faster |

**Rule of thumb**: GPU wins on throughput across 1024+ independent threads. CPU wins on every single serial dep chain. NEON DD throughput is the single most outlying result: 106-bit precision cheaper than scalar f64 by 6.6×.

---

## GPU clock estimate

| Method | Implied clock |
|--------|--------------|
| fmul throughput (1 cycle) | 1/0.751 ns = **1.331 GHz** |
| fadd dep-chain (2.20 cycles, philipturner) | 2.20/1.695 = **1.298 GHz** |
| imul dep-chain (4.02 cycles, philipturner) | 4.02/3.376 = **1.191 GHz** |
| **Best estimate** | **~1.28–1.33 GHz** (dynamic clock range) |

---

## Key architectural facts (RE-derived)

| Fact | Source |
|------|--------|
| SIMD-group = 32 threads | dougallj/applegpu |
| 128 GPRs/SIMD-group (32-bit) = 256 × 16-bit regs | dougallj + Rosenzweig |
| Hardware-scheduled (no NOP padding) | Rosenzweig Part I |
| More 16-bit ALUs than 32-bit ALUs | Rosenzweig Part I |
| Free 16↔32-bit conversion on instruction operands | Rosenzweig Part I |
| Cache: 8KB L1 / 60KB LDS / 768KB L2/core / 8MB L3 | philipturner |
| RECIP32: 6-cycle TP (philip); RSQRT fast: T=L=12 cyc on M1 (non-pipelineable — philip's 8-cycle TP may be M2/M3) | philipturner + steinmarder |
| `air.rsqrt.f32` (precise) ≠ `air.fast_rsqrt.f32`: `metal::precise::rsqrt` → multi-step NR firmware (~31 cyc); plain `rsqrt(x)` → hardware approx (~12 cyc) | steinmarder B4 |
| AGX wait-instruction overhead: dep-chain pointer-chase L1=64 cyc vs sequential L1=13 cyc; hierarchy: L1≤8KB/64cyc, L2≤96KB/92-109cyc, L3/sys≥64KB/110cyc | steinmarder C3 |
| M1 lacks hardware image atomics | Asahi blog |
| AGX is firmware-driven (ARM64 ASC coprocessor) | Asahi blog |
