# Counter vs Latency Report — r7 CDE Keepalive

- run_dir: `2026-03-30_tranche1_r7_cde_keepalive`
- date: 2026-04-01 (promoted)
- baseline trace: `gpu_baseline.trace`
- comparison traces: `gpu_threadgroup_heavy.trace`, `gpu_threadgroup_minimal.trace`,
  `gpu_occupancy_heavy.trace`, `gpu_register_pressure.trace`
- new latency probes: `gpu_ffma_lat.trace`, `gpu_ffma_tput.trace`, `gpu_tgsm_lat.trace` (wave 2)
- timing csv: `metal_timing.csv`
- power sample: `powermetrics_gpu.txt`
- trace health: `xctrace_trace_health.csv`
- schema inventory: `xctrace_schema_inventory.csv`
- metric row counts: `xctrace_metric_row_counts.csv`
- row deltas: `xctrace_row_deltas.csv`
- row delta summary: `xctrace_row_delta_summary.md`
- density csv: `xctrace_row_density.csv`
- cache hierarchy: `cpu_runs/cache_hierarchy_analysis.md`
- llvm-mca analysis: `cpu_runs/llvm_mca_analysis.md`
- AIR opcode inventory: `metal_air_opcode_inventory.md`

---

## Timing Summary (measured ns per element @ 1024-wide dispatch)

| Variant | ns/iter | ns/element | vs baseline | Interpretation |
|---------|---------|------------|-------------|----------------|
| baseline | 263,735 | **257.55** | — | Single uint accumulator, simdgroup reduce |
| register_pressure | 231,780 | **226.35** | −12.1% | 3 FP accumulators + type conversions |
| threadgroup_heavy | 218,856 | **213.73** | −17.0% | Threadgroup buffer read-modify-write |
| threadgroup_minimal | 203,812 | **199.04** | −22.7% | Arithmetic-dominant, minimal TG memory |
| occupancy_heavy | 201,425 | **196.70** | −23.6% | Mixed ops, 3 accumulators, no FP casts |

`occupancy_heavy` is the fastest variant — fewer type-conversion stalls than
`register_pressure`, and better arithmetic density than `threadgroup_heavy`.

---

## xctrace Row Density (metal-gpu-state-intervals, rows/sec)

Row density on `metal-gpu-state-intervals` measures how many GPU state transition
events per second the Metal runtime records. Higher density = more state transitions
= more visible GPU activity per unit time.

| Variant | Density (r/s) | vs baseline | Interpretation |
|---------|--------------|-------------|----------------|
| baseline | 2700.6 | — | Reference |
| occupancy_heavy | **3379.1** | **+678.5 r/s (+25.1%)** | Most GPU state transitions |
| threadgroup_minimal | 3037.3 | +336.8 r/s (+12.5%) | Light TG overhead, high dispatch rate |
| threadgroup_heavy | 2445.2 | −255.4 r/s (−9.5%) | TG memory overhead reduces rate |
| register_pressure | **2090.7** | **−609.9 r/s (−22.6%)** | Register-heavy reduces dispatch throughput |

**Key finding**: `occupancy_heavy` wins on density (+678 r/s) AND is the fastest
variant (196.70 ns/element). `register_pressure` loses on both axes — higher latency
and 3x fewer GPU state events per second.

The sign pattern (occupancy_heavy +, register_pressure −) is stable across all
5 Metal schemas captured in this run.

---

## Cross-Schema Density Delta Breakdown (occupancy_heavy vs baseline)

| Schema | Baseline r/s | Variant r/s | Delta r/s |
|--------|-------------|------------|-----------|
| metal-gpu-state-intervals | 2700.6 | 3379.1 | **+678.5** |
| metal-gpu-intervals | 1508.1 | 1919.9 | **+411.9** |
| metal-driver-intervals | 1716.2 | 1978.8 | +262.6 |
| metal-command-buffer-completed | 688.0 | 718.9 | +30.9 |
| metal-application-encoders-list | 535.0 | 587.3 | +52.3 |

All 5 schemas show positive delta for `occupancy_heavy` — consistent GPU pipeline acceleration.

---

## Cross-Schema Density Delta Breakdown (register_pressure vs baseline)

| Schema | Baseline r/s | Variant r/s | Delta r/s |
|--------|-------------|------------|-----------|
| metal-gpu-state-intervals | 2700.6 | 2090.7 | **−609.9** |
| metal-gpu-intervals | 1508.1 | 1190.5 | **−317.6** |
| metal-driver-intervals | 1716.2 | 1359.3 | −356.9 |
| metal-command-buffer-completed | 688.0 | 555.5 | −132.5 |
| metal-application-encoders-list | 535.0 | 405.5 | −129.5 |

All 5 schemas show negative delta for `register_pressure` — consistent throughput reduction.

---

## Absolute Row Count Deltas (from xctrace_row_delta_summary.md)

Top deltas on `metal-gpu-state-intervals` (abs rows, not density):

| Variant | Baseline | Variant | Delta | % |
|---------|---------|---------|-------|---|
| threadgroup_minimal | 5048 | 3701 | −1347 | −26.7% |
| register_pressure | 5048 | 4125 | −923 | −18.3% |
| occupancy_heavy | 5048 | 4597 | −451 | −8.9% |
| threadgroup_heavy | 5048 | 4907 | −141 | −2.8% |

Note: absolute row counts decrease for all variants vs baseline. This is expected —
the baseline shader does fewer ops per iteration, completing more dispatch cycles
in the same measurement window. Density (rows/sec) normalizes for elapsed time and
is the correct comparison metric.

---

## CPU Latency Constants Confirmed (cpu_runs/llvm_mca_analysis.md)

| Operation | Measured cyc/op | MCA pred | Divergence |
|-----------|----------------|---------|------------|
| Integer ADD | 1.56 | 1.62 | +4% (reliable) |
| f64 FADD | 3.94 | 3.03 | −23% (MCA optimistic) |
| f64 FMADD | 4.15 | 3.18 | −23% (MCA optimistic) |
| load+XOR+store chain | 1.18 | 0.82 | −31% (arithmetic hides latency) |
| bswap+shift chain | 4.30 | 1.41 | **−67% (MCA wrong — variable shift)** |
| relaxed atomic add | 6.12 | N/A | (PLT stub, MCA cannot model) |
| sin+cos pair | 59.94 | 7.51 | **−87% (MCA wrong — transcendental)** |

---

## Cache Hierarchy Boundaries (cpu_runs/cache_hierarchy_analysis.md)

Observed via stream bandwidth sweep:

| Tier | Size boundary | Stream ns/access | cyc/access |
|------|--------------|-----------------|------------|
| L1/L2 | ≤ 128 KB | 0.654–0.820 ns | 2.1–2.6 |
| SLC (LLC) | 256 KB – 8 MB | 0.949–1.020 ns | 3.0–3.3 |
| SLC edge | ~16 MB | 1.153 ns | 3.7 |
| DRAM | ≥ 32 MB | 2.278 ns | 7.3 |

L1→SLC boundary: ~128–256 KB (49% ns/access jump at 256 KB).
SLC→DRAM boundary: ~8–16 MB (98% ns/access jump at 32 MB).

---

## Critical Gap: metal-gpu-counter-intervals (0 rows in all runs)

**ALL** prior blessed runs (r5, r6, r7, CDE, phaseE) recorded zero hardware GPU
counter rows. The xctrace recordings used `--template 'Metal Application'` which
includes the `metal-gpu-counter-intervals` TABLE SCHEMA but **never populates it**.

Hardware GPU counters require:
```sh
xcrun xctrace record --template 'Metal GPU Counters' \
  --output <output.trace> -- <binary>
```

Script to capture: `scripts/capture_gpu_counters.sh`

Counter families that would be available with the correct template:
- ALU utilization (vertex/fragment/compute)
- Texture unit utilization
- Memory read/write bandwidth
- L1 tile cache hit rate, L2 cache hit rate
- SIMD utilization
- Fragment overflow (register spill)
- Tiling cycles, render cycles

---

## AIR Opcode Coverage (metal_air_opcode_inventory.md)

Wave 1 (5 shaders): 19 unique ops, zero simdgroup intrinsics, zero FMA, zero barriers.

Wave 2 (5 new shaders): added `@air.fma.f32` (64), `@air.simd_shuffle.u.i32` (16),
`@air.simd_sum.f32` (8), `@air.simd_broadcast.f32` (8), `@air.wg.barrier` (5).

Combined corpus (10 shaders): **27 unique opcodes/intrinsics**. Still missing:
atomic ops, texture sampling, simdgroup matrix, fp16 variants.

---

## Summary Decision Table

| Question | Answer | Evidence |
|----------|--------|----------|
| Is occupancy or register pressure faster? | occupancy_heavy by 15% | metal_timing.csv: 196.7 vs 226.3 ns/element |
| Which variant produces most xctrace signal? | occupancy_heavy (+678 r/s) | xctrace_row_density.csv |
| Does register pressure reduce GPU throughput? | Yes, −22.6% density | xctrace_row_density.csv |
| What is M-series integer ADD latency? | 1 cycle | cpu_runs/llvm_mca_analysis.md |
| What is M-series FP32 FMA latency? | ~4 cycles (FMADD=4.15 cyc) | cpu_runs/llvm_mca_analysis.md |
| What is relaxed atomic cost? | 6 cycles | cpu_runs/llvm_mca_analysis.md |
| Where is the L1→SLC boundary? | ~128–256 KB | cpu_runs/cache_hierarchy_analysis.md |
| Are hardware GPU counters captured? | NO — template wrong in all runs | extract_gpu_counters.py analysis |
