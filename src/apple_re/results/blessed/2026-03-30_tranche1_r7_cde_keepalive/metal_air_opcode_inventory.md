# Metal AIR Opcode Inventory — All Probes

- date: 2026-04-01
- Metal SDK: Apple metal version 32023.830 (air64_v28)
- compiler flags: `xcrun metal -S -emit-llvm -O2`
- shaders: 10 kernel probes across 10 .metal files
- context: cross-run corpus covering all probe variants ever compiled

## Cumulative AIR Opcode Corpus (10 shaders)

| Rank | Opcode | Count | Category |
|------|--------|-------|----------|
| 1 | `call @air.fma.f32` | 64 | FP32 FMA intrinsic (new in wave 2) |
| 2 | `zext` | 51 | integer zero-extend |
| 3 | `getelementptr` | 51 | pointer arithmetic |
| 4 | `load` | 43 | memory load |
| 5 | `phi` | 42 | SSA phi node |
| 6 | `xor` | 37 | integer XOR |
| 7 | `add` | 34 | integer add |
| 8 | `fadd` | 27 | FP32 add |
| 9 | `br` | 24 | branch |
| 10 | `and` | 24 | integer AND |
| 11 | `freeze` | 24 | LLVM freeze (poison prevention) |
| 12 | `fmul` | 22 | FP32 mul |
| 13 | `icmp` | 19 | integer compare |
| 14 | `store` | 16 | memory store |
| 15 | `trunc` | 16 | integer truncate |
| 16 | `call @air.simd_shuffle.u.i32` | 16 | simdgroup shuffle (NEW) |
| 17 | `shl` | 14 | shift left |
| 18 | `ret` | 12 | return |
| 19 | `mul` | 9 | integer mul |
| 20 | `call @air.convert.f.f32.u.i32` | 8 | uint32 → float32 |
| 21 | `call @air.simd_sum.f32` | 8 | simdgroup reduction sum (NEW) |
| 22 | `call @air.simd_broadcast.f32` | 8 | simdgroup broadcast (NEW) |
| 23 | `call @air.wg.barrier` | 5 | threadgroup barrier (NEW) |
| 24 | `lshr` | 4 | logical shift right |
| 25 | `or` | 3 | integer OR |
| 26 | `call @air.convert.u.i32.f.f32` | 1 | float32 → uint32 |
| 27 | `call @llvm.fshl.i32` | 1 | funnel shift left |

Total unique opcodes/intrinsics: **27**

## Wave 1 vs Wave 2 Delta

Wave 1 shaders (baseline, occupancy_heavy, register_pressure, threadgroup_heavy,
threadgroup_minimal, simdgroup_reduce) had 19 unique ops with zero coverage of:
- `@air.fma.f32`
- `@air.simd_shuffle.*`
- `@air.simd_sum.*`
- `@air.simd_broadcast.*`
- `@air.wg.barrier`

Wave 2 added 5 shaders (ffma_lat, ffma_tput, tgsm_lat, simd_reduce_lat,
register_light) and filled ALL of those gaps.

**New intrinsics added by wave 2:** `@air.fma.f32`, `@air.simd_shuffle.u.i32`,
`@air.simd_sum.f32`, `@air.simd_broadcast.f32`, `@air.wg.barrier`

## Per-Probe Breakdown

### probe_ffma_lat (32-deep dependent FMA chain)
```
32  call @air.fma.f32      ← 32 sequential, data-dependent FMA ops per iteration
 3  phi                    ← loop counter, accumulator, loop exit
 2  icmp / br              ← loop control
 1  call @air.convert.f.f32.u.i32  ← seed from gid
 1  fmul / fadd / load / zext / getelementptr / store / ret / add
```
**Purpose**: Isolates FP32 FMA *latency*. Data dependency forces serial execution.
**SASS analog**: FFMA dep-chain probe. Verified: AIR preserves the dep chain (no CSE).

### probe_ffma_tput (8 independent FMA accumulators)
```
32  call @air.fma.f32      ← 32 total FMA (4 per accumulator × 8 accumulators)
17  phi                    ← 8 accumulator phis + loop + exit
15  fadd                   ← sink reduction: a0+a1+...+a7
 2  icmp / br
 1  call @air.convert.f.f32.u.i32
```
**Purpose**: Measures FP32 FMA *throughput*. 8 independent chains allow out-of-order issue.
Latency probe has 3 phi nodes; throughput probe has 17 — compiler kept all 8 accumulators
independent as designed. The 15 fadd ops are the sink expression preventing DCE.

### probe_occupancy_heavy (3 FP accumulators, mixed ops)
```
 7  add / 5  xor / 3  shl / 3  mul
 2  br / phi
```
Minimal intrinsic usage — compiler lowered the float iteration to integer-equivalent ops.
Designed to exercise many registers (3 accumulators + float casts), not FMA depth.

### probe_register_light (single uint LCG accumulator)
```
 1  mul / add / zext / getelementptr / store / ret
```
Minimal — 6 total instructions. No intrinsics, no FP, no branches. This is the
"bracket from below" for register pressure, confirming single-register footprint.

### probe_register_pressure (3 FP accumulators + type conversions)
```
 6  add / 4  shl / 4  phi / 3  xor / 3  or
 2  call @air.convert.f.f32.u.i32 / 2  fmul
```
More ops than register_light but fewer than occupancy_heavy. Designed to demonstrate
the density difference from register footprint.

### probe_simd_reduce_lat (simd_sum, simd_shuffle, simd_broadcast latency)
```
24  freeze                        ← poison-safe for shuffle index computation
18  fmul / 17  xor / 16  trunc
16  and / 16  call @air.simd_shuffle.u.i32  ← 16 dependent shuffles
10  fadd / 9  phi
 8  call @air.simd_sum.f32        ← 8 dependent simd_sum ops (kernel 1)
 8  call @air.simd_broadcast.f32  ← 8 dependent broadcast ops (kernel 3)
 6  icmp / br
 4  call @air.convert.f.f32.u.i32
```
**Key AIR findings**:
- `simd_sum` appears as `call @air.simd_sum.f32` — confirmed as an AIR intrinsic
- `simd_shuffle` lowered to `call @air.simd_shuffle.u.i32(val, idx)` with integer index
- `simd_broadcast` lowered to `call @air.simd_broadcast.f32(val, lane_id)`
- 24 `freeze` instructions: LLVM inserts these to prevent poison propagation from
  the variable shuffle index `(val ^ lane) & 31u` — the index derivation path
- The dependent chain structure is preserved: each shuffle index is derived from
  the prior shuffle result, forcing serialization

### probe_simdgroup_reduce (simdgroup matrix-style reduction)
```
 2  shl / xor
 1  call @air.wg.barrier / add / zext / getelementptr / store / ret
```
Notably sparse — only 8 total instructions. The actual simdgroup reduction work
is absent from AIR because the shader computes the reduction index via shifts and
XOR, then stores a single result. Either: (a) the simdgroup reduction got optimized
away, or (b) the probe design isn't exercising simdgroup opcodes at the AIR level.
This probe needs redesign to force actual simdgroup reduction emission.

### probe_tgsm_lat (threadgroup pointer-chase for TGSM latency)
```
34  zext / 34  getelementptr ← pointer arithmetic per chase step
33  load                      ← 32 dependent loads (TGSM pointer-chase) + init
 4  icmp / br / phi
 2  add / and
 2  store                     ← init write + output write
 1  call @air.wg.barrier      ← explicit threadgroup barrier before chase
```
**Purpose**: Measures threadgroup shared memory (TGSM) load latency via pointer-chase.
32 sequential dependent loads from `tgsm[]` array. The `call @air.wg.barrier` ensures
the init writes by thread 0 are visible before the chase begins.
**SASS analog**: LDS dependent-chain probe (`latency_tgsm_probe.cu`).
**Note**: pointer-chase is sequential-stride (same prefetcher concern as CPU probe) —
random permutation would give cleaner latency isolation.

### probe_threadgroup_heavy (threadgroup shared buffer, mixed ops)
```
 4  zext / getelementptr / br
 3  xor / add / store / load / and
```
Standard threadgroup memory pattern without barrier. Tests basic TG memory access.

### probe_threadgroup_minimal (arithmetic-heavy, minimal TG memory)
```
 9  add / 7  xor / 4  zext / getelementptr
 4  mul / 3  and / 2  shl / store
```
No threadgroup barrier, minimal load count. Arithmetic-dominant.

## AIR Intrinsic Reference Table

| Intrinsic | Count | Metal source | Notes |
|-----------|-------|-------------|-------|
| `@air.fma.f32` | 64 | `fma(a, b, c)` | FP32 FMA, preserves dependency |
| `@air.simd_shuffle.u.i32` | 16 | `simd_shuffle(val, idx)` | Cross-lane shuffle with integer index |
| `@air.simd_sum.f32` | 8 | `simd_sum(val)` | SIMD-width reduction sum |
| `@air.simd_broadcast.f32` | 8 | `simd_broadcast(val, lane)` | Broadcast from lane to all |
| `@air.convert.f.f32.u.i32` | 8 | `(float)uint_val` | uint32 → float32 |
| `@air.wg.barrier` | 5 | `threadgroup_barrier(mem_flags::mem_threadgroup)` | Threadgroup memory fence |
| `@air.convert.u.i32.f.f32` | 1 | `(uint)float_val` | float32 → uint32 |
| `@llvm.fshl.i32` | 1 | compiler-generated | Funnel shift left |

## Coverage Gaps (AIR Intrinsics Not Yet Seen)

| Missing intrinsic (expected) | Metal source | Gap |
|------------------------------|-------------|-----|
| `@air.atomicrmw.*` / `@air.atomic_fetch_add.*` | `atomic_fetch_add_explicit(...)` | No atomic probe yet |
| `@air.texture.*` / `@air.sample.*` | `texture.sample(...)` | No texture sampling probe |
| `@air.simdgroup_matrix_*` | `simdgroup_float8x8::multiply_accumulate` | AGX AMX/matrix units |
| `@air.convert.f.f16.*` | `half` precision ops | No fp16 probe |
| `@air.fma.f16` | `half fma(...)` | fp16 FMA variant |
| `@air.rsqrt.*` / `@air.sqrt.*` | `rsqrt()`, `sqrt()` | Transcendental intrinsics |
| `@air.read_imageui` / texture reads | `texture2d<float>.read(coord)` | Texture read intrinsic |

## Notes on AGX Hardware Opacity

The AIR LLVM IR is the **deepest observable level** of the Metal compilation pipeline.
Apple does not expose:
- AGX native machine code (no `metal-objdump` equivalent for final ISA)
- Hardware instruction timings at the native ISA level
- Register file details (number of physical registers, register classes)

The AIR corpus is therefore the primary mnemonic evidence for the Apple track.
Latency/throughput ground truth comes from `xctrace` Metal timing, not ISA analysis.
This is the fundamental difference from the SASS track (where `nvdisasm` shows
the actual machine code after PTX → SASS compilation).
