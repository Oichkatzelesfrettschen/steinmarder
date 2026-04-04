# AMD Radeon HD 6310 (TeraScale-2/Evergreen) Hardware Capabilities Reference

**GPU**: AMD Radeon HD 6310 (Wrestler die, device 0x9802)
**Architecture**: TeraScale-2 / VLIW5 / Evergreen ISA
**Process**: 40nm
**TDP**: shared with E-300 APU (18W total)

---

## 1. ALU Capabilities

### Native Data Types

| Type | Native | Width | ISA Opcodes | Notes |
|------|--------|-------|-------------|-------|
| FP32 (float) | YES | 32-bit | MUL_IEEE, ADD, MULADD_IEEE, MOV, etc. | Full IEEE 754 single precision |
| FP64 (double) | NO | 64-bit | (emulated via FP32 pairs) | Dekker/Knuth algorithm, ~10-15 ops per FP64 op |
| FP16 (half) | NO | 16-bit | (pack/unpack via INT32 bitops) | Can pack 2x FP16 in one 32-bit register |
| INT32 | YES | 32-bit | MUL, ADD, AND, OR, XOR, LSHL, LSHR, ASHR, BFE, BFI | Full integer arithmetic |
| UINT24 | YES | 24-bit | UMUL24, UMAD24 | Fast 24-bit multiply (single cycle) |
| INT16 | NO | 16-bit | (lower to 32-bit) | Can pack 2x INT16 in INT32 |
| INT8 | NO | 8-bit | (lower to 32-bit) | Can pack 4x INT8 in INT32 |
| BOOL | Via INT32 | 1-bit | PRED_SETE, PRED_SETGT, etc. | Predicate register for conditional exec |

### Transcendental Operations (t-slot only)

| Operation | Opcode | Precision | Cycles |
|-----------|--------|-----------|--------|
| Reciprocal | RECIP_IEEE | Full FP32 | 1 |
| Reciprocal Square Root | RECIPSQRT_IEEE | Full FP32 | 1 |
| Square Root | SQRT_IEEE | Full FP32 | 1 |
| Sine | SIN | ~0.0008 max error | 1 |
| Cosine | COS | ~0.0008 max error | 1 |
| Log2 | LOG_IEEE | Full FP32 | 1 |
| Exp2 | EXP_IEEE | Full FP32 | 1 |
| Multiply (overflow) | MUL_IEEE | Full FP32 | 1 |

### Comparison/Conditional Operations

| Operation | Opcode | Notes |
|-----------|--------|-------|
| Set Equal | SETE | Result = (a == b) ? 1.0 : 0.0 |
| Set Greater Than | SETGT | Result = (a > b) ? 1.0 : 0.0 |
| Set Greater/Equal | SETGE | |
| Set Not Equal | SETNE | |
| Conditional Select | CNDE / CNDGT / CNDE_INT | Ternary select: result = (cond) ? a : b |
| Kill Fragment | KILLGT / KILLE | Discard pixel if condition met |
| Max | MAX / MAX_DX10 | max(a, b) |
| Min | MIN / MIN_DX10 | min(a, b) |
| Clamp/Saturate | _sat modifier | Clamp result to [0.0, 1.0] |

---

## 2. VLIW5 SIMD Architecture

### Structure

```
HD 6310 GPU
  +-- SIMD Engine 0
  |     +-- VLIW5 ALU Array
  |     |     +-- Slot x: FP32/INT32 general ALU
  |     |     +-- Slot y: FP32/INT32 general ALU
  |     |     +-- Slot z: FP32/INT32 general ALU
  |     |     +-- Slot w: FP32/INT32 general ALU
  |     |     +-- Slot t: Transcendental + FP32 overflow
  |     +-- TEX Unit (texture fetch)
  |     +-- VTX Unit (vertex fetch)
  |     +-- Register File: 128 GPRs x 128-bit (vec4)
  |
  +-- SIMD Engine 1
        +-- (identical structure)
```

### Per-Cycle Throughput (per SIMD)

| Resource | Throughput | Notes |
|----------|-----------|-------|
| FP32 MAD/MUL/ADD | 5 ops | All 5 VLIW slots |
| INT32 ops | 5 ops | All 5 VLIW slots |
| Transcendental | 1 op | t-slot only |
| TEX fetch | 1 fetch | Per clause, interleaved |
| VTX fetch | 1 fetch | Per clause, interleaved |

### Wavefront Scheduling

- Wavefront size: 64 threads (but HD 6310 may use 32-wide on Wrestler)
- Max wavefronts per SIMD: depends on GPR usage
- GPR budget: 128 vec4 registers per SIMD, shared among active wavefronts
- More GPRs per shader = fewer concurrent wavefronts = less latency hiding
- Ideal: small shaders (low GPR count) allow more wavefronts in flight

### GPR Pressure vs Wavefront Occupancy

| GPRs Used | Max Wavefronts | Occupancy | Latency Hiding |
|-----------|---------------|-----------|----------------|
| 1-16 | 8 | 100% | Excellent |
| 17-32 | 4 | 50% | Good |
| 33-64 | 2 | 25% | Limited |
| 65-128 | 1 | 12.5% | Poor |

---

## 3. Clause Architecture

TeraScale-2 organizes instructions into **clauses** controlled by CF (Control Flow):

| Clause Type | Purpose | Latency | Hiding Strategy |
|-------------|---------|---------|-----------------|
| ALU | Compute (VLIW5 bundles) | 1 cycle/bundle | Fill all 5 slots |
| TEX | Texture sampling | ~100-400 cycles | Wavefront interleaving |
| VTX | Vertex buffer fetch | ~100-400 cycles | Wavefront interleaving |
| CF | Control flow (branch, loop) | Variable | Minimize branching |
| EXPORT | Write results | ~10 cycles | Pipeline end |

The hardware switches between wavefronts during TEX/VTX stalls:
1. Wavefront A issues TEX clause (starts memory fetch)
2. Hardware switches to Wavefront B, runs its ALU clause
3. Wavefront B issues TEX clause
4. Hardware switches back to Wavefront A (TEX data arrived)
5. Wavefront A runs ALU clause consuming TEX results

This is the TeraScale equivalent of NVIDIA "warp scheduling" / "latency hiding".

---

## 4. Packing and Emulation Strategies

### 4x INT8 in INT32 (SIMD-in-register)

```
INT32 register = [byte3 | byte2 | byte1 | byte0]

Extract byte0: AND_INT reg, 0xFF
Extract byte1: LSHR reg, 8; AND_INT result, 0xFF
Extract byte2: LSHR reg, 16; AND_INT result, 0xFF
Extract byte3: LSHR reg, 24

Pack bytes: byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24)
  Uses: OR_INT, LSHL
  On Evergreen: BFI (bit-field insert) is faster

8-bit arithmetic on packed data:
  ADD: add packed INT32s, then mask overflow between bytes
  MUL: extract, multiply pairs with UMUL24, repack
  AND/OR/XOR: operate directly on packed INT32
```

### 2x FP16 in FP32

```
FP32 register = [FP16_high | FP16_low]

Mesa already has nir_lower_pack_unpack_2x16 for this.
The r600 driver calls r600_nir_lower_pack_unpack_2x16.

Unpack: extract 16-bit halves via LSHR/AND, convert to FP32
Pack: convert FP32 to FP16 (truncate), combine via OR/LSHL
```

### FP64 via Dekker Double-Single (2x FP32)

```
A double-precision value D is represented as:
  D = hi + lo  (where hi, lo are FP32)
  hi = float(D)  (the leading bits)
  lo = float(D - hi)  (the residual)

Addition: D = (a_hi + b_hi) + (a_lo + b_lo) with error correction
  ~6-8 FP32 ops

Multiplication: D = a_hi * b_hi + (a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo
  ~10-15 FP32 ops

VLIW5 advantage: hi and lo operations can run in parallel across
x/y (hi path) and z/w (lo path) slots in the same bundle.
The t-slot handles any transcendental component.
```

### Q16.16 Fixed Point (native INT32)

```
Q16.16: 16 bits integer, 16 bits fraction
  Stored as INT32 where value = raw_int / 65536.0

Addition: plain INT32 ADD (1 op)
Multiplication: UMUL * UMUL + shift
  result = (a * b) >> 16
  Uses: MUL_IEEE or UMUL24 + LSHR

Useful for: position interpolation, UV coordinates, animation
Precision: ~0.000015 resolution (good enough for most graphics)
```

---

## 5. Memory Architecture

| Resource | Size | Bandwidth | Notes |
|----------|------|-----------|-------|
| VRAM (shared) | 384 MB max (from 16GB system RAM) | ~8.5 GB/s (DDR3-1066) | UMA - shared with CPU |
| GTT (system) | ~14 GB | ~8.5 GB/s | Same bus as VRAM on UMA |
| LDS (local) | 32 KB per SIMD | ~150 GB/s | Local data share for compute |
| GPR File | 128 x vec4 per SIMD | Internal | Zero-latency register access |
| Texture Cache | ~16 KB per TEX unit | Internal | L1 texture cache |
| Vertex Cache | ~16 KB per VTX unit | Internal | Vertex reuse cache |

### Memory Bandwidth Optimization

The shared DDR3 bus is the primary bandwidth bottleneck:
1. Minimize VTX fetches: use vertex reuse, indexed drawing
2. Minimize TEX fetches: use texture compression (BC/DXT), mipmaps
3. Batch draws: fewer state changes = fewer cache flushes
4. Use LDS for compute: 32KB local memory avoids global memory trips

---

## 6. ISA Reference Quick Card

### ALU Instruction Format (VLIW5 bundle)

```
CF_ALU_WORD0 | CF_ALU_WORD1     -- clause header
  ALU_WORD0 | ALU_WORD1_OP2/3   -- instruction 1 (slot x)
  ALU_WORD0 | ALU_WORD1_OP2/3   -- instruction 2 (slot y)
  ALU_WORD0 | ALU_WORD1_OP2/3   -- instruction 3 (slot z)
  ALU_WORD0 | ALU_WORD1_OP2/3   -- instruction 4 (slot w) LAST=1
  ALU_WORD0 | ALU_WORD1_OP2/3   -- instruction 5 (slot t) LAST=1

LAST bit (bit 31 of ALU_WORD0) marks the end of a VLIW5 group.
Each instruction is 64 bits (2 DWORDs).
```

### TEX Instruction Format

```
TEX_WORD0 | TEX_WORD1 | TEX_WORD2 | TEX_WORD3  -- 128 bits per instruction

Encodes: resource ID, sampler ID, source register, destination register,
         coordinate selection, LOD bias, offsets, format
```

### VTX Instruction Format

```
VTX_WORD0 | VTX_WORD1 | VTX_WORD2 | VTX_WORD3  -- 128 bits per instruction

Encodes: resource ID, source register, destination register,
         data format, stride, offset, mega-fetch count
```

---

## 7. Relevant ISA Documents

- `AMD_Evergreen-Family_ISA.pdf` -- Chapter 3: ALU, Chapter 4: TEX, Chapter 5: VTX
- `R600_Instruction_Set_Architecture.pdf` -- R600/R700 base ISA
- `R600-R700-Evergreen_Assembly_Language_Format.pdf` -- Encoding tables
- `LLVM_R600_Backend_Stellard_2013.pdf` -- LLVM backend internals

All in `~/eric/TerakanMesa/isa-docs/`

---

*Generated 2026-03-29 from RE toolkit analysis + AMD ISA documentation*
