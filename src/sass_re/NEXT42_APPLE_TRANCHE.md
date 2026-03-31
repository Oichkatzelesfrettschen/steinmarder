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
