# Apple Silicon Research Bridge

This note translates the `src/sass_re/` research method to Apple silicon.

It is intentionally a bridge, not a claim ledger. The goal is to keep the same
operating style:

- probe corpus
- normalized results
- lane-specific runners
- semantic validation
- profiler sidecars
- differential comparisons when the hardware surface is partially opaque

## What Maps Cleanly

The parts that transfer directly from the NVIDIA SASS workflow are the process
shape and the artifact discipline:

- one probe should isolate one question
- every probe should have a stable build and run path
- every result should capture the exact toolchain, OS, and machine context
- every lane should have a disassembly or emitted-artifact check
- every profiler pass should sit next to the source or lowering result it
  explains

That makes Apple silicon a good fit for the same repository habits even though
it does not expose the same public machine-code surfaces as CUDA.

## Lane Translation

### CPU lane

The CPU lane is the most direct place to do instruction-level work.

Use it for:

- dependent-chain latency microbenchmarks
- throughput sweeps with multiple independent accumulators
- load/store and cache behavior
- integer, floating-point, shuffle, atomic, and transcendental kernels
- compiler comparison across flag and target combinations

Useful outputs:

- asm listings
- timing tables
- compiler flag diffs
- disassembly notes

### Graphics lane

The graphics lane should center on Metal compute probes and host-side timing.

Use it for:

- shader lowering studies
- simdgroup behavior
- threadgroup and tile-memory effects
- resource binding and argument-buffer experiments
- counter capture from Xcode profiling tools

Useful outputs:

- `.air` and `.metallib` artifacts
- shader source plus host harness
- counter snapshots
- timing and occupancy notes

### Neural lane

The neural lane should cover the Apple-side placement and lowering stack.

Use it for:

- Core ML conversion and execution placement
- MPSGraph tensor experiments
- MLX and PyTorch MPS comparison points
- CPU, GPU, and ANE fallback behavior
- dtype and quantization sweeps

Useful outputs:

- model conversion logs
- runtime placement summaries
- dtype and precision matrices
- per-backend timing comparisons

## Current In-Tree Scaffold

The Apple subtree already provides the first pass at that translation:

- [`../apple_re/README.md`](../apple_re/README.md)
- [`../apple_re/CMakeLists.txt`](../apple_re/CMakeLists.txt)
- [`../apple_re/probes/apple_cpu_latency.c`](../apple_re/probes/apple_cpu_latency.c)
- [`../apple_re/shaders/probe_simdgroup_reduce.metal`](../apple_re/shaders/probe_simdgroup_reduce.metal)
- [`../apple_re/scripts/audit_macos_re_env.sh`](../apple_re/scripts/audit_macos_re_env.sh)
- [`../apple_re/scripts/compile_metal_probe.sh`](../apple_re/scripts/compile_metal_probe.sh)
- [`../apple_re/scripts/disassemble_apple_cpu_latency.sh`](../apple_re/scripts/disassemble_apple_cpu_latency.sh)
- [`../apple_re/scripts/bootstrap_neural_lane.sh`](../apple_re/scripts/bootstrap_neural_lane.sh)
- [`../apple_re/scripts/neural_lane_probe.py`](../apple_re/scripts/neural_lane_probe.py)
- [`../apple_re/requirements-neural.txt`](../apple_re/requirements-neural.txt)

That scaffold is enough to start collecting repeatable evidence without trying
to force a CUDA-shaped workflow onto Apple tooling.

## Recommended First Expansion

The next useful additions are straightforward:

1. Expand the CPU lane with load/store, shuffle, atomics, and transcendental
   probes.
2. Add a tiny Metal host harness so the shader probes can be timed and profiled
   end to end.
3. Add Core ML graph generators for dtype and compute-unit sweep experiments.
4. Normalize Apple result output so it can sit beside the existing
   `src/sass_re/` tables and scripts.
5. Use [FRONTIER_ROADMAP_APPLE.md](FRONTIER_ROADMAP_APPLE.md) as the working
   checklist while the Apple lane grows.

## First Deep-Dive Runner

The first concrete 42-step tranche is now scripted in:

- [`../apple_re/scripts/run_apple_tranche1.sh`](../apple_re/scripts/run_apple_tranche1.sh)

It keeps the same manifest-first discipline as the NVIDIA path:

- per-step stdout/stderr artifacts
- status CSV
- capability snapshot
- run manifest and synthesis note
- explicit phase slicing (`A`..`G`) for reruns

For details, use [FRONTIER_ROADMAP_APPLE.md](FRONTIER_ROADMAP_APPLE.md).

## What To Expect

- CPU-level work is the most direct and should give the fastest feedback loop.
- Metal work should focus on emitted artifacts, timing, and counters rather
  than assuming a public SASS-equivalent ISA.
- Neural work should treat CPU, GPU, and ANE placement as the main variable,
  with the exact backend chosen by the framework and toolchain.
- The bridge is best treated as a sibling research program, not as a rewrite of
  the Ada work.

## Best Starting Points

- [`RESULTS.md`](RESULTS.md) for the NVIDIA measurement style this bridge is
  borrowing
- [`../../docs/sass/APPLE_SILICON_RE_GUIDE.md`](../../docs/sass/APPLE_SILICON_RE_GUIDE.md)
  for the longer Apple-specific translation
- [`FRONTIER_ROADMAP_APPLE.md`](FRONTIER_ROADMAP_APPLE.md) for the concrete
  Apple checklist
- [`../apple_re/README.md`](../apple_re/README.md) for the current Apple subtree
  scaffold
