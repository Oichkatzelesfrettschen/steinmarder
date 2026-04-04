# Hardware Cache/Latency/Cycle Boundary Measurements
**Date**: 2026-03-30
**Conditions**: Under CPU load (dEQP + Rusticl building simultaneously)
**GPU**: AMD Radeon HD 6310 (487MHz shader, 533MHz memory, 68C sustained)

## Critical Finding: GPU is 62-75% IDLE

Under CPU load, the GPU pipeline shows only 25-38% utilization. The GPU is
waiting for the CPU to submit work 62-75% of the time. This confirms that
CPU command submission overhead is THE bottleneck, not GPU compute power.

## Resolution Scaling (Fill Rate)

- 100x100: 273 FPS (2.7 Mpix/s) -- CPU-bound
- 200x200: 279 FPS (11.2 Mpix/s) -- CPU-bound
- 400x400: 209 FPS (33.4 Mpix/s) -- Transition zone
- 800x600: 235 FPS (113 Mpix/s) -- GPU fill-rate bound

Fill rate saturates at ~400x400. Below that, CPU overhead dominates.

## Vulkan vs GL (Same Geometry)

- GL (glmark2 build): 258 FPS
- VK (vkmark cube): 509 FPS -- 2.0x FASTER

Pure driver overhead reduction. No GL state machine = 2x throughput.

## Texture Filtering: ZERO COST

- nearest: 251 FPS
- linear: 250 FPS
- mipmap: 254 FPS

All three modes identical. The TeraScale-2 texture cache handles everything.

## Buffer Updates: THE WORST CASE

- map: 31 FPS
- subdata: 33 FPS

CPU touching GPU buffers every frame is catastrophic. This is where
GPU-only rendering would help most.

## Pipeline Utilization Under Load

- GPU load: 25-38%
- Shaders busy: 23-36%
- TA busy: 20-33%
- SC busy: 23-36%
- CB/DB busy: 20-31%
- VGT/PA: 9-22%
- SDMA: 0%
- Scratch RAM: 0% (no register spills)

## Memory: Not Saturated

- VRAM: 65MB / 384MB (17%)
- Bytes moved: ~2MB/frame
- Evictions: 0
- Buffer wait: 0-14ns
- Temperature: 68C stable

## Console-Style Optimization Path

1. Minimize CPU-GPU sync (triple-buffer CS, async submit)
2. Persistent mapped buffers (avoid map/unmap per frame)
3. Vulkan over GL (2x from CPU overhead reduction alone)
4. Batch draw calls (currently 3/frame)
5. GPU-driven rendering (compute shaders building cmd lists)
6. Pre-compiled shader cache (avoid runtime compilation)
