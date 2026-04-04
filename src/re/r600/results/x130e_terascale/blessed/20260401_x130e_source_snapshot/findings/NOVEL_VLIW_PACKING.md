# Novel VLIW5 Packing Strategies — Evergreen ISA Deep Dive

## ALL Computing Units on HD 6310

### 1. General ALU (x/y/z/w slots) — 4 per SIMD

**Arithmetic (FP32)**:
- ADD, MUL, MUL_IEEE, MULADD (FMA), MULADD_IEEE
- MULADD_M2/M4/D2 — multiply-add with implicit x2, x4, /2 scaling (FREE!)
- MUL_PREV, ADD_PREV, MULADD_PREV — chain with previous result (zero-latency forwarding!)
- MIN, MAX, MIN_DX10, MAX_DX10
- FRACT, TRUNC, CEIL, FLOOR, RNDNE (round to nearest even)
- DOT4, DOT4_IEEE, DOT (dot product — uses multiple slots)

**Arithmetic (INT32)**:
- ADD_INT, SUB_INT, MUL_UINT24(!), MULHI_UINT24(!), MULADD_UINT24(!)
- MULLO_INT, MULLO_UINT, MULHI_INT, MULHI_UINT
- MAX_INT, MIN_INT, MAX_UINT, MIN_UINT
- ADDC_UINT (add with carry!), SUBB_UINT (subtract with borrow!)

**Bit Manipulation**:
- AND_INT, OR_INT, XOR_INT, NOT_INT
- LSHL_INT, LSHR_INT, ASHR_INT
- BFREV_INT (bit reverse!)
- BFE_INT, BFE_UINT (bit-field extract — 3-operand!)
- BFI_INT (bit-field insert — 3-operand!)
- BIT_ALIGN_INT (arbitrary bit alignment!)
- BYTE_ALIGN_INT (byte-level alignment!)
- BCNT_INT (population count!)
- FFBH_INT, FFBH_UINT (find first bit high)
- FFBL_INT (find first bit low)

**Conversion**:
- FLT_TO_INT, INT_TO_FLT, FLT_TO_UINT, UINT_TO_FLT
- FLT16_TO_FLT32, FLT32_TO_FLT16 (NATIVE half-float conversion!)
- UBYTE0_FLT, UBYTE1_FLT, UBYTE2_FLT, UBYTE3_FLT (byte-to-float extraction!)
- FLT_TO_INT_FLOOR, FLT_TO_INT_RPI (round modes)
- FLT_TO_UINT4 (4-bit quantization!)
- OFFSET_TO_FLT

**Conditional**:
- CNDE, CNDGT, CNDGE (float conditional select)
- CNDE_INT, CNDGT_INT, CNDGE_INT (integer conditional select)
- LERP_UINT (integer linear interpolation!)
- MUL_LIT (lit function for lighting!)

**Predicate/Kill**:
- PRED_SETE/GT/GE/NE, PRED_SETE_INT/GT/GE/NE
- PRED_SET_PUSH/POP/CLR/RESTORE/INV
- KILLE/GT/GE/NE (fragment kill)

**Special**:
- CUBE (cube map coordinate conversion!)
- MAX4 (4-way max in one op!)
- GROUP_BARRIER (compute barrier)
- LDS_1A, LDS_1A1D, LDS_2A (LDS access patterns)
- SAD_ACCUM_UINT, SAD_ACCUM_HI_UINT (sum of absolute differences!)
- MBCNT_32HI_INT, MBCNT_32LO_ACCUM_PREV_INT (wavefront bit count!)

### 2. Transcendental (t-slot) — 1 per SIMD

- SIN, COS (trig)
- LOG_IEEE, LOG_CLAMPED (logarithm)
- EXP_IEEE (exponent)
- RECIP_IEEE, RECIP_CLAMPED, RECIP_FF (reciprocal)
- RECIPSQRT_IEEE, RECIPSQRT_CLAMPED, RECIPSQRT_FF (inverse square root)
- SQRT_IEEE (square root)
- RECIP_INT, RECIP_UINT (integer reciprocal!)
- Also: any general ALU op can overflow to t-slot

### 3. 64-bit Operations (dual-slot, Evergreen)

- ADD_64, MUL_64, MIN_64, MAX_64
- FRACT_64, FREXP_64, LDEXP_64, SQRT_64
- RECIP_64, RECIPSQRT_64, RECIP_CLAMPED_64, RECIPSQRT_CLAMPED_64
- SETE_64, SETGT_64, SETGE_64, SETNE_64
- PRED_SETE_64, PRED_SETGT_64, PRED_SETGE_64
- CNDNE_64
- FLT32_TO_FLT64, FLT64_TO_FLT32
- MULADD_64, MULADD_64_M2/M4/D2

NOTE: 64-bit ops use TWO consecutive ALU slots. So a VLIW5 bundle can have
2x 64-bit ops + 1 t-slot, or 1x 64-bit op + 2x 32-bit + 1 t-slot.

### 4. Texture Unit (TEX clauses)

- SAMPLE (with LOD, bias, gradients, offsets)
- Independent of ALU — runs in parallel via wavefront interleaving

### 5. Vertex Fetch Unit (VTX clauses)

- VFETCH (multiple formats, stride, offset, mega-fetch)
- Independent of ALU

### 6. LDS (Local Data Share) — 32KB per SIMD

- Accessed via special ALU instructions (LDS_1A, LDS_1A1D, LDS_2A)
- Atomic operations via LDS_IDX_OP

---

## NOVEL PACKING STRATEGIES

### Strategy 1: UINT24 Multiply — The Hidden Gem

`MUL_UINT24`, `MULHI_UINT24`, and `MULADD_UINT24` are **SINGLE-CYCLE** 24-bit
integer multiply operations. These are faster than full 32-bit MULLO_INT which
takes multiple cycles on the general ALU.

**Use cases:**
- Q8.8 fixed-point multiply: pack as 16-bit, multiply with MUL_UINT24 (fits in 24 bits)
- 3-component color multiply: 8-bit R,G,B values * 8-bit factor = 16-bit result (fits 24-bit)
- Index arithmetic: most array indices fit in 24 bits
- Texture coordinate quantization: 12.12 fixed-point via UINT24

**VLIW5 packing:** MUL_UINT24 runs on any x/y/z/w slot. You can do 4x UINT24 multiplies
per cycle + 1 transcendental. That's 4 parallel 24-bit multiplies per VLIW5 bundle!

### Strategy 2: UBYTE_FLT — Direct Byte-to-Float Extraction

`UBYTE0_FLT` through `UBYTE3_FLT` extract a specific byte from a 32-bit register
and convert it directly to float in **ONE CYCLE**.

**Use cases:**
- Unpack 4xINT8 from INT32 directly to float: 4 UBYTE_FLT ops in one VLIW5 bundle
- No AND/SHIFT needed — the hardware does byte extraction + int-to-float in one op
- Color conversion: packed RGBA8 → 4 float channels in 1 VLIW5 cycle
- Texture format conversion: bypass the TEX unit for simple byte unpacking

This is FASTER than the traditional AND+SHIFT+INT_TO_FLT approach by 3x.

### Strategy 3: BFE/BFI — Bit-Field Extract/Insert

`BFE_UINT(src, offset, width)` extracts arbitrary bit fields in ONE CYCLE.
`BFI_INT(src, insert, width)` inserts bit fields.

**Use cases:**
- Pack/unpack arbitrary bit widths: INT4, INT6, INT10, INT12
- Q4.4 fixed-point: BFE to extract 4-bit integer and fraction parts
- Compact vertex attributes: pack normal as 10.10.10.2 format
- Palette indices: 4-bit or 2-bit index extraction from packed data

**Novel VLIW5 combo:**
```
x: BFE_UINT R0, packed, 0, 8    // extract byte 0
y: BFE_UINT R1, packed, 8, 8    // extract byte 1
z: BFE_UINT R2, packed, 16, 8   // extract byte 2
w: BFE_UINT R3, packed, 24, 8   // extract byte 3
t: RECIPSQRT for something else  // free t-slot!
```
All 4 bytes extracted in ONE cycle with BFE, vs 4 cycles with AND+SHIFT.

### Strategy 4: BIT_ALIGN + BYTE_ALIGN — Arbitrary Bit Rotation

`BIT_ALIGN_INT(a, b, shift)` = (a << (32-shift)) | (b >> shift)
`BYTE_ALIGN_INT(a, b, shift)` = same but byte-aligned

**Use cases:**
- Barrel shifter operations for crypto/hash
- Packed data format conversion
- Cross-lane data sharing within a register

### Strategy 5: FLT16_TO_FLT32 / FLT32_TO_FLT16 — NATIVE Half-Float

The hardware has **native** FP16↔FP32 conversion instructions. No need for
software emulation.

**Use cases:**
- Load FP16 textures, convert to FP32 in ALU, process, convert back
- 2x FP16 packed in one 32-bit register: split with BFE, convert with FLT16_TO_FLT32
- Half-precision compute for ML inference: native conversion at full speed
- VLIW5: 4x FLT16_TO_FLT32 conversions per cycle on x/y/z/w slots

### Strategy 6: SAD_ACCUM — Sum of Absolute Differences

`SAD_ACCUM_UINT` and `SAD_ACCUM_HI_UINT` compute sum of absolute differences
in hardware. This is the core operation for:
- Video motion estimation
- Image comparison/matching
- Block matching for compression
- Simple edge detection

### Strategy 7: MULADD_M2/M4/D2 — Free Scaling

`MULADD_M2(a, b, c)` = a*b*2 + c (multiply result by 2 for FREE)
`MULADD_M4(a, b, c)` = a*b*4 + c (multiply result by 4 for FREE)
`MULADD_D2(a, b, c)` = a*b/2 + c (divide result by 2 for FREE)

These are the same cycle cost as regular MULADD but with implicit scaling.

**Use cases:**
- Fixed-point arithmetic: Q16.16 multiply can use MULADD_D2 to shift the result
- Power-of-two scaling in lighting calculations
- Gamma correction approximation

### Strategy 8: PREV-Chaining — Zero-Latency Forwarding

`MUL_PREV`, `ADD_PREV`, `MULADD_PREV`, `MULADD_IEEE_PREV` use the
**Previous Vector (PV)** or **Previous Scalar (PS)** result with zero-latency
forwarding. No register read needed.

**Use cases:**
- Long dependency chains: each op feeds directly to the next without GPR read
- Reduces register pressure: intermediate results don't need GPR storage
- Polynomial evaluation: Horner's method with MULADD_PREV chains

### Strategy 9: MAX4 — 4-Way Maximum

`MAX4(a, b, c, d)` computes the maximum of 4 values in ONE CYCLE.
Normal approach needs 3 MAX ops (3 cycles). MAX4 does it in 1.

**Use cases:**
- Bounding box computation
- Image processing: local maximum filter
- Depth buffer resolve

---

## REGISTER PRESSURE vs WAVEFRONT OCCUPANCY

| GPRs/Shader | Max Wavefronts | Occupancy | Best For |
|-------------|---------------|-----------|----------|
| 1-16 | 8 | 100% | Simple shaders, max latency hiding |
| 17-32 | 4 | 50% | Moderate complexity |
| 33-64 | 2 | 25% | Complex lighting |
| 65-128 | 1 | 12.5% | Very complex (avoid if possible) |

**Key insight**: With GPU at 62-75% idle, wavefront occupancy matters LESS
because there's already excess capacity. The bottleneck is CPU command
submission, not GPU compute.

**Novel approach**: Use MORE GPRs to do MORE work per shader invocation
(reducing draw calls and CPU overhead) rather than minimizing GPRs for
occupancy. The GPU has idle time to fill.
