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
- Metal lane starter shader: `../apple_re/shaders/probe_simdgroup_reduce.metal`
  plus `../apple_re/scripts/compile_metal_probe.sh`
- Neural lane bootstrap: `../apple_re/scripts/bootstrap_neural_lane.sh` and
  `../apple_re/scripts/neural_lane_probe.py`
- Environment and diagnostics scaffolding: `../apple_re/scripts/audit_macos_re_env.sh`
- `run_apple_tranche1.sh` drives the 62-step deep dive with explicit phase
  slicing, `ANALYSIS_NEXT_STEPS.md` documents the host diagnostics workflow,
  and the current blessed bundles already capture density deltas, trace
  exports, and PID-scoped host captures.

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

- [ ] Expand `sm_apple_cpu_latency` with explicit load/store, shuffle, and atomic
  subchains plus transcendental tests.
- [ ] Keep the CPU lane results in CSV/TSV tables that mirror the existing SASS
  timing tables and disassemble targets with `objdump`.
- [ ] Add the FS-event probe the host harness now references so `fs_usage_gpu_host.txt`
  is never empty under keepalive runs.
- [ ] Add the Metal variants: threadgroup-heavy, occupancy-heavy, register-pressure-heavy,
  and the new register-pressure-light variant focused on occupancy deltas with minimal
  threadgroup work.
- [ ] Add the threadgroup-minimal variant to isolate occupancy with the smallest
  threadgroup footprint that still produces a stable Metal trace.
- [ ] Normalize `xctrace_row_density.csv` (rows/sec) and `xctrace_row_delta_summary.md`
  into the Metal variant reports so density bias is communicated clearly.
- [ ] Grade the stage-27 harness to emit `xctrace` metrics plus PID-scoped `gpu_host_leaks.txt`
  / `gpu_host_vmmap.txt` captures every rerun of the tranche.
- [ ] Keep `ANALYSIS_NEXT_STEPS.md`, `counter_latency_report.md`, and the zipped bundle
  referenced in the README/docs index so downstream reviewers can grab the CUDA-grade
  bundle.
- [ ] Document every lane and bundle in the repo-level docs index (see `docs/README.md`)
  so the Apple track stands beside the NVIDIA and Ryzen stories.

## Tranche 1 (62-step deep dive)

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
  links to the refreshed inventory plus the CUDA-grade bundle
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
