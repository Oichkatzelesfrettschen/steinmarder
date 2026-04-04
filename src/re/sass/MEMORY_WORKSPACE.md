# Memory — Workspace

## Local / Remote Split

Local canonical coordination surface:

- `steinmarder/src/sass_re`

Remote implementation and evidence surfaces on x130e:

- `/home/eirikr/workspaces/mesa/mesa-26-debug`
- `/home/eirikr/workspaces/mesa/deqp`
- `/home/eirikr/workspaces/mesa/dxvk`
- `/home/eirikr/workspaces/research/TerakanMesa`
- archived project-adjacent material under
  `/home/eirikr/workspaces/research/x130e-eric-archive`
- non-canonical stragglers and machine-history material remain under `/home/nick`
  only as legacy/archive inputs

Normalized remote layout:

- `/home/eirikr/workspaces/mesa/mesa-26-debug`
- `/home/eirikr/workspaces/mesa/deqp`
- `/home/eirikr/workspaces/mesa/dxvk`
- `/home/eirikr/workspaces/research/TerakanMesa`
- `/home/eirikr/workspaces/research/x130e-eric-archive`
- `/home/eirikr/artifacts/mesa/{builds,stage,traces,conformance,benchmarks}`

## Current Known State

- x130e Mesa source checkout now lives at
  `/home/eirikr/workspaces/mesa/mesa-26-debug`
- migration from `/home/nick` into `/home/eirikr` has been executed
- the preserved source snapshot is published to the private GitHub remote under
  `snapshot/2026-04-01-source-preserved`
- the main implementation branch now tracks the private GitHub origin
- the active implementation branch is now the unified `main`
- preserved local blessed snapshot exists at
  [`results/x130e_terascale/blessed/20260401_x130e_source_snapshot/`](results/x130e_terascale/blessed/20260401_x130e_source_snapshot/README.md)
- migration script exists at
  [`scripts/migrate_x130e_workspace.sh`](scripts/migrate_x130e_workspace.sh)
- corpus and machine-state generator exists at
  [`scripts/build_terascale_project_state.py`](scripts/build_terascale_project_state.py)

## Account / Access Notes

- `eirikr@x130e` exists and has SSH key access
- `x130e` hostname is the preferred alias
- `x130e-ip` exists as a fallback when local discovery is flaky
- `ssh x130e` now defaults to `eirikr`
- `ssh x130e-tmux` attaches to the persistent remote tmux session
  `steinmarder`
- additional tmux/admin aliases exist for fresh X11-capable sessions:
  `x130e-x11` and `x130e-tmux-x11`
- x130e has a GUI-capable `sudo -A` path via
  `/home/eirikr/.local/bin/sudo-askpass`
- sudo timestamp timeout is configured as infinite for both `eirikr` and `nick`

## Branching / Remotes

Current remote Mesa branch scheme:

- `main`
- `snapshot/2026-04-01-source-preserved`
- `work/terakan-vk-build-unblock`
- `work/r600-rusticl-compute`
- `work/zink-dri3-wsi`
- `work/drm-dri-observability`

Current remote wiring:

- `origin`: `git@github.com:Oichkatzelesfrettschen/mesa-26-debug-gororoba.git`
- `upstream`: `https://gitlab.freedesktop.org/mesa/mesa.git`
- `main` tracks `origin/main`

Remote build workflow:

- use SSH ControlMaster locally plus `ssh x130e-tmux` for persistent shell reuse
- use `clang-19` through `/tmp/distcc-wrap/{cc,c++}`
- x130e distcc host pool:
  - `DESKTOP-CKP9KB6/32,lzo`
  - `127.0.0.1/4`
- current active build root:
  `/home/eirikr/workspaces/mesa/mesa-26-debug/build-terakan-distcc`
- current probe/tracing control surface:
  - `TERASCALE_PROBE_PROGRAM.md`
  - `scripts/run_terascale_probe_tranche.sh`
  - `scripts/run_terascale_trace_stack.sh`
  - `probes/r600/`

Repo role split:

- remote Mesa repo: implementation source of truth
- local `src/sass_re`: documentation, manifests, synthesis, planning, probes,
  and results
