# Metal GPU Hardware Counter Analysis — occupancy_heavy variant

- date: 2026-04-01
- machine: Apple M1, 16 GB, macOS 15 (Sequoia / arm64e) — AGXG13G
- method: `xcrun xctrace record --instrument 'Metal GPU Counters' --time-limit 20s`
- variant: `occupancy_heavy` (3 float accumulators + mixed ops, 1024 threads/dispatch)
- iters: 5,000,000 kernel dispatches (GPU active for full 20s recording window)
- n_samples: 60,703 per counter (sampled every ~330µs of wall time, 20µs GPU windows)

## Key Finding: M1 GPU is ALU-balanced, not ALU-bound

This is the first hardware GPU counter capture for the steinmarder Apple track.
All prior blessed runs (r5–r7) captured zero hardware counter rows because they
used the `Metal Application` xctrace template. This run uses `Metal GPU Counters`.

## Critical Findings

### 1. Peak ALU Utilization: 78.94% — balanced, not ALU-limited

The `occupancy_heavy` compute shader reaches 79% ALU utilization at peak.
P99 (99th percentile, sustained load) is 34.41%. Average across the 20s window
(including dispatch overhead gaps) is only 1.59%, confirming what the r7 timing
showed: GPU executes 14–22× faster than the host can schedule the next dispatch.

The GPU is NOT ALU-bound for this shader — 79% peak ALU with remaining capacity
means the shader could sustain more work before hitting the ALU ceiling.

### 2. Fragment Occupancy = 100% for compute kernels on M1 TBDR

Fragment Occupancy hits 100% (P99: 90.11%) for a compute kernel. This is NOT
fragment shader activity — the M1 TBDR (Tile-Based Deferred Rendering) GPU routes
compute work through the fragment pipeline hardware. On Apple GPU, 'Fragment Occupancy'
is effectively the GPU compute tile occupancy metric.

Compute Occupancy (counter 24) shows 0% at P99 and 98% max — confirming it tracks
a different, less-reported aspect. Fragment Occupancy is the metric to watch.

### 3. Memory bandwidth: 45% read, 54% write at peak

Peak GPU Read Bandwidth: 44.68% (P99: 28.07%)
Peak GPU Write Bandwidth: 53.74% (P99: 23.96%)

Neither read nor write bandwidth is saturated. The shader is not memory-bound.

### 4. LLC shows moderate activity: 38.78% peak

LLC (Last Level Cache) Utilization peak 38.78%, P99 24.63%. The occupancy_heavy
shader does access the LLC, but not intensively. This is consistent with the
threadgroup-based design that keeps most working data on-chip.

### 5. Dispatch overhead dominates: average ALU utilization 1.59%

The average ALU utilization over the 20-second window is only 1.59%, despite
bursts to 79%. This quantifies the host-side dispatch overhead: even with 5M
kernel dispatches, the GPU spends most of its time idle between dispatches.
This is the 14–22× factor seen in commandBuffer timing (r7, gpu_commandbuffer_timing.md).

## Counter Table (occupancy_heavy, M1 AGXG13G, 60,703 samples)

| Counter | Samples | Min% | P50% | P90% | P99% | Max% | Avg% |
|---------|---------|------|------|------|------|------|------|
| ALU Utilization | 60703 | 0.00 | 0.00 | 2.39 | 34.41 | 78.94 | 1.59 |
| Buffer Load Utilization | 60703 | 0.00 | 0.00 | 0.23 | 2.21 | 8.96 | 0.10 |
| Buffer Read Limiter | 60703 | 0.00 | 0.00 | 0.23 | 2.22 | 8.96 | 0.10 |
| Buffer Store Utilization | 60703 | 0.00 | 0.00 | 0.00 | 0.00 | 0.39 | 0.00 |
| Buffer Write Limiter | 60703 | 0.00 | 0.00 | 0.00 | 0.00 | 0.39 | 0.00 |
| Compute Occupancy | 60703 | 0.00 | 0.00 | 0.00 | 0.00 | 98.34 | 0.03 |
| F16 Utilization | 60703 | 0.00 | 0.00 | 0.00 | 8.75 | 24.79 | 0.35 |
| F32 Utilization | 60703 | 0.00 | 0.00 | 1.26 | 6.77 | 38.44 | 0.46 |
| Fragment Interp Limiter | 60703 | 0.00 | 0.00 | 0.50 | 11.48 | 24.40 | 0.51 |
| Fragment Interp Utilization | 60703 | 0.00 | 0.00 | 0.18 | 6.65 | 17.81 | 0.29 |
| Fragment Occupancy | 60703 | 0.00 | 0.00 | 2.39 | 90.11 | 100.00 | 3.93 |
| GPU Read Bandwidth | 60703 | 0.00 | 0.00 | 0.55 | 28.07 | 44.68 | 1.05 |
| GPU Write Bandwidth | 60703 | 0.00 | 0.02 | 0.43 | 23.96 | 53.74 | 0.79 |
| LLC Limiter | 60703 | 0.00 | 0.01 | 1.07 | 44.30 | 52.90 | 1.61 |
| LLC Utilization | 60703 | 0.00 | 0.01 | 1.05 | 24.63 | 38.78 | 1.15 |
| MMU Limiter | 60703 | 0.00 | 0.01 | 0.70 | 30.11 | 47.01 | 1.18 |
| MMU Utilization | 60703 | 0.00 | 0.01 | 0.69 | 23.88 | 37.63 | 0.96 |
| Partial Renders | 60703 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| TG/IB Load Limiter | 60703 | 0.00 | 0.00 | 0.04 | 5.54 | 18.15 | 0.20 |
| TG/IB Load Utilization | 60703 | 0.00 | 0.00 | 0.04 | 4.45 | 11.73 | 0.17 |
| TG/IB Store Limiter | 60703 | 0.00 | 0.00 | 0.04 | 5.54 | 18.15 | 0.20 |
| TG/IB Store Utilization | 60703 | 0.00 | 0.00 | 0.09 | 12.63 | 19.11 | 0.47 |
| Texture Cache Limiter | 60703 | 0.00 | 0.00 | 0.31 | 27.25 | 99.56 | 1.11 |
| Texture Filtering Limiter | 60703 | 0.00 | 0.00 | 1.16 | 24.43 | 41.59 | 1.05 |
| Texture Sample Limiter | 60703 | 0.00 | 0.00 | 1.12 | 40.85 | 99.07 | 1.65 |
| Texture Sample Utilization | 60703 | 0.00 | 0.00 | 1.12 | 24.26 | 41.77 | 1.05 |
| Texture Write Limiter | 60703 | 0.00 | 0.00 | 0.90 | 48.67 | 100.00 | 1.99 |
| Texture Write Utilization | 60703 | 0.00 | 0.00 | 0.90 | 34.57 | 81.70 | 1.62 |
| Top Performance Limiter | 60700 | 0.00 | 0.01 | 4.58 | 67.27 | 100.00 | 3.05 |
| Total Occupancy | 14941 | 0.00 | 1.92 | 72.53 | 94.26 | 100.08 | 16.15 |
| Vertex Occupancy | 60703 | 0.00 | 0.00 | 0.02 | 0.63 | 4.02 | 0.02 |

## Build Decision Implications

| Question | Answer |
|----------|--------|
| Is `occupancy_heavy` ALU-bound? | **No** — peak ALU 79%, sustained 34% (P99). Room to add more ALU work. |
| Is it memory-bound? | **No** — peak read 45%, write 54%. Memory bus not saturated. |
| Why is average utilization 1.6%? | **Host dispatch overhead** — GPU executes 14–22× faster than host scheduling. |
| What limits throughput most? | **Host-side dispatch latency** (waitUntilCompleted + serial launch). Not GPU compute capacity. |
| Why does 'Fragment Occupancy' hit 100%? | **M1 TBDR architecture** — compute work routes through tile fragment pipeline. This is the real compute occupancy metric. |
| Can we add more FP32 work to `occupancy_heavy`? | **Yes** — F32 peak is 38.44%, ALU peak is 79%. Budget for more FP32 FMAs exists. |
| Is the `register_pressure` penalty visible in counters? | **Need second capture** — this run only covers `occupancy_heavy`. |

## What Was Missing in Prior Runs

All r5–r7 and CDE blessed runs used `xcrun xctrace record --template 'Metal Application'`
or `xcrun xctrace record --template 'Metal System Trace'`, neither of which populates
`metal-gpu-counter-intervals` with hardware data. Those runs produced 0 counter rows.

This run uses `--instrument 'Metal GPU Counters'` (without a base template), which
enables the Apple GPU hardware performance monitoring unit and populates the counter table.

## How to Reproduce

```sh
METAL_HOST=src/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/metal_host/sm_apple_metal_probe_host
xcrun xctrace record \
  --instrument 'Metal GPU Counters' \
  --output /tmp/gpu_counters.trace \
  --time-limit 20s \
  -- $METAL_HOST --variant occupancy_heavy --iters 5000000 --width 1024

xcrun xctrace export \
  --input /tmp/gpu_counters.trace \
  --xpath '//trace-toc[1]/run[1]/data[1]/table[@schema="metal-gpu-counter-intervals"]' \
  --output /tmp/gpu_counter_intervals.xml
```
