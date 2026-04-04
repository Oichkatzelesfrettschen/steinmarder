# xctrace Row Delta Summary

- baseline_trace: `gpu_baseline.trace`
- compared_traces: `2`

Top absolute deltas on metal schemas:
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=2947, variant=2327, delta=-620 (-21.04%)
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=2369, variant=1888, delta=-481 (-20.30%)
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=2369, variant=2026, delta=-343 (-14.48%)
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=1545, variant=1219, delta=-326 (-21.10%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=2947, variant=2695, delta=-252 (-8.55%)
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=1087, variant=870, delta=-217 (-19.96%)
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=999, variant=797, delta=-202 (-20.22%)
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=999, variant=799, delta=-200 (-20.02%)
- `gpu_threadgroup_heavy.trace` `metal-command-buffer-completed`: baseline=1087, variant=912, delta=-175 (-16.10%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=1545, variant=1409, delta=-136 (-8.80%)
- `gpu_occupancy_heavy.trace` `metal-driver-event-per-thread-intervals`: baseline=0, variant=0, delta=0 (n/a)
- `gpu_occupancy_heavy.trace` `metal-gpu-counter-intervals`: baseline=0, variant=0, delta=0 (n/a)
- `gpu_occupancy_heavy.trace` `metal-shader-profiler-intervals`: baseline=0, variant=0, delta=0 (n/a)
- `gpu_threadgroup_heavy.trace` `metal-driver-event-per-thread-intervals`: baseline=0, variant=0, delta=0 (n/a)
- `gpu_threadgroup_heavy.trace` `metal-gpu-counter-intervals`: baseline=0, variant=0, delta=0 (n/a)
- `gpu_threadgroup_heavy.trace` `metal-shader-profiler-intervals`: baseline=0, variant=0, delta=0 (n/a)
