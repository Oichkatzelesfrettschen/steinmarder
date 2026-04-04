# zen_re -- Ryzen / Zen CPU Reverse Engineering Stack

This directory is a CPU-side parallel to `src/sass_re/`. It does not try to
force GPU idioms onto x86. Instead it builds a Ryzen-focused measurement lane
for instruction-level and memory-system behavior, then uses those measurements
to steer the existing CPU hot loops in steinmarder.

## First probe set

The current binaries are tiny, single-purpose probes:

- `zen_probe_fma` -- dependent vs multi-accumulator FMA chains
- `zen_probe_load_use` -- pointer-chase load-to-use latency
- `zen_probe_permute` -- AVX2 lane-permute cost
- `zen_probe_gather` -- AVX2 indexed gather cost on a large working set
- `zen_probe_branch` -- predictable vs unpredictable branch cost
- `zen_probe_prefetch` -- streaming scan with and without software prefetch

They are intentionally small and bias toward repeatability over heroic tuning.
Each probe prints the machine features it sees, accepts `--iterations`,
`--size`, `--cpu`, and can emit a CSV summary with `--csv`.

## Profiling wrappers

Scripts under `src/zen_re/scripts/` mirror the current CUDA-side separation:

- `profile_perf.sh` is the CPU-side `ncu` analogue:
  it captures `perf stat` counters and a `perf record` sample for one probe.
- `profile_perf_timeline.sh` is the CPU-side `nsys` analogue:
  it captures scheduler and call-stack samples around one probe or target.
- `profile_uprof.sh` is an AMD uProf wrapper:
  it stays thin on purpose and lets `UPROF_ARGS` carry the current AMD CLI
  collection arguments rather than freezing a possibly stale syntax in repo code.

## Translation targets

The first targets are existing CPU hot loops that already reflect the same
themes as the CUDA work:

- [`src/nerf/nerf_simd.c`](../nerf/nerf_simd.c)
  hash-grid lookups, prefetching, AVX2/FMA dispatch, and MLP math
- [`src/nerf/nerf_batch.c`](../nerf/nerf_batch.c)
  batch scheduling and cache reuse
- [`src/render/render.c`](../render/render.c)
  traversal / shading hot paths and branch structure
- [`src/render/sm_mt.c`](../render/sm_mt.c)
  thread scheduling and cache locality at the tile level
- [`src/render/bvh.c`](../render/bvh.c)
  pointer-heavy traversal and branch predictability

The CUDA LBM docs are still useful as idea sources, but the CPU translation
path should focus on SoA layout, branch shape, software pipelining, cache
blocking, and precomputation. It should *not* assume that Ada-specific wins,
such as fixed-point beating FP32 because of GPU pipeline asymmetry, automatically
carry over to Ryzen.
