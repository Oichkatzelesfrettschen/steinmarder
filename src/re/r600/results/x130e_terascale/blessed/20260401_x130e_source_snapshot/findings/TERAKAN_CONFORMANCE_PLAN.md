# Terakan Vulkan & Rusticl Conformance Plan

## Current Status (2026-03-31, post-fixes)

### dEQP-VK Results — unsupported_image_usage suite

| Metric | Before Fix | After Fix |
|--------|-----------|-----------|
| Pass | 3,049 | 4,899+ |
| Fail | 459 | **0** |
| Not Supported | 3,555 | 3,925 |
| Pass rate | 86.9% | **100%** (0 failures) |

**Fix applied**: Added Vulkan spec-required usage-vs-features validation in
`terakan_format.c:GetPhysicalDeviceImageFormatProperties2`. Validates SAMPLED,
STORAGE, COLOR_ATTACHMENT, DEPTH_STENCIL, TRANSFER_SRC, TRANSFER_DST, and
INPUT_ATTACHMENT usage bits against format feature flags. Modeled after RADV.

### Rusticl (OpenCL) Results

| Metric | Before Fix | After Fix |
|--------|-----------|-----------|
| Pass | 36/48 (75%) | **37/48 (77%)** |
| Fail | 11 | 11 |
| Crash | 1 | **0** |

**Crash fix applied**: Added NULL guard for `n==0` and `pool->bo==NULL` in
`evergreen_set_global_binding()`. Prevents segfault in binary program creation.

**Scratch threshold**: Lowered from 64 to 16 bytes for compute shaders only.
Graphics shaders keep the original 64-byte threshold (no perf regression).

**RA graceful failure**: Changed `assert(0)` to `return nullptr` on GPR overflow
in SFN register allocation. Shaders that exceed 123 GPRs now fail gracefully
instead of crashing the entire driver.

### Remaining Failures

#### Rusticl (11 failures)

| # | Category | Test | Root Cause | Fix Path |
|---|----------|------|------------|----------|
| 1 | Upstream | cl-api-build-program | Rusticl wrong error code | Mesa Rusticl patch |
| 2 | Upstream | cl-api-compile-program | Same | Same |
| 3 | Upstream | cl-api-link-program | Same | Same |
| 4 | Upstream | cl-api-get-extension-function-address | Missing function | Same |
| 5 | HW limit | cl-api-get-device-info | 16 samplers (hw max) | Cannot fix |
| 6 | HW limit | cl-api-get-image-info | Related image caps | Cannot fix |
| 7 | HW limit | cl-api-get-kernel-arg-info | Embedded profile | Cannot fix |
| 8 | r600 compute | cl-custom-run-simple-kernel | global_binding API mismatch | Major: rewrite global binding |
| 9 | r600 compute | cl-custom-flush-after-enqueue | Same root cause | Same |
| 10 | r600 compute | cl-custom-buffer-flags | Same root cause | Same |
| 11 | r600 compute | cl-custom-use-sub-buffer | Same root cause | Same |

**Root cause #8-11**: r600 `evergreen_set_global_binding` expects `r600_resource_global`
objects with chunk-based memory pool (`compute_memory_pool`), but Rusticl passes plain
`pipe_resource` objects. This is a fundamental API impedance mismatch between Rusticl
Gallium usage and the legacy r600 OpenCL compute path.

**Fix strategy**: Adapt `evergreen_set_global_binding` to handle both `r600_resource_global`
(legacy clover path) and plain `pipe_resource` (Rusticl path). Check `resource->bind &
PIPE_BIND_GLOBAL` and branch accordingly. For Rusticl resources, use the resource BO
directly instead of the compute_memory_pool.

## dEQP-VK Remaining Work

### Phase 1: Run broader test suites (Next)
- `dEQP-VK.api.smoke.*` — 6/6 pass (done)
- `dEQP-VK.api.info.format_properties.*` — 187/225 pass (done)
- `dEQP-VK.api.info.unsupported_image_usage.*` — **0 failures** (done)
- `dEQP-VK.renderpass.*` — NOT YET RUN
- `dEQP-VK.pipeline.*` — NOT YET RUN
- `dEQP-VK.spirv_assembly.*` — NOT YET RUN
- `dEQP-VK.draw.*` — 1 test, crashed (sparse feature)

### Phase 2: Known crash fixes
- Sparse image queries → add early-return guard
- VkEvent creation → implement stub
- Command pool reuse → debug CS teardown

### Phase 3: GPR Spill Support (SFN)
Full infrastructure exists:
- `ScratchIOInstr` / `LoadFromScratch` — already in SFN
- `sfn_assembler.cpp` — emits `CF_OP_MEM_SCRATCH` and `FETCH_OP_READ_SCRATCH`
- `evergreen_setup_scratch_buffers()` — programs HW scratch ring
- **New**: Compute dispatch path now calls `evergreen_setup_scratch_buffers()`

Missing: Spill candidate selection + instruction insertion in RA failure path.

## Files Changed

| File | Change |
|------|--------|
| `src/amd/terascale/vulkan/terakan_format.c` | Usage-vs-features validation (kills 459 dEQP failures) |
| `src/gallium/drivers/r600/evergreen_compute.c` | NULL guard + scratch setup in compute dispatch |
| `src/gallium/drivers/r600/sfn/sfn_nir.cpp` | Compute-only scratch threshold, graceful RA failure |

## Benchmark Verification (post-fixes)
- glxgears: **367 FPS** (no regression, was 337 — variance from CPU load)
- vkmark cube: 1215 FPS (unchanged)
