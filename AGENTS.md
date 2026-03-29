# steinmarder -- Agent and Developer Reference

## Overview

steinmarder is a C11 rendering and physics engine with several optional GPU and
research subsystems:

- CPU path tracer with BVH, adaptive sampling, denoising, and postprocess
- CPU NeRF inference with SIMD paths and batching
- Vulkan compute rendering and physics demos
- CUDA D3Q19 Lattice Boltzmann Method (LBM) kernels
- SASS reverse engineering and inline PTX optimization tooling
- Physics modules for reactor thermal, fission, and quantum volume demos
- raylib-based mesh editing tools

The repo is intentionally modular. A plain C11 + pthreads build covers the core
renderer. Vulkan, CUDA, raylib, OpenMP, and ONNX-dependent targets are added
only when the local toolchain supports them.

## High-Value Entry Points

| Path | Why it matters |
|------|----------------|
| `README.md` | Current project-wide feature summary, build notes, and run commands |
| `CMakeLists.txt` | Source of truth for optional targets, tests, and dependency gating |
| `src/render/render.c` | Main CPU rendering pipeline |
| `src/render/sm_main.c` | Environment-variable configuration and executable entry point |
| `src/cuda_lbm/README.md` | LBM kernel index, performance tables, and benchmark conventions |
| `src/cuda_lbm/CMakeLists.txt` | CUDA LBM targets and compiled kernel source list |
| `src/sass_re/RESULTS.md` | Measured Ada Lovelace SASS latency and throughput data |
| `docs/sass/SM89_ARCHITECTURE_REFERENCE.md` | Roofline and hardware-specific optimization context |

## Build Matrix

### Always-on core targets

Built with a C11 compiler, pthreads, and CMake:

- `steinmarder`
- `nerf_simd_test`
- `inspect_ppm`
- `smb_info`
- `smb_to_ppm`
- `test_render_smoke`

### Optional targets

- Vulkan found:
  `gpu_demo`, `quantum_demo`, `test_reactor_thermal`
- raylib found:
  `sm_mesh_edit`, `sm_viewport`, `sm_edit_mode`
- CUDA Toolkit found:
  `src/cuda_lbm/` is added and builds `lbm_bench` and `lbm_test`
- OpenMP found:
  enabled for NeRF batch processing

### Canonical build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Notes:

- Release defaults to `-O2 -march=native` and enables IPO/LTO when supported.
- Debug defaults to `-g -O0 -fsanitize=address,undefined`.
- The runtime output directory is `build/bin/`.

## Testing and Verification

CTest is enabled at the top level:

```sh
cd build
ctest --output-on-failure
```

Important tests:

- `render_smoke` always exists and checks for NaN/Inf/zero-output regressions
- `test_reactor_thermal` exists when Vulkan is available
- `lbm_conservation` and `lbm_benchmark_quick` exist when CUDA is available

Direct binaries:

```sh
build/bin/test_render_smoke
build/bin/test_reactor_thermal
build/bin/lbm_test
```

## CUDA LBM Reference Architecture

The main GPU physics subsystem currently builds from 33 CUDA source files and
ships 39 kernel variants across 12 precision tiers plus sparse, tensor-core,
and analysis utilities.

Primary references:

| Document | Purpose |
|----------|---------|
| `src/cuda_lbm/README.md` | Kernel inventory, measured performance tables, layout guidance |
| `src/cuda_lbm/CMakeLists.txt` | Exact compiled CUDA sources for `lbm_bench` and `lbm_test` |
| `src/cuda_lbm/include/lbm_kernels.h` | Kernel variant registry |
| `src/cuda_lbm/include/lbm_metrics.h` | Benchmark result structs and GPU specs |
| `docs/sass/SM89_ARCHITECTURE_REFERENCE.md` | Ada-specific performance model |
| `docs/sass/NVIDIA_SASS_ADA_LOVELACE_REFERENCE.md` | ISA reference |

Current measured highlights from the checked-in docs:

- All scalar FP32-and-below LBM production kernels are bandwidth-bound on Ada
- INT8 SoA remains the top MLUPS tier at about 5460 MLUPS on 128^3
- FP8 e4m3 SoA follows at about 5170 MLUPS
- Q32.32 fixed-point is faster than FP64 while preserving zero mass drift
- SoA pull-scheme is the preferred production layout; AoS mainly remains as a
  comparison baseline
- Precomputed `inv_tau` removes the reciprocal stall and improves stability

Canonical commands:

```sh
build/bin/lbm_bench --all --grid 128 --output table
build/bin/lbm_bench --kernel int8_soa --grid 64,128,256
build/bin/lbm_bench --validate
build/bin/lbm_bench --regression
build/bin/lbm_bench --occupancy
build/bin/lbm_test
```

## SASS Reverse Engineering Toolkit

The SASS RE subsystem is a first-party measurement and optimization workflow
for NVIDIA Ada Lovelace, with probe kernels, microbenchmarks, cubin/SASS
analysis, and inline-PTX case studies under `src/sass_re/`.

Key references:

| Path | Purpose |
|------|---------|
| `src/sass_re/RESULTS.md` | Latency and throughput measurements |
| `src/sass_re/microbench/` | Microbench sources |
| `src/sass_re/probes/` | Probe kernels |
| `src/sass_re/instant_ngp/` | Inline PTX rewrites and reproduction scripts |

Representative measured values called out in repo docs:

- `FFMA`: 4.54 cycles
- `IADD3`: 2.51 cycles
- `MUFU.RCP`: 41.55 cycles
- `LDG`: 92.29 cycles
- `LDS`: 28.03 cycles

## CPU Renderer and NeRF Notes

Useful realities when editing the non-CUDA engine:

- The core renderer lives in `src/render/` and links through `sm_render`
- `src/render/postprocess.c` is compiled into `sm_core` on purpose because
  `image.c` depends on it before `sm_render` is linked
- NeRF sources are in `src/nerf/`; AVX2/FMA are enabled only when the compiler
  supports them, and runtime dispatch still handles feature detection
- Denoise code is in `src/denoise/`; upscale code is in `src/upscale/`

Common run commands:

```sh
SM_W=320 SM_H=180 SM_SPP=4 ./build/bin/steinmarder
SM_W=1920 SM_H=1080 SM_SPP=128 SM_ADAPTIVE=1 SM_NEURAL_DENOISE=1 ./build/bin/steinmarder
```

The renderer is configured through environment variables rather than CLI flags.
Start with `src/render/sm_main.c` when adding or changing runtime controls.

## Project Structure

```text
src/
  core/        math, rays, camera, primitives, image, material, color
  render/      CPU renderer, BVH, scene loading, g-buffer, postprocess
  denoise/     bilateral and neural denoising
  nerf/        NeRF SIMD inference and scheduling
  vulkan/      Vulkan compute demos and GPU acceleration pieces
  physics/     reactor, fission, quantum volume, thermal simulation
  editor/      raylib-based mesh editing tools
  cuda_lbm/    CUDA LBM kernels, benchmarks, tests, sparse variants
  sass_re/     SASS probes, microbenchmarks, runners, paper assets
  upscale/     image upscaling pipeline
  tools/       small standalone utilities and smoke tests
  third_party/ stb_image_write
docs/          engine, nerf, sass, and result documentation
scripts/       helper scripts
```

## Working Guidance

When making changes:

1. Check `CMakeLists.txt` first so you update the right target and dependency
   assumptions.
2. Preserve optional-build behavior. Do not make Vulkan, CUDA, raylib, ONNX, or
   OpenMP mandatory unless the user explicitly wants that.
3. For CUDA LBM work, benchmark or validate with the checked-in commands above.
4. For renderer changes, prefer `test_render_smoke` or `ctest` before finishing.
5. If documentation and code disagree, trust the code and refresh the docs.
