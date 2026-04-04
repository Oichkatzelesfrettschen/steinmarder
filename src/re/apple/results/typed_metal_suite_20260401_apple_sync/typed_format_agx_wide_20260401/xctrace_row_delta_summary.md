# xctrace Row Delta Summary

- baseline_trace: `gpu_fmt_float32.trace`
- compared_traces: `9`

Top absolute deltas on metal schemas:
- `gpu_fmt_uint64.trace` `metal-gpu-state-intervals`: baseline=983, variant=8485, delta=7502 (763.17%)
- `gpu_fmt_int32.trace` `metal-gpu-state-intervals`: baseline=983, variant=7503, delta=6520 (663.28%)
- `gpu_fmt_fp4.trace` `metal-gpu-state-intervals`: baseline=983, variant=6499, delta=5516 (561.14%)
- `gpu_fmt_uint64.trace` `metal-gpu-intervals`: baseline=492, variant=4673, delta=4181 (849.80%)
- `gpu_fmt_int64.trace` `metal-gpu-state-intervals`: baseline=983, variant=5064, delta=4081 (415.16%)
- `gpu_fmt_int32.trace` `metal-gpu-intervals`: baseline=492, variant=3920, delta=3428 (696.75%)
- `gpu_fmt_fp4.trace` `metal-gpu-intervals`: baseline=492, variant=3410, delta=2918 (593.09%)
- `gpu_fmt_int16.trace` `metal-gpu-state-intervals`: baseline=983, variant=3788, delta=2805 (285.35%)
- `gpu_fmt_uint64.trace` `metal-driver-intervals`: baseline=876, variant=3642, delta=2766 (315.75%)
- `gpu_fmt_int64.trace` `metal-gpu-intervals`: baseline=492, variant=2661, delta=2169 (440.85%)
- `gpu_fmt_int32.trace` `metal-driver-intervals`: baseline=876, variant=2795, delta=1919 (219.06%)
- `gpu_fmt_fp4.trace` `metal-driver-intervals`: baseline=876, variant=2645, delta=1769 (201.94%)
- `gpu_fmt_int16.trace` `metal-gpu-intervals`: baseline=492, variant=1951, delta=1459 (296.54%)
- `gpu_fmt_int64.trace` `metal-driver-intervals`: baseline=876, variant=2108, delta=1232 (140.64%)
- `gpu_fmt_uint4.trace` `metal-gpu-state-intervals`: baseline=983, variant=1912, delta=929 (94.51%)
- `gpu_fmt_int4.trace` `metal-gpu-state-intervals`: baseline=983, variant=1875, delta=892 (90.74%)
- `gpu_fmt_int16.trace` `metal-driver-intervals`: baseline=876, variant=1664, delta=788 (89.95%)
- `gpu_fmt_uint16.trace` `metal-gpu-state-intervals`: baseline=983, variant=1728, delta=745 (75.79%)
- `gpu_fmt_uint64.trace` `metal-command-buffer-completed`: baseline=438, variant=1052, delta=614 (140.18%)
- `gpu_fmt_uint4.trace` `metal-gpu-intervals`: baseline=492, variant=982, delta=490 (99.59%)

Top density deltas (rows/sec) on metal schemas:
- `gpu_fmt_int32.trace` `metal-gpu-state-intervals`: baseline=406.0 r/s, variant=4151.1 r/s, delta=3745.0 r/s
- `gpu_fmt_int16.trace` `metal-gpu-state-intervals`: baseline=406.0 r/s, variant=4038.0 r/s, delta=3631.9 r/s
- `gpu_fmt_int64.trace` `metal-gpu-state-intervals`: baseline=406.0 r/s, variant=3127.1 r/s, delta=2721.1 r/s
- `gpu_fmt_int32.trace` `metal-gpu-intervals`: baseline=203.2 r/s, variant=2168.8 r/s, delta=1965.5 r/s
- `gpu_fmt_int16.trace` `metal-gpu-intervals`: baseline=203.2 r/s, variant=2079.8 r/s, delta=1876.5 r/s
- `gpu_fmt_int64.trace` `metal-gpu-intervals`: baseline=203.2 r/s, variant=1643.2 r/s, delta=1440.0 r/s
- `gpu_fmt_int16.trace` `metal-driver-intervals`: baseline=361.8 r/s, variant=1773.8 r/s, delta=1412.0 r/s
- `gpu_fmt_int32.trace` `metal-driver-intervals`: baseline=361.8 r/s, variant=1546.3 r/s, delta=1184.5 r/s
- `gpu_fmt_int4.trace` `metal-gpu-state-intervals`: baseline=406.0 r/s, variant=1368.0 r/s, delta=962.0 r/s
- `gpu_fmt_int64.trace` `metal-driver-intervals`: baseline=361.8 r/s, variant=1301.7 r/s, delta=939.9 r/s
