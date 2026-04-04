# Apple Deep-Dive Stack

This note is the working stack map for the Apple reverse-engineering lane after
the typed atlas, Metal frontier proxies, and AGX counter/tracing work.

It answers the practical question:

> what is the full probe stack from CPU through Metal/GPU, neural, memory,
> I/O, disk, OpenGL, and Vulkan-on-Metal, and what should we deepen next?

## Canonical references

- [`APPLE_TYPED_BOUNDARY_ATLAS.md`](APPLE_TYPED_BOUNDARY_ATLAS.md)
- [`APPLE_FULL_SCOPE_GAP_MAP.md`](APPLE_FULL_SCOPE_GAP_MAP.md)
- [`APPLE_STEINMARDER_PROBE_EXAMPLES.md`](APPLE_STEINMARDER_PROBE_EXAMPLES.md)
- [`tables/table_apple_full_scope_gap_matrix.csv`](tables/table_apple_full_scope_gap_matrix.csv)
- [`tables/table_apple_steinmarder_probe_examples.csv`](tables/table_apple_steinmarder_probe_examples.csv)
- [`tables/table_apple_type_boundary_matrix.csv`](tables/table_apple_type_boundary_matrix.csv)
- [`tables/table_apple_type_timings.csv`](tables/table_apple_type_timings.csv)

## Current stack

### CPU lane

- direct probes
  - [`../apple_re/probes/apple_cpu_latency.c`](../apple_re/probes/apple_cpu_latency.c)
  - [`../apple_re/probes/apple_cpu_cache_pressure.c`](../apple_re/probes/apple_cpu_cache_pressure.c)
- tranche runners
  - [`../apple_re/scripts/run_next42_cpu_probes.sh`](../apple_re/scripts/run_next42_cpu_probes.sh)
  - [`../apple_re/scripts/run_next42_cpu_cache_probes.sh`](../apple_re/scripts/run_next42_cpu_cache_probes.sh)
- trace/disassembly sidecars
  - `clang -S`, `otool`, `llvm-objdump`, `llvm-mca`
  - `xctrace Time Profiler`
  - `sample`, `spindump`, `vmmap`, `leaks`
- wide-precision / extended-FP fastpath sub-lane (2026-04-02)
  - probes: `nibble_unpack_u4`, `nibble_unpack_i4`, `fadd_dep_ld_aarch64`,
    `dd_twosum_dep`, `dd_twosum_fma_dep`, `f64x2_vadd_dep`, `f32x4_vfma_dep`,
    `f64x2_dd_twosum_dep`, `f64x2_dd_throughput`, `veltkamp_split_dep`
    (all in `apple_cpu_latency.c`)
  - key finding: NEON-vectorised double-double at 0.520 ns/106-bit value —
    1.91× more throughput than scalar float64; the M-series OOO collapses
    the 4-op dep chain to ~3.3 cycles via float64x2 pair parallelism
  - `f64x2_dd_throughput`: 4 independent float64x2_t accumulators (32 steps/iter);
    breaks dep chain to reveal peak DD issue bandwidth
  - `veltkamp_split_dep`: Veltkamp 2^27+1 split dep chain; 5 FP ops/step
    (fmul+fsub+fsub+fsub+fadd), critical path ~4 cycles; first half of DD multiply
  - platform invariants confirmed: long double == binary64; __float128 absent;
    fp80 x86-exclusive; all "emulated" extended precision routes through NEON FPU
  - timings in `tables/table_apple_type_timings.csv` backend `aarch64_neon_dd`
  - full writeup: [`APPLE_WIDE_PRECISION_FINDINGS.md`](APPLE_WIDE_PRECISION_FINDINGS.md)

### Metal lane

- direct shaders
  - [`../apple_re/shaders/probe_simdgroup_reduce.metal`](../apple_re/shaders/probe_simdgroup_reduce.metal)
  - [`../apple_re/shaders/probe_threadgroup_heavy.metal`](../apple_re/shaders/probe_threadgroup_heavy.metal)
  - [`../apple_re/shaders/probe_threadgroup_minimal.metal`](../apple_re/shaders/probe_threadgroup_minimal.metal)
  - [`../apple_re/shaders/probe_occupancy_heavy.metal`](../apple_re/shaders/probe_occupancy_heavy.metal)
  - [`../apple_re/shaders/probe_register_pressure.metal`](../apple_re/shaders/probe_register_pressure.metal)
  - [`../apple_re/shaders/probe_dd_lat.metal`](../apple_re/shaders/probe_dd_lat.metal) — double-double dep chain (float2 hi/lo, 8 steps/iter)
  - [`../apple_re/shaders/probe_dd_tput.metal`](../apple_re/shaders/probe_dd_tput.metal) — double-double throughput (4 independent float2 pairs, 32 steps/iter)
- typed surface frontier
  - [`../apple_re/scripts/classify_metal_type_surface.py`](../apple_re/scripts/classify_metal_type_surface.py)
- host harness
  - [`../apple_re/host/metal_probe_host.m`](../apple_re/host/metal_probe_host.m)
- tranche runners
  - [`../apple_re/scripts/run_next42_metal_probes.sh`](../apple_re/scripts/run_next42_metal_probes.sh)
  - [`../apple_re/scripts/capture_gpu_counters.sh`](../apple_re/scripts/capture_gpu_counters.sh)
- trace/counter sidecars
  - `xctrace Metal System Trace`
  - `xctrace Metal GPU Counters`
  - `extract_xctrace_metrics.py`
  - `extract_gpu_counters.py`
  - `analyze_gpu_counter_deltas.py`

### Neural lane

- direct probes
  - [`../apple_re/scripts/neural_lane_probe.py`](../apple_re/scripts/neural_lane_probe.py)
  - [`../apple_re/scripts/neural_typed_matrix.py`](../apple_re/scripts/neural_typed_matrix.py)
- tranche runners
  - [`../apple_re/scripts/run_next42_neural_suite.sh`](../apple_re/scripts/run_next42_neural_suite.sh)
  - [`../apple_re/scripts/run_next42_neural_raw_probes.sh`](../apple_re/scripts/run_next42_neural_raw_probes.sh)

### OpenGL lane

- draft raw shader probes
  - [`../apple_re/shaders_gl/probe_fullscreen.vert`](../apple_re/shaders_gl/probe_fullscreen.vert)
  - [`../apple_re/shaders_gl/probe_register_pressure.frag`](../apple_re/shaders_gl/probe_register_pressure.frag)
  - [`../apple_re/shaders_gl/probe_texture_stride.frag`](../apple_re/shaders_gl/probe_texture_stride.frag)
  - [`../apple_re/shaders_gl/probe_int_quantize.frag`](../apple_re/shaders_gl/probe_int_quantize.frag)
- draft runner
  - [`../apple_re/scripts/run_next42_opengl_probe_suite.sh`](../apple_re/scripts/run_next42_opengl_probe_suite.sh)

### Vulkan-on-Metal lane

- preferred translation layer
  - MoltenVK
- draft raw shaders
  - [`../apple_re/shaders_vk/probe_fp32_pressure.comp`](../apple_re/shaders_vk/probe_fp32_pressure.comp)
  - [`../apple_re/shaders_vk/probe_fp16_convert.comp`](../apple_re/shaders_vk/probe_fp16_convert.comp)
  - [`../apple_re/shaders_vk/probe_int8_pack.comp`](../apple_re/shaders_vk/probe_int8_pack.comp)
- draft runner
  - [`../apple_re/scripts/run_next42_moltenvk_probe_suite.sh`](../apple_re/scripts/run_next42_moltenvk_probe_suite.sh)

### Full-stack orchestration

- wrapper
  - [`../apple_re/scripts/run_next42_full_stack_suite.sh`](../apple_re/scripts/run_next42_full_stack_suite.sh)
- sidecar telemetry
  - `vm_stat`
  - `iostat`
  - `diskutil`
  - `df`
  - `mount`
  - `powermetrics`

## Migration posture from CUDA probes

The CUDA/SASS corpus already gives the probe families we should keep porting:

- conversion sweep
  - port to CPU and Metal typed conversion probes
- int tiling
  - port to CPU integer latency/throughput chains plus Metal packed-integer kernels
- mx / nf tiling
  - port to Metal frontier proxies, neural proxy models, and MoltenVK compute probes
- cache control / data movement
  - port to CPU cache-pressure, Metal threadgroup/global-memory probes, and OpenGL texture-stride probes
- edge atomics / mixed-precision tiles
  - port to Metal atomics, GPU counters, and neural mixed-dtype placement/timing work

## Steinmarder-inspired examples

The typed Apple lane now also has a concrete example ledger so the frontier
formats stay tied to actual repo workloads:

- packed 4-bit examples
  - `lbm_int4_nibble_pair_soa`
  - `lbm_fp4_nibble_pair_soa`
  - `lbm_uint4_nibble_pair_soa`
- widened integer examples
  - `lbm_int8_i_major_soa`
  - `lbm_uint8_pack4_stream`
  - `lbm_int16_i_major_soa`
  - `lbm_uint16_pair_stream`
  - `lbm_int32_accumulator`
  - `lbm_uint32_carrier`
  - `lbm_int64_long_accumulator`
  - `lbm_uint64_long_carrier`
- neural frontier examples
  - `neural_tf32_proxy_tile`
  - `neural_fp8_proxy_tile`
  - `neural_mx_proxy_tile`

See:

- [`APPLE_STEINMARDER_PROBE_EXAMPLES.md`](APPLE_STEINMARDER_PROBE_EXAMPLES.md)
- [`tables/table_apple_steinmarder_probe_examples.csv`](tables/table_apple_steinmarder_probe_examples.csv)

## Next tranche priorities

1. Add per-format AGX trace/counter capture for `float16`, `bfloat16`, `int8`, `uint8`, `tf32`, `fp8`, and `MX`.
2. Add CPU typed probes for `int8`, `int16`, `int64`, `uint8`, `uint16`, `uint32`, `bfloat16`, and `float32`.
3. Expand Metal direct surfaces for `int16`, `uint16`, `int64`, `uint64`, and `float64`.
4. Turn the new OpenGL lane from shader-only validation into an actual host-run lane if we decide the API ceiling is still worth it.
5. Stand up a real MoltenVK host lane if `vulkaninfo` plus a working MoltenVK ICD are present on this machine.
