<!--
@file ZEN4_PROBE_REFERENCE.md
@brief Complete reference for all Zen 4 microarchitecture probes — what each measures,
       why it matters, and first-run results on the Ryzen 9 7945HX (DESKTOP-CKP9KB6, WSL2).
-->
# Zen 4 Probe Reference

**Machine:** Ryzen 9 7945HX (Zen 4, 16C/32T, 2 CCDs)
**Platform:** WSL2 on Windows, Ubuntu 24.04 guest
**Compiler:** clang-22, `-O2 -march=native`
**Date:** 2026-04-04

---

## Portable Probes (Zen 3 / Zen 4)

### 1. `probe_fma` — FMA Latency and Throughput (AVX2)

**What it measures:** The cost of a 256-bit fused multiply-add (`vfmadd231ps ymm`).
- *Latency:* chain of dependent FMAs where each output feeds the next input. Measures
  the minimum time from FMA input ready to FMA output ready (pipeline depth).
- *Throughput:* eight independent FMA accumulators running in parallel. Measures how
  many FMAs the backend can retire per cycle when there are no data dependencies.

**Why it matters:** FMA is the inner-loop instruction for matrix-vector multiply (MLP
inference), dot products, and blending. Latency determines how deep you must unroll a
reduction. Throughput determines peak FLOPS.

**Zen 4 result:** Latency 2.19 cyc, throughput 0.24 cyc/FMA (4.2 FMAs/cycle at 256-bit).

---

### 2. `probe_load_use` — Pointer-Chase Load Latency

**What it measures:** The time from issuing a load to using the loaded value as an
address for the next load. A linked-list pointer chase through a 1 MB arena — each
cache line's first 8 bytes point to the next node in a randomized ring.

**Why it matters:** This is the irreducible latency for dependent memory access patterns
(hash table lookups, tree traversals, BVH ray intersection). No prefetching or
out-of-order execution can hide this chain. The 1 MB working set targets L2.

**Zen 4 result:** 14.5 cyc — consistent with Zen 4's documented L2 load-use latency.

---

### 3. `probe_permute` — Cross-Lane Shuffle Throughput (AVX2)

**What it measures:** Sustained throughput of `vpermps ymm` — a 32-bit granularity
cross-lane permute within a 256-bit register.

**Why it matters:** Lane permutes are used for horizontal reductions, data transposition
(e.g. AoS→SoA), and scatter/gather emulation. If permutes are expensive, layout
transformations at prepack time are more valuable than runtime shuffles.

**Zen 4 result:** 0.062 cyc/permute — essentially free (~16 per cycle). The shuffle
unit is not a bottleneck.

---

### 4. `probe_gather` — Vector Gather Latency (AVX2)

**What it measures:** The time for an 8-element indexed load (`vpgatherdd ymm`) from a
1 MB random table. Each element loads from a different cache line.

**Why it matters:** Gathers are the core operation for hash-grid lookup in the NeRF
pipeline (indexed access into a spatial hash table). Gather cost determines whether the
hash-grid phase or the MLP phase dominates.

**Zen 4 result:** 6.39 cyc per 8-element gather (0.80 cyc/element).

---

### 5. `probe_branch` — Branch Misprediction Penalty

**What it measures:** Two loops over the same data:
- *Predictable:* sorted array — branch predictor gets nearly 100% accuracy.
- *Unpredictable:* random array — ~50% misprediction rate.
- *Delta:* the difference is the cost of a misprediction.

**Why it matters:** The NeRF renderer has conditional paths for material evaluation,
BVH traversal (left/right child), and early termination. High mispredict cost favors
branchless (cmov/select) implementations.

**Zen 4 result:** Predictable 0.48 cyc, unpredictable 2.93 cyc, penalty delta 2.44 cyc.

---

### 6. `probe_prefetch` — Software Prefetch Benefit/Harm

**What it measures:** A streaming sum over a 1 MB buffer, with and without
`_mm_prefetch(_MM_HINT_T0)` one cache line ahead. Positive delta means prefetch hurts
(data is already in cache).

**Why it matters:** At working sets that fit in L2, software prefetch adds instruction
overhead without benefit. This probe identifies the crossover point where prefetching
starts helping (typically when data spills to L3 or DRAM).

**Zen 4 result:** +0.24 cyc overhead at 1 MB — prefetch hurts. Data fits in L2.

---

## Cache Hierarchy

### 7. `probe_cache_hierarchy` — Load Latency vs Working Set Size

**What it measures:** Pointer-chase latency at 15 working set sizes from 4 KB to 64 MB.
At each size, builds a randomized ring of 64-byte-stride nodes and measures the average
cycles per chase step. Latency jumps reveal cache level boundaries.

**Why it matters:** Maps the full memory hierarchy for this specific machine. The
results determine optimal tile sizes, weight matrix partitioning, and working set
targets for each kernel.

**Zen 4 result:**

| Working Set | Latency | Level |
|-------------|---------|-------|
| 4-32 KB | ~2 cyc | L1d (32 KB/core) |
| 64-256 KB | 6.8 cyc | L2 lower |
| 512 KB | 8.6 cyc | L2 upper |
| 1 MB | 16.3 cyc | L2 boundary (1 MB/core) |
| 2-4 MB | 23-25 cyc | L3 entry |
| 8-16 MB | 27-32 cyc | L3 (32 MB shared) |
| 32 MB | 78 cyc | L3 boundary |
| 64 MB | 219 cyc | DRAM |

---

## AVX-512 Compute

### 8. `probe_avx512_fma` — 512-bit vs 256-bit FMA

**What it measures:** Same methodology as probe_fma but comparing zmm (512-bit) and ymm
(256-bit) FMA. Zen 4 executes 512-bit instructions as two 256-bit micro-ops.

**Why it matters:** Quantifies the actual cost of using zmm registers. If zmm throughput
is close to 2× ymm latency, AVX-512 gives a net win per element despite the split.

**Zen 4 result:**
- zmm FMA latency: 1.44 cyc (overlapped 2×256 uops)
- zmm FMA throughput: 0.58 cyc (vs ymm 0.24 cyc → ratio 2.4×)
- Per-element: zmm processes 16 floats at 0.58 cyc = 0.036 cyc/elem
  vs ymm 8 floats at 0.24 cyc = 0.030 cyc/elem → ymm is ~17% better per-element for pure FMA

---

### 9. `probe_avx512_gather` — 16-Element vs 8-Element Gather

**What it measures:** `vpgatherdd zmm` (16 elements) vs `vpgatherdd ymm` (8 elements)
from a 1 MB table. Measures both total cycles per gather and per-element cost.

**Why it matters:** For the hash-grid lookup, wider gathers reduce loop overhead if the
per-element cost doesn't increase proportionally. The split penalty is the question.

**Zen 4 result:**
- zmm: 9.01 cyc / 16 elem = 0.56 cyc/elem
- ymm: 4.71 cyc / 8 elem = 0.59 cyc/elem
- **zmm is 5% cheaper per element** — modest but consistent win for 512-bit gathers.

---

### 10. `probe_avx512_vnni` — Integer Quantized Dot Products

**What it measures:** Throughput and latency of:
- `vpdpbusd` — uint8 × int8 dot product accumulate into int32. Each instruction does
  4 byte multiplies + 3 additions per 32-bit lane, 16 lanes = 64 multiply-adds.
- `vpdpwssd` — int16 × int16 dot product accumulate into int32. Each instruction does
  2 word multiplies + 1 addition per 32-bit lane, 16 lanes = 32 multiply-adds.

**Why it matters:** VNNI is the path for INT8-quantized inference (e.g. quantized NeRF
MLP weights). If VNNI throughput is high enough relative to FP32 FMA, quantization
becomes a viable optimization strategy.

**Zen 4 result:**
- vpdpbusd: latency 2.06 cyc, throughput 0.40 cyc → 160 int8 ops/cycle
- vpdpwssd: throughput 0.25 cyc → 131 int16 ops/cycle

---

### 11. `probe_avx512_bf16` — BF16 Dot Product vs FP32 FMA

**What it measures:** Throughput of `vdpbf16ps` (BF16 pair dot product accumulate into
FP32) compared to `vfmadd231ps` (FP32 FMA). Each `vdpbf16ps zmm` performs 32 BF16
multiplies and 16 FP32 additions — double the multiply work of a FP32 FMA.

**Why it matters:** BF16 weights halve memory footprint and double multiply throughput
without changing the accumulator precision (still FP32). This is the primary path for
the NeRF MLP BF16 variant.

**Zen 4 result:**
- vdpbf16ps throughput: 0.48 cyc — **identical** to FP32 FMA throughput
- But 32 BF16 muls vs 16 FP32 muls per instruction
- **66.5 BF16 ops/cycle vs 33.3 FP32 ops/cycle = exact 2× throughput**
- vdpbf16ps latency: 2.96 cyc (vs FP32 FMA ~1.44 cyc — longer pipeline)

---

## AVX-512 Memory

### 12. `probe_avx512_bandwidth` — Sustained Load/Store Bandwidth

**What it measures:** Sequential streaming bandwidth using `vmovaps zmm` (64 B/load),
`vmovaps ymm` (32 B/load), and `vmovaps zmm` stores, swept across 9 working set sizes
from 16 KB (L1) to 64 MB (DRAM). Reports bytes per cycle.

**Why it matters:** Bandwidth determines whether a kernel is compute-bound or
memory-bound. If the MLP weight matrix exceeds L2, the kernel becomes bandwidth-limited
and the FMA throughput advantage of BF16 matters less than the 2× memory reduction.

**Zen 4 result:**

| Working Set | zmm Load | ymm Load | zmm Store |
|-------------|----------|----------|-----------|
| L1d (32 KB) | 39 B/cyc | 43 B/cyc | 65 B/cyc |
| L2 (1 MB) | 43 B/cyc | 41 B/cyc | 51 B/cyc |
| L3 (16 MB) | 42 B/cyc | 41 B/cyc | 52 B/cyc |
| DRAM (64 MB) | 23 B/cyc | 26 B/cyc | 14 B/cyc |

ymm slightly wins at L1 and DRAM (native 256-bit path). At L2/L3 they converge.

---

## AVX-512 Scheduling

### 13. `probe_avx512_throttle` — Frequency Stability Under Sustained AVX-512

**What it measures:** 10 consecutive bursts of 50 million zmm FMA iterations each,
timed independently. Reports min/max cycle-per-FMA and drift percentage. Also measures
scalar→zmm transition cost (a scalar burst immediately followed by a zmm burst).

**Why it matters:** Intel CPUs aggressively downclock (100-200 MHz) when AVX-512 is
active. If Zen 4 also throttles, sustained AVX-512 kernels would lose the compute
advantage to frequency reduction. This probe detects it.

**Zen 4 result:**
- Drift: 0.72% (min 0.4946, max 0.4981 cyc/FMA) → **NO throttle**
- Scalar→zmm transition: zero penalty (0.49 cyc immediately)

---

### 14. `probe_avx512_port_contention` — FMA + Gather Overlap

**What it measures:** Three scenarios at 256 KB (L2):
- FMA-only burst (4 independent zmm FMAs per iteration)
- Gather-only burst (2 zmm gathers per iteration)
- Interleaved FMA + gather (2 FMAs + 2 gathers, alternating)

Compares interleaved time to max(FMA, gather) — perfect overlap means the slower
operation completely hides the faster one. Negative overlap means contention.

**Why it matters:** If FMA and gather overlap, interleaving hash-grid lookups with MLP
math would improve throughput. If they contend, phase-separating them is better.

**Zen 4 result:**
- FMA-only: 1.96 cyc, gather-only: 17.2 cyc, interleaved: 19.0-21.1 cyc
- Overlap efficiency: ~10% — **mostly serialized, slight overlap**
- Conclusion: **phase-separate hash lookup from MLP computation**

---

## AVX-512 Integer / Crypto

### 15. `probe_avx512_ifma` — 52-bit Integer Multiply-Accumulate

**What it measures:** Latency and throughput of:
- `vpmadd52luq` — multiply 52-bit unsigned integers, accumulate low 52 bits
- `vpmadd52huq` — multiply 52-bit unsigned integers, accumulate high 52 bits

**Why it matters:** IFMA is the key instruction for big-integer arithmetic (RSA, ECC),
polynomial multiplication, and CRC acceleration. The lo/hi throughput asymmetry reveals
which half of the multiplier is the bottleneck.

**Zen 4 result:**
- vpmadd52luq: latency 4.24 cyc, throughput 1.06 cyc (~1 zmm/cycle)
- vpmadd52huq: latency 2.36 cyc, throughput 0.47 cyc (~2 zmm/cycle)
- **2× asymmetry** — the hi path is twice as fast, suggesting different execution units.

---

## Multi-Core Scaling

### 16. `probe_avx512_multicore` — Cross-CCD FMA Throughput Scaling

**What it measures:** Spawns N threads (1, 2, 4, 8, 16), each pinned to a sequential
core, each running a sustained zmm FMA throughput loop for a fixed iteration count.
Measures per-thread FMA throughput and aggregate GFLOPS across all threads.

**Why it matters:** The 7945HX has 2 CCDs (compute chiplet dies), 8 cores each. Cores
0-7 are CCD0, cores 8-15 are CCD1. Cross-CCD communication goes through the Infinity
Fabric. This probe reveals whether all cores achieve equal FMA throughput and how
aggregate performance scales.

**Zen 4 result:**

| Threads | Worst cyc/FMA | Aggregate GFLOPS |
|---------|---------------|------------------|
| 1 | 0.51 | 123 |
| 2 | 0.68 | 177 |
| 4 | 0.83 | 307 |
| 8 (CCD0) | 0.87 | 474 |
| 16 (both CCDs) | 0.99 | 668 |

CCD1 cores (8-15) showed ~0.95 cyc/FMA vs CCD0 at 0.49-0.59 cyc/FMA — likely WSL2
scheduling asymmetry rather than hardware. 8→16 thread scaling: 1.4×.

---

## Harness

### 17. `probe_zen4_full.sh` — Full Suite Runner

**What it does:** Runs all probes in sequence, pins to a specified CPU core, collects
per-probe `.txt` output files, machine info, and a unified `summary.csv` with all CSV
lines from all probes. Creates a timestamped results directory.

**Usage:** `./probe_zen4_full.sh [cpu] [output_dir]`

---

## AVX-512 Data Manipulation

### 18. `probe_avx512_shuffle` — Cross-Lane Permute at 512-bit
ZMM permutes are exactly 2× YMM cost (double-pumped). Byte-granularity `vpermb` has zero
penalty vs 32-bit `vpermps`. All at ~0.47 cyc throughput.

### 19. `probe_avx512_convert` — FP32↔BF16 Conversion
`vcvtne2ps2bf16 zmm`: 0.82 cyc → 39 BF16/cycle. Manual BF16→FP32 expand: 1.24 cyc.

### 20. `probe_avx512_mask` — Masked Operation Throughput
Masked FMA is actually *faster* than unmasked (0.48 vs 0.66 cyc) — fewer writeback ports used.

### 21. `probe_avx512_dq` — AVX-512DQ Extension
`vpmullq zmm`: 3.28 cyc latency, ~1 op/cycle. **Native 64-bit SIMD multiply confirmed.**
`vfpclassps zmm → k`: 0.06 cyc — mask-producing ops are essentially free.

### 22. `probe_avx512_cd` — Conflict Detection
`vpconflictd zmm`: 2.86 cyc latency, 0.88 cyc throughput. No data-dependent penalty.

### 23. `probe_avx512_vbmi2` — Byte Compress/Expand + Variable Shift
`vpcompressb` fast-path for all-1s mask: 0.06 cyc vs 0.86 cyc with real masks (~14× difference).
`vpshldvd/vpshrdvd`: 0.48 cyc, direction and width independent.

### 24. `probe_avx512_bw_extended` — Byte Shuffle, Min/Max, Pack, Compare
`vpshufb zmm`: 0.48 cyc. Byte min/max: 0.26 cyc. Saturating pack: 0.12 cyc.
`vpcmpub zmm → k`: 0.06 cyc.

---

## Crypto and Bit Manipulation

### 25. `probe_vaes` — Vectorized AES
zmm/xmm throughput ratio exactly 2.00× (double-pump). AES pipe is natively 256-bit.
`vaesenc zmm`: 0.47 cyc throughput = 8.56 blocks/cycle.

### 26. `probe_gfni` — Galois Field Instructions
`vgf2p8affineqb zmm`: 0.48 cyc. Universal byte→byte LUT in one instruction.

### 27. `probe_sha` — SHA-NI Hardware Acceleration
`sha256rnds2`: 0.48 cyc throughput. `sha256msg2`: 1.22 cyc (bottleneck).
SHA-1 message schedule ops have sub-cycle latency (~0.94 cyc).

### 28. `probe_bmi` — BMI1/BMI2 Bit Manipulation
**pdep/pext FIXED on Zen 4**: 1.44 cyc latency (was ~18 cyc on Zen 1–3). ~12× improvement.
`tzcnt/lzcnt/popcnt`: 0.48 cyc latency, 0.14 cyc throughput.

### 29. `probe_vpclmulqdq` — Carry-Less Multiplication
Near-ideal width scaling: zmm/xmm throughput ratio only 1.05×. Latency identical at ~1.92 cyc.

---

## Memory and Synchronization

### 30. `probe_atomic_contention` — Atomic Ops Under Thread Contention
Single-threaded: `lock xadd` ~3.8 cyc/op. 16-thread contention: 87× slowdown.
Aggregate throughput peaks at 8 threads, drops at 16 (cross-CCD coherence).

### 31. `probe_streaming_bandwidth` — Non-Temporal Stores
NT stores offer NO benefit on Zen 4 single-core (11.3 vs 11.4 B/cyc).
NT loads give ~5% benefit (17.8 vs 17.0 B/cyc).

### 32. `probe_false_sharing` — Cache Line Contention
Same-CCD false sharing: **4–6× throughput penalty**. Cross-CCD: ~5.5×.

### 33. `probe_tlb_reach` — TLB Miss and Page Walk Cost
L1 dTLB: covers ~288 KB. L2 TLB: covers ~12 MB. Full page walk at 128+ MB: 174 cyc.

### 34. `probe_store_forwarding` — Store-to-Load Forwarding
Same-size: free (0.48 cyc). **Narrow-to-wide: FAILS with 9.22 cyc penalty.**

---

## Exhaustive Dependency Chains

### 35. `probe_depchain_int` / `probe_depchain_integer` — Integer Type Chains
All SIMD integer adds (8/16/32/64-bit) are width-uniform at 0.49 cyc (1-cycle latency).
INT32 mul: 1.46 cyc. Signed/unsigned identical.

### 36. `probe_depchain_float` — Floating-Point Type Chains
FP32/FP64 add/mul/fma: ~3 cyc. FP80 x87: ~8 cyc. FP128 (__float128): ~80 cyc (software).
BF16 vdpbf16ps: ~6 cyc. FP16 emulated: ~9 cyc. TF32 emulated: ~4 cyc.

### 37. `probe_depchain_simd_width` — Width Scaling
**512-bit latency equals 256-bit on Zen 4.** Double-pump halves throughput but adds zero latency.

---

## Compute Translations (from SASS/Apple/r600)

### 38. `probe_transcendental` — Scalar and SIMD Transcendentals
`vrcp14ps/vrsqrt14ps zmm`: 0.49 cyc throughput (free). `vsqrtps zmm`: 4.98 cyc.

### 39. `probe_reduction` — Horizontal Reduction Patterns
All 16-lane strategies converge to ~12.7 cyc latency. No shortcut.

### 40. `probe_dot_product` — Multi-Width Dot Products
`vpdpbusd zmm` (int8): 0.49 cyc throughput. Float dot products dominated by reduction cost.

### 41. `probe_popcount_lzcnt` — Bit Manipulation
`vpternlogd zmm`: 0.30 cyc (~3.3 ops/cycle). `vpopcntd zmm`: 1.10 cyc.

---

## Complete Coverage Probes

### 42. `probe_dotproduct_complete` — All 9 Dot Product Mnemonics × All Widths
VNNI is width-agnostic (~0.48 cyc at 128/256/512). DPPS is 5.5 cyc (microcoded). DPPD: 3.4 cyc.

### 43. `probe_x87_complete` — AMD Volume 5 x87 Gap-Fill (44 forms)
FBSTP m80: 131 cyc. FNSAVE: 143 cyc. FPREM1: 17.6 cyc. FCOMP anomaly at 48 cyc.

### 44. `probe_prefetchw` — 3DNow! Survivor
0.36 cyc throughput, matches PREFETCHT1/T2/NTA. Write-intent prefetch has no extra cost.

### 45. `probe_legacy_exhaustive` — MMX/SSE4a/SSE4.2/CRC32/VEX-vs-Legacy
**CRITICAL: Legacy SSE encoding has 6× throughput penalty vs VEX on Zen 4.**
MOVNTSD (SSE4a): anomalously slow at 26 cyc. CRC32 is width-independent at 1.44 cyc.

---

## Exhaustive Measurement Probes (single-binary, hundreds of forms each)

### 46–52. `probe_*_exhaustive` — Automated Measurement Suite
Seven exhaustive probes, each measuring 58–227 instruction forms in a single binary.
See `ZEN4_MEASUREMENT_CAMPAIGN_STATUS.md` for the full 1,096-form results table
and `results/zen4_*.csv` for raw data.

---

## Key Findings for NeRF MLP Optimization

1. **BF16 is free 2× throughput** — `vdpbf16ps` matches FP32 FMA cycle cost but does 2× multiplies
2. **No AVX-512 throttle** — use zmm freely, zero transition penalty
3. **Gather + FMA don't overlap** — phase-separate hash lookup from matrix multiply
4. **L2 is 1 MB/core** — all MLP weights (24 KB FP32, 12 KB BF16) fit trivially in L1d
5. **ymm beats zmm for pure bandwidth** at L1 — but BF16's 2× compute win dominates for compute-bound kernels
6. **VNNI offers 160+ int8 ops/cycle** — quantized inference is viable if accuracy permits
7. **Cross-CCD scaling is ~1.4×** from 8 to 16 threads — diminishing returns beyond one CCD for compute-bound work
8. **Legacy SSE is 6× slower** — always compile with `-mavx` or higher on Zen 4
9. **pdep/pext are fast** — use freely for bit manipulation (1.44 cyc, was 18 cyc on Zen 1–3)
10. **Narrow-to-wide store forwarding fails** — 9 cyc penalty; avoid mixed-width spill/reload
11. **vpternlogd is the fastest logic op** — 0.30 cyc (~3.3/cycle), use as universal boolean
12. **Mask-producing ops are free** — vpcmpub/vfpclassps at 0.06 cyc; use liberally for branchless
13. **Native 64-bit multiply exists** — vpmullq 3.28 cyc via AVX-512DQ; no emulation needed
