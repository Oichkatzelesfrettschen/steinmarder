# Memory — Project

## Mission

Build `src/sass_re` into a durable research-and-engineering control plane for:

- NVIDIA SASS reverse engineering
- cross-track optimization method transfer
- the x130e TeraScale-2 / r600 / Terakan Mesa program

For the x130e lane specifically, the target is:

- honest, native-max Vulkan bring-up
- explicit documentation of hard hardware limits
- shared improvements across Terakan Vulkan, r600 GL/GLES, Rusticl compute,
  Zink gating, and the DRM/DRI/libva support surfaces that matter to those
  stacks

## Current Top-Level Goals

1. Normalize and preserve the x130e workspace under `/home/eirikr`.
2. Convert the detached dirty Mesa 26 worktree into a branch-based source
   project with private remote backing.
3. Vacuum the remote x130e document corpus into structured local tables and
   canonical docs.
4. Unblock the Mesa 26 Terakan build.
5. Expand conformance, performance, and observability in a manifest-backed way.

## Success Criteria

- no critical x130e project knowledge depends on chat history alone
- canonical TeraScale state is readable from local docs and tables
- the x130e Mesa worktree is preserved, migratable, and branchable
- build blockers, capability ceilings, and validation baselines are explicit
- optimization work follows the same evidence standards as the stronger CUDA and
  SM89 tracks

## Non-Goals

- pretending unsupported hardware features are implemented
- flattening every raw imported note into one giant markdown file
- making Zink or DXVK the primary bring-up path
- polluting the source-control layer with bulky raw traces when a manifest and
  curated summary are enough
