# Apple Tranche1 Keepalive Summary

- **Promotion note:** this keepalive evidence has been promoted into the dated Apple hierarchy for downstream review.
- **Phase E keepalive (2026-03-30_tranche1_phaseE_keepalive):** `fs_usage_gpu_host.txt` now shows real filesystem events for `sm_apple_metal_probe_host` thanks to the cached sudo keepalive, and `host_capture_pid.txt` can be replayed with `leaks`/`vmmap` once the harness runs again.
- **C,D,E keepalive (2026-03-30_tranche1_CDE_keepalive):** The rerun extract/analyze pipeline regenerated `xctrace_row_deltas.csv` plus the new `xctrace_row_density.csv`, confirming the register-pressure variant trades off density for throughput while the occupancy-heavy variant keeps the highest interval density and the baseline/variant deltas stay stable across normalized rows/sec.
- **Register vs occupancy deltas:** `xctrace_row_delta_summary.md` inside the CDE run now highlights the metal schemas with the largest absolute deltas and the newly computed density rows confirm the register-pressure variant loses density while the occupancy-heavy variant retains it near `+579 r/s` on `metal-gpu-state-intervals`.
- **Evidence bundle:** See `2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz` for the full CUDA-grade Apple evidence package that captures both keepalive directories and their xtrace artifacts.
