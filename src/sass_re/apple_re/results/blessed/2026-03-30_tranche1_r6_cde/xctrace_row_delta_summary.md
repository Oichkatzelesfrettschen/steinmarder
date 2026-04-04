# xctrace Row Delta Summary

- baseline_trace: `gpu_baseline.trace`
- compared_traces: `3`

Top absolute deltas on metal schemas:
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=2491, variant=2934, delta=443 (17.78%)
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=2491, variant=2833, delta=342 (13.73%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=1289, variant=1587, delta=298 (23.12%)
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=1289, variant=1541, delta=252 (19.55%)
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=2206, variant=1973, delta=-233 (-10.56%)
- `gpu_register_pressure.trace` `metal-application-encoders-list`: baseline=999, variant=785, delta=-214 (-21.42%)
- `gpu_register_pressure.trace` `metal-command-buffer-completed`: baseline=1075, variant=870, delta=-205 (-19.07%)
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=999, variant=798, delta=-201 (-20.12%)
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=999, variant=798, delta=-201 (-20.12%)
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=1075, variant=896, delta=-179 (-16.65%)
- `gpu_threadgroup_heavy.trace` `metal-command-buffer-completed`: baseline=1075, variant=906, delta=-169 (-15.72%)
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=2206, variant=2074, delta=-132 (-5.98%)
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=2491, variant=2609, delta=118 (4.74%)
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=1289, variant=1399, delta=110 (8.53%)
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=2206, variant=2108, delta=-98 (-4.44%)

Top density deltas (rows/sec) on metal schemas:
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=2280.6 r/s, variant=3003.4 r/s, delta=722.9 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=2280.6 r/s, variant=2859.8 r/s, delta=579.2 r/s
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=2280.6 r/s, variant=2807.6 r/s, delta=527.0 r/s
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=1180.1 r/s, variant=1624.6 r/s, delta=444.5 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=1180.1 r/s, variant=1555.6 r/s, delta=375.5 r/s
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=1180.1 r/s, variant=1505.5 r/s, delta=325.4 r/s
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=2019.6 r/s, variant=2157.9 r/s, delta=138.3 r/s
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=914.6 r/s, variant=805.5 r/s, delta=-109.1 r/s
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=2019.6 r/s, variant=2123.2 r/s, delta=103.5 r/s
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=914.6 r/s, variant=816.9 r/s, delta=-97.7 r/s
