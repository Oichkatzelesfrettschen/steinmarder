# Apple AGX M1 — Exhaustive Operation Compendium

**Purpose**: Single reference absorbing ALL known latency/throughput data for the M1 GPU  
**Sources**: philipturner/metal-benchmarks (P), steinmarder dep-chain probes (S), dougallj/applegpu ISA (D)  
**Clock**: ~1.28–1.40 GHz (nominal 1.28–1.33 GHz; boost observed up to ~1.40 GHz; calibrate per batch via FMA dep-chain)  
**Last updated**: 2026-04-03

---

## Methodology note — Philip vs. steinmarder numbers

Philip Turner's "adjusted latency" is **pipeline depth / half-throughput** measured at 2 simds/core ILP=1.
For simple ops (FADD, FMUL) pipeline depth = true dep-chain latency → both agree.
For SFU ops (RECIP, RSQRT) the SFU pipeline depth < true round-trip latency: Philip ≈ throughput,
steinmarder dep-chain ≈ structural stall time. Neither is wrong; they measure different things.

| Metric | Philip | steinmarder |
|--------|--------|-------------|
| What it measures | Pipeline depth (≈ throughput) | True serialized dep-chain stall |
| Good for | Peak-throughput modeling | Bottleneck analysis for serial code |
| FADD32 | 2.20 cyc | 2.20 cyc ✓ agree |
| RECIP32 | 6.50 cyc | 11 cyc (dep-chain stall is longer) |
| RSQRT32 | 8.99 cyc | 12 cyc (same reason) |

---

## FP32 Arithmetic

| Operation | P TP (cyc) | P adj_lat (cyc) | S dep-chain (ns) | S dep-chain (cyc) | S TP (ns) | S TP (cyc) | Notes |
|-----------|-----------|-----------------|------------------|-------------------|-----------|------------|-------|
| FADD32    | 1–2       | 2.20            | **1.695**        | **2.20** ✓        | 1.090     | 1.42       | Agree perfectly |
| FMUL32    | 1–2       | 2.21            | **1.347**        | **1.75**          | 0.751     | 0.97       | S dep slightly lower — fmul has shorter pipeline? |
| FFMA32    | 1–2       | 2.21            | **1.993**        | **2.59**          | 1.764     | 2.29       | S TP anomaly (reg pressure at 8 acc); P=1 cyc TP |
| FMAX32    | 1         | 4.74            | step=3.148 ns    | step=**4.31** cyc | —         | —          | H1b (2026-04-03): step=(FMA∥FMA)→FMAX. Pure FMAX ≈ (4.31−2.20−1.0)=**~1 cyc** (ALU tier). P adj_lat=4.74 may include more pipeline stages. AIR: `air.fast_fmax.f32`; AGX likely implements as FCMPSEL |
| FMIN32    | 1         | 4.74            | step=3.0–3.1 ns  | step=**4.31 cyc** ✓ | —      | —          | H1d (2026-04-03): interleaved vs probe_fmax_dep4. Δ(FMAX−FMIN)=0±0.1 cyc (5 trials). Same dep-chain step as FMAX → same hardware unit (FCMPSEL). Pure FMIN adj_lat same as FMAX ~1–2 cyc. P adj_lat=4.74 (reg-file). |
| FCMPSEL32 | 1         | 4.74            | step=3.047 ns    | step=**4.17** cyc | —         | —          | H1b (2026-04-03): Metal `select(y,x,x>y)` → AIR `fcmp+select i1`. Step=(FMA∥FMA)→FCMPSEL. Pure FCMPSEL ≈ (4.17−2.20−1.0)=**~1 cyc**. FMAX−SELECT=0.14 cyc (noise); same HW unit |
| CONVERT(F→I32) | 4    | 3.66            | —                | — (**gap**)       | —         | —          | rounding/convert |
| RINT32    | 4         | 3.66            | —                | — (**gap**)       | —         | —          |
| FRACT32   | 4         | ~5 (seq)        | —                | — (**gap**)       | —         | —          | TRUNC+FADD sequence |
| FSQRT32   | 8         | 8.57–11.13      | **9.846** (8-ch); **8.764** (1-ch) | 8-ch: **12.80**; 1-ch dep: **10.08** | —         | —          | S (2026-04-03): 8-chain TP=8 confirmed; H1b single dep-chain: 10.08 cyc adj_lat (within Philip's 8.57–11.13 range) ✓ |
| RECIP32 (fast) | 6    | 6.50            | **8.605**        | **11.18**         | 6.269     | 8.15       | S dep > P adj: SFU stall; P TP=6 confirmed by S TP=8 (discrepancy) |
| RSQRT32 (fast) | 8    | 8.99            | **9.405**        | **12.23**         | 9.393     | 12.21      | T=L=12 cyc — **non-pipelineable on M1**; P TP=8 on M2? |
| RSQRT32 precise | 8  | 8.99 (P same!)  | **29.2**         | **37.97**         | —         | —          | AIR: `air.rsqrt.f32` = Newton-Raphson multi-step. P may have older SDK that compiled precise→fast |
| FDIV32    | 6.01      | 7.62–8.90       | —                | — (**gap**)       | —         | —          | P: = RECIP32 + FMUL32 in sequence |
| EXP2_32   | 4         | 4.31            | **4.932** (1-ch) | **4.56 adj_lat** ✓ | —        | —          | H1b (2026-04-03): clean FMA→EXP2 dep-chain. adj_lat=(6.76−2.20)=4.56 cyc vs Philip 4.31 (Δ=6%, within noise). AIR: `air.fast_exp2.f32`. Converges to a*≈1.135, arg≈0.18 (non-trivial) |
| LOG2_32   | 4         | 4.31            | **4.276** (8-ch); v3: **5.784** ns (SFU-SFU delta) | 8-ch: **5.56**; v3 adj_lat: **~4.38** ✓ | — | — | H1b v2 (arg=2.0 exact): 5.90 cyc (SLOW PATH — power-of-2 special case). H1b v3 (arg≈1.858): 4.38 cyc (Δ=+1.6% vs Philip 4.31) ✓. Measured via SFU-SFU delta vs EXP2 (loop overhead cancels). Interleaved, 200 iters, 3 trials |
| EXPE_32   | 4         | 7.61–7.66       | —                | — (**gap**)       | —         | —          | Software: base-2 change + EXP2 |
| LOGE_32   | 4         | 7.61–7.66       | —                | — (**gap**)       | —         | —          |
| SIN32     | 14.28     | 23.04–27.35     | —                | — (**gap**)       | —         | —          | Two hardware phases: SIN_PT1+SIN_PT2 |
| COS32     | 14.28     | 23.04–27.35     | —                | — (**gap**)       | —         | —          |
| Precise RECIP32 | 10.46 | 24.99–28.48  | —                | — (**gap**)       | —         | —          | Multi-step NR; ~25 cyc P |
| Precise SQRT32 | 15.03 | 34.27–37.12   | —                | — (**gap**)       | —         | —          |
| Precise SIN32  | 24.39 | 224–226        | —                | — (**gap**)       | —         | —          | Extremely slow! |

---

## FP16 Arithmetic

| Operation | P TP (cyc) | P adj_lat (cyc) | S dep-chain (ns) | S dep-chain (cyc) | S TP (ns) | S TP (cyc) | Notes |
|-----------|-----------|-----------------|------------------|-------------------|-----------|------------|-------|
| FADD16    | 1         | 2.16            | **2.101**        | **2.73**          | 1.228     | 1.60       | S ≈ 2.73 vs P 2.16 — fp16 dep-chain heavier |
| FMUL16    | 1         | 2.17            | **2.018**        | **2.62**          | 0.836     | 1.09       |
| FFMA16    | 1         | 2.18            | **2.006**        | **2.61**          | 0.736     | 0.96       | S TP sub-cycle suspect; agree on dep-chain |
| FCMPSEL16 | 1         | 2.17            | —                | — (**gap**)       | —         | —          |
| RECIP16   | 6         | 6.50            | —                | — (**gap**)       | —         | —          |
| RSQRT16   | 8         | 8.61            | —                | — (**gap**)       | —         | —          |
| FDIV16    | 6.01      | 8.58–9.36       | —                | — (**gap**)       | —         | —          |
| FSQRT16   | 8         | 9.56–10.74      | —                | — (**gap**)       | —         | —          |
| EXP2_16   | 4         | 4.62            | —                | — (**gap**)       | —         | —          |
| LOG2_16   | 4         | 4.62            | —                | — (**gap**)       | —         | —          |
| SIN16     | 13.56     | 23.78–27.90     | —                | — (**gap**)       | —         | —          |

---

## INT32 Arithmetic

| Operation | P TP (cyc) | P adj_lat (cyc) | S dep-chain (ns) | S dep-chain (cyc) | Notes |
|-----------|-----------|-----------------|------------------|-------------------|-------|
| IADD32    | 1         | 2.21            | —                | — (proxy: same as FADD32) | Shares FP32/INT pipeline |
| IMUL32    | 4         | 4.02            | **3.376**        | **4.39** ✓        | Agree |
| IMAD32    | 4         | 4.02            | —                | —                 | = IMUL32 + ADD |
| IMULHI32  | 8.01      | 9.83            | ~5.4 ns (fwd)    | **~6 cyc (fwd)** ✓ | H1c: forwarding adj_lat 6 cyc vs P's 9.83 cyc reg-file lat. See 64-bit section + note. |
| BITWISE32 | 1.06      | —               | —                | — (**gap**)       |
| BITREV32  | 4         | 3.62            | —                | — (**gap**)       |
| POPCOUNT32| 4         | 3.62            | —                | — (**gap**)       |
| CLZ32     | 4.05      | 7–9             | —                | — (**gap**)       | (CPU CLZ=2 cyc; GPU different unit) |
| LSHIFT32  | 4.01      | 5.56–6.74       | —                | — (**gap**)       |
| RSHIFT32  | 7.89      | 10.80–12.19     | —                | — (**gap**)       |
| IMAX32/IMIN32 | 1     | 4.74            | —                | — (**gap**)       |
| ICMPSEL32 | 1         | 4.74            | —                | — (**gap**)       |

---

## INT16 Arithmetic

| Operation | P TP (cyc) | P adj_lat (cyc) | S dep-chain (ns) | S dep-chain (cyc) | Notes |
|-----------|-----------|-----------------|------------------|-------------------|-------|
| IADD16    | 1         | 2.17            | —                | — (**gap**)       | Same pipeline as IADD32 (16-bit mode) |
| IMUL16    | 4         | 3.69            | —                | — (**gap**)       |
| IMAD16    | 4         | 3.68            | —                | — (**gap**)       |
| IMUL(16x16=32) | 4     | 3.86            | —                | — (**gap**)       |
| ICMPSEL16 | 1         | 2.17            | —                | — (**gap**)       |
| BITREV16  | 4         | ~5.76–6.76      | —                | — (**gap**)       |

---

## 64-bit Operations

| Operation | P TP (cyc) | P adj_lat (cyc) | S dep-chain (ns) | S dep-chain (cyc) | Notes |
|-----------|-----------|-----------------|------------------|-------------------|-------|
| IMUL(32x32=64) | 8.01 | 9.84           | —                | **~7.2 cyc** ⚠️   | S H1c (2026-04-03): v2 probe (native uint a,b) → dep-chain step ≈ 7.2 cyc, Δ vs IMUL32 ≈ +2.1 cyc (clock-independent). Implies critical path through hi32 has adj_lat ≈ 6 cyc. See analysis note below. |
| IMULHI32       | 8.01 | 9.83           | —                | **~6 cyc (forwarding)** | S H1c: v2 + imulhi32_dep4 indistinguishable (Δ=-0.08±0.38 cyc). Both: step = 1_pre + 6_adj = 7 cyc. P's 9.83 cyc = register-file latency; our 6 cyc = forwarding latency. 3.8-cycle bypass window. |
| IMUL(64x64=64) | 16.06| 15.18–21.72    | —                | **17.7 cyc** ✓    | S H1c: v1 probe (ulong acc, TRUNC→ZEXT path) = 17.7 cyc. Δ vs IMUL32 = +12.5 cyc → 2_pre + 16_IMUL64. Confirmed TRUNC→ZEXT prevents widening-multiply recognition. |
| IADD(32+32=64) | 3.07 | 6.89–7.86      | —                | — (**gap**)       | P: emulated = IADD32 + ICMPSEL32 + IMV |
| IADD(64+64=64) | 4.68 | 10.01–11.62    | —                | — (**gap**)       |
| xorshift i64  | —    | —               | **16.328 ns/xor** | **2.512× int32**  | S: AIR i64 shift/XOR → 2 i32 ops each |
| CONVERT(F→I64) | 7.11 | 10.30–12.67   | —                | — (**gap**)       |

_Note: `IMUL(32x32=64)` hardware path — H1c dep-chain findings (2026-04-03):_
- _v1 probe (`ulong acc` → TRUNC→ZEXT path): 17.7 cyc = 2_pre + 16 → IMUL64. `ulong(uint(ulong_acc))` → AGX backend picks IMUL64, not widening._
- _v2 probe (native `uint a, b`; `ulong r = ulong(a)*ulong(b|1)`): step ≈ 7.2 cyc. Dep chain through hi32 (b=uint(r>>32))._
- _imulhi32_dep4 probe (hi32 → `a`, no OR pre-cost): step ≈ 7 cyc. **Indistinguishable from v2** (Δ = −0.08 ± 0.38 cyc)._
- _Both v2 and imulhi32 give Δ ≈ +2.1 cyc vs IMUL32 (7 run avg). → **forwarding adj_lat for hi32 path = 6 cycles**._
- _P's IMULHI32 adj_lat = 9.83 cycles = register-file latency. Our 6 cycles = AGX pipeline-bypass / forwarding path. 3.8-cycle forwarding advantage for the widening multiply unit (vs IMUL32 which has zero forwarding advantage)._

---

## Extended / Precision / Compound Operations (GPU)

| Operation | Source | dep-chain ns | dep-chain cyc | Notes |
|-----------|--------|-------------|---------------|-------|
| DS two-sum (f32×2) | S | **3.348** | **4.35** | GPU fadd-chain bottleneck; 2.49× CPU DD |
| DS-mul step (f32×2) | S | **2.072/fma** | **2.69** | FMA-bottlenecked (not 3 serial ops) |
| Veltkamp split step | S | **1.468/FP-op** | **1.91** | 7 FP ops/step; GPU 2.16× slower than CPU |
| simdgroup_matmul 8×8 f32 (SGMM) | S | **24.235** | **31.5** | 512 MADs; T=L≈32 cyc (NON-pipelineable); 0.024 ns/FP-op effective |

---

## Memory Hierarchy

| Access Type | Source | ns/op | cyc @1.3GHz | Notes |
|-------------|--------|-------|-------------|-------|
| L1 sequential load (8KB, volatile ptr, dep-chain) | S | **10.07** | **13.1** | Per-thread, no contention; actually pointer-chase but step < L1 |
| L1 pointer-chase dep-chain (4KB, step=20B) | S | **49.33** | **64.1** | Wait-instruction overhead; 5× sequential! |
| L2 pointer-chase (16KB) | S | **70.87** | **92.1** | Clear L1→L2 jump from 64 cyc |
| L2/L3 boundary (64KB ≈ 96KB L2/core) | S | **83.81** | **108.9** | |
| L3/system pointer-chase (256KB, 512KB) | S | **85.22** | **110.8** | Plateau: 64KB–512KB all same tier |
| Texture sample L1-warm (1×1 RGBA32F, bilinear, dep-chain) | S | **102 ± 5** | **~112 cyc ⚠️** | C4 (2026-04-03): clock uncertain (GPU throttles during 9s run); range 92–143 cyc @ 900–1400 MHz; FMA in dep chain fast-pathed (fixed pt 0.5) → step ≈ L_TEX. ~50× FMA, ~8× L1 ptr-chase, ~4× L2. Bilinear filter included. |
| LDS store→load (threadgroup, volatile) | S | **21.8/access** | **28.3** | Cross-thread; L1 is FASTER than LDS |
| Threadgroup barrier | S | **25.44** | **33.1** | |
| Threadgroup atomic (fetch_add, uncontended) | S | **89.6** | **116.5** | Serial dep-chain |
| Global atomic (fetch_add, uncontended) | S | **143.8** | **186.9** | 60% slower than TG atomic |
| MTLSharedEvent signal→wait | S | p50=**10,000 ns** | ~13,000 | Host-GPU sync primitive |
| L1 data cache size | P+D | 8 KB | — | Hardware spec |
| LDS (threadgroup) size | P | ~60 KB | — | Shared memory per core |
| L2 per-core size | P | 768 KB | — | Per GPU core (M1: 8 cores) |
| L3 (system cache) size | P | 8 MB | — | Shared across CPU+GPU on M1 |
| Cache line size | P | 128 bytes | — | Global memory |

_Note: Texture dep-chain C4 findings (2026-04-03):_
- _Probe: `probe_tex_dep4` — 1×1 RGBA32F, filter::linear, 4 samples/iter, 100K iters, dep chain: coord → sample → fma(s.xy,0.4,0.3) → coord_
- _FMA in dep chain is fast-pathed (fixed point 0.5 = power of 2; texture always returns {0.5,…}) → tex_step ≈ L_TEX alone_
- _Clock during tex run is uncertain: GPU throttles over 9-second run. ns/step stable at 102±5 ns; cycle count requires clock pin._
- _Interleave mode failed: tex_dep4 at ~45ms/dispatch is 12× slower than reference kernels → GPU state mismatch between kernels in interleaved session_
- _Open: (1) nearest-neighbor comparison to isolate bilinear filter cost; (2) mixed-kernel (FMA+TEX in same dispatch) for clock-independent Δ; (3) MTLCounterSampleBuffer for GPU-internal timestamps_

---

## SIMD-Group Operations

| Operation | Source | ns/op | cyc @1.3GHz | Notes |
|-----------|--------|-------|-------------|-------|
| simd_broadcast (lane 0→all) | S | **10.690** | **13.9** | ≈ L1 load; all permutations cost same |
| simd_shuffle_down (delta=1) | S | **10.162** | **13.2** | |
| simd_shuffle_xor (mask=1) | S | **10.215** | **13.3** | |
| simd_prefix_inclusive_sum | S | **23.720** | **30.8** | log2(32) stages; faster than simd_sum! |
| simd_prefix_exclusive_sum | S | **22.275** | **29.0** | 1.73 ns cheaper than inclusive |
| simd_sum (32-lane reduce) | S | **28.31** | **36.8** | 2.77× single shuffle |
| simdgroup_matmul 8×8 f32 dep | S | **24.235** | **31.5** | T=L (non-pipelineable); 512 MADs |
| simdgroup_matmul 8×8 f32 TP | S | **24.384** | **31.7** | ILP=1.0: no TP gain — hardware limitation |
| Shuffle bandwidth/cycle | P | 256 B/cyc | — | AGX 2× AMD/Nvidia (128 B) |
| SIMD-group size | D+P | 32 threads | — | |

---

## CPU (AArch64 / NEON, for cross-domain)

| Operation | S dep-chain (ns) | S dep-chain (cyc @3.2GHz) | Notes |
|-----------|-----------------|--------------------------|-------|
| INT ADD (u8/u16/u32/u64) | 0.315–0.333 | ~1 | Fastest CPU primitive |
| CLZ (u64) | 0.645 | ~2 | Count-leading-zeros |
| f32 fadd | 0.945 | ~3 | CPU FP baseline |
| f64 fadd | 0.945 | ~3 | = f32 on M1 |
| f16 fadd (FEAT_FP16) | 0.945–0.980 | ~3 | Zero penalty vs f32 |
| f16 fmul | 1.306 | ~4.2 | Slightly slower |
| int MUL (u32/u64) | 0.944–0.979 | ~3 | = FP add |
| UMULH / SMULL | 0.944–0.964 | ~3 | Wide multiply |
| f64 FSQRT | 4.870 | ~15.6 | 5.15× fadd |
| libm sin/cos | 20.30 | ~65 | Software |
| libm log | 14.497 | ~46 | |
| libm exp | 14.879 | ~48 | |
| atomic add (relaxed, u64) | 1.912 | ~6.1 | Uncontended |
| f32x4 FMA (NEON, dep) | 0.951/vec = **0.237**/f32 | ~3 CPU | 4-wide FMA |
| f64x2 vadd (NEON) | 0.964/vec = **0.482**/f64 | ~3 | |
| f64×2 DD two-sum (dep) | 1.044/vec = **0.522**/DD-val | ~3.3 | |
| f64×2 DD throughput (ILP) | 0.289/vec = **0.144**/DD-val | ~0.9 | FASTEST 106-bit; 6.6× scalar f64 |
| DS two-sum (f32×2, GPU) | 3.348 ns = **4.35 cyc GPU** | — | GPU 2.49× slower than CPU DD |

---

## Neural / Framework Dispatch

| Metric | S measurement | Notes |
|--------|--------------|-------|
| PyTorch MPS identity FC 64-dim (p50) | **195 µs** | Lower bound for any MPS model |
| PyTorch MPS identity FC 64-dim (min) | **160 µs** | |
| PyTorch CPU identity FC 64-dim (p50) | **5 µs** | ~39× faster than MPS for trivial model |
| CoreML MLMultiArray(64) alloc+fill | **46.6 µs** | Framework overhead before any compute |
| CoreML ANE dispatch | estimated 1–10 ms | Not directly measured (Python 3.14 blocks coremltools) |
| fp16 GEMM 1024×1024: MPS | **2.012 ms** | 355× faster than CPU |
| fp16 GEMM 1024×1024: MLX | **1.444 ms** | Fastest measured |
| fp16 GEMM 1024×1024: CPU | **714 ms** | macOS BLAS scalar fallback for fp16 |
| bf16 GEMM 1024×1024: MPS | **2.773 ms** | 298× faster than CPU |
| bf16 GEMM 1024×1024: CPU | **827 ms** | Scalar fallback |
| int8 GEMM 1024×1024: MPS | **8.506 ms** | 5.6× faster than CPU |
| fp32 GEMM 1024×1024: MPS | **2.3 ms** | ≈ fp16 MPS; CPU competitive for fp32 |

---

## True Gaps — Unmeasured Operations

The following have Philip's reference data but **no steinmarder dep-chain probe**:

### Highest priority (P has data; we should confirm or contradict)
| Operation | P adj_lat (cyc) | Why measure |
|-----------|-----------------|-------------|
| _~~LOG2_32 re-probe~~_ | _~~4.31~~_ | _DONE: H1b v3 (arg≈1.858) = 4.38 cyc ✓; v2 5.90 cyc confirmed as power-of-2 slow path_ |
| FMIN32 | 4.74 | Not yet probed; FMAX confirmed ~1 cyc ALU — FMIN expected same |
| FMUL32 fused `(x*x)+1` pattern | ~1 cyc | Philip showed single-operand doubles TP |
| IMUL(32x32=64) hardware | 9.84 | H1c: v2 probe (native uint a,b) → 7.2 cyc/step; Δ=+2.1 vs IMUL32 → hi32 path adj_lat ≈ 6 cyc. True 8-cyc widening may need dep chain through full ulong (not split lo/hi). Probe v3 needed. |
| _~~FSQRT32~~_ | _~~8.57–11.13~~_ | _DONE: H1b dep-chain = 10.08 cyc ✓_ |
| _~~EXP2_32~~_ | _~~4.31~~_ | _DONE: H1b dep-chain = 4.56 cyc ✓_ |
| _~~FMAX32~~_ | _~~4.74~~_ | _DONE: H1b ~1 cyc ALU tier_ |
| _~~FCMPSEL32~~_ | _~~4.74~~_ | _DONE: H1b ~1 cyc, same unit as FMAX_ |

### Medium priority (gaps with practical relevance)
| Operation | P adj_lat (cyc) | Why measure |
|-----------|-----------------|-------------|
| RECIP16 | 6.50 | Confirm fp16 SFU uses same unit |
| RSQRT16 | 8.61 | fp16 path |
| FDIV32 dep-chain | 7.62–8.90 | Metal fast division path |
| SIN32/COS32 | 23–27 | GPU transcendentals (useful to verify vs Philip) |
| CLZ32 (GPU) | 7–9 | We have CLZ64 on CPU; GPU CLZ is different unit |
| POPCOUNT32 (GPU) | 3.62 | Cryptography/compression kernels |
| INT16 mul dep-chain | 3.69 | S has no GPU int16 probe |

### Lower priority / novel (neither Philip nor us)
| Operation | Notes |
|-----------|-------|
| Metal texture sample dep-chain (C4) | Separate texture unit; completely unmeasured |
| AGX memory ordering / coherency model | Full barrier vs. threadgroup vs. simd scope |
| subgroup ballot / vote | `simd_vote_all`, `simd_ballot` latency |
| INT32 division (GPU) | No GPU division probe; different from RECIP |
| IMUL(32×32=64) true hardware path | H1c confirmed TRUNC→ZEXT → IMUL64; hi32-only path at ~6 cyc. True 8-cyc widening (full ulong output) not yet verified with v3 probe. |
| fp16 dep-chain from fp16 texture reads | Mixed texture+ALU pipeline |

### CPU gaps (no AArch64 probe)
| Operation | Notes |
|-----------|-------|
| CPU FSQRT32 (scalar) | We have f64 sqrt=4.87 ns; f32 sqrt likely same |
| CPU bitwise ops (AND/OR/XOR/SHIFT) | Likely 1 cycle; not explicitly probed |
| CPU f32 SIMD max/min (NEON) | Comparison ops |
| CPU address generation | Load + index arithmetic pipeline |

---

## Cross-Domain Summary (GPU vs CPU, selected ops)

| Op | CPU dep (ns) | GPU dep (ns) | GPU dep (cyc) | P adj_lat (cyc) |
|----|-------------|-------------|---------------|-----------------|
| fadd32 | 0.945 | 1.695 | 2.20 | 2.20 ✓ |
| fmul32 | 0.945 | 1.347 | 1.75 | 2.21 |
| ffma32 | 1.260 | 1.993 | 2.59 | 2.21 |
| imul32 | 0.944 | 3.376 | 4.39 | 4.02 ✓ |
| recip32 | — | 8.605 | 11.18 | 6.50 (TP) |
| rsqrt32 fast | — | 9.405 | 12.23 | 8.99 (TP) |
| rsqrt32 precise | — | 29.2 | 37.97 | 8.99 (??) |
| sqrt64 CPU | 4.870 | — (gap) | — | 8.57–11.13 |
| exp2 GPU | — | 4.932 (1-ch dep) | 4.56 adj_lat | 4.31 (Philip) |
| log2 GPU (H1b) | — | 5.784 (1-ch dep) | 5.90 adj_lat ⚠ | 4.31 (Philip) |
| log CPU | 14.497 (libm) | — (gap) | — | 4.31 (native!) |
| sin CPU | 20.30 (libm) | — (gap) | — | 23–27 (similar!) |
| L1 load | ~1.6 ns (CPU) | 10.07 ns (GPU) | 13.1 | — |
| atomic | 1.912 ns (CPU) | 89.6 ns (GPU) | 116.5 | — |

**Key insight**: GPU has **native hardware** EXP2/LOG2 at ~4 cycles. CPU libm log ≈ 46 cycles = **11× slower** than GPU hardware log2. This is one of the strongest cases for offloading to GPU.

---

## Key Architectural Constants

| Fact | Value | Source |
|------|-------|--------|
| GPU clock (M1) | ~1.28–1.33 GHz | S derived |
| SIMD-group size | 32 threads | D (dougallj/applegpu) |
| Registers per core | ~208 KB (≈ 128 GPRs × 32 threads × 4 bytes × 4 pipelines) | P |
| L1 data cache | 8 KB/core | P |
| LDS (threadgroup) | ~60 KB/core | P |
| L2 per-core | 768 KB | P |
| L3 (SLC) | 8 MB | P |
| Cache line | 128 bytes | P |
| SIMD shuffle BW | 256 B/cycle | P |
| FP32 OPs/cycle/core | 256 (FMA = 2 ops) | P |
| FP16 OPs/cycle/core | 256 (same as FP32 on M1+!) | P |
| INT32 OPs/cycle/core | 128 (muls) / 512 (adds) | P |
| AGX firmware-driven | ARM64 ASC coprocessor | Asahi |
| No hardware image atomics | M1 | Asahi |
| Hardware scheduling (no NOP padding) | M1 AGX | Rosenzweig Part I |
| 16-bit ALU reuse | All 4 FP32 pipelines accept 16-bit | P+Rosenzweig |
| AGX `wait` overhead | L1 dep-chain = 64 cyc vs 13 cyc sequential | S (C3) |
| IMUL(32x32=64) hardware | 8 cyc (NOT 12 if split manually) | P |

---

_Sources: P = philipturner/metal-benchmarks (https://github.com/philipturner/metal-benchmarks),  
S = steinmarder probes (this repo), D = dougallj/applegpu (https://dougallj.github.io/applegpu/docs.html)_
