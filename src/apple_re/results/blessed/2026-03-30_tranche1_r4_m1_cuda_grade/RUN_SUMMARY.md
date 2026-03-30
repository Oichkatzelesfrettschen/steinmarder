# Apple M1 Tranche1 "CUDA-grade" Evidence Summary (r4)

Date: 2026-03-30  
Source run: `tranche1_raw_20260330_m1_cuda_grade_r4`  
Status: `42/42 ok`, `failed_steps=0`

## What this run proves

- The Apple tranche harness now executes all 42 steps with full-root capture
  enabled through cached sudo (`--sudo cache`) and askpass priming.
- Step 27 now produces real `xctrace` metric exports (`trace_health`,
  `schema_inventory`, row-count CSVs, per-schema XML), not placeholder outputs.
- Step 28 uses live PID-targeted host capture flow for `sample` + `spindump`
  (and attempts PID-targeted `fs_usage` in the same window).
- CPU diagnostics now run `leaks` and `vmmap` against a live process PID.

## Key evidence artifacts

- Trace health: `xctrace_trace_health.csv`
  - `cpu_time_profiler.trace`: target `exit(0)`
  - `gpu_baseline.trace`: target `exit(0)`
  - `gpu_compare.trace`: target `exit(0)`
- Trace export coverage: `xctrace_export_summary.md`
  - `traces_found=3`
  - `per-schema_xml_exports=28`
- GPU counters/tables: `xctrace_metric_row_counts.csv`
  - `metal-command-buffer-completed`: `1059`
  - `metal-driver-intervals`: `2197`
  - `metal-gpu-intervals`: `1260`
  - `metal-gpu-state-intervals`: `2447`
- Host/system lane:
  - `cpu_runs/dtruss.log` (SIP-limited but valid syscall trace output)
  - `cpu_runs/powermetrics_cpu.txt` (CPU residency + power)
  - `powermetrics_gpu.txt` (GPU residency + power)
  - `sample_gpu_host.txt` (PID-scoped stack sample)
  - `spindump_gpu_host.txt` (PID-scoped root capture)
- Memory diagnostics:
  - `diagnostics/leaks.txt` shows `0 leaks` for live probe PID
  - `diagnostics/vmmap.txt` maps live probe memory layout

## Mnemonic interpretation (data-backed)

Generated files:

- `cpu_mnemonic_counts.csv`
- `metal_air_opcode_counts.csv`
- `mnemonic_interpretation.md`

CPU AArch64 top mnemonics (aggregated from O0/O2/O3/Ofast disassembly):

- `add` (252), `mov` (129), `fadd` (128), `fmadd` (128),
  `adrp` (106), `bl` (94), `ldr` (77), `stp` (72)

Interpretation:

- Integer/branch/control and FP lanes are both clearly represented and align
  with probe semantics.
- O3/Ofast improve integer and FP-add timing relative to O2 in this run:
  - `add_dep_u64`: O3 `0.516 ns/op` vs O2 `0.914 ns/op`
  - `fadd_dep_f64`: O3 `1.553 ns/op` vs O2 `2.389 ns/op`

Metal AIR op frontier from metallib disassembly:

- `shl`, `xor`, `add`, `store`, `ret`, and `tail_call` (`air.wg.barrier`)

Interpretation:

- Apple GPU visibility remains AIR/LLVM-level (not private hardware ISA).
- CUDA-style evidence transfer is therefore: AIR semantic ops + counters +
  host/power traces, rather than SASS opcode claims.

## Remaining caveat

- `fs_usage_gpu_host.txt` is still empty in this run. The capture command now
  executes in a valid PID window, but this short probe can legitimately emit no
  file-system events.
