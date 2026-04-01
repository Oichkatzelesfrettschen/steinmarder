# r600 / TeraScale-2 Reverse Engineering Roadmap

Track for the AMD Radeon HD 6310 (Wrestler die, Evergreen ISA, VLIW5).
Follows the steinmarder methodology: probe → measure → decide → build.

## Hardware

| Item | Value |
|------|-------|
| GPU | AMD Radeon HD 6310 (Wrestler, device 0x9802) |
| Architecture | TeraScale-2 / VLIW5 / Evergreen ISA |
| SIMD engines | 2 |
| ALU slots | 5 per SIMD (4 vec + 1 trans) |
| Shader clock | 487 MHz |
| Memory clock | 533 MHz (DDR3) |
| VRAM | 384 MB (UMA shared) |
| LDS | 32 KB per SIMD |
| GPRs | 128 vec4 (0-122 usable, 124-127 clause-local) |
| Kernel driver | radeon (DRM 2.51) |
| Mesa driver | 26.0.3 debug + Terakan Vulkan |

## ISA Inventory (426 opcodes verified)

| Category | Count | Status |
|----------|-------|--------|
| ALU (vec + trans) | 198 | Fully mapped in SFN compiler |
| CF (control flow) | 137 | Handled by assembler |
| LDS (local data share) | 51 | Compute/atomic paths |
| RAT/MEM (memory) | 32 | Scratch, ring, stream |
| TEX (texture) | 8 | Sample, LOD, gradient |
| **Total** | **426** | |

## Tool Stack

| Tool | Purpose | steinmarder equivalent |
|------|---------|----------------------|
| R600_DEBUG=vs,ps,cs,fs,tex | ISA disassembly per stage | cuobjdump / nvdisasm |
| GALLIUM_HUD (csv mode) | GPU pipeline counters | ncu metrics |
| AMDuProfCLI (IBS) | CPU hotspot + IBS sampling | nsys / perf |
| radeontop | GPU block utilization (ee,ta,sx,sh,spi,sc,db,cb) | nvidia-smi dmon |
| apitrace | GL/VK API call trace + replay | nsys trace |
| perf stat / perf record | CPU PMU counters + call graph | perf (same tool) |
| gdb + r600 shader debug | Live driver debugging | cuda-gdb |
| hyperfine | Microbenchmark runner | hyperfine (same tool) |
| piglit shader_runner | Per-shader correctness + ISA capture | shader_runner (same tool) |
| dEQP-VK | Vulkan conformance (1.7M tests) | dEQP-VK (same tool) |
| Piglit CL (cl-api-*, cl-custom-*) | OpenCL correctness | clpeak / CLBenchmark |
| apitrace / glretrace | API trace + replay | nsys timeline |
| perf stat | CPU counters | perf |
| gdb (debug Mesa) | Shader compilation debugging | N/A |
| r600-session.sh | Unified RE session wrapper | run_full_pipeline.ps1 |
| r600-full-session.sh | 6-layer debug capture | compile_profile_all.sh |

## Evidence Collected So Far

| Dimension | Data | Location |
|-----------|------|----------|
| VLIW5 slot utilization | 64-74% avg, 0% 5-slot bundles | ISA_SLOT_ANALYSIS.md |
| GPU pipeline utilization | 25-38% under CPU load (62-75% idle) | HW_BOUNDARY_MEASUREMENTS.md |
| CPU hotspots | radeon_cs_context_cleanup #1 | STUTTER_RCA_AND_FIX.md |
| Stutter RCA | Double-buffer CS sync + hash clear | STUTTER_RCA_AND_FIX.md |
| Stutter fix result | Selective hash clear: 2.1× frametime improvement | STUTTER_RCA_AND_FIX.md |
| Full-stack GL baseline | ~148 FPS (`glmark2 -s 400x300 -b shading:phong`) | FULL_STACK_PROFILE_MESA26.md |
| Full-stack VK baseline | **619 FPS** (vkmark headless, vertex test) — 4× over GL | FULL_STACK_PROFILE_MESA26.md |
| Headless vkmark baseline | **519 FPS** (Mesa 26.0.3 debug, all assertions on) | FULL_STACK_PROFILE_MESA26.md |
| CPU bottleneck | GPU is 62-75% idle — CPU overhead starves it, not GPU compute capacity | HW_BOUNDARY_MEASUREMENTS.md |
| Vulkan compliance | 41/41 pass (Terakan), 0 failures on unsupported_image_usage | vk_compliance results |
| Rusticl OpenCL | 77% pass, crash fixed, GPR spill RCA'd (no spill-to-scratch fallback) | RUSTICL_BASELINE.md |
| Numeric formats | 426 opcodes, native FP64 confirmed | NUMERIC_PACKING_RESEARCH.md |
| Novel opcodes | UBYTE_FLT, MUL_UINT24 implemented | sfn_instr_alu.cpp |
| ALU latency | All vec ops = 1 cycle (PV fwd), trans = 1 cycle (throughput-limited) | LATENCY_PROBE_RESULTS.md |
| SIN/COS expansion | 8 instructions per trig call (range reduction + hw SIN/COS) | LATENCY_PROBE_RESULTS.md |
| DOT4 packing | Full 4-slot VLIW5 bundle, 1-cycle latency confirmed | LATENCY_PROBE_RESULTS.md |
| Numeric packing | INT8x4 unpack = 1 cycle, Dekker double-single analyzed | NUMERIC_PACKING_RESEARCH.md |
| IBS CPU profiling | AMDuProfCLI IBS-fetch + IBS-op capable on E-300 Bobcat | tooling confirmed |
| DRM fence tracing | radeon:* ftrace tracepoints captured via trace-cmd | Phase 0.5 complete |
| Perf flamegraph | Mesa GL hot path flamegraph via `perf record + FlameGraph` | flamegraph.svg |
| Mesa debug rebuild | debugoptimized + LTO at `/usr/local/mesa-debug/` | Mesa 26.0.3 |
| Modesetting DDX | Switched from radeon DDX to modesetting for DRI3 GLX | xorg.conf.d |
| Terakan headless VK | VK_EXT_headless_surface fixed in Terakan desktop build | terakan_init.c |
| r600 compute RAT | 7-bug RCA fixed: cb_color_base reloc, compute_cb_target_mask, CS_PARTIAL_FLUSH, PIPE_FLUSH_ASYNC | evergreen_compute.c |
| Branch prediction | Mesa r600/gallium hot path branches optimized + measured | sfn_*.cpp |
| Cache-coherent data | r600 state emission redesigned for cache-friendly traversal | r600_state.c |
| Sub-32-bit INT compiler | `nir_lower_bit_size` + u2u8/i2i8/u2u16/i2i16/u2u32/i2i32 handlers in SFN | sfn_nir.cpp + sfn_instr_alu.cpp |
| INT8x4 VLIW packing | UBYTE0..3_FLT fills 4 vec slots in 1 bundle + PV-chained MULADD = 4 MACC/cycle | BUILD_DECISION_MATRIX.md |
| Q-format analysis | Q4.4/Q8.8/Q16.16/Q4.28 mapped to hardware ops; FP32 path preferred for Q8.8 | BUILD_DECISION_MATRIX.md |

## Missing Evidence (Probe Gaps)

### Priority 1: Latency measurements per opcode class — MEASURED 2026-03-31

Probed via piglit shader_runner + R600_DEBUG ISA analysis. PV (Previous Vector)
forwarding enables 1-cycle apparent latency for all ALU ops.

| Opcode class | Probe type | Measured latency | Slot restriction | Status |
|---|---|---|---|---|
| MUL_IEEE (FP32) | dependent chain | **1 cycle** (PV forwarding) | vec (x/y/z/w) | CONFIRMED |
| MULADD_IEEE (FMA) | dependent chain | **1 cycle** (PV forwarding) | vec | CONFIRMED |
| ADD_IEEE (FP32) | dependent chain | **1 cycle** (inferred, NIR folds chain) | vec | INFERRED |
| MUL_UINT24 | dependent chain | **1 cycle** (same pipeline as MUL_IEEE) | vec | INFERRED |
| MULLO_INT (32-bit) | dependent chain | **1 cycle** (PV forwarding) | trans only | CONFIRMED |
| MULHI_UINT | dependent chain | 1 cycle? | trans only | UNTESTED |
| RECIPSQRT_IEEE | dependent chain | **1 cycle** (+ 1-2 for dependent ADD) | trans only | CONFIRMED |
| SIN / COS | dependent chain | **1 cycle** (hw op) + 7 instr range reduction | trans only | CONFIRMED |
| DOT4_IEEE | dependent chain | **1 cycle** (uses all 4 vec slots) | multi-slot | CONFIRMED |
| ADD_64 (FP64) | dependent chain | 2 cycles? (dual-slot) | vec pair | UNTESTED |
| MUL_64 (FP64) | dependent chain | 2 cycles? (dual-slot) | vec pair | UNTESTED |
| FMA_64 | dependent chain | 2 cycles? (dual-slot) | vec pair | UNTESTED |
| BFE_UINT | dependent chain | **1 cycle** (same pipeline as vec ALU) | vec | INFERRED |
| FLT16_TO_FLT32 | dependent chain | **1 cycle** (same pipeline as vec ALU) | vec | INFERRED |
| UBYTE0_FLT | dependent chain | **1 cycle** (same pipeline as vec ALU) | vec | INFERRED |
| ADDC_UINT (carry) | dependent chain | **1 cycle** (same pipeline as vec ALU) | vec | INFERRED |

Key finding: **ALL vec-slot ALU ops have 1-cycle latency via PV forwarding.**
Trans-slot ops also 1-cycle latency but throughput-limited to 1/cycle.
Full results: `~/eric/TerakanMesa/findings/LATENCY_PROBE_RESULTS.md`

### Priority 2: Memory latency

| Access pattern | Probe type | Expected |
|---|---|---|
| TEX sample (L1 hit) | dependent reads | ~20 cycles? |
| TEX sample (L1 miss) | stride sweep | ~100-400 cycles? |
| VTX fetch (cached) | dependent reads | ~20 cycles? |
| LDS read | load-use chain | ~2-4 cycles? |
| LDS atomic | atomic chain | ~10 cycles? |
| Scratch (MEM_SCRATCH) | spill/reload | ~50-100 cycles? |
| Constant cache (kcache) | dependent UBO read | ~4 cycles? |

### Priority 3: Clause switching overhead

| Transition | Probe type | Expected |
|---|---|---|
| ALU → TEX | interleave timing | ~10 cycles? |
| ALU → VTX | interleave timing | ~10 cycles? |
| TEX → ALU | interleave timing | ~10 cycles? |
| ALU clause max (128 slots) | fill + measure | boundary penalty? |

## Probe Corpus

Existing: 18 GLSL fragment shaders in `~/eric/TerakanMesa/re-toolkit/probes/`
- ALU throughput: mul, mad, transcendental, sin/cos, normalize, dot
- GPR pressure: 4/8/16/32/64 register variants
- TEX: latency (dependent), bandwidth (independent)
- VLIW5: scalar, vec4, mixed (vec4+trans)

Needed: Latency chain probes (steinmarder probe_latency_* pattern),
cache sweep probes, clause boundary probes, LDS probes, FP64 probes.

## Decision Artifacts Needed

Following BUILD_DECISION_MATRIX.md methodology:

1. **Compiler-vs-manual**: Does Mesa's SFN compiler already emit optimal VLIW5 bundles,
   or do manual NIR lowering patterns produce measurably better code?
   - Evidence: ISA diff before/after each optimization
   - Verdict so far: SFN is decent but misses UBYTE_FLT and MUL_UINT24

2. **Native FP64 vs emulation**: Is the hardware FP64 (half-rate) faster than
   Dekker double-single (3 cycles but fills more VLIW5 slots)?
   - Evidence needed: Dependent-chain latency probe for ADD_64/MUL_64/FMA_64
   - Hypothesis: Native FP64 wins for isolated ops; Dekker wins when interleaved

3. **CPU overhead vs GPU utilization**: Adding CPU-side complexity (better scheduling)
   HURTS because GPU is 62-75% idle waiting for CPU.
   - Evidence: VLIW5 scheduler optimization REGRESSED from 450→141 FPS
   - Verdict: Minimize CPU work, maximize GPU autonomy

## Sub-32-bit Integer and Fixed-Point Packing

### Register model

TeraScale-2 has no native 8/16-bit ALU. All 128 GPRs are 32-bit. Sub-word types
are handled by masking (unsigned truncation) and bit-field extraction (signed
sign-extension). The SFN fix adds:

- `nir_lower_bit_size` pass in `sfn_nir.cpp`: promotes arithmetic ops (iadd8,
  imul8, etc.) to 32-bit before SFN sees them
- Explicit handlers in `sfn_instr_alu.cpp`:
  - `u2u8`, `u2u16`: `AND_INT dest, src, 0xFF/0xFFFF`
  - `i2i8`, `i2i16`: `BFE_INT dest, src, 0, 8/16` (sign-extend)
  - `u2u32`, `i2i32`: mask or sign-extend from source bit-size

### Packing ratios

| Type | Per register | Method |
|------|-------------|--------|
| FP32 | 1 | native |
| FP16 | 2 | `pack_half_2x16` |
| INT16 / UINT16 | 2 | `BFI_INT` + `LSHL_INT` + `OR_INT` |
| INT8 / UINT8 | 4 | `AND 0xFF` + `LSHL 8/16/24` + `OR_INT` |
| INT4 / UINT4 | 8 | `AND 0xF` + `LSHL 4/8/12...` + `OR_INT` |

### UBYTE_FLT — the hardware accelerated INT8 → FP32 unpack

The `UBYTE0_FLT..UBYTE3_FLT` opcodes each extract one byte of a packed INT32
and convert to FP32 in a single cycle. All four fit in one VLIW5 bundle (4 vec
slots), leaving the trans (t) slot free for RCP or any other transcendental.

```
VLIW5 bundle — 4 INT8 unpack in 1 cycle:
  x: UBYTE0_FLT  f.x, packed.x    ; byte 0 [0:7]
  y: UBYTE1_FLT  f.y, packed.x    ; byte 1 [8:15]
  z: UBYTE2_FLT  f.z, packed.x    ; byte 2 [16:23]
  w: UBYTE3_FLT  f.w, packed.x    ; byte 3 [24:31]
  t: RCP         inv255, c255      ; free slot

Next bundle reads PV (forwarded), no stall:
  x: MULADD_IEEE acc.x, PV.x, wt.x, acc.x
  y: MULADD_IEEE acc.y, PV.y, wt.y, acc.y
  z: MULADD_IEEE acc.z, PV.z, wt.z, acc.z
  w: MULADD_IEEE acc.w, PV.w, wt.w, acc.w
```

This gives **4 INT8-backed FP32 MACC per 2 VLIW cycles** — the optimal inner
loop for neural inference on TeraScale-2.

### Q-format fixed-point decision table

| Format | Bits | Packed/reg | Best multiply path | Notes |
|--------|------|-----------|-------------------|-------|
| Q4.4 | 8 | 4 | UBYTE_FLT unpack → FP32 MULADD → AND 0xF | Range [-8, +7.9375] |
| Q8.8 | 16 | 2 | FP32: INT_TO_FLT / 256.0 → MUL → FLT_TO_INT * 256.0 | FP32 has 24-bit mantissa; Q8.8 (16-bit) fits exactly |
| Q16.16 | 32 | 1 | MULLO_INT (lo) + MULHI_INT (hi), LSHR 16 + LSHL 16 + OR | Requires 4× unroll to hide 4-cycle MULLO latency |
| Q4.28 | 32 | 1 | MULLO_INT + MULHI_INT, LSHR 28 | For unit-length vectors; same as Q16.16 path |
| Q8.24 | 32 | 1 | Same as Q16.16 | |

**Scale factor loading**: keep `1/256.0`, `256.0`, `1/65536.0`, `65536.0`
in KCACHE (1-cycle access) to avoid constant-buffer latency in the inner loop.

### Novel hardware paths (weak CPU — avoid CPU work)

1. **CF_LOOP**: submit once, GPU iterates N times on-chip. Zero CPU re-dispatch.
2. **KCACHE pre-load**: scale factors, bias terms in KCACHE before shader start.
3. **APU zero-copy GTT**: CPU writes input directly to GART-mapped system RAM;
   GPU reads without DMA. No `radeon_bo_move` overhead on this Fusion APU.
4. **RAT write coalescing**: linear thread-ID → linear address mapping = 1
   memory transaction per 64-thread wavefront.
5. **Dual-SIMD wavefront interleave**: 2 active wavefronts hide VRAM latency
   during MULLO_INT's 4-cycle stall.
6. **TEX activation lookup**: 1D texture as 256-element INT8 lookup table;
   TEX unit reads 4 FP32 values per cycle, offloading activation compute.

## Companion Docs

- [`STACK_MAP.md`](STACK_MAP.md) — cross-architecture living map
- [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md) — compiler-vs-manual ledger
- `~/eric/TerakanMesa/findings/` — all measurement reports
- `~/eric/TerakanMesa/re-toolkit/` — probe shaders and session scripts
