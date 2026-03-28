# Copilot / AI agent instructions for steinmarder

This repo is a C-based raytracer, GPU compute engine, mesh editor, and physics simulator. The goal of these notes is to help an AI coding agent become productive quickly with minimal hand-holding.

## Repository layout

```
src/
  core/       -- math (vec2-4, ray, camera), primitives (sphere, triangle), image, material, color
  render/     -- CPU renderer, BVH, scene loader, G-buffer, postprocess, sm_main.c entrypoint
  denoise/    -- bilateral, neural, ONNX denoiser modules
  nerf/       -- NeRF SIMD inference, hash grid, batch scheduler, depth hints
  vulkan/     -- GPU Vulkan pipelines, LBVH builder, GPU BVH, OBJ loader
  physics/    -- quantum volume raymarcher, nuclear fission/fusion, reactor thermal sim
  editor/     -- mesh editor, viewport, edit mode, OBJ exporter (requires raylib)
  upscale/    -- neural upscaling pipeline
  tools/      -- standalone CLI utilities (inspect_ppm, smb_info, smb_to_ppm)
  third_party/-- stb_image_write.h and other vendored headers
  sass_re/    -- SASS reverse engineering: probes, microbench, instant-NGP kernels
  cuda_lbm/   -- D3Q19 precision-tier CUDA LBM kernels
shaders/      -- GLSL compute shaders + compiled .spv files
docs/         -- all markdown documentation and notes
scripts/
  build/      -- build scripts (.bat, .ps1)
  test/       -- test and benchmark scripts
  analysis/   -- Python analysis, training, and data scripts
build/        -- compiled artifacts (gitignored)
```

- **Big picture:** The primary executable is a CPU raytracer. Rendering is driven from `src/render/sm_main.c` which prepares a `Camera` and calls the renderer in `src/render/render.c` (single-threaded `render_scene_st` or thread-pool `render_scene_mt`). Geometry/acceleration is implemented across `src/core/sphere.c`, `src/core/triangle.c`, and `src/render/bvh.c`. Postprocessing / denoising hooks live in `src/denoise/`.

- **Key files to read first:**
  - **Renderer & threading:** `src/render/render.c`
  - **CLI / entrypoint:** `src/render/sm_main.c`
  - **Math & ray types:** `src/core/vec3.c`, `src/core/ray.c`, `src/core/camera.c`
  - **Primitives & BVH:** `src/core/sphere.c`, `src/core/triangle.c`, `src/render/bvh.c`
  - **Image / output:** `src/core/image.c`
  - **Editor / viewport:** `src/editor/sm_mesh_edit.c`, `src/editor/sm_viewport.c`
  - **GPU Vulkan pipeline:** `src/vulkan/gpu_vulkan_demo.c`
  - **Physics sims:** `src/physics/quantum_volume.c`, `src/physics/reactor_thermal.c`

- **Build system (CMake):**
  ```bash
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  cmake --build .
  ```
  CMake auto-detects Vulkan, raylib, OpenMP, AVX2. Missing deps just skip those targets.

- **Runtime configuration (important):** The program is controlled exclusively via environment variables (see `sm_main.c` and `render.c`). Useful variables:
  - `SM_W`, `SM_H` : image width / height
  - `SM_SPP` : samples per pixel
  - `SM_DEPTH` : max bounce depth
  - `SM_THREADS` : thread count (0 = autodetect)
  - `SM_TILE` : tile size for MT renderer
  - Adaptive sampling toggles in `render.c`: `SM_ADAPTIVE`, `SM_SPP_MIN`, `SM_SPP_BATCH`, `SM_REL_ERR`, `SM_ABS_ERR`
  - `SM_NEURAL_DENOISE` and `SM_DUMP_RGB` control postprocessing and optional dumps

- **Include convention:** All `#include` directives use flat names (e.g., `#include "vec3.h"`). The CMake build adds `-I` flags for every `src/` subdirectory so headers resolve without path prefixes. **Do not** add subdirectory prefixes to includes.

- **Project-specific conventions & gotchas:**
  - Prefix convention: many modules and globals use `sm_` (eg. `sm_neural_denoise_maybe`, `sm_rng_next01`). Follow that naming when adding helpers.
  - Deterministic RNG seeding is implemented per-tile / per-pixel in `render.c`. Changing seeding affects reproducibility tests.
  - The MT renderer uses a persistent thread pool and per-thread `WorkerLocal` structs aligned to 64 bytes to avoid false sharing -- be conservative when changing that struct layout.
  - Adaptive sampling statistics are collected with `_Atomic` counters in `render.c`; preserve atomic semantics if refactoring concurrency.
  - The mesh editor (`sm_mesh_edit.c`) is a standalone, single-file immediate-mode UI using `raylib`/`raymath` with its own fixed limits (`MAX_VERTS`, `MAX_TRIS`). Keep edits to that file consistent with those limits.

- **Integration & external deps:**
  - There are optional/experimental denoiser modules: `src/denoise/neural_denoise.c`, `src/denoise/onnx_denoise.c` -- these may require external libraries (ONNX runtime or prebuilt shaders). If modifying, search these files first to see required headers.
  - `shaders/` contains compiled SPV files -- watch for GPU/compute integration when touching denoising code.

- **Where to make common changes:**
  - Change default render config: `src/render/sm_main.c`
  - Modify shading / material logic: `src/core/material.c` and `ray_color_internal` stub in `src/render/render.c`
  - Change topology/editing logic: `src/editor/sm_mesh_edit.c` and `src/editor/sm_mesh_topology.c`
  - Add new Vulkan compute passes: `src/vulkan/`
  - Add new physics simulations: `src/physics/`

- **Examples to copy/paste when running locally:**
  - Fast debug (low samples, single-thread):
    - Linux / WSL: `SM_W=320 SM_H=180 SM_SPP=4 SM_THREADS=0 ./build/bin/steinmarder`
  - Enable adaptive sampling and denoise:
    - `SM_ADAPTIVE=1 SM_NEURAL_DENOISE=1 ./build/bin/steinmarder`

- **If you change public APIs:** prefer to add `sm_`-prefixed wrappers and keep global state changes minimal -- many modules expect plain C-style structs (no allocation wrappers) and rely on plain arrays.
