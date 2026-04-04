<!--
@file TERASCALE_SYNTHESIS.md
@brief Project synthesis and execution strategy for the x130e TeraScale-2 rehabilitation effort.

This document outlines the high-level project shape, preserved evidence, and working thesis
for modernizing the TeraScale full-stack driver ecosystem (including Terakan, r600, and Rusticl).
It serves as the definitive reference for near-term execution goals, details the current
active iteration build loop, and acts as an index linking to canonical roadmap and status tracking files.
-->
# TeraScale Synthesis

## Project Shape

The x130e program is no longer just “bring up a hobby Vulkan ICD.” It is a
full-stack rehabilitation effort for a TeraScale-2 machine where:

- Terakan is the native Vulkan lane
- r600 remains the native GL/GLES and Gallium base
- Rusticl is the compute validation and bug-finding lane
- Zink and DXVK are dependent validation layers that help expose capability gaps
- DRM/DRI/libva work matters when it unlocks graphics, media, or telemetry

## What the Preserved Evidence Already Says

The blessed x130e snapshot and imported findings point to a coherent picture:

- the machine is capable of useful native graphics work despite severe age and
  architectural constraints
- Terakan already has enough implementation to run meaningful Vulkan tests and
  workloads
- the most damaging blockers are now structural and integration-oriented rather
  than “driver does nothing” class failures
- the biggest open performance and correctness wins span shared infrastructure:
  NIR lowering, SFN register pressure/spill behavior, winsys cleanup costs, and
  VLIW scheduling quality

## Working Thesis

The best path is not to optimize one API surface in isolation. The right path
is to:

1. stabilize the workspace and source history
2. unblock the shared compiler/runtime layers
3. preserve conformance wins as regression anchors
4. use measurement-led probe work to separate fixable inefficiency from hard
   hardware ceiling

## Near-Term Execution Order

1. Keep the normalized x130e workspace under `/home/eirikr` as the only
   canonical remote workspace.
2. Continue implementation on named Mesa branches with the private remote as
   `origin`.
3. Vacuum and classify the remote corpus into local tables.
4. Unblock the Mesa 26 build frontier in layers:
   first toolchain seams, then generated/NIR seams, then Terakan-specific code.
5. Re-run smoke, vkmark, dEQP, and Rusticl baselines under manifest-backed
   wrappers.

## Current Active Iteration

The current bring-up loop is now concrete rather than hypothetical:

- build root:
  `/home/eirikr/workspaces/mesa/mesa-26-debug/build-terakan-distcc`
- compiler path:
  `clang-19` through distcc wrappers
- distributed compile pool:
  `DESKTOP-CKP9KB6/32,lzo` plus `127.0.0.1/4`
- persistent execution surface:
  `ssh x130e-tmux` into the remote `steinmarder` tmux session

The first newly observed blocker in this loop was a Rust nightly / bindgen
contract mismatch, not a Terakan compile error. Clearing that seam has already
allowed the build to advance through NIR, Gallium auxiliary code, and into the
mid-stack Rusticl/GLSL area, which is a strong signal that the build root is
healthier than the original stale migrated tree.

## Canonical Inputs

- [`FRONTIER_ROADMAP_R600_TERASCALE.md`](FRONTIER_ROADMAP_R600_TERASCALE.md)
- [`TERAKAN_PERFORMANCE_TRACKER.md`](TERAKAN_PERFORMANCE_TRACKER.md)
- [`MEMORY_NOTES.md`](MEMORY_NOTES.md)
- [`results/x130e_terascale/blessed/20260401_x130e_source_snapshot/findings/COMPREHENSIVE_SCOPE.md`](results/x130e_terascale/blessed/20260401_x130e_source_snapshot/findings/COMPREHENSIVE_SCOPE.md)
- [`results/x130e_terascale/blessed/20260401_x130e_source_snapshot/findings/TERAKAN_PORT_STATUS.md`](results/x130e_terascale/blessed/20260401_x130e_source_snapshot/findings/TERAKAN_PORT_STATUS.md)
- [`results/x130e_terascale/blessed/20260401_x130e_source_snapshot/findings/TERAKAN_CONFORMANCE_PLAN.md`](results/x130e_terascale/blessed/20260401_x130e_source_snapshot/findings/TERAKAN_CONFORMANCE_PLAN.md)
