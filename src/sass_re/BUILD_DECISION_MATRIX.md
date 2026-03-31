# Build Decision Matrix

This document narrows the wider `steinmarder` reverse-engineering program to
the evidence that actually changes build choices.

The repo already has plenty of raw captures:

- SASS mnemonic frontiers and timing tables
- CUDA LBM kernel inventory, performance tables, and measured variants
- Apple CPU / Metal / Neural tranche outputs
- Ryzen 5600X3D cache and execution-lane roadmaps
- compiler-flag sweeps, profiler sidecars, and result bundles

What is still missing is a compact, explicit matrix that answers:

- when the compiler already emits the pattern we wanted
- when a manual pattern is still worth keeping
- what the performance, code-size, spill, and complexity tradeoff looks like
- which pattern families should be treated as reusable build templates

## Decision questions

For each probe family, the repo should answer these questions in one place:

1. Does the compiler already produce the desired structure at `-O2`, `-O3`, or
   with the current backend defaults?
2. Does a manual pattern materially improve runtime, spill count, or cache
   behavior?
3. Does the manual version reduce portability, increase compile time, or make
   the code harder to maintain?
4. Is the pattern a one-off research probe, or should it become a reusable
   production helper?

## Evidence dimensions

The missing decision-grade data should be normalized across every lane:

- runtime: wall time, latency, throughput, rows/sec, or counter-derived timing
- code shape: mnemonics, AIR opcodes, disassembly diffs, binary size, spill
  counts, and register pressure
- profiler sidecars: `xctrace`, `perf`, `uProf`, `ncu`, `nsys`, or equivalent
- correctness: sanitizer pass/fail, semantic diffs, and output stability
- maintainability: whether the manual transformation is simple, fragile, or
  compiler-dependent

## Manual patterns worth comparing

These are the manual families that should be compared directly against compiler
output instead of being assumed useful:

- dependent chains for latency characterization
- load/store and stride patterns for cache and memory pressure
- shuffle/broadcast patterns for lane-crossing and vectorization behavior
- atomic and memory-ordering idioms for synchronization cost
- transcendental approximations for special-unit and math-library behavior
- loop unrolling and accumulator splitting for ILP versus register pressure
- threadgroup / shared-memory shaping for Metal occupancy tradeoffs

## What is missing across the repo

### SASS track

- a cross-probe mapping that says which handwritten SASS-like patterns still
  outperform the compiler and which are already compiler-default behavior
- a standardized build-decision ledger that records code size, spills, and
  runtime together instead of treating them as separate notes

### CUDA LBM measurement checklist

- [ ] Baseline the handwritten CUDA LBM kernels against the closest
  template-generated or compiler-generated path on the same grid sizes so the
  comparison reflects identical work rather than different problem setups.
- [ ] Sweep the main kernel families already in the tree
  (`int8_soa`, `fp16_soa`, `bf16_soa`, `fp32_soa`, `fp64_soa`, `aa`, `mrt`,
  `coarsened`, and the tensor-core proxy) under the same launch bounds.
- [ ] Record `MLUPS`, `BW_GBS`, `BW_PCT`, `mass_drift`, `VRAM_MB`, occupancy,
  registers per thread, shared memory per block, and binary size in one CSV so
  build choices can be compared without reopening every run log.
- [ ] Capture `ncu`, `nsys`, and `objdump` sidecars for every CUDA run directory
  so the runtime, occupancy, and instruction-shape claims remain reproducible.
- [ ] Mark a kernel `compiler-sufficient` only if the generated path matches
  the handwritten version within the expected runtime and correctness band and
  does not lose on VRAM or occupancy.
- [ ] Mark a kernel `manual-preferred` only if the handwritten path wins on
  runtime, occupancy, memory footprint, or mass drift enough to justify the
  extra maintenance cost.
- [ ] Mark a kernel `research-only` when the variant is useful for learning but
  does not improve the production build decision enough to keep shipping it.
- [ ] Summarize each tranche in `cuda_variant_matrix.csv`,
  `cuda_launch_health.csv`, `cuda_occupancy_vs_bw.csv`,
  `cuda_benchmark_notes.md`, and `cuda_decision_summary.md`.
- [ ] Cross-link the final verdict back to `src/cuda_lbm/README.md` so the CUDA
  inventory and the build-decision matrix stay in sync.

#### CUDA measurement passes

Use one run root per decision tranche so the outputs can be compared without
guessing which file came from which sweep:

```sh
CUDA_RUN_ROOT="src/cuda_lbm/results/$(date +%Y%m%d_%H%M%S)_cuda_decision"
mkdir -p "$CUDA_RUN_ROOT"/{00_build,01_baseline,02_ncu,03_nsys,04_disasm,05_summary}
```

Pass 0: build, smoke, and spill gate

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  2>&1 | tee "$CUDA_RUN_ROOT/00_build/cmake.log"
cmake --build build --target lbm_bench lbm_test \
  2>&1 | tee "$CUDA_RUN_ROOT/00_build/build.log"
src/cuda_lbm/scripts/check_spills.sh "$CUDA_RUN_ROOT/00_build/build.log" \
  | tee "$CUDA_RUN_ROOT/00_build/spills.txt"
build/bin/lbm_test 2>&1 | tee "$CUDA_RUN_ROOT/00_build/lbm_test.log"
build/bin/lbm_bench --validate 2>&1 | tee "$CUDA_RUN_ROOT/00_build/lbm_validate.log"
build/bin/lbm_bench --regression 2>&1 | tee "$CUDA_RUN_ROOT/00_build/lbm_regression.log"
```

Expected layout:

- `00_build/cmake.log`
- `00_build/build.log`
- `00_build/spills.txt`
- `00_build/lbm_test.log`
- `00_build/lbm_validate.log`
- `00_build/lbm_regression.log`

Pass 1: baseline benchmark sweep and kernel catalog

```sh
build/bin/lbm_bench --all --grid 128 --output table \
  | tee "$CUDA_RUN_ROOT/01_baseline/all_128_table.log"
build/bin/lbm_bench --all --grid 128 --output csv \
  > "$CUDA_RUN_ROOT/01_baseline/all_128.csv"
build/bin/lbm_bench --list > "$CUDA_RUN_ROOT/01_baseline/kernel_list.txt"
build/bin/lbm_bench --occupancy > "$CUDA_RUN_ROOT/01_baseline/occupancy.txt"
```

Expected layout:

- `01_baseline/all_128_table.log`
- `01_baseline/all_128.csv`
- `01_baseline/kernel_list.txt`
- `01_baseline/occupancy.txt`

Pass 2: Nsight Compute profiling for the variant matrix

Quick path for the physics-valid SoA baseline:

```sh
sh src/cuda_lbm/scripts/profile_ncu_all.sh "$CUDA_RUN_ROOT/02_ncu"
```

Full matrix path for the handwritten and control variants:

```sh
for k in int8_soa fp16_soa fp16_soa_h2 int16_soa bf16_soa \
         fp32_soa_cs fp32_soa_fused fp64_soa int8_soa_lloydmax \
         int8_soa_lloydmax_pipe q16_soa q16_soa_lm q32_soa \
         int8_soa_mrt_aa; do
  src/cuda_lbm/scripts/profile_ncu.sh "$k" 128 "$CUDA_RUN_ROOT/02_ncu"
done
```

Expected layout:

- `02_ncu/<kernel>_128_<timestamp>.ncu-rep`
- `02_ncu/<kernel>_128_<timestamp>_metrics.csv`

Pass 3: Nsight Systems timeline capture

```sh
sh src/cuda_lbm/scripts/profile_nsys.sh --grid 128 --output "$CUDA_RUN_ROOT/03_nsys"
```

Expected layout:

- `03_nsys/lbm_bench_128_<timestamp>.nsys-rep`
- `03_nsys/lbm_bench_128_<timestamp>_kernel_summary.csv`
- `03_nsys/lbm_bench_128_<timestamp>_api_summary.csv`

Pass 4: disassembly and resource-usage capture

```sh
cuobjdump --dump-sass build/bin/lbm_bench \
  > "$CUDA_RUN_ROOT/04_disasm/lbm_bench.sass"
cuobjdump --dump-resource-usage build/bin/lbm_bench \
  > "$CUDA_RUN_ROOT/04_disasm/lbm_bench_resources.txt"
nvdisasm --print-code build/bin/lbm_bench \
  > "$CUDA_RUN_ROOT/04_disasm/lbm_bench_raw.sass" 2>/dev/null || true
```

Expected layout:

- `04_disasm/lbm_bench.sass`
- `04_disasm/lbm_bench_resources.txt`
- `04_disasm/lbm_bench_raw.sass`

Pass 5: synthesis files that turn raw outputs into build decisions

Write these into the summary directory after reviewing the pass outputs:

- `05_summary/cuda_variant_matrix.csv`
- `05_summary/cuda_launch_health.csv`
- `05_summary/cuda_occupancy_vs_bw.csv`
- `05_summary/cuda_benchmark_notes.md`
- `05_summary/cuda_decision_summary.md`

The summary pass should cite the build log, spill report, `ncu` sidecars,
`nsys` sidecars, and the disassembly bundle so the final verdict is traceable.

### Apple track

- a compiler-vs-manual matrix across Apple CPU, Metal, and Neural lanes
- an explicit record of when `clang`, Metal lowering, or Core ML placement
  already produce the desired shape without manual intervention
- a short list of manual transforms that are worth keeping because they change
  the build decision, not just the benchmark score

### Ryzen 5600X3D track

- a cache/stride decision matrix that ties working-set behavior to concrete
  source patterns and compiler outputs
- a clear split between reusable build helpers and one-off tuning experiments
- a normalized counter set that makes cache behavior part of the build choice
  rather than a side quest

## Recommended output files

The repo should eventually produce the following per-tranche artifacts:

- `compiler_vs_manual.csv`
- `manual_pattern_catalog.md`
- `build_decision_notes.md`
- `pattern_recommendations.md`
- `decision_lattice.csv`

Those files should be cross-linked from the Apple and Ryzen roadmaps so the
next person can answer “manual or compiler?” without re-reading every run log.
