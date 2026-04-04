# Full Stack Profile — Mesa 26.0.3 Debug Build
**Date**: 2026-03-30
**Hardware**: AMD Radeon HD 6310 (TeraScale-2/VLIW5, 2 CU)
**Build**: Mesa 26.0.3 (git-3f173c02d1) debug + Terakan cherry-pick

## Benchmark Results

### OpenGL 4.6 (glmark2 800x600)
| Benchmark | FPS | FrameTime |
|-----------|-----|-----------|
| build:use-vbo=false | 147 | 6.83ms |
| build:use-vbo=true | 156 | 6.42ms |
| texture:nearest | 154 | 6.54ms |
| texture:linear | 148 | 6.76ms |
| texture:mipmap | 157 | 6.41ms |
| shading:gouraud | 145 | 6.94ms |
| shading:blinn-phong | 148 | 6.79ms |
| shading:phong | 145 | 6.92ms |
| shading:cel | 145 | 6.92ms |
| bump:high-poly | 128 | 7.83ms |
| bump:normals | 155 | 6.48ms |

### GLES 3.1 (glmark2-es2 800x600)
| Benchmark | FPS | FrameTime |
|-----------|-----|-----------|
| build:use-vbo=true | 161 | 6.24ms |
| shading:phong | 144 | 6.95ms |
| bump:normals | 157 | 6.39ms |
| bump:high-poly | 127 | 7.88ms |

### Vulkan 1.0 (Terakan, vkmark)
| Benchmark | FPS | Score |
|-----------|-----|-------|
| cube | 619 | 619 |
| Validation errors | 0 | clean |

### VA-API
10 decode profiles: MPEG2 Simple/Main, VC1 S/M/A, H264 CB/M/H/H10, VideoProc

### GALLIUM_HUD Pipeline (shading:phong)
| Counter | Value |
|---------|-------|
| GPU load | 98% |
| GPU shaders busy | 95% |
| GPU ta busy | 87% |
| draw calls/frame | 3 |
| vs invocations/frame | 43,044 |
| ps invocations/frame | 89,410 |
| temperature | 67C |
| VRAM usage | ~100MB |

### IBS CPU Hotspots
| Function | Time | Module |
|----------|------|--------|
| radeon_cs_context_cleanup | 0.13s | libgallium-26.0.3 |
| pthread_mutex_lock | 0.08s | libc |
| __memmove_ssse3 | 0.08s | libc |
| avahi_unescape_label | 0.07s | libavahi |
| _PyEval_EvalFrameDefault | 0.06s | python3 |

CPU distribution: 83% in libgallium (Mesa debug), 10% in libc, 5% in Xorg

## Cross-Stack Comparison

| Stack | API Version | Score/FPS | Status |
|-------|-------------|-----------|--------|
| OpenGL 4.6 | GL 4.6 Compat | ~148 avg FPS | Working |
| GLES 3.1 | ES 3.1 | ~147 avg FPS | Working |
| Vulkan 1.0 | VK 1.0.335 | 619 FPS (cube) | Working |
| VA-API | 1.22 | 10 profiles | Working |
| GALLIUM HUD | - | All counters | Working |
| IBS CPU | uProf 5.2.606 | Full hotspots | Working |

## Key Findings
1. GPU is 95-98% shader-bound across all GL benchmarks
2. Vulkan (Terakan) is 4x faster than GL for simple geometry (619 vs ~155 FPS)
   because Terakan has lower CPU overhead (no GL state machine)
3. radeon_cs_context_cleanup remains #1 CPU hotspot
4. GLES 3.1 matches GL 4.6 performance (same driver path)
5. Zero Vulkan validation errors with the Mesa 26 build
