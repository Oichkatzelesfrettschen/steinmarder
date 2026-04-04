# x130e / TeraScale Compendium

This lane is the local preservation and synthesis home for the active
x130e-based Radeon HD 6310 / TeraScale-2 / Terakan work.

At the repo level, this lane is also part of the shared cross-track control
plane documented in [`../../CROSS_TRACK_CONTROL_PLANE.md`](../../CROSS_TRACK_CONTROL_PLANE.md)
and registered in
[`../../tables/table_cross_track_control_plane.csv`](../../tables/table_cross_track_control_plane.csv).

It has a different job than the remote Mesa checkout:

- remote `nick@x130e:~/mesa-26-debug/` is the live driver workspace
- remote `nick@x130e:~/eric/TerakanMesa/` is the raw evidence and tooling depot
- local `src/sass_re/results/x130e_terascale/` is the curated compendium lane

## Layout

- `blessed/`
  - promoted snapshots that are stable enough to cite from roadmap and tracker
- `imports/`
  - generated preservation snapshots from the sync script
  - gitignored by default
- `workspace/`
  - future local notes on normalized Mesa/Terakan workspace layout
  - gitignored by default unless promoted

## Promoted synthesis artifacts

- [`../../tables/table_x130e_terascale_findings_matrix.csv`](../../tables/table_x130e_terascale_findings_matrix.csv)
  - machine-readable first-pass matrix for build blockers, perf baselines,
    conformance status, workspace migration targets, and next actions
- [`../../tables/table_x130e_home_corpus.csv`](../../tables/table_x130e_home_corpus.csv)
  - full first-pass inventory of the x130e `/home/nick` document corpus
- [`../../tables/table_x130e_file_inventory.csv`](../../tables/table_x130e_file_inventory.csv)
  - filtered project-facing inventory for canonical, validation, archive, and
    review work
- [`../../TERASCALE_FILE_SCOPE.md`](../../TERASCALE_FILE_SCOPE.md)
  - narrative description of the corpus-normalization and file-classification
    policy

## Preservation workflow

Use the sync script to capture the remote state without mutating it:

```sh
python3 src/sass_re/scripts/sync_x130e_terascale_compendium.py \
  --snapshot-name 20260401_source_snapshot
```

That script:

- records remote Mesa head, branch/detached status, dirty worktree state
- inventories `src/amd/terascale/` in the remote Mesa checkout
- inventories the remote findings tree
- imports a curated, decision-grade subset of the x130e findings and toolkit docs
- emits a `SOURCE_MANIFEST.tsv` with hashes and remote source paths

## What belongs here

Commit:

- blessed snapshots
- compact inventories and manifests
- decision-grade summaries and promoted findings

Do not commit by default:

- giant logs
- temporary imports
- raw build trees
- large profiler captures
