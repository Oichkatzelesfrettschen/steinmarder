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

## Current contents

- `probes/apple_cpu_latency.c`
  - starter dependent-chain microbenchmark for integer add, FP add, and FP
    fused multiply-add
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
- `scripts/run_apple_tranche1.sh`
  - orchestrates the first 42-step deep-dive tranche across CPU, Metal, and
    neural lanes with manifest outputs
- `host/metal_probe_host.m`
  - minimal Metal host harness used for end-to-end GPU lane timing
- `requirements-neural.txt`
  - pinned package entrypoint for the repo-local neural lane
- `shaders/probe_simdgroup_reduce.metal`
  - starter SIMD-group probe kernel for the Metal lane

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

## Tranche 1 orchestration (42 steps)

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
- All steps write artifacts into
  `src/apple_re/results/tranche1_<timestamp>/steps/`.
- Run subsets with `--phase A,B,C` (or any comma-separated phase list).
- Latest promoted snapshot:
  `src/apple_re/results/blessed/2026-03-30_tranche1_r2/`.

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
