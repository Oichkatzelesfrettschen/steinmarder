# Apple Silicon Next 42-Step Tranche

This follow-on tranche turns the current promoted keepalive evidence into a
repeatable analysis and extension loop.

## Phase A: Promote and verify

1. Compare the newest promoted keepalive bundle against the prior promoted C/D/E
   synthesis bundle.
2. Confirm `fs_usage_gpu_host.txt` is populated and still tied to a real PID.
3. Confirm `gpu_host_leaks.txt` and `gpu_host_vmmap.txt` are present and readable.
4. Confirm `xctrace_row_density.csv` was generated from `xctrace_trace_health.csv`.
5. Confirm `counter_latency_report.md` lists baseline plus the four comparison
   Metal traces.
6. Confirm `RUN_SUMMARY.md` and `KEEPALIVE_SUMMARY.md` describe the promoted
   keepalive evidence without using legacy bundle labels.

## Phase B: CPU lane expansion

7. Add a load-chain probe for dependent integer and pointer-heavy operations.
8. Add a store-chain probe for write-back latency and memory ordering effects.
9. Add a shuffle-heavy probe to isolate lane-crossing costs.
10. Add atomic-probe variants for CAS, exchange, and add-style contention.
11. Add transcendental probes for sqrt, log, exp, and reciprocal-style families.
12. Record the CPU lane in CSV form so the Apple timings line up with the SASS
    tables.
13. Disassemble the CPU probes and archive the relevant asm excerpts beside the
    timing tables.

## Phase C: Metal lane refinement

14. Sweep the `threadgroup_heavy`, `threadgroup_minimal`, `occupancy_heavy`, and
    `register_pressure` variants against the same host harness.
15. Add a register-light variant to bracket the density behavior from the other
    side.
16. Export `xctrace` metrics for every Metal variant with the same schema filter.
17. Recompute `xctrace_row_density.csv` after each sweep so row-rate bias stays
    normalized.
18. Rank the Metal variants by density delta and by elapsed timing separately.
19. Compare the new ranking against the prior promoted keepalive bundle.
20. Record which variant leads each schema so the findings stay traceable.

## Phase D: Host diagnostics

21. Keep the FS event probe in the host path so `fs_usage` continues to emit
    non-empty signal.
22. Preserve PID-scoped `leaks` capture as the primary memory-safety view.
23. Preserve PID-scoped `vmmap` capture as the primary mapping/heap view.
24. Capture `sample` and `spindump` only as supporting evidence, not as the
    headline artifacts.
25. Keep the keepalive sudo path validated under the same automation contract.
26. Verify the host diagnostics remain stable after repeated reruns.

## Phase E: Neural lane follow-on

27. Rebuild the local neural environment only when the lockfile or package set
    changes.
28. Sweep Core ML placement across CPU, GPU, and ANE.
29. Sweep dtype and quantization settings across the same model family.
30. Record the placement matrix as a companion artifact rather than a separate
    ad hoc note.
31. Keep PyTorch MPS and MLX sidecar checks as comparison points.
32. Capture any backend fallback behavior directly in the summary text.

## Phase F: Publication and linkage

33. Promote the newest results into a dated promoted hierarchy.
34. Keep the Apple track indexed in `docs/README.md` alongside SASS and Ryzen.
35. Keep the Apple bridge note linked from the SASS docs index.
36. Keep the promoted bundle tarball and summaries referenced together.
37. Update the Apple README pointers to the latest promoted bundle.
38. Update the roadmap snapshot when a new promoted bundle changes the findings.

## Phase G: Quality gates

39. Re-run the Apple tranche harness sanity checks after any script edit.
40. Re-run the density comparison helper after any new promoted bundle.
41. Keep the summary text aligned with the actual artifacts on disk.
42. Commit the docs/harness patch separately from the promoted results tree.

## Concrete run sequence

```sh
# 1) Compare the newest promoted keepalive bundle against the prior promoted C/D/E bundle
python3 src/sass_re/apple_re/scripts/compare_xctrace_density_runs.py \
  src/sass_re/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/xctrace_row_density.csv \
  src/sass_re/apple_re/results/blessed/2026-03-30_tranche1_r6_cde/xctrace_row_density.csv \
  /tmp/apple_r7_vs_r6_density.csv \
  /tmp/apple_r7_vs_r6_density.md

# 2) Refresh the current keepalive evidence with the patched tranche harness
SUDO_ASKPASS=/Users/eirikr/bin/askpass \
src/sass_re/apple_re/scripts/run_apple_tranche1.sh \
  --phase C,D,E \
  --sudo keepalive \
  --iters 500000

# 3) Re-run the row-delta and density extraction over the refreshed run directory
python3 src/sass_re/apple_re/scripts/extract_xctrace_metrics.py /path/to/new_run_dir
python3 src/sass_re/apple_re/scripts/analyze_xctrace_row_deltas.py \
  /path/to/new_run_dir/xctrace_metric_row_counts.csv \
  /path/to/new_run_dir/xctrace_trace_health.csv \
  /path/to/new_run_dir/xctrace_row_deltas.csv \
  /path/to/new_run_dir/xctrace_row_delta_summary.md \
  /path/to/new_run_dir/xctrace_row_density.csv

# 4) Compare the refreshed density output against the prior promoted bundle again
python3 src/sass_re/apple_re/scripts/compare_xctrace_density_runs.py \
  /path/to/new_run_dir/xctrace_row_density.csv \
  src/sass_re/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/xctrace_row_density.csv \
  /tmp/apple_new_vs_r7_density.csv \
  /tmp/apple_new_vs_r7_density.md
```

## Run checklist with expected artifacts

### 1-6: Promote and verify

Use the density comparison helper first, then inspect the promoted bundle.

Expected artifacts:
- `xctrace_density_compare.csv`
- `xctrace_density_compare.md`
- `fs_usage_gpu_host.txt`
- `gpu_host_leaks.txt`
- `gpu_host_vmmap.txt`
- `counter_latency_report.md`
- `RUN_SUMMARY.md`
- `KEEPALIVE_SUMMARY.md`

### 7-13: CPU lane expansion

Use `src/sass_re/apple_re/scripts/run_next42_cpu_probes.sh` to run the CPU probe draft.

Expected artifacts:
- `cpu_probe_families.txt`
- `cpu_probe_inventory.txt`
- `compile_matrix.txt`
- `cpu_runs/all_probes.csv`
- `cpu_runs/*.csv`
- `cpu_runs/hyperfine.csv`
- `disassembly/*.otool.txt`
- `disassembly/*.objdump.txt`
- `llvm_mca/*.mca.txt`
- `diagnostics/asan_run.csv`
- `diagnostics/leaks.txt`
- `diagnostics/vmmap.txt`
- `cpu_notes.md`

### 14-20: Metal lane refinement

Use `src/sass_re/apple_re/scripts/run_next42_metal_probes.sh` to run the Metal probe draft.

Expected artifacts:
- `metal_variant_matrix.csv`
- `metal_variant_matrix_published.csv`
- `metal_timing.csv`
- `metal_probe_variants/*.metallib`
- `gpu_baseline.trace/`
- `gpu_threadgroup_heavy.trace/`
- `gpu_threadgroup_minimal.trace/`
- `gpu_occupancy_heavy.trace/`
- `gpu_register_pressure.trace/`
- `xctrace_trace_health.csv`
- `xctrace_metric_row_counts.csv`
- `xctrace_row_deltas.csv`
- `xctrace_row_density.csv`
- `xctrace_row_delta_summary.md`
- `counter_latency_report.md`
- `metal_notes.md`

### 21-26: Host diagnostics

Expected artifacts:
- `host_capture_pid.txt`
- `sample_gpu_host.txt`
- `spindump_gpu_host.txt`
- `fs_usage_gpu_host.txt`
- `gpu_host_leaks.txt`
- `gpu_host_vmmap.txt`
- `powermetrics_gpu.txt`

### 27-32: Neural lane follow-on

Use `src/sass_re/apple_re/scripts/run_next42_neural_suite.sh` to run the neural suite.

Expected artifacts:
- `neural_probe.json`
- `neural_model_family.json`
- `coreml_placement_sweep.json`
- `torch_cpu_vs_mps.json`
- `mlx_jax_checks.json`
- `run_manifest.json`
- `run_manifest_final.json`
- `summary.md`

### 33-38: Publication and linkage

Expected artifacts:
- dated promoted results directory under `src/sass_re/apple_re/results/blessed/`
- promoted bundle tarball
- `KEEPALIVE_SUMMARY.md`
- `docs/README.md` index entries
- `README.md` top-level cross-links
- `FRONTIER_ROADMAP_APPLE.md`
- `APPLE_SILICON_RE_BRIDGE.md`

### 39-42: Quality gates

Expected artifacts:
- `step_status.csv`
- `run_manifest.json`
- `run_manifest_final.json`
- `quality_gates.txt`
- `failed_steps.csv`
- commit containing only the docs/harness patch
