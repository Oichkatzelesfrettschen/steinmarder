# 2026-04-01 x130e Source Snapshot

This blessed snapshot preserves the current remote x130e TeraScale/Terakan
state before any workspace cleanup or refactor.

## Captured facts

- remote host used for import: `nick@10.0.0.88`
- live Mesa checkout: `/home/nick/mesa-26-debug`
- live findings depot: `/home/nick/eric/TerakanMesa/findings`
- Mesa commit at snapshot time:
  `3f173c02d169ccc38c88fea9d58ce46ee1cf8d55`
- checkout state: detached `HEAD`

## Why this snapshot matters

- the remote Mesa worktree is actively dirty across `r600`, NIR, winsys,
  runtime, and `zink`
- the remote tree contains an untracked `src/amd/terascale/` subtree
- the findings depot already contains the decision-grade reports needed for the
  local compendium and later results monograph

## Included artifacts

- `inventory/remote_mesa_head.txt`
- `inventory/remote_mesa_status.txt`
- `inventory/remote_terakan_tree.txt`
- `inventory/remote_findings_inventory.txt`
- `inventory/remote_findings_hashes.txt`
- curated copies of the core findings and toolkit notes
- `SOURCE_MANIFEST.tsv` with local hashes and remote source paths
