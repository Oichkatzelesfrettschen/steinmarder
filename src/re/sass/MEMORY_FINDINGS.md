# Memory — Findings

## Current High-Confidence x130e Facts

- Mesa 26 debug worktree on x130e is preserved at snapshot commit
  `3f173c02d169ccc38c88fea9d58ce46ee1cf8d55`
- the canonical remote Mesa workspace now lives under
  `/home/eirikr/workspaces/mesa/mesa-26-debug`
- the unified implementation branch is now `main`, with the preserved branch
  kept at `snapshot/2026-04-01-source-preserved`
- Terakan source already exists under `src/amd/terascale/` in the remote Mesa
  tree
- the earlier Mesa 26 NIR builder drift and `nir_uav_instr_r600` helper
  mismatch have already been landed into the unified main branch
- Vulkan conformance has a meaningful foothold:
  - unsupported image usage failures reduced to zero
  - smoke suite green
- Rusticl is no longer crashing in the previously known global-binding path, but
  compute remains blocked by API impedance and missing spill support
- the current Mesa 26 distcc build root is
  `/home/eirikr/workspaces/mesa/mesa-26-debug/build-terakan-distcc`
- the first fresh-build blocker on LMDE 7 was not Terakan code itself, but a
  Rust nightly / `bindgen 0.71.1` incompatibility:
  Meson passed `--rust-target 1.96.0-nightly`, and `bindgen` only accepts the
  numeric semver form
- that bindgen seam is currently routed around with a user-local wrapper at
  `/home/eirikr/.local/bin/bindgen`, keeping the Mesa tree itself clean while
  the now-unified main branch continues toward the next runtime frontier
- the canonical Mesa 26 distcc build now completes successfully at
  `/home/eirikr/workspaces/mesa/mesa-26-debug/build-terakan-distcc`
- `vulkaninfo --summary` succeeds against the Terakan ICD and enumerates
  `AMD R8xx Palm (Terakan)` with `apiVersion 1.1.335`
- `vkmark --list-scenes` succeeds, but `vkmark --list-devices` still fails
  because `VK_KHR_display` is not exposed by the ICD
- forwarded X11 presentation still blocks on DRI3:
  the current xlib presentation path reports
  `MESA: info: vulkan: No DRI3 support detected - required for presentation`
- the first completed probe tranche confirms:
  - a local probe corpus of 58 inventoried probe files
  - 37 imported/local OpenGL fragment probes
  - 13 shader_runner probes
  - seeded Vulkan compute, OpenCL, and OpenGL compute kernels
- the first completed trace bundle confirms:
  - `perf`, `trace-cmd`, `radeontop`, `bpftrace`, `AMDuProfCLI`, `vmstat`,
    and `iostat` are installed on x130e
  - `perf stat` is currently blocked by `perf_event_paranoid=2`
  - render-node ACLs for `eirikr` are in place on `/dev/dri/renderD128` and
    `/dev/dri/card0`
- the root perf tranche now works under cached `sudo -A` and captures real
  root-owned `perf stat`, `perf record`, and report artifacts with
  `perf_event_paranoid=-1`
- the Vulkan compute probe host is now past the earlier common-runtime crash:
  a minimal Terakan `vk_device_shader_ops` bridge and guard path are in place,
  so targeted compute probes reach `vkCreateComputePipelines` and fail cleanly
  with `VK_ERROR_FEATURE_NOT_PRESENT (-8)` instead of segfaulting
- the current explicit Vulkan boundary is now:
  Terakan still needs a real `vk_shader` compile wrapper around
  `terakan_shader_impl_compile`, plus shader serialization/deserialization and
  cache-stable pipeline integration
- the OpenCL probe host is now operational and profile-capable:
  - default, `-Os` host build, and `-cl-opt-disable` sweeps all complete
  - depchain and vec4 probes run on Rusticl PALM with timings
  - scalar and packed lanes still show severe numeric pathologies, including
    huge checksums and high `nan_count`, so the next frontier is classifying
    overflow, packing, and lowering behavior rather than harness setup
- the generated typed-integer and occupancy families now exist locally for both
  OpenCL and Vulkan compute:
  `u8/i8/u16/i16/u32/i32/u64/i64`, `int24/uint24`, and
  `occupancy_gpr_{04,08,16,24,32,48,64,96}`
- manual OpenCL runs already show useful architectural spread:
  - `depchain_u16_add`: build `70.34 ms`, kernel `15.63 ms`, `nan_count=1075`
  - `depchain_u64_add`: build `7.43 ms`, kernel `24.97 ms`, `nan_count=1653`
  - `occupancy_gpr_32`: build `128.22 ms`, kernel `37.76 ms`, `nan_count=766`
  - `occupancy_gpr_64`: build `195.68 ms`, kernel `92.03 ms`, `nan_count=272`
- `occupancy_gpr_64` compiles to a much larger ISA body (`880` dwords) and
  `48 gprs`, which is the first strong local evidence that the register-stuffing
  ladder is actually surfacing the VLIW/GPR pressure frontier we wanted
- winsys / submission overhead is material:
  `radeon_cs_context_cleanup` is still the top recorded CPU hotspot in the
  preserved full-stack profile

## Fastest Moving Risks

1. The current host-local `bindgen` wrapper is a practical workaround, but it
   is not yet normalized into a durable “local prerequisite vs in-tree fix”
   decision.
2. The real active frontier has shifted from compile to runtime:
   `VK_KHR_display`, DRI3/X11 presentation, the missing full Terakan
   common-compute shader bridge, Rusticl global bindings, and SFN spill support
   now matter more than the earlier NIR helper seam.
3. The probe lane is now executable and partially diagnostic, but not yet
   trustworthy enough for strong arithmetic claims:
   Vulkan compute now fails cleanly instead of crashing, and OpenCL numerics are
   still unstable enough that we need packed/int/uint-focused sweeps before
   drawing architectural conclusions.

## Best Existing Anchors

- [`tables/table_x130e_terascale_findings_matrix.csv`](tables/table_x130e_terascale_findings_matrix.csv)
- [`results/x130e_terascale/blessed/20260401_x130e_source_snapshot/SUMMARY.md`](results/x130e_terascale/blessed/20260401_x130e_source_snapshot/SUMMARY.md)
- [`MEMORY_NOTES.md`](MEMORY_NOTES.md)
