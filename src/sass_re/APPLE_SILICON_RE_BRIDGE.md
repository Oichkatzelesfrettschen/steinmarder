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

- [`APPLE_TYPED_BOUNDARY_ATLAS.md`](APPLE_TYPED_BOUNDARY_ATLAS.md)
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

## Current Expansion Status

The bridge has now moved beyond the original starter scaffold:

1. The CPU lane already has promoted integer, atomic, cache, multiply, and
   fp16 reference rows.
2. The Metal lane already has host harness timing, GPU command-buffer timing,
   and promoted hardware counters.
3. The neural lane already has promoted placement evidence, but it still needs
   a true typed matrix runner instead of a thin blessed summary.
4. The new atlas tables in `src/sass_re/tables/` are now the canonical place to
   say whether a format is native, lowered, proxy-only, or unsupported.
5. Use [FRONTIER_ROADMAP_APPLE.md](FRONTIER_ROADMAP_APPLE.md) as the working
   checklist and [APPLE_TYPED_BOUNDARY_ATLAS.md](APPLE_TYPED_BOUNDARY_ATLAS.md)
   as the typed ledger while the runner side catches up.

## First Deep-Dive Runner

The first concrete 64-step tranche is now scripted in:

- [`../apple_re/scripts/run_apple_tranche1.sh`](../apple_re/scripts/run_apple_tranche1.sh)

It keeps the same manifest-first discipline as the NVIDIA path:

- per-step stdout/stderr artifacts
- status CSV
- capability snapshot
- run manifest and synthesis note
- explicit phase slicing (`A`..`H`) for reruns

The next 42-step follow-on is outlined in:

- [`NEXT42_APPLE_TRANCHE.md`](NEXT42_APPLE_TRANCHE.md)

For details, use [FRONTIER_ROADMAP_APPLE.md](FRONTIER_ROADMAP_APPLE.md).

## What To Expect

- CPU-level work is the most direct and should give the fastest feedback loop.
- Metal work should focus on emitted artifacts, timing, and counters rather
  than assuming a public SASS-equivalent ISA.
- Neural work should treat CPU, GPU, and ANE placement as the main variable,
  with the exact backend chosen by the framework and toolchain.
- The bridge is best treated as a sibling research program, not as a rewrite of
  the Ada work.

## Gap Analysis Cross-Link

The full gap analysis between the Apple and CUDA tracks is in:

- [`APPLE_TRACK_GAP_ANALYSIS.md`](APPLE_TRACK_GAP_ANALYSIS.md)

It covers:
- Tool-by-tool comparison (16 SASS tools vs Apple equivalents)
- Probe coverage table (CPU: 14 types, Metal GPU: 19 types)
- 16 identified gaps in 3 tiers (Critical, Important, Medium)
- The current top gap is no longer raw GPU counters; it is the missing typed
  matrix runner that keeps low-bit, TF, BF, and MX families explicit across CPU,
  Metal, and neural lanes

## Current Evidence Status (as of 2026-04-01)

### Confirmed measurements

| Measurement | Value | Source |
|-------------|-------|--------|
| M-series integer ADD latency | 1 cycle | `cpu_runs/llvm_mca_analysis.md` (r7) |
| M-series f64 FMADD latency | ~4 cycles | `cpu_runs/llvm_mca_analysis.md` (r7) |
| M-series relaxed atomic add | ~6 cycles | `cpu_runs/llvm_mca_analysis.md` (r7) |
| M-series sin+cos pair (libm) | ~60 cycles | `cpu_runs/llvm_mca_analysis.md` (r7) |
| L1/L2 → SLC boundary | ~128–256 KB | `cpu_runs/cache_hierarchy_analysis.md` (r7) |
| SLC → DRAM boundary | ~8–16 MB | `cpu_runs/cache_hierarchy_analysis.md` (r7) |
| Fastest Metal variant | occupancy_heavy (196.7 ns/element) | `counter_latency_report.md` (r7) |
| Density winner | occupancy_heavy (+678 r/s on gpu-state-intervals) | `counter_latency_report.md` (r7) |
| FP32 FMA → AIR intrinsic | `@air.fma.f32` confirmed | `metal_air_opcode_inventory.md` |
| simdgroup → AIR intrinsics | `@air.simd_sum.f32`, `@air.simd_shuffle.u.i32`, `@air.simd_broadcast.f32` | `metal_air_opcode_inventory.md` |
| Threadgroup barrier → AIR | `call @air.wg.barrier` confirmed | `metal_air_opcode_inventory.md` |

### AIR corpus (all probes compiled, 10 shaders)

27 unique opcodes/intrinsics. Top: `call @air.fma.f32` (64), `zext` (51),
`getelementptr` (51), `load` (43), `call @air.simd_shuffle.u.i32` (16).

Still absent: atomics, texture sampling, simdgroup matrix ops (`simdgroup_matrix_multiply_accumulate`), fp16 variants.

### Remaining frontier gap

The weak link is now typed normalization, not basic visibility:

- GPU counters and GPUStart/GPUEnd timing are promoted and real
- the CPU lane has direct measured type rows, but not yet the full requested
  4/8/16/32/64 matrix
- the Metal lane still needs compile-and-AIR classification for `f16`, `bf16`,
  `int8`, `uint8`, `fp8`, `tf32`, and `MX`
- the neural lane still needs a blessed typed matrix runner that preserves raw
  JSON/CSV artifacts instead of a thin markdown summary

## Best Starting Points

- [`RESULTS.md`](RESULTS.md) for the NVIDIA measurement style this bridge is
  borrowing
- [`../../docs/sass/APPLE_SILICON_RE_GUIDE.md`](../../docs/sass/APPLE_SILICON_RE_GUIDE.md)
  for the longer Apple-specific translation
- [`FRONTIER_ROADMAP_APPLE.md`](FRONTIER_ROADMAP_APPLE.md) for the concrete
  Apple checklist
- [`APPLE_TRACK_GAP_ANALYSIS.md`](APPLE_TRACK_GAP_ANALYSIS.md) for the gap analysis
- [`APPLE_TYPED_BOUNDARY_ATLAS.md`](APPLE_TYPED_BOUNDARY_ATLAS.md) for the typed
  Apple boundary ledger
- [`../apple_re/README.md`](../apple_re/README.md) for the current Apple subtree
  scaffold
