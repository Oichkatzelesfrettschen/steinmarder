# steinmarder -- Documentation Index

Organized into four categories: **SASS**, **NeRF**, **Engine**, and **Results**.

---

## [`sass/`](sass/) -- SASS Reverse Engineering & GPU ISA

GPU shader assembly analysis, encoding research, and instant-NGP kernel optimization.

### Core SASS references
| Document | Description |
|----------|-------------|
| [SM89_ARCHITECTURE_REFERENCE](sass/SM89_ARCHITECTURE_REFERENCE.md) | SM 8.9 hardware specs, roofline model, L2 regime analysis |
| [NVIDIA_SASS_ADA_LOVELACE_REFERENCE](sass/NVIDIA_SASS_ADA_LOVELACE_REFERENCE.md) | Full SASS ISA reference for Ada Lovelace |
| [SASS_KERNEL_OPTIMIZATION_GUIDE](sass/SASS_KERNEL_OPTIMIZATION_GUIDE.md) | Kernel optimization patterns and techniques |
| [OPTIX_INTEGRATION_PATTERNS](sass/OPTIX_INTEGRATION_PATTERNS.md) | OptiX integration patterns |
| [NEXT_PHASES](sass/NEXT_PHASES.md) | Planned next phases for SASS RE |

### Cross-track primers
| Document | Description |
|----------|-------------|
| [APPLE_SILICON_RE_GUIDE](sass/APPLE_SILICON_RE_GUIDE.md) | Apple silicon CPU, Metal, and Core ML research plan that mirrors the SASS discipline |
| [STACK_MAP](../src/sass_re/STACK_MAP.md) | Living side-by-side architecture map for Apple, Ryzen, CUDA, SASS, and future track slots |
| [BUILD_DECISION_MATRIX](../src/sass_re/BUILD_DECISION_MATRIX.md) | Repo-wide compiler-vs-manual build decision matrix |
| [CUDA_LBM_README](../src/cuda_lbm/README.md) | CUDA LBM kernel inventory, performance tables, and baseline build decisions |
| [FRONTIER_ROADMAP_APPLE](../src/sass_re/FRONTIER_ROADMAP_APPLE.md) | Apple silicon frontier checklist, tranche status, and checklists |
| [FRONTIER_ROADMAP_RYZEN_5600X3D](../src/sass_re/FRONTIER_ROADMAP_RYZEN_5600X3D.md) | Ryzen 5600X3D frontier checklist and tool-stack mapping |
| [RYZEN_5600X3D_RE_GUIDE](sass/RYZEN_5600X3D_RE_GUIDE.md) | Ryzen 5 5600X3D CPU + 3D V-Cache translation guide |

### Apple track artifacts
- [`../src/apple_re/results/blessed/KEEPALIVE_SUMMARY.md`](../src/apple_re/results/blessed/KEEPALIVE_SUMMARY.md) -- promoted Apple keepalive summary + CUDA-grade evidence bundle
- [`../src/apple_re/results/blessed/2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz`](../src/apple_re/results/blessed/2026-03-30_tranche1_keepalive_cuda_grade_bundle.tar.gz) -- compressed promoted Apple keepalive evidence
- [`../src/sass_re/NEXT42_APPLE_TRANCHE.md`](../src/sass_re/NEXT42_APPLE_TRANCHE.md) -- next 42-step promoted Apple run checklist and artifact map

**See also:** [`src/sass_re/`](../src/sass_re/) for the full toolkit:
- [`src/sass_re/RESULTS.md`](../src/sass_re/RESULTS.md) -- Measured latencies and encoding analysis
- [`src/sass_re/instant_ngp/`](../src/sass_re/instant_ngp/) -- Inline PTX kernels with SASS diffs
- [`src/sass_re/instant_ngp/docs/`](../src/sass_re/instant_ngp/docs/) -- Three-tier documentation
- [`src/apple_re/`](../src/apple_re/) -- Apple silicon CPU, Metal, and Core ML scaffolding
- [`src/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/`](../src/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/) -- Latest promoted Apple keepalive bundle (real fs_usage, PID-scoped leaks/vmmap, normalized density)
- [`src/apple_re/results/blessed/2026-03-30_tranche1_r5_variants_frontier/`](../src/apple_re/results/blessed/2026-03-30_tranche1_r5_variants_frontier/) -- Latest promoted M1 variant frontier bundle (3 Metal variants, row deltas, mnemonic frontier, ranked next steps)
- [`src/apple_re/results/blessed/2026-03-30_tranche1_r4_m1_cuda_grade/`](../src/apple_re/results/blessed/2026-03-30_tranche1_r4_m1_cuda_grade/) -- Full-stack promoted Apple tranche baseline (42/42, trace health + schema exports + diagnostics)
- [`src/sass_re/STACK_MAP.md`](../src/sass_re/STACK_MAP.md) -- living architecture stack map for Apple, Ryzen, CUDA, SASS, and future slots
- [`src/sass_re/FRONTIER_ROADMAP_APPLE.md`](../src/sass_re/FRONTIER_ROADMAP_APPLE.md) -- Apple silicon frontier checklist
- [`src/sass_re/APPLE_SILICON_RE_BRIDGE.md`](../src/sass_re/APPLE_SILICON_RE_BRIDGE.md) -- Method bridge from SASS RE to Apple silicon lanes
- [`src/apple_re/scripts/run_apple_tranche1.sh`](../src/apple_re/scripts/run_apple_tranche1.sh) -- 64-step Apple deep-dive tranche runner
- [`src/apple_re/scripts/run_next42_cpu_suite.sh`](../src/apple_re/scripts/run_next42_cpu_suite.sh) -- next CPU follow-on wrapper
- [`src/apple_re/scripts/run_next42_cpu_probes.sh`](../src/apple_re/scripts/run_next42_cpu_probes.sh) -- next CPU probe draft runner
- [`src/apple_re/scripts/run_next42_cpu_cache_probes.sh`](../src/apple_re/scripts/run_next42_cpu_cache_probes.sh) -- next CPU cache-pressure probe runner
- [`src/apple_re/scripts/run_next42_metal_suite.sh`](../src/apple_re/scripts/run_next42_metal_suite.sh) -- next Metal follow-on wrapper
- [`src/apple_re/scripts/run_next42_metal_probes.sh`](../src/apple_re/scripts/run_next42_metal_probes.sh) -- next Metal probe draft runner
- [`src/apple_re/scripts/run_next42_neural_suite.sh`](../src/apple_re/scripts/run_next42_neural_suite.sh) -- next neural follow-on wrapper
- [`src/sass_re/NEXT42_APPLE_TRANCHE.md`](../src/sass_re/NEXT42_APPLE_TRANCHE.md) -- next 42-step Apple follow-on tranche and analysis plan
- [`src/apple_re/scripts/prime_sudo_cache.sh`](../src/apple_re/scripts/prime_sudo_cache.sh) -- Touch ID/password sudo cache prime for `--sudo cache` runs
- [`src/sass_re/FRONTIER_ROADMAP_RYZEN_5600X3D.md`](../src/sass_re/FRONTIER_ROADMAP_RYZEN_5600X3D.md) -- Ryzen 5600X3D frontier checklist

---

## [`nerf/`](nerf/) -- Neural Radiance Fields

NeRF inference, SIMD implementation, depth conditioning, and neural upscaling.

### Architecture & design
| Document | Description |
|----------|-------------|
| [HYBRID_CPU_GPU_NERF_ARCHITECTURE](nerf/HYBRID_CPU_GPU_NERF_ARCHITECTURE.md) | CPU NeRF + GPU mesh parallel execution design |
| [SM_NEURAL_UPSCALE_ARCHITECTURE](nerf/SM_NEURAL_UPSCALE_ARCHITECTURE.md) | DLSS-class temporal super-resolution architecture |

### SIMD implementation
| Document | Description |
|----------|-------------|
| [NERF_CPU_SIMD_REALTIME](nerf/NERF_CPU_SIMD_REALTIME.md) | AVX2 SIMD real-time NeRF approach |
| [NERF_SIMD_COMPLETE](nerf/NERF_SIMD_COMPLETE.md) | Full SIMD implementation documentation |
| [NERF_SIMD_QUICKREF](nerf/NERF_SIMD_QUICKREF.md) | One-page SIMD cheat sheet |
| [BUILD_NERF_SIMD](nerf/BUILD_NERF_SIMD.md) | Build and integration steps |
| [NERF_INTEGRATION_GUIDE](nerf/NERF_INTEGRATION_GUIDE.md) | How to integrate NeRF into the renderer |

### Depth-conditioned NeRF
| Document | Description |
|----------|-------------|
| [DEPTH_CONDITIONED_NERF](nerf/DEPTH_CONDITIONED_NERF.md) | Depth conditioning overview and 11% speedup results |
| [DEPTH_CONDITIONED_NERF_QA](nerf/DEPTH_CONDITIONED_NERF_QA.md) | Q&A: training, hyperparameters, experimental validation |
| [README_DEPTH_CONDITIONED_NERF](nerf/README_DEPTH_CONDITIONED_NERF.md) | Research project overview |

### GPU optimization
| Document | Description |
|----------|-------------|
| [NERF_GPU_OPTIMIZATIONS](nerf/NERF_GPU_OPTIMIZATIONS.md) | GPU-side NeRF optimizations with benchmarks |
| [NERF_GPU_SHADER_DEBUG_REPORT](nerf/NERF_GPU_SHADER_DEBUG_REPORT.md) | Critical shader bugs found and fixed |

### Data formats & tools
| Document | Description |
|----------|-------------|
| [NERF_CUSTOM_FORMAT](nerf/NERF_CUSTOM_FORMAT.md) | `.smb` binary format specification |
| [README_NERF_TRAIN_EXPORT](nerf/README_NERF_TRAIN_EXPORT.md) | Training and export pipeline |

### Troubleshooting & fixes
| Document | Description |
|----------|-------------|
| [NERF_TROUBLESHOOTING](nerf/NERF_TROUBLESHOOTING.md) | Common NeRF issues and diagnostics |
| [NERF_CPU_FIX_SUMMARY](nerf/NERF_CPU_FIX_SUMMARY.md) | Critical CPU inference bug fixes |
| [NERF_FIX_FLOAT16](nerf/NERF_FIX_FLOAT16.md) | Float16 precision fix |
| [NERF_WALKABLE_COMPLETE](nerf/NERF_WALKABLE_COMPLETE.md) | Walkable NeRF (12.4x speedup, 489 FPS) |
| [NERF_QUICK_REF](nerf/NERF_QUICK_REF.md) | Quick reference |

---

## [`engine/`](engine/) -- Renderer, Denoiser & Pipeline

Core raytracer, denoising, BVH/LBVH, GPU pipeline, and optimization.

### Denoiser
| Document | Description |
|----------|-------------|
| [README_DENOISER](engine/README_DENOISER.md) | Denoiser overview |
| [BILATERAL_DENOISE](engine/BILATERAL_DENOISE.md) | Bilateral filter algorithm and implementation |
| [ADVANCED_DENOISE_IMPLEMENTATION](engine/ADVANCED_DENOISE_IMPLEMENTATION.md) | History reset, immediate denoise, adaptive denoise |

### GPU pipeline & optimization
| Document | Description |
|----------|-------------|
| [GPU_OPTIMIZATION_FINAL_REPORT](engine/GPU_OPTIMIZATION_FINAL_REPORT.md) | Layer-by-layer optimization (8.6% speedup, 50.5->54.85 FPS) |
| [GPU_OPTIMIZATION_RESULTS](engine/GPU_OPTIMIZATION_RESULTS.md) | Baseline metrics and test results |
| [GPU_OPTIMIZATION_QUICK_REF](engine/GPU_OPTIMIZATION_QUICK_REF.md) | Quick reference for optimization techniques |
| [GPU_RENDER_SCALE_2X_BOOST](engine/GPU_RENDER_SCALE_2X_BOOST.md) | 2x render-scale performance measurements |

### BVH / LBVH
| Document | Description |
|----------|-------------|
| [LBVH_QUICK_REFERENCE](engine/LBVH_QUICK_REFERENCE.md) | LBVH algorithm spec (O(n) construction, Morton codes) |
| [LBVH_DISCOVERY_REPORT](engine/LBVH_DISCOVERY_REPORT.md) | LBVH integration analysis with code locations |
| [LBVH_INTEGRATION_SUMMARY](engine/LBVH_INTEGRATION_SUMMARY.md) | Cache coherence benefits |
| [LBVH_INTEGRATION_COMPLETE](engine/LBVH_INTEGRATION_COMPLETE.md) | Implementation completion status |

### Geometry & platform
| Document | Description |
|----------|-------------|
| [QUAD_LOADER_IMPLEMENTATION](engine/QUAD_LOADER_IMPLEMENTATION.md) | Quad-aware OBJ loader |
| [AVX_PORTABILITY](engine/AVX_PORTABILITY.md) | Runtime CPU detection and SIMD portability |

---

## [`results/`](results/) -- Benchmarks & Performance Data

Measured performance data from optimization and testing runs.

| Document | Description |
|----------|-------------|
| [FINAL_FPS_RESULTS](results/FINAL_FPS_RESULTS.md) | Performance breakthrough: 44.8->117.0 FPS (+161%) |
| [FPS_TEST_RESULTS](results/FPS_TEST_RESULTS.md) | Baseline FPS benchmarks (39.5 FPS) |
| [FPS_TEST_RESULTS_ADVANCED](results/FPS_TEST_RESULTS_ADVANCED.md) | Denoise configuration performance impact |
| [TEST_RESULTS_1080P_60FPS](results/TEST_RESULTS_1080P_60FPS.md) | 1080p upsampling frame time measurements |
| [OPTIMIZATION_RESULTS_1080P_60FPS](results/OPTIMIZATION_RESULTS_1080P_60FPS.md) | 1080p optimization results |
