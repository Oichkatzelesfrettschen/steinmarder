# Per-Format AGX Capture

- source lane: typed Metal type-surface build
- traces:
  - `gpu_<format>.trace` via `Metal GPU Counters`
  - `gpu_fmt_<format>.trace` via `Metal System Trace`
- exported counters:
  - `gpu_counter_<format>_metal-gpu-counter-intervals.xml`
  - `gpu_counter_<format>_metal-shader-profiler-intervals.xml`
- baseline for counter deltas: `float32`
