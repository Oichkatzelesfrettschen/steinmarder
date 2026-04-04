# Blessed x130e Snapshots

Promoted x130e/TeraScale snapshots live here.

Each promoted snapshot should include:

- imported curated findings
- a source manifest with hashes and remote paths
- remote Mesa state capture (`HEAD`, detached/branch state, dirty worktree)
- any compact summary needed to understand why this snapshot matters

The first promoted snapshot was taken after confirming:

- remote Mesa checkout at commit `3f173c0`
- detached `HEAD`
- active dirty worktree across `r600`, `zink`, winsys, runtime, and `src/amd/terascale/`
- local repo already had an active TeraScale roadmap and performance tracker
