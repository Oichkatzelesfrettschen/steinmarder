# Apple Tranche1 Run Summary

- run_dir: `src/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/`
- source_run: `src/apple_re/results/tranche1_20260330_205404/`
- phases: `C,D,E`
- sudo_mode: `keepalive`
- promotion_status: `promoted`
- total_steps: `62`
- evidence: `fs_usage_gpu_host.txt`, `gpu_host_leaks.txt`, `gpu_host_vmmap.txt`, `xctrace_row_density.csv`, `counter_latency_report.md`
- density_story: `gpu_occupancy_heavy.trace` delivered the strongest positive delta on `metal-gpu-state-intervals` at `+678.4869 r/s`, while `gpu_register_pressure.trace` was the weakest at `-609.9195 r/s`
- variant_note: `gpu_threadgroup_minimal.trace` was the strongest positive delta on `metal-application-encoders-list`, `metal-command-buffer-completed`, and `metal-driver-intervals`
