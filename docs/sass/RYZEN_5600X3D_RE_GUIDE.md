# Ryzen 5600X3D Reverse-Engineering Guide

This guide maps the `src/sass_re/` research style to AMD Zen 3 with 3D V-Cache
(`Ryzen 5 5600X3D` class systems).

## Core translation

Keep the method, not the vendor specifics:

- probe corpus
- normalized results
- tool/version stamps per run
- semantic validation + profiler sidecars
- reproducible command surfaces

## Main lanes

- execution lane
  - integer/FP/branch/atomic probe families
  - compiler and flag sweep comparisons
- cache lane
  - L1/L2/L3 + V-Cache boundary sweeps
  - stride/random pressure and reuse-pattern studies
- profiler lane
  - PMU counter capture and timeline artifacts
  - safety diagnostics and sanitizers

## Recommended tools

- compile/disassembly
  - `clang`/`gcc`, `objdump`, `llvm-objdump`, `llvm-mca`
- Linux perf lane
  - `perf stat`, `perf record`, `perf report`
- AMD perf lane
  - `uProf` for AMD-specific PMU views
- diagnostics lane
  - `ASan/UBSan` (and `valgrind` where available)

## In-repo pointers

- `src/sass_re/FRONTIER_ROADMAP_RYZEN_5600X3D.md`
- `src/sass_re/FRONTIER_ROADMAP.md`
- `src/sass_re/RESULTS.md`
- `src/sass_re/APPLE_SILICON_RE_BRIDGE.md`

## First concrete target

Build and run a baseline matrix that captures:

1. CPU probe timings (`-O0/-O2/-O3/-Ofast`)
2. disassembly diffs by compiler/flag
3. `perf stat` counter snapshots
4. cache-regime sweeps (working-set and stride)
