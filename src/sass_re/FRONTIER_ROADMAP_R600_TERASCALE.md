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
| r600 compute RAT | Kernel writes zeros — RAT readback bug under investigation | Task #15 (open) |
| Branch prediction | Mesa r600/gallium hot path branches optimized + measured | sfn_*.cpp |
| Cache-coherent data | r600 state emission redesigned for cache-friendly traversal | r600_state.c |

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

## Companion Docs

- [`STACK_MAP.md`](STACK_MAP.md) — cross-architecture living map
- [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md) — compiler-vs-manual ledger
- `~/eric/TerakanMesa/findings/` — all measurement reports
- `~/eric/TerakanMesa/re-toolkit/` — probe shaders and session scripts
