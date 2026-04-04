# Apple Typed Boundary Atlas

This note is the canonical typed control plane for the Apple research lane.
It turns the current CPU, Metal, and neural findings into one honest matrix so
we can answer a single question consistently:

> for each format family and width, what is native, what is merely lowered,
> what is only a framework proxy, and where are the real timing boundaries?

It does not replace the Apple tranche runner or the blessed bundles. It gives
them a shared taxonomy and a stable set of machine-readable outputs.

The typed neural runner now also writes incremental `neural_progress.json` and
partial CSV/JSON outputs while it is still executing, so a long Phase F sweep
is observable before the final bundle lands.

The canonical promotion path from a typed Apple run back into these tables is:

- [`scripts/sync_apple_typed_atlas.py`](scripts/sync_apple_typed_atlas.py)
  - promotes neural backend/timing rows plus Metal type-surface and
    `metal_timing.csv` variant timings
  - also promotes `metal_frontier_timing.csv` into typed Metal timing rows so
    native/lowered surfaces land as `agx_type_surface` and honest software
    proxies land as `agx_frontier_proxy`
  - also promotes selected `gpu_hardware_counters.csv` summaries into
    `agx_counters` rows, using variant-prefixed peak/avg metrics with explicit
    sample counts in the notes field
  - promotes a compact comparison slice from `gpu_hardware_counter_deltas.csv`
    into `agx_counter_deltas` rows so baseline-vs-variant ratios and p99 deltas
    are canonical without duplicating the entire sidecar report
  - also lifts selected GPU-trace facts from `xctrace` exports:
    trace duration, per-schema row counts, row density and delta density, and
    command-buffer completion-gap timing derived from
    `metal-command-buffer-completed` exports

## Canonical tables

- [`tables/table_apple_type_taxonomy.csv`](tables/table_apple_type_taxonomy.csv)
  - one row per requested family/format/width
  - defines the honesty tier we expect to prove: `native`, `lowered`,
    `framework_proxy`, `host_proxy`, `emulated`, `unsupported`, or `unknown`
- [`tables/table_apple_type_boundary_matrix.csv`](tables/table_apple_type_boundary_matrix.csv)
  - one row per lane/backend/format boundary
  - records whether the current evidence is a direct measurement, a classifier
    result, a placement-only row, or a planned frontier row
- [`tables/table_apple_type_timings.csv`](tables/table_apple_type_timings.csv)
  - one row per promoted timing value or cache knee
  - keeps warmup handling explicit so neural rows do not silently mix first-run
    compilation with steady-state execution

## Current stance by lane

### CPU lane

The CPU lane is the current native timing reference for Apple:

- `uint32`, `int64`, and `float32` now have direct promoted typed reference
  rows in the atlas
- `int8`, `uint8`, `int16`, and `uint16` now have explicit lowered reference
  rows rather than being silently absent
- `bfloat16` is now present as an explicit host-side roundtrip proxy row
- `float64` and `float16` latency rows are directly measured
- cache knees are promoted and already give the first concrete Apple boundary
  atlas: L1→SLC around `128-256 KB`, SLC→DRAM around `8-16 MB`
- low-bit, `MX`, and non-IEEE families remain proxy or unsupported here unless
  we add software or packed-reference probes on purpose

### Metal lane

The Metal lane now has enough infrastructure to support a true typed atlas:

- direct, classified rows exist for `f32`, `u32`, simdgroup ops, threadgroup
  memory, and atomics
- typed compile-and-timing rows now also exist for `f16`, `bf16`, `int8`,
  `uint8`, `int16`, `uint16`, `int64`, and `uint64`, plus honest proxy rows
  for `tf32`, `fp8_e4m3`, `fp8_e5m2`, `mxfp8`, `mxint8`, `mxfp6`, and `mxfp4`
- AIR-backed evidence now separates direct typed surfaces into real `native`
  rows versus `compiler_lowered` rows instead of leaving them all as
  `native_or_lowered`
- GPU-side timing and hardware counters are now real and promoted
- the promoted counter surface now includes machine-readable
  `gpu_hardware_counters.csv` summaries, not just one-off markdown claims
- the per-format AGX lane is now canonical too:
  `agx_format_trace`, `agx_format_counters`, and
  `agx_format_counter_deltas` are promoted for `bf16`, `float16`, `int8`,
  `uint8`, `tf32`, both `fp8` variants, and the tracked `MX` proxies
- counter deltas are split intentionally:
  compact ratio/p99 delta metrics may be promoted, while the full multi-counter
  comparison table remains a bundle-side sidecar artifact
- AIR/native-vs-lowered separation is still outstanding for the direct typed
  rows, and the frontier formats remain explicit `host_proxy` rows until we can
  prove anything stronger with GPU-side execution evidence

### Neural lane

The neural lane is now the main frontier for typed Apple work:

- current evidence already shows a backend boundary:
  PyTorch CPU beats MPS below roughly `1024x1024`, MPS wins above it
- `f16` on PyTorch MPS is currently an anomaly, not a settled hardware claim
- ANE is currently a placement-visible backend, not an instruction-level lane
- `TF`, `MX`, `NF4`, and other low-bit families belong in the atlas now, but
  only as explicit proxy or unsupported rows until a backend really executes
  them
- targeted frontier-only neural reruns are now first-class too:
  the typed runner can skip native PyTorch, MLX, or Core ML rows so the
  heavier `tf32`/`fp8`/`mx` proxy workloads can be deepened without waiting on
  the full matrix every time

## Wide-precision and extended-FP fastpath findings (2026-04-02)

This section records the novel wide-precision discoveries that opened a new
sub-lane of the CPU probe track.

### Platform facts confirmed

| claim | evidence |
|---|---|
| `long double` == `binary64` on AArch64 | `fadd_dep_ld_aarch64` = 0.949 ns vs `fadd_dep_f64` = 0.949 ns; delta 0.001 ns |
| `__float128` / `_Float128` absent | Apple Clang: "not supported on this target" |
| fp80 x87 not available | x86-exclusive ISA; no AArch64 SIMD equivalent |
| "emulated fp128" routes through NEON FPU | All 5 ladder levels have FPU-comparable latency, not integer-ALU |

### The emulated-type fastpath ladder

```
Level 1  dd_twosum_dep          1.348 ns/value  106-bit   Knuth 2-sum, 4 FP ops/step
Level 2  dd_twosum_fma_dep      1.347 ns/value  106-bit   FMA 2-sum, same dep chain
Level 3  f64x2_vadd_dep         0.473 ns/value   53-bit   NEON 2×f64 per instruction
Level 4  f32x4_vfma_dep         0.237 ns/value   24-bit   NEON 4×f32 FMA, 4.2× f64
Level 5  f64x2_dd_twosum_dep    0.520 ns/value  106-bit   NEON vectorised double-double ◄
```

Level 5 is the breakthrough: **106-bit extended precision at 0.520 ns/value,
1.91× more throughput than scalar float64.** The NEON 128-bit Q-register carries
2 independent double-double (hi, lo) pairs; the M-series OOO engine overlaps the
4-op dep chain to ~3.3 cycles (expected 4). The NEON register IS fp128-equivalent
for this workload at lower cost than plain float64.

FMA is neutral for dep-chain addition (both = 4.31 cy). FMA advantage
only emerges with independent pairs or Veltkamp product splitting.

Timings in: [`tables/table_apple_type_timings.csv`](tables/table_apple_type_timings.csv)
backend `aarch64_neon_dd`. Full writeup: [`APPLE_WIDE_PRECISION_FINDINGS.md`](APPLE_WIDE_PRECISION_FINDINGS.md)

### Next targets opened

1. Metal MSL double-double shader (float2 hi/lo pair in compute kernel).
2. NEON dd throughput probe (4 independent accumulators to unblock FMA advantage).
3. Veltkamp product splitting (~8 FP ops/step, ~3.5× f64 overhead expected).
4. r600 double-single equivalent (f32-pair on VLIW5; see `NUMERIC_PACKING_RESEARCH.md`).

## Honesty rules

Use these rules when extending the tables:

1. Do not omit a requested format just because Apple support is weak.
2. Do not call a row `native` unless the lane exposes a direct executable or
   lowering surface for it.
3. Do not collapse framework behavior into hardware claims.
4. Keep unsupported, proxy, and planned rows visible so the frontier stays
   legible.
5. When a row is only present through Core ML, PyTorch MPS, or MLX, mark it as
   `framework_proxy` or `placement_only` until the backend behavior is directly
   evidenced.

## Immediate extension targets

Completed since last update (2026-04-02):
- [x] Widened AGX GPU counter bundles for int4/uint4/fp4/int16-64/uint16-64 synced.
- [x] int4/uint4 CPU nibble probes added and measured.
- [x] fp4/int4/uint4 neural proxy timing promoted to `measured`.
- [x] long-double alias confirmed; fp80/fp128/fp160 taxonomy rows added to gap matrix.
- [x] Five-level wide-precision fastpath ladder measured; Level 5 is novel finding.

Completed since last entry (2026-04-02):
- [x] NEON DD throughput: `bench_f64x2_dd_throughput` = **0.280 ns/step = 0.140 ns/106-bit value (7.1× scalar f64)**; 3.73× speedup vs dep chain.
- [x] Veltkamp split: `bench_veltkamp_split_dep` = **4.102 ns/step** (fmul-dominated dep chain; 3.06× Knuth 2-sum).
- [x] Metal AGX DD dep chain: `probe_dd_lat.metal` = **3.968 ns/step** (calibrated; FP32 float2 hi/lo).
- [x] Metal AGX DD throughput: `probe_dd_tput.metal` = **1.058 ns/step** (4 independent chains, 3.749× speedup; calibrated).
- [x] Metal host harness iters fix: `setBytes:...atIndex:1` added; inner loop now runs 100k GPU iters per dispatch.
- [x] Metal FMA calibration: `probe_ffma_lat` = **2.960 ns/step**, `probe_ffma_tput` = **2.853 ns/step** (1.038× speedup — AGX FP pipeline saturated at 32 ops/iter).
- [x] NEON DD FMA throughput: `bench_neon_dd_fma_throughput` = **0.281 ns/step** (vs plain 0.279 ns/step; 0.7% diff — FMA neutral confirmed in throughput mode).

- [x] r600 VLIW5 Dekker probe: ISA analysis shows **11 bundles / 8 steps = 1.375 cycles/step** (37.5% overhead vs scalar ADD, not 4× as on scalar; VLIW5 parallelism packs Dekker sub-ops). Rusticl r600 does not expose cl_khr_fp64 so native FP64 comparison is ISA-based only.
- [x] AGX integer native-vs-lowered resolved (2026-04-02):
  - `int16`/`uint16`: **compiler_lowered** — AIR uses `i32`, final `and i32 %n, 65535` truncates; no 16-bit ALU exposed.
  - `int32`/`uint32`: **native** — AIR uses `i32` throughout; xorshift dep-chain baseline = **16,803,740 ns/dispatch** (24 ops/iter, 400 iters, 1024 threads).
  - `int64`/`uint64` shift/XOR: **backend_lowered** — AIR uses genuine `i64` ops (verified via `llvm-dis`), but AGX backend lowers to i32 pairs. INT64 xorshift = **42,170,115 ns** wall-clock = **2.512×** INT32. GPU-internal ratio (commandbuffer GPUTime): **2.544×** — higher than wall-clock, confirming the penalty is structural and not a host-dispatch artefact. Neural GEMM cross-check: MPS int64/int32 at gemm_1024=**2.17×**, gemm_2048=**2.16×** — stable ratio across sizes confirms structural lowering.
  - `int64`/`uint64` multiply: **partially efficient** — INT64 mul (1 dependent mul/iter after LLVM fold) = **4,381,112 ns** vs INT32 = **3,275,605 ns** = **1.337×** (possible widened 32×32→64 multiplier path on AGX vs pure i32-pair lowering for shift/XOR).
  - `fp64`: **unsupported** — Metal compiler rejects `double` with explicit "not supported" error; no action needed beyond atlas classification.

- [x] Neural integer backend timings resolved (2026-04-02):
  - `int16`/`int32`/`int64`: **GEMM + reduce measured** on both `pytorch_cpu` and `pytorch_mps`. MPS shows strong scaling: int16 8.50×, int32 11.8×, int64 19.8× faster than CPU at gemm_1024. MPS int64/int32 GEMM ratio = 2.17× — consistent with AGX i64 backend-lowering penalty (xorshift: 2.51×); GEMM parallelism partially masks latency cost.
  - `uint16`/`uint32`/`uint64`: **GEMM fails** on both CPU (`NotImplementedError: addmm_impl_cpu_ not implemented`) and MPS (`RuntimeError: Failed to create function state object for matmul_ushort/uint/ulong`). **Reduce only**: measured on cpu+mps. Classification: `partial_timed` (reduce-only).

- [x] Metal AGX wide-precision GPU probes (Q.q / FP16xN / FP8x8) resolved (2026-04-02):
  - **Fast-math folding**: Metal `unsafe-fp-math=true` enables `reassoc` flag, which folds
    `e = delta-(s-hi)` to 0 for ALL standard Knuth two-sum probes. Confirmed by GPU timing
    (folded probes cluster at baseline 3.43ms/dispatch).
  - **Fix**: `#pragma clang fp reassociate(off)` on inner compound statement. Disables only
    `reassoc`; full register allocation preserved. `[[clang::optnone]]` was tried — works but
    causes ~30,000× overhead via alloca (stack memory), not useful for FP latency measurement.
  - **Genuine float32 DD**: 5.83ms/dispatch = 1.70× baseline. Extra GPU compute: 2.40ms/100K
    inner iters = 24ns/iter (11-cycle dep chain depth).
  - **Genuine half QH2**: 6.23ms/dispatch = 1.82× baseline. Extra: 2.80ms/100K = **1.17× MORE
    expensive than float32** — AGX promotes half to float internally. Half has no latency
    advantage over float32 for serial dep chains on AGX. Implication: FP16x4 as FP64 substitute
    offers no hardware benefit over FP32x2 double-single.
  - **Genuine half QH4**: 17.10ms/dispatch = 4.99× baseline. Extra: 13.67ms/100K = 4.88× more
    than QH2. Cascade is strictly serial (3 carry levels); AGX has no OOO overlap across levels.
  - **Veltkamp DS-multiply (r600 Q.q)**: 4.82ms/dispatch = 1.40× baseline. LLVM parallelizes
    H-path; L-path fmul+fadd dep chain = genuine serial 14ns/inner_iter.
  - **Software FP8x8**: 295.3ms/dispatch = 86× baseline. 2919ns/inner_iter; encode/decode
    dominates (~116 ops/8-step iter). Not viable without native FP8.
  - **Conclusion**: For Metal/AGX FP64 software emulation, float32 double-single is the optimal
    base. Half precision adds overhead. QH4 cascade depth is confirmed serial.

Still open (hardware-constrained only):
- `fp/float64` GPU column: AGX has no FP64 hardware. Metal compiler rejects `double` keyword.
  Classification is permanently `not_available` at the Metal/GPU layer. CPU is measured; PyTorch CPU float64 neural timing exists. No further GPU action possible.
- `uint16`/`uint32`/`uint64` neural: `partial_timed` (reduce only). PyTorch has no GEMM kernel for unsigned types on CPU or MPS. This is a framework gap, not a hardware gap.
- All other format/lane combinations are resolved: either `measured`, `proxy_measured`, `not_available`, or confirmed `unsupported`. The gap matrix has zero actionable gaps remaining.

## Cross-links

- [`APPLE_FULL_SCOPE_GAP_MAP.md`](APPLE_FULL_SCOPE_GAP_MAP.md)
- [`APPLE_DEEP_DIVE_STACK.md`](APPLE_DEEP_DIVE_STACK.md)
- [`APPLE_STEINMARDER_PROBE_EXAMPLES.md`](APPLE_STEINMARDER_PROBE_EXAMPLES.md)
- [`CROSS_TRACK_CONTROL_PLANE.md`](CROSS_TRACK_CONTROL_PLANE.md)
- [`tables/table_apple_full_scope_gap_matrix.csv`](tables/table_apple_full_scope_gap_matrix.csv)
- [`tables/table_apple_steinmarder_probe_examples.csv`](tables/table_apple_steinmarder_probe_examples.csv)
- [`tables/table_apple_probe_migration_matrix.csv`](tables/table_apple_probe_migration_matrix.csv)
- [`tables/table_cross_track_control_plane.csv`](tables/table_cross_track_control_plane.csv)
- [`APPLE_WIDE_PRECISION_FINDINGS.md`](APPLE_WIDE_PRECISION_FINDINGS.md)
- [`FRONTIER_ROADMAP_APPLE.md`](FRONTIER_ROADMAP_APPLE.md)
- [`APPLE_SILICON_RE_BRIDGE.md`](APPLE_SILICON_RE_BRIDGE.md)
- [`APPLE_TRACK_GAP_ANALYSIS.md`](APPLE_TRACK_GAP_ANALYSIS.md)
- [`../apple_re/README.md`](../apple_re/README.md)
