# TeraScale Capability Ledger

This ledger accompanies the
[`tables/table_x130e_capability_matrix.csv`](tables/table_x130e_capability_matrix.csv).

## Principles

- target native-max capability honestly
- separate “implemented,” “blocked by current driver limitations,” and “blocked by
  hardware architecture”
- keep unsupported cases distinct from true failures

## Current Snapshot

### Working

- OpenGL 4.6 via r600
- GLES 3.1 via r600
- Vulkan device enumeration via Terakan (`vulkaninfo --summary`)
- VA-API profile exposure
- Rusticl device enumeration
- Observability tooling:
  - GALLIUM_HUD
  - radeontop
  - perf/uProf
  - trace-oriented observability tooling
- imported OpenGL fragment and shader_runner probe corpus, plus seeded Vulkan
  compute / OpenCL / OpenGL compute probe lanes

### Partially Working

- Vulkan dEQP coverage beyond smoke and selected API info suites
- Rusticl compute beyond simple kernels
- swapchain/WSI validation in all configurations
- `vkmark --list-scenes` on the Terakan ICD
- OpenCL 1.2 device exposure via rusticl on PALM (TeraScale-based Lenovo ThinkPad X130e, codename "PALM")
- root-only perf capture for Vulkan/OpenCL observability via cached `sudo -A`
- OpenCL probe harness execution across default, `-Os`, and `-cl-opt-disable`
  profiles

- Vulkan compute pipeline creation beyond the new graceful guard path:
  - Terakan now exposes a minimal `vk_device_shader_ops` bridge to stop the old common-runtime crash.
  - Still lacks the real `vk_shader` compile wrapper.
  - Serialization/deserialization is not yet implemented.
  - Pipeline-cache integration is needed for common compute pipelines.
  Terakan now exposes a minimal `vk_device_shader_ops` bridge to stop the old
  common-runtime crash. However, it still lacks the real `vk_shader` compile wrapper,
  serialization/deserialization, and pipeline-cache integration needed for
  common compute pipelines.
- Rusticl global binding compatibility for several failing tests
- SFN spill path for high-pressure compute kernels
- OpenCL numeric stability for scalar and packed probe lanes

### Known Hard or Likely Hard Limits

- no geometry/tessellation class support on TeraScale 1 and TeraScale 2 hardware generations[^1]
- old kernel and driver capabilities around syncobj/DRI3 impose practical
  limits on some Zink paths
- OpenCL reports no FP64 support on this PALM device

## Immediate Companion Tables

- [`tables/table_x130e_capability_matrix.csv`](tables/table_x130e_capability_matrix.csv)
- [`tables/table_x130e_build_blockers.csv`](tables/table_x130e_build_blockers.csv)
- [`tables/table_x130e_terascale_findings_matrix.csv`](tables/table_x130e_terascale_findings_matrix.csv)

[^1]: These architectures do not provide the necessary hardware functionality for geometry or tessellation class support.
