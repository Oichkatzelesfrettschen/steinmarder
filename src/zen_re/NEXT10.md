# Zen RE Next 10

This is the current execution sequence for turning the new probe lane into
useful Ryzen optimization work.

1. Build and smoke-test the micro-probe binaries on the local Ryzen host.
2. Batch-profile the probes with `perf stat` and `perf record`.
3. Summarize those perf counters into a single sortable CSV.
4. Establish baseline runs for the first CPU translation targets:
   `nerf_simd_test` and `test_render_smoke`.
5. Split the target code into hypothesis buckets:
   hash-grid lookup, MLP math, traversal branches, and scheduling/locality.
6. Compare the local numbers against the Agner Fog manuals and AMD's official
   Ryzen-facing documentation cache.
7. Turn the strongest deltas into concrete code hypotheses, not generic advice.
8. Apply one low-risk change per target area, starting with `src/nerf/nerf_simd.c`.
9. Re-run the probe and baseline scripts to see if the intended bottleneck moved.
10. Keep only the changes that improve both the local measurements and the
    actual steinmarder hot paths.

The scripts added in this directory implement steps 1-6 today.
