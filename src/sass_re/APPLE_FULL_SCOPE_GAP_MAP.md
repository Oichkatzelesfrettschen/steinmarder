# Apple Full-Scope Gap Map

This note answers the literal scope question for the current Apple typed lane:

> where do we stand for `INT/UINT/BF/FP/TF/MX` across the requested width grid
> `4/8/16/32/64`, split by `cpu`, `metal`, `gpu`, and `neural`?

The machine-readable source of truth is:

- [`tables/table_apple_full_scope_gap_matrix.csv`](tables/table_apple_full_scope_gap_matrix.csv)

That table is generated from the canonical taxonomy, boundary, and timing
tables by:

- [`scripts/build_apple_full_scope_gap_matrix.py`](scripts/build_apple_full_scope_gap_matrix.py)

## Domain meanings

- `cpu`
  - direct CPU-lane boundary and timing rows
- `metal`
  - public Metal/MSL type-surface classification plus typed dispatch timing
- `gpu`
  - GPU-side trace/counter evidence tied to a specific format, not just generic
    float32 probe variants
- `neural`
  - Core ML / PyTorch / MLX typed backend rows and timings

## Current aggregate state

Across the 30 requested family-width slots:

- CPU
  - `12` slots are measured
  - `7` slots are modeled but still gaps
  - `11` slots are not currently modeled in taxonomy
- Metal
  - `11` slots have timed direct/native-or-lowered surfaces
  - `4` slots have timed proxy surfaces
  - `1` slot is classified-only unsupported
  - `3` slots are modeled but still gaps
  - `11` slots are not currently modeled in taxonomy
- GPU
  - `9` slots now have per-format GPU trace/counter evidence
  - `10` modeled slots still lack per-format AGX trace/counter evidence
  - `11` slots are not currently modeled in taxonomy
- Neural
  - `6` slots have timed backend evidence
  - `4` slots are proxy/classified-only
  - `9` modeled slots are still gaps
  - `11` slots are not currently modeled in taxonomy

## Strongest covered slots

- `fp:16`
  - CPU measured
  - Metal timed surface
  - GPU traced and countered through the new per-format `agx_format_*` lane
  - Neural timed backend
- `fp:32`
  - CPU measured
  - Metal timed surface
  - GPU traced and countered through both the legacy float32 lane and the new
    per-format `agx_format_*` lane
  - Neural timed backend
- `bf:16`
  - CPU measured through the explicit host-side proxy row
  - Metal timed surface
  - GPU traced and countered through the new per-format `agx_format_*` lane
  - Neural timed backend
- `int:8` and `uint:8`
  - CPU measured
  - Metal timed surfaces
  - GPU traced and countered through the new per-format `agx_format_*` lane
  - Neural timed backend rows

## Frontier slots now real on Metal

The frontier Metal lane is no longer just planned:

- `tf:32`
  - tracked through `tf32` with payload width `19`
  - Metal has timed `host_proxy` rows
  - GPU trace/counter evidence is now present through the per-format
    `agx_format_*` lane
  - Neural now has timed proxy backend rows on `pytorch_proxy_cpu` and
    `pytorch_proxy_mps`
- `fp:8`
  - `fp8_e4m3` and `fp8_e5m2` now have timed Metal proxy rows
  - GPU trace/counter evidence is now present through the per-format
    `agx_format_*` lane
  - Neural now has timed proxy backend rows on `pytorch_proxy_cpu` and
    `pytorch_proxy_mps`
- `mx:4`
  - `mxfp4` now has timed Metal proxy rows
  - GPU trace/counter evidence is now present through the per-format
    `agx_format_*` lane
  - Neural now has timed proxy backend rows on `pytorch_proxy_cpu` and
    `pytorch_proxy_mps`
  - CPU is still a gap
- `mx:8`
  - `mxfp8` and `mxint8` now have timed Metal proxy rows
  - GPU trace/counter evidence is now present through the per-format
    `agx_format_*` lane
  - Neural now has timed proxy backend rows on `pytorch_proxy_cpu` and
    `pytorch_proxy_mps`
  - CPU is still a gap

## Biggest open gaps

The current high-yield gaps are:

- per-format GPU telemetry
  - the new `agx_format_trace`, `agx_format_counters`, and
    `agx_format_counter_deltas` lanes now cover `bf:16`, `fp:16`, `fp:32`,
    `fp:8`, `int:8`, `uint:8`, `tf:32`, `mx:4`, and `mx:8`
  - the remaining modeled GPU gaps are still the wider integer slots plus
    `fp:4` and `fp:64`
- CPU breadth
  - the Apple CPU atlas still lacks `int:4`, `uint:4`, `int:32`, `uint:64`,
    `fp:4`, `fp:8`, and `tf` / `mx` family reference rows
- Metal breadth for wide/scarce formats
  - only `int:4`, `uint:4`, and `fp:4` remain modeled Metal gaps
  - `fp:64` is now explicitly classified as unsupported rather than silently
    missing
- Neural breadth for frontier floats
  - `tf:32`, `fp:8`, and `mx:4/8` now have timed PyTorch proxy rows
  - the remaining neural gap is breadth and scale:
    extend those proxy timings beyond the current focused workloads and add a
    second non-PyTorch backend when MLX or Core ML frontier support becomes usable

## Scope notes

- `tf32` is represented in the literal `tf:32` slot even though the atlas
  keeps its payload width as `19` for honesty.
- `mxfp6` remains tracked in the core atlas as an extra frontier width outside
  the literal `4/8/16/32/64` request grid.
- Many `bf`, `tf`, and `mx` width slots are intentionally `not_modeled` rather
  than fake gaps, because Apple does not currently expose a plausible direct
  family-width candidate there.
