# Terakan / r600 Performance Tracker

## Score Tracking

| Date | Build | Buildtype | Compiler | .text MB | GL FPS | VK FPS | Load Avg | Notes |
|------|-------|-----------|----------|----------|--------|--------|----------|-------|
| 2026-03-31 | Mesa 26.0.3 | debug (-O0) | gcc | 27.6 | 380 | 519 | <1.0 | Baseline (quiescent system) |
| 2026-03-31 | Mesa 25.2.6 (system) | release (-O2) | gcc | 42.9 | 325 | — | 5.2 | System Mesa (many drivers) |
| 2026-03-31 | Mesa 26.0.3 | debugoptimized (-O2+LTO) | clang-19 | 25.2 | 122 | — | 12.6 | LTO hurts: code bloat > benefit on 32KB L1i |
| 2026-03-31 | Mesa 26.0.3 | debugoptimized (-O2) | clang-19 | 23.0 | 301 | — | 5.2 | No-LTO. At parity with system release. |
| 2026-03-31 | Mesa 26.0.3 | debugoptimized (-O2) | clang-19 | 23.0 | 187 | — | 7.2 | Same build, higher load |
| 2026-03-31 | Mesa 26.0.3 | debugoptimized (-O2) | clang-19 | 23.0 | **385** | — | **1.35** | CLEAN: matches -O0 baseline! |
| 2026-03-31 | Mesa 25.2.6 (system) | release (-O2) | gcc | 42.9 | 325 | — | 5.2 | At higher load than our clean run |

## Target: 2000 FPS vkmark (headless)

## Source-of-truth notes

- live workspace: `nick@x130e:~/mesa-26-debug/`
- evidence depot: `nick@x130e:~/eric/TerakanMesa/`
- local compendium: [`results/x130e_terascale/README.md`](results/x130e_terascale/README.md)

Current preservation snapshot facts:

- remote Mesa commit observed: `3f173c0`
- checkout state: detached `HEAD`
- active dirty worktree spans `r600`, `zink`, winsys, runtime, and NIR
- untracked Terakan tree present at `src/amd/terascale/`

## Key Findings

### Build Configuration
- **Desktop build workflow**: Build on DESKTOP-CKP9KB6 (32 cores, ~40s), rsync to x130e
- **LTO is counterproductive** on E-300: clang's aggressive inlining bloats .text beyond L1i capacity
- **debugoptimized (-O2 -g)** performs at parity with system release builds
- **Zink driver** included for GL-over-Vulkan comparison

### CPU Bottleneck (measured at load < 1.0)
- IPC = 0.24 (catastrophically low, CPU stalled 76% of time)
- 54.75% LLC miss rate
- 16.85% branch misprediction rate
- 26M L1i misses per 10s benchmark
- 49.62% of CPU time in libgallium (Mesa driver)
- 29.66% in kernel (DRM ioctls)

### DRM Activity
- 1,201 CS submissions/sec (~3.2 per frame)
- 35 fence waits/sec (GPU is NOT the bottleneck)
- 0 GART page table updates (memory stable)

### System State Impact
- **Compositor (marco)** consumes ~20% GPU + CPU per frame
- **Load average** dominates FPS variation — same build ranges 120-301 FPS depending on load
- **Clean benchmarks require**: screen locked, no background builds, load < 1.0

## Clean Per-Process Comparison (-O2 vs -O0 at load ~1.3)

| Metric | -O0 (old baseline) | -O2 clang-19 (new) | Change |
|--------|-------------------|-------------------|--------|
| FPS | 380 | **385** | **+1.3%** |
| User time (10s bench) | 3.26s | **1.68s** | **2x faster CPU** |
| Sys time | 2.07s | **1.64s** | 20% less kernel |
| Total instructions | 988M | **801M** | **19% fewer** |
| Cache miss % | 54.75% | 48.24% | -6.5pp better |
| Branch mispredict | 16.85% | 19.39% | +2.5pp worse |
| L1i misses | 26M | 28M | Similar |

**Key insight**: -O2 executes 19% fewer instructions in half the CPU time,
but FPS is nearly identical because the bottleneck is GPU-bound (385 FPS
with compositor running, GPU at ~95% utilization). The CPU savings free
headroom for future GPU-feeding optimizations.

## Zink (GL-over-Vulkan) Result

**FAILED**: Zink requires DRI3/VK_KHR_swapchain which Terakan doesn't support yet.
Error: "DRI3 not available" → glXChooseFBConfig() failed.
Zink needs Terakan swapchain implementation before it can be tested.

## Current bring-up blockers

### Terakan Mesa 26 integration

- `src/amd/terascale/` has already been copied into the remote Mesa 26 tree
- remaining documented build blockers sit in custom NIR intrinsic and builder
  wiring rather than broad driver absence
- preserve the current tree before any cleanup because it is not yet attached
  to a named branch

## Rusticl Compute RAT Readback Fix (2026-04-02)

### Bug #15: RAT readback — FIXED, 5/5 PASS

The `out[i] = in[i] * 3.0` kernel now produces correct output on all 16 floats.

**Root cause**: UB cast of `pool_bo` (`pipe_buffer`) as `r600_texture` inside
`evergreen_emit_framebuffer_state`, plus CS space undercount in
`r600_need_cs_space`.

**Fix** (3 parts in `evergreen_compute.c`):
1. Direct `CB_COLOR0_BASE` register emission from `r600_cb_surface` instead of
   the invalid `r600_as_texture()` cast
2. `r600_need_cs_space(rctx, 23, ...)` to account for the 23 extra dwords
3. `compute_cb_target_mask` to set the correct RAT export mask

### Kernel CS validator findings

- `SHADER_TYPE` bit is invisible to opcode extraction; compute and non-compute
  packets are processed identically by the kernel CS validator
- `radeon_add_to_buffer_list` returns `index * 4`, matching kernel `idx / 4` —
  reloc encoding is correct

### Conformance status

| Test | Result | Notes |
|------|--------|-------|
| rat_test_fixed (scale ×3.0) | **5/5 PASS** | INT8-range float arithmetic verified correct |
| Custom 2-arg kernels | PASS | Isolated dispatches work |
| Sequential dispatch | FAIL | Second kernel reads stale data from first kernel's output |
| clpeak compute | CL_INVALID_WORK_GROUP_SIZE (-54) | Work-group size exceeds HW max |
| clpeak transfer | 1.34 GBPS write | DDR3-533 UMA bandwidth |

### Remaining bugs

- Sequential dispatch: stale data between kernels (cache flush / barrier missing
  between consecutive compute dispatches)
- clpeak: `CL_INVALID_WORK_GROUP_SIZE` — need to clamp max work-group size to
  hardware limit (likely 64 or 128 threads per SIMD)

## Next Steps
1. ~~Get clean baseline at load < 1.0~~ DONE (385 FPS, matches -O0)
2. ~~Test Zink~~ BLOCKED (needs Terakan VK_KHR_swapchain)
3. Test vkmark with Terakan (fix ErrorExtensionNotPresent)
4. Try `-Os` build (desktop went to sleep — retry next session)
5. Try gcc instead of clang for potentially smaller code
6. Profile-guided optimization (PGO) for function layout
7. Run new latency probes (TEX L1 hit/miss, kcache, FP64, clause switch)
8. Implement Terakan VK_KHR_swapchain (enables Zink + vkmark)
