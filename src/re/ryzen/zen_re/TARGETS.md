# Zen RE Targets

First translation targets for the Ryzen lane.

## 1. `src/nerf/nerf_simd.c`

Why first:
- already contains AVX2/FMA dispatch and software prefetch hooks
- hash-grid lookup and MLP inference are structurally similar to the CUDA work
- likely to respond directly to load-use, gather, branch, and FMA findings

Initial hypotheses:
- prefetch distance and placement can likely be tuned from measured pointer-chase
  and gather behavior
- the MLP loops can likely benefit from a clearer multi-accumulator strategy
- gather-heavy hash lookups may prefer layout reshaping before more SIMD tricks

Current findings on the local Ryzen 5 5600X3D:
- isolated hash-grid lookup prefers `SM_NERF_HASHGRID_PREFETCH=0` over the
  current immediate same-iteration prefetch in the warm-cache regime
- a one-ray-ahead prefetch prototype is worse than both `off` and `immediate`
  because the extra next-ray hash work costs too much
- end-to-end reduced-size NeRF runs keep a small win for `off`, but the phase
  share shows MLP inference is still the dominant cost, not hash-grid lookup
- focused MLP benchmarking shows the 8x single-ray path is about 17-20% slower
  than the batch kernel for the same 8-ray work unit on ramp/random-like inputs
- activation-only benchmarking shows the nonlinear tail is still about 25-33%
  of single-path MLP time on nontrivial inputs, so the next optimization pass
  should separate matrix-body work from activation-tail work
- an explicit matrix-body-only benchmark confirms the matrix body dominates the
  single-ray path on ramp inputs; activation is meaningful but secondary
- a first same-ray micro-batch bridge using the batch kernel did not survive
  phase-timed end-to-end NeRF testing and was reverted
- a fixed-shape `27->64->64->4` single-ray benchmark variant helps, but the
  stronger win comes from weight/layout prepacking: aligned `W0`/`W1`, aligned
  biases, and a transposed/aligned output weight matrix for the single-ray path
- the prepacked single-ray variant now survives end-to-end reduced-size NeRF
  testing behind `SM_NERF_MLP_VARIANT=prepacked`, cutting local measured
  `mlp_ns_per_step` by about 25-41% depending on run ordering/noise
- an interleaved variant sweep in `build/zen_re_variant_sweep/` shows
  `prepacked` is a strong single-thread default candidate and usually a win in
  phase time at 6 threads too, but one `96x96x64 @ 6 threads` case regressed in
  total phase time because hash-grid pressure rose enough to cancel some MLP win
- the first coherent-ray bridge prototype at the existing 8-lane boundary did
  not survive measurement and was reverted; its batch-at-step-index structure
  increased both hash-grid and MLP cost on this machine
- next work should prioritize MLP structure and per-step control flow after
  keeping hash-grid prefetch configurable for measurement; the next bridge
  attempt should only return if it also reduces memory pressure, not just calls
  the batch kernel more often

## 2. `src/nerf/nerf_batch.c`

Why next:
- acts as the bridge from fast kernels to real scene scheduling
- cache reuse and batch-size decisions show up here before they show up in math

Initial hypotheses:
- batch sizing should be aligned with actual cache residency, not just SIMD width
- queue ordering may be able to improve locality for hash-grid traffic

## 3. `src/render/bvh.c`

Why:
- branch-heavy and pointer-heavy traversal logic
- likely to respond to branch predictability and load-use measurements

Initial hypotheses:
- node layout and near-first child ordering should be benchmarked together
- some branch structure may want to become conditional-move/select style

## 4. `src/render/render.c`

Why:
- large hot path where shading, traversal, and material logic meet
- the place where branch shape and arithmetic mix most visibly

Initial hypotheses:
- separate coherent and incoherent paths where it reduces front-end waste
- keep expensive invariant work out of inner loops

## 5. `src/render/sm_mt.c`

Why:
- scheduler and tile-level locality shape the whole renderer
- timeline-style CPU profiling matters more here than instruction-level probes

Initial hypotheses:
- worker wakeups and tile ordering should be profiled with `perf sched`
- there may be avoidable cross-core traffic when tiles bounce between threads
