# xctrace Row Delta Summary

- baseline_trace: `gpu_baseline.trace`
- compared_traces: `3`

Top absolute deltas on metal schemas:
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=3100, variant=1974, delta=-1126 (-36.32%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=3100, variant=2011, delta=-1089 (-35.13%)
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=1655, variant=1010, delta=-645 (-38.97%)
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=2413, variant=1785, delta=-628 (-26.03%)
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=2413, variant=1800, delta=-613 (-25.40%)
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=1655, variant=1043, delta=-612 (-36.98%)
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=3100, variant=3460, delta=360 (11.61%)
- `gpu_threadgroup_heavy.trace` `metal-command-buffer-completed`: baseline=1075, variant=872, delta=-203 (-18.88%)
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=997, variant=797, delta=-200 (-20.06%)
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=1655, variant=1854, delta=199 (12.02%)
- `gpu_register_pressure.trace` `metal-command-buffer-completed`: baseline=1075, variant=876, delta=-199 (-18.51%)
- `gpu_register_pressure.trace` `metal-application-encoders-list`: baseline=997, variant=800, delta=-197 (-19.76%)
- `gpu_threadgroup_heavy.trace` `metal-application-encoders-list`: baseline=997, variant=800, delta=-197 (-19.76%)
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=1075, variant=906, delta=-169 (-15.72%)
- `gpu_occupancy_heavy.trace` `metal-driver-intervals`: baseline=2413, variant=2261, delta=-152 (-6.30%)

Top density deltas (rows/sec) on metal schemas:
- `gpu_register_pressure.trace` `metal-gpu-state-intervals`: baseline=2754.4 r/s, variant=2170.2 r/s, delta=-584.2 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-state-intervals`: baseline=2754.4 r/s, variant=3311.5 r/s, delta=557.1 r/s
- `gpu_threadgroup_heavy.trace` `metal-gpu-state-intervals`: baseline=2754.4 r/s, variant=2234.8 r/s, delta=-519.6 r/s
- `gpu_register_pressure.trace` `metal-gpu-intervals`: baseline=1470.5 r/s, variant=1110.4 r/s, delta=-360.1 r/s
- `gpu_threadgroup_heavy.trace` `metal-gpu-intervals`: baseline=1470.5 r/s, variant=1159.1 r/s, delta=-311.4 r/s
- `gpu_occupancy_heavy.trace` `metal-gpu-intervals`: baseline=1470.5 r/s, variant=1774.4 r/s, delta=303.9 r/s
- `gpu_register_pressure.trace` `metal-driver-intervals`: baseline=2144.0 r/s, variant=1962.4 r/s, delta=-181.6 r/s
- `gpu_threadgroup_heavy.trace` `metal-driver-intervals`: baseline=2144.0 r/s, variant=2000.3 r/s, delta=-143.6 r/s
- `gpu_occupancy_heavy.trace` `metal-application-encoders-list`: baseline=885.8 r/s, variant=762.8 r/s, delta=-123.1 r/s
- `gpu_occupancy_heavy.trace` `metal-command-buffer-completed`: baseline=955.1 r/s, variant=867.1 r/s, delta=-88.0 r/s
