# Apple Silicon Reverse-Engineering Guide

This document translates the success of `src/sass_re/` from CUDA/NVIDIA into a
research stack that fits macOS on Apple silicon.

## Core translation

Do **not** try to port the CUDA disassembly workflow literally. Port the method:

- probe corpus
- normalized results
- multiple measurement lanes
- semantic validation
- profiler and counter sidecars
- differential fuzzing when the hardware surface is not directly documented

That produces 3 Apple lanes:

- CPU lane
  - best place for mnemonic-level work
  - use `clang`, `otool`, `llvm-objdump`, `llvm-mca`, Instruments, and
    `powermetrics`
- GPU lane
  - use Metal compute probes, Xcode GPU counters, and shader profiling
  - expect strong timing and counter data rather than a public SASS-like ISA
- Neural lane
  - use Core ML for CPU/GPU/ANE placement and op-cost studies
  - use MLX and MPSGraph for programmable tensor experiments on Apple silicon

## Required macOS tools

- Full Xcode app, not only Command Line Tools
- Homebrew packages:
  - `llvm`
  - `lld`
  - `libomp`
  - `cmake`
  - `ninja`
  - `ccache`
  - `graphviz`
  - `openblas`
- Python packages:
  - `mlx`
  - `coremltools`
  - `torch`
  - `jax-metal`

## In-repo entry points

- `src/apple_re/README.md`
- `src/apple_re/scripts/run_apple_tranche1.sh`
- `src/apple_re/probes/apple_cpu_latency.c`
- `src/apple_re/scripts/audit_macos_re_env.sh`
- `src/apple_re/scripts/compile_metal_probe.sh`
- `src/apple_re/scripts/disassemble_apple_cpu_latency.sh`
- `src/apple_re/scripts/bootstrap_neural_lane.sh`
- `src/apple_re/scripts/neural_lane_probe.py`
- `src/apple_re/shaders/probe_simdgroup_reduce.metal`

## Recommended next buildout

1. Expand the CPU lane with load/store, shuffle, atomics, and transcendental
   probes.
2. Add a tiny Metal host harness so the shader probes can be timed and profiled
   end to end.
3. Add Core ML graph generators for dtype and compute-unit sweep experiments.
4. Normalize Apple result output so it can sit beside the current `sass_re`
   tables and scripts.

## Current machine status

- Metal lane: working
- PyTorch MPS lane: working
- MLX lane: working
- Core ML conversion lane: working
- JAX Metal lane: importable but still experimental; current sample compute on
  this machine fails with a StableHLO runtime mismatch, so keep it secondary
  until Apple and JAX versioning settles
- Torch is pinned to `2.7.0` in the repo-local neural lane because Core ML
  tooling reports that as its most recently tested Torch release
- JAX and jaxlib are pinned to `0.4.38` to match Apple and PyPI guidance for
  `jax-metal 0.1.1`, which expects matching `jax` / `jaxlib` versions at or
  above `0.4.34`
