# Memory — Methods

## Shared Optimization Law

The steinmarder reverse-engineering tracks already have a strong method
template. The x130e lane should inherit it rather than reinvent it.

Core law:

- do not accept an optimization claim without runtime evidence, code-shape
  evidence, profiler evidence, and correctness evidence

## Methods to Transfer into TeraScale

1. Compiler-vs-manual decision law
   - keep hand-tuned paths only when they survive measurement, maintenance, and
     portability scrutiny
2. Manifest-backed tranche design
   - every profiling or validation run should emit enough structured metadata to
     reproduce the result later
3. Layout-first optimization
   - improve data layout, submission shape, or state churn before getting lost
     in instruction cleverness
4. Expensive-recurring-math elimination
   - the `inv_tau` lesson from CUDA LBM generalizes to TeraScale compiler and
     shader work: precompute or hoist when the hardware penalty is predictable
5. Hardware-native packing
   - use the ISA and scheduler the machine actually offers; do not optimize as
     though the machine were scalar or modern scalar-SIMD when it is VLIW5
6. CPU-overhead minimization
   - if the GPU is underfed, host submission, cleanup, synchronization, and
     state emission are fair game and often first-order effects

## Method Sources Already in This Repo

- [`CROSS_TRACK_CONTROL_PLANE.md`](CROSS_TRACK_CONTROL_PLANE.md)
- [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md)
- [`RESULTS.md`](RESULTS.md)
- [`MONOGRAPH_SM89_SYNTHESIS.md`](MONOGRAPH_SM89_SYNTHESIS.md)
- [`NEXT120_AUTO_EXPLORER.md`](NEXT120_AUTO_EXPLORER.md)
- [`PAPER_CLAIMS_MATRIX.md`](PAPER_CLAIMS_MATRIX.md)

## x130e-Specific Method Consequences

- performance work must track both CPU-side Mesa overhead and GPU-side VLIW
  packing losses
- scheduler or NIR changes should be kept honest with microbench + benchmark +
  validation loops
- hard architecture ceilings should be logged explicitly rather than treated as
  “to be optimized away”
