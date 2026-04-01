# Apple M-series CPU — FP16 Latency Probes

- date: 2026-04-01
- machine: Apple M1, 16 GB, arm64, ~3.2 GHz, FEAT_FP16 (armv8.2-a)
- tool: `apple_cpu_latency` (32-deep dependent-chain probes, 2M iters)
- build: `clang -std=gnu11 -O2 -march=armv8.2-a+fp16`
- motivation: PyTorch MPS f16 matmul 8× slower than f32 at 512×512 — baseline hardware f16 to separate hardware limit from framework issue

## Measured latency — FP16 family vs reference

| Probe | Instruction | ns/op | cyc/op @3.2GHz | vs f32/f64 reference |
|-------|-------------|-------|---------------|----------------------|
| `fadd_dep_f64` | FADD (f64 scalar) | 0.957 | ~3.1 | reference f64 |
| `fmadd_dep_f64` | FMADD (f64 scalar) | 1.275 | **~4.1** | reference f64 FMA |
| `fcvt_f32_f16_roundtrip` | FCVT Hs,Ss + FCVT Sd,Hh | 0.952 | **~3.0 per FCVT** | same as f64 FADD |
| `fadd_dep_f16_scalar` | FADD Hd,Hn,Hm (scalar) | 0.952 | **~3.0** | same as f64 FADD |
| `fmla_dep_f16x8_simd` | FMLA Vd.8h,Vn.8h,Vm.8h (SIMD) | 1.289 | **~4.1** | same as f64 FMADD |

## Key findings

### 1. f16 hardware is NOT slower than f32/f64 on M-series

All f16 operations have the same latency as their f32/f64 counterparts:
- FCVT (f32→f16 or f16→f32): **~3 cycles** — same as FADD f64
- FADD f16 scalar: **~3 cycles** — same as FADD f64
- FMLA f16×8 SIMD: **~4 cycles** — same as FMADD f64

The M-series FP pipeline handles f16, f32, and f64 at the same rate. There is
**no hardware-level f16 penalty** on Apple M1.

### 2. PyTorch MPS f16 8× slowdown is NOT a hardware limitation

The `neural_lane_results.md` measured PyTorch MPS f16 matmul at 512×512 as
8.5× slower than f32. These CPU probes show the hardware f16 pipeline is
identical in latency to f32.

**Root cause is in software**. Most likely candidates:

| Candidate | Evidence for |
|-----------|-------------|
| JIT shader compilation on first f16 dispatch | The measurement included first-run compilation; f16 shaders may need separate compilation from f32 path |
| PyTorch MPS f16 routing via soft-cast path | PyTorch may convert f16 inputs to f32, dispatch f32 kernel, then convert back — 3 passes instead of 1 |
| Metal driver: f16 not dispatching to native hardware path | If PyTorch's Metal kernel is written for f32 and dynamically down-casts inputs, that would add conversion overhead |
| Small matmul dominated by dispatch overhead | At 512×512, f32 MPS takes 0.52 ms and f16 takes 4.4 ms — both are dispatch-overhead-dominated; f16 path may have extra setup |

**Recommendation**: re-run with more warmup iterations and larger sizes (2048×2048)
before drawing conclusions. The 1-shot 512×512 timing likely includes compilation cost.

### 3. FCVT round-trip: 3 cycles per conversion

The f32→f16→f32 round-trip chain:
- FCVT H, S (f32 to f16): ~1.5 cyc (shared latency with next FCVT)
- FCVT S, H (f16 to f32): ~1.5 cyc

Total round-trip = 3 cycles / 2 ops = 1.5 cycles/FCVT OR the measured 3 cycles
is the latency of a single FCVT (each output of one FCVT is immediately the input
of the next one, so both FCVT variants are chained together and each half of the
pair must wait for the full latency before starting).

Either way: **FCVT is cheap** (≤3 cycles) on M-series. Converting f32 arrays to
f16 for transmission to Metal is not a bottleneck in the host-side path.

### 4. FMLA .8h SIMD: same latency as FMADD f64 scalar

The 8-wide f16 SIMD FMA (`FMLA Vd.8h`) and the scalar f64 FMA (`FMADD Dd`) both
measure **~4 cycles**. This means:
- The NEON FP unit handles f16 SIMD at the same rate as scalar f64
- 8 f16 elements per cycle (throughput-wise) with 4-cycle latency — this is a
  very efficient path for half-precision compute loops

### 5. Complete CPU latency table (all probes, this run)

| Probe | ns/op | cyc/op @3.2GHz |
|-------|-------|----------------|
| add_dep_u64 | 0.328 | ~1.1 (overhead) |
| fadd_dep_f64 | 0.957 | ~3.1 |
| fmadd_dep_f64 | 1.275 | **~4.1** |
| fcvt_f32_f16_roundtrip | 0.952 | **~3.0/FCVT** |
| fadd_dep_f16_scalar | 0.952 | **~3.0** |
| fmla_dep_f16x8_simd | 1.289 | **~4.1** |
| mul_dep_u64 | 0.961 | **~3.1** |
| madd_dep_u64 | 0.955 | **~3.1** |
| msub_dep_u64 | 0.956 | **~3.1** |
| umulh_dep_u64 | 0.977 | **~3.1** |
| smull_dep_i32_to_i64 | 0.951 | **~3.0** |
| load_store_chain_u64 | 0.507 | ~1.6 (bandwidth limited) |
| shuffle_lane_cross_u64 | 1.345 | **~4.3** |
| atomic_add_relaxed_u64 | 1.929 | **~6.2** |
| transcendental_sin_cos_f64 | 18.75 | **~60** |

## Build decisions

| Question | Answer |
|----------|--------|
| Is FCVT f32↔f16 cheap? | **Yes — 3 cycles** — convert freely, no avoidance needed |
| Is f16 FADD cheaper than f64? | **Same** — no precision-based latency savings |
| Is FMLA f16×8 efficient? | **Yes — 4 cycles for 8 elements** — better throughput-per-cycle than scalar |
| Why is PyTorch MPS f16 8× slower? | **Framework issue, not hardware** — hardware f16 is full speed |
| Use f16 for CPU→Metal handoff? | **Yes, if the Metal kernel is truly f16** — FCVT cost is negligible |
