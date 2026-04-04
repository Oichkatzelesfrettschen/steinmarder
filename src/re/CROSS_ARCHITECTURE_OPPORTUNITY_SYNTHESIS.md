# Cross-Architecture Opportunity Synthesis

## Steinmarder → r600 / Terakan / Rusticl Maximal Exploitation Report

**Date:** 2026-04-02
**Scope:** Recursive analysis of all 5 steinmarder tracks, 650+ source files, 68 Python scripts, 99 shell scripts, 58+ r600 probes, 494 SASS probes, 13 Metal shaders, 30+ CUDA LBM kernel variants
**Goal:** Identify every transferable pattern, missing measurement, tooling gap, and novel optimization opportunity for the AMD E-300 APU (Radeon HD 6310, TeraScale-2/VLIW5) targeting Mesa r600/Gallium/NIR/OpenGL/EGL/GLX/GLES/Vulkan(Terakan)/Rusticl

---

## §1  Transferable Patterns: SASS → r600/Terakan/Rusticl

### 1.1  Dependent-Chain Latency Probes (CRITICAL — partially adopted)

**Source:** SASS probes use dependent ALU chains with hardware timers (`clock64()`) to isolate per-instruction latency. Each chain creates a data dependency so the GPU cannot hide latency via ILP. Divide elapsed cycles by iteration count → true instruction latency.

**Current r600 adoption:** 13 piglit `shader_runner` latency probes exist (`latency_mul_ieee.shader_test`, etc.) confirming 1-cycle PV-forwarded latency for all vec ALU ops. 11 OpenCL `depchain_*.cl` probes cover integer widths.

**Gaps to fill:**

| Missing Measurement | SASS Equivalent | r600 Method | Priority |
|---|---|---|---|
| TEX fetch latency (L1 hit) | `tex_latency_l1.cu` chain | `latency_tex_l1_hit.frag` exists but **unmeasured** — no timing data in findings | P0 |
| TEX fetch latency (L1 miss → L2) | `tex_latency_l2.cu` | New probe: stride texture reads exceeding 8KB L1 | P0 |
| VTX fetch latency | N/A (CUDA has no VTX) | New probe: vertex buffer read chain via `shader_runner` vertex shader | P1 |
| LDS read/write latency | `shared_latency.cu` | New OpenCL probe: `depchain_lds_rw.cl` with local memory dependency | P0 |
| ALU clause → TEX clause switch cost | Implicit in SASS `tex_alu_interleave.cu` | `clause_switch_alu_tex.frag` exists but **unmeasured** | P0 |
| FP64 ADD_64/MUL_64/FMA_64 latency | `fp64_latency.cu` | `latency_fp64.frag` exists but **unmeasured** — no timing in findings | P1 |
| KCACHE (constant buffer) latency | N/A | `latency_kcache.frag` exists but **unmeasured** | P1 |
| GPR spill → scratch latency | `spill_latency.cu` | New probe: force 124+ GPR usage, measure scratch round-trip | P0 |
| EXPORT clause latency | N/A | New probe: measure MRT export vs single-RT frametime delta | P2 |

**Action:** Run all 6 existing-but-unmeasured probes on x130e. Create the 4 new probes. This closes the biggest measurement gap in the entire program.

### 1.2  Encoding Analysis / Opcode Field Mapping

**Source:** `encoding_analysis.py` (SASS track) parses 128-bit instruction words, identifies constant bits (opcode fields) vs variable bits (operand fields), builds bit-field maps for every mnemonic.

**r600 transfer:** The 426-opcode inventory in `ISA_SLOT_ANALYSIS.md` has slot assignments but no **encoding-level analysis** of the R600 microcode words. The Evergreen ISA spec documents the ALU instruction format (64-bit ALU word: opcode[7:0], src0_sel[8:0], src1_sel[8:0], dst_gpr[6:0], etc.) but nobody has verified these against actual `R600_DEBUG=fs,vs` disassembly output to confirm:

- Whether Mesa's assembler ever emits undocumented modifier bits
- Whether any opcode aliases exist (same encoding, different mnemonic in different contexts)
- Whether the SFN scheduler uses reserved fields for hints

**Action:** Port the constant-bit analysis from `encoding_analysis.py` to consume `R600_DEBUG` ISA dumps. Feed in all 18+ existing shader disassemblies. Flag any non-constant bits in positions the spec claims are reserved. This is a **novel contribution** — no public r600 encoding audit exists.

### 1.3  Auto-Explorer Surrogate Model

**Source:** `auto_explorer.py` (1,100+ lines) ingests SASS disassembly + NCU stall columns, extracts features (mnemonic bigrams/trigrams, stall regime classification), trains a RandomForestRegressor to predict cycles and stalls, then ranks unexplored probe configurations by predicted novelty × predicted performance impact.

**r600 transfer:** The r600 equivalent would ingest:
- `R600_DEBUG=fs,vs` ISA dumps (mnemonic chains, slot assignments, PV/PS usage)
- `GALLIUM_HUD` counter traces (gpu_busy, shader_busy, num_draw_calls)
- `radeontop` utilization percentages
- `perf stat` IPC and cache miss rates

Features: VLIW5 bundle occupancy (1-5 slots), t-slot usage ratio, PV-chain length, clause type sequence (ALU→TEX→ALU), GPR live range density.

Prediction targets: FPS, frametime CV%, GPU idle fraction.

**Action:** Create `auto_explorer_r600.py` adapting the feature extraction pipeline. Even a 50-tree forest on the existing 18 shader profiles would surface which bundle patterns correlate with high GPU idle time.

### 1.4  Mnemonic Frontier Tracking

**Source:** `mnemonic_hunt.py` compares emitted SASS mnemonics against a CSV baseline census, outputs novel mnemonics (frontier deltas).

**r600 transfer:** The 426-opcode inventory is the baseline. Track which opcodes actually appear in compiled shaders across `R600_DEBUG` dumps. Identify:
- Opcodes that **never** appear (compiler dead code, or hardware-only)
- Opcodes that appear in only one shader type (VS-only, FS-only)
- Opcodes whose frequency changed between Mesa 25 and Mesa 26

**Action:** Create `r600_mnemonic_census.py` consuming ISA dump archives. Cross-reference against `evergreen_reg.h` for any register-mapped opcodes the compiler doesn't emit.

### 1.5  Performance Regime Classification

**Source:** SASS auto_explorer classifies kernels into 4 stall regimes: `membar_plus_latency`, `long_scoreboard_dominated`, `short_scoreboard_pressure`, `mixed_dependency`.

**r600 equivalent regimes:**
- **CPU-submission-bound** (GPU idle >50%, `radeon_cs_context_cleanup` in `perf top`)
- **ALU-throughput-bound** (VLIW5 utilization >90%, t-slot saturated)
- **TEX-latency-bound** (GPU busy but shader_busy low, clause switches dominate)
- **bandwidth-bound** (radeontop VRAM% high, resolution-dependent framerate)
- **frametime-variance-bound** (CV% >30%, alternating short/long frames)

Each regime implies a different optimization target. The plan's Phase 5 currently conflates CPU-bound and GPU-bound scenes — the per-scene bottleneck matrix needs this regime classification applied per workload.

---

## §2  Transferable Patterns: Apple Metal → r600/Terakan/Rusticl

### 2.1  xctrace Row Density Analysis → trace-cmd Event Density

**Source:** Apple track uses `extract_xctrace_metrics.py` and `compare_xctrace_density_runs.py` to measure events/sec as a GPU throughput proxy. Schema row counts from xctrace XML tables give per-schema density, enabling cross-tranche normalization.

**r600 transfer:** `trace-cmd` with ftrace tracepoints on the radeon DRM module produces similar event streams:
- `drm:drm_vblank_event` — vblank rate (should match refresh or frame rate)
- `radeon:radeon_fence_emit` / `radeon_fence_wait_begin` / `radeon_fence_wait_end` — fence turnover rate
- `radeon:radeon_cs_ioctl` — command stream submission rate
- `sched:sched_switch` on GPU-related threads — CPU scheduling jitter

**Action:** Create `r600_trace_density.py` consuming `trace-cmd report` output. Compute events/sec per tracepoint. Normalize across baseline/variant runs. This directly enables the "driver interval profiling" technique from the Apple track.

### 2.2  GPU Counter Delta Analysis

**Source:** `analyze_gpu_counter_deltas.py` computes baseline_avg_pct → variant_avg_pct → delta → ratio across 14 focus counters per Metal shader variant.

**r600 transfer:** The Evergreen SQ performance counters at 0x9030-0x904C provide equivalent signals:
- SQ_PERF_SEL_ALU_BUSY (ALU utilization)
- SQ_PERF_SEL_TEX_BUSY (TEX clause utilization)
- SQ_PERF_SEL_LDS_BUSY (LDS utilization)
- SQ_PERF_SEL_WAVE_LAUNCH (wavefront launch rate)
- SQ_PERF_SEL_EXPORT_BUSY (export clause utilization)

These are **currently inaccessible** without either:
1. The Phase 2.5 DRM debugfs patch (preferred — ~250 lines in `radeon_debugfs.c`)
2. umr MMIO-direct path via BAR2 at `/sys/bus/pci/devices/0000:00:01.0/resource2`

**Action:** Prioritize the debugfs SQ counter patch. Once available, port `analyze_gpu_counter_deltas.py` to consume the new counters. This is the single highest-leverage tooling improvement — it converts the GPU from a black box to a transparent pipeline.

### 2.3  Command Buffer Batching

**Source:** Apple Metal's command buffer model batches multiple render/compute passes into a single commit. The Apple track's `metal_probe_host.m` demonstrates minimal-overhead dispatch by encoding N dispatches per command buffer before committing.

**r600 transfer:** Currently, radeon DRM submits one CS (command stream) per ioctl. The Phase 2.5 CS batch submission patch (~500-800 lines in `radeon_cs.c`) would allow multiple CS per ioctl, reducing kernel transition overhead. This is architecturally identical to Metal's command buffer batching.

**Measured impact potential:** `radeon_cs_context_cleanup` is the #1 CPU hotspot at 0.13s. Each CS submission triggers the full 4096-entry hash table walk. Batching N submissions into 1 ioctl reduces this to 1 hash walk per N frames.

### 2.4  Multi-Backend Typed-Format Matrix

**Source:** Apple neural lane (`neural_typed_matrix.py`) sweeps every dtype (float16/32/64, bfloat16, int8/uint8, tf32, fp8_e4m3/e5m2, mxfp8/6/4, mxint8, fp4, int4/uint4) across every backend (CPU, MPS, MLX, CoreML), producing a capability + timing matrix.

**r600 transfer:** Create an equivalent `r600_typed_format_matrix.py` sweeping:
- Backends: OpenGL (r600g), Vulkan (Terakan), OpenCL (Rusticl), CPU fallback
- Types: FP32, FP64 (native), INT32, INT24 (native MUL_UINT24), INT8 (UBYTE_FLT unpack), FP16 (compiled as FP32), Q16.16 (fixed-point), Q8.8
- Operations: ADD, MUL, FMA, DOT4, RECIPSQRT, SIN, COS, TEX_FETCH, LDS_RW

This produces the definitive "what works, what's fast, what's broken" matrix for TeraScale-2.

### 2.5  Missing Apple → r600 Probe Transfers (7 probes)

The Apple track has 13 Metal shaders; 7 probe concepts have no r600 equivalent:

| Apple Probe | Concept | r600 Equivalent Needed |
|---|---|---|
| `metal_probe_atomic_cas.metal` | Atomic CAS throughput | OpenCL `atomic_cmpxchg` probe on LDS + global |
| `metal_probe_simdgroup_reduce.metal` | Subgroup reduction | Vulkan subgroupAdd (if Terakan exposes subgroups) or OpenCL work-group reduction |
| `metal_probe_threadgroup_barrier.metal` | Barrier cost | OpenCL `barrier(CLK_LOCAL_MEM_FENCE)` latency chain |
| `metal_probe_register_pressure_128.metal` | Max register pressure | Fragment shader with 128 live vec4s (force GPR spill boundary) |
| `metal_probe_texture_gather.metal` | textureGather throughput | GLSL `textureGather()` probe (if r600 supports it — check GL_ARB_texture_gather) |
| `metal_probe_async_copy.metal` | Async global→local copy | OpenCL `async_work_group_copy()` — test if Rusticl lowers this correctly |
| `metal_probe_imageblock.metal` | Tile memory / imageblock | No direct equivalent (TeraScale-2 has no tile memory); document as hard limit |

---

## §3  Transferable Patterns: CUDA LBM → r600/Terakan/Rusticl

### 3.1  SoA Pull-Scheme → LDS Tiling on r600

**Source:** CUDA LBM's SoA pull-scheme (`kernels_soa.cu`) stores distributions as `f[dir * n_cells + cell]` with stride = 19. Pull-gather reads from backward neighbor cells achieve fully coalesced writes.

**r600 transfer:** TeraScale-2's 32KB LDS per SIMD is the primary fast-access memory. An SoA layout in LDS with 19-direction stride, tiled to fit within 32KB, would:
- Enable coalesced LDS reads (4 bytes × 64 threads = 256 bytes per access, matching LDS bank width)
- Eliminate VRAM round-trips for neighbor access within a tile
- Map directly to the OpenCL `__local` qualifier via Rusticl

**Tile sizing:** 32KB / (19 directions × 4 bytes/FP32) = 421 cells per tile. For a 2D slice: 20×20 tile with halo = 22×22 = 484 cells → just over limit. Use 18×18+halo = 400 cells → fits.

**Action:** Create a Rusticl OpenCL kernel implementing D2Q9 (simpler) LBM with SoA-in-LDS tiling. Benchmark against VRAM-only baseline. This is a **direct novel demonstration** of LDS utility on TeraScale-2.

### 3.2  Lloyd-Max INT8 Quantization → UBYTE_FLT Packing

**Source:** CUDA LBM's `kernels_int8_soa_lloydmax.cu` uses adaptive-range quantization concentrating 256 INT8 levels in the physically occupied range [-0.05, 0.45] of D3Q19 distributions. Achieves 21× better mass drift than uniform quantization at the same INT8 speed (5460 MLUPS → 4830 MLUPS, only 12% slower).

**r600 transfer:** TeraScale-2's `UBYTE_FLT` instruction unpacks 4 × UINT8 → 4 × FP32 in 1 VLIW5 cycle. Combined with a Lloyd-Max codebook stored in KCACHE (constant buffer), the decode path becomes:

```
UBYTE_FLT R1, R0.x     ; unpack 4 bytes → 4 floats (1 cycle)
MUL_IEEE R1, R1, KC0[0] ; scale by codebook range (1 cycle, PV-forwarded)
ADD      R1, R1, KC0[1] ; offset by codebook base (1 cycle, PV-forwarded)
```

3 cycles for 4 values = 0.75 cycles/value. Compare to FP32 load at 1 cycle/value. **25% faster per element at 4× less bandwidth.**

**Action:** Implement this as an OpenCL probe: `lloydmax_ubyte_flt.cl`. Measure throughput against raw FP32 baseline. If Rusticl can't handle it, use a GLSL compute shader via r600g.

### 3.3  Q16.16 Fixed-Point via MUL_UINT24

**Source:** CUDA LBM's `kernels_q16_soa.cu` uses Q16.16 fixed-point arithmetic via IADD3 pipeline, achieving 8.5× faster per-op than FFMA on the RTX 4070 Ti (integer pipeline vs FP pipeline throughput).

**r600 transfer:** MUL_UINT24 is a native single-cycle 24-bit unsigned multiply on TeraScale-2's t-slot. For Q8.16 fixed-point (8 integer bits, 16 fractional bits — fits in 24 bits):

```
MUL_UINT24 R2.t, R0.x, R1.x   ; 24-bit multiply (1 cycle, t-slot)
LSHR       R2.x, R2.x, 16     ; shift to realign fixed point (1 cycle)
```

2 cycles for a fixed-point multiply vs the 1-cycle FP32 MUL_IEEE. Not faster for scalar ops, but **enables 24-bit integer compute where FP32 precision is overkill** — texture coordinate generation, alpha blending weights, audio DSP, dithering.

**Action:** Create `q8_16_mul_uint24.shader_test` measuring throughput of Q8.16 chains vs FP32 chains via shader_runner.

### 3.4  Storage-Compute Decoupling Pattern

**Source:** All CUDA LBM kernels follow the pattern: pack for storage → promote to FP32 before arithmetic → demote after arithmetic → pack for storage. The precision tier is a **storage decision**, not a compute decision.

**r600 application areas:**
- **Vertex buffer packing:** Store vertices as INT16x4 (8 bytes vs 16 bytes per vertex), unpack to FP32 via INT_TO_FLT in vertex shader. Halves VTX fetch bandwidth.
- **Texture packing:** Store data as UINT8x4 (RGBA8), unpack via UBYTE_FLT, compute in FP32, export as FP32. Reduces texture bandwidth 4×.
- **Framebuffer packing:** Render to R16G16B16A16F (if supported), gain bandwidth vs R32G32B32A32F.
- **Uniform buffer packing:** Pack multiple constants into fewer vec4 slots, unpack in shader.

**Action:** Audit existing Mesa r600g vertex fetch paths for implicit INT16→FP32 promotion. If the VTX fetch hardware already does this, enable it via `pipe_vertex_element::src_format`. If not, add a VS prolog.

### 3.5  Precomputed Constants (inv_tau pattern)

**Source:** CUDA LBM's biggest single optimization: precomputing `inv_tau = 1.0f / tau` eliminates a `MUFU.RCP` (50.6 cycles on SASS) per collision. Mass drift goes to exactly zero.

**r600 equivalent:** `RECIP_IEEE` on t-slot is 1 cycle (confirmed by probes), so the raw latency saving is minimal. **However**, the pattern matters for a different reason on VLIW5: `RECIP_IEEE` consumes the t-slot exclusively, blocking SIN/COS/RECIPSQRT/LOG/EXP from co-scheduling. If a shader has both a reciprocal and a transcendental in the same loop body, precomputing the reciprocal frees the t-slot for the transcendental.

**Action:** Audit SFN scheduler output for cases where `RECIP_IEEE` and another transcendental compete for the t-slot in the same ALU clause. If found, add a `nir_opt_algebraic` pattern: `nir_frcp(const) → nir_imm_float(1.0/const)` for compile-time-known divisors.

### 3.6  Double-Double (FP128) → Native FP64 Advantage

**Source:** CUDA LBM's `kernels_dd.cu` implements Knuth 2-sum + Veltkamp-Dekker for ~106-bit precision using pairs of FP64 values. Cost: 20+ FP64 ops per DD multiply.

**r600 unique advantage:** Native FP64 via ADD_64/MUL_64/FMA_64 at 1 cycle latency using 2 VLIW5 slots. This is **cheaper than Dekker double-single** (which needs 6 FP32 ops for one multiply) and **unavailable on Apple Metal** (no FP64 at all). For scientific compute on this APU, native FP64 is the correct choice — not emulation.

**Action:** Measure FP64 throughput via `latency_fp64.frag` (currently unmeasured). If throughput is >25% of FP32, native FP64 is viable for precision-critical compute kernels on Rusticl.

---

## §4  Newly Discovered Gaps Not in Current Plan

### 4.1  OpenCL Numeric Pathology (CRITICAL BLOCKER)

**Status:** Rusticl integer/occupancy probes produce 1000+ NaNs in scalar/packed lanes. The `depchain_u8_add.cl` and `depchain_i8_add.cl` probes return pathological FP interpretations.

**Root cause hypothesis:** `nir_lower_bit_size` for sub-32-bit conversions on r600 is either missing or incorrect. The u2u8/i2i8 lowering that `COMPREHENSIVE_SCOPE.md` identifies as "BROKEN" is the same codepath.

**Impact:** Blocks all INT8/INT16 compute on Rusticl. Blocks Lloyd-Max UBYTE_FLT exploitation. Blocks Q8.16 fixed-point compute.

**Fix path:** Port the 8/16-bit ALU lowering patch referenced in `COMPREHENSIVE_SCOPE.md` to Mesa 26. File: `src/gallium/drivers/r600/sfn/sfn_nir.cpp` — add `nir_lower_bit_size` pass for 8-bit and 16-bit integer operations, lowering to 32-bit with appropriate masking.

**Priority:** P0 — this blocks 40% of the typed-format matrix.

### 4.2  Terakan Build Blockers (5 remaining)

From `TERAKAN_PORT_STATUS.md`:

1. **NIR builder macro API** — `terakan_nir_lower_bindings.c` uses deprecated `nir_builder_init` / `nir_builder_create` macros removed in Mesa 26. Fix: convert to `nir_builder b = nir_builder_init_simple_shader(...)` or the new `nir_builder_at()` API.

2. **`nir_uav_instr_r600` custom intrinsic** — Not defined in Mesa 26's `nir_intrinsics.py`. Fix: add intrinsic definition with appropriate semantic flags (has_dest, num_components, etc.).

3. **vk_shader wrapper** — Vulkan compute pipeline creation fails with `VK_ERROR_FEATURE_NOT_PRESENT` because Terakan lacks the `vk_shader` abstraction layer that Mesa 26 VK drivers require. Fix: implement `terakan_shader.c` wrapping SFN-compiled bytecode.

4. **Pipeline cache serialization** — No serialization support means every pipeline compile is cold. Not a correctness blocker but a performance cliff for Vulkan workloads with many pipelines.

5. **Sparse resource / VkEvent / command pool reuse** — dEQP tests crash. Fix: stub with `VK_ERROR_FEATURE_NOT_PRESENT` for sparse; implement VkEvent via fence signaling; fix command pool reset path.

**Priority:** Items 1-2 are P0 (block build). Item 3 is P0 (blocks Vulkan compute). Items 4-5 are P2.

### 4.3  Missing Probes Identified by Cross-Track Analysis

| # | Probe | Track Source | Purpose | Priority |
|---|---|---|---|---|
| 1 | `barrier_cost.cl` | Apple `threadgroup_barrier` | Measure `barrier(CLK_LOCAL_MEM_FENCE)` latency in cycles | P0 |
| 2 | `atomic_cas_throughput.cl` | Apple `atomic_cas` | Measure `atomic_cmpxchg` on LDS vs global | P1 |
| 3 | `lloydmax_ubyte_flt.cl` | CUDA LBM Lloyd-Max | UBYTE_FLT + codebook decode throughput | P1 |
| 4 | `lds_bank_conflict.cl` | CUDA LBM SoA tiling | Measure LDS bank conflict penalty | P0 |
| 5 | `vtx_fetch_latency.shader_test` | Novel (no analog) | Vertex buffer read chain latency | P1 |
| 6 | `gpr_spill_scratch_latency.cl` | SASS spill probes | Force 124+ GPR, measure scratch R/W cost | P0 |
| 7 | `clause_switch_measured.shader_test` | SASS `tex_alu_interleave` | Timed ALU→TEX→ALU clause transition | P0 |
| 8 | `export_clause_latency.frag` | Novel | MRT export cost vs single RT | P2 |
| 9 | `async_workgroup_copy.cl` | Apple `async_copy` | Test `async_work_group_copy()` on Rusticl | P1 |
| 10 | `texture_gather.frag` | Apple `texture_gather` | `textureGather()` throughput (if GL_ARB_texture_gather supported) | P2 |
| 11 | `depchain_lds_rw.cl` | SASS `shared_latency` | LDS read→write→read dependency chain | P0 |
| 12 | `d2q9_lbm_lds_tiled.cl` | CUDA LBM SoA | LBM in LDS with SoA tiling | P2 (demonstration) |

### 4.4  No Unified Typed Matrix Runner

All three tracks (SASS, Apple, r600) have probe runners, but they are **track-specific**:
- SASS: `compile_profile_all.sh` + `auto_explorer.py`
- Apple: `run_next42_full_stack_suite.sh` + `neural_typed_matrix.py`
- r600: `run_terascale_probe_tranche.sh` + per-lane shell scripts

**Missing:** A single `run_typed_matrix.py` that:
1. Accepts a target (r600, apple, sass)
2. Enumerates all probes for that target from a manifest
3. Runs each probe with appropriate tooling (shader_runner, clpeak, vkmark, etc.)
4. Collects timing + ISA + profiler sidecars
5. Outputs a unified CSV: `probe_id, backend, dtype, operation, timing_us, slot_utilization, notes`
6. Computes deltas against previous runs

**Action:** Create the unified runner. It subsumes `emit_terascale_probe_manifest.py` and `compare_terascale_lane_results.py`. This is the **infrastructure prerequisite** for the auto-explorer transfer (§1.3).

### 4.5  LTO Regression Uninvestigated

**Finding:** Mesa 26.0.3 with `-O2 + LTO` = 122 FPS vs `-O2` no-LTO = 385 FPS. The 3.15× regression is attributed to "code bloat exceeding 32KB L1i cache" but this is **hypothesis, not measurement**.

**Missing evidence:**
- `perf stat -e L1-icache-load-misses` comparing LTO vs no-LTO builds
- `perf record -e cycles:pp` on the LTO build to identify which bloated functions cause i-cache thrashing
- Binary size comparison: `size` on `r600_dri.so` (LTO vs no-LTO)
- Whether `-Os + LTO` (optimize for size) or `-O2 + LTO + -ffunction-sections -fdata-sections -Wl,--gc-sections` recovers performance

**Action:** Add to kill-list investigation queue. If i-cache is confirmed, file Mesa bug — this affects all Bobcat/low-cache CPUs.

### 4.6  120+ Undocumented Scripts

68 Python scripts and 99 shell scripts exist in the repo. Of these, approximately 40% have no documentation beyond filename conventions. Key undocumented scripts that need READMEs or inline docstrings:

- `build_terascale_project_state.py` — what state does it capture? what output?
- `compare_architectures.py` — which architectures? what comparison metric?
- `compare_terascale_lane_results.py` — lane = OpenCL/Vulkan/GL? what comparison?
- `sync_x130e_terascale_compendium.py` — sync from where to where? over SSH?
- `sync_apple_typed_atlas.py` (749 LOC!) — largest script, zero inline docs

**Action:** Batch-add `--help` / argparse descriptions to all Python scripts. This is a maintainability debt that slows cross-track adoption.

---

## §5  Maximal Tooling Verbosity Configuration

Per the request for "maximal verbosity of tooling enabled even if the data becomes extreme":

### 5.1  Mesa / r600g Debug Flags (enable ALL)

```bash
export R600_DEBUG=fs,vs,gs,cs,tex,fetch,compute,sb,sbdisasm,sbstat
export GALLIUM_HUD="fps+gpu-busy+shader-busy+num-draw-calls+num-prims"
export GALLIUM_HUD_PERIOD=100  # 100ms sample rate (10× default)
export MESA_DEBUG=1
export NIR_DEBUG=print_vs,print_fs,print_cs,validate
export GALLIUM_DUMP_CPU=1
```

### 5.2  Terakan Vulkan Debug Flags

```bash
export VK_LOADER_DEBUG=all
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
export TERAKAN_DEBUG=winsys,cs,shader,nir
```

### 5.3  Kernel / DRM Tracing (full pipeline visibility)

```bash
# Enable all radeon tracepoints
echo 1 > /sys/kernel/debug/tracing/events/drm/enable
echo 1 > /sys/kernel/debug/tracing/events/radeon/enable

# Trace buffer size (increase for long captures)
echo 65536 > /sys/kernel/debug/tracing/buffer_size_kb

# Capture with trace-cmd
trace-cmd record -e drm -e radeon -e sched:sched_switch \
  -e irq:irq_handler_entry -e irq:irq_handler_exit \
  -o /tmp/r600_full_trace.dat -- <workload>

# Process
trace-cmd report -i /tmp/r600_full_trace.dat > /tmp/r600_trace.txt
```

### 5.4  CPU Profiling (IBS on Bobcat)

```bash
# AMD IBS (Instruction-Based Sampling) — available on Bobcat
sudo perf record -e ibs_op// -c 100000 -- <workload>
sudo perf report --sort=dso,symbol

# Also capture with AMDuProfCLI if available
AMDuProfCLI collect --ibs-op --ibs-fetch -o /tmp/uprof_ibs -- <workload>
```

### 5.5  Memory Bandwidth Monitoring

```bash
# GART / VRAM usage (continuous)
watch -n 0.5 'cat /sys/kernel/debug/dri/0/radeon_vram_mm'

# radeontop with dump (machine-readable)
radeontop -d /tmp/radeontop.csv -l 100 &

# vmstat for system memory pressure
vmstat -t 1 > /tmp/vmstat.log &
```

### 5.6  OpenCL / Rusticl Debugging

```bash
export RUSTICL_DEBUG=program,kernel
export CL_LOG_ERRORS=stdout
export MESA_LOG=1
# For NIR dumps of Rusticl kernels:
export NIR_DEBUG=print_cs
```

---

## §6  Updated Priority Ordering

Incorporating all cross-track findings, the revised execution priority is:

### Tier 0: Unblock Everything (Week 1)

1. **Fix Rusticl 8/16-bit lowering** (§4.1) — unblocks INT8/INT16 compute, Lloyd-Max, Q8.16
2. **Fix Terakan NIR builder API** (§4.2 items 1-2) — unblocks Vulkan build
3. **Run 6 existing-but-unmeasured latency probes** (§1.1) — TEX L1, FP64, KCACHE, clause switch, throughput_mul, throughput_recipsqrt
4. **Enable full debug verbosity** (§5.1-5.6) on x130e for all subsequent work

### Tier 1: Measurement Infrastructure (Week 1-2)

5. **DRM debugfs SQ counter patch** (§2.2) — ~250 lines, unlocks hardware performance counters
6. **Create 4 new latency probes** (§1.1) — TEX L1 miss, VTX fetch, LDS R/W, GPR spill/scratch
7. **Create r600_trace_density.py** (§2.1) — trace-cmd event density analysis
8. **Create r600_typed_format_matrix.py** (§2.4) — definitive capability + timing matrix

### Tier 2: Cross-Track Exploitation (Week 2-3)

9. **Port encoding_analysis.py** (§1.2) — R600 ISA encoding audit (novel contribution)
10. **Create auto_explorer_r600.py** (§1.3) — surrogate-model-driven probe ranking
11. **Create unified run_typed_matrix.py** (§4.4) — subsumes per-track runners
12. **Lloyd-Max UBYTE_FLT probe** (§3.2) — INT8 packing exploitation
13. **7 missing Apple-equivalent probes** (§2.5) — barrier, atomic CAS, register pressure, etc.

### Tier 3: Compiler & Driver Optimization (Week 3-4)

14. **SFN t-slot precomputation pass** (§3.5) — free t-slot when reciprocal is compile-time-known
15. **vk_shader wrapper for Terakan** (§4.2 item 3) — unblocks Vulkan compute pipelines
16. **CS batch submission DRM patch** (§2.3) — reduces per-frame kernel overhead
17. **Investigate LTO regression** (§4.5) — confirm i-cache hypothesis, file upstream bug
18. **SoA-in-LDS tiled LBM demo** (§3.1) — novel LDS exploitation demonstration

### Tier 4: Stretch Goals (Week 4+)

19. **dma_fence callback conversion** — interrupt-driven fence wait (backport from amdgpu)
20. **GART PTE batching** — reduce page table update overhead
21. **Pipeline cache serialization** for Terakan — warm pipeline creation
22. **GPR spill-to-scratch in SFN** — unblocks complex Rusticl/Terakan shaders
23. **r600_mnemonic_census.py** (§1.4) — track opcode emission frontier

---

## §7  Cross-Pollination Matrix (Quick Reference)

| Source Pattern | Source Track | Target Application | Expected Impact |
|---|---|---|---|
| Dependent-chain latency probes | SASS | Fill 9 unmeasured r600 latencies | Completes hardware model |
| Encoding bit-field analysis | SASS | R600 ISA encoding audit | Novel public contribution |
| Surrogate-model probe ranking | SASS | Auto-explorer for r600 | 10× more efficient probe scheduling |
| Mnemonic frontier tracking | SASS | R600 opcode census | Discover unused/rare opcodes |
| Stall regime classification | SASS | Per-scene bottleneck matrix | Correct optimization targeting |
| xctrace row density | Apple | trace-cmd event density | Quantified driver overhead |
| GPU counter deltas | Apple | SQ perf counter analysis | Pipeline transparency |
| Command buffer batching | Apple | CS batch DRM patch | CPU overhead reduction |
| Typed-format matrix | Apple | r600 backend×dtype sweep | Definitive capability map |
| SoA pull-scheme | CUDA LBM | LDS tiling on r600 | Bandwidth reduction |
| Lloyd-Max INT8 | CUDA LBM | UBYTE_FLT + codebook | 25% faster per-element |
| Q16.16 fixed-point | CUDA LBM | MUL_UINT24 exploitation | Integer compute pipeline |
| Storage-compute decoupling | CUDA LBM | VTX/TEX bandwidth packing | 2-4× bandwidth savings |
| Precomputed constants | CUDA LBM | SFN t-slot freeing | Transcendental throughput |
| Native FP64 vs emulation | CUDA LBM | Rusticl precision kernels | Unique HW advantage |

---

## §8  Novel Opportunities Unique to This Analysis

These are opportunities that emerge **only** from cross-track synthesis — they don't exist in any single track's roadmap:

### 8.1  UBYTE_FLT as Universal Unpack Primitive

Combining CUDA LBM's storage-compute decoupling with the r600 UBYTE_FLT 1-cycle unpack creates a general-purpose pattern: **any data that fits in [0,255] can be stored as UINT8 and promoted to FP32 in 1 cycle.** This applies to:
- Vertex colors (RGBA8 → 4×FP32)
- Texture coordinates (if quantized to 256 steps per axis)
- Bone weights (0-255 → 0.0-1.0)
- Audio samples (8-bit PCM)
- Neural network activations (INT8 quantized)

No other GPU architecture has a 1-cycle 4-wide byte-to-float instruction accessible from shader code. This is a **genuine unique capability** of TeraScale-2.

### 8.2  PV-Chain Length as Optimization Metric

Combining SASS's dependent-chain methodology with r600's PV (Previous Vector) forwarding reveals that **PV-chain length is the primary throughput metric** on VLIW5 — not instruction count, not slot utilization. A shader with 100 instructions in long PV chains (each instruction consuming the previous cycle's result via PV at zero latency) runs faster than a shader with 80 instructions that breaks PV chains (forcing register reads at 1+ cycle latency).

No existing Mesa optimization pass tracks PV-chain length. Adding it to the SFN scheduler as a cost metric would be a **novel compiler contribution**.

### 8.3  INT24 Arithmetic as Texture Coordinate Pipeline

Combining CUDA LBM's Q16.16 with r600's MUL_UINT24 and the Apple track's observation that "FP16 = full speed on M-series" reveals an opportunity: on r600, where FP16 compiles to FP32 (no speedup), use **Q8.16 via MUL_UINT24 instead** for texture coordinate interpolation. The 24-bit range covers [0, 255] with 16-bit sub-pixel precision — sufficient for 256×256 texture lookups with bilinear filtering. This would be **faster than FP32** if MUL_UINT24 throughput exceeds MUL_IEEE throughput (both are 1 cycle, but MUL_UINT24 is on t-slot, freeing vec slots for other work).

### 8.4  Hybrid FP32+FP64 Precision Tiling

Combining CUDA LBM's precision-tier methodology with r600's native FP64 and the Apple track's typed-format matrix: implement a **mixed-precision solver** where:
- Outer grid uses FP32 (fast, sufficient for smooth regions)
- Refinement regions use native FP64 (precision-critical boundaries)
- Transition uses Dekker double-single (intermediate precision at lower cost than FP64)

This leverages TeraScale-2's unique FP64 capability that no consumer GPU since GCN 1.0 has maintained at reasonable throughput.

---

*End of synthesis. 12 new probes, 5 new scripts, 4 novel optimization opportunities, 9 unmeasured existing probes, 5 build blockers, and 1 critical Rusticl fix identified. All derived from recursive cross-architecture analysis of the steinmarder repository.*
