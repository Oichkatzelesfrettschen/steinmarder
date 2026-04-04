# xctrace Row Delta Summary

- baseline_trace: `gpu_fmt_float32.trace`
- compared_traces: `11`

Top absolute deltas on metal schemas:
- `gpu_fmt_tf32.trace` `metal-gpu-state-intervals`: baseline=1118, variant=3071, delta=1953 (174.69%)
- `gpu_fmt_mxfp8.trace` `metal-gpu-state-intervals`: baseline=1118, variant=2847, delta=1729 (154.65%)
- `gpu_fmt_fp8_e5m2.trace` `metal-gpu-state-intervals`: baseline=1118, variant=2568, delta=1450 (129.70%)
- `gpu_fmt_mxint8.trace` `metal-gpu-state-intervals`: baseline=1118, variant=2328, delta=1210 (108.23%)
- `gpu_fmt_tf32.trace` `metal-gpu-intervals`: baseline=577, variant=1592, delta=1015 (175.91%)
- `gpu_fmt_mxfp8.trace` `metal-gpu-intervals`: baseline=577, variant=1466, delta=889 (154.07%)
- `gpu_fmt_mxfp6.trace` `metal-gpu-state-intervals`: baseline=1118, variant=1977, delta=859 (76.83%)
- `gpu_fmt_fp8_e5m2.trace` `metal-gpu-intervals`: baseline=577, variant=1345, delta=768 (133.10%)
- `gpu_fmt_mxint8.trace` `metal-gpu-intervals`: baseline=577, variant=1213, delta=636 (110.23%)
- `gpu_fmt_tf32.trace` `metal-driver-intervals`: baseline=683, variant=1297, delta=614 (89.90%)
- `gpu_fmt_mxfp8.trace` `metal-driver-intervals`: baseline=683, variant=1233, delta=550 (80.53%)
- `gpu_fmt_fp8_e5m2.trace` `metal-driver-intervals`: baseline=683, variant=1205, delta=522 (76.43%)
- `gpu_fmt_mxfp6.trace` `metal-gpu-intervals`: baseline=577, variant=1084, delta=507 (87.87%)
- `gpu_fmt_mxint8.trace` `metal-driver-intervals`: baseline=683, variant=1105, delta=422 (61.79%)
- `gpu_fmt_mxfp6.trace` `metal-driver-intervals`: baseline=683, variant=1038, delta=355 (51.98%)
- `gpu_fmt_uint8.trace` `metal-gpu-state-intervals`: baseline=1118, variant=809, delta=-309 (-27.64%)
- `gpu_fmt_fp8_e4m3.trace` `metal-gpu-state-intervals`: baseline=1118, variant=891, delta=-227 (-20.30%)
- `gpu_fmt_uint8.trace` `metal-gpu-intervals`: baseline=577, variant=408, delta=-169 (-29.29%)
- `gpu_fmt_mxfp4.trace` `metal-gpu-state-intervals`: baseline=1118, variant=955, delta=-163 (-14.58%)
- `gpu_fmt_fp8_e4m3.trace` `metal-gpu-intervals`: baseline=577, variant=454, delta=-123 (-21.32%)

Top density deltas (rows/sec) on metal schemas:
- `gpu_fmt_mxfp6.trace` `metal-gpu-state-intervals`: baseline=1133.8 r/s, variant=1694.6 r/s, delta=560.8 r/s
- `gpu_fmt_mxint8.trace` `metal-gpu-state-intervals`: baseline=1133.8 r/s, variant=1675.9 r/s, delta=542.1 r/s
- `gpu_fmt_tf32.trace` `metal-gpu-state-intervals`: baseline=1133.8 r/s, variant=1526.0 r/s, delta=392.2 r/s
- `gpu_fmt_mxfp6.trace` `metal-gpu-intervals`: baseline=585.1 r/s, variant=929.1 r/s, delta=344.0 r/s
- `gpu_fmt_mxfp8.trace` `metal-driver-intervals`: baseline=692.6 r/s, variant=359.8 r/s, delta=-332.9 r/s
- `gpu_fmt_uint8.trace` `metal-gpu-state-intervals`: baseline=1133.8 r/s, variant=804.0 r/s, delta=-329.8 r/s
- `gpu_fmt_bfloat16.trace` `metal-gpu-state-intervals`: baseline=1133.8 r/s, variant=1448.4 r/s, delta=314.6 r/s
- `gpu_fmt_mxfp8.trace` `metal-gpu-state-intervals`: baseline=1133.8 r/s, variant=830.7 r/s, delta=-303.1 r/s
- `gpu_fmt_mxint8.trace` `metal-gpu-intervals`: baseline=585.1 r/s, variant=873.2 r/s, delta=288.1 r/s
- `gpu_fmt_int8.trace` `metal-gpu-state-intervals`: baseline=1133.8 r/s, variant=1370.5 r/s, delta=236.7 r/s
