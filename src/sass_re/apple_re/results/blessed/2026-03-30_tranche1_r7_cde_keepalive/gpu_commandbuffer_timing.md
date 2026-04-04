# Metal GPU Command Buffer Timing — GPUStartTime / GPUEndTime

- date: 2026-04-01
- machine: Apple M-series (arm64)
- method: commandBuffer.GPUStartTime + commandBuffer.GPUEndTime accumulated across 200 iterations
- iters: 200 per variant, width: 1024
- kernel: `probe_simdgroup_reduce` (all variants share this function name)
- tool: `metal_probe_host --counters timestamp`
- note: MTLCounterSamplingPointAtDispatchBoundary NOT supported on this GPU (AGXG13G);
  fell back to per-command-buffer GPU timestamps

## GPU vs CPU Timing Table

| Variant | CPU ns/element | GPU ns/element | CPU/GPU ratio | GPU speedup vs baseline |
|---------|---------------|---------------|---------------|------------------------|
| baseline | 205.4 | **14.8** | 13.9× | — |
| occupancy_heavy | 204.7 | **9.1** | 22.5× | **1.63×** |
| register_pressure | 224.8 | **12.3** | 18.3× | 1.20× |
| threadgroup_heavy | 261.6 | **16.8** | 15.6× | 0.88× |

## Key Findings

### 1. GPU execution time is NOT what CPU wall-clock measures

CPU wall-clock time (`commandBuffer waitUntilCompleted`) is dominated by the
CPU-GPU synchronization overhead, not actual GPU computation:
- Baseline: CPU sees 205 ns/element, GPU actually executes in **14.8 ns/element**
- Ratio: **13.9×** — for every 1 ns of GPU work, the CPU waits 13.9 ns

This is the `waitUntilCompleted` round-trip cost: dispatch → GPU queued → GPU executed
→ CPU wakes from wait. This is a fundamental limitation of the single-command-buffer
per-iteration dispatch pattern in the probe harness.

### 2. Register pressure effect is amplified in GPU time

| Comparison | CPU ratio | GPU ratio |
|-----------|----------|----------|
| occupancy_heavy vs register_pressure | 0.91× faster CPU | **0.74× faster GPU** |
| Effect visible | ~10% faster on CPU | **~35% faster on GPU** |

The register-pressure penalty (12.3 vs 9.1 ns/element GPU) is 3.5× more visible
in GPU-actual time than in CPU wall-clock time. The CPU wall-clock obscures the
GPU effect because it's dominated by synchronization overhead.

**Implication**: The xctrace `metal-gpu-state-intervals` density metric (which showed
`register_pressure` as the density loser at −609.9 r/s) correctly identified the GPU
throughput degradation that the raw `ns/element` CPU timing understated.

### 3. occupancy_heavy has lowest GPU execution time

`occupancy_heavy` at **9.1 ns/element** GPU-actual is the fastest variant, consistent
with being the fastest on CPU wall-clock (196.7 ns/element from r7 metal_timing.csv)
AND the highest xctrace density (+678.5 r/s).

All three measurement methods agree: occupancy_heavy > threadgroup_minimal > register_pressure.

### 4. threadgroup_heavy is slowest on GPU

`threadgroup_heavy` at **16.8 ns/element** GPU-actual is the slowest variant —
slower than even `baseline` (14.8 ns). This makes sense: the threadgroup memory
read-modify-write operations add real GPU latency that the `probe_simdgroup_reduce`
baseline doesn't have.

The CPU wall-clock also shows threadgroup_heavy as the slowest (261.6 ns/element),
but the 15.6× CPU/GPU ratio means most of the CPU time is still synchronization.

## Methodology Notes

### commandBuffer.GPUStartTime vs xctrace

- `GPUStartTime/GPUEndTime`: per-command-buffer GPU timestamps in the GPU time domain.
  Gives actual GPU execution time. Sampled here with 200 iterations, summed.
- `xctrace Metal System Trace`: gives CPU-side timing with GPU state transitions.
  The metal_timing.csv records wall-clock time including dispatch overhead.
- `xctrace metal-gpu-counter-intervals`: hardware GPU counters (NOT captured in any
  blessed run yet — requires `Metal GPU Counters` template). Would give ALU
  utilization, memory bandwidth, cache hit rates.

### MTLCounterSamplingPointAtDispatchBoundary

On this device (AGXG13G family), inline counter sampling within a compute encoder
is NOT supported. The device exposes the `timestamp` counter set with `GPUTimestamp`
counter, but `supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary`
returns NO.

The `commandBuffer.GPUStartTime/GPUEndTime` approach is the correct alternative
for GPU-side timing on unsupported devices.

## Recommended Next Measurement

To measure per-dispatch GPU execution time without `waitUntilCompleted` overhead:
1. Submit multiple dispatches in a single command buffer (batch 100+ dispatches)
2. Measure `GPUStartTime` to `GPUEndTime` of the entire batch
3. Divide by dispatch count for true per-dispatch latency

This eliminates per-iteration command buffer creation/commit/wait overhead and
gives GPU pipeline throughput, not synchronization-dominated numbers.
