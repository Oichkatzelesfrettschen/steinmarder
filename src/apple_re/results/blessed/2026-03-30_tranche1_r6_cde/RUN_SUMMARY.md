# Apple M1 Tranche1 C/D/E Synthesis (r6)

Date: 2026-03-30  
Source run: `tranche1_CDE_cache_20260330_185518`  
Status: `18/18 executed steps OK` (phases `C,D,E` only; `hyperfine_cpu_timing` fails because `hyperfine` is not installed)

## What this run proves

- The CPU lane now captures `xctrace` time profiler, `dtrace`/`powermetrics`, and ASan diagnostics while also exercising the expanded register/atomic families via `sm_apple_cpu_latency`.
- The Metal lane now records four probe variants (`baseline`, `threadgroup_heavy`, `register_pressure`, `occupancy_heavy`) with timing deltas and density deltas (see `xctrace_row_density.csv`).
- Root instrumentation attempted `fs_usage` and emitted the expected “`sudo: a password is required`” guardrail because `sudo -n` is used; the rest of the host evidence (`sample`, `spindump`, `powermetrics_gpu.txt`) completed.
- `counter_latency_report.md` now lists both `xctrace_row_deltas.csv` and `xctrace_row_density.csv` so delta vs elapsed-normalized comparisons are explicit.

## Key artifacts

- `counter_latency_report.md`
- `metal_timing.csv`
- `xctrace_trace_health.csv`
- `xctrace_metric_row_counts.csv`
- `xctrace_row_deltas.csv`
- `xctrace_row_delta_summary.md`
- `xctrace_row_density.csv`
- `ANALYSIS_NEXT_STEPS.md`
