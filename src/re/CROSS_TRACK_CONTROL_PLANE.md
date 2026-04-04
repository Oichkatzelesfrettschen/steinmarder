# Cross-Track Control Plane

This note makes the coexistence rule explicit:

- CUDA findings stay first-class.
- Vulkan findings stay first-class.
- Apple typed-lane findings stay first-class.
- TeraScale and Terakan findings stay first-class.

The point of `src/sass_re/` is not to replace one track with another. It is to
give every active architecture lane the same control-plane contract:

1. one canonical narrative entrypoint
2. one or more machine-readable ledgers
3. one runner or preservation path that can regenerate fresh evidence
4. one results root that preserves provenance
5. one explicit promotion path back into canonical docs and tables

The machine-readable registry for that contract is:

- [`tables/table_cross_track_control_plane.csv`](tables/table_cross_track_control_plane.csv)

## Shared law

Every architecture lane should answer the same five questions:

1. What hardware and software surface is this track trying to understand?
2. Which docs are canonical enough to cite?
3. Which tables or normalized artifacts carry the current decision-grade facts?
4. Which runner, sync script, or importer can regenerate the next evidence set?
5. Where do raw or blessed results live when we need provenance?

If a lane cannot answer those questions yet, it is still a sketch, not a
stable research surface.

## Current active lanes

### SASS / SM89

- canonical note: [`README.md`](README.md) plus [`RESULTS.md`](RESULTS.md)
- machine-readable surfaces: the `table_a*.csv` frontier tables
- regeneration path: direct probe builds and tranche runners in `src/sass_re/`
- results root: `src/sass_re/results/`

### Apple CPU + Metal + Neural

- canonical note: [`APPLE_TYPED_BOUNDARY_ATLAS.md`](APPLE_TYPED_BOUNDARY_ATLAS.md)
- machine-readable surfaces:
  [`tables/table_apple_type_taxonomy.csv`](tables/table_apple_type_taxonomy.csv),
  [`tables/table_apple_type_boundary_matrix.csv`](tables/table_apple_type_boundary_matrix.csv),
  [`tables/table_apple_type_timings.csv`](tables/table_apple_type_timings.csv)
- regeneration path:
  [`../apple_re/scripts/run_apple_tranche1.sh`](../apple_re/scripts/run_apple_tranche1.sh)
  and the focused next-step runners under `../apple_re/scripts/`
- promotion path:
  [`scripts/sync_apple_typed_atlas.py`](scripts/sync_apple_typed_atlas.py)
  with machine-readable promotion from `metal_timing.csv`,
  `gpu_hardware_counters.csv`, `gpu_hardware_counter_deltas.csv`, and selected
  `xctrace` exports
- results root: `src/sass_re/apple_re/results/`

### CUDA LBM

- canonical note: [`../cuda_lbm/README.md`](../cuda_lbm/README.md)
- machine-readable surfaces: `cuda_variant_matrix.csv`,
  `cuda_launch_health.csv`, `cuda_occupancy_vs_bw.csv`
- regeneration path:
  [`../cuda_lbm/scripts/run_cuda_decision_tranche.sh`](../cuda_lbm/scripts/run_cuda_decision_tranche.sh)
- results root: `src/cuda_lbm/results/`

### r600 / TeraScale-2 / Terakan Vulkan

- canonical note:
  [`FRONTIER_ROADMAP_R600_TERASCALE.md`](FRONTIER_ROADMAP_R600_TERASCALE.md)
- machine-readable surfaces:
  [`tables/table_x130e_terascale_findings_matrix.csv`](tables/table_x130e_terascale_findings_matrix.csv),
  [`tables/table_x130e_capability_matrix.csv`](tables/table_x130e_capability_matrix.csv),
  [`tables/table_x130e_build_blockers.csv`](tables/table_x130e_build_blockers.csv),
  [`tables/table_x130e_pattern_transfer.csv`](tables/table_x130e_pattern_transfer.csv),
  [`tables/table_x130e_run_manifest.csv`](tables/table_x130e_run_manifest.csv)
- regeneration path:
  [`scripts/sync_x130e_terascale_compendium.py`](scripts/sync_x130e_terascale_compendium.py)
  plus local state builders such as
  [`scripts/build_terascale_project_state.py`](scripts/build_terascale_project_state.py)
- results root: `src/sass_re/results/x130e_terascale/`

### Ryzen 5600X3D

- canonical note:
  [`FRONTIER_ROADMAP_RYZEN_5600X3D.md`](FRONTIER_ROADMAP_RYZEN_5600X3D.md)
- machine-readable surfaces: currently lighter than the other lanes and still
  needs fuller normalization
- regeneration path: local CPU/cache tranche work
- results root: `src/sass_re/results/`

## Promotion discipline

Use the same promotion discipline across every lane:

- raw captures stay in a results root first
- promoted facts move into CSV or markdown ledgers second
- roadmap claims link back to those ledgers third
- if a lane has hardware counters, prefer promoting a normalized counter CSV
  rather than only prose summaries or screenshots

That keeps CUDA, Vulkan, Apple, and TeraScale findings mutually visible without
forcing them into one fake benchmark schema.

## Immediate next promotions

- extend the Apple hardware-counter lane from the new `occupancy_heavy`
  promotion to multi-variant baseline/threadgroup/register-pressure captures
- keep the CUDA decision tranche outputs linked from this control plane instead
  of leaving them isolated under `src/cuda_lbm/`
- continue importing x130e/Terakan results into the local TeraScale tables so
  Vulkan findings remain normalized beside Apple and CUDA work
