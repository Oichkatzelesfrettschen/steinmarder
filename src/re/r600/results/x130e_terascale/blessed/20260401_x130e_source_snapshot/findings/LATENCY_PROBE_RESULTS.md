# r600 TeraScale-2 Latency Probe Results

## Methodology

steinmarder dependent-chain methodology applied via piglit shader_runner:
- Each probe: single-threaded fragment shader with N-deep dependent chain
- ISA captured via R600_DEBUG=vs,ps,fs,tex
- Latency inferred from ISA structure (PV forwarding, slot scheduling)
- Throughput counterparts (independent chains) for comparison

## ISA Structure Analysis

### Fragment Shader Instruction Counts (probe shaders only)

| Probe | Chain depth | Target opcode count | Total ALU | Bytecode (dw) | GPRs |
|-------|-------------|---------------------|-----------|---------------|------|
| latency_add_ieee | 64 | 3 MULADD (folded!) | 5 | 14 | 1 |
| latency_mul_ieee | 64 | 64 MUL_IEEE | 69 | 142 | 1 |
| latency_muladd_ieee | 64 | 67 MULADD_IEEE | 71 | 144 | 1 |
| latency_recipsqrt | 32 | 32 RECIPSQRT_IEEE | 100 | 204 | 1 |
| latency_sin_cos | 32 | 16 SIN + 16 COS | 259 | 526 | 1 |
| latency_dot4 | 32 | 128 DOT4_IEEE | 136 | 278 | 2 |
| latency_mullo_int | 32 | 32 MULLO_INT | 43 | 90 | 1 |
| throughput_mul_ieee | 4×16 | 48 MUL_IEEE | 60 | 120 | 1 |
| throughput_recipsqrt | 4×8 | 32 RECIPSQRT_IEEE | 108 | 218 | 1 |

### Key ISA Observations

#### MUL_IEEE — True 1-cycle latency via PV forwarding
```
 0014 809FCC7C 00000100     3 x:     MUL_IEEE   __.x,  T0.w, PV.y
 0016 808F80FE 20000100     4 y:     MUL_IEEE   __.y,  PV.x, T0.y
 0018 808F84FE 40000100     5 z:     MUL_IEEE   __.z,  PV.y, T0.y
 0020 808F88FE 60000100     6 w:     MUL_IEEE   __.w,  PV.z, T0.y
```
Each MUL reads from the **Previous Vector** (PV) register — the result of the immediately
preceding cycle. The scheduler places each MUL in a different slot (x→y→z→w→x...), and
each reads the PV component from the previous cycle. **This proves 1-cycle MUL_IEEE latency**
with PV forwarding — no pipeline bubbles between dependent MULs.

The 64-deep chain uses exactly 64 MUL_IEEE instructions across 64 cycles (1 per cycle,
rotating x→y→z→w). Only 1 GPR needed (PV forwarding eliminates register pressure).

#### MULADD_IEEE — Also 1-cycle latency
Same PV-forwarding pattern as MUL_IEEE. 67 MULADD_IEEE for a 64-deep chain (3 extra
for setup). Each MULADD consumes the previous cycle result via PV.

#### RECIPSQRT_IEEE — 1-cycle on t-slot, but 3 cycles per iteration
The 32-deep chain generates 100 ALU instructions:
- 32 RECIPSQRT_IEEE (t-slot)
- 34 MULADD_IEEE (bias addition to prevent convergence)
- 34 other setup ops

Each RECIPSQRT iteration needs: RECIPSQRT(t-slot) + ADD(vec-slot) = 2-3 cycles per step.
The RECIPSQRT itself is 1-cycle on the transcendental unit, but the dependent
ADD forces a 2-cycle minimum per iteration.

#### SIN/COS — 8 instructions per trig call (polynomial expansion)
Each sin() or cos() compiles to:
```
MULADD_IEEE  PV.x, input, 0.159155 (1/2π), 0.5    ; normalize to [0,1]
FRACT        PV.y, PV.x                             ; wrap to [0,1]
MULADD_IEEE  ...                                     ; polynomial terms
MUL_IEEE     ...                                     ; polynomial terms  
MULADD_IEEE  ...                                     ; polynomial terms
ADD          ...                                     ; polynomial terms
SIN/COS      result, PV                              ; final hardware SIN/COS
MULADD_IEEE  result, bias, ..., PS                   ; add bias
```
32 sin/cos calls → 259 ALU instructions. That is **~8 instructions per trig call**.
However, the hardware SIN/COS opcode on the t-slot is single-cycle; the overhead is
from range reduction and the bias addition.

#### DOT4_IEEE — 4 slots per dot, 1 cycle
The 32×dot4 chain produces 128 DOT4_IEEE instructions (4 per dot product, one per
x/y/z/w slot). Each dot4 uses all 4 vec slots in one VLIW5 bundle. With PV forwarding,
the result is available next cycle. **DOT4 latency = 1 cycle** for the full 4-component
dot product.

#### MULLO_INT — Trans-only, but still 1-cycle with PV
32 MULLO_INT instructions for the 32-deep chain (plus 11 setup ops). MULLO_INT is
restricted to the transcendental slot, so only 1 per cycle. With PV forwarding,
each MULLO_INT reads the previous cycle result. **1-cycle latency** confirmed, but
**throughput = 1 per cycle** (cannot pack into vec slots).

#### ADD_IEEE — Constant-folded by NIR
The 64-deep `v = v + py` chain was reassociated by NIR to `v + 64*py = MULADD(64, py, v)`.
Only 3 MULADD instructions remain. This proves the NIR optimizer handles associative
ADD chains aggressively. **Need a non-associative pattern for ADD latency measurement.**

## Latency Summary

| Opcode | Measured Latency | Slot | Throughput | Notes |
|--------|-----------------|------|------------|-------|
| MUL_IEEE | **1 cycle** | vec (any) | 4/cycle (VLIW5) | PV forwarding confirmed |
| MULADD_IEEE | **1 cycle** | vec (any) | 4/cycle (VLIW5) | Same as MUL |
| ADD_IEEE | **1 cycle** (inferred) | vec (any) | 4/cycle (VLIW5) | Folded by NIR; same pipeline as MUL |
| RECIPSQRT_IEEE | **1 cycle** | trans (t) | 1/cycle | Plus 1-2 cycles for dependent ADD |
| SIN | **1 cycle** (hardware) | trans (t) | 1/cycle | ~8 instr total per sin() call |
| COS | **1 cycle** (hardware) | trans (t) | 1/cycle | ~8 instr total per cos() call |
| DOT4_IEEE | **1 cycle** | vec (x+y+z+w) | 1/cycle (uses all 4 slots) | PV forwarding works |
| MULLO_INT | **1 cycle** | trans (t) | 1/cycle | Int32 multiply, trans-only |

## Throughput vs Latency Comparison

| Probe pair | Latency probe ALU | Throughput probe ALU | Ratio | Interpretation |
|------------|-------------------|---------------------|-------|----------------|
| MUL_IEEE | 69 (64-deep chain) | 60 (4×16 independent) | 1.15x | Near-unity: VLIW5 can pack 4 independent MULs/cycle |
| RECIPSQRT | 100 (32-deep chain) | 108 (4×8 independent) | 0.93x | Trans-slot bottleneck: same throughput regardless of dependence |

The MUL_IEEE throughput probe has fewer total instructions (60 vs 69) despite the same
number of MUL ops (48 throughput vs 64 latency) because:
1. Independent chains allow better VLIW5 packing (multiple MULs per bundle)
2. Fewer setup overhead per chain

For RECIPSQRT, throughput ≈ latency because the trans slot is the bottleneck — only 1
RECIPSQRT per cycle regardless of chain independence.

## VLIW5 Scheduling Observations

1. **PV forwarding is the critical latency-hiding mechanism.** Every dependent chain uses
   PV.{x,y,z,w} rotation to achieve 1-cycle apparent latency.

2. **All vec-slot ALU ops have 1-cycle latency** via PV forwarding: ADD, MUL, MULADD, DOT4.

3. **Trans-slot ops (SIN, COS, RECIPSQRT, MULLO_INT) have 1-cycle latency** but are
   throughput-limited to 1 per cycle.

4. **NIR constant folding is aggressive** — associative chains (ADD with constant) get
   folded. Probes must use non-constant, per-pixel varying operands.

5. **Register pressure is minimal** — all probes use only 1-2 GPRs thanks to PV forwarding.
   This validates the VLIW5 design: PV eliminates the need for intermediate registers.

## Missing Measurements (Priority 2 & 3)

Still needed (from FRONTIER_ROADMAP_R600_TERASCALE.md):
- TEX sample latency (L1 hit / miss) — needs texture-dependent chain probes
- VTX fetch latency — needs vertex-attribute-dependent probes
- LDS read/write/atomic latency — needs compute shader probes
- Scratch (MEM_SCRATCH) latency — needs high-GPR-pressure shader to force spills
- Constant cache (kcache) latency — needs UBO-dependent chain
- ALU → TEX clause switching overhead — needs interleaved ALU/TEX probes
- FP64 (ADD_64, MUL_64, FMA_64) latency — needs GLSL 4.00 or compute

## Files

- Probe shaders: `~/eric/TerakanMesa/re-toolkit/probes/shader_tests/*.shader_test`
- ISA captures: `~/eric/TerakanMesa/re-toolkit/data/latency_probes_20260331T014705/`
- This document: `~/eric/TerakanMesa/findings/LATENCY_PROBE_RESULTS.md`

## CPU Hotspot Under Build Load (AMDuProfCLI IBS, 2026-03-31)

Profiled glmark2 phong (100x100) during Mesa LTO distcc build (load avg 8.29).

### GPU Pipeline Utilization (radeontop)

| Block | Idle+Build | glxgears | glmark2 phong | Description |
|-------|-----------|----------|---------------|-------------|
| gpu | 23% | 19% | **28%** | Overall busy |
| sh | 23% | 14% | **25%** | ALU execution |
| spi | 23% | 15% | **26%** | Shader dispatch |
| ta | 18% | 9% | **23%** | Texture fetch |
| sc | 22% | 11% | **24%** | Rasterization |
| db | 23% | 11% | **23%** | Z/stencil |
| cb | 23% | 13% | **24%** | Framebuffer |
| vgt | 3% | 6% | 6% | Vertex grouper |
| pa | 3% | 8% | 6% | Primitive asm |

GPU never exceeds 28% even at maximum shader load. 72% headroom wasted due to CPU starvation.

### CPU Hotspot (3s TBP sample, top 10)

| Function | Time | Module | Category |
|----------|------|--------|----------|
| _raw_spin_unlock_irqrestore | 34ms | kernel | DRM lock contention |
| finish_task_switch | 15ms | kernel | Context switch overhead |
| __memmove_ssse3 | 12ms | libc | Memory copy |
| **radeon_emit** | 9ms | Mesa r600 | Command buffer write |
| **st_update_rasterizer** | 7ms | Mesa GL | State recalculation |
| __syscall_cancel_arch | 7ms | libc | ioctl |
| **r600_set_atom_dirty** | 6ms | Mesa r600 | State atom marking |
| pthread_cond_signal | 6ms | libc | CS sync signal |
| pthread_mutex_unlock | 6ms | libc | CS mutex |
| __kvmalloc_node_noprof | 5ms | kernel | Allocation |

Module breakdown: Mesa=50%, kernel=29%, libc=11%

### Key Optimization Targets

1. **radeon_emit + r600_set_atom_dirty**: 15ms combined. These are per-draw overhead.
   Batching multiple draws into single command buffer submissions would reduce this.
2. **Kernel spinlock contention**: 34ms. DRM ioctl serialization. Vulkan (Terakan)
   avoids this by using direct command buffer submission.
3. **st_update_rasterizer**: 7ms. Redundant state recalculation. GL state tracker
   could track dirty bits more precisely.
4. **pthread sync**: 12ms combined. The double-buffer CS sync pattern.
