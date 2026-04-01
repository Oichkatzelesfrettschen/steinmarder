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
  See `APPLE_TRACK_GAP_ANALYSIS.md` §4.
- [ ] **Metal dependent-chain latency probes** — add `probe_ffma_lat.metal`,
  `probe_fadd_lat.metal`, `probe_imad_lat.metal` with 32-deep dependency chains.
  Required to build a Metal analog to the SASS 448-mnemonic latency table.
- [ ] **Metal independent-accumulator throughput probes** — add 8-accumulator
  variants for FFMA, FADD, IMAD to separate latency from throughput.
- [ ] **`llvm-mca` in every keepalive run** — present in r5, missing from r7.
  Required for CPU-side latency-vs-predicted-throughput comparison.

### Important gaps

- [ ] **Metal simdgroup operation probes** — `simd_sum`, `simd_broadcast`,
  `simd_shuffle`, `simd_prefix_inclusive_sum` dependent-chain probes. Currently
  zero simdgroup intrinsics in any blessed AIR corpus.
- [ ] **Metal threadgroup memory latency probe** — pointer-chase in `threadgroup`
  buffer, analogous to SASS LDS latency measurement.
- [ ] **Metal atomic probes** — `atomic_fetch_add` on threadgroup and device buffers.
- [ ] **CPU pointer-chase cache sweep in blessed results** — `apple_cpu_cache_pressure.c`
  exists but L1/L2/LLC boundary measurements are not in any promoted run.
- [ ] **Metal register-light variant** — brackets density behavior below
  `register_pressure` (per `NEXT42_APPLE_TRANCHE.md` Phase C step 15).
- [ ] **`MTLCounterSet` host harness integration** — runtime GPU counter sampling
  around each dispatch, per-dispatch granularity.

### Medium gaps

- [ ] **Neural lane blessed run** — Core ML placement sweep, torch CPU vs MPS,
  MLX/JAX checks. Scripts exist in `scripts/`, no promoted results yet.
- [ ] **Metal `flag_matrix_sweep`** — `metal -O0`, `-O1`, `-O2`, `-Os` comparison
  on each shader variant. Shows compiler vs. manual patterns.
- [ ] **Library mnemonic mining** — `otool -tv` on `Metal.framework`, `MPSCore`,
  `Accelerate` to mine real-world operation usage, analogous to
  `mine_cudnn_library_mnemonics.sh`.
- [ ] **CPU integer multiply probe** — `UMULH`, `MADD`, `MSUB`, `SMULL` chains
  are absent from current mnemonic corpus (only `add`, `fadd`, `fmadd` covered).
- [ ] **CPU FP16 probes** — `FCVT`, `FMLA` (SIMD), half-precision dependent chains.
- [ ] **Run `analyze_tranche_mnemonics.py` on every keepalive** — now run manually
  for r7 (outputs: `cpu_mnemonic_counts.csv`, `metal_air_opcode_counts.csv`,
  `mnemonic_interpretation.md`). Should be part of the phase-H synthesis step.

### Documentation

- [ ] Document every lane and bundle in the repo-level docs index (see `docs/README.md`)
  so the Apple track stands beside the NVIDIA and Ryzen stories.
- [ ] Cross-link `APPLE_TRACK_GAP_ANALYSIS.md` from `APPLE_SILICON_RE_BRIDGE.md`
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
