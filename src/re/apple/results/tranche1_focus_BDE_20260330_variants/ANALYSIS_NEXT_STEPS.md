# Apple Tranche Focus Analysis (B/D/E)

Date: 2026-03-30  
Run: `tranche1_focus_BDE_20260330_variants`  
Scope: phases `B,D,E` (`18/42` steps executed, `0` failed)

## Executive signal

- The new three-variant Metal lane is producing meaningful, directional deltas.
- The occupancy-heavy variant is currently the best performer in this probe set.
- CPU mnemonic frontier now visibly includes atomics (`ldadd`) and transcendentals (`fdiv`).
- Root-only host evidence is still partially constrained in this run because it was
  executed with `--sudo none`.

## Key data points

## Metal timing (`metal_timing.csv`)

- `baseline`: `276.450 ns/element`
- `threadgroup_heavy`: `324.750 ns/element` (`+17.47%` vs baseline)
- `occupancy_heavy`: `215.436 ns/element` (`-22.07%` vs baseline)

Interpretation:
- For this probe shape, added arithmetic/occupancy pressure improved throughput.
- Threadgroup-heavy pressure reduced throughput, consistent with a stronger local
  memory/synchronization overhead profile.

## xctrace row deltas (`xctrace_row_deltas.csv`)

Largest metal-schema deltas vs baseline:

- `occupancy_heavy`:
  - `metal-gpu-state-intervals`: `-21.04%`
  - `metal-gpu-intervals`: `-21.10%`
  - `metal-driver-intervals`: `-20.30%`
- `threadgroup_heavy`:
  - `metal-driver-intervals`: `-14.48%`
  - `metal-gpu-state-intervals`: `-8.55%`
  - `metal-gpu-intervals`: `-8.80%`

Interpretation:
- Row-count contraction tracks the timing deltas and suggests lower per-run
  interval/event volume for the occupancy-heavy path.
- This is strong comparative evidence but still row-count based; next tranche
  should normalize by elapsed time and add metric-level values where available.

## CPU mnemonic frontier (`cpu_mnemonic_counts.csv`)

Top observed mnemonics include:
- control/int: `add`, `b`, `bl`, `cbz`, `cbnz`
- memory: `ldr`, `str`, `ldp`, `stp`, `ldur`
- floating-point: `fadd`, `fmadd`, `fmov`, `fdiv`
- bit/shuffle-like: `eor`, `ror`
- atomics: `ldadd`

Aggregated family coverage snapshot:
- load/store-family evidence: `1091` hits
- branch/control-family evidence: `776` hits
- floating-point-family evidence: `611` hits
- transcendental/slow-path-family evidence: `253` hits
- shuffle/bit-family evidence: `159` hits
- atomic-family evidence: `97` hits

Interpretation:
- Requested family expansion is now visible in emitted assembly.
- Atomic and transcendental families have crossed from scaffold to observed data.

## CUDA-method parity status (Apple translation)

Completed equivalents:
- `cuobjdump/nvdisasm` class -> `otool`/`llvm-objdump` + Metal AIR disassembly
- `ncu` class -> `xctrace` schema export + host timing CSV
- `nsys` class -> trace TOC/XML inventory + PID-scoped host captures
- valgrind/sanitizer class -> ASan/UBSan lane + `leaks`/`vmmap` workflow

Still strengthening:
- root-dependent host I/O evidence consistency (`fs_usage`) under non-interactive
  cache-safe sudo flow
- metric normalization from row-count deltas into elapsed-normalized comparisons

## Immediate next tranche (ranked)

1. Rerun phase `E` with cached sudo (`--sudo cache`) to force root host evidence
   and confirm non-empty `fs_usage` on the new FS probe path.
2. Add elapsed-normalized xctrace comparison (rows/sec per schema) to separate
   timing-duration effects from true event-density changes.
3. Run `C,D,E` together so CPU runtime lane and GPU variant lane are captured in
   one manifest for direct cross-lane synthesis.
4. Add one more GPU variant emphasizing register pressure only (minimal
   threadgroup use) to isolate occupancy effects from memory/barrier effects.
5. Add CPU throughput mode (independent accumulators) beside dependent-latency
   mode for each new probe family to mirror the NVIDIA latency/throughput split.

## Suggested commands

```sh
# 1) Root-capable GPU lane rerun
src/apple_re/scripts/prime_sudo_cache.sh
src/apple_re/scripts/run_apple_tranche1.sh \
  --phase E \
  --sudo cache \
  --iters 5000 \
  --out src/apple_re/results/tranche1_phaseE_cache_$(date +%Y%m%d_%H%M%S)

# 2) Full CPU+GPU synthesis rerun
src/apple_re/scripts/run_apple_tranche1.sh \
  --phase C,D,E \
  --sudo cache \
  --iters 5000 \
  --out src/apple_re/results/tranche1_CDE_cache_$(date +%Y%m%d_%H%M%S)
```
