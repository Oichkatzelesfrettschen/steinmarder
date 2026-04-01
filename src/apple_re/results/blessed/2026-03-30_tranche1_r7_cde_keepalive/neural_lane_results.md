# Neural Lane Probe Results

- date: 2026-04-01
- machine: Apple M1, 16 GB
- torch: 2.7.0
- mlx: 0.31.1
- coremltools: 9.0
- jax: 0.4.38 (METAL experimental backend)

## PyTorch CPU vs MPS — Matrix Multiply

| Matrix size | CPU ms | MPS ms | MPS speedup | Interpretation |
|-------------|--------|--------|-------------|----------------|
| 64×64 | 0.007 | 0.124 | **0.06×** (CPU wins) | MPS dispatch overhead dominates |
| 256×256 | 0.066 | 0.169 | **0.39×** (CPU wins) | Still dispatch-bound |
| 512×512 | 0.511 | 0.633 | **0.81×** (CPU wins) | Near crossover |
| 1024×1024 | 3.293 | 2.553 | **1.29×** (MPS wins) | MPS faster |
| 2048×2048 | 20.648 | 11.509 | **1.79×** (MPS wins) | MPS clearly faster |

**MPS crossover point: ~1024×1024**. Below this size, CPU (using Apple AMX/BLAS
via Accelerate) is faster than MPS due to GPU dispatch overhead.

For reference, MLX matmul 512×512 = 1.91 ms (3× slower than PyTorch MPS at same size).
MLX uses a different scheduling model (lazy evaluation with explicit `mx.eval()`).

## PyTorch CPU vs MPS — Vector Ops (1M elements)

| Operation | CPU ms | MPS ms | MPS speedup | Interpretation |
|-----------|--------|--------|-------------|----------------|
| Element-wise add 1M f32 | 0.213 | 3.141 | **0.07×** (CPU wins) | MPS overhead catastrophic for small ops |
| Reduction sum 1M f32 | 0.089 | 0.135 | **0.66×** (CPU wins) | Reduction partially amortized |

**Key finding**: MPS is NOT suitable for small vector operations. The GPU dispatch
overhead (kernel launch, memory transfer setup, sync) costs ~3 ms, which is ~15×
the CPU compute time for element-wise ops on 1M elements.

CPU uses optimized BLAS/SIMD paths (AMX, SSE) that beat GPU dispatch for small workloads.

## PyTorch MPS dtype — f32 vs f16 (512×512 matmul)

| dtype | MPS ms | vs f32 |
|-------|--------|--------|
| f32 | 0.519 | reference |
| f16 | 4.405 | **8.5× SLOWER** — anomalous |

The f16 result is anomalously slow — f16 matmul taking 8× longer than f32 is unexpected.
Possible causes:
1. Shader compilation on first f16 dispatch (JIT compilation included in timing)
2. PyTorch MPS f16 not routing through the hardware matrix unit (AMX-GPU / ANE path)
3. dtype conversion overhead (f32 inputs converted to f16 before dispatch)

**Not yet validated**: This result needs more warmup iterations at larger sizes to
separate compilation latency from actual execution latency. Flagged for re-measurement.

## Core ML Tools — Compute Unit Availability

Available compute units on this device:
| ComputeUnit | Description |
|-------------|-------------|
| `ALL` | CPU + GPU + ANE (Neural Engine) — framework chooses optimal placement |
| `CPU_AND_GPU` | CPU + Metal GPU (no ANE) |
| `CPU_ONLY` | CPU only (BLAS, SIMD) |
| `CPU_AND_NE` | CPU + Apple Neural Engine (no Metal GPU) |

`ComputeUnit.ALL` is the recommended default — allows Core ML to use ANE for
supported layers, which is typically 10-30× more energy-efficient than GPU for
transformer/conv inference.

**Not yet measured**: Core ML model timing per compute unit. The placement sweep
(running the same model on each ComputeUnit and comparing timing) is the next step
for Task #31.

## JAX METAL Backend

JAX 0.4.38 has experimental METAL (Apple GPU) support. Device string: `METAL:0`.
The backend is NOT production-ready (prints `Platform 'METAL' is experimental`).

| Status | Detail |
|--------|--------|
| JAX device | METAL:0 |
| Matmul | Not tested (backend experimental) |
| Recommendation | Do not use JAX METAL for production; use MLX for Apple GPU workloads |

## MLX vs PyTorch MPS vs CPU Summary

| Framework | 512×512 matmul | Backend | Notes |
|-----------|---------------|---------|-------|
| PyTorch CPU | **0.51 ms** | Accelerate (AMX) | Fastest for this size |
| PyTorch MPS | 0.63 ms | Metal compute | 1.24× slower than CPU |
| MLX | 1.91 ms | Metal (custom) | 3.8× slower than CPU at this size |
| JAX METAL | n/a | experimental | Not benchmarked |

For 512×512, CPU wins. MPS wins at ≥1024×1024. MLX's lazy evaluation model may
trade off per-operation latency for better throughput in larger computation graphs.

## Implications for Build Decisions

| Question | Answer |
|----------|--------|
| Use MPS for large matmul (≥1024)? | **Yes** — 1.3–1.8× speedup confirmed |
| Use MPS for small ops (<512)? | **No** — GPU dispatch overhead dominates |
| Use MPS for element-wise ops? | **No** — CPU is 15× faster for 1M elements |
| Core ML ANE available? | **Yes** — CPU_AND_NE compute unit confirmed |
| MLX available and functional? | **Yes** — v0.31.1, GPU default device |
| f16 speedup on MPS? | **Unclear** — anomalous 8× slowdown; needs re-measurement |
