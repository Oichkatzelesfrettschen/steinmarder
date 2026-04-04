# Data Types & Compute Architecture — AMD Radeon HD 6310 (TeraScale-2 / Evergreen ISA)

**GPU**: AMD Radeon HD 6310 (Wrestler die, device 0x9802)  
**Architecture**: TeraScale-2 / VLIW5 / Evergreen ISA  
**Sources**: HW_CAPABILITIES.md, NUMERIC_PACKING_RESEARCH.md, ISA_SLOT_ANALYSIS.md, LATENCY_PROBE_RESULTS.md, NOVEL_VLIW_PACKING.md  
**Date**: 2026-04-03

---

## 1. Data Types

### 1.1 Natively Supported (dedicated hardware instructions)

| Type | Width | Key Opcodes | Notes |
|------|-------|-------------|-------|
| **FP32** | 32-bit | `MUL_IEEE`, `MULADD_IEEE`, `ADD`, `DOT4_IEEE`, `MIN`, `MAX`, `FRACT`, `TRUNC` | Full IEEE 754 single precision; primary working type |
| **INT32** | 32-bit | `ADD_INT`, `SUB_INT`, `AND_INT`, `OR_INT`, `XOR_INT`, `BFE_INT`, `BFI_INT`, `LSHL/LSHR/ASHR_INT` | Full integer arithmetic + extensive bit manipulation |
| **UINT24** | 24-bit | `MUL_UINT24`, `MULHI_UINT24`, `MULADD_UINT24` | Single-cycle 24-bit multiply; runs on any x/y/z/w slot |
| **FP64** | 64-bit | `ADD_64`, `MUL_64`, `FMA_64`, `MULADD_64`, `RECIP_64`, `RECIPSQRT_64`, `SQRT_64`, `FRACT_64`, `FREXP_64`, `LDEXP_64` | Dual-slot — consumes two consecutive ALU slots per op; full IEEE 754 double |
| **BOOL / predicate** | 1-bit effective | `PRED_SETE/GT/GE/NE`, `PRED_SETE_INT/GT/GE/NE`, `PRED_SET_PUSH/POP/CLR/RESTORE/INV` | Hardware predicate register; drives per-wavefront conditional execution |

FP64 comparisons: `SETE_64`, `SETGT_64`, `SETGE_64`, `SETNE_64`, `PRED_SETE_64`, `PRED_SETGT_64`, `PRED_SETGE_64`, `CNDNE_64`  
FP64 conversion: `FLT32_TO_FLT64`, `FLT64_TO_FLT32`  
FP64 scaling variants: `MULADD_64_M2`, `MULADD_64_M4`, `MULADD_64_D2`

### 1.2 Emulated / Lowered

| Type | Mechanism | Cost | Notes |
|------|-----------|------|-------|
| **FP16** | No FP16 arithmetic. Native `FLT16_TO_FLT32` / `FLT32_TO_FLT16` conversion instructions exist. 2×FP16 packed into one INT32. | Unpack: 2 cycles. Pack: 4 cycles (with normalize). | Mesa uses `r600_nir_lower_pack_unpack_2x16` |
| **INT8** | Lowered to INT32 via `nir_lower_bit_size`. Packed 4×INT8 per INT32 register. | Unpack: **1 cycle** via `UBYTE0–3_FLT` or `BFE_UINT`. Pack: 2–3 cycles. | See §3 for UBYTE_FLT detail |
| **INT16** | Lowered to INT32. Packed 2×INT16 per INT32. | 1 cycle pack/unpack with `BFE_UINT` | |
| **INT4** | Extracted via `BFE_UINT(src, offset, 4)`. 8 nibbles per INT32. | 2 cycles (4 BFE per cycle on x/y/z/w) | |
| **Dekker FP48** | 2×FP32 representing a 48-bit significand (Dekker Double-Single). | Addition: 3 cycles. Multiply: 10–15 cycles. | Alternative to native FP64 when VLIW5 slot interleaving is more important than raw throughput |
| **Q-format fixed-point** | INT32 with implicit binary point. Q16.16, Q8.8, Q4.4, Q32.32 all supported via existing int ops. | Q8.8 mul: 2 cycles via `MUL_UINT24` + `LSHR`. Q32.32 add: 2 cycles via `ADD_INT` + `ADDC_UINT`. | |

### 1.3 Unique / Architecturally Notable

#### UINT24 — the hidden fast multiply path

`MUL_UINT24`, `MULHI_UINT24`, and `MULADD_UINT24` are **single-cycle** 24-bit integer multiplies that run on any of the four general vec slots (x/y/z/w). A full VLIW5 bundle can issue four UINT24 multiplies simultaneously. This is distinct from the full 32-bit `MULLO_INT` which is restricted to the t-slot and takes multiple cycles.

Primary use cases: Q8.8 fixed-point color math, 3-component RGB×factor (8-bit values fit in 24 bits), array index arithmetic.

#### FP64 native presence

The Evergreen ISA has full hardware FP64 — a discovery confirmed by opcode analysis. `ADD_64`, `MUL_64`, `FMA_64` consume two consecutive ALU slots per operation (x+y pair or z+w pair), yielding ~2.5 GFLOPS peak (half FP32 rate). All transcendentals have FP64 variants (`RECIP_64`, `RECIPSQRT_64`, `SQRT_64`) on the t-slot.

Mesa's `sfn_nir_lower_64bit.cpp` handles the lowering pass; `lower_int64_options = ~0` is set in nir_options.

#### UBYTE0–3_FLT — byte-to-float in one cycle

Four opcodes (`UBYTE0_FLT` through `UBYTE3_FLT`) each extract one byte from a 32-bit register and convert it directly to float, all in **one cycle** with no AND/shift required. All four fit in a single VLIW5 bundle, unpacking packed RGBA8 to four float channels in 1 cycle — 3× faster than the traditional AND+SHIFT+`INT_TO_FLT` path.

#### FLT16_TO_FLT32 / FLT32_TO_FLT16 — native half-float conversion

Native single-cycle conversion between FP16 and FP32 in hardware. Four conversions per cycle across x/y/z/w slots. FP16 cannot be computed on (no FP16 ALU), but packing/unpacking is native.

#### BFE / BFI — 3-operand bit-field extract and insert

`BFE_UINT(src, offset, width)` and `BFI_INT(src, insert, width)` are 3-operand bit-field instructions, each single-cycle on any vec slot. These cover arbitrary sub-word widths: INT4, INT6, INT10, INT12, INT24, any packing layout.

#### FP32 + UINT24 hybrid (56-bit effective range)

A documented novel format: FP32 carries the 24-bit significand and full exponent range; an accompanying UINT24 extends precision by another 24 bits. Together they yield 48 effective mantissa bits with the FP32 exponent range in one VLIW5 bundle (x: FP32 op, z: UINT24 op). Not IEEE-compliant; useful for extended-precision accumulation.

---

## 2. ALU Structure

The GPU has **2 identical SIMD engines**. Each contains one VLIW5 ALU array issuing 5 operations simultaneously per clock cycle. Peak: 2 × 5 × 487 MHz = **~4.87 GFLOPS** (FP32 MAD).

```
HD 6310 GPU
├── SIMD Engine 0
│     ├── VLIW5 ALU (5 ops/cycle)
│     │     ├── Slot x  — general FP32 / INT32 / UINT24
│     │     ├── Slot y  — general FP32 / INT32 / UINT24
│     │     ├── Slot z  — general FP32 / INT32 / UINT24
│     │     ├── Slot w  — general FP32 / INT32 / UINT24
│     │     └── Slot t  — transcendental (+ general overflow)
│     ├── TEX unit — texture sampling
│     ├── VTX unit — vertex buffer fetch
│     └── LDS — 32 KB local data share
└── SIMD Engine 1
      └── (identical)
```

### 2.1 Slots x / y / z / w — general-purpose (4 per SIMD)

All four are functionally identical. Supported operations:

**FP32 arithmetic**
- `ADD`, `MUL_IEEE`, `MULADD_IEEE` (FMA)
- `MULADD_M2`, `MULADD_M4`, `MULADD_D2` — implicit ×2, ×4, ÷2 scaling at no extra cost
- `MUL_PREV`, `ADD_PREV`, `MULADD_PREV`, `MULADD_IEEE_PREV` — read Previous Vector (PV) with zero-latency forwarding; no GPR read required
- `MIN`, `MAX`, `MIN_DX10`, `MAX_DX10`
- `FRACT`, `TRUNC`, `CEIL`, `FLOOR`, `RNDNE` (round to nearest even)
- `DOT4`, `DOT4_IEEE` — occupies all four x/y/z/w simultaneously (full vec4 dot in 1 cycle)
- `MAX4` — 4-way maximum of 4 values in 1 cycle (vs 3 cycles with chained MAX)
- `MUL_LIT` — hardware lighting lit function
- `CUBE` — cube-map face/coordinate conversion

**INT32 arithmetic**
- `ADD_INT`, `SUB_INT`, `MAX_INT`, `MIN_INT`, `MAX_UINT`, `MIN_UINT`
- `ADDC_UINT` (add with carry), `SUBB_UINT` (subtract with borrow) — enables exact 64-bit integer arithmetic
- `MULLO_INT`, `MULLO_UINT`, `MULHI_INT`, `MULHI_UINT` (NOTE: these are t-slot only — see §2.2)

**UINT24 arithmetic**
- `MUL_UINT24`, `MULHI_UINT24`, `MULADD_UINT24` — single-cycle; all four slots can run independently

**Bit manipulation**
- `AND_INT`, `OR_INT`, `XOR_INT`, `NOT_INT`
- `LSHL_INT`, `LSHR_INT`, `ASHR_INT`
- `BFE_INT`, `BFE_UINT` — 3-operand bit-field extract
- `BFI_INT` — 3-operand bit-field insert
- `BIT_ALIGN_INT` — `(a << (32-shift)) | (b >> shift)` in one op
- `BYTE_ALIGN_INT` — byte-granularity version
- `BFREV_INT` — bit reversal
- `BCNT_INT` — population count (popcount)
- `FFBH_INT`, `FFBH_UINT` — find first bit high (leading zero count)
- `FFBL_INT` — find first bit low (trailing zero count)
- `SAD_ACCUM_UINT`, `SAD_ACCUM_HI_UINT` — sum of absolute differences (video motion estimation, image matching)
- `MBCNT_32HI_INT`, `MBCNT_32LO_ACCUM_PREV_INT` — wavefront-level bit counting

**Conversion**
- `FLT_TO_INT`, `INT_TO_FLT`, `FLT_TO_UINT`, `UINT_TO_FLT`
- `FLT_TO_INT_FLOOR`, `FLT_TO_INT_RPI`
- `FLT_TO_UINT4` — 4-bit quantization
- `UBYTE0_FLT`, `UBYTE1_FLT`, `UBYTE2_FLT`, `UBYTE3_FLT` — byte-to-float extraction
- `FLT16_TO_FLT32`, `FLT32_TO_FLT16` — native half-float conversion
- `OFFSET_TO_FLT`

**Conditional / select**
- `CNDE`, `CNDGT`, `CNDGE` — float ternary select: `result = (cond) ? a : b`
- `CNDE_INT`, `CNDGT_INT`, `CNDGE_INT` — integer ternary select
- `LERP_UINT` — integer linear interpolation
- `SETE`, `SETGT`, `SETGE`, `SETNE` — set-on-condition (result: 1.0 or 0.0)

**Fragment / pixel**
- `KILLGT`, `KILLE`, `KILLGE`, `KILLNE` — discard fragment if condition met

**FP64 (dual-slot)**
- `ADD_64`, `MUL_64`, `FMA_64` — each consumes x+y or z+w pair; leaves 1 free pair + t-slot
- Comparisons: `SETE_64`, `SETGT_64`, `SETGE_64`, `SETNE_64`
- Select: `CNDNE_64`
- Scaling: `MULADD_64_M2`, `MULADD_64_M4`, `MULADD_64_D2`
- Misc: `FRACT_64`, `FREXP_64`, `LDEXP_64`, `MIN_64`, `MAX_64`

**LDS access (compute)**
- `LDS_1A`, `LDS_1A1D`, `LDS_2A` — local data share read/write patterns
- `GROUP_BARRIER` — compute wavefront barrier

### 2.2 Slot t — transcendental (1 per SIMD)

Dedicated transcendental unit executing in parallel with x/y/z/w every cycle. Also handles integer multiply (which cannot run on vec slots).

| Opcode | Operation |
|--------|-----------|
| `SIN` | Sine — 1 cycle hardware, ~8 total instructions per call (range reduction + polynomial) |
| `COS` | Cosine — same as SIN |
| `LOG_IEEE`, `LOG_CLAMPED` | log₂ |
| `EXP_IEEE` | 2^x |
| `RECIP_IEEE`, `RECIP_CLAMPED`, `RECIP_FF` | 1/x |
| `RECIPSQRT_IEEE`, `RECIPSQRT_CLAMPED`, `RECIPSQRT_FF` | 1/√x |
| `SQRT_IEEE` | √x |
| `RECIP_INT`, `RECIP_UINT` | Integer reciprocal |
| `MULLO_INT`, `MULLO_UINT` | 32×32→lower-32 integer multiply |
| `MULHI_INT`, `MULHI_UINT` | 32×32→upper-32 integer multiply |
| FP64 transcendentals | `RECIP_64`, `RECIPSQRT_64`, `RECIPSQRT_CLAMPED_64`, `SQRT_64` |
| General overflow | Any vec-slot op can spill into t when x/y/z/w are full |

### 2.3 VLIW5 bundle layout by operation mix

| Scenario | x | y | z | w | t | Slots used |
|----------|---|---|---|---|---|-----------|
| 4 independent FP32 MAD | MAD | MAD | MAD | MAD | free | 4 |
| 4 MAD + 1 RSQ | MAD | MAD | MAD | MAD | RSQ | 5 (full) |
| DOT4 | DOT | DOT | DOT | DOT | free | 4 (inseparable) |
| 1 FP64 ADD + 2 FP32 MAD | ADD_64(hi) | ADD_64(lo) | MAD | MAD | free | 4 |
| 2 FP64 ADD + 1 RSQ | ADD_64 | ADD_64 | ADD_64 | ADD_64 | RSQ | 5 (full) |
| 4 UBYTE_FLT unpack | UB0 | UB1 | UB2 | UB3 | free | 4 |
| 4 BFE extract + RSQ | BFE | BFE | BFE | BFE | RSQ | 5 (full) |
| 4 UINT24 multiply | MUL24 | MUL24 | MUL24 | MUL24 | free | 4 |

### 2.4 PV forwarding — the 1-cycle latency mechanism

The **Previous Vector (PV)** register holds the output of the immediately preceding VLIW5 bundle. Any slot can read `PV.x/y/z/w` with zero additional latency — the result is available the very next cycle. All vec-slot ops (MUL, MULADD, ADD, DOT4) and all t-slot ops (RSQ, MULLO_INT) exhibit **1-cycle latency** via this mechanism, as verified by dependent-chain probes.

`PS` (Previous Scalar) holds the t-slot result from the prior bundle for analogous forwarding.

The `_PREV` op variants (`MUL_PREV`, `ADD_PREV`, `MULADD_PREV`) encode PV reads implicitly, eliminating a GPR operand slot and reducing register pressure.

---

## 3. Measured Latencies (dependent-chain probe results)

| Opcode | Slot | Latency | Throughput | Notes |
|--------|------|---------|------------|-------|
| `MUL_IEEE` | vec | **1 cycle** | 4/cycle | PV forwarding confirmed via 64-deep chain |
| `MULADD_IEEE` | vec | **1 cycle** | 4/cycle | Same pipeline as MUL |
| `ADD` | vec | **1 cycle** (inferred) | 4/cycle | NIR constant-folds chains; same pipeline |
| `DOT4_IEEE` | vec (all 4) | **1 cycle** | 1/cycle | Full vec4 result in PV.x next cycle |
| `RECIPSQRT_IEEE` | t | **1 cycle** | 1/cycle | t-slot is the throughput bottleneck |
| `SIN` | t | **1 cycle** hardware | 1/cycle | But ~8 total instructions per call |
| `COS` | t | **1 cycle** hardware | 1/cycle | Same as SIN |
| `MULLO_INT` | t | **1 cycle** | 1/cycle | PV forwarding works on t-slot too |
| `ADD_64` / `MUL_64` | vec×2 | ~1 cycle (not yet measured) | 2/cycle (2 FP64 ops per bundle) | Uses 2 slots; t-slot still free |

FP64 latency probes are in the pending measurement list — see LATENCY_PROBE_RESULTS.md §Missing Measurements.

---

## 4. Other Compute Blocks

### TEX unit

One per SIMD. Issues texture sample instructions (`SAMPLE`, with LOD/bias/gradient/offset variants). Executes as a separate *clause* — the CF dispatcher switches a wavefront to a TEX clause when it needs texture data, then switches to a different wavefront's ALU clause while the memory fetch resolves. Latency: ~100–400 cycles (hidden by wavefront interleaving). Measured usage: 4% of total instructions for geometry-heavy glmark2 workloads; dominant for blur/convolution/shadow-map workloads.

### VTX unit

One per SIMD. Issues vertex fetch instructions (`VFETCH`) with configurable format, stride, offset, and mega-fetch count. Same clause-switching model as TEX. Measured usage: 58% of total instructions for geometry-heavy benchmarks — the dominant bottleneck for those workloads, not ALU.

### LDS (Local Data Share)

32 KB per SIMD. Dedicated on-chip scratchpad for compute workloads. Bandwidth ~150 GB/s (no round-trip to VRAM). Accessed from ALU clauses via `LDS_1A`, `LDS_1A1D`, `LDS_2A` instructions; atomic operations via `LDS_IDX_OP`. Not applicable to graphics shaders (compute-only).

### CF (Control Flow)

Global control fabric dispatching clauses to SIMDs. Instructions: `CALL_FS` (vertex fetch subroutine), `ALU`, `TEX`, `VTX`, `EXPORT`, `EXPORT_DONE`, `LOOP_START/END`, `JUMP`, `ELSE`, `POP`, `RETURN`. The CF layer also manages the VLIW5 bundle encoding: each ALU clause is prefixed by `CF_ALU_WORD0/1`; the LAST bit (bit 31 of `ALU_WORD0`) marks the final instruction in each bundle.

### Export

End-of-pipeline write block. Writes vertex position or fragment color to the downstream fixed-function pipeline. ~10 cycle overhead. `EXPORT_DONE` marks the final write and triggers wavefront retirement.

### GPR register file

128 × vec4 (128-bit) registers per SIMD, shared across all active wavefronts:

| GPRs/shader | Max wavefronts | Occupancy | Latency hiding |
|-------------|---------------|-----------|----------------|
| 1–16 | 8 | 100% | Excellent |
| 17–32 | 4 | 50% | Good |
| 33–64 | 2 | 25% | Limited |
| 65–128 | 1 | 12.5% | Poor |

Counter-intuitive note from profiling: with the GPU at 62–75% idle due to CPU-side command submission bottlenecks (`radeon_emit`, DRM spinlock), maximizing wavefront occupancy is less important than reducing draw-call overhead. Using more GPRs to consolidate work into fewer draws can be net positive in this regime.

---

## 5. GPU Pipeline Utilization Summary (radeontop, glmark2 on-screen)

| Block | Utilization | Interpretation |
|-------|------------|----------------|
| gpu | 100% | Overall busy |
| ee (execution engine) | 100% | Shader cores fully loaded |
| ta (texture address) | 100% | Texture unit saturated |
| sh (shader) | 100% | VLIW5 at max dispatch |
| spi (shader processor input) | 100% | Shader dispatch full |
| sc (scan converter) | 100% | Rasterizer pegged |
| db (depth buffer) | 100% | Depth test active |
| cb (color buffer) | 100% | Color write active |
| vgt (vertex grouper) | ~0% | Not geometry-bound |
| pa (primitive assembly) | ~0% | Not vertex-bound |
| vram | 22% (83/384 MB) | Memory pressure minimal |

The workload is **ALU + texture-bound, not memory or geometry bound**. Improving VLIW5 slot utilization (currently 64–74%) would directly increase FPS since the GPU is fully saturated on shader execution.

---

## 6. ISA Opcode Coverage

426 opcodes verified across all clause types:

| Category | Count | Examples |
|----------|-------|---------|
| ALU (vec + trans) | 198 | MUL_IEEE, MULADD_IEEE, DOT4, BFE, BFI, UBYTE_FLT, ADD_64, MUL_UINT24 |
| CF (control flow) | 137 | CALL_FS, ALU, TEX, VTX, EXPORT, LOOP, JUMP |
| LDS (local data share) | 51 | LDS_1A, LDS_1A1D, LDS_IDX_OP |
| RAT / MEM (memory) | 32 | RAT_STORE, RAT_ATOMIC |
| TEX (texture) | 8 | SAMPLE, SAMPLE_L, SAMPLE_G, SET_GRADIENTS |

---

*Synthesized 2026-04-03 from RE toolkit findings, dependent-chain latency probes, and ISA opcode analysis.*  
*Cross-reference: AMD_Evergreen-Family_ISA.pdf (Chapters 3–5), R600_Instruction_Set_Architecture.pdf*  
*Source documents: HW_CAPABILITIES.md, NUMERIC_PACKING_RESEARCH.md, ISA_SLOT_ANALYSIS.md, LATENCY_PROBE_RESULTS.md, NOVEL_VLIW_PACKING.md*
