# Apple Steinmarder Probe Examples

This note ties the Apple probe lanes back to concrete steinmarder examples
instead of leaving the typed atlas at the level of abstract formats alone.

The goal is simple:

> if a CUDA or SASS-era steinmarder pattern mattered, what is the closest Apple
> probe we have today, and what still needs to be built?

The machine-readable source is:

- [`tables/table_apple_steinmarder_probe_examples.csv`](tables/table_apple_steinmarder_probe_examples.csv)

## Current example families

### Packed 4-bit examples

- `lbm_int4_nibble_pair_soa`
  - CUDA source: [`../cuda_lbm/kernels_int4.cu`](../cuda_lbm/kernels_int4.cu)
  - Apple analogue: `probe_int4_proxy`
  - status: timed on Metal, widened AGX trace/counter capture in progress
- `lbm_fp4_nibble_pair_soa`
  - CUDA source: [`../cuda_lbm/kernels_fp4.cu`](../cuda_lbm/kernels_fp4.cu)
  - Apple analogue: `probe_fp4_proxy`
  - status: timed on Metal, widened AGX trace/counter capture in progress
- `lbm_uint4_nibble_pair_soa`
  - CUDA inspiration: INT4 nibble packing as an unsigned carrier companion
  - Apple analogue: `probe_uint4_proxy`
  - status: timed on Metal, widened AGX trace/counter capture in progress

### Packed and widened integer examples

- `lbm_int8_i_major_soa`
  - CUDA source: [`../cuda_lbm/kernels_int8_soa.cu`](../cuda_lbm/kernels_int8_soa.cu)
  - Apple analogue: `probe_int8_surface`
  - status: timed and AGX-traced
- `lbm_uint8_pack4_stream`
  - CUDA source: [`../cuda_lbm/kernels_int8.cu`](../cuda_lbm/kernels_int8.cu)
  - Apple analogue: `probe_uint8_surface`
  - current gap: still needs an explicit Apple pack4 byte-group kernel
- `lbm_int16_i_major_soa`
  - CUDA source: [`../cuda_lbm/kernels_int16_soa.cu`](../cuda_lbm/kernels_int16_soa.cu)
  - Apple analogue: `probe_int16_surface`
  - status: timed on Metal, widened AGX trace/counter capture in progress
- `lbm_uint16_pair_stream`
  - CUDA inspiration: pair-packed 16-bit carrier path
  - Apple analogue: `probe_uint16_surface`
  - status: timed on Metal, widened AGX trace/counter capture in progress
- `lbm_int32_accumulator`
  - CUDA inspiration: INT8 and mixed packed lanes that widen into int32
  - Apple analogue: `probe_int32_surface`
  - status: timed on Metal, widened AGX trace/counter capture in progress
- `lbm_uint32_carrier`
  - CUDA inspiration: byte-packed and halfword-packed carrier type
  - Apple analogue: `probe_uint32_surface`
  - status: timed on Metal, widened AGX trace/counter capture in progress
- `lbm_int64_long_accumulator`
  - Apple analogue: `probe_int64_surface`
  - status: timed on Metal, widened AGX trace/counter capture in progress
- `lbm_uint64_long_carrier`
  - Apple analogue: `probe_uint64_surface`
  - status: timed on Metal, widened AGX trace/counter capture in progress

### Neural frontier examples

- `neural_tf32_proxy_tile`
  - Apple analogue: `pytorch_proxy_cpu` and `pytorch_proxy_mps`
  - status: timed
- `neural_fp8_proxy_tile`
  - Apple analogue: timed `fp8_e4m3` and `fp8_e5m2` proxy rows on CPU and MPS
  - status: timed
- `neural_mx_proxy_tile`
  - Apple analogue: timed `mxfp8`, `mxfp6`, `mxfp4`, and `mxint8` proxy rows on
    CPU and MPS
  - status: timed

## Why this matters

These examples keep the Apple lane anchored to the repo's existing research
identity:

- nibble-packed `INT4` and `FP4` stay visible as bandwidth-ceiling probes
- `INT8` and `INT16` stay tied to the real i-major SoA production story
- `INT32` and `UINT32` stay visible as the widening and carrier lanes behind
  packed formats
- `INT64` and `UINT64` stay available for overflow-sensitive or address-like
  follow-on probes
- neural `TF32`, `FP8`, and `MX` stay honest as timed framework proxies rather
  than undocumented hardware-native claims

## Next extension targets

1. Add explicit Apple pack4 byte-group and pair-packed 16-bit kernels so the
   `uint8` and `uint16` examples stop leaning on scalar-surface stand-ins.
2. Promote the widened AGX format bundle for `int16`, `int32`, `int64`,
   `uint16`, `uint32`, `uint64`, `int4`, `uint4`, and `fp4`.
3. Extend the neural proxy timings from the current smoke workload set to the
   larger `gemm_1024`, `gemm_2048`, and `reduce_16m` cells.
