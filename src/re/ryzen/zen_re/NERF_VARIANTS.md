# NeRF CPU Variant Registry

Current CPU-side NeRF MLP variants are selected through `SM_NERF_MLP_VARIANT`
and parsed centrally in `src/nerf/nerf_simd.c`.

## Current variants

- `generic`
  - baseline single-ray path
- `prepacked`
  - aligned/prepacked single-ray path for the fixed `27 -> 64 -> 64 -> 4` model

## Measurement lane

- focused MLP comparisons:
  - `build/zen_re_mlp_matrix/specialization_ramp_compare.csv`
- interleaved end-to-end phase sweep:
  - `build/zen_re_variant_sweep/aggregate.csv`
  - `build/zen_re_variant_sweep/delta_summary.csv`

## Current decision

- `prepacked` is the leading fast-path candidate.
- It is not the unconditional default yet because the `96x96x64 @ 6 threads`
  case showed a small total-phase regression even while MLP time improved.
- Any future variant should only stay in the registry if it wins in the
  end-to-end phase sweep, not just in a focused microbench.
