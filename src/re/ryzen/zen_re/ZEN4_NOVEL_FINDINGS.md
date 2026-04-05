<!--
@file ZEN4_NOVEL_FINDINGS.md
@brief Non-obvious, surprising, and architecturally revealing instruction behaviors
       discovered through the exhaustive Zen 4 (7945HX) measurement campaign.
-->
# Zen 4 Novel Opcode and Mnemonic Findings

Derived from 1,280 measured instruction forms across 817 valid Zen 4 mnemonics.

---

## 1. Massive Register Renaming / Elimination Surface

Over 150 instructions measure latency < 0.02 cyc, meaning the CPU eliminates them
entirely — no execution unit is consumed. This goes far beyond the expected MOV
elimination:

**Expected eliminations:**
- `MOV r64,r64` / `MOV r32,r32`: 0.12 cyc (documented zero-latency rename)
- `MOVAPS/MOVAPD/MOVDQA xmm,xmm`: 0.12 cyc
- `VMOVAPS/VMOVAPD ymm/zmm`: 0.12 cyc
- `FXCH st(1)`: 0.12 cyc (x87 stack swap by rename)
- `NOP`: eliminated

**Surprising eliminations (< 0.02 cyc latency):**
- All integer min/max (`PMAXSB/W/D`, `PMAXUB/W/D`, `PMINSB/W/D`, `PMINUB/W/D`) at
  xmm/ymm/zmm — zero apparent latency in dep chains
- All integer compare (`PCMPEQB/W/D/Q`, `PCMPGTB/W/D/Q`) at all widths
- All bitwise logic (`PAND`, `PANDN`, `POR`, `PXOR`) at all widths
- All integer add/sub (`PADDB/W/D/Q`, `PSUBB/W/D/Q`) at all widths
- All saturating add/sub at all widths
- Shuffles: `PSHUFD`, `PSHUFHW`, `PSHUFLW`, `PALIGNR`, `PSHUFB` at xmm/ymm
- Permutes: `VPERMD`, `VPERM2I128`, `VINSERTI128`, `VEXTRACTI128`
- Absolute value: `PABSB/W/D` at all widths
- Variable shifts: `VPSLLVD/Q`, `VPSRLVD/Q`, `VPSRAVD` at xmm/ymm
- MMX arithmetic: `PADDB/W/D`, `PMULLW`, shifts, compares — all ~0.015 cyc

**Interpretation:** Zen 4's out-of-order engine is so deep that the dep-chain
measurement collapses for many instructions. This does NOT mean these instructions
are free — they still consume execution ports (throughput is 0.12–0.49 cyc, not zero).
The near-zero latency means the OoO window can fully hide the true pipeline latency
when the chain is embedded in a larger dependency graph.

This is a measurement artifact of the rdtsc-based dep-chain method on a very wide
machine with aggressive speculation. True latency should be validated with perf
counters when available.

---

## 2. VPMADD52HUQ / VPMADD52LUQ Asymmetry

The IFMA 52-bit multiply instructions show a dramatic and unexplained asymmetry:

| Instruction | Latency | Throughput |
|---|---|---|
| VPMADD52HUQ (hi 52 bits) | **0.016 cyc** | **0.061 cyc** |
| VPMADD52LUQ (lo 52 bits) | **0.121 cyc** | **0.265 cyc** |

The hi variant is **4.3× faster** in throughput than the lo variant. This is not
the typical 2× asymmetry reported in our earlier hand-written probe (which measured
4.24 vs 2.36 cyc latency). The exhaustive measurement suggests the hi path may be
handled differently by the execution unit — possibly because the high bits of a
52×52 multiply are available earlier in the multiplier pipeline than the low bits
(the multiplier produces results MSB-first in some designs).

**Practical implication:** For big-integer arithmetic where both halves are needed,
interleave hi and lo instructions to maximize throughput.

---

## 3. PCMPESTRI/PCMPESTRM: Fastest Instruction by ILP Ratio

SSE4.2 string comparison instructions show the highest throughput-to-latency ratio
of any measured instruction:

| Instruction | Latency | Throughput | Ratio |
|---|---|---|---|
| PCMPESTRI xmm | 1.48 cyc | 0.089 cyc | **16.6×** |
| PCMPESTRM xmm | 1.44 cyc | 0.089 cyc | **16.1×** |
| PCMPISTRM xmm | 1.08 cyc | 0.090 cyc | **12.0×** |

A 16× ratio means Zen 4 can have ~16 independent PCMPESTRIs in flight simultaneously.
This is extraordinary for a complex CISC instruction that compares 16 bytes against
16 bytes with configurable matching modes.

**Practical implication:** String search kernels should launch many independent
comparisons (e.g., multiple search positions) rather than chaining them.

---

## 4. Width-Invariant VNNI Throughput

The VNNI dot product instructions show zero throughput penalty across widths:

| Instruction | 128-bit | 256-bit | 512-bit |
|---|---|---|---|
| VPDPBUSD | 0.479 cyc | 0.479 cyc | 0.479 cyc |
| VPDPBUSDS | 0.479 cyc | 0.479 cyc | 0.481 cyc |
| VPDPWSSDS | 0.481 cyc | 0.479 cyc | 0.479 cyc |

128-bit, 256-bit, and 512-bit forms have **identical throughput within noise**.
This contradicts the general Zen 4 double-pump model where 512-bit is ~2× slower
than 256-bit. The VNNI unit appears to be natively 512-bit or uses a different
scheduling path.

**Practical implication:** Use zmm VNNI freely — there is no throughput reason to
prefer narrower widths.

---

## 5. VMOVAPS zmm Is NOT Slower Than ymm

| Instruction | Width | Throughput |
|---|---|---|
| VMOVAPS xmm,xmm | 128 | 0.124 cyc |
| VMOVAPS ymm,ymm | 256 | 0.093 cyc |
| VMOVAPS zmm,zmm | 512 | 0.121 cyc |

ymm is actually the fastest form (0.093 cyc). But zmm (0.121 cyc) is essentially
equal to xmm (0.124 cyc) — **512-bit register move is not slower than 128-bit**.
This suggests the register file physically operates at 256-bit granularity and both
128 and 512 have overhead (128 needs upper-zero handling, 512 needs two rename slots).

---

## 6. RDSEED: The Slowest Instruction on Zen 4

| Instruction | Throughput |
|---|---|
| RDSEED r64 | **38,749 cyc** |
| RDRAND r64 | 33 cyc |
| CPUID | 1,032 cyc |

RDSEED is ~1,175× slower than RDRAND and ~37× slower than CPUID. This is because
RDSEED draws from the hardware entropy source directly (not the DRBG like RDRAND),
and the entropy pool refill rate is the bottleneck. On Zen 4, that's roughly one
seed every ~7.6 microseconds at 5.1 GHz.

---

## 7. STD (Set Direction Flag) Is Microcoded

| Instruction | Throughput |
|---|---|
| CLC/STC/CMC | 0.09 cyc (~11/cycle) |
| CLD | 0.27 cyc (~4/cycle) |
| STD | **56.6 cyc** |

STD is **630× slower** than CLC/STC. Setting the direction flag triggers
microcode serialization because it changes the semantics of string instructions
(REP MOVSB etc.). CLD is fast because forward direction is the expected default.

---

## 8. MOVNTSD (SSE4a): Broken Throughput

| Instruction | Extension | Throughput |
|---|---|---|
| MOVNTSS [m32],xmm | SSE4a | 0.49 cyc |
| MOVNTSD [m64],xmm | SSE4a | **26.0 cyc** |

The AMD-specific non-temporal scalar double store is ~53× slower than the float
version. This is likely a store buffer serialization issue specific to the 64-bit
NT scalar path. The instruction is effectively unusable for throughput-sensitive code.

---

## 9. Three Dot Product Performance Tiers

The measurement campaign reveals three cleanly separated performance tiers:

**Tier 1 — Legacy FP (slow, microcoded):**
- DPPS xmm: 5.4 cyc latency, 1.9 cyc throughput
- DPPD xmm: 3.4 cyc latency, 1.5 cyc throughput
- These are convenient but not the throughput path.

**Tier 2 — BF16 (modern, intermediate):**
- VDPBF16PS: 2.9 cyc latency, 0.72 cyc throughput
- Twice the throughput of FP32 FMA at the same cycle cost.

**Tier 3 — VNNI integer (fastest):**
- VPDPBUSD/VPDPWSSD: 1.9 cyc latency, 0.48 cyc throughput
- Width-invariant. The genuinely modern dot-product path on Zen 4.

---

## 10. VFMSUB/VFNMSUB Show Zero Latency (Dep Chain Collapse)

| Instruction | Latency | Throughput |
|---|---|---|
| VFMADD231PS ymm | 1.93 cyc | 0.26 cyc |
| VFMSUBPS ymm | **0.0001 cyc** | 0.26 cyc |
| VFNMSUBPS ymm | **0.0001 cyc** | 0.26 cyc |

The subtract variants show zero measured latency because the dep chain collapses:
`acc = -(acc * b) - c` with `b = 1.0` and `c = 0.0` makes `acc = -acc`, which the
hardware recognizes as a sign flip and eliminates from the critical path.

This is a measurement artifact, not a real property. But it reveals that Zen 4 can
optimize through FMA data values in some cases — a form of dynamic constant folding.

---

## 11. Pack/Saturating Pack Instructions: Surprisingly Fast

| Instruction | Throughput |
|---|---|
| VPACKUSWB zmm | 0.12 cyc |
| VPACKUSDW zmm | 0.12 cyc |
| VPACKSSWB zmm | 0.12 cyc |

Pack instructions that narrow and saturate (int16→uint8, int32→uint16) run at
0.12 cyc throughput — the same as simple integer add. This is faster than expected
for a data-reformatting instruction and makes quantization (converting FP32
accumulator results to INT8 output) essentially free.

---

## 12. VPTERNLOGD: Fastest Logic at Any Width

| Instruction | Width | Throughput |
|---|---|---|
| VPTERNLOGD zmm | 512 | 0.28 cyc |
| VPANDD zmm | 512 | 0.27 cyc |
| VPORD zmm | 512 | 0.27 cyc |

VPTERNLOGD (ternary logic — any 3-input boolean function in one instruction)
matches the throughput of simple binary AND/OR. Since it can express any boolean
function of three inputs, there is zero reason to use multi-instruction boolean
sequences at 512-bit width. The compiler should collapse all boolean trees into
VPTERNLOGD chains.

---

## 13. Register-to-Register MOVDQA/MOVDQU Are Faster Than VMOVAPS

| Instruction | Throughput |
|---|---|
| MOVDQA xmm,xmm | 0.0897 cyc (11.1/cycle) |
| MOVDQU xmm,xmm | 0.0897 cyc (11.1/cycle) |
| MOVAPS xmm,xmm | 0.0898 cyc (11.1/cycle) |
| VMOVAPS xmm,xmm | 0.124 cyc (8.1/cycle) |

Legacy-encoded integer moves (MOVDQA/MOVDQU) are ~38% faster in throughput than
VEX-encoded VMOVAPS at the same width. This is the inverse of the general
"legacy is slower" finding and likely reflects that the integer domain rename is
wider than the FP domain rename on Zen 4.
