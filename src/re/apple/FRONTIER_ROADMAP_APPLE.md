# Apple Silicon Frontier Roadmap

This document scopes the highest-yield remaining work for the Apple silicon
research track while mirroring the same `FRONTIER_ROADMAP` operating style:

1. Direct-lane results first (CPU, Metal, Neural)
2. Normalized metrics and artifacts next
3. Repeatable tranche runners and documentation last

## Current frontier

Stable launch points:

- CPU lane starter harness: `../apple_re/probes/apple_cpu_latency.c` plus
  `../apple_re/scripts/disassemble_apple_cpu_latency.sh`
- Cache-pressure lane starter harness:
  `../apple_re/scripts/run_next42_cpu_cache_probes.sh`
- Metal lane starter shader: `../apple_re/shaders/probe_simdgroup_reduce.metal`
  plus `../apple_re/scripts/compile_metal_probe.sh`
- Neural lane bootstrap: `../apple_re/scripts/bootstrap_neural_lane.sh` and
  `../apple_re/scripts/neural_lane_probe.py`
- Environment and diagnostics scaffolding: `../apple_re/scripts/audit_macos_re_env.sh`
- `run_apple_tranche1.sh` drives the 64-step deep dive with explicit phase
  slicing, `ANALYSIS_NEXT_STEPS.md` documents the host diagnostics workflow,
  and the current promoted bundles already capture density deltas, trace
  exports, and PID-scoped host captures.

Decision-grade evidence now exists for CPU timing, Metal counters, and the
first neural placement boundary. The remaining frontier is typed normalization:

- a single matrix that keeps every requested family visible, including
  unsupported and proxy-only rows
- a normalized comparison of compiler/native exposure versus lowered or
  framework-mediated exposure across CPU, Metal, and neural lanes
- a shared ledger that marks each format family as `native`, `lowered`,
  `framework_proxy`, `host_proxy`, `emulated`, `unsupported`, or `unknown`
- a repo-wide note tying those build decisions back to
  [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md)

The canonical typed ledger is now:

- [`APPLE_TYPED_BOUNDARY_ATLAS.md`](APPLE_TYPED_BOUNDARY_ATLAS.md)
- [`tables/table_apple_type_taxonomy.csv`](tables/table_apple_type_taxonomy.csv)
- [`tables/table_apple_type_boundary_matrix.csv`](tables/table_apple_type_boundary_matrix.csv)
- [`tables/table_apple_type_timings.csv`](tables/table_apple_type_timings.csv)

## Priority ranking

### Rank 1: CPU lane expansion

Target family:

- integer add/subtract, fused multiply-add, and mixed FP chains
- load, store, and cache-bound dependency chains
- shuffle/quad/warp-crossing operations
- atomics (shared/global, CAS, EXCH, INC/ADD)
- transcendental units (MUFU, LOG/E, EX2, SQRT)

Why this is the best first frontier:

- the existing `sm_apple_cpu_latency` probe already exercises a narrow
  family, so the lane grows by extension rather than rebuilding infrastructure
- CPU timing tables are the quickest place to demonstrate stable deltas
  across compiler flags, targets, and `llvm-mca`/`objdump` diffs
- the lane gives the cleanest ASM trace so the Apple track can publish a
  direct SASS-style table beside the NVIDIA results

Falsifiable hypotheses:

1. Wider load/store and shuffle sweeps will surface new timing tiers that stay
   stable between `-O2` and `-O3`, proving the compiler scheduling differences.
2. Atomics and transcendental probes will reveal a second, slower plateau that
   marks the special-unit border, not just the base ALU.
3. Explicit CPU flag sweeps will produce the same raw timing ordering regardless
   of threadgroup affinity, confirming the data dependencies.

Cache-pressure follow-on plan:

- add a dedicated cache probe family with working-set sweeps sized to the
  likely L1, L2, and LLC boundaries on the current machine
- record stride, working-set size, and miss-sensitive timing in a single
  `cache_pressure.csv` so cache-size inference is tied to evidence rather than
  memory of the chip family
- export the live run into a cache-specific `xctrace`/Time Profiler bundle and
  capture `powermetrics` alongside the timing table so latency and cache
  residency can be compared in the same run directory
- compare the cache-pressure sweep against the existing CPU plateau tables and
  call out where the miss-sensitive knee appears, rather than guessing the
  cache sizes from the vendor datasheet
- keep the follow-on result set small and explicit: `cache_probe_families.txt`,
  `cache_pressure.csv`, `cache_trace_health.csv`, and a short interpretation
  note that says whether the working set crosses the observed knee

### Rank 2: Metal graphics / compute frontier

Target family:

- shader lowering study and `metalcc` variant capture
- simdgroup/threadgroup memory reduction and broadcast patterns
- occupancy vs. register-pressure variants plus host-side timing
- argument buffer and resource binding stress
- `xctrace` export, counter snapshots, and density-normalized traces
- host-side FS probes for `fs_usage_gpu_host.txt` plus PID-scoped host captures

Why this is the right GPU frontier:

- the shader is already in-tree, so the missing pieces are the host harness and
  the profiling discipline
- row-density normalization (`xctrace_row_density.csv`) keeps variant comparisons
  stable even when raw row counts fluctuate
- dual Metal paths (threadgroup-heavy, occupancy-heavy, register-pressure-heavy)
  plus the new FS event probe prove the lane can deliver a CUDA-grade evidence
  bundle without assuming a public ISA

Falsifiable hypotheses:

1. The `fs_event` probe keeps `fs_usage_gpu_host.txt` populated under cached
   sudo keepalive runs, so downstream reviewers can verify trace health.
2. Occupancy-heavy vs. register-pressure variants will keep the same row-density
   delta even when the host workload oscillates, confirming the driver-level
   effect.
3. PID-scoped host captures (leaks/vmmap) plus `xctrace` metric export make step
   27 deterministic and replayable.

### Rank 3: Neural placement frontier

Target family:

- Core ML conversion and execution placement sweeps
- MPSGraph/MPSNNGraph tensor experiments
- MLX and PyTorch MPS comparison points
- CPU, GPU, and ANE fallback behavior
- dtype, quantization, precision, TF, BF, and MX sweeps

Why this belongs in the same roadmap:

- the current neural scaffold already exposes conversion + runtime probes
- placement behavior plays the same role as lowering choices in the NVIDIA lane
- normalized matrices can compare the backend choices against CPU/Metal timings
- the typed atlas now gives us a fixed place to record low-bit and proxy-only
  families honestly instead of dropping them from the promoted story

Falsifiable hypotheses:

1. Explicit compute-unit selection yields stable placement differences for the
   same model and input shape.
2. dtype/quantization changes affect CPU, GPU, ANE placement differently rather
   than acting as a single universal toggle.
3. The probe outputs can be normalized into a repeatable matrix of backend and
   precision choices without needing heavy training loops.

Primary remaining deliverables:

- replace the thin neural blessed summary with raw typed JSON/CSV artifacts
- sweep `CoreML CPU_ONLY`, `CPU_AND_GPU`, `CPU_AND_NE`, and `ALL` alongside
  PyTorch CPU/MPS and MLX GPU
- preserve planned rows for `tf32`, `mxfp8`, `mxfp6`, `mxfp4`, `mxint8`, `nf4`,
  and `fp4` even when the backend only exposes them as proxies

### Rank 4: Normalization and publication

This lane must end with the same disciplined documentation trail as the SASS
track:

- typed atlas note plus machine-readable Apple tables in `src/sass_re/tables/`
- bridge note (`APPLE_SILICON_RE_BRIDGE.md`)
- frontier checklist (`FRONTIER_ROADMAP_APPLE.md`)
- trunk README section and cross-links in the repo and docs index
- results manifest plus `KEEPALIVE_SUMMARY.md`/tarball so reviewers see the
  CUDA-grade evidence
- `ANALYSIS_NEXT_STEPS.md` capturing the host diagnostics sequence (fs probe,
  leaks/vmmap, `xctrace` exports)

## Checklist

### Completed

- [x] Expand `sm_apple_cpu_latency` with explicit load/store, shuffle, and atomic
  subchains plus transcendental tests (7 probe families in r5/r7).
- [x] Keep the CPU lane results in CSV/TSV tables that mirror the existing SASS
  timing tables and disassemble targets with `objdump` (r5: `cpu_mnemonic_counts.csv`).
- [x] Add the FS-event probe (`fs_usage_gpu_host.txt` populated in r7 via keepalive).
- [x] Add the Metal variants: threadgroup-heavy, occupancy-heavy, register-pressure-heavy,
  threadgroup-minimal — all present in r7.
- [x] Normalize `xctrace_row_density.csv` and `xctrace_row_delta_summary.md` (r7).
- [x] Grade the stage-27 harness to emit `xctrace` metrics plus PID-scoped
  `gpu_host_leaks.txt` / `gpu_host_vmmap.txt` (r7).
- [x] `counter_latency_report.md` and CUDA-grade bundle tarball present in r7.

### Critical gaps (change build decisions)

- [x] **`xctrace gpu-counters` schema** — captured via `--instrument 'Metal GPU
  Counters'`, 60,703 samples/counter over 20s. Key result: `occupancy_heavy`
  is ALU-balanced (peak 79%), not ALU-bound; Fragment Occupancy 100% for
  compute kernels = M1 TBDR routes compute through fragment pipeline. Average
  1.6% ALU due to host dispatch overhead confirmed. See `gpu_hardware_counters.md`.
- [x] **Metal dependent-chain latency probes** — `probe_ffma_lat.metal` (32-deep
  dep chain), `probe_tgsm_lat.metal` (TGSM pointer-chase). AIR confirmed.
- [x] **Metal independent-accumulator throughput probes** — `probe_ffma_tput.metal`
  (8 independent accumulators, 32 FMA ops/iter). AIR confirmed: 17 phi nodes.
- [x] **`llvm-mca` in every keepalive run** — `cpu_runs/llvm_mca_analysis.md` and
  `llvm_mca_summary.csv` in r7. 7 probes analyzed, M-series constants confirmed.

### Important gaps

- [x] **Metal simdgroup operation probes** — `probe_simd_reduce_lat.metal` with
  `simd_sum`, `simd_broadcast`, `simd_shuffle` dependent chains. AIR confirmed:
  `@air.simd_sum.f32`, `@air.simd_shuffle.u.i32`, `@air.simd_broadcast.f32`.
- [x] **Metal threadgroup memory latency probe** — `probe_tgsm_lat.metal`:
  32-deep pointer-chase through 256-word threadgroup array, wg.barrier. AIR confirmed.
- [x] **Metal atomic probes** — `probe_atomic_add.metal`: `probe_atomic_tgsm_lat`
  (32-deep serial chain on threadgroup `atomic_uint`) and `probe_atomic_device_lat`
  (same on device buffer). Key finding: Metal does NOT emit `atomicrmw`; the compiler
  emits AIR-specific intrinsics: `@air.atomic.local.add.u.i32` (TGSM, 33×) and
  `@air.atomic.global.add.u.i32` (device, 33×).
- [x] **CPU pointer-chase cache sweep in blessed results** — `cpu_runs/cache_pressure.csv`
  and `cache_hierarchy_analysis.md` in r7. L1→SLC ~128-256 KB, SLC→DRAM ~8-16 MB.
- [x] **Metal register-light variant** — `probe_register_light.metal`: single uint
  LCG accumulator. AIR confirms 6 total instructions, no FP, no temps.
- [x] **`MTLCounterSet` host harness integration** — `metal_probe_host.m` updated
  with `--list-counters` and `--counters SETNAME`. Uses `commandBuffer.GPUStartTime/
  GPUEndTime` (universally supported). AGXG13G: dispatch-boundary sampling unsupported.
  Key finding: GPU-actual is 14–22× faster than CPU wall-clock. See `gpu_commandbuffer_timing.md`.

### Medium gaps

- [x] **Neural lane blessed run** — `neural_lane_results.md` in r7. PyTorch MPS
  crossover ~1024×1024; coremltools 9.0 all compute units; MLX 0.31.1 GPU default;
  JAX METAL experimental. f16 anomaly flagged.
- [x] **Metal `flag_matrix_sweep`** — `run_metal_flag_sweep.sh` written and tested.
  -O0 → -O2: 420 → 48 instructions for `probe_ffma_lat` (stack spills eliminated).
- [x] **Library mnemonic mining** — runtime disassembly via `lldb` + `dyld_info`
  (dyld shared cache; `otool` doesn't work on macOS 13+). Critical finding:
  **Accelerate (libvDSP) uses AMX, not NEON**. The `vDSP_vadd` hot path contains
  53 AMX opcodes (0x00201xxx) and zero NEON float instructions for n≥20480.
  See `library_mnemonic_mining.md`.
- [x] **CPU integer multiply probe** — `MUL`, `MADD`, `MSUB`, `UMULH`, `SMULL` dep
  chains added to `apple_cpu_latency.c`. All measure **3 cycles** on M-series.
  MCA over-predicts by 60–70% (predicts 5 cyc). See `cpu_runs/integer_multiply_latency.md`.
- [x] **CPU FP16 probes** — `FCVT`, `FADD H`, `FMLA .8h` dep chains added to
  `apple_cpu_latency.c`. FCVT: ~3 cyc/conversion; scalar FADD f16: ~3 cyc;
  FMLA .8h SIMD: ~4 cyc. All same speed as f32/f64 counterparts. PyTorch MPS
  f16 8× anomaly is a framework issue, NOT hardware. See `cpu_runs/fp16_latency.md`.
- [x] **Run `analyze_tranche_mnemonics.py` on every keepalive** — automated in
  phase-H `mnemonic_synthesis` step of `run_apple_tranche1.sh`.

### Documentation

- [x] Document every lane and bundle in the repo-level docs index (`docs/README.md`).
  Apple track r7 blessed bundle now has dedicated artifact table covering CPU latency,
  Metal GPU measurements, neural lane, and library mnemonic mining (AMX discovery).
- [x] Cross-link `APPLE_TRACK_GAP_ANALYSIS.md` from `APPLE_SILICON_RE_BRIDGE.md`
  and `STACK_MAP.md`.

## Tranche 1 (64-step deep dive)

`run_apple_tranche1.sh` now drives the CPU + Metal + neural lanes with eight phases
(`A`..`H`). Every phase writes its manifest, capability snapshot, and per-step logs.

### Canonical run

```sh
SUDO_ASKPASS=/Users/eirikr/bin/askpass \
src/sass_re/apple_re/scripts/run_apple_tranche1.sh \
  --phase C,D,E \
  --sudo keepalive \
  --iters 500000
```

Phase E now carries the refreshed FS probe, the PID-scoped host captures, and
the register-vs-occupancy exports so the next bundle becomes CUDA-grade.

### Status snapshot (keepalive C/D/E)

- [x] CPU lane still records `xctrace` time profiler, `dtrace`, and `powermetrics`
  metadata for the expanded atomic/shuffle families.
- [x] Metal lane compares baseline, threadgroup-heavy, occupancy-heavy, and
  threadgroup-minimal, and register-pressure variants via
  `xctrace_row_delta_summary.md` + density CSV.
- [x] Register-pressure variants trade row density (`≈-103 r/s`) for throughput
  while the occupancy-heavy variant keeps the strongest density win (`+579 r/s`
  on `metal-gpu-state-intervals`), so the delta story stays stable through live
  host noise.
- [x] The new density comparison helper now compares successive keepalive
  bundles so the sign pattern and density gap remain explicit in the post-run H
  synthesis lane.
- [x] int4/uint4/fp4 CPU nibble-unpack probes (`nibble_unpack_u4`, `nibble_unpack_i4`):
  0.338 ns/op (add+and) and 0.316 ns/op (add+sbfx) — both 1-cycle dep chains.
- [x] long-double alias confirmed on AArch64: `fadd_dep_ld_aarch64` = 0.949 ns/op,
  indistinguishable from `fadd_dep_f64` (0.001 ns delta). `long double` == binary64.
- [x] `__float128` / `_Float128` confirmed absent from Apple Clang / AArch64.
  fp80 is x86-exclusive; the extended-precision path is always the NEON FPU.
- [x] Five-level wide-precision fastpath ladder measured (see Rank 0 above):
  scalar dd-2sum (1.348 ns/106-bit), FMA-dd (1.347 ns, same), f64×2 NEON
  (0.473 ns/value), f32×4 FMA (0.237 ns/value), vectorised dd (0.520 ns/106-bit).
- [x] BREAKTHROUGH: NEON-vectorised double-double at 0.520 ns/value dep-chain —
  and 0.144 ns/value in throughput mode (6.6× faster than scalar f64 for 106-bit precision).
  Updated: NEON DD throughput probe (f64x2_dd_throughput) confirmed 0.289 ns/vector = 0.144 ns/value.
- [x] int4/uint4/fp4 neural proxy timing via pytorch_proxy_cpu and pytorch_proxy_mps
  promoted from `planned` to `measured` across gemm_1024 and gemm_2048.
  fp4/int4/uint4 added to `TORCH_PROXY_FORMATS` in `neural_typed_matrix.py`.
- [x] All widened AGX GPU counter bundles (int4-64, uint4-64, fp4) synced to atlas.
  int4 ALU peak 93.7% / Top Limiter P99 98.7% — AGX is genuinely ALU-bound on int16.
- [x] Gap matrix fully updated: fp/80 (not_available), fp/128 (software_emulation),
  fp/160 alias (measured), fp/106 dd (measured), fp/128_neon (measured).
- [x] Phase 27 now exports `xctrace` metrics, PID-scoped `gpu_host_leaks.txt`,
  `gpu_host_vmmap.txt`, and `fs_usage_gpu_host.txt`, and `counter_latency_report.md`
  links to the refreshed inventory plus the promoted CUDA-grade bundle
  (`2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz`).
- [x] `ANALYSIS_NEXT_STEPS.md` captures how to replay the host diagnostics (fs probe,
  leak/vmmap capture, density normalization) so future reruns lock in the same SUDO
  keepalive story.

### Rank 0 (newly opened): Wide-precision and extended-FP fastpath lane

The int4/uint4/fp4 and wide-precision work revealed a new measurement lane that
did not exist in the original roadmap: the **emulated-type fastpath ladder**.

The key experimental finding (2026-04-02):

- `__float128` and `_Float128` are both absent from Apple Clang / AArch64.
  `long double` is binary64 (confirmed: 0.0001 ns delta vs `fadd_dep_f64`).
  There is no x87 80-bit path; fp80 is x86-exclusive hardware.
- The software fastpath for extended precision on M-series is NOT integer ALU
  softfp — it is the NEON FPU the whole time.
- Five levels were measured; the top level is a **novel finding**:

| probe | ns/value | ×f64 | precision | note |
|---|---|---|---|---|
| `fadd_dep_f64` | 0.992 | 1.00× | 53-bit | baseline |
| `dd_twosum_dep` | 1.348 | 1.36× | **106-bit** | scalar 2-sum; 4 FP ops/step |
| `dd_twosum_fma_dep` | 1.347 | 1.36× | **106-bit** | FMA neutral on linear dep chain |
| `f64x2_vadd_dep` | 0.473 | 0.48× | 53-bit × 2 | NEON: 2 doubles per instruction |
| `f32x4_vfma_dep` | 0.237 | **0.24×** | 24-bit × 4 | NEON: 4-wide FMA, 4.2× f64 |
| `f64x2_dd_twosum_dep` | **0.520** | **0.52×** | **106-bit × 2** | **BREAKTHROUGH** |

The last row is the core finding: **NEON-vectorised double-double delivers
106-bit extended precision at 0.520 ns per value — 1.91× more throughput than
scalar f64.** The M-series OOO engine collapses a 4-op dep chain to ~3.3 cycles
by overlapping 2 independent (hi, lo) pairs in the same 128-bit Q-register.
The 128-bit NEON register IS fp128-equivalent precision for this workload, and
it costs less per value than plain float64.

FMA is neutral for dep-chain double-double addition (both paths = 4.31 cycles).
FMA only pays out for independent-pair throughput or Veltkamp product splitting.

These probes are now in `../apple_re/probes/apple_cpu_latency.c` and their
results are in `tables/table_apple_type_timings.csv` under backend
`aarch64_neon_dd`.

#### Metal arithmetic latency sub-lane — COMPLETED (2026-04-02/03)

This sub-lane is now fully measured. See `APPLE_WIDE_PRECISION_FINDINGS.md` sections 7–12
and `APPLE_CROSS_DOMAIN_PERF_ATLAS.md` for the complete tables.

**Completed measurements:**
- [x] AGX FP32 fadd/fmul/fma dep-chain latency: 1.695 / 1.347 / 1.993 ns
- [x] AGX FP32 fadd/fmul/fma throughput (8 independent chains): 1.090 / 0.751 / 1.764 ns
  - FMA half-rate unit confirmed: only 1.13× ILP gain vs 1.79× for fmul
- [x] AGX INT32 imul dep-chain: 3.376 ns (2× fadd; separate pipeline)
- [x] AGX FP16 fadd/fmul/fma dep-chain: all ~2.0 ns (widening overhead dominates)
- [x] AGX DS-multiply step: 2.072 ns (FMA-bottlenecked; not 3 serial ops)
- [x] AGX DD genuine dep-chain: 3.348 ns/step (fadd-chain bottleneck)
- [x] AGX Veltkamp product split dep-chain: 11.739 ns/step (1.468 ns/FP-op)
- [x] AGX simdgroup_sum: 28.31 ns (~36 cycles for 32-lane cross-lane reduction)
- [x] AGX threadgroup atomic / global atomic: 89.6 / 143.8 ns
- [x] AGX genuine LDS latency (volatile fix): 43.66 ns/step (~22 ns/access, ~28 cycles)
- [x] AGX threadgroup_barrier cost: 25.44 ns (~33 cycles)
- [x] AGX warm L1 global latency (per-thread): 10.07 ns (~13 cycles) ← L1 faster than LDS!
- [x] FP8x8 / FP16x4 cascade probes: 295 ms / 17.1 ms (software emulation impractical)
- [x] Metal fast-math folding characterized: #pragma clang fp reassociate(off) fix documented

**Completed CPU measurements (extended, 2026-04-03):**
- [x] CPU fp16 scalar fadd: 0.945–0.980 ns (= fp32 latency; FEAT_FP16 zero-penalty)
- [x] CPU fp16 scalar fmul: 1.306 ns (slightly slower than fadd)
- [x] CPU int32/int64 mul: ~0.944–0.979 ns (= FP add tier; M-series multiply pipeline)
- [x] CPU sqrt (f64): 4.87 ns (~5 cycles, hardware FSQRT)
- [x] CPU log/exp: ~14.5–14.9 ns (libm software, ~15 cycles — no hardware transcendental)
- [x] CPU CRC32: 0.998 ns (~1 cycle, FEAT_CRC32 hardware)
- [x] CPU CLZ: 0.645 ns (sub-cycle effective latency)
- [x] CPU fmadd dep-chain: 1.260 ns (1.33× slower than fadd — FMA deeper on CPU too)
- [x] NEON DD throughput probe (4 independent pairs): 0.540 ns/4-pairs = 0.132 ns/pair
- [x] NEON DD throughput (f64x2 vector mode): 0.289 ns/vector = **0.144 ns/value** (fastest measured)
- [x] CPU Veltkamp DS-multiply dep-chain: 5.44–5.57 ns/step (CPU 2.16× faster than GPU)
- [x] CPU int ADD (all widths): 0.315–0.333 ns (3× faster than CPU FP add)

**Cross-domain atlas created:** `APPLE_CROSS_DOMAIN_PERF_ATLAS.md`
**AGX RE sources indexed:** `dougallj/applegpu` ISA docs, `philipturner/metal-benchmarks`,
Rosenzweig blog series, Asahi Linux hardware docs (L1=8KB/LDS=60KB/L2=768KB/L3=8MB confirmed)

#### Remaining wide-precision / cross-arch targets

- **Metal f32x4 Kahan reduction**: compensated sum shader via `float4` lanes — tests
  whether NEON vectorisation OOO benefit transfers to GPU SIMD lanes (hypothesis: no,
  GPU lanes are in-order within a simdgroup).
- **Cross-arch DD**: r600 TeraScale double-single on 32-bit VLIW5 f32 pairs via
  OpenCL/Rusticl — expected ~4–5× f32 overhead; compare against AGX DS-mul (2.07 ns).
- **Metal FP16 throughput probes** (independent accumulators) — expose the "more 16-bit
  ALUs than 32-bit" advantage that Rosenzweig identifies; dep-chain probes hide it.
- **Metal FMA throughput re-probe with 16 accumulators + AIR dump verification** —
  reconcile our 1.764 ns throughput with philipturner's "1-cycle FFMA" claim.

## What success looks like

- repeatable CPU timings and asm diffs that live beside the NVIDIA tables
- repeatable Metal probe timings, counter exports, and row-density normalization
- repeatable Core ML placement sweeps with clear backend matrices
- a documentation trail (bridge note + checklist + cross-links) that explains the
  Apple-side limits without forcing readers to re-interpret the NVIDIA story
- a new `docs/README.md` entry and `FRONTIER_ROADMAP_APPLE.md` cross-link so the
  Apple track sits shoulder to shoulder with the SASS and Ryzen tracks
