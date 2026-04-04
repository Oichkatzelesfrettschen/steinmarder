# Stutter RCA and Fix — radeon DRM winsys

**Date**: 2026-03-30
**Hardware**: AMD Radeon HD 6310 (TeraScale-2), E-300 APU

## Root Cause Analysis

### Issue: SHORT-LONG alternating frame times (CV% >45%)

Two independent causes identified:

### Cause 1: radeon_cs_context_cleanup (FIXED)
**Location**: src/gallium/winsys/radeon/drm/radeon_drm_cs.c:137
**Problem**: Clears 4096-entry hash table (16KB) every frame using scalar loop
**Fix applied**: Selective hash clear — only reset entries actually used (~3-10 per frame)
**Result**: Mean frametime 6.8ms -> 3.3ms (2.1x), glxgears 408 -> 450 FPS (+10%)

### Cause 2: Double-buffer CS sync stall (DOCUMENTED FOR FUTURE)
**Location**: src/gallium/winsys/radeon/drm/radeon_drm_cs.c:648
**Problem**: radeon_drm_cs_sync_flush() blocks CPU waiting for previous frame DRM ioctl.
With only 2 CS buffers (csc1/csc2), CPU must wait for GPU to finish frame N before
starting frame N+2. Creates SHORT-LONG alternation pattern.

**Attempted fix**: Triple-buffer CS contexts (csc1/csc2/csc3)
**Status**: Prototype built but regressed due to shared flush_completed fence.
Proper fix needs per-context fence tracking.

**Future fix path (kernel DRM level)**:
- The radeon DRM kernel driver could support async command stream completion
  notification instead of requiring userspace to poll/wait
- Alternatively, the winsys could use 3 CS contexts with independent
  util_queue_fence per context instead of sharing one flush_completed fence
- This is a linux kernel DRM + Mesa winsys co-design issue

## Benchmark Summary

| Config | glxgears (no vsync) | Mean frametime | P95 | Frames <4ms |
|--------|--------------------:|---------------:|----:|------------:|
| Original Mesa 26 | 408 FPS | 6.77ms | 14.4ms | 20.7% |
| + Selective hash clear | **450 FPS** | **3.26ms** | **6.58ms** | **77.8%** |
| + Triple-buffer (reverted) | 439 FPS | 3.23ms | 6.67ms | 78.0% |

## Files Modified
- src/gallium/winsys/radeon/drm/radeon_drm_cs.c (selective hash clear)
- src/gallium/winsys/radeon/drm/radeon_drm_cs.h (hash slot tracking fields)
