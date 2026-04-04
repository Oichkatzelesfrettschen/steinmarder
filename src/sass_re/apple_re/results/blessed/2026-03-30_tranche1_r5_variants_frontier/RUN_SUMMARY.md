# Apple M1 Tranche1 Variant Frontier Summary (r5)

Date: 2026-03-30  
Source run: `tranche1_focus_BDE_20260330_variants`  
Status: `18/18 executed steps ok` (`B,D,E` focus), `failed_steps=0`

## What this run adds over r4

- Adds two new GPU variants (`threadgroup_heavy`, `occupancy_heavy`) and traces
  all three variants side-by-side (`baseline` + 2 variants).
- Adds row-delta synthesis (`xctrace_row_deltas.csv`,
  `xctrace_row_delta_summary.md`) for quick variant ranking.
- Confirms expanded CPU probe-family disassembly surface for atomic, shuffle,
  load/store, and transcendental families.
- Adds a dedicated host FS probe path in the GPU host harness so `fs_usage`
  has a deterministic signal source when root capture is enabled.

## Key findings

## Metal timing

- `baseline`: `276.450 ns/element`
- `threadgroup_heavy`: `324.750 ns/element` (`+17.47%` vs baseline)
- `occupancy_heavy`: `215.436 ns/element` (`-22.07%` vs baseline)

## xctrace row deltas (largest metal-schema shifts)

- `occupancy_heavy`:
  - `metal-gpu-intervals`: `-21.10%`
  - `metal-gpu-state-intervals`: `-21.04%`
  - `metal-driver-intervals`: `-20.30%`
- `threadgroup_heavy`:
  - `metal-driver-intervals`: `-14.48%`
  - `metal-gpu-intervals`: `-8.80%`
  - `metal-gpu-state-intervals`: `-8.55%`

## CPU mnemonic frontier evidence

Top emitted mnemonics include:
- atomics: `ldadd`
- load/store: `ldr`, `str`, `ldp`, `stp`, `ldur`
- shuffle/bit: `eor`, `ror`
- transcendental/slow path: `fdiv`
- floating-point core lane: `fadd`, `fmadd`, `fmov`

## Notes

- This is a focused tranche (`B,D,E`) and does not include full CPU runtime lane
  (`C`) or neural lane (`F`) in the same manifest.
- `fs_usage` capture in this snapshot was from a `--sudo none` run; root-enabled
  validation should be rerun with `--sudo cache` for full host-evidence parity.

## Primary artifacts

- `ANALYSIS_NEXT_STEPS.md`
- `metal_timing.csv`
- `xctrace_row_deltas.csv`
- `xctrace_row_delta_summary.md`
- `xctrace_trace_health.csv`
- `xctrace_schema_inventory.csv`
- `xctrace_metric_row_counts.csv`
- `cpu_mnemonic_counts.csv`
- `metal_air_opcode_counts.csv`
- `mnemonic_interpretation.md`
