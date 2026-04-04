# xctrace Row Delta Summary

- baseline_trace: `gpu_baseline.trace`
- compared_traces: `4`

Top absolute deltas on metal schemas:
- `gpu_threadgroup_minimal.trace` `metal-gpu-state-intervals`: baseline=5048, variant=3701, delta=-1347 (-26.68%)
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=5048, variant=4125, delta=-923 (-18.28%)
- `gpu_threadgroup_minimal.trace` `metal-driver-intervals`: baseline=3208, variant=2412, delta=-796 (-24.81%)
- `gpu_threadgroup_minimal.trace` `metal-gpu-intervals`: baseline=2819, variant=2077, delta=-742 (-26.32%)
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=3208, variant=2682, delta=-526 (-16.40%)
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=3208, variant=2692, delta=-516 (-16.08%)
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=2819, variant=2349, delta=-470 (-16.67%)
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=5048, variant=4597, delta=-451 (-8.93%)
- `gpu_threadgroup_minimal.trace` `metal-command-buffer-completed`: baseline=1286, variant=969, delta=-317 (-24.65%)
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=1286, variant=978, delta=-308 (-23.95%)
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=3208, variant=2939, delta=-269 (-8.39%)
- `gpu_threadgroup_minimal.trace` `metal-application-encoders-list`: baseline=1000, variant=792, delta=-208 (-20.80%)
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=2819, variant=2612, delta=-207 (-7.34%)
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=1000, variant=799, delta=-201 (-20.10%)
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=1000, variant=799, delta=-201 (-20.10%)
- `gpu_register_pressure.trace` `metal-application-encoders-list`: baseline=1000, variant=800, delta=-200 (-20.00%)
- `gpu_register_pressure.trace` `metal-command-buffer-completed`: baseline=1286, variant=1096, delta=-190 (-14.77%)
- `gpu_threadgroup_heavy.trace` `metal-command-buffer-completed`: baseline=1286, variant=1138, delta=-148 (-11.51%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=5048, variant=4907, delta=-141 (-2.79%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=2819, variant=2776, delta=-43 (-1.53%)

Top density deltas (rows/sec) on metal schemas:
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=2700.6 r/s, variant=3379.1 r/s, delta=678.5 r/s
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=2700.6 r/s, variant=2090.7 r/s, delta=-609.9 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=1508.1 r/s, variant=1920.0 r/s, delta=411.9 r/s
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=1716.2 r/s, variant=1359.3 r/s, delta=-356.9 r/s
- `gpu_threadgroup_minimal.trace` `metal-gpu-state-intervals`: baseline=2700.6 r/s, variant=3037.3 r/s, delta=336.8 r/s
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=1508.1 r/s, variant=1190.5 r/s, delta=-317.6 r/s
- `gpu_threadgroup_minimal.trace` `metal-driver-intervals`: baseline=1716.2 r/s, variant=1979.5 r/s, delta=263.3 r/s
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=1716.2 r/s, variant=1978.8 r/s, delta=262.6 r/s
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=2700.6 r/s, variant=2445.2 r/s, delta=-255.4 r/s
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=1716.2 r/s, variant=1464.5 r/s, delta=-251.7 r/s
