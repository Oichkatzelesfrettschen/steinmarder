<!--
@file ZEN4_ISA_EXTENSION_COVERAGE.md
@brief Decomposition of every SIMD ISA extension on Zen 4 (Ryzen 9 7945HX),
       mapped against the probe suite. Identifies which extensions have coverage,
       which have gaps, and what each extension provides.
-->
# Zen 4 ISA Extension Decomposition and Probe Coverage

**Machine:** Ryzen 9 7945HX (Zen 4, Family 25h Model 61h)
**CPUID flags:** 40 SIMD-relevant extensions detected

---

## AVX-512 Sub-Extensions

AVX-512 is not one thing — it is 15+ independent CPUID feature bits. Each sub-extension
adds a specific set of instructions. Zen 4 supports 14 of the 17 known sub-extensions.

### AVX-512F (Foundation) — COVERED

The base 512-bit extension. Without this, nothing else works.

| Instruction Class | Key Instructions | Probe Coverage |
|---|---|---|
| FP32 arithmetic | `vaddps/vmulps/vfmadd231ps zmm` | `probe_avx512_fma`, `probe_depchain_float`, `probe_depchain_simd_width` |
| FP64 arithmetic | `vaddpd/vmulpd/vfmadd231pd zmm` | `probe_depchain_float`, `probe_depchain_simd_width` |
| INT32 arithmetic | `vpaddd/vpmulld zmm` | `probe_depchain_integer`, `probe_depchain_simd_width` |
| INT64 arithmetic | `vpaddq zmm` | `probe_depchain_integer` |
| Compare + mask | `vcmpps zmm → k` | `probe_avx512_mask` |
| Gather/scatter | `vpgatherdd/vpscatterdd zmm` | `probe_avx512_gather` |
| Permute | `vpermps/vpermd zmm` | `probe_avx512_shuffle` |
| Broadcast | `vbroadcastss zmm` | used in many probes |
| Reduce | `vreduceps zmm` | `probe_reduction` |
| Reciprocal/rsqrt approx | `vrcp14ps/vrsqrt14ps zmm` | `probe_transcendental` |
| Sqrt/div | `vsqrtps/vdivps zmm` | `probe_transcendental`, `probe_depchain_float` |
| Blend with mask | `vblendmps zmm {k}` | `probe_avx512_mask` |
| Compress/expand | `vcompressps/vexpandps zmm` | **GAP — no probe** |
| Convert | `vcvtdq2ps/vcvtps2dq zmm` | `probe_avx512_convert` (partial) |
| Ternary logic | `vpternlogd zmm` | `probe_popcount_lzcnt` |
| Move with mask | `vmovaps zmm {k}` | `probe_avx512_bandwidth`, `probe_avx512_mask` |

**Gap:** `vcompressps/vexpandps` (conditional pack/unpack — critical for sparse operations)

---

### AVX-512BW (Byte and Word) — COVERED

Extends 512-bit operations to 8-bit and 16-bit integer element sizes.

| Instruction Class | Key Instructions | Probe Coverage |
|---|---|---|
| INT8 add/sub | `vpaddb/vpsubb zmm` | `probe_depchain_integer` |
| INT16 add/sub | `vpaddw/vpsubw zmm` | `probe_depchain_integer` |
| INT16 multiply | `vpmullw zmm` | `probe_depchain_integer` |
| INT8 compare | `vpcmpb zmm → k` | **GAP** |
| INT8 max/min | `vpmaxub/vpminsb zmm` | **GAP** |
| INT8 SAD | `vpsadbw zmm` | `probe_depchain_int` |
| Mask ops (64-bit) | `kmovq/kandq/korq` | `probe_avx512_mask` (partial — 16-bit masks only) |
| Pack/unpack | `vpunpckhbw/vpackuswb zmm` | **GAP** |
| Shuffle bytes | `vpshufb zmm` | **GAP** (only vpermb in `probe_avx512_shuffle`) |

**Gap:** INT8 compare, min/max, vpshufb throughput, pack/unpack, 64-bit mask register ops

---

### AVX-512DQ (Doubleword/Quadword) — PARTIALLY COVERED

Enhanced 32/64-bit operations including FP64 conversions and broadcast from memory.

| Instruction Class | Key Instructions | Probe Coverage |
|---|---|---|
| FP64↔INT64 convert | `vcvtqq2pd/vcvttpd2qq zmm` | **GAP** |
| FP32 range/reduce | `vrangeps/vreduceps zmm` | **GAP** |
| INT64 multiply | `vpmullq zmm` | **GAP** (critical — Zen 4 may have native 64-bit mul here) |
| Broadcast from mem | `vbroadcastf64x2/vbroadcastf32x8` | **GAP** |
| Extract/insert 256→512 | `vextractf64x4/vinsertf64x4` | **GAP** |
| FPCLASS test | `vfpclassps zmm → k` | **GAP** |

**Gap:** Most of DQ is unprobed. `vpmullq zmm` especially — we assumed no native 64×64 mul but DQ may provide it.

---

### AVX-512VL (Vector Length) — IMPLICITLY COVERED

Not a new instruction set — it allows AVX-512 instructions to operate on 128/256-bit
registers (xmm/ymm) with mask support. Every AVX-512 instruction that we test at zmm
width also exists at xmm/ymm with VL.

| Probe Coverage |
|---|
| `probe_depchain_simd_width` tests FP32 FMA and INT32 ADD at 128/256/512 widths |
| `probe_avx512_fma` compares zmm vs ymm FMA |
| `probe_avx512_gather` compares zmm vs ymm gather |

**Status:** Implicitly covered through width-comparison probes.

---

### AVX-512CD (Conflict Detection) — GAP

Provides `vpconflictd/q zmm` — detects duplicate indices in a gather/scatter vector.
Critical for safe vectorized histogram updates and scatter-with-conflicts.

| Instruction | Purpose | Probe |
|---|---|---|
| `vpconflictd zmm` | Per-lane: bitmask of earlier lanes with same value | **NO PROBE** |
| `vplzcntd/q zmm` | Per-lane leading zero count | `probe_popcount_lzcnt` (partial) |
| `vpbroadcastmb2q/vpbroadcastmw2d` | Broadcast mask to vector | **NO PROBE** |

**Gap:** `vpconflictd` — the signature CD instruction. `vplzcntd` is covered.

---

### AVX-512VNNI (Vector Neural Network Instructions) — COVERED

| Instruction | Purpose | Probe |
|---|---|---|
| `vpdpbusd zmm` | uint8×int8 dot product → int32 | `probe_avx512_vnni`, `probe_dot_product` |
| `vpdpbusds zmm` | Same with saturation | **GAP** |
| `vpdpwssd zmm` | int16×int16 dot product → int32 | `probe_avx512_vnni` |
| `vpdpwssds zmm` | Same with saturation | **GAP** |

**Gap:** Saturating variants (`vpdpbusds/vpdpwssds`). Likely same throughput but worth confirming.

---

### AVX-512IFMA (Integer Fused Multiply-Add) — COVERED

| Instruction | Purpose | Probe |
|---|---|---|
| `vpmadd52luq zmm` | 52-bit multiply, accumulate low | `probe_avx512_ifma` |
| `vpmadd52huq zmm` | 52-bit multiply, accumulate high | `probe_avx512_ifma` |

**Status:** Fully covered. Found 2× asymmetry between lo/hi paths.

---

### AVX-512VBMI (Vector Byte Manipulation Instructions) — PARTIALLY COVERED

| Instruction | Purpose | Probe |
|---|---|---|
| `vpermb zmm` | Byte-granularity permute | `probe_avx512_shuffle` |
| `vpermi2b zmm` | 2-source byte permute | **GAP** |
| `vpmultishiftqb zmm` | Multi-shift within qwords | **GAP** |

**Gap:** `vpermi2b` (2-source byte permute) and `vpmultishiftqb` (bitfield extraction).

---

### AVX-512VBMI2 — GAP

| Instruction | Purpose | Probe |
|---|---|---|
| `vpcompressb/w zmm` | Byte/word compress (conditional pack) | **NO PROBE** |
| `vpexpandb/w zmm` | Byte/word expand (conditional unpack) | **NO PROBE** |
| `vpshldv/vpshrdv zmm` | Variable-count concatenate-and-shift | **NO PROBE** |

**Gap:** Entire extension unprobed. `vpcompressb` is critical for UTF-8 processing and sparse byte streams.

---

### AVX-512BF16 — COVERED

| Instruction | Purpose | Probe |
|---|---|---|
| `vdpbf16ps zmm` | BF16 pair dot product → FP32 | `probe_avx512_bf16`, `probe_depchain_float` |
| `vcvtne2ps2bf16 zmm` | Convert 2×FP32 → BF16 | `probe_avx512_convert` |
| `vcvtneps2bf16 ymm` | Convert FP32 → BF16 (half width) | `probe_avx512_convert` |

**Status:** Fully covered.

---

### AVX-512BITALG — PARTIALLY COVERED

| Instruction | Purpose | Probe |
|---|---|---|
| `vpopcntb/w zmm` | Per-byte/word population count | **GAP** (only dword/qword in `probe_popcount_lzcnt`) |
| `vpshufbitqmb zmm → k` | Bit-select within qwords → mask | **NO PROBE** |

**Gap:** Byte/word granularity popcount and the exotic `vpshufbitqmb`.

---

### AVX-512VPOPCNTDQ — COVERED

| Instruction | Purpose | Probe |
|---|---|---|
| `vpopcntd zmm` | Per-dword population count | `probe_popcount_lzcnt` |
| `vpopcntq zmm` | Per-qword population count | `probe_popcount_lzcnt` |

**Status:** Fully covered.

---

### AVX-512FP16 — NOT AVAILABLE ON ZEN 4

This extension (native FP16 compute — `vaddph`, `vmulph`, `vfmaph`) exists only on
Intel Sapphire Rapids and later. Zen 4 has F16C (convert only) but no FP16 ALU.

**Status:** N/A. Probes note FP16 as "emulated via F16C converts" in `probe_depchain_float`.

---

## Pre-AVX-512 SIMD Extensions (128-bit and 256-bit)

### SSE / SSE2 / SSE3 / SSSE3 / SSE4.1 / SSE4.2 — PARTIALLY COVERED

| Extension | Key Instructions | Probe Coverage |
|---|---|---|
| SSE2 | `addps/mulps/divps/sqrtps xmm`, `paddd xmm` | `probe_depchain_simd_width` (128-bit), `probe_depchain_float` (scalar) |
| SSE4.1 | `dpps xmm` (4-wide dot product), `ptest`, `roundps` | `probe_dot_product` |
| SSE4.1 | `pmulld xmm` (INT32 mul), `packusdw`, `pblendvb` | **GAP** |
| SSE4.2 | `pcmpestri/pcmpistri` (string compare) | **GAP** |
| SSSE3 | `pshufb xmm`, `pmaddubsw`, `phaddw/d` | **GAP** |

**Gap:** SSE4.2 string instructions, SSSE3 `pshufb` at 128-bit, horizontal add (`phaddw`).

---

### AVX / AVX2 — COVERED

| Extension | Key Instructions | Probe Coverage |
|---|---|---|
| AVX | `vaddps/vmulps ymm`, 3-operand encoding | `probe_fma`, `probe_depchain_simd_width` |
| AVX2 | `vpaddd/vpaddq ymm`, `vpermps ymm`, `vpgatherdd ymm` | `probe_permute`, `probe_gather`, `probe_depchain_simd_width` |
| AVX2 | `vpsravd ymm` (variable shift), `vpbroadcastd` | **GAP** (variable shift) |

**Gap:** Variable-count shifts (`vpsravd/vpsrlvd/vpsllvd`) — important for bit manipulation.

---

### FMA3 — COVERED

| Instruction | Probe Coverage |
|---|---|
| `vfmadd231ps ymm/zmm` | `probe_fma`, `probe_avx512_fma`, `probe_depchain_float`, `probe_depchain_simd_width` |
| `vfmadd231pd ymm/zmm` | `probe_depchain_float`, `probe_depchain_simd_width` |
| `vfmsub/vfnmadd/vfnmsub` variants | **GAP** (assumed same throughput but unverified) |

**Gap:** FMA negation/subtraction variants. Likely identical throughput but unconfirmed.

---

### F16C — PARTIALLY COVERED

| Instruction | Purpose | Probe |
|---|---|---|
| `vcvtph2ps xmm/ymm` | FP16 → FP32 convert | `probe_depchain_float` (used in FP16 emulated path) |
| `vcvtps2ph xmm/ymm` | FP32 → FP16 convert | `probe_depchain_float` (used in FP16 emulated path) |

**Status:** Used as part of FP16 emulation in `probe_depchain_float`. Throughput not measured standalone.

---

### Non-SIMD Crypto / Bit Extensions

| Extension | Key Instructions | Probe Coverage |
|---|---|---|
| **AES-NI** | `aesenc/aesdec/aesimc xmm` | **NO PROBE** |
| **VAES** | `vaesenc ymm/zmm` (vectorized AES) | **NO PROBE** |
| **VPCLMULQDQ** | `vpclmulqdq ymm/zmm` (carry-less mul) | **NO PROBE** |
| **PCLMULQDQ** | `pclmulqdq xmm` | **NO PROBE** |
| **SHA-NI** | `sha1rnds4/sha256rnds2 xmm` | **NO PROBE** |
| **GFNI** | `vgf2p8mulb/vgf2p8affineqb zmm` (Galois Field) | **NO PROBE** |
| **BMI1** | `andn/bextr/blsi/blsr/tzcnt` | **NO PROBE** |
| **BMI2** | `bzhi/mulx/pdep/pext/rorx/sarx/shlx/shrx` | **NO PROBE** |
| **ADX** | `adcx/adox` (multi-precision add) | **NO PROBE** |
| **POPCNT** | `popcnt r64` (scalar) | **GAP** (only SIMD popcnt probed) |
| **LZCNT** | `lzcnt r64` (scalar) | **GAP** |
| **RDRAND/RDSEED** | Hardware RNG | **NO PROBE** |
| **MOVBE** | Byte-swap load/store | **NO PROBE** |
| **CLFLUSHOPT/CLWB** | Cache line flush/writeback | **NO PROBE** |

---

## Coverage Summary

| Category | Extensions | Fully Covered | Partial | No Probe |
|---|---|---|---|---|
| AVX-512 compute | F, BW, DQ, VL | 2 | 2 | 0 |
| AVX-512 specialized | VNNI, IFMA, BF16, VPOPCNTDQ | 4 | 0 | 0 |
| AVX-512 manipulation | VBMI, VBMI2, BITALG, CD | 0 | 2 | 2 |
| Pre-512 SIMD | SSE*, AVX, AVX2, FMA3, F16C | 3 | 2 | 1 |
| Crypto/bit | AES, VAES, SHA, GFNI, BMI, VPCLMUL, ADX | 0 | 0 | 7 |

**32 probes cover 11 extensions fully or partially. 9 extensions have zero coverage.**

---

## Gap Priority for New Probes

### High (directly relevant to NeRF/MLP/compute workloads)
1. **AVX-512DQ `vpmullq zmm`** — native 64×64 integer multiply, critical gap
2. **AVX-512CD `vpconflictd`** — histogram/scatter safety
3. **AVX-512VBMI2 `vpcompressb/vpshldv`** — sparse byte ops, variable shifts
4. **AVX2 variable shifts `vpsravd/vpsrlvd`** — bit manipulation building block

### Medium (crypto/hashing/string processing)
5. **VAES `vaesenc zmm`** — vectorized AES (16 blocks per instruction)
6. **GFNI `vgf2p8mulb zmm`** — Galois field ops (error correction, crypto)
7. **SHA-NI `sha256rnds2`** — hardware SHA-256
8. **BMI2 `pdep/pext`** — bit deposit/extract (hash functions, compact)

### Low (niche/legacy)
9. **SSE4.2 string ops `pcmpestri`** — text processing
10. **VPCLMULQDQ** — carry-less multiply (CRC, polynomial arithmetic)
11. **RDRAND/RDSEED** — hardware RNG throughput
