# Cross-Architecture Synthesis

**steinmarder** reverse-engineering project -- comparison of four measured architectures.

**Date**: 2026-04-04
**Tracks**: Zen 4 (Ryzen), TeraScale-2 (r600), Apple AGX (M1), SM89 (Ada Lovelace)
**Methodology**: All data sourced from direct steinmarder probe measurements. Where a
dimension has not been measured for a given track, the cell reads "not measured."

---

## 1. Architecture Comparison Table

| Dimension | Zen 4 (7945HX) | TeraScale-2 (HD 6310) | Apple AGX (M1) | SM89 (RTX 4070 Ti) |
|---|---|---|---|---|
| **Year** | 2022 | 2011 | 2020 | 2022 |
| **Type** | CPU (x86-64) | GPU (VLIW5) | GPU + CPU (ARM) | GPU (CUDA) |
| **Process** | 5 nm | 40 nm | 5 nm | 5 nm |
| **Clock (measured/nominal)** | ~5.1 GHz | 487 MHz | ~1.28-1.33 GHz (GPU) / ~3.2 GHz (CPU) | 2625 MHz |
| **Execution model** | OoO superscalar, 6-wide dispatch | VLIW5, 5 ops/bundle, wavefront-interleaved | OoO (CPU); HW-scheduled SIMD-group (GPU) | Warp-scheduled, 32-wide SIMT |
| **SIMD/warp width** | 512 bits (AVX-512, double-pumped from 256) | 5-slot VLIW bundle x 64-thread wavefronts | 32-thread SIMD-group (GPU); 128-bit NEON (CPU) | 32-thread warp |
| **FP32 FMA latency** | ~1.9 cy (ymm), ~1.4 cy (zmm overlapped) | 1 cycle (MULADD_IEEE via PV forwarding) | ~2.6 cy / ~1.99 ns (GPU dep-chain); ~4 cy / 1.26 ns (CPU) | 4.53 cy |
| **FP32 FMA throughput** | 0.24 cyc/FMA (ymm); 0.48 cyc/FMA (zmm) | 4/cycle (4 vec slots); 1 DOT4/cycle | ~1.0 cyc/fmul (GPU throughput); 0.237 ns/f32 (CPU NEON 4-wide) | 4.53 cy (dep-chain; throughput per-SM not isolated) |
| **FP64 support** | Native, same pipeline, ~3 cy dep-chain | Native dual-slot (ADD_64/MUL_64/FMA_64); ~half-rate FP32 | CPU: native, 0.945 ns (= FP32); GPU: no FP64 | Native but very slow: DFMA 54.48 cy, DADD 48.47 cy |
| **FP16 support** | No native FP16 compute (conversion only) | No FP16 ALU; native FLT16_TO_FLT32 conversion | CPU: native FEAT_FP16, zero penalty vs FP32; GPU: more 16-bit ALUs than 32-bit, but dep-chain shows widening overhead | HFMA2: 4.54 cy; HFMA2.BF16_V2: 4.01 cy (fastest FMA on Ada) |
| **BF16 support** | VDPBF16PS: 0.72 cyc throughput (2x FP32 ops/cycle) | Not supported | Not measured | F2FP.BF16.F32.PACK_AB: ~8.5 cy conversion; HMMA BF16 tensor core: 66.33 cy/WMMA |
| **INT8 dot product** | VPDPBUSD: 0.48 cyc throughput, width-invariant 128/256/512 | UBYTE_FLT unpack (1 cycle per 4 bytes) + FP32 MULADD path | Not measured on GPU; CPU int ADD: 0.315 ns (~1 cycle) | IDP.4A: 4.53 cy (dp4a packed INT8 dot product) |
| **Tensor/matrix unit** | VNNI (integer dot product); no matrix unit | None | simdgroup_matrix_multiply 8x8: ~32 cy (non-pipelineable) | HMMA tensor cores: INT8 34 cy, FP16 42 cy, FP32-accum 66 cy, INT4 28 cy per WMMA |
| **Register file** | 32 GPRs (architectural) + deep rename | 128 x vec4 (128-bit) per SIMD, shared across wavefronts | 128 GPRs/SIMD-group (32-bit) = 256 x 16-bit | 255 GPRs per thread (tunable via --maxrregcount) |
| **L1 data cache** | 32 KB/core, ~2 cy latency | ~16 KB texture cache per TEX unit | 8 KB L1, ~13 cy (~10 ns) sequential; ~64 cy dep-chain pointer-chase | L1 unified with smem (configurable split) |
| **L2 cache** | 1 MB/core, 6.8-16 cy | Not separately measured | 768 KB/core, ~92 cy (16 KB pointer-chase) | 48 MB (AD104), L2 hit ~92-123 cy (LDG.E) |
| **L3 / SLC** | 32 MB shared, 23-32 cy | N/A (UMA, no L3) | 8 MB system-level cache, ~110 cy | N/A (L2 is last-level on-chip) |
| **DRAM latency** | ~219 cy (64 MB pointer-chase) | ~100-400 cy (TEX clause, hidden by wavefront interleave) | Not separately measured beyond SLC | Not separately measured |
| **Memory bandwidth** | 43 B/cyc (L2 zmm load); 23 B/cyc (DRAM zmm load) | ~8.5 GB/s (DDR3-1066 UMA shared) | Not measured | Not measured (504 GB/s spec) |
| **Shared/local memory** | N/A (CPU) | 32 KB LDS per SIMD, ~150 GB/s | 60 KB LDS, ~28 cy per access (~22 ns) | LDS: 28.03 cy; configurable split with L1 |
| **Peak FP32 (measured)** | 668 GFLOPS (16-thread aggregate) | ~4.87 GFLOPS peak (2 SIMD x 5 ops x 487 MHz) | Not directly measured (theoretical ~2.6 TFLOPS) | Not directly measured (theoretical ~40 TFLOPS) |

---

## 2. Dot Product Comparison

Dot products are a critical inner-loop operation for the NeRF MLP across all four platforms. The measurement campaign reveals strikingly different strategies.

### Zen 4 -- Three performance tiers

Zen 4 has the richest dot product instruction set, with three cleanly separated tiers:

| Tier | Instruction | Latency | Throughput | Data type |
|------|-------------|---------|------------|-----------|
| Legacy FP (slow) | DPPS xmm | 5.36 cy | 1.9 cy | FP32 |
| Legacy FP (slow) | DPPD xmm | 3.36 cy | 1.5 cy | FP64 |
| BF16 (modern) | VDPBF16PS zmm | 2.93 cy | 0.72 cy | BF16 -> FP32 accum |
| VNNI integer (fastest) | VPDPBUSD zmm | 1.98 cy | 0.48 cy | INT8 -> INT32 accum |
| VNNI integer (fastest) | VPDPWSSD zmm | 1.92 cy | 0.48-0.53 cy | INT16 -> INT32 accum |

The VNNI integer path is width-invariant: 128-bit, 256-bit, and 512-bit forms all
achieve ~0.48 cyc throughput. This contradicts the general double-pump model where
512-bit should be 2x slower, suggesting the VNNI unit is natively 512-bit. At 512-bit
width, VPDPBUSD processes 64 INT8 multiply-adds per instruction at 0.48 cyc throughput --
approximately 160 INT8 ops/cycle.

### TeraScale-2 -- DOT4 as a VLIW primitive

TeraScale-2 has a fundamentally different dot product: DOT4_IEEE occupies all four vec
slots of a VLIW5 bundle and completes a full vec4 dot product in 1 cycle with PV
forwarding. There is exactly one dot product shape -- FP32 vec4 -- and it is deeply
integrated into the ISA:

| Instruction | Latency | Throughput | Notes |
|-------------|---------|------------|-------|
| DOT4_IEEE | 1 cycle | 1/cycle | Uses all 4 vec slots; result available next cycle via PV |

For sub-word types, the fast path is UBYTE_FLT unpack (4 bytes in 1 cycle across
x/y/z/w slots) followed by MULADD_IEEE on the next cycle via PV forwarding. This gives
4 INT8-backed FP32 multiply-accumulates per 2 VLIW cycles.

### Apple AGX -- simdgroup matrix multiply

Apple's GPU lacks a dedicated scalar dot product instruction. The closest hardware
primitive is `simdgroup_matrix_multiply_accumulate` (8x8 FP32), which processes 512
MADs per call in ~32 GPU cycles. However, this unit is non-pipelineable: throughput
equals latency at ~32 cycles, with zero ILP benefit from independent accumulators.

For scalar FP32, the GPU approach would be serial fmul+fadd chains at ~1.75 cy/fmul
and ~2.2 cy/fadd (dep-chain). On the CPU side, NEON `vfmaq_f32` achieves 0.237 ns per
FP32 value in a 4-wide dep chain -- 4.2x faster than scalar f64.

### SM89 (Ada) -- dp4a and tensor cores

SM89 provides two dot product paths:

| Instruction | Latency | Type | Notes |
|-------------|---------|------|-------|
| IDP.4A.S8.S8 (dp4a) | 4.53 cy | INT8 x 4 -> INT32 | Scalar pipeline, all signedness combos same speed |
| HMMA.16816.F16 | 42.14 cy/WMMA | FP16 tensor core | Fastest float tensor core path |
| HMMA.16816.F32 | 66.28 cy/WMMA | FP32-accum tensor core | 256 FMA operations per WMMA |
| IMMA.16816.S8.S8 | 34.06 cy/WMMA | INT8 tensor core | 2x faster than float tensor core |
| IMMA.8832.S4.S4 | 28.05 cy/WMMA | INT4 tensor core | Fastest tensor core operation |

No FP32 scalar dot product instruction exists on SM89. FP32 dot products are built from
FFMA chains at 4.53 cy/op or routed through tensor cores.

### Cross-platform fast path summary

| Platform | Fastest dot product path | Effective throughput | Data type |
|----------|------------------------|---------------------|-----------|
| Zen 4 | VPDPBUSD zmm (VNNI) | ~160 INT8 ops/cycle | INT8 |
| Zen 4 | VDPBF16PS zmm (BF16) | ~66.5 BF16 ops/cycle | BF16 |
| TeraScale-2 | DOT4_IEEE | 1 vec4 dot/cycle (8 FP32 ops/cycle) | FP32 |
| Apple AGX | simdgroup_matmul 8x8 | 512 MADs / ~32 cycles (~16 MADs/cycle), non-pipelineable | FP32 |
| SM89 | IMMA.8832.S4.S4 (tensor core) | INT4 tensor core, 28 cy/WMMA | INT4 |
| SM89 | IDP.4A (dp4a) | 4.53 cy per 4 INT8 MADs | INT8 |

---

## 3. Memory Hierarchy Comparison

### Latency by level

| Level | Zen 4 (7945HX) | TeraScale-2 (HD 6310) | Apple M1 CPU | Apple M1 GPU | SM89 (Ada) |
|-------|----------------|----------------------|-------------|-------------|-----------|
| **Register** | 0 cy | 0 cy (PV forwarding) | ~0.3 ns (~1 cy) | not measured | ~2 cy (MOV) |
| **L1** | ~2 cy (4-32 KB) | ~16 KB TEX cache (latency not measured) | 0.5 ns (~1.6 cy, load-use chain) | ~10 ns (~13 cy, 8 KB L1, sequential) | not separately measured |
| **L1 dep-chain** | ~2 cy | not measured | not measured | ~49 ns (~64 cy, pointer-chase 4 KB) | not measured |
| **L2** | 6.8-16 cy (64 KB-1 MB) | N/A | not measured | ~71 ns (~92 cy, 16 KB pointer-chase) | ~92-123 cy (LDG.E global load) |
| **L3 / SLC** | 23-32 cy (2-16 MB) | N/A | not measured | ~85 ns (~110 cy, 256 KB pointer-chase) | N/A |
| **Shared/LDS** | N/A | ~2-4 cy expected (32 KB LDS) | N/A | ~22 ns (~28 cy, 60 KB LDS) | 28.03 cy (LDS) |
| **Constant cache** | N/A | ~1-4 cy (KCACHE) | N/A | N/A | 70.57 cy (LDC dep chain) |
| **L3 boundary** | 78 cy (32 MB) | N/A | not measured | not measured | N/A |
| **DRAM** | 219 cy (64 MB) | ~100-400 cy (hidden by clause interleave) | not measured | not measured | not measured |
| **TLB reach** | 288 KB (L1 dTLB), 12 MB (L2 TLB) | not measured | not measured | not measured | not measured |
| **Full page walk** | 174 cy | not measured | not measured | not measured | not measured |

### Bandwidth

| Path | Zen 4 | TeraScale-2 | Apple M1 | SM89 |
|------|-------|-------------|----------|------|
| **L1 load** | 39-43 B/cyc (zmm/ymm) | not measured | not measured | not measured |
| **L2 load** | 41-43 B/cyc | not measured | not measured | not measured |
| **L3 load** | 41-42 B/cyc | not measured | not measured | not measured |
| **DRAM load** | 23-26 B/cyc | ~8.5 GB/s (DDR3-1066 UMA) | not measured | not measured (504 GB/s spec) |
| **L1 store** | 51-65 B/cyc (zmm) | not measured | not measured | not measured |
| **DRAM store** | 14 B/cyc (zmm) | not measured | not measured | not measured |
| **NT store benefit** | None (single-core) | not measured | not measured | not measured |
| **LDS bandwidth** | N/A | ~150 GB/s | not measured | not measured |

### Architectural notes

- **Zen 4**: ymm slightly beats zmm at L1 and DRAM (native 256-bit path), but they converge at L2/L3. Store forwarding is free at same-size, but narrow-to-wide fails with a 9.22 cy penalty.
- **TeraScale-2**: UMA (shared system memory) is the single biggest constraint. The GPU is 62-75% idle under shader load because the CPU side cannot feed commands fast enough. LDS at ~150 GB/s internal bandwidth is the only fast local memory.
- **Apple M1 GPU**: L1 global loads (10 ns, ~13 cy) are actually faster than LDS access (~22 ns, ~28 cy) for warm single-thread dep chains -- the opposite of the usual GPU assumption. The AGX `wait` instruction overhead makes dep-chain pointer-chase L1 latency (64 cy) much worse than sequential L1 latency (13 cy).
- **SM89**: L2 is the effective last-level cache at 48 MB. Global loads via `LDG.E` show 92-123 cy; `__ldg()` via the read-only constant cache achieves ~33 cy. Shared memory (LDS) at 28 cy is faster than global.

---

## 4. Compute Density

Operations per cycle per core/SM/SIMD, where measured.

### FP32 ops/cycle

| Metric | Zen 4 | TeraScale-2 (per SIMD) | Apple M1 GPU | SM89 |
|--------|-------|----------------------|-------------|------|
| FP32 FMA throughput | 4.2/cycle (ymm); ~2.1/cycle (zmm, double-pumped) | 4/cycle (4 vec FMA) + 1 trans = 5 total | ~1.0/cycle (fmul throughput) | not isolated per-SM |
| FP32 add throughput | ~4/cycle (ymm) | 4/cycle (vec slots) | ~0.7/cycle (fadd throughput, 1.42 cy/op) | not isolated |
| Peak FP32 MAD/cycle | ~8.3 (ymm, FMA = 2 FLOP) | 5 (4 vec MAD + 1 trans) | not measured | not measured per-SM |

### INT8 ops/cycle

| Metric | Zen 4 | TeraScale-2 | Apple M1 | SM89 |
|--------|-------|-------------|----------|------|
| INT8 dot product | ~160 ops/cycle (VPDPBUSD zmm) | 4 UBYTE_FLT unpack/cycle + 4 MULADD/cycle = ~4 MACC/2 cycles | not measured | dp4a: 4 INT8 MADs per IDP.4A instruction |
| INT8 tensor core | N/A | N/A | not measured | IMMA.16816.S8.S8: 34 cy/WMMA |

### BF16 ops/cycle

| Metric | Zen 4 | TeraScale-2 | Apple M1 | SM89 |
|--------|-------|-------------|----------|------|
| BF16 dot product | ~66.5 ops/cycle (VDPBF16PS zmm, 2x FP32) | Not supported | Not measured | HMMA tensor core path only |

### INT4 ops/cycle

| Metric | Zen 4 | TeraScale-2 | Apple M1 | SM89 |
|--------|-------|-------------|----------|------|
| INT4 compute | Not supported (no INT4 instructions) | BFE_UINT extraction: 4/cycle (8 nibbles per register) | CPU nibble unpack: 0.316 ns (~1 cy) via add+sbfx | IMMA.8832.S4.S4: 28 cy/WMMA (fastest TC op) |

### Multi-core aggregate

| Metric | Zen 4 | TeraScale-2 | Apple M1 | SM89 |
|--------|-------|-------------|----------|------|
| Measured peak GFLOPS | 668 (16 threads, zmm FMA) | ~4.87 (theoretical, 2 SIMDs) | not measured | not measured |
| Scaling 8->16 threads | 1.4x (cross-CCD) | N/A (2 SIMDs only) | N/A | N/A |

---

## 5. Architectural Novelties Per Platform

### Zen 4 (AMD Ryzen 9 7945HX)

**AVX-512 double-pump without throttle.** Unlike Intel, which aggressively downclocks
under sustained AVX-512 workloads, Zen 4 maintains full clock speed with only 0.72%
drift across 10 sustained zmm FMA bursts. Zero scalar-to-zmm transition penalty.
512-bit instructions are split into 2 x 256-bit micro-ops, halving throughput but adding
zero latency.

**VNNI width invariance.** VPDPBUSD/VPDPWSSD achieve identical throughput (~0.48 cyc) at
128-bit, 256-bit, and 512-bit widths. This contradicts the double-pump model and
suggests the VNNI unit is natively 512-bit or uses a different scheduling path.

**Massive register elimination surface.** Over 150 instructions measure latency < 0.02 cy,
including all SIMD integer adds, compares, min/max, bitwise logic, shuffles, permutes,
and variable shifts. These are not free (throughput is 0.12-0.49 cy), but the OoO engine
completely hides their latency in dependency graphs.

**VPTERNLOGD universal boolean.** The ternary logic instruction matches the throughput of
simple AND/OR (~0.28 cyc at 512-bit) while expressing any 3-input boolean function.
There is zero reason to use multi-instruction boolean sequences.

**pdep/pext hardware fix.** 1.47 cy latency on Zen 4, down from ~18 cy (microcoded) on
Zen 1-3 -- a 12x improvement.

**Legacy SSE 6x throughput penalty.** VEX-encoded `VADDPS xmm` achieves 0.24 cy
throughput vs legacy-encoded `ADDPS xmm` at 1.46 cy. Any code compiled without `-mavx`
loses 6x FP SIMD throughput.

### TeraScale-2 (AMD Radeon HD 6310)

**VLIW5 slot packing.** The 5-slot bundle (4 general vec + 1 transcendental) allows
extraordinary instruction-level parallelism within a single clock. A full bundle can
execute 4 independent FP32 MADs plus one transcendental (RSQ, SIN, COS, LOG, EXP) in
one cycle. DOT4 occupies all 4 vec slots for a complete vec4 dot product in 1 cycle.

**PV (Previous Vector) forwarding.** Every ALU op achieves 1-cycle apparent latency via
the PV register, which holds the output of the preceding VLIW5 bundle. Dependent chains
rotate across x/y/z/w slots (MUL in x -> reads PV.x in y next cycle) with zero stall.
This eliminates register pressure for dependent chains: all latency probes need only
1-2 GPRs.

**UBYTE_FLT hardware byte-to-float.** Four opcodes (UBYTE0-3_FLT) each extract one byte
from a packed INT32 and convert to FP32 in a single cycle. All four fit in one VLIW5
bundle, unpacking RGBA8 to four float channels 3x faster than the shift+mask+convert
path.

**MUL_UINT24 in vec slots.** 24-bit integer multiply runs on any of the four general vec
slots at single-cycle throughput. Full 32-bit MULLO_INT is restricted to the
transcendental slot (1/cycle). This makes Q8.8 fixed-point via UINT24 the fast integer
multiply path.

**CPU starvation dominates.** The GPU is 62-75% idle under shader workloads because the
E-300 Bobcat CPU cannot submit draw commands fast enough. Adding CPU-side scheduling
complexity (e.g., a better VLIW5 packer) actually regressed performance from 450 to 141
FPS. The optimization strategy is to minimize CPU work and maximize GPU autonomy.

### Apple AGX (M1)

**SIMD-group matrix multiply (non-pipelineable).** `simdgroup_matrix_multiply_accumulate`
for 8x8 FP32 runs in ~32 GPU cycles and processes 512 MADs per call. However,
throughput equals latency: independent accumulators see zero ILP benefit, indicating
a structural hazard in the matrix unit. This is distinct from NVIDIA tensor cores, which
can overlap with other compute.

**L1 faster than LDS.** Warm L1 global loads (~13 GPU cycles) are faster than
threadgroup memory (LDS) access (~28 cycles) for single-thread dep chains. This inverts
the usual GPU assumption and means that for read-only data, global memory with L1
residency is preferred over explicit LDS staging.

**NEON double-double breakthrough (CPU).** Vectorised double-double on the M1 CPU
delivers 106-bit extended precision at 0.144 ns per value in throughput mode -- 6.6x
faster than scalar float64 (0.949 ns). The M-series OoO engine collapses a 4-op dep
chain to ~3.3 cycles by overlapping the two independent (hi, lo) halves within a 128-bit
NEON Q-register. This is the most surprising result in the entire measurement campaign.

**CPU integer ADD is 3x faster than CPU FP add.** M-series integer ALU runs at 0.315-
0.333 ns (~1 cycle) vs FP add at 0.945 ns (~3 cycles), indicating a much shorter
integer pipeline. CPU integer multiply also matches FP add latency (~3 cycles), unlike
the GPU where integer multiply is 3.57x slower than FP add.

**GPU FP16 widening overhead.** Despite having more 16-bit ALUs than 32-bit (per Asahi
RE), GPU dep-chain FP16 latency (~2.0 ns) is worse than FP32 fadd (1.695 ns) by
24-50%. The overhead comes from pipeline-level 16-to-32-bit widening, not instruction
count. FP16 advantage is in throughput with many independent threads, not dep chains.

**RSQRT non-pipelineable on M1.** `rsqrt(x)` (fast hardware approx) runs at throughput =
latency = 12 GPU cycles. Eight independent accumulators show zero ILP gain. The precise
variant (`metal::precise::rsqrt`) costs ~38 cycles via multi-step Newton-Raphson firmware.

### SM89 (NVIDIA Ada Lovelace)

**Tensor core hierarchy.** SM89 has a clear performance hierarchy across tensor core
data types: INT4 (28 cy/WMMA) > INT8 (34 cy) > FP16 (42 cy) > FP32-accum (66 cy) >
TF32 (67 cy via 2x HMMA). The INT4 path is 2.4x faster than the FP16 path for the
same operation shape. BF16 matches FP16-to-FP32 performance exactly (66.33 cy).

**HFMA2.BF16_V2 is the fastest scalar FMA.** At 4.01 cy, the BF16 packed half2 FMA
is faster than both FP32 FFMA (4.53 cy) and FP16 HFMA2 (4.54 cy). This is a micro-
architectural quirk: BF16 is not just a format -- it has a dedicated, slightly faster
execution path.

**Warp-level reduction in hardware.** `REDUX.SUM` completes a full 32-lane reduction in
60 cy, which is 2.6x faster than a manual SHFL butterfly tree. The hardware reduction
covers SUM, MIN, MAX, AND, OR, and XOR in a single instruction.

**FP64 is extremely slow.** DFMA at 54.48 cy and DADD at 48.47 cy place FP64 at roughly
12x the FP32 FMA latency. This is architecturally intentional: consumer Ada cards
(RTX 4070 Ti) have minimal FP64 hardware. For comparison, Zen 4 FP64 runs at the same
speed as FP32 (~3 cy dep-chain), and TeraScale-2 FP64 uses dual VLIW slots at roughly
half FP32 rate.

**Async global-to-shared copy.** `LDGSTS.E` (cp.async) moves data from global memory
directly to shared memory without an intermediate register, measured at 363 cy per
iteration including the synchronization barrier. The L2 prefetch-hint variants
(`LTC128B.128`) provide cache-line-level control.

**SFU transcendentals.** The Special Function Unit (MUFU) executes hardware
transcendentals at 17.5-41.5 cy: MUFU.EX2 (17.5 cy), MUFU.COS/SIN (23.5 cy),
MUFU.LG2 (39.5 cy), MUFU.RCP (41.5 cy), MUFU.RSQ (39.5 cy). These are significantly
faster than software libm equivalents but slower than AGX hardware RECIP (~11 cy) for
reciprocal.

---

## 6. Cross-Platform Optimization Implications

The NeRF renderer spans CPU and GPU paths (hash-grid lookup, MLP inference, volume
rendering). The measurements across these four architectures produce concrete
optimization rules:

### 6.1. Data type selection

**Use INT8 or BF16 wherever precision permits.** The fastest compute path on every
architecture except TeraScale-2 is a low-precision dot product:
- Zen 4: VNNI INT8 at ~160 ops/cycle, or BF16 at 2x FP32 throughput
- SM89: INT8 tensor core at 34 cy/WMMA, or dp4a at 4.53 cy per 4 MADs
- Apple AGX: simdgroup_matmul for FP32 (no native INT8 dot product)
- TeraScale-2: FP32 is the native precision; INT8 requires UBYTE_FLT unpack

For the MLP weights (24 KB FP32 / 12 KB BF16), BF16 halves memory footprint while
doubling Zen 4 multiply throughput at no extra cycle cost (VDPBF16PS).

### 6.2. Dot product strategy per platform

- **Zen 4**: Use VPDPBUSD/VPDPWSSD for quantized weights; VDPBF16PS for BF16 weights.
  Avoid legacy DPPS/DPPD (5-6x slower). Use zmm freely -- no throttle, no throughput
  penalty for VNNI.
- **TeraScale-2**: Use DOT4_IEEE for vec4 operations. For INT8 inputs, UBYTE_FLT unpack
  into FP32 then MULADD (4 MACC per 2 VLIW cycles). Keep scale factors in KCACHE.
- **Apple M1 GPU**: Route matrix work through simdgroup_matrix_multiply when batch size
  permits. For scalar, rely on fmul chains (1.0 cy throughput). On CPU, use NEON vfmaq
  for 4-wide FP32 at 0.237 ns/value.
- **SM89**: Route batch matrix work through HMMA tensor cores. For scalar dot products,
  use IDP.4A (dp4a) for INT8 or FFMA chains for FP32.

### 6.3. Memory hierarchy targeting

The MLP weight matrices are small (24 KB FP32, 12 KB BF16, 6 KB INT8). This has
different implications per platform:

| Platform | Fits in | Implication |
|----------|---------|-------------|
| Zen 4 | L1d (32 KB) | Weights are L1-resident. Compute-bound, not memory-bound. |
| TeraScale-2 | LDS (32 KB) | Pre-load weights into LDS via KCACHE. Still compute-bound. |
| Apple M1 GPU | L1 (8 KB) -- partially | INT8 weights (6 KB) fit in L1. BF16/FP32 need L2 (768 KB/core). |
| SM89 | Shared memory (up to 100 KB) | Stage weights into smem via LDGSTS, then compute from smem. |

For the hash grid (much larger, random access):

| Platform | Strategy | Key measurement |
|----------|----------|----------------|
| Zen 4 | Gather (vpgatherdd), but phase-separate from FMA (~10% overlap) | gather 4.71 cy (ymm), 9.01 cy (zmm) |
| TeraScale-2 | TEX unit as lookup table (4 FP32/cycle), clause interleaving | TEX latency hidden by wavefront switching |
| Apple M1 GPU | Global buffer with L1 residency preferred over LDS | L1 global: 13 cy; LDS: 28 cy |
| SM89 | Global LDG + L2 cache; smem staging for repeated access | LDG.E: 92-123 cy; LDS: 28 cy |

### 6.4. Branching vs branchless

| Platform | Branch mispredict cost | Recommendation |
|----------|----------------------|----------------|
| Zen 4 | 2.44 cy (predictable: 0.48 cy; unpredictable: 2.93 cy) | Branchless (cmov) for unpredictable paths |
| TeraScale-2 | Wavefront predication (PRED_SET + CNDE), no branch | Always branchless; CNDE is single-cycle |
| Apple M1 GPU | Hardware-scheduled SIMD divergence | Minimize divergence width; use select() |
| SM89 | Warp divergence (BSSY/BSYNC) | Minimize warp divergence; use predicated ops |

### 6.5. Atomic operations

Serial atomics have wildly different costs across platforms:

| Platform | Uncontended atomic cost | Recommendation |
|----------|----------------------|----------------|
| Zen 4 (CPU) | lock xadd ~3.8 cy (single-thread); 87x slowdown at 16 threads | Prefer per-thread accumulators, reduce at end |
| Apple M1 (CPU) | 1.912 ns (~6 cy) uncontended | Same: per-thread accumulation |
| Apple M1 (GPU) | TG atomic: 89.6 ns (~115 cy); global: 143.8 ns (~184 cy) | Avoid serial atomic chains on GPU entirely |
| SM89 | ATOMS.MIN.S32: ~4.37 cy (smem, single-thread) | Use warp-level REDUX (60 cy) to reduce before atomics |

### 6.6. Cross-architecture portable patterns

The measurements identify patterns that perform well universally:

1. **Phase-separate memory access from compute.** Zen 4 shows only 10% gather+FMA
   overlap. TeraScale-2 naturally phase-separates via clause switching. SM89 separates
   via LDGSTS async copies. Apple AGX has hardware scheduling.

2. **Keep working sets small.** All four architectures have fast local memory (L1 or
   LDS/smem), but the sizes differ dramatically: 32 KB (Zen 4 L1), 32 KB (TeraScale-2
   LDS), 8 KB (AGX L1), and configurable up to ~100 KB (SM89 smem). The MLP weights
   fit in all of them at INT8 precision.

3. **Prefer multiply-accumulate over separate multiply+add.** FMA is a native single
   instruction on all four platforms: Zen 4 VFMADD, TeraScale-2 MULADD_IEEE, Apple
   AGX `fma()`, SM89 FFMA.

4. **Minimize serial dependency chains.** All four architectures have some form of
   latency hiding (Zen 4 OoO, TeraScale-2 wavefront interleaving, AGX hardware
   scheduling, SM89 warp scheduling), but none can completely hide long serial chains.
   Unroll and use independent accumulators.

5. **Use the platform's native fast-path type.** FP32 on TeraScale-2 and AGX GPU; BF16
   or INT8 on Zen 4; INT8/INT4 tensor cores or FP32 FFMA on SM89. Forcing a non-native
   type incurs format conversion overhead on every platform.

---

*Generated 2026-04-04 from steinmarder measurement data across all four RE tracks.*
*Source documents: ZEN4_NOVEL_FINDINGS.md, ZEN4_MEASUREMENT_CAMPAIGN_STATUS.md,
ZEN4_PROBE_REFERENCE.md, FRONTIER_ROADMAP_R600_TERASCALE.md, DATA_TYPES_AND_COMPUTE_BLOCKS.md,
HW_CAPABILITIES.md, LATENCY_PROBE_RESULTS.md, APPLE_ARITH_LATENCY_SUMMARY.md,
APPLE_CROSS_DOMAIN_PERF_ATLAS.md, APPLE_WIDE_PRECISION_FINDINGS.md,
FRONTIER_ROADMAP_APPLE.md, SM89_SASS_INSTRUCTION_REFERENCE.md, MONOGRAPH_SM89_SYNTHESIS.md.*
