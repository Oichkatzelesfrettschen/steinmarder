# Blessed Run Summary: 2026-03-30 Tranche 1 (r2)

This directory is the promoted, curated snapshot of the successful Apple
silicon tranche runner execution:

- source run: `src/apple_re/results/tranche1_manual_20260330_r2/`
- promoted on: `2026-03-30`
- final status: `42 / 42` steps `ok`

## Outcome

- CPU lane: matrix build/disasm/timing completed across `O0/O2/O3/Ofast`
- Metal lane: host harness + probe compile + timing/correctness completed
- Neural lane: `torch/coreml/mlx/jax` probe captures completed
- orchestration: no failed steps (`quality_gates.txt` reports `failed_steps=0`)

## Key metrics (from promoted artifacts)

- `hyperfine` mean for `sm_apple_cpu_latency_O3 --iters 5000 --csv`:
  `0.00307 s`
- `metal_timing.csv` run (`iters=200`, `width=1024`):
  - `elapsed_ns=31974000`
  - `ns_per_iter=159870.000000`
  - `ns_per_element=156.123046875`

## Included in this blessed snapshot

- manifests and gate status:
  - `step_status.csv`
  - `run_manifest.json` (runner-emitted, mid-run snapshot)
  - `run_manifest_final.json` (final 42-step summary)
  - `quality_gates.txt`
- capability + environment snapshots:
  - `capabilities.json`
  - `tool_matrix.csv`
  - `versions.json`
- CPU / Metal / neural result tables and supporting diagnostics

## Raw trace bundles

Large `.trace` artifacts and full raw logs remain in:

- `src/apple_re/results/tranche1_manual_20260330_r2/`

They are intentionally referenced, not duplicated here, to keep the blessed
snapshot lightweight and review-friendly.
