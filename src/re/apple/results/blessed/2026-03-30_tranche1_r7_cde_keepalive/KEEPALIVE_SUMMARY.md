# Apple Tranche1 Keepalive Summary

- keepalive_status: the cached sudo ticket stayed alive through the phase-E host capture path
- promotion_status: the run was promoted into the dated keepalive hierarchy
- fs_usage_signal: `fs_usage_gpu_host.txt` is populated with `315` lines of filesystem activity
- host_diagnostics: `gpu_host_leaks.txt` and `gpu_host_vmmap.txt` were recorded for the PID-scoped host capture
- trace_inventory: `counter_latency_report.md` now covers baseline plus `threadgroup_heavy`, `threadgroup_minimal`, `occupancy_heavy`, and `register_pressure`
- density_read: `threadgroup_minimal` won 3 of 5 schemas, `occupancy_heavy` won 2 of 5, and `register_pressure` was worst on all 5
- takeaway: the keepalive run produces a stable CUDA-grade Apple evidence bundle with real host signal instead of empty traces
