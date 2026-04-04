# Numeric Packing & Extended Precision Research — Evergreen ISA

## 1. INT4x8 / INT8x4 / Q8.8 / Q4.4 Packing via BFE+UBYTE Chains

### 4x INT8 in INT32 (pack/unpack)

**Unpack** (INT32 → 4 separate bytes):
```
Bundle 1 (ALL in one VLIW5 cycle):
  x: UBYTE0_FLT  R0, packed   → float(byte0)    ; or BFE_UINT R0, packed, 0, 8
  y: UBYTE1_FLT  R1, packed   → float(byte1)    ; or BFE_UINT R1, packed, 8, 8
  z: UBYTE2_FLT  R2, packed   → float(byte2)    ; or BFE_UINT R2, packed, 16, 8
  w: UBYTE3_FLT  R3, packed   → float(byte3)    ; or BFE_UINT R3, packed, 24, 8
  t: (free for any transcendental)
```
**Cost: 1 VLIW5 cycle for all 4 bytes.** UBYTE_FLT gives float directly. BFE_UINT gives integer.

**Pack** (4 bytes → INT32):
```
Bundle 1:
  x: LSHL_INT   T0, byte1, 8      ; shift byte1 to position
  y: LSHL_INT   T1, byte2, 16     ; shift byte2 to position
  z: LSHL_INT   T2, byte3, 24     ; shift byte3 to position
  w: MOV        T3, byte0          ; byte0 is already at position 0
  t: (free)
Bundle 2:
  x: OR_INT     T4, T3, T0        ; combine byte0 | byte1
  y: OR_INT     T5, T1, T2        ; combine byte2 | byte3
  z: (free)
  w: (free)
  t: (free)
Bundle 3:
  x: OR_INT     packed, T4, T5    ; final combine
```
**Cost: 3 cycles.** Or use BFI_INT for 2 cycles:
```
Bundle 1:
  x: BFI_INT    T0, 0xFF00, byte1<<8, byte0     ; insert byte1 at [15:8]
  y: BFI_INT    T1, 0xFF0000, byte2<<16, T0     ; insert byte2 at [23:16]
Bundle 2:
  x: BFI_INT    packed, 0xFF000000, byte3<<24, T1  ; insert byte3 at [31:24]
```
**Cost: 2 cycles with BFI_INT.**

### 8x INT4 in INT32

**Unpack** (INT32 → 8 nibbles):
```
BFE_UINT R0, packed, 0, 4     ; nibble 0
BFE_UINT R1, packed, 4, 4     ; nibble 1
BFE_UINT R2, packed, 8, 4     ; nibble 2
BFE_UINT R3, packed, 12, 4    ; nibble 3
; ... 4 more for nibbles 4-7
```
**Cost: 2 VLIW5 cycles** (4 BFE per cycle on x/y/z/w slots).

### Q8.8 Fixed-Point (16-bit: 8 integer + 8 fraction)

**Multiply two Q8.8 values packed in INT32:**
```
; Q8.8 * Q8.8 = Q16.16, then truncate back to Q8.8
; a and b are 16-bit values (0-65535 representing 0.0-255.99609375)
Bundle 1:
  x: MUL_UINT24  T0, a, b       ; 24-bit multiply (Q8.8 fits in 16 bits < 24)
Bundle 2:
  x: LSHR_INT    result, T0, 8  ; shift right by 8 to get Q8.8 result
```
**Cost: 2 cycles.** MUL_UINT24 is single-cycle because Q8.8 values fit in 16 bits.

### Q4.4 Fixed-Point (8-bit: 4 integer + 4 fraction)

Same pattern but with UBYTE extraction first:
```
UBYTE0_FLT → gives float directly (0.0-255.0)
Then: fract = FRACT(float_val / 16.0)  ; extract 4-bit fraction
      integer = FLT_TO_INT(float_val / 16.0)  ; extract 4-bit integer
```
Or pure integer:
```
BFE_UINT  frac, packed_byte, 0, 4    ; lower 4 bits = fraction
BFE_UINT  intg, packed_byte, 4, 4    ; upper 4 bits = integer
```
**Cost: 1 cycle** (both BFE in one VLIW5 bundle).

---

## 2. FP16 from INT8x2 Packing via UBYTE+FLT16 Chain

### The Chain: packed_int32 → 2x INT8 → 2x FP32 → pack to FP16 pair

```
Bundle 1 (unpack bytes to float):
  x: UBYTE0_FLT  F0, packed     ; byte0 → float
  y: UBYTE1_FLT  F1, packed     ; byte1 → float
  z: (free)
  w: (free)
  t: (free)

Bundle 2 (scale to FP16 range and convert):
  x: MUL_IEEE    F0, F0, 1/255.0   ; normalize to [0,1]
  y: MUL_IEEE    F1, F1, 1/255.0   ; normalize to [0,1]
  z: (free)
  w: (free)
  t: (free)

Bundle 3 (convert FP32 → FP16 and pack):
  x: FLT32_TO_FLT16  H0, F0     ; FP32 → FP16 (lower half)
  y: FLT32_TO_FLT16  H1, F1     ; FP32 → FP16 (upper half)
  z: LSHL_INT        H1s, H1, 16  ; shift upper half
  w: (free)
  t: (free)

Bundle 4 (combine):
  x: OR_INT  packed_fp16, H0, H1s  ; pack 2x FP16 into INT32
```
**Total: 4 VLIW5 cycles** for INT8x2 → packed FP16x2.

### Reverse: packed FP16x2 → 2x float

```
Bundle 1:
  x: FLT16_TO_FLT32  F0, packed        ; lower FP16 → FP32
  y: LSHR_INT        T0, packed, 16     ; extract upper FP16
  z: (free)
  w: (free)
  t: (free)

Bundle 2:
  x: FLT16_TO_FLT32  F1, T0            ; upper FP16 → FP32
```
**Total: 2 cycles.** Native hardware conversion.

### Validation: Does UBYTE_FLT + FLT32_TO_FLT16 compose correctly?

YES — the chain is mathematically sound:
1. UBYTE_FLT(byte) = exact integer-to-float (byte values 0-255 map to floats 0.0-255.0)
2. MUL_IEEE(x, 1/255) = normalize to [0,1] range (IEEE multiply, full precision)
3. FLT32_TO_FLT16(x) = hardware truncation to half-float (correct rounding)

The only precision loss is in step 3 (FP16 has 10-bit mantissa = ~3 decimal digits).
For normalized color values [0,1], this is sufficient for all visual purposes.

---

## 3. FP64 Emulation: Dekker Double-Single, Q32.32, and Novel Formats

### Available Hardware for Extended Precision

The Evergreen ISA provides these building blocks:

| Instruction | What it does | Cycles | Slot |
|---|---|---|---|
| ADD_64 | FP64 add | 2 (uses 2 slots) | vec |
| MUL_64 | FP64 multiply | 2 (uses 2 slots) | vec |
| FMA_64 | FP64 fused multiply-add | 2 (uses 2 slots) | vec |
| FLT32_TO_FLT64 | FP32→FP64 conversion | 2 | vec |
| FLT64_TO_FLT32 | FP64→FP32 conversion | 2 | vec |
| RECIP_64 | FP64 reciprocal | trans | t-slot |
| RECIPSQRT_64 | FP64 inverse sqrt | trans | t-slot |
| SQRT_64 | FP64 square root | trans | t-slot |
| FRACT_64 | FP64 fractional part | 2 | vec |
| FREXP_64 | FP64 exponent extract | 2 | vec |
| LDEXP_64 | FP64 load exponent | 2 | vec |
| ADDC_UINT | Add with carry | 1 | vec |
| SUBB_UINT | Subtract with borrow | 1 | vec |
| MULHI_UINT | Upper 32 bits of 32x32 multiply | multi-cycle | trans |

### WAIT — The HD 6310 HAS native FP64!

The opcode list shows `ADD_64`, `MUL_64`, `FMA_64`, `RECIP_64`, `SQRT_64` etc.
These are **hardware FP64 instructions** on Evergreen. They use 2 ALU slots per
operation (consuming an x+y or z+w pair) but they ARE native, not emulated.

**This means:** Full IEEE 754 double-precision is available in hardware.
It runs at **half the FP32 rate** (2 slots per op instead of 1), so peak FP64
is 2.5 GFLOPS (half of the ~5 GFLOPS FP32 peak).

The Mesa nir_options already has `lower_int64_options = ~0` and handles 64-bit
through the SFN 64-bit lowering pass in `sfn_nir_lower_64bit.cpp`.

### Dekker Double-Single (2x FP32 → ~48-bit precision)

Even though we have native FP64, Dekker can be useful for:
- **Avoiding 2-slot penalty**: Dekker uses independent FP32 ops that can fill different VLIW5 slots
- **VLIW5 packing**: hi and lo halves can execute in parallel (x/y for hi, z/w for lo, t for RSQ)

**Dekker addition** (a_hi+a_lo) + (b_hi+b_lo):
```
Bundle 1:
  x: ADD    s, a_hi, b_hi           ; approximate sum
  y: ADD    v, s, -a_hi             ; error term part 1 (using ADD_PREV possible)
  z: (free for other work)
  w: (free for other work)
  t: (free)

Bundle 2:
  x: ADD    w, v, -b_hi             ; error recovery
  y: ADD    e, a_lo, b_lo           ; low parts
  z: ADD    e2, w, e                ; combine errors
  w: (free)
  t: (free)

Bundle 3:
  x: ADD    result_hi, s, e2        ; corrected high
  y: ADD    T, result_hi, -s        ; extract correction
  z: ADD    result_lo, e2, -T       ; remaining error = low part
```
**Cost: 3 cycles for ~48-bit addition** using only FP32 slots.
Compare: native ADD_64 = 1 cycle but consumes 2 slots.

**Verdict**: For pure FP64 math, use the NATIVE hardware (it's there!).
Use Dekker only when you need to interleave extended-precision with other
work in the same VLIW5 bundles to maximize slot utilization.

### Q32.32 Fixed-Point (64-bit: 32 integer + 32 fraction)

Uses ADDC_UINT (add with carry) for exact 64-bit integer arithmetic:
```
; Add two Q32.32: (a_hi.a_lo) + (b_hi.b_lo)
Bundle 1:
  x: ADD_INT     lo, a_lo, b_lo      ; add low 32 bits
  y: ADDC_UINT   carry, a_lo, b_lo   ; get carry bit
  z: (free)
  w: (free)
  t: (free)

Bundle 2:
  x: ADD_INT     hi, a_hi, b_hi      ; add high 32 bits
  y: ADD_INT     hi, hi, carry        ; add carry
```
**Cost: 2 cycles for exact 64-bit integer add.**

Q32.32 multiply needs MULHI_UINT for cross-terms:
```
; Q32.32 multiply: result = (a_hi.a_lo) * (b_hi.b_lo)
; Full 64x64→128 multiply requires 4 partial products + carries
; For Q32.32, we need the middle 64 bits of the 128-bit result

Bundle 1:
  x: MUL_UINT24  or MULLO_UINT   P0, a_lo, b_lo     ; low*low (lower 32 bits)
  t: MULHI_UINT                   P1, a_lo, b_lo     ; low*low (upper 32 bits)

Bundle 2:
  x: MULLO_UINT   P2, a_lo, b_hi    ; low*high (lower 32 bits)
  t: MULHI_UINT   P3, a_lo, b_hi    ; low*high (upper 32 bits)

Bundle 3:
  x: MULLO_UINT   P4, a_hi, b_lo    ; high*low (lower 32 bits)
  t: MULHI_UINT   P5, a_hi, b_lo    ; high*low (upper 32 bits)

Bundle 4:
  x: ADD_INT      M, P1, P2          ; accumulate middle
  y: ADDC_UINT    C1, P1, P2         ; carry
  z: ADD_INT      M, M, P4           ; add other cross-term
  w: ADDC_UINT    C2, M, P4          ; carry

Bundle 5:
  x: ADD_INT      result_hi, P3, P5  ; high cross-terms
  y: ADD_INT      result_hi, result_hi, C1
  z: ADD_INT      result_hi, result_hi, C2
  w: MOV          result_lo, M
```
**Cost: ~5 cycles for Q32.32 multiply.** Compare: native MUL_64 = 1 cycle (2 slots).

### Novel: FP32 + UINT24 Tandem (56-bit effective range)

A creative combination: use FP32 for the significant bits (24-bit mantissa)
and UINT24 for an additional 24 bits of integer precision:

```
value = fp32_mantissa * 2^fp32_exponent + uint24_extension * 2^(fp32_exponent-24)
```

This gives 48 bits of mantissa (24 from FP32 + 24 from UINT24) with the
FP32 exponent range. It's not IEEE-compliant but could be useful for:
- Extended-precision accumulation (reduce rounding error in long sums)
- Ray-tracing distance calculations (need range + precision)
- Scientific computing where FP64 is too slow but FP32 isn't precise enough

**Cost**: Each operation needs 2 FP32 ops + 1 UINT24 op = fits in one VLIW5 bundle.

### VLIW5 Tandem Layout for Extended Precision

The ultimate VLIW5 utilization for extended precision:
```
x: FP32 operation (high precision part)
y: FP32 operation (low precision part / error correction)
z: UINT24 operation (extension bits / carry)
w: INT32 operation (exponent manipulation / bit packing)
t: Transcendental (RSQ/RCP/SIN/COS on the high part)
```
ALL 5 SLOTS BUSY — full VLIW5 utilization for extended-precision math.

---

## Summary: What's Available and When to Use It

| Format | Precision | Pack cost | Unpack cost | Math cost | Best for |
|--------|-----------|-----------|-------------|-----------|----------|
| INT8x4 | 8-bit integer | 2-3 cycles | 1 cycle | N/A | Color, indices |
| INT4x8 | 4-bit integer | 3-4 cycles | 2 cycles | N/A | Palette, weights |
| Q8.8 | 16-bit fixed | 1 cycle | 1 cycle | 2 cycles (mul) | UV coords, animation |
| Q4.4 | 8-bit fixed | 1 cycle | 1 cycle | 2 cycles (mul) | Compact weights |
| FP16x2 | 2x half-float | 4 cycles | 2 cycles | N/A (promote to FP32) | Packed textures |
| FP32 | 32-bit float | N/A | N/A | 1 cycle | Default |
| FP64 (native!) | 64-bit float | N/A | N/A | 1 cycle (2 slots) | When precision needed |
| Dekker 2xFP32 | ~48-bit float | N/A | N/A | 3 cycles (add) | VLIW5 interleaving |
| Q32.32 | 64-bit fixed | N/A | N/A | 2 cycles (add), 5 (mul) | Exact integer |
| FP32+UINT24 | ~48-bit hybrid | N/A | N/A | 1 VLIW5 cycle | Novel extended precision |
