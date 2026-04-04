# System Trace Probes

This lane keeps the host-side tracing stack close to the shader and kernel
probes instead of treating CPU/DRM/IO evidence as a separate project.

The canonical capture families are:

- `perf stat` and `perf record`
- `trace-cmd` DRM and scheduler tracepoints
- `radeontop` block-utilization samples
- `AMDuProfCLI` where available
- VM, page-fault, and IO summaries tied to a probe tranche

The runner lives at `scripts/run_terascale_trace_stack.sh`.
