# Ryzen 5 5600X3D Frontier Roadmap

This roadmap scopes the first high-yield reverse-engineering and performance
characterization tranche for Zen 3 + 3D V-Cache, aligned with the same working
style used by `src/sass_re/` and the Apple bridge.

It separates three goals that should not be conflated:

1. Core execution-lane microarchitecture characterization
2. Cache hierarchy and 3D V-Cache behavior characterization
3. Toolchain and profiler normalization for repeatable publication

## Current frontier

Stable process targets:

- treat CPU probes like first-class research artifacts (build + disasm + timing)
- preserve semantic checks beside performance checks
- keep tool snapshots and version stamps in each run directory
- record profiler captures next to the exact binaries they describe

Decision-grade evidence still missing:

- a compiler-vs-manual matrix that says which source patterns should be
  handwritten and which should stay compiler-driven
- a normalized ledger of runtime, code size, spill count, and cache behavior
  so the build decision is visible in one table
- a shared note linking Ryzen cache sweeps to
  [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md)

## Priority ranking

### Rank 1: execution-lane probe matrix

Target families:

- integer ALU chains
- FP add/mul/fma chains
- branch and mispredict-sensitive kernels
- dependency-depth and ILP sweeps
- atomic and memory-ordering probes

### Rank 2: cache + 3D V-Cache matrix

Target families:

- L1/L2/L3/3D V-Cache boundary sweeps
- stride and random-access pressure
- streaming vs reuse regimes
- SMT-on/off and core pinning sensitivity

### Rank 3: profiler normalization

Target families:

- Linux-first: `perf` + `perf stat` + `perf record/report`
- AMD-first: `uProf` timeline and PMU lanes
- safety sidecar: sanitizers and memory diagnostics

## Tool-stack mapping

- `ncu/nsys` class (GPU timeline/counters) -> `perf`/`uProf` CPU PMU timelines
- `valgrind` class -> `ASan/UBSan` (+ optional `valgrind` where available)
- static scheduling class -> `llvm-mca` + compiler asm output diffs

## Checklist

- [ ] Add a dedicated `src/cpu_re_ryzen/` probe subtree (or equivalent lane)
- [ ] Build compiler-flag matrix (`-O0/-O2/-O3/-Ofast`, LTO toggles)
- [ ] Capture disassembly (`objdump`/`llvm-objdump`) per matrix cell
- [ ] Add baseline timing CSVs with pinning metadata
- [ ] Add `perf stat` counter packs for baseline probes
- [ ] Add `perf record` flame/timeline snapshots for hot probes
- [ ] Add `uProf` capture workflow for AMD-specific counter validation
- [ ] Add cache-size sweep probes for V-Cache behavior boundaries
- [ ] Add stride/random sweep probes for latency/bandwidth envelopes
- [ ] Add atomics + memory-order stress probes
- [ ] Add sanitizer lane and memory diagnostics lane
- [ ] Normalize output manifests and link from repo-level docs index

## What success looks like

- repeatable Zen 3/5600X3D timing and counter baselines
- cache-regime transition plots that clearly show V-Cache effects
- profiler artifacts that can be replayed and compared across runs
- a documentation trail that sits beside Apple and NVIDIA tracks without
  ambiguity
