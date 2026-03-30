# Apple Silicon Frontier Roadmap

This document scopes the highest-yield remaining work for the Apple silicon
research track after the first bridge note, the starter CPU benchmark, the
starter Metal probe, and the neural-lane bootstrap scripts.

It separates three goals that should not be conflated:

1. CPU instruction-level bring-up
2. Metal graphics / compute characterization
3. Neural placement and backend behavior

## Current frontier

Stable local facts:

- CPU lane starter exists in `../apple_re/probes/apple_cpu_latency.c`
- Metal lane starter exists in `../apple_re/shaders/probe_simdgroup_reduce.metal`
- Neural lane bootstrap and probe scripts exist in `../apple_re/scripts/`
- Environment-audit coverage exists in `../apple_re/scripts/audit_macos_re_env.sh`
- CPU disassembly helper exists in `../apple_re/scripts/disassemble_apple_cpu_latency.sh`
- Metal compile helper exists in `../apple_re/scripts/compile_metal_probe.sh`
- The current Apple track is intentionally scaffold-first, not claim-first

## Priority ranking

### Rank 1: CPU lane expansion

This is the highest-yield direct-local frontier because it gives the fastest
iteration loop and the clearest disassembly / timing feedback.

Target family:

- integer add and subtract
- floating-point add, multiply, and fused multiply-add
- load and store chains
- shuffle and lane-crossing operations
- atomic operations
- transcendental operations

Why this is the best first frontier:

- the current benchmark is already in place, so the next step is extension
  rather than new infrastructure
- CPU timing tables are the easiest way to establish a baseline measurement
  style for the Apple track
- asm diffs and compiler-flag sweeps can be recorded immediately beside the
  benchmark outputs

Falsifiable hypotheses:

1. A wider CPU probe corpus will surface stable timing deltas across
   optimization levels and target CPU selections.
2. Load/store and shuffle probes will show stronger compiler sensitivity than
   the current narrow starter chain.
3. Atomic and transcendental probes will give the first useful "slow path" and
   "special unit" references for the Apple lane.

### Rank 2: Metal graphics / compute frontier

This is the best second frontier because it gives the Apple GPU track a real
execution loop, even without assuming a public SASS-equivalent ISA.

Target family:

- shader lowering studies
- simdgroup reduction and broadcast patterns
- threadgroup memory behavior
- resource binding and argument-buffer experiments
- host-side timing and profiling
- Xcode counter capture and trace export

Why this is the right GPU frontier:

- the repository already has a starter Metal shader, so the missing piece is
  the host harness and measurement discipline
- GPU research should focus on emitted artifacts, timings, and counters rather
  than pretending the Apple GPU surface looks like NVIDIA SASS
- the same probe / results / normalization pattern from `sass_re` applies
  cleanly once a host runner exists

Falsifiable hypotheses:

1. A minimal host harness will make Metal probe timings reproducible enough to
   compare shader variants side by side.
2. Simdgroup and threadgroup-memory changes will produce observable timing or
   counter shifts even when the emitted artifact surface stays compact.
3. Counter capture will be more useful than disassembly for ranking GPU probe
   variants on Apple silicon.

### Rank 3: Neural placement frontier

This is the most framework-dependent frontier, but it is still a good fit for
the repository because it can be structured around repeatable conversion and
placement sweeps.

Target family:

- Core ML conversion and execution placement
- MPSGraph tensor experiments
- MLX and PyTorch MPS comparison points
- CPU, GPU, and ANE fallback behavior
- dtype, precision, and quantization sweeps

Why this belongs in the same roadmap:

- the existing neural scaffold already breaks the work into a bootstrap script
  and a probe script
- placement behavior is the Apple-side equivalent of "which lowering path did
  the compiler or runtime choose?"
- results can be normalized into matrices that compare backends, dtypes, and
  model shapes

Falsifiable hypotheses:

1. Explicit compute-unit selection will produce stable placement differences
   across the same model and input shape.
2. Dtype and quantization changes will affect CPU, GPU, and ANE placement in
   different ways rather than acting as a single universal toggle.
3. The probe output can be normalized into a repeatable matrix of backend,
   precision, and runtime choice without needing a heavyweight training loop.

### Rank 4: Result normalization and publication

This is the last step, not the first step.

The Apple track should end up with the same kind of documentation trail the
NVIDIA track already has:

- a readable README for the subtree
- a concrete roadmap
- a bridge note that explains the methodology transfer
- a results summary once the lane work starts producing measurements

## Checklist

- [ ] Expand the CPU lane with load/store, shuffle, atomics, and transcendental
  probes.
- [ ] Add a tiny Metal host harness so the shader probe can be timed and
  profiled end to end.
- [ ] Add Core ML graph generators for dtype and compute-unit sweep
  experiments.
- [ ] Record Apple-side results in a stable table format beside the existing
  SASS results style.
- [ ] Keep the bridge note linked from the SASS README so the Apple track stays
  discoverable.

## What success looks like

- repeatable CPU timings and asm diffs
- repeatable Metal probe timings and counter captures
- repeatable Core ML placement sweeps
- a documentation trail that explains the Apple-side limits clearly
- a cross-link path from the SASS docs into the Apple track without forcing
  readers to hunt for it

## Tranche 1 (42-step deep dive)

The first deep-dive tranche is now scripted in:

- [`../apple_re/scripts/run_apple_tranche1.sh`](../apple_re/scripts/run_apple_tranche1.sh)

It is structured as 42 explicit steps across seven phases (`A` through `G`) so
we can run the whole stack or rerun specific lanes without losing provenance.

### Canonical run

```sh
SUDO_ASKPASS=/Users/eirikr/bin/askpass \
src/apple_re/scripts/run_apple_tranche1.sh \
  --phase all \
  --sudo keepalive \
  --iters 500000
```

Touch ID / cached-sudo flow:

```sh
src/apple_re/scripts/prime_sudo_cache.sh
src/apple_re/scripts/run_apple_tranche1.sh --phase all --sudo cache --iters 500000
```

Default output: `src/apple_re/results/tranche1_<timestamp>/`

Latest blessed snapshot:

- `src/apple_re/results/blessed/2026-03-30_tranche1_r4_m1_cuda_grade/`
  (includes `xctrace_trace_health.csv`, `xctrace_schema_inventory.csv`,
  `xctrace_metric_row_counts.csv`, `RUN_SUMMARY.md`,
  `cpu_mnemonic_counts.csv`, and `mnemonic_interpretation.md`)

### Tool-stack mapping (non-overlapping, complementary)

- NVIDIA `ncu` / `nsys` class -> `xctrace` + host CSV timing + `powermetrics`
- AMD `uProf` class -> `xctrace Time Profiler` + `sample` + `spindump`
- Linux `valgrind` class -> `ASan/UBSan` + `leaks` + `vmmap`
- Linux `strace` / syscall tracing class -> `dtruss` (via `dtrace`)
- Linux FS trace class -> `fs_usage`

### Phase map

- `A` environment stamp + capability matrix + package inventory + sudo keepalive
- `B` CPU probe matrix build + disassembly + `llvm-mca` static modeling
- `C` CPU runtime timing + profiler + diagnostics
- `D` Metal host harness + compile + correctness + timing sweep
- `E` GPU trace + host-overhead capture + power sampling + synthesis note
- `F` neural lane bootstrap + placement probes (`torch`, `coreml`, `mlx`, `jax`)
- `G` manifest + quality gates + failed-step snapshot + linkage notes

### 42-step checklist

- [ ] 01 `A::sudo_prime`
- [ ] 02 `A::sudo_keepalive_start`
- [ ] 03 `A::machine_stamp`
- [ ] 04 `A::tool_matrix`
- [ ] 05 `A::package_inventory`
- [ ] 06 `A::capabilities_json`
- [ ] 07 `B::cpu_family_matrix`
- [ ] 08 `B::cpu_probe_inventory`
- [ ] 09 `B::compile_matrix_define`
- [ ] 10 `B::build_cpu_matrix`
- [ ] 11 `B::disassemble_cpu_matrix`
- [ ] 12 `B::llvm_mca_cpu_matrix`
- [ ] 13 `C::cpu_baseline_timing`
- [ ] 14 `C::hyperfine_cpu_timing`
- [ ] 15 `C::xctrace_cpu_profile`
- [ ] 16 `C::dtrace_dtruss_cpu`
- [ ] 17 `C::powermetrics_cpu`
- [ ] 18 `C::cpu_diagnostics`
- [ ] 19 `D::metal_host_harness_build`
- [ ] 20 `D::metal_variant_matrix_define`
- [ ] 21 `D::metal_compile_variants`
- [ ] 22 `D::metal_correctness`
- [ ] 23 `D::metal_timing_sweep`
- [ ] 24 `D::publish_metal_matrix`
- [ ] 25 `E::xctrace_gpu_baseline`
- [ ] 26 `E::xctrace_gpu_compare`
- [ ] 27 `E::extract_xctrace_metrics`
- [ ] 28 `E::host_overhead_capture`
- [ ] 29 `E::powermetrics_gpu`
- [ ] 30 `E::counter_latency_report`
- [ ] 31 `F::venv_rebuild`
- [ ] 32 `F::neural_probe_all`
- [ ] 33 `F::model_family_define`
- [ ] 34 `F::coreml_placement_sweep`
- [ ] 35 `F::torch_cpu_vs_mps`
- [ ] 36 `F::mlx_jax_checks`
- [ ] 37 `G::consolidate_manifest`
- [ ] 38 `G::quality_gates`
- [ ] 39 `G::rerun_failed_snapshot`
- [ ] 40 `G::synthesis_report`
- [ ] 41 `G::linkage_notes`
- [ ] 42 `G::sudo_teardown`
