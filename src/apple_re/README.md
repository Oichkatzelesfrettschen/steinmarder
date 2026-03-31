# Apple Silicon Reverse-Engineering Toolkit

This subtree ports the `src/sass_re/` research method to Apple silicon without
pretending Apple exposes the same machine-code surfaces as CUDA.

## Goals

- Keep the good parts of `sass_re`: manifest-driven probes, repeatable scripts,
  normalized outputs, and side-by-side semantic and performance validation.
- Split Apple research into 3 lanes:
  - CPU lane: instruction-level asm plus latency and throughput microbenchmarks
  - GPU lane: Metal compute probes plus Xcode counter capture
  - Neural lane: Core ML placement and dtype studies across CPU, GPU, and ANE

## CUDA-Method Translation (M1-specific)

This lane keeps the same research posture as `sass_re`, but translated to tools
that exist on Apple silicon:

- `ncu`-style per-kernel evidence:
  - Apple equivalent: `xctrace` table exports + host CSV timing from
    `run_metal_probe_host.sh`
- `nsys`-style timeline/context evidence:
  - Apple equivalent: `xctrace` TOC + per-schema XML exports + `sample`,
    PID-targeted `spindump`, and PID-targeted `fs_usage`
- `compute-sanitizer`/valgrind-style memory safety lane:
  - Apple equivalent: ASan/UBSan build plus `leaks <pid>` and `vmmap <pid>`
    against a live target process
- SASS mnemonic frontier lane:
  - Apple equivalent: CPU assembly/disassembly deltas (`clang -S`, `otool`,
    `llvm-objdump`, `llvm-mca`) plus Metal shader artifact inspection
- cross-lane synthesis:
  - keep one tranche output dir with manifest, schema inventory, row-count
    metrics, timing CSVs, and neural placement probes

## Current contents

- `probes/apple_cpu_latency.c`
  - starter dependent-chain microbenchmark for integer add, FP add, and FP
    fused multiply-add
- `probes/apple_cpu_cache_pressure.c`
  - cache-pressure sweep probe with working-set and stride families around the
    likely L1/L2/LLC boundaries
- `scripts/audit_macos_re_env.sh`
  - checks the local Xcode, Metal, Homebrew, and Python ML stack
- `scripts/compile_metal_probe.sh`
  - compiles a starter Metal probe to `.air` and `.metallib`
- `scripts/build_metal_probe_host.sh`
  - compiles the Objective-C Metal host harness
- `scripts/run_metal_probe_host.sh`
  - runs the host harness against a selected `.metallib` and emits CSV timing
- `scripts/disassemble_apple_cpu_latency.sh`
  - disassembles the built CPU probe with `otool` and `llvm-objdump`
- `scripts/apple_capability_report.py`
  - emits a JSON capability matrix covering Apple/native + NVIDIA/AMD/Linux
    tool substitutions
- `scripts/check_python_ml_stack.py`
  - reports versions and basic availability for `mlx`, `coremltools`,
    PyTorch MPS, and JAX
- `scripts/bootstrap_neural_lane.sh`
  - creates the repo-local `.venv` with the neural-lane Python stack
- `scripts/neural_lane_probe.py`
  - exercises `torch`, `mlx`, `jax-metal`, and `coremltools` in one pass
- `scripts/analyze_tranche_mnemonics.py`
  - parses tranche disassembly outputs into opcode/mnemonic counts and a
  short interpretation report
- `scripts/analyze_xctrace_row_deltas.py`
  - computes schema row-count deltas between baseline and variant GPU traces
- `scripts/compare_xctrace_density_runs.py`
  - compares normalized xctrace density across successive promoted keepalive bundles
- `scripts/run_apple_tranche1.sh`
  - orchestrates the first 64-step deep-dive tranche across CPU, cache-pressure,
    Metal, and neural lanes with manifest outputs
- `scripts/run_next42_cpu_suite.sh`
  - wrapper for the CPU follow-on tranche with the next 42-step artifact map
- `scripts/run_next42_cpu_probes.sh`
  - CPU-lane draft runner for the add/load-store/shuffle/atomic/transcendental probe family
- `scripts/run_next42_cpu_cache_probes.sh`
  - CPU cache-pressure draft runner for working-set and stride sweeps, Time Profiler export, and cache-knee analysis
- `scripts/run_next42_metal_suite.sh`
  - wrapper for the Metal follow-on tranche with the next 42-step artifact map
- `scripts/run_next42_metal_probes.sh`
  - Metal-lane draft runner for the baseline, threadgroup, occupancy, and register-pressure variants
- `scripts/run_next42_neural_suite.sh`
  - wrapper for the neural follow-on tranche with the next 42-step artifact map
- `host/metal_probe_host.m`
  - minimal Metal host harness used for end-to-end GPU lane timing
- `requirements-neural.txt`
  - pinned package entrypoint for the repo-local neural lane
- `shaders/probe_simdgroup_reduce.metal`
  - starter SIMD-group probe kernel for the Metal lane
- `shaders/probe_threadgroup_heavy.metal`
  - threadgroup-memory-heavy variant for counter/timing deltas
- `shaders/probe_threadgroup_minimal.metal`
  - threadgroup-minimal variant for isolating occupancy with the smallest
    shared-memory footprint that still produces a stable trace
- `shaders/probe_occupancy_heavy.metal`
  - arithmetic/occupancy-heavy variant for counter/timing deltas

## Local bootstrap

This machine already has:

- Homebrew
- `cmake`
- `ninja`
- `llvm`
- `libomp`
- `openblas`
- `ccache`

It does **not** currently expose `metal` or `metallib` on `PATH`, and
`xcode-select -p` points at the Command Line Tools path. The first unblocker is
to select the full Xcode toolchain:

```sh
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
```

Then audit the full stack:

```sh
src/apple_re/scripts/audit_macos_re_env.sh
```

Prime sudo cache once (supports Touch ID if enabled on your host):

```sh
src/apple_re/scripts/prime_sudo_cache.sh
```

If the neural-lane packages are missing, bootstrap the repo-local environment:

```sh
src/apple_re/scripts/bootstrap_neural_lane.sh
```

Then probe the full neural lane:

```sh
.venv/bin/python src/apple_re/scripts/neural_lane_probe.py
```

## Build

When `APPLE` is true, CMake builds `sm_apple_cpu_latency` automatically:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target sm_apple_cpu_latency
./build/bin/sm_apple_cpu_latency --csv
```

## Tranche 1 orchestration (64 steps)

Run the first deep-dive tranche end to end:

```sh
SUDO_ASKPASS=/Users/eirikr/bin/askpass \
src/apple_re/scripts/run_apple_tranche1.sh \
  --phase all \
  --sudo keepalive \
  --iters 500000
```

Notes:

- `--sudo keepalive` starts a background `sudo -A` refresh loop and tears it
  down automatically at the end.
- `--sudo cache` uses one interactive `sudo -v` prime (Touch ID/password),
  then keeps the run non-interactive via cached `sudo -n` refresh.
  Cache mode now preserves sudo ticket state at teardown.
- `--sudo none` skips sudo priming and allows non-root evidence capture only.
- All steps write artifacts into
  `src/apple_re/results/tranche1_<timestamp>/steps/`.
- Run subsets with `--phase A,B,C` (or any comma-separated phase list).
- Phase `H` is the post-run evidence and comparison lane; it refreshes
  `xctrace` exports, compares `xctrace_row_density.csv` against the latest
  promoted bundle, and emits run / keepalive summaries plus bundle notes.
- Latest promoted C/D/E synthesis snapshot:
  `src/apple_re/results/blessed/2026-03-30_tranche1_r6_cde/`.
- Latest promoted keepalive snapshot:
  `src/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/`.
- Latest promoted variant frontier snapshot:
  `src/apple_re/results/blessed/2026-03-30_tranche1_r5_variants_frontier/`.
- Latest full-stack baseline snapshot:
  `src/apple_re/results/blessed/2026-03-30_tranche1_r4_m1_cuda_grade/`.
- The variant matrix now includes `threadgroup_minimal` to isolate occupancy
  with the smallest shared-memory footprint that still produces a stable
  trace.
- For lane-local drafts, run `scripts/run_next42_cpu_probes.sh` and
  `scripts/run_next42_cpu_cache_probes.sh` and `scripts/run_next42_metal_probes.sh`
  when you want just the CPU, cache-pressure, or Metal artifact sets without
  the full 64-step synthesis pass.
- Step 27 emits structured trace artifacts:
  `xctrace_trace_health.csv`, `xctrace_schema_inventory.csv`,
  `xctrace_metric_row_counts.csv`, and per-schema XML exports.
- Step 30 now also emits variant-vs-baseline delta artifacts:
  `xctrace_row_deltas.csv` and `xctrace_row_delta_summary.md`.
- Variant focus interpretation and ranked next actions are documented for r5
  in `ANALYSIS_NEXT_STEPS.md` and `RUN_SUMMARY.md`, while the r6 C/D/E bundle
  adds `xctrace_row_density.csv` plus a fresh `ANALYSIS_NEXT_STEPS.md`.
- Post-run mnemonic analysis:
  `python3 src/apple_re/scripts/analyze_tranche_mnemonics.py <run_dir>`
  writes `cpu_mnemonic_counts.csv`, `metal_air_opcode_counts.csv`, and
  `mnemonic_interpretation.md` into that run directory.
- If GUI askpass is unavailable in terminal automation, use `--sudo none` to
  complete non-root evidence capture without hanging.

## Recommended workflow

1. CPU lane
   - build `sm_apple_cpu_latency`
   - disassemble it with `scripts/disassemble_apple_cpu_latency.sh`
   - correlate timing deltas with compiler flags and asm
2. GPU lane
   - compile `shaders/probe_simdgroup_reduce.metal`
   - wrap it in a tiny host harness
   - capture timings and counters with Xcode Metal tools
3. Neural lane
   - bootstrap the repo-local `.venv`
   - run `scripts/neural_lane_probe.py`
   - convert controlled graphs with `coremltools`
   - compare `CPU only`, `CPU and GPU`, and `all compute units`
   - use `check_python_ml_stack.py` to verify the Python-side toolchain

## What to expect from Apple

- CPU mnemonics and timing tables are realistic.
- GPU timing tables and counter atlases are realistic.
- ANE timing and placement studies are realistic.
- Public Apple-GPU or ANE machine-ISA inventories are not currently a safe
  assumption, so this toolkit is intentionally biased toward emitted asm,
  shader artifacts, counters, and runtime behavior instead.
- `jax-metal` is still experimental in Apple's own documentation. On this
  machine it imports and exposes `METAL:0`, but the sample compute path still
  trips a StableHLO runtime mismatch, so treat JAX as a sidecar rather than
  the primary neural-lane stack for now.
- `torch` is pinned to `2.7.0`, which matches the version Core ML tooling
  explicitly reports as its most recently tested Torch release.
- `jax` and `jaxlib` are pinned to `0.4.38` to match the current Apple
  `jax-metal 0.1.1` compatibility guidance.
