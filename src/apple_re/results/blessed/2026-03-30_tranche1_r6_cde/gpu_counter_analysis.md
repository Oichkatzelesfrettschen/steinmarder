# Metal GPU Counter Analysis

- run_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/blessed/2026-03-30_tranche1_r6_cde`
- total_counter_rows: 0
- files_with_data: 0
- schema-only_files (0 rows): 7

## Schema-only captures (0 data rows)

These files contain only the table schema, no counter values.
This means the xctrace recording was made with a template that does
**not** enable hardware GPU counter sampling.

**Fix**: Record with `xcrun xctrace record --template 'Metal GPU Counters'`
instead of `--template 'Metal Application'`.

Empty files:
- `xctrace_gpu_baseline_metal-gpu-counter-intervals.xml`
- `xctrace_gpu_occupancy_heavy_metal-gpu-counter-intervals.xml`
- `xctrace_gpu_register_pressure_metal-gpu-counter-intervals.xml`
- `xctrace_gpu_threadgroup_heavy_metal-gpu-counter-intervals.xml`
- `xctrace_gpu_baseline_metal-shader-profiler-intervals.xml`
- `xctrace_gpu_occupancy_heavy_metal-shader-profiler-intervals.xml`
- `xctrace_gpu_register_pressure_metal-shader-profiler-intervals.xml`

## No hardware counter data captured

All GPU counter XML exports in this run directory are schema-only.

### How to capture hardware GPU counters

1. Use the `Metal GPU Counters` or `GPU` template:
   ```sh
   xcrun xctrace record \
     --template 'Metal GPU Counters' \
     --output gpu_counters.trace \
     -- ./sm_apple_metal_probe_host
   ```

2. Export the counter table:
   ```sh
   xcrun xctrace export \
     --input gpu_counters.trace \
     --xpath '//trace-toc[1]/run[1]/data[1]/table[@schema="metal-gpu-counter-intervals"]' \
     --output gpu_counter_data.xml
   ```

3. Run this script on the directory containing gpu_counter_data.xml.

### Available templates with GPU counter support
- `Metal GPU Counters` — dedicated GPU counter template
- `GPU` — GPU + Metal + counter bundle

### Counter families exposed by Metal GPU Counters on Apple silicon
- Vertex cycles, fragment cycles, tiling cycles
- ALU utilization (vertex, fragment, compute)
- Texture unit utilization
- Memory read/write bandwidth
- L1 tile cache hit rate, L2 cache hit rate
- SIMD utilization
- Fragment overflow (spill to memory)
