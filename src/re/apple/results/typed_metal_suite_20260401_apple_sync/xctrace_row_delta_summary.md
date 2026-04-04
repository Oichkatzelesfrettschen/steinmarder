# xctrace Row Delta Summary

- baseline_trace: `gpu_baseline.trace`
- compared_traces: `4`

Top absolute deltas on metal schemas:
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=1793, variant=2895, delta=1102 (61.46%)
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=902, variant=1487, delta=585 (64.86%)
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=1793, variant=2343, delta=550 (30.67%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=1793, variant=2310, delta=517 (28.83%)
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=1676, variant=2017, delta=341 (20.35%)
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=902, variant=1186, delta=284 (31.49%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=902, variant=1184, delta=282 (31.26%)
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=1676, variant=1849, delta=173 (10.32%)
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=1676, variant=1808, delta=132 (7.88%)
- `gpu_threadgroup_minimal.trace` `metal-gpu-state-intervals`: baseline=1793, variant=1669, delta=-124 (-6.92%)
- `gpu_threadgroup_minimal.trace` `metal-gpu-intervals`: baseline=902, variant=835, delta=-67 (-7.43%)
- `gpu_threadgroup_minimal.trace` `metal-driver-intervals`: baseline=1676, variant=1630, delta=-46 (-2.74%)
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=822, variant=861, delta=39 (4.74%)
- `gpu_register_pressure.trace` `metal-command-buffer-completed`: baseline=822, variant=847, delta=25 (3.04%)
- `gpu_threadgroup_heavy.trace` `metal-command-buffer-completed`: baseline=822, variant=837, delta=15 (1.82%)
- `gpu_threadgroup_minimal.trace` `metal-command-buffer-completed`: baseline=822, variant=810, delta=-12 (-1.46%)
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=800, variant=799, delta=-1 (-0.12%)
- `gpu_register_pressure.trace` `metal-application-encoders-list`: baseline=800, variant=800, delta=0 (0.00%)
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=800, variant=800, delta=0 (0.00%)
- `gpu_threadgroup_minimal.trace` `metal-application-encoders-list`: baseline=800, variant=800, delta=0 (0.00%)

Top density deltas (rows/sec) on metal schemas:
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=1754.1 r/s, variant=2425.2 r/s, delta=671.1 r/s
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=1639.6 r/s, variant=1275.5 r/s, delta=-364.1 r/s
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=882.4 r/s, variant=1243.0 r/s, delta=360.6 r/s
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=1639.6 r/s, variant=1898.2 r/s, delta=258.5 r/s
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=1639.6 r/s, variant=1381.3 r/s, delta=-258.3 r/s
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=782.6 r/s, variant=547.2 r/s, delta=-235.5 r/s
- `gpu_register_pressure.trace` `metal-application-encoders-list`: baseline=782.6 r/s, variant=551.9 r/s, delta=-230.8 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=1754.1 r/s, variant=1982.6 r/s, delta=228.5 r/s
- `gpu_register_pressure.trace` `metal-command-buffer-completed`: baseline=804.2 r/s, variant=584.3 r/s, delta=-219.9 r/s
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=804.2 r/s, variant=589.7 r/s, delta=-214.5 r/s
