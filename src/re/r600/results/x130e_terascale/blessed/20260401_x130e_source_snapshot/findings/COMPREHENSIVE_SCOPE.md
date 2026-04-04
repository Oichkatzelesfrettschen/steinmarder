# Comprehensive Graphics Stack Scope - AMD HD 6310 (TeraScale-2)

**Date**: 2026-03-29
**Mesa**: 26.0.3 debug (git-3f173c02d1) + Terakan Vulkan ICD
**Hardware**: AMD Radeon HD 6310 (Wrestler, VLIW5, 2 CU, 32-bit ALU)

---

## 1. Stack Status Summary

| Stack | Status | Score/FPS | Key Issue |
|-------|--------|-----------|-----------|
| **OpenGL 4.6** | Working | glmark2: 144 | 68% VLIW5 utilization |
| **GLES 3.1** | Working | glmark2-es2: 153 | Slightly better than GL (lighter state) |
| **Vulkan 1.0** | Working (Terakan) | vkmark cube: 612 FPS | Semaphore validation errors |
| **OpenCL 1.2** | BROKEN (system Mesa) | clpeak: CRASH | u2u8 lowering missing in stock Mesa |
| **VA-API** | Working | 10 decode profiles | MPEG2, VC1, H264 decode |
| **VDPAU** | Working | Video surfaces OK | Decode via G3DVL |
| **WebGL** | Untested | Needs Firefox profiling | Depends on GL performance |

---

## 2. Critical Bug: Rusticl 8-bit ALU Lowering

### Problem
clpeak crashes with:
```
Unknown instruction '8     %5 = u2u8 %4.x'
R600: Unsupported instruction: 8     %5 = u2u8 %4.x
EE r600_sfn.cpp:95 r600_shader_from_nir - translation from NIR failed !
```

### Root Cause
The r600 hardware only has 32-bit ALU. When clpeak generates an OpenCL kernel
using 8-bit integers (`compute_char`), the NIR compiler emits `u2u8` (unsigned
truncation to 8-bit) which the r600 NIR backend cannot translate.

The custom `r600rat0vtx1` Mesa build had a NIR lowering pass that converts
8/16-bit operations to 32-bit equivalents. This patch is NOT in stock Mesa 26.0.3.

### Resolution
Port the 8/16-bit lowering patch from the r600rat0vtx1 build to Mesa 26.0.3.
The patch goes in `src/gallium/drivers/r600/r600_sfn.cpp` or as a NIR pass in
`src/gallium/drivers/r600/r600_pipe.c` (the `nir_shader_compiler_options` struct).

---

## 3. Terakan Vulkan Driver Status

### Working
- Device enumeration: AMD R8xx Palm (Terakan) 0x9802
- VK_KHR_swapchain with X11 WSI
- vkcube: renders correctly
- vkmark cube: 612 FPS (1.634ms frametime)
- Dynamic rendering, timeline semaphores, depth bias control

### Issues Found
- **Semaphore validation errors** (from earlier session):
  `vkAcquireNextImageKHR(): Semaphore must not have any pending operations`
  This was seen with validation layers but current run shows 0 validation messages
  with vkcube --c 60. May be fixed in the current Terakan build or only triggers
  under certain swapchain configurations.

- **Non-conformant**: "WARNING: terakan is not a conformant Vulkan implementation"
- **No geometry/tessellation shaders** (hardware limitation)
- **No multiDrawIndirect** (hardware limitation)
- **Device UUID all zeros** - should be derived from PCI bus info

### Terakan Build Source
The Terakan ICD (`libvulkan_terascale.so`, 57MB) appears to be pre-built.
Source location: `~/eric/TerakanMesa/vulkan/terakan-vulkan-icd.deb`
Need to locate the Terakan source tree to integrate with Mesa 26 build.

---

## 4. Tool Inventory

### Installed and Working
| Tool | Version | Purpose |
|------|---------|---------|
| apitrace | system | GL/GLES API trace capture |
| glretrace | system | GL trace replay + benchmark |
| eglretrace | system | EGL trace replay |
| radeontop | system | GPU block utilization monitor |
| AMDuProfCLI | 5.2.606 | IBS fetch/op CPU profiling |
| perf | 6.12.74 | Linux perf counters |
| gdb | system | Debugging with debug Mesa symbols |
| valgrind | system | Memory analysis |
| strace/ltrace | system | Syscall/library tracing |
| glmark2 | system | GL 4.6 benchmark |
| glmark2-es2 | system | GLES 2.0/3.1 benchmark |
| vkmark | system | Vulkan benchmark |
| vkcube | system | Vulkan smoke test |
| vulkaninfo | system | Vulkan capability query |
| clinfo | system | OpenCL device info |
| clpeak | system | OpenCL performance benchmark |
| vainfo | system | VA-API capability query |
| vdpauinfo | system | VDPAU capability query |
| glslang-tools | system | GLSL/SPIR-V compiler |
| spirv-tools | system | SPIR-V disassembler/validator |
| validation layers | system | Vulkan validation |

### Missing (would be useful)
| Tool | Purpose | Install |
|------|---------|---------|
| umr | AMD GPU register/wave debugger | Build from source (amd only) |
| RenderDoc | GPU frame capture/debug | Not practical on r600 |
| Chromium | WebGL comparison browser | Installed but untested |
| mesa-overlay | Vulkan overlay HUD | Already available via VK_LAYER_MESA_overlay |

---

## 5. GALLIUM_HUD Counters Available

The debug Mesa build exposes these GPU pipeline counters:

### Pipeline Block Utilization
- GPU-load, GPU-shaders-busy, GPU-ta-busy, GPU-gds-busy
- GPU-vgt-busy, GPU-ia-busy, GPU-sx-busy, GPU-wd-busy
- GPU-bci-busy, GPU-sc-busy, GPU-pa-busy, GPU-db-busy
- GPU-cp-busy, GPU-cb-busy, GPU-sdma-busy, GPU-pfp-busy
- GPU-meq-busy, GPU-me-busy, GPU-surf-sync-busy
- GPU-cp-dma-busy, GPU-scratch-ram-busy

### Shader Statistics
- num-compilations, num-shaders-created, num-shader-cache-hits
- num-vs-flushes, num-ps-flushes, num-cs-flushes
- spill-draw-calls, spill-compute-calls

### Draw/Dispatch Counters
- draw-calls, compute-calls, dma-calls, cp-dma-calls
- MRT-draw-calls, prim-restart-calls, decompress-calls

### Pipeline Stage Invocations
- vs-invocations, gs-invocations, ps-invocations
- hs-invocations, ds-invocations, cs-invocations
- ia-vertices, ia-primitives, primitives-generated
- clipper-invocations, clipper-primitives-generated

### Memory
- VRAM-usage, VRAM-vis-usage, GTT-usage
- requested-VRAM, requested-GTT, mapped-VRAM, mapped-GTT
- buffer-wait-time, num-mapped-buffers, num-bytes-moved, num-evictions

### System
- fps, frametime, cpu, cpu0, cpu1
- temperature, shader-clock, memory-clock
- cpufreq-min/cur/max per core

---

## 6. ISA Analysis Summary (from earlier sessions)

### VLIW5 Utilization
- **68% average** across glmark2 benchmarks
- **Zero 5-slot bundles** achieved
- t-slot (transcendental) used in 75-90% of bundles
- Key bottleneck: normalize() chains serialize RSQ on t-slot

### Instruction Mix
- ALU: 38% of total instructions (MUL_IEEE dominates)
- VTX: 58% (vertex fetch)
- TEX: 4% (texture sample)

### Optimization Opportunities
1. Better VLIW5 scheduling (compiler-level, 10-25% ALU reduction possible)
2. normalize() batching (separate DOT3 -> RSQ -> MUL phases)
3. MOV elimination (copy propagation before scheduling)
4. TEX clause batching for blur/convolution shaders

---

## 7. IBS CPU Profiling Status

AMD uProf 5.2.606 is installed with IBS support:
- IBS Fetch: YES (instruction fetch address sampling)
- IBS OP: YES (instruction retire sampling, includes load/store latency)
- Core PMC: NO (not available on Bobcat Family 14h)

The IBS profile in the comprehensive run was attempted but produced minimal
output. Need to run with sudo for full IBS access:
```
sudo AMDuProfCLI collect --ibs-fetch --ibs-op -o /path/to/output -- <command>
```

---

## 8. Next Steps

### Immediate (can do now)
1. Fix Rusticl 8-bit lowering in Mesa 26 debug build
2. Run IBS profiling with sudo for Mesa compiler hotspots
3. Run GALLIUM_HUD CSV capture with all counters (fix the redirect issue)
4. WebGL benchmarking in Firefox with debug Mesa
5. Full Vulkan validation pass on Terakan

### Short-term
6. Locate Terakan source and integrate into Mesa 26 build
7. Patch VLIW5 scheduler for better slot packing
8. Build Mesa 26 with Rusticl enabled (needs SPIRV-Tools)
9. Compare GL vs GLES performance systematically

### Medium-term
10. Upstream the 8/16-bit lowering patch to Mesa
11. Improve Terakan Vulkan: fix Device UUID, test more extensions
12. WebGL performance optimization loop
13. Software GPU-only rendering paths (bypass CPU for compositing)

---

## 9. Files and Locations

```
~/eric/TerakanMesa/
  findings/
    ISA_SLOT_ANALYSIS.md                    -- VLIW5 slot analysis (661 lines)
    STANDARDIZED_ISA_DECOMPOSITION.txt      -- Cross-session comparison
    COMPREHENSIVE_SCOPE.md                  -- THIS DOCUMENT
    analyze_deep.py                         -- Python ISA parser
    analyze_vliw5_slots.sh                  -- Shell VLIW5 counter
  re-toolkit/
    r600-session.sh                         -- Quick RE session (ISA + trace)
    r600-full-session.sh                    -- 6-layer comprehensive session
    r600-profile-all.sh                     -- All-stack profiling
    r600-shader-dump.sh                     -- ISA extraction
    r600-monitor.sh                         -- GPU utilization
    r600-gltrace.sh                         -- apitrace wrapper
    r600-hud.sh                             -- GALLIUM_HUD
    r600-uprof-ibs.sh                       -- IBS CPU profiling
    run-debug-mesa.sh                       -- Debug Mesa wrapper
    data/
      session_mesa26_debug_*/               -- Mesa 26 RE sessions
      profile_mesa26_comprehensive_*/       -- All-stack profiles
  isa-docs/
    AMD_Evergreen-Family_ISA.pdf            -- Evergreen ISA reference
    R600_Instruction_Set_Architecture.pdf   -- R600/R700 ISA
    R600-R700-Evergreen_Assembly_Language_Format.pdf
    LLVM_R600_Backend_Stellard_2013.pdf
  vulkan/
    terakan-vulkan-icd.deb                  -- Terakan Vulkan driver package
    VP_VULKANINFO_*.json                    -- Vulkan profile

~/mesa-26-debug/                            -- Mesa 26.0.3 source + debug build
/usr/local/mesa-debug/                      -- Installed debug Mesa
```

---

*Generated 2026-03-29*

---

## APPENDIX: Profiling Results (2026-03-29)

### A. IBS CPU Hotspot Analysis

Profiled glmark2 (phong + bump:normals) for 10 seconds with AMD uProf IBS.

Top CPU hotspots in Mesa debug build:

| Rank | Function | Time (s) | Module | Analysis |
|------|----------|----------|--------|----------|
| 1 | radeon_cs_context_cleanup | 0.36 | libgallium-26.0.3 | CS buffer teardown after every submit |
| 2 | r600_update_derived_state | 0.11 | libgallium-26.0.3 | Per-draw state validation |
| 3 | __memmove_ssse3 | 0.10 | libc | Buffer copies (BO uploads) |
| 4 | pthread_mutex_lock | 0.09 | libc | Lock contention in winsys |

CPU time distribution: 81% in libgallium (Mesa), 11% in Xorg, 10% in libc.

Key finding: radeon_cs_context_cleanup is the single biggest CPU-side
bottleneck. This function frees command stream buffers after GPU submission.
Potential optimization: batch CS cleanup, use a free-list pool instead of
per-submit allocation/deallocation.

### B. GALLIUM_HUD Pipeline Utilization

Per-frame GPU block utilization during glmark2 (phong lighting):

| Block | Utilization | Interpretation |
|-------|-------------|----------------|
| GPU load | 98% | Fully GPU-bound |
| Shaders busy | 96% | ALU/shader cores saturated |
| TA (texture address) | 88% | Texture unit near saturation |
| SC (scan converter) | 95% | Rasterizer busy |
| DB (depth buffer) | 96% | Depth testing active |
| CB (color buffer) | 95% | Color writes busy |
| VGT (vertex grouper) | 7% | NOT geometry-bound (expected) |

Per-frame counters:
- Draw calls: 3/frame (very efficient batching)
- VS invocations: 21,516/frame
- PS invocations: 57,165/frame (2.7x more fragments than vertices)
- Primitives generated: 7,172/frame
- Temperature: 63-67C
- VRAM: ~95MB

### C. Terakan Vulkan Validation

vkcube with VK_LAYER_KHRONOS_validation: 0 errors (improved from earlier)
vkmark cube: 612 FPS with Terakan ICD

### D. Rusticl/OpenCL Bug

clpeak fails on 8-bit integer kernels:
- Error: u2u8 instruction not supported in r600 NIR backend
- Fix: Port 8/16-bit lowering patch from r600rat0vtx1 custom build
- Location: src/gallium/drivers/r600/r600_sfn.cpp line 95

### E. VA-API Decode Support

10 profiles available:
- MPEG2 Simple/Main (VLD)
- VC1 Simple/Main/Advanced (VLD)
- H264 Constrained Baseline/Main/High/High10 (VLD)
- VideoProc (post-processing)

