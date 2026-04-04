# xctrace Row Delta Summary

- baseline_trace: `gpu_baseline.trace`
- compared_traces: `4`

Top absolute deltas on metal schemas:
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=6815, variant=2363, delta=-4452 (-65.33%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=6815, variant=3140, delta=-3675 (-53.93%)
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=6815, variant=3334, delta=-3481 (-51.08%)
- `gpu_threadgroup_minimal.trace` `metal-gpu-state-intervals`: baseline=6815, variant=3359, delta=-3456 (-50.71%)
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=3839, variant=1252, delta=-2587 (-67.39%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=3839, variant=1745, delta=-2094 (-54.55%)
- `gpu_threadgroup_minimal.trace` `metal-gpu-intervals`: baseline=3839, variant=1817, delta=-2022 (-52.67%)
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=3839, variant=1865, delta=-1974 (-51.42%)
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=3779, variant=1958, delta=-1821 (-48.19%)
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=3779, variant=2168, delta=-1611 (-42.63%)
- `gpu_threadgroup_minimal.trace` `metal-driver-intervals`: baseline=3779, variant=2276, delta=-1503 (-39.77%)
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=3779, variant=2322, delta=-1457 (-38.56%)
- `gpu_threadgroup_heavy.trace` `metal-command-buffer-completed`: baseline=1340, variant=892, delta=-448 (-33.43%)
- `gpu_register_pressure.trace` `metal-command-buffer-completed`: baseline=1340, variant=928, delta=-412 (-30.75%)
- `gpu_threadgroup_minimal.trace` `metal-command-buffer-completed`: baseline=1340, variant=955, delta=-385 (-28.73%)
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=1340, variant=977, delta=-363 (-27.09%)
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=1000, variant=799, delta=-201 (-20.10%)
- `gpu_threadgroup_minimal.trace` `metal-application-encoders-list`: baseline=1000, variant=799, delta=-201 (-20.10%)
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=1000, variant=800, delta=-200 (-20.00%)
- `gpu_register_pressure.trace` `metal-application-encoders-list`: baseline=1000, variant=800, delta=-200 (-20.00%)

Top density deltas (rows/sec) on metal schemas:
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=3208.7 r/s, variant=2442.5 r/s, delta=-766.2 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=3208.7 r/s, variant=2447.3 r/s, delta=-761.4 r/s
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=1779.3 r/s, variant=2356.5 r/s, delta=577.2 r/s
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=1807.5 r/s, variant=1294.1 r/s, delta=-513.4 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=1807.5 r/s, variant=1369.0 r/s, delta=-438.5 r/s
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=470.8 r/s, variant=868.5 r/s, delta=397.6 r/s
- `gpu_register_pressure.trace` `metal-application-encoders-list`: baseline=470.8 r/s, variant=826.9 r/s, delta=356.1 r/s
- `gpu_threadgroup_heavy.trace` `metal-command-buffer-completed`: baseline=630.9 r/s, variant=969.6 r/s, delta=338.7 r/s
- `gpu_register_pressure.trace` `metal-command-buffer-completed`: baseline=630.9 r/s, variant=959.2 r/s, delta=328.3 r/s
- `gpu_threadgroup_minimal.trace` `metal-driver-intervals`: baseline=1779.3 r/s, variant=2100.9 r/s, delta=321.6 r/s
