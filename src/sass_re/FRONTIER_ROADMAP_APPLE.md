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

Decision-grade evidence still missing:

- a single matrix that says when the compiler already emits the desired
  pattern and when a manual pattern still changes the build choice
- a normalized comparison of compiler output versus handwritten CPU or Metal
  patterns across runtime, code size, spills, and maintainability
- a shared ledger that marks each pattern family as compiler-sufficient,
  manual-only, or manual-preferred for the current hardware/toolchain
- a repo-wide note tying those build decisions back to
  [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md)

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
- dtype, quantization, and precision sweeps

Why this belongs in the same roadmap:

- the current neural scaffold already exposes conversion + runtime probes
- placement behavior plays the same role as lowering choices in the NVIDIA lane
- normalized matrices can compare the backend choices against CPU/Metal timings

Falsifiable hypotheses:

1. Explicit compute-unit selection yields stable placement differences for the
   same model and input shape.
2. dtype/quantization changes affect CPU, GPU, ANE placement differently rather
   than acting as a single universal toggle.
3. The probe outputs can be normalized into a repeatable matrix of backend and
   precision choices without needing heavy training loops.

### Rank 4: Normalization and publication

This lane must end with the same disciplined documentation trail as the SASS
track:

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

- [ ] **`xctrace gpu-counters` schema** — capture hardware GPU counters (ALU
  utilization, tile cache hit rates, memory bandwidth) alongside Metal schemas.
  Without this, we cannot answer "ALU-bound or memory-bound?" per variant.
  Script ready: `capture_gpu_counters.sh`. Needs next tranche run.
  See `APPLE_TRACK_GAP_ANALYSIS.md` §4.
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
- [ ] **Library mnemonic mining** — `otool -tv` on `Metal.framework`, `MPSCore`,
  `Accelerate` to mine real-world operation usage.
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

- [ ] Document every lane and bundle in the repo-level docs index (see `docs/README.md`)
  so the Apple track stands beside the NVIDIA and Ryzen stories.
- [x] Cross-link `APPLE_TRACK_GAP_ANALYSIS.md` from `APPLE_SILICON_RE_BRIDGE.md`
  and `STACK_MAP.md`.

## Tranche 1 (64-step deep dive)

`run_apple_tranche1.sh` now drives the CPU + Metal + neural lanes with eight phases
(`A`..`H`). Every phase writes its manifest, capability snapshot, and per-step logs.

### Canonical run

```sh
SUDO_ASKPASS=/Users/eirikr/bin/askpass \
src/apple_re/scripts/run_apple_tranche1.sh \
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
- [x] Phase 27 now exports `xctrace` metrics, PID-scoped `gpu_host_leaks.txt`,
  `gpu_host_vmmap.txt`, and `fs_usage_gpu_host.txt`, and `counter_latency_report.md`
  links to the refreshed inventory plus the promoted CUDA-grade bundle
  (`2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz`).
- [x] `ANALYSIS_NEXT_STEPS.md` captures how to replay the host diagnostics (fs probe,
  leak/vmmap capture, density normalization) so future reruns lock in the same SUDO
  keepalive story.

## What success looks like

- repeatable CPU timings and asm diffs that live beside the NVIDIA tables
- repeatable Metal probe timings, counter exports, and row-density normalization
- repeatable Core ML placement sweeps with clear backend matrices
- a documentation trail (bridge note + checklist + cross-links) that explains the
  Apple-side limits without forcing readers to re-interpret the NVIDIA story
- a new `docs/README.md` entry and `FRONTIER_ROADMAP_APPLE.md` cross-link so the
  Apple track sits shoulder to shoulder with the SASS and Ryzen tracks
