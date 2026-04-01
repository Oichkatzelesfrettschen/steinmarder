# Apple Tranche C/D/E Keepalive Analysis

Date: 2026-03-30
Run: `tranche1_CDE_keepalive_20260330_190100`
Scope: phases C, D, and E with `--sudo keepalive` (full CPU+GPU synthesis manifest)

## Executive signal

- The combined C/D/E manifest now runs under a cached sudo ticket so every host-side capture (fs_usage, sample, spindump) stays root-proxied for the duration of the GPU lane.
- Step E's host capture has been patched to invoke `leaks`/`vmmap` for `host_capture_pid.txt` on the next rerun, so future keepalive bundles will include those PID-scoped diagnostics in addition to the existing `fs_usage` data.
- The rerun extract/analyze pipeline regenerated `xctrace_row_delta_summary.md` plus the new `xctrace_row_density.csv`, and `KEEPALIVE_SUMMARY.md`/`2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz` collect the CUDA-grade evidence for downstream review.

## Key data points

### Metal timing (`metal_timing.csv`)

- `baseline`: 168.1 ns/element (200 iters, width 1024).
- `threadgroup_heavy`: 192.2 ns/element (160 iters) because extra threadgroup pressure slows the kernel.
- `occupancy_heavy`: 184.7 ns/element (160 iters) while adding arithmetic occupancy pressure.
- `register_pressure`: 192.7 ns/element (200 iters) despite the extra register work, confirming the per-variant throughput differences used in the density comparison.

### Normalized density (`xctrace_row_density.csv`)

- Occupancy-heavy extends the baseline density by ≈+557 r/s on `metal-gpu-state-intervals` and by +304 r/s on `metal-gpu-intervals`, proving the throughput gain is reflected in higher event density.
- Register-pressure loses roughly -584 r/s on `metal-gpu-state-intervals` and -360 r/s on `metal-gpu-intervals`, showing the density loss that matches the slower `ns/element`.
- All density deltas mirror the register vs occupancy delta story recorded in `xctrace_row_delta_summary.md` and the KEEPALIVE summary.

### Host diagnostics (`fs_usage` + `sample` + `spindump`)

- `fs_usage_gpu_host.txt` now emits repeated opens/writes/closes to `/private/tmp/steinmarder_fs_probe_*.log`, so the dedicated FS probe in the host harness is generating observable filesystem traffic.
- `sample_gpu_host.txt` + `spindump_gpu_host.txt` remain available for quick timing or stack capture if needed.
- The host-capture invocation is now instrumented to record `gpu_host_leaks.txt` and `gpu_host_vmmap.txt` on the next keepalive rerun, so the PID-scoped diagnostics will be in the follow-up bundle.

### Summary & bundling

- `KEEPALIVE_SUMMARY.md` stitches the phase-E and C/D/E keepalive runs into a single narrative.
- `2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz` packages those directories plus their xctrace assets so reviewers can treat the Apple lane like a CUDA-grade evidence bundle.

## CUDA-method parity status (Apple translation)

Completed equivalents:
- CUDA `cuobjdump/nvdisasm` → Metal/AIR disassembly + `metal_host` binaries.
- CUDA `ncu` → `xctrace` record + schema exports + normalized density CSVs.
- CUDA `nsys` → `xctrace` TOC/traces and the PID-scoped host captures (sample/spindump + `gpu_host_leaks`/`gpu_host_vmmap`).
- CUDA sanitizers/valgrind → ASan/UBSan + `leaks`/`vmmap` (now covering both CPU and GPU hosts).
- CUDA `strace`/FS traces → keepalive-enabled `fs_usage` probe that stores concrete filesystem events.

Still maturing:
- Tie the host PID evidence back into the neural lane so phase `F` can reuse the same instrumentation.
- Compare processed densities across multiple keepalive runs to ensure the register vs occupancy story is reproducible.

## Immediate next tranche (ranked)

1. Inspect `gpu_host_leaks.txt` and `gpu_host_vmmap.txt` from the keepalive run and confirm no residual host leaks occur after the GPU kernel exits.
2. Expand the register-pressure variant (or add a new `threadgroup_minimal` variant) so we can separate register pressure from shared-memory/threadgroup cost.
3. Launch a new `--phase E` keepalive pass while capturing `fs_usage` as carbon copy evidence for the new variant; include the new outputs in the next `KEEPALIVE_SUMMARY`.
4. Align the CPU throughput/latency pairs for the newly observed families (shuffle, atomics, transcendentals) so the Apple lane mirrors the NVIDIA latency-vs-throughput posture.
5. Publish the updated `2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz` and cross-link it through the root README + docs index.

## Suggested commands

```sh
src/apple_re/scripts/prime_sudo_cache.sh
src/apple_re/scripts/run_apple_tranche1.sh --phase E --sudo keepalive --iters 500000 --out src/apple_re/results/tranche1_phaseE_keepalive_$(date +%Y%m%d_%H%M%S)

src/apple_re/scripts/run_apple_tranche1.sh --phase C,D,E --sudo keepalive --iters 500000 --out src/apple_re/results/tranche1_CDE_keepalive_$(date +%Y%m%d_%H%M%S)
```
