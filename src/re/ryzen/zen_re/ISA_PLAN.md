# ISA Plan

The CPU-side counterpart to `src/sass_re/` should eventually have two layers:

1. `src/zen_re/isa/`
   - mnemonic-family probes
   - dependency-chain and throughput variants
   - cache/alignment/stride sweeps
   - ISA-family coverage from scalar x86-64 through AVX2/FMA, with legacy
     archaeology for x87/MMX/SSE4a
2. `src/zen_re/translation/`
   - repo hot-path translation targets
   - NeRF, BVH, renderer, scheduler, and CPU-side LBM-adjacent structure work

## First mnemonic families

- scalar add / mul / div
- `popcnt`, `tzcnt`, `lzcnt`
- loads / stores
- prefetch variants
- SSE add / mul / shuffle / compare
- AVX / AVX2 `vaddps`, `vmulps`, `vfmadd*`, `vperm*`, `vgather*`
- branch and cmov/select patterns

## Priority rule

Measure everything, but optimize what the repo actually executes today:
- first: scalar x86-64 + AVX2/FMA lanes
- second: SSE-family fallback
- later: x87/MMX/SSE4a archaeology
