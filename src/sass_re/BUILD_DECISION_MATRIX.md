# Build Decision Matrix

This document narrows the wider `steinmarder` reverse-engineering program to
the evidence that actually changes build choices.

For the side-by-side living architecture map, see [`STACK_MAP.md`](STACK_MAP.md).

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

The canonical launcher is
[`src/cuda_lbm/scripts/run_cuda_decision_tranche.sh`](../cuda_lbm/scripts/run_cuda_decision_tranche.sh);
the commands below mirror the same run-root layout and pass structure.

Use one run root per decision tranche so the outputs can be compared without
guessing which file came from which sweep:

```sh
CUDA_RUN_ROOT="src/cuda_lbm/results/$(date +%Y%m%d_%H%M%S)_cuda_decision"
BUILD_DIR="${BUILD_DIR:-build}"
mkdir -p \
  "$CUDA_RUN_ROOT/00_build" \
  "$CUDA_RUN_ROOT/01_baseline" \
  "$CUDA_RUN_ROOT/02_ncu/baseline" \
  "$CUDA_RUN_ROOT/02_ncu/extended" \
  "$CUDA_RUN_ROOT/03_nsys" \
  "$CUDA_RUN_ROOT/04_disasm" \
  "$CUDA_RUN_ROOT/05_summary"
```

Pass 0: build, smoke, and spill gate

```sh
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
  2>&1 | tee "$CUDA_RUN_ROOT/00_build/cmake.log"
cmake --build "$BUILD_DIR" --target lbm_bench lbm_test \
  2>&1 | tee "$CUDA_RUN_ROOT/00_build/build.log"
sh src/cuda_lbm/scripts/check_spills.sh "$CUDA_RUN_ROOT/00_build/build.log" \
  | tee "$CUDA_RUN_ROOT/00_build/spills.txt"
"$BUILD_DIR/bin/lbm_test" 2>&1 | tee "$CUDA_RUN_ROOT/00_build/lbm_test.log"
"$BUILD_DIR/bin/lbm_bench" --validate 2>&1 | tee "$CUDA_RUN_ROOT/00_build/lbm_validate.log"
"$BUILD_DIR/bin/lbm_bench" --regression 2>&1 | tee "$CUDA_RUN_ROOT/00_build/lbm_regression.log"
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
"$BUILD_DIR/bin/lbm_bench" --all --grid 128 --output table \
  | tee "$CUDA_RUN_ROOT/01_baseline/all_128_table.log"
"$BUILD_DIR/bin/lbm_bench" --all --grid 128 --output csv \
  | awk 'BEGIN{flag=0} /^kernel,/{flag=1} flag' \
  > "$CUDA_RUN_ROOT/01_baseline/all_128.csv"
"$BUILD_DIR/bin/lbm_bench" --list > "$CUDA_RUN_ROOT/01_baseline/kernel_list.txt"
"$BUILD_DIR/bin/lbm_bench" --occupancy > "$CUDA_RUN_ROOT/01_baseline/occupancy.txt"
```

Expected layout:

- `01_baseline/all_128_table.log`
- `01_baseline/all_128.csv`
- `01_baseline/kernel_list.txt`
- `01_baseline/occupancy.txt`

Pass 2: Nsight Compute profiling for the variant matrix

Quick path for the physics-valid SoA baseline:

```sh
sh src/cuda_lbm/scripts/profile_ncu_all.sh "$CUDA_RUN_ROOT/02_ncu/baseline"
```

Full matrix path for the handwritten and control variants:

```sh
for k in int8_soa fp16_soa fp16_soa_h2 int16_soa bf16_soa \
         fp32_soa_cs fp32_soa_fused fp64_soa int8_soa_lloydmax \
         int8_soa_lloydmax_pipe q16_soa q16_soa_lm q32_soa \
         int8_soa_mrt_aa; do
  sh src/cuda_lbm/scripts/profile_ncu.sh "$k" 128 "$CUDA_RUN_ROOT/02_ncu/extended"
done
```

Expected layout:

- `02_ncu/baseline/<kernel>_128_<timestamp>.ncu-rep`
- `02_ncu/baseline/<kernel>_128_<timestamp>_metrics.csv`
- `02_ncu/extended/<kernel>_128_<timestamp>.ncu-rep`
- `02_ncu/extended/<kernel>_128_<timestamp>_metrics.csv`

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
cuobjdump --dump-sass "$BUILD_DIR/bin/lbm_bench" \
  > "$CUDA_RUN_ROOT/04_disasm/lbm_bench.sass"
cuobjdump --dump-resource-usage "$BUILD_DIR/bin/lbm_bench" \
  > "$CUDA_RUN_ROOT/04_disasm/lbm_bench_resources.txt"
cubin_path=$(find "$BUILD_DIR" -type f -name '*.cubin' | head -n 1 || true)
if [ -n "$cubin_path" ] && command -v nvdisasm >/dev/null 2>&1; then
  nvdisasm --print-code "$cubin_path" \
    > "$CUDA_RUN_ROOT/04_disasm/lbm_bench_raw.sass" 2>/dev/null || true
else
  printf '%s\n' \
    '# nvdisasm raw disassembly was not available for this run.' \
    '# Inspect the cuobjdump sidecars and the embedded cubin search path instead.' \
    > "$CUDA_RUN_ROOT/04_disasm/lbm_bench_raw.sass"
fi
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

## r600 / TeraScale-2 Decision Entries

### Pattern families compared against compiler output

| Pattern | Compiler emits at -O2? | Manual improvement? | Runtime delta | Spill impact | Decision |
|---------|----------------------|--------------------|--------------|-----------| ---------|
| UBYTE_FLT (byte→float) | No — compiler emits AND+SHIFT+INT_TO_FLT (3 ops) | Yes — single UBYTE0_FLT opcode | 3x fewer ALU cycles | None | **Keep manual: implemented in sfn_instr_alu.cpp** |
| MUL_UINT24 (24-bit int mul) | No — compiler emits MULLO_INT (trans-only, multi-cycle) | Yes — MUL_UINT24 (vec slot, single-cycle) | Free vec slot + faster | None | **Keep manual: implemented in sfn_instr_alu.cpp** |
| MULADD_M2/M4/D2 (power-of-2 scale) | No — compiler emits separate MUL+ADD | Yes — uses output modifier bits (free) | 1 fewer instruction | None | **Keep manual: sfn_peephole.cpp** |
| BFE_UINT/BFI_INT (bit-field ops) | Yes — compiler already emits via nir_op_ubfe/ibfe | No — already optimal | N/A | N/A | **Compiler wins: no manual pattern needed** |
| FLT16_TO_FLT32 (half conversion) | Yes — compiler uses native ops | No — conversion-only, no FP16 ALU | N/A | N/A | **Compiler wins: documented as conversion-only** |
| DOT4_IEEE (4-component dot) | Yes — compiler packs into 4 vec slots | No — already uses all 4 slots in 1 cycle | N/A | N/A | **Compiler wins: verified via latency probes** |
| PREV-chaining (MUL_PREV etc) | Partial — SFN uses PV forwarding extensively | Maybe — PREV variants save 1 GPR | <1% | -1 GPR | **Research: needs dependency analysis in SFN scheduler** |
| Scratch spill (MEM_SCRATCH) | No — SFN asserts on >123 GPRs | Yes — infrastructure exists, needs spill logic | Enables compute | Enables compute | **Critical: blocks Rusticl kernel execution** |

### Measured latencies (steinmarder dependent-chain methodology)

| Opcode | Math type | Slot | Latency | Throughput | Status |
|--------|-----------|------|---------|------------|--------|
| ADD_IEEE | FP32 | vec | 1 cycle | 4/cycle | Inferred (NIR folds chain) |
| MUL_IEEE | FP32 | vec | 1 cycle | 4/cycle | **Confirmed** via PV forwarding |
| MULADD_IEEE | FP32 | vec | 1 cycle | 4/cycle | **Confirmed** |
| DOT4_IEEE | FP32 | vec×4 | 1 cycle | 1/cycle | **Confirmed** |
| RECIPSQRT_IEEE | FP32 | trans | 1 cycle | 1/cycle | **Confirmed** |
| SIN | FP32 | trans | 1 cycle (hw) + 7 instr range reduction | 1/cycle | **Confirmed** |
| COS | FP32 | trans | 1 cycle (hw) + 7 instr range reduction | 1/cycle | **Confirmed** |
| MULLO_INT | INT32 | trans | 1 cycle | 1/cycle | **Confirmed** |
| MUL_UINT24 | UINT24 | vec | 1 cycle | 4/cycle | **Inferred** (same pipeline) |
| UBYTE0_FLT | UINT8→FP32 | vec | 1 cycle | 4/cycle | **Inferred** |
| BFE_UINT | UINT bit-field | vec | 1 cycle | 4/cycle | **Inferred** |
| ADD_64 | FP64 | vec pair | ~2 cycles (dual-slot) | 2/cycle | **Untested** |
| MUL_64 | FP64 | vec pair | ~2 cycles (dual-slot) | 2/cycle | **Untested** |

### Tool stack health

| Tool | steinmarder equivalent | Installed? | Working? | Notes |
|------|----------------------|-----------|---------|-------|
| R600_DEBUG | cuobjdump | Yes | Yes | Full ISA dump per shader stage |
| GALLIUM_HUD | ncu | Yes | Yes | CSV pipeline counters |
| AMDuProfCLI | nsys | Yes | Yes | IBS sampling for CPU hotspots |
| radeontop | nvidia-smi | Yes | Yes | GPU block utilization in real-time |
| apitrace | nsys trace | Yes | Yes | GL/VK API recording + replay |
| perf | perf | Yes | Yes | CPU PMU counters |
| piglit | N/A | Yes | Yes | Shader correctness + ISA capture |
| dEQP-VK | dEQP-VK | Yes | Yes | 1.7M Vulkan conformance tests |
| umr | N/A | No | N/A | AMD GPU register debugger — not in Debian |
| radeon_profile | N/A | No | N/A | Would need manual build |
