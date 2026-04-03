# `src/sass_re` Agent Guide

This subtree is the canonical control plane for the steinmarder reverse
engineering tracks, including the x130e / r600 / TeraScale-2 / Terakan
program.

The root repo [`AGENTS.md`](../../AGENTS.md) remains the global reference for
steinmarder as a whole. This file narrows that guidance to the `src/sass_re`
project lane.

## Purpose

Use this subtree for:

- canonical reverse-engineering documentation
- machine-readable research tables
- probe, runner, and tranche tooling
- preserved and curated result artifacts
- cross-track method transfer from CUDA, Vulkan, Apple, CPU, and NVIDIA SASS
  work into the TeraScale lane

Do not treat the live remote Mesa worktree on x130e as the documentation source
of truth. The live Mesa tree is the implementation surface. This subtree is the
analysis, evidence, and coordination surface.

## Canonical Memory Files

- [`MEMORY_PROJECT.md`](MEMORY_PROJECT.md)
- [`MEMORY_WORKSPACE.md`](MEMORY_WORKSPACE.md)
- [`MEMORY_METHODS.md`](MEMORY_METHODS.md)
- [`MEMORY_FINDINGS.md`](MEMORY_FINDINGS.md)

## Canonical TeraScale Docs

- [`FRONTIER_ROADMAP_R600_TERASCALE.md`](FRONTIER_ROADMAP_R600_TERASCALE.md)
- [`TERAKAN_PERFORMANCE_TRACKER.md`](TERAKAN_PERFORMANCE_TRACKER.md)
- [`TERASCALE_SYNTHESIS.md`](TERASCALE_SYNTHESIS.md)
- [`TERASCALE_METHOD_TRANSFER.md`](TERASCALE_METHOD_TRANSFER.md)
- [`TERASCALE_CAPABILITY_LEDGER.md`](TERASCALE_CAPABILITY_LEDGER.md)
- [`TERASCALE_FILE_SCOPE.md`](TERASCALE_FILE_SCOPE.md)
- [`TERASCALE_EXTERNAL_REFERENCES.md`](TERASCALE_EXTERNAL_REFERENCES.md)

## Machine-Readable Control Surfaces

- [`tables/table_repo_doc_corpus.csv`](tables/table_repo_doc_corpus.csv)
- [`tables/table_repo_claims_matrix.csv`](tables/table_repo_claims_matrix.csv)
- [`tables/table_x130e_home_corpus.csv`](tables/table_x130e_home_corpus.csv)
- [`tables/table_x130e_file_inventory.csv`](tables/table_x130e_file_inventory.csv)
- [`tables/table_x130e_terascale_findings_matrix.csv`](tables/table_x130e_terascale_findings_matrix.csv)
- [`tables/table_x130e_capability_matrix.csv`](tables/table_x130e_capability_matrix.csv)
- [`tables/table_x130e_build_blockers.csv`](tables/table_x130e_build_blockers.csv)
- [`tables/table_x130e_pattern_transfer.csv`](tables/table_x130e_pattern_transfer.csv)
- [`tables/table_x130e_run_manifest.csv`](tables/table_x130e_run_manifest.csv)

## Operational Rules

1. Keep canonical outputs in open formats only:
   `.md`, `.txt`, `.csv`, `.tsv`, `.json`, `.svg`, `.sh`, `.py`.
2. Preserve provenance:
   every promoted claim should point back to a snapshot, source doc, or run.
3. Treat x130e remote findings as raw or preserved evidence, then merge the
   stable parts into canonical local docs and tables here.
4. Keep Zink and DXVK as dependent validation lanes, not early bring-up goals.
5. Target native-max Vulkan honestly. Record hardware or architecture hard
   stops explicitly rather than hiding them behind workaround language.

## Important Artifacts

- preserved x130e snapshot:
  [`results/x130e_terascale/blessed/20260401_x130e_source_snapshot/SUMMARY.md`](results/x130e_terascale/blessed/20260401_x130e_source_snapshot/SUMMARY.md)
- existing memory seed:
  [`MEMORY_NOTES.md`](MEMORY_NOTES.md)
- x130e import helper:
  [`scripts/sync_x130e_terascale_compendium.py`](scripts/sync_x130e_terascale_compendium.py)
- x130e corpus/table builder:
  [`scripts/build_terascale_project_state.py`](scripts/build_terascale_project_state.py)
- x130e migration script:
  [`scripts/migrate_x130e_workspace.sh`](scripts/migrate_x130e_workspace.sh)
