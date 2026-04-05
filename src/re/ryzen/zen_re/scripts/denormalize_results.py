#!/usr/bin/env python3
"""
Denormalization pipeline for the Zen 4 measurement campaign.

Reads all 12 CSV result files from results/zen4_*.csv, normalizes them to a
unified schema, deduplicates, and writes:
  - results/zen4_master_denormalized.csv  (the master table)
  - results/zen4_coverage_gaps.txt        (gap report vs known Zen 4 ISA)

Run from src/re/ryzen/zen_re/:
    python3 scripts/denormalize_results.py
"""

import csv
import glob
import os
import re
import sys
from collections import defaultdict
from pathlib import Path


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parents[5]   # steinmarder/
RESULTS_DIR = REPO_ROOT / "results"
MASTER_CSV = RESULTS_DIR / "zen4_master_denormalized.csv"
GAP_REPORT = RESULTS_DIR / "zen4_coverage_gaps.txt"

UNIFIED_COLUMNS = [
    "mnemonic",
    "operands",
    "width_bits",
    "element_type",
    "latency_cycles",
    "throughput_cycles",
    "category",
    "extension",
    "source_file",
    "mode_64bit",
    "encoding",
]

# Instructions invalid in 64-bit mode (removed from AMD64)
INVALID_64BIT_MNEMONICS = frozenset([
    "AAA", "AAS", "AAD", "AAM", "DAA", "DAS",
    "INTO", "PUSHA", "PUSHAD", "POPA", "POPAD",
    "ARPL", "LDS", "LES", "BOUND",
])

# Source-file priority for deduplication (higher number wins).
# More specialised / later probes override earlier / broader ones.
SOURCE_PRIORITY = {
    "zen4_scalar_exhaustive":       10,
    "zen4_scalar_memory_exhaustive": 15,
    "zen4_legacy_exhaustive":       20,
    "zen4_prefetchw":               25,
    "zen4_x87_exhaustive":          30,
    "zen4_x87_complete":            35,
    "zen4_simd_int_exhaustive":     40,
    "zen4_simd_fp_exhaustive":      45,
    "zen4_avx512_int_exhaustive":   50,
    "zen4_avx512_fp_exhaustive":    55,
    "zen4_avx512_mask_exhaustive":  60,
    "zen4_dotproduct_complete":     70,
}

# Known Zen 4 instruction set (Agner Fog table structure) grouped by extension.
# This is a representative set for the gap report, not an exhaustive ISA ref.
KNOWN_ZEN4_INSTRUCTIONS = {
    "x86": [
        "MOV", "MOVZX", "MOVSX", "MOVSXD", "CMOVNE", "CMOVE", "CMOVB",
        "CMOVBE", "CMOVL", "CMOVLE", "LEA", "XCHG", "PUSH", "POP",
        "ADD", "SUB", "ADC", "SBB", "CMP", "INC", "DEC", "NEG",
        "IMUL", "MUL", "DIV", "IDIV",
        "AND", "OR", "XOR", "NOT", "TEST",
        "SHL", "SHR", "SAR", "ROL", "ROR", "RCL", "RCR", "SHLD", "SHRD",
        "BT", "BTS", "BTR", "BTC", "BSF", "BSR",
        "SETNE", "SETE", "SETB", "SETBE", "SETL", "SETLE",
        "LAHF", "SAHF", "CLC", "STC", "CMC", "CLD", "STD",
        "CBW", "CWDE", "CDQE", "CQO", "CDQ", "CWD", "BSWAP",
        "NOP", "LFENCE", "SFENCE", "MFENCE",
        "RDTSC", "RDTSCP", "CPUID", "RDRAND", "RDSEED",
    ],
    "BMI1": ["ANDN", "BEXTR", "BLSI", "BLSR", "BLSMSK", "TZCNT", "LZCNT"],
    "BMI2": ["BZHI", "PDEP", "PEXT", "MULX", "RORX", "SARX", "SHLX", "SHRX"],
    "ABM": ["POPCNT", "LZCNT"],
    "MMX": [
        "MOVD", "MOVQ", "PADDB", "PADDW", "PADDD", "PSUBB", "PSUBW",
        "PSUBD", "PADDUSB", "PADDUSW", "PADDSB", "PADDSW",
        "PMULLW", "PMULHW", "PMADDWD",
        "PAND", "PANDN", "POR", "PXOR",
        "PSLLW", "PSLLD", "PSLLQ", "PSRLW", "PSRLD", "PSRLQ",
        "PSRAW", "PSRAD",
        "PCMPEQB", "PCMPEQW", "PCMPEQD", "PCMPGTB", "PCMPGTW", "PCMPGTD",
        "PUNPCKHBW", "PUNPCKHWD", "PUNPCKHDQ",
        "PUNPCKLBW", "PUNPCKLWD", "PUNPCKLDQ",
        "PACKUSWB", "PACKSSWB", "PACKSSDW", "EMMS",
    ],
    "SSE": ["PREFETCHT0", "PREFETCHT1", "PREFETCHT2", "PREFETCHNTA"],
    "SSE2": [
        "ADDPS", "ADDPD", "ADDSS", "ADDSD",
        "SUBPS", "SUBPD", "SUBSS", "SUBSD",
        "MULPS", "MULPD", "MULSS", "MULSD",
        "DIVPS", "DIVPD", "DIVSS", "DIVSD",
        "SQRTPS", "SQRTPD", "SQRTSS", "SQRTSD",
        "MAXPS", "MAXPD", "MAXSS", "MAXSD",
        "MINPS", "MINPD", "MINSS", "MINSD",
        "RCPPS", "RCPSS", "RSQRTPS", "RSQRTSS",
        "CMPPS", "CMPPD", "CMPSS", "CMPSD",
        "CVTPS2PD", "CVTPD2PS", "CVTDQ2PS", "CVTPS2DQ",
        "CVTTPS2DQ", "CVTPD2DQ", "CVTTPD2DQ", "CVTDQ2PD",
        "CVTSI2SS", "CVTSI2SD", "CVTSS2SI", "CVTSD2SI",
        "MOVAPS", "MOVAPD", "MOVUPS", "MOVUPD", "MOVSS", "MOVSD",
        "MOVHPS", "MOVHPD", "MOVLPS", "MOVLPD",
        "MOVMSKPS", "MOVMSKPD",
        "ANDPS", "ANDPD", "ANDNPS", "ANDNPD",
        "ORPS", "ORPD", "XORPS", "XORPD",
        "SHUFPS", "SHUFPD", "UNPCKLPS", "UNPCKHPS",
        "UNPCKLPD", "UNPCKHPD",
        "PAND", "PANDN", "POR", "PXOR",
        "PADDB", "PADDW", "PADDD", "PADDQ",
        "PSUBB", "PSUBW", "PSUBD", "PSUBQ",
        "PMULLW", "PMULHW", "PMULHUW", "PMULUDQ",
        "PSADBW", "PAVGB", "PAVGW",
        "PCMPEQB", "PCMPEQW", "PCMPEQD",
        "PCMPGTB", "PCMPGTW", "PCMPGTD",
        "PACKUSWB", "PACKSSWB", "PACKSSDW",
        "PUNPCKLBW", "PUNPCKLWD", "PUNPCKLDQ", "PUNPCKLQDQ",
        "PUNPCKHBW", "PUNPCKHWD", "PUNPCKHDQ", "PUNPCKHQDQ",
        "PSHUFD", "PSHUFHW", "PSHUFLW",
        "PSLLW", "PSLLD", "PSLLQ", "PSRLW", "PSRLD", "PSRLQ",
        "PSRAW", "PSRAD",
        "PMADDWD",
    ],
    "SSSE3": [
        "PSHUFB", "PALIGNR",
        "PABSB", "PABSW", "PABSD",
        "PSIGNB", "PSIGNW", "PSIGND",
        "PMADDUBSW", "PHADDW", "PHADDD", "PHSUBW", "PHSUBD",
        "PMULHRSW",
    ],
    "SSE4.1": [
        "PMULLD", "PMULDQ", "PMINSB", "PMINSW", "PMINSD",
        "PMAXSB", "PMAXSW", "PMAXSD",
        "PMINUB", "PMINUW", "PMINUD",
        "PMAXUB", "PMAXUW", "PMAXUD",
        "PCMPEQQ",
        "PACKUSDW",
        "PMOVZXBW", "PMOVZXBD", "PMOVZXBQ",
        "PMOVZXWD", "PMOVZXWQ", "PMOVZXDQ",
        "PMOVSXBW", "PMOVSXBD", "PMOVSXBQ",
        "PMOVSXWD", "PMOVSXWQ", "PMOVSXDQ",
        "ROUNDPS", "ROUNDPD", "ROUNDSS", "ROUNDSD",
        "BLENDPS", "BLENDPD", "BLENDVPS", "BLENDVPD",
        "INSERTPS", "EXTRACTPS",
        "PBLENDW", "PBLENDVB",
        "PINSRB", "PINSRW", "PINSRD", "PINSRQ",
        "PEXTRB", "PEXTRW", "PEXTRD", "PEXTRQ",
        "DPPD", "DPPS", "MPSADBW",
        "PHMINPOSUW",
        "PTEST",
    ],
    "SSE4.2": [
        "PCMPESTRI", "PCMPESTRM", "PCMPISTRI", "PCMPISTRM",
        "PCMPGTQ", "CRC32",
    ],
    "SSE4a": ["EXTRQ", "INSERTQ", "MOVNTSS", "MOVNTSD"],
    "AVX": [
        "VADDPS", "VADDPD", "VADDSS", "VADDSD",
        "VSUBPS", "VSUBPD", "VSUBSS", "VSUBSD",
        "VMULPS", "VMULPD", "VMULSS", "VMULSD",
        "VDIVPS", "VDIVPD", "VDIVSS", "VDIVSD",
        "VSQRTPS", "VSQRTPD", "VSQRTSS", "VSQRTSD",
        "VMAXPS", "VMAXPD", "VMAXSS", "VMAXSD",
        "VMINPS", "VMINPD", "VMINSS", "VMINSD",
        "VRCPPS", "VRCPSS", "VRSQRTPS", "VRSQRTSS",
        "VROUNDPS", "VROUNDPD", "VROUNDSS", "VROUNDSD",
        "VCMPPS", "VCMPPD", "VCMPSS", "VCMPSD",
        "VCVTPS2PD", "VCVTPD2PS", "VCVTDQ2PS", "VCVTPS2DQ",
        "VCVTTPS2DQ", "VCVTPD2DQ", "VCVTTPD2DQ", "VCVTDQ2PD",
        "VCVTSI2SS", "VCVTSI2SD", "VCVTSS2SI", "VCVTSD2SI",
        "VMOVAPS", "VMOVAPD", "VMOVUPS", "VMOVUPD",
        "VMOVSS", "VMOVSD", "VMOVHPS", "VMOVHPD",
        "VMOVMSKPS", "VMOVMSKPD",
        "VANDPS", "VANDPD", "VANDNPS", "VANDNPD",
        "VORPS", "VORPD", "VXORPS", "VXORPD",
        "VSHUFPS", "VSHUFPD",
        "VUNPCKLPS", "VUNPCKHPS", "VUNPCKLPD", "VUNPCKHPD",
        "VPERMILPS", "VPERMILPD", "VPERM2F128",
        "VINSERTF128", "VEXTRACTF128",
        "VBROADCASTSS", "VBROADCASTSD", "VBROADCASTF128",
        "VBLENDPS", "VBLENDPD", "VBLENDVPS", "VBLENDVPD",
        "VTESTPS", "VTESTPD",
        "VDPPS",
        "VZEROUPPER", "VZEROALL",
    ],
    "AVX2": [
        "VPADDB", "VPADDW", "VPADDD", "VPADDQ",
        "VPSUBB", "VPSUBW", "VPSUBD", "VPSUBQ",
        "VPMULLW", "VPMULLD", "VPMULDQ", "VPMULUDQ",
        "VPMULHW", "VPMULHUW",
        "VPMADDWD", "VPMADDUBSW",
        "VPADDUSB", "VPADDUSW", "VPADDSB", "VPADDSW",
        "VPSUBUSB", "VPSUBUSW", "VPSUBSB", "VPSUBSW",
        "VPHADDW", "VPHADDD", "VPHSUBW", "VPHSUBD",
        "VPSADBW", "VPAVGB", "VPAVGW",
        "VPABSB", "VPABSW", "VPABSD",
        "VPMINSB", "VPMINSW", "VPMINSD",
        "VPMAXSB", "VPMAXSW", "VPMAXSD",
        "VPMINUB", "VPMINUW", "VPMINUD",
        "VPMAXUB", "VPMAXUW", "VPMAXUD",
        "VPAND", "VPANDN", "VPOR", "VPXOR",
        "VPCMPEQB", "VPCMPEQW", "VPCMPEQD", "VPCMPEQQ",
        "VPCMPGTB", "VPCMPGTW", "VPCMPGTD", "VPCMPGTQ",
        "VPSHUFB", "VPSHUFD", "VPSHUFHW", "VPSHUFLW",
        "VPERMD", "VPERMQ", "VPERM2I128",
        "VPALIGNR",
        "VPUNPCKLBW", "VPUNPCKHBW", "VPUNPCKLWD", "VPUNPCKHWD",
        "VPUNPCKLDQ", "VPUNPCKHDQ", "VPUNPCKLQDQ", "VPUNPCKHQDQ",
        "VPACKUSWB", "VPACKSSWB", "VPACKUSDW", "VPACKSSDW",
        "VPSLLW", "VPSLLD", "VPSLLQ", "VPSRLW", "VPSRLD", "VPSRLQ",
        "VPSRAW", "VPSRAD",
        "VPSLLVD", "VPSLLVQ", "VPSRLVD", "VPSRLVQ", "VPSRAVD",
        "VPBROADCASTB", "VPBROADCASTW", "VPBROADCASTD", "VPBROADCASTQ",
        "VPMOVZXBW", "VPMOVZXBD", "VPMOVZXBQ",
        "VPMOVZXWD", "VPMOVZXWQ", "VPMOVZXDQ",
        "VPMOVSXBW", "VPMOVSXBD", "VPMOVSXBQ",
        "VPMOVSXWD", "VPMOVSXWQ", "VPMOVSXDQ",
        "VPGATHERDD", "VPGATHERDQ", "VPGATHERQD", "VPGATHERQQ",
        "VPMOVMSKB",
        "VPTEST",
        "VPBLENDD",
        "VINSERTI128", "VEXTRACTI128",
        "VMPSADBW",
        "VPMASKMOVD", "VPMASKMOVQ",
    ],
    "FMA": [
        "VFMADD132PS", "VFMADD213PS", "VFMADD231PS",
        "VFMADD132PD", "VFMADD213PD", "VFMADD231PD",
        "VFMADD132SS", "VFMADD213SS", "VFMADD231SS",
        "VFMADD132SD", "VFMADD213SD", "VFMADD231SD",
        "VFMSUB132PS", "VFMSUB213PS", "VFMSUB231PS",
        "VFMSUB132PD", "VFMSUB213PD", "VFMSUB231PD",
        "VFMSUB132SS", "VFMSUB213SS", "VFMSUB231SS",
        "VFMSUB132SD", "VFMSUB213SD", "VFMSUB231SD",
        "VFNMADD132PS", "VFNMADD213PS", "VFNMADD231PS",
        "VFNMADD132PD", "VFNMADD213PD", "VFNMADD231PD",
        "VFNMADD132SS", "VFNMADD213SS", "VFNMADD231SS",
        "VFNMADD132SD", "VFNMADD213SD", "VFNMADD231SD",
        "VFNMSUB132PS", "VFNMSUB213PS", "VFNMSUB231PS",
        "VFNMSUB132PD", "VFNMSUB213PD", "VFNMSUB231PD",
        "VFNMSUB132SS", "VFNMSUB213SS", "VFNMSUB231SS",
        "VFNMSUB132SD", "VFNMSUB213SD", "VFNMSUB231SD",
        "VFMADDSUB132PS", "VFMADDSUB213PS", "VFMADDSUB231PS",
        "VFMADDSUB132PD", "VFMADDSUB213PD", "VFMADDSUB231PD",
        "VFMSUBADD132PS", "VFMSUBADD213PS", "VFMSUBADD231PS",
        "VFMSUBADD132PD", "VFMSUBADD213PD", "VFMSUBADD231PD",
    ],
    "FMA4": [
        "VFMADDPS", "VFMADDPD", "VFMADDSS", "VFMADDSD",
        "VFMSUBPS", "VFMSUBPD", "VFMSUBSS", "VFMSUBSD",
        "VFNMADDPS", "VFNMADDPD", "VFNMADDSS", "VFNMADDSD",
        "VFNMSUBPS", "VFNMSUBPD", "VFNMSUBSS", "VFNMSUBSD",
        "VFMADDSUBPS", "VFMADDSUBPD",
        "VFMSUBADDPS", "VFMSUBADDPD",
    ],
    "F16C": ["VCVTPH2PS", "VCVTPS2PH"],
    "3DNow!_PREFETCH": ["PREFETCHW"],
    "x87": [
        "FLD", "FST", "FSTP", "FXCH",
        "FCMOVB", "FCMOVE", "FCMOVBE", "FCMOVNB", "FCMOVNE",
        "FCMOVNBE", "FCMOVU", "FCMOVNU",
        "FILD", "FISTP", "FISTTP",
        "FADD", "FSUB", "FMUL", "FDIV", "FABS", "FCHS", "FRNDINT",
        "FADDP", "FSUBP", "FSUBRP", "FMULP", "FDIVP", "FDIVRP",
        "FSUBR", "FDIVR",
        "FIADD", "FISUB", "FISUBR", "FIMUL", "FIDIV", "FIDIVR",
        "FCOM", "FCOMP", "FCOMPP", "FCOMI", "FCOMIP",
        "FUCOM", "FUCOMP", "FUCOMPP", "FUCOMI", "FUCOMIP",
        "FTST", "FXAM",
        "FSQRT", "FSIN", "FCOS", "FSINCOS", "FPTAN", "FPATAN",
        "F2XM1", "FYL2X", "FYL2XP1", "FSCALE", "FXTRACT",
        "FPREM", "FPREM1",
        "FLD1", "FLDZ", "FLDPI", "FLDL2E", "FLDL2T", "FLDLN2", "FLDLG2",
        "FLDCW", "FNSTCW", "FNSTSW", "FLDENV", "FNSTENV",
        "FNSAVE", "FRSTOR", "FNINIT", "FNCLEX",
        "FINCSTP", "FDECSTP", "FFREE", "FNOP", "FWAIT",
        "FBLD", "FBSTP",
        "FICOM", "FICOMP",
    ],
    "AVX-512F": [
        "VADDPS", "VADDPD", "VSUBPS", "VSUBPD",
        "VMULPS", "VMULPD", "VDIVPS", "VDIVPD",
        "VMAXPS", "VMAXPD", "VMINPS", "VMINPD",
        "VSQRTPS", "VSQRTPD",
        "VRCP14PS", "VRCP14PD", "VRCP14SS",
        "VRSQRT14PS", "VRSQRT14PD", "VRSQRT14SS",
        "VREDUCEPS", "VREDUCEPD",
        "VRANGEPS", "VRANGEPD",
        "VRNDSCALEPS", "VRNDSCALEPD",
        "VSCALEFPS", "VSCALEFPD",
        "VGETEXPPS", "VGETEXPPD", "VGETEXPSS",
        "VGETMANTPS", "VGETMANTPD", "VGETMANTSS",
        "VFIXUPIMMPS", "VFIXUPIMMPD",
        "VCMPPS", "VCMPPD",
        "VFPCLASSPS", "VFPCLASSPD",
        "VCVTPS2PD", "VCVTPD2PS", "VCVTDQ2PS", "VCVTPS2DQ",
        "VCVTTPS2DQ", "VCVTPD2DQ", "VCVTTPD2DQ",
        "VCVTQQ2PD", "VCVTTPD2QQ",
        "VCVTPS2UDQ", "VCVTUDQ2PS", "VCVTPD2UDQ", "VCVTUDQ2PD",
        "VCVTPH2PS", "VCVTPS2PH",
        "VSHUFPS", "VSHUFPD",
        "VUNPCKLPS", "VUNPCKHPS", "VUNPCKLPD", "VUNPCKHPD",
        "VPERMILPS", "VPERMILPD",
        "VPERMPS", "VPERMPD",
        "VPERMI2PS", "VPERMI2PD",
        "VSHUFF32X4", "VSHUFF64X2",
        "VINSERTF32X4", "VINSERTF64X2",
        "VEXTRACTF32X4", "VEXTRACTF64X2",
        "VBLENDMPS", "VBLENDMPD",
        "VMOVAPS", "VMOVAPD",
        "VBROADCASTSS", "VBROADCASTSD",
        "VCOMPRESSPS", "VCOMPRESSPD", "VEXPANDPS", "VEXPANDPD",
        "VPADDD", "VPADDQ", "VPSUBD", "VPSUBQ",
        "VPMULLD", "VPMULDQ", "VPMULUDQ",
        "VPANDD", "VPANDND", "VPORD", "VPXORD",
        "VPTERNLOGD", "VPTERNLOGQ",
        "VPCMPEQD", "VPCMPEQQ", "VPCMPGTD", "VPCMPGTQ",
        "VPERMD", "VPERMQ",
        "VPUNPCKLDQ", "VPUNPCKHDQ", "VPUNPCKLQDQ", "VPUNPCKHQDQ",
        "VPSHUFD",
        "VPSLLD", "VPSLLQ", "VPSRLD", "VPSRLQ", "VPSRAD",
        "VPSLLVD", "VPSLLVQ", "VPSRLVD", "VPSRLVQ", "VPSRAVD",
        "VPSRAVQ", "VPROLVD", "VPROLVQ", "VPRORVD", "VPRORVQ",
        "VMOVDQA32", "VMOVDQU64",
        "VPBROADCASTD", "VPBROADCASTQ",
        "VPCOMPRESSD", "VPCOMPRESSQ", "VPEXPANDD", "VPEXPANDQ",
        "VPCONFLICTD", "VPCONFLICTQ", "VPLZCNTD", "VPLZCNTQ",
        "VPGATHERDD", "VPGATHERDQ", "VPGATHERQD", "VPGATHERQQ",
        "VPSCATTERDD", "VPSCATTERQD",
        "VPMOVZXBW", "VPMOVZXBD", "VPMOVZXBQ",
        "VPMOVZXWD", "VPMOVZXWQ", "VPMOVZXDQ",
        "VPMOVSXBW", "VPMOVSXBD", "VPMOVSXBQ",
        "VPMOVSXWD", "VPMOVSXWQ", "VPMOVSXDQ",
    ],
    "AVX-512BW": [
        "VPADDB", "VPADDW", "VPSUBB", "VPSUBW",
        "VPADDUSB", "VPADDUSW", "VPADDSB", "VPADDSW",
        "VPSUBUSB", "VPSUBUSW", "VPSUBSB", "VPSUBSW",
        "VPMULLW", "VPMULHW", "VPMULHUW",
        "VPMADDWD", "VPMADDUBSW",
        "VPSADBW", "VPAVGB", "VPAVGW",
        "VPABSB", "VPABSW",
        "VPMINSB", "VPMINSW", "VPMAXSB", "VPMAXSW",
        "VPMINUB", "VPMINUW", "VPMAXUB", "VPMAXUW",
        "VPCMPEQB", "VPCMPEQW", "VPCMPGTB", "VPCMPGTW",
        "VPSHUFB", "VPUNPCKLBW", "VPUNPCKHBW",
        "VPUNPCKLWD", "VPUNPCKHWD",
        "VPACKUSWB", "VPACKSSWB", "VPACKUSDW", "VPACKSSDW",
        "VPSLLW", "VPSRLW", "VPSRAW",
        "VPBROADCASTB", "VPBROADCASTW",
        "VPCOMPRESSB", "VPCOMPRESSW", "VPEXPANDB", "VPEXPANDW",
        "VPERMB",
    ],
    "AVX-512DQ": [
        "VPMULLQ", "VPABSQ",
        "VPMINSQ", "VPMAXSQ", "VPMINUQ", "VPMAXUQ",
        "VCVTNE2PS2BF16", "VCVTNEPS2BF16",
    ],
    "AVX-512CD": ["VPCONFLICTD", "VPCONFLICTQ", "VPLZCNTD", "VPLZCNTQ"],
    "AVX-512_VNNI": [
        "VPDPBUSD", "VPDPBUSDS", "VPDPWSSD", "VPDPWSSDS",
    ],
    "AVX-512_IFMA": ["VPMADD52LUQ", "VPMADD52HUQ"],
    "AVX-512_BF16": ["VDPBF16PS", "VCVTNE2PS2BF16", "VCVTNEPS2BF16"],
    "AVX-512_BITALG": ["VPOPCNTB", "VPOPCNTW", "VPSHUFBITQMB"],
    "AVX-512_VPOPCNTDQ": ["VPOPCNTD", "VPOPCNTQ"],
    "AVX-512_VBMI2": [
        "VPSHLDVD", "VPSHLDVQ", "VPSHLDVW",
        "VPSHRDVD", "VPSHRDVQ", "VPSHRDVW",
    ],
    "AVX-512_MASK": [
        "KADDB", "KADDW", "KADDD", "KADDQ",
        "KANDB", "KANDW", "KANDD", "KANDQ",
        "KANDNB", "KANDNW", "KANDND", "KANDNQ",
        "KORB", "KORW", "KORD", "KORQ",
        "KXORB", "KXORW", "KXORD", "KXORQ",
        "KXNORB", "KXNORW", "KXNORD", "KXNORQ",
        "KNOTB", "KNOTW", "KNOTD", "KNOTQ",
        "KSHIFTLB", "KSHIFTLW", "KSHIFTLD", "KSHIFTLQ",
        "KSHIFTRB", "KSHIFTRW", "KSHIFTRD", "KSHIFTRQ",
        "KMOVB", "KMOVW", "KMOVD", "KMOVQ",
        "KTESTB", "KTESTW", "KTESTD", "KTESTQ",
        "KORTESTB", "KORTESTW", "KORTESTD", "KORTESTQ",
    ],
    "Memory": [
        "CLFLUSH", "CLFLUSHOPT", "CLWB", "MOVNTI",
        "REP MOVSB", "REP STOSB", "REP CMPSB",
    ],
}


# ---------------------------------------------------------------------------
# Helpers: infer metadata from instruction mnemonic / operands / category
# ---------------------------------------------------------------------------

def infer_width_bits(mnemonic, operands_str, category):
    """Infer vector/scalar width from operand register names."""
    upper_operands = operands_str.upper()
    upper_mnemonic = mnemonic.upper()

    if "ZMM" in upper_operands or "ZMM" in upper_mnemonic:
        return 512
    if "YMM" in upper_operands or "YMM" in upper_mnemonic:
        return 256
    if "XMM" in upper_operands or "XMM" in upper_mnemonic:
        return 128
    if upper_operands.startswith("K,") or upper_operands.startswith("K ") or upper_operands == "K":
        return 0  # mask registers are scalar-width operationally
    if "MM," in upper_operands or upper_operands.startswith("MM") or ",MM" in upper_operands:
        return 64  # MMX
    if "ST(" in upper_operands or "ST(0)" in upper_operands or "ST(1)" in upper_operands:
        return 80
    if category and ("x87" in category.lower() or "x87" in upper_mnemonic[:1] == "F"):
        return 80

    # x87 instructions start with F (but not V)
    if upper_mnemonic.startswith("F") and not upper_mnemonic.startswith("V"):
        return 80

    return 0  # scalar GPR


def infer_element_type(mnemonic, operands_str, category, precision_hint=""):
    """Infer element data type from mnemonic suffixes and context."""
    upper_mnemonic = mnemonic.upper()
    upper_category = (category or "").upper()

    # Direct precision hint from the simd_fp_exhaustive format
    if precision_hint:
        precision_map = {
            "fp32": "fp32", "fp64": "fp64", "fp16": "fp16",
            "i32": "int32", "i128": "mixed", "i256": "mixed",
            "n/a": "NA",
        }
        if precision_hint.lower() in precision_map:
            return precision_map[precision_hint.lower()]

    # BF16
    if "BF16" in upper_mnemonic:
        return "bf16"
    if "DPBF16" in upper_mnemonic:
        return "bf16"

    # x87 floating point
    if upper_mnemonic.startswith("F") and not upper_mnemonic.startswith("V"):
        return "fp80"
    if "X87" in upper_category:
        return "fp80"

    # Mask instructions
    if upper_mnemonic.startswith("K") and upper_mnemonic[1:2] in (
        "A", "O", "X", "N", "S", "M", "T"
    ):
        return "mask"

    # VNNI / IFMA — integer dot-product instructions that happen to end in
    # PS/PD/SD suffixes but are actually integer operations.
    vnni_ifma_integer_mnemonics = {
        "VPDPBUSD", "VPDPBUSDS", "VPDPWSSD", "VPDPWSSDS",
        "VPMADD52LUQ", "VPMADD52HUQ",
    }
    if upper_mnemonic in vnni_ifma_integer_mnemonics:
        return "int32"

    # AVX-512 suffixes: PS=fp32, PD=fp64, SS=fp32, SD=fp64
    if upper_mnemonic.endswith("PS") or upper_mnemonic.endswith("SS"):
        return "fp32"
    if upper_mnemonic.endswith("PD") or upper_mnemonic.endswith("SD"):
        return "fp64"

    # Packed integer suffixes
    if upper_mnemonic.endswith("B") and not upper_mnemonic.endswith("SUB"):
        # Check for integer pack/byte ops
        if any(keyword in upper_mnemonic for keyword in [
            "PADD", "PSUB", "PMIN", "PMAX", "PUNPCK", "PACK",
            "PAVG", "PABS", "PCMPEQ", "PCMPGT", "PSHUF",
            "PMOVZX", "PMOVSX", "PCOMPRESS", "PEXPAND",
            "VPOPCNT", "VPERM",
        ]):
            return "int8"

    if upper_mnemonic.endswith("W"):
        if any(keyword in upper_mnemonic for keyword in [
            "PADD", "PSUB", "PMUL", "PMIN", "PMAX", "PUNPCK",
            "PACK", "PAVG", "PABS", "PCMPEQ", "PCMPGT",
            "PSHUF", "PMOVZX", "PMOVSX", "PSLL", "PSRL", "PSRA",
            "PMADDUBS", "PMADD", "PSADB", "PCOMPRESS", "PEXPAND",
            "VPOPCNT", "VPERM", "VPSHLD", "VPSHRD",
        ]):
            return "int16"

    if upper_mnemonic.endswith("D"):
        if any(keyword in upper_mnemonic for keyword in [
            "PADD", "PSUB", "PMUL", "PMIN", "PMAX", "PUNPCK",
            "PACK", "PABS", "PCMPEQ", "PCMPGT", "PSHUF",
            "PMOVZX", "PMOVSX", "PSLL", "PSRL", "PSRA",
            "PAND", "PANDN", "POR", "PXOR", "PTERNLOG",
            "PBROADCAST", "PCOMPRESS", "PEXPAND",
            "VPOPCNT", "VPERM", "VPERMI2", "VALIGN",
            "VPROLV", "VPRORV", "VPSHLD", "VPSHRD",
            "VPCONFLICT", "VPLZCNT",
            "VPDPBUSD", "VPDPWSSD",
            "VPGATHER", "VPSCATTER",
            "VMOVDQA32",
        ]):
            return "int32"

    if upper_mnemonic.endswith("Q"):
        if any(keyword in upper_mnemonic for keyword in [
            "PADD", "PSUB", "PMUL", "PMIN", "PMAX", "PUNPCK",
            "PABS", "PCMPEQ", "PCMPGT", "PSLL", "PSRL", "PSRA",
            "PTERNLOG", "PBROADCAST", "PCOMPRESS", "PEXPAND",
            "VPOPCNT", "VPERM", "VPERMI2", "VALIGN",
            "VPROLV", "VPRORV", "VPSHLD", "VPSHRD",
            "VPCONFLICT", "VPLZCNT",
            "VPGATHER", "VMOVDQU64",
            "VPMADD52",
        ]):
            return "int64"

    # Category-based fallback
    if "FP32" in upper_category or "FP_ARITH" in upper_category:
        return "fp32"
    if "FP64" in upper_category:
        return "fp64"
    if "FMA" in upper_category and "FP" not in upper_category:
        # FMA instructions without explicit type — check mnemonic
        if "PS" in upper_mnemonic or "SS" in upper_mnemonic:
            return "fp32"
        if "PD" in upper_mnemonic or "SD" in upper_mnemonic:
            return "fp64"
        return "fp32"  # default for FMA

    # Convert instructions — try to detect from mnemonic
    if "CVT" in upper_mnemonic:
        if "PS2PD" in upper_mnemonic or "SS2SD" in upper_mnemonic:
            return "fp32"
        if "PD2PS" in upper_mnemonic or "SD2SS" in upper_mnemonic:
            return "fp64"
        if "DQ2PS" in upper_mnemonic or "DQ2PD" in upper_mnemonic:
            return "int32"
        if "PS2DQ" in upper_mnemonic or "PS2UDQ" in upper_mnemonic:
            return "fp32"
        if "PD2DQ" in upper_mnemonic or "PD2UDQ" in upper_mnemonic:
            return "fp64"
        if "PD2QQ" in upper_mnemonic or "QQ2PD" in upper_mnemonic:
            return "fp64"
        if "UDQ2PS" in upper_mnemonic or "UDQ2PD" in upper_mnemonic:
            return "int32"
        if "PH2PS" in upper_mnemonic:
            return "fp16"
        if "PS2PH" in upper_mnemonic:
            return "fp32"
        if "NE2PS2BF16" in upper_mnemonic or "NEPS2BF16" in upper_mnemonic:
            return "bf16"
        if "SI2S" in upper_mnemonic:
            return "int64"
        if "S2SI" in upper_mnemonic:
            return "fp32" if "SS2SI" in upper_mnemonic else "fp64"
        return "mixed"

    # MMX integer
    if "MMX" in upper_category:
        return "int32"

    # DOT product
    if "DPP" in upper_mnemonic or "DPPS" in upper_mnemonic:
        return "fp32"
    if "DPPD" in upper_mnemonic:
        return "fp64"
    if "MPSADBW" in upper_mnemonic:
        return "int8"

    # Shuffle / blend / move for FP
    if upper_category in ("SHUFFLE", "FP_SHUFFLE", "FP_BLEND", "FP_MOVE",
                          "MOVE", "BITWISE"):
        if "PS" in upper_mnemonic:
            return "fp32"
        if "PD" in upper_mnemonic:
            return "fp64"

    # String compare
    if "STR_CMP" in upper_category or "PCMPESTR" in upper_mnemonic or "PCMPISTR" in upper_mnemonic:
        return "int8"
    if "CRC" in upper_category or "CRC32" in upper_mnemonic:
        return "mixed"

    # Legacy SIMD integers that didn't match above
    if any(keyword in upper_category for keyword in [
        "ARITH", "LOGIC", "SHIFT", "COMPARE", "SHUFFLE", "UNPACK",
        "PACK", "MISC",
    ]):
        if "MM" in (operands_str or "").upper() and not upper_mnemonic.startswith("V"):
            return "int32"  # legacy MMX/SSE integers

    return "NA"


def infer_extension(mnemonic, operands_str, width_bits, category,
                    explicit_extension=""):
    """Infer ISA extension from mnemonic, width, and context."""
    if explicit_extension and explicit_extension.upper() not in ("", "NA"):
        return explicit_extension

    upper_mnemonic = mnemonic.upper()
    upper_category = (category or "").upper()
    upper_operands = (operands_str or "").upper()

    # x87
    if upper_mnemonic.startswith("F") and not upper_mnemonic.startswith("V"):
        return "x87"
    if "X87" in upper_category:
        return "x87"

    # Mask register operations
    if upper_mnemonic.startswith("K") and upper_mnemonic[1:2] in (
        "A", "O", "X", "N", "S", "M", "T"
    ):
        return "AVX-512_MASK"

    # 3DNow! prefetch
    if upper_mnemonic == "PREFETCHW":
        return "3DNow!_PREFETCH"

    # SSE4a
    if upper_mnemonic in ("EXTRQ", "INSERTQ", "MOVNTSS", "MOVNTSD"):
        return "SSE4a"
    if "SSE4A" in upper_category:
        return "SSE4a"

    # SSE4.2 string
    if upper_mnemonic.startswith("PCMPESTR") or upper_mnemonic.startswith("PCMPISTR"):
        return "SSE4.2"
    if upper_mnemonic == "CRC32":
        return "SSE4.2"

    # SSSE3
    if upper_mnemonic in ("PSIGNB", "PSIGNW", "PSIGND"):
        return "SSSE3"
    if "SSSE3" in upper_category:
        return "SSSE3"

    # SSE4.1
    if upper_mnemonic in ("PHMINPOSUW", "DPPD", "DPPS", "MPSADBW"):
        return "SSE4.1"
    if "SSE41" in upper_category:
        return "SSE4.1"

    # AVX-512 BF16
    if "BF16" in upper_mnemonic:
        return "AVX-512_BF16"

    # AVX-512 VNNI
    if upper_mnemonic in ("VPDPBUSD", "VPDPBUSDS", "VPDPWSSD", "VPDPWSSDS"):
        return "AVX-512_VNNI"

    # AVX-512 IFMA
    if upper_mnemonic in ("VPMADD52LUQ", "VPMADD52HUQ"):
        return "AVX-512_IFMA"

    # AVX-512 BITALG
    if upper_mnemonic in ("VPSHUFBITQMB",):
        return "AVX-512_BITALG"
    if upper_mnemonic in ("VPOPCNTB", "VPOPCNTW"):
        return "AVX-512_BITALG"

    # AVX-512 VPOPCNTDQ
    if upper_mnemonic in ("VPOPCNTD", "VPOPCNTQ"):
        return "AVX-512_VPOPCNTDQ"

    # AVX-512 VBMI2
    if upper_mnemonic in (
        "VPSHLDVD", "VPSHLDVQ", "VPSHLDVW",
        "VPSHRDVD", "VPSHRDVQ", "VPSHRDVW",
        "VPCOMPRESSB", "VPCOMPRESSW", "VPEXPANDB", "VPEXPANDW",
    ):
        return "AVX-512_VBMI2"

    # AVX-512 CD
    if upper_mnemonic in ("VPCONFLICTD", "VPCONFLICTQ", "VPLZCNTD", "VPLZCNTQ"):
        return "AVX-512CD"

    # AVX-512 VBMI
    if upper_mnemonic in ("VPERMB", "VPERMI2B"):
        return "AVX-512_VBMI"

    # zmm or mask-destination → AVX-512F / AVX-512BW / AVX-512DQ
    if width_bits == 512 or "ZMM" in upper_operands:
        # Byte/word operations → BW
        byte_word_instructions = {
            "VPADDB", "VPADDW", "VPSUBB", "VPSUBW",
            "VPADDUSB", "VPADDUSW", "VPADDSB", "VPADDSW",
            "VPSUBUSB", "VPSUBUSW", "VPSUBSB", "VPSUBSW",
            "VPMULLW", "VPMULHW", "VPMULHUW",
            "VPMADDWD", "VPMADDUBSW",
            "VPSADBW", "VPAVGB", "VPAVGW",
            "VPABSB", "VPABSW",
            "VPMINSB", "VPMINSW", "VPMAXSB", "VPMAXSW",
            "VPMINUB", "VPMINUW", "VPMAXUB", "VPMAXUW",
            "VPCMPEQB", "VPCMPEQW", "VPCMPGTB", "VPCMPGTW",
            "VPSHUFB", "VPUNPCKLBW", "VPUNPCKHBW",
            "VPUNPCKLWD", "VPUNPCKHWD",
            "VPACKUSWB", "VPACKSSWB", "VPACKUSDW", "VPACKSSDW",
            "VPSLLW", "VPSRLW", "VPSRAW",
            "VPBROADCASTB", "VPBROADCASTW",
        }
        if upper_mnemonic in byte_word_instructions:
            return "AVX-512BW"

        # 64-bit integer ops → DQ
        dq_only_instructions = {"VPMULLQ", "VPABSQ", "VPMINSQ", "VPMAXSQ",
                                "VPMINUQ", "VPMAXUQ", "VPSRAQ"}
        if upper_mnemonic in dq_only_instructions:
            return "AVX-512DQ"

        return "AVX-512F"

    # FMA4 (AMD-specific 4-operand FMA)
    fma4_mnemonics = {
        "VFMADDPS", "VFMADDPD", "VFMADDSS", "VFMADDSD",
        "VFMSUBPS", "VFMSUBPD", "VFMSUBSS", "VFMSUBSD",
        "VFNMADDPS", "VFNMADDPD", "VFNMADDSS", "VFNMADDSD",
        "VFNMSUBPS", "VFNMSUBPD", "VFNMSUBSS", "VFNMSUBSD",
        "VFMADDSUBPS", "VFMADDSUBPD",
        "VFMSUBADDPS", "VFMSUBADDPD",
    }
    if upper_mnemonic in fma4_mnemonics:
        return "FMA4"

    # FMA3 (Intel 3-operand FMA)
    if re.match(r"VF(N?M(ADD|SUB|ADDSUB|SUBADD))\d{3}[PpSs][SDsd]", upper_mnemonic):
        return "FMA"

    # F16C
    if upper_mnemonic in ("VCVTPH2PS", "VCVTPS2PH"):
        if width_bits <= 256:
            return "F16C"

    # V-prefixed xmm/ymm → VEX-encoded AVX or AVX2
    if upper_mnemonic.startswith("V"):
        # AVX2 integer instructions
        avx2_int_prefixes = (
            "VPADD", "VPSUB", "VPMUL", "VPMIN", "VPMAX",
            "VPAND", "VPANDN", "VPOR", "VPXOR",
            "VPCMPEQ", "VPCMPGT",
            "VPSHUF", "VPALIGNR", "VPERM2I128",
            "VPUNPCK", "VPACK",
            "VPSLL", "VPSRL", "VPSRA",
            "VPABS", "VPAVG", "VPSADB",
            "VPMADD", "VPMOVZX", "VPMOVSX",
            "VPBROADCAST", "VPGATHER",
            "VPBLEND", "VPTEST", "VPMOVMSK",
            "VPEXTR", "VPINSR",
        )
        if any(upper_mnemonic.startswith(prefix) for prefix in avx2_int_prefixes):
            return "AVX2"

        # VPERMD is AVX2
        if upper_mnemonic == "VPERMD":
            return "AVX2"

        return "AVX"

    # SSE prefetch
    if upper_mnemonic in ("PREFETCHT0", "PREFETCHT1", "PREFETCHT2", "PREFETCHNTA"):
        return "SSE"

    # MMX
    if "MMX" in upper_category or width_bits == 64:
        return "MMX"
    if upper_operands and "MM" in upper_operands and "XMM" not in upper_operands:
        return "MMX"
    if upper_mnemonic == "EMMS":
        return "MMX"

    # Legacy SSE (non-V-prefix xmm)
    if width_bits == 128 and not upper_mnemonic.startswith("V"):
        if "SSE4.2" in upper_category or "STR_CMP" in upper_category:
            return "SSE4.2"
        if "SSE4.1" in upper_category or "SSE41" in upper_category:
            return "SSE4.1"
        if "SSSE3" in upper_category:
            return "SSSE3"
        if "SSE" in upper_category:
            return "SSE2"
        # Default legacy 128-bit
        if upper_mnemonic.endswith("PS") or upper_mnemonic.endswith("SS"):
            return "SSE"
        if upper_mnemonic.endswith("PD") or upper_mnemonic.endswith("SD"):
            return "SSE2"
        return "SSE2"

    # BMI1 / BMI2
    bmi1_instructions = {"ANDN", "BEXTR", "BLSI", "BLSR", "BLSMSK", "TZCNT"}
    bmi2_instructions = {"BZHI", "PDEP", "PEXT", "MULX", "RORX", "SARX", "SHLX", "SHRX"}
    if upper_mnemonic in bmi1_instructions:
        return "BMI1"
    if upper_mnemonic in bmi2_instructions:
        return "BMI2"
    if upper_mnemonic in ("POPCNT", "LZCNT"):
        return "ABM"

    return "x86"


def infer_encoding(mnemonic, operands_str, width_bits, extension):
    """Infer instruction encoding format."""
    upper_mnemonic = mnemonic.upper()
    upper_operands = (operands_str or "").upper()
    upper_extension = (extension or "").upper()

    # x87
    if upper_mnemonic.startswith("F") and not upper_mnemonic.startswith("V"):
        return "x87"
    if "X87" in upper_extension:
        return "x87"

    # EVEX: AVX-512 instructions, zmm, mask registers
    if "512" in upper_extension or "AVX-512" in upper_extension:
        return "evex"
    if "ZMM" in upper_operands or width_bits == 512:
        return "evex"
    if upper_mnemonic.startswith("K") and upper_mnemonic[1:2] in (
        "A", "O", "X", "N", "S", "M", "T"
    ):
        return "evex"

    # VEX: V-prefixed xmm/ymm (AVX/AVX2/FMA)
    if upper_mnemonic.startswith("V") and ("AVX" in upper_extension or
                                           "FMA" in upper_extension or
                                           "F16C" in upper_extension):
        return "vex"
    if upper_mnemonic.startswith("V") and width_bits in (128, 256):
        return "vex"

    # Legacy SSE (non-V prefix with xmm)
    if width_bits == 128 and not upper_mnemonic.startswith("V"):
        return "legacy"
    if "XMM" in upper_operands and not upper_mnemonic.startswith("V"):
        return "legacy"

    # MMX
    if "MMX" in upper_extension or width_bits == 64:
        return "legacy"
    if upper_mnemonic == "EMMS":
        return "legacy"

    # GPR / scalar
    return "gpr"


def infer_mode_64bit(mnemonic):
    """Determine if instruction is valid in 64-bit mode."""
    if mnemonic.upper() in INVALID_64BIT_MNEMONICS:
        return "invalid"
    return "valid"


def source_key(filename):
    """Extract the base source name (without date suffix) for priority lookup."""
    # zen4_avx512_fp_exhaustive_2026-04-04.csv -> zen4_avx512_fp_exhaustive
    basename = os.path.basename(filename)
    # Remove .csv
    stem = basename.rsplit(".", 1)[0]
    # Remove trailing date pattern _YYYY-MM-DD
    date_pattern = re.compile(r"_\d{4}-\d{2}-\d{2}$")
    return date_pattern.sub("", stem)


def priority_of(filename):
    """Return priority score for a source file."""
    key = source_key(filename)
    return SOURCE_PRIORITY.get(key, 0)


# ---------------------------------------------------------------------------
# CSV Parsing — handle all the varied formats
# ---------------------------------------------------------------------------

def skip_metadata_preamble(filepath):
    """Read a CSV file, skip probe metadata lines, return (header, data_lines)."""
    with open(filepath, "r", newline="") as fh:
        raw_lines = fh.readlines()

    header_line = None
    data_start_index = 0

    for line_index, raw_line in enumerate(raw_lines):
        stripped = raw_line.strip()
        if not stripped:
            continue
        # Skip metadata lines: key=value, informational text
        if "=" in stripped and "," not in stripped.split("=", 1)[0]:
            continue
        if stripped.startswith("Measuring ") or stripped.startswith("Total "):
            continue
        if stripped.startswith("Buffer sizes"):
            continue
        # First line that looks like CSV header
        if stripped.startswith("mnemonic"):
            header_line = stripped
            data_start_index = line_index + 1
            break

    if header_line is None:
        print(f"  WARNING: no CSV header found in {filepath}, skipping")
        return None, []

    data_lines = []
    for raw_line in raw_lines[data_start_index:]:
        stripped = raw_line.strip()
        if stripped:
            data_lines.append(stripped)

    return header_line, data_lines


def detect_schema(header_line):
    """Parse the header and return a list of canonical column names."""
    raw_columns = [column.strip() for column in header_line.split(",")]
    return raw_columns


def parse_row_with_schema(raw_columns, data_line, filepath):
    """Parse a single data line according to the detected column schema.

    Returns a dict with unified column keys, or None on parse error.

    The tricky part: the operands field can contain commas (e.g., zmm,zmm,zmm).
    We need to figure out which commas are field separators vs operand separators.
    Strategy: count the expected number of non-operand fields and assign the
    remaining middle tokens to the operands field.
    """
    tokens = [token.strip() for token in data_line.split(",")]

    schema_columns = raw_columns
    schema_count = len(schema_columns)

    # Identify which columns are present
    has_operands = "operands" in schema_columns
    has_width = "width_bits" in schema_columns
    has_extension = "extension" in schema_columns
    has_precision = "precision" in schema_columns

    # Count fixed (non-operands) columns
    fixed_column_count = schema_count - (1 if has_operands else 0)

    # The number of extra tokens that belong to operands
    operand_token_count = len(tokens) - fixed_column_count

    if not has_operands:
        # All tokens map 1:1 to columns
        if len(tokens) < schema_count:
            return None
        result_mapping = {}
        for column_index, column_name in enumerate(schema_columns):
            if column_index < len(tokens):
                result_mapping[column_name] = tokens[column_index]
            else:
                result_mapping[column_name] = ""
        result_mapping["operands"] = ""  # no operands column
        return result_mapping

    # Figure out operand field boundaries
    operand_column_index = schema_columns.index("operands")

    # Columns before operands
    pre_operand_columns = schema_columns[:operand_column_index]
    # Columns after operands
    post_operand_columns = schema_columns[operand_column_index + 1:]

    # Assign pre-operand tokens
    result_mapping = {}
    for column_index, column_name in enumerate(pre_operand_columns):
        if column_index < len(tokens):
            result_mapping[column_name] = tokens[column_index]

    # Assign operand tokens (the variable-width middle)
    if operand_token_count < 0:
        operand_token_count = 0

    operand_start = len(pre_operand_columns)
    operand_end = operand_start + operand_token_count
    operand_tokens = tokens[operand_start:operand_end]
    result_mapping["operands"] = ",".join(operand_tokens) if operand_tokens else ""

    # Assign post-operand tokens
    for column_offset, column_name in enumerate(post_operand_columns):
        token_index = operand_end + column_offset
        if token_index < len(tokens):
            result_mapping[column_name] = tokens[token_index]
        else:
            result_mapping[column_name] = ""

    return result_mapping


def normalize_row(raw_row, filepath):
    """Convert a raw parsed row dict into the unified schema."""
    mnemonic = raw_row.get("mnemonic", "").strip().upper()
    if not mnemonic:
        return None

    operands = raw_row.get("operands", "").strip()
    category = raw_row.get("category", "").strip()
    precision_hint = raw_row.get("precision", "").strip()

    # Width
    raw_width = raw_row.get("width_bits", "").strip()
    if raw_width and raw_width.isdigit():
        width_bits = int(raw_width)
    else:
        width_bits = infer_width_bits(mnemonic, operands, category)

    # Latency / throughput
    try:
        latency_cycles = float(raw_row.get("latency_cycles", "-1"))
    except (ValueError, TypeError):
        latency_cycles = -1.0

    try:
        throughput_cycles = float(raw_row.get("throughput_cycles", "-1"))
    except (ValueError, TypeError):
        throughput_cycles = -1.0

    # Extension
    explicit_extension = raw_row.get("extension", "").strip()
    extension = infer_extension(mnemonic, operands, width_bits, category,
                                explicit_extension)

    # Element type
    element_type = infer_element_type(mnemonic, operands, category,
                                      precision_hint)

    # Encoding
    encoding = infer_encoding(mnemonic, operands, width_bits, extension)

    # Mode
    mode_64bit = infer_mode_64bit(mnemonic)

    # Source file
    source_basename = os.path.basename(filepath)

    return {
        "mnemonic": mnemonic,
        "operands": operands,
        "width_bits": str(width_bits),
        "element_type": element_type,
        "latency_cycles": f"{latency_cycles:.4f}",
        "throughput_cycles": f"{throughput_cycles:.4f}",
        "category": category,
        "extension": extension,
        "source_file": source_basename,
        "mode_64bit": mode_64bit,
        "encoding": encoding,
    }


# ---------------------------------------------------------------------------
# Deduplication
# ---------------------------------------------------------------------------

def dedup_key(row):
    """Create a deduplication key from mnemonic + operands + width."""
    return (row["mnemonic"], row["operands"], row["width_bits"])


def deduplicate_rows(all_rows):
    """Keep the highest-priority row for each (mnemonic, operands, width) tuple."""
    best_rows = {}
    for row in all_rows:
        key = dedup_key(row)
        if key not in best_rows:
            best_rows[key] = row
        else:
            existing_priority = priority_of(best_rows[key]["source_file"])
            new_priority = priority_of(row["source_file"])
            if new_priority >= existing_priority:
                best_rows[key] = row
    return list(best_rows.values())


# ---------------------------------------------------------------------------
# Gap Report
# ---------------------------------------------------------------------------

def build_measured_mnemonics(rows):
    """Build a set of all measured mnemonics (uppercase)."""
    measured_set = set()
    for row in rows:
        measured_set.add(row["mnemonic"].upper())
    return measured_set


def generate_gap_report(rows, output_path):
    """Write a gap report listing known Zen 4 instructions not in our data."""
    measured_mnemonics = build_measured_mnemonics(rows)

    with open(output_path, "w") as report_file:
        report_file.write("=" * 72 + "\n")
        report_file.write("Zen 4 Coverage Gap Report\n")
        report_file.write(f"Generated from {len(rows)} measured instruction forms\n")
        report_file.write(f"Measured unique mnemonics: {len(measured_mnemonics)}\n")
        report_file.write("=" * 72 + "\n\n")

        total_known_count = 0
        total_missing_count = 0

        for extension_name in sorted(KNOWN_ZEN4_INSTRUCTIONS.keys()):
            known_instruction_list = KNOWN_ZEN4_INSTRUCTIONS[extension_name]
            missing_instructions = []
            for instruction_name in sorted(set(known_instruction_list)):
                if instruction_name.upper() not in measured_mnemonics:
                    missing_instructions.append(instruction_name)

            total_known_count += len(set(known_instruction_list))
            total_missing_count += len(missing_instructions)

            if missing_instructions:
                report_file.write(f"--- {extension_name} "
                                  f"({len(missing_instructions)} missing / "
                                  f"{len(set(known_instruction_list))} known) ---\n")
                for missing_instruction in missing_instructions:
                    report_file.write(f"  {missing_instruction}\n")
                report_file.write("\n")

        coverage_percentage = (
            (1.0 - total_missing_count / total_known_count) * 100
            if total_known_count > 0 else 0
        )

        report_file.write("=" * 72 + "\n")
        report_file.write(f"Total known instruction mnemonics: {total_known_count}\n")
        report_file.write(f"Total missing: {total_missing_count}\n")
        report_file.write(f"Coverage: {coverage_percentage:.1f}%\n")
        report_file.write("=" * 72 + "\n")

    return total_missing_count


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

def print_summary(rows):
    """Print a human-readable summary of the master table."""
    total_row_count = len(rows)

    extension_counts = defaultdict(int)
    category_counts = defaultdict(int)
    latency_measured_count = 0
    throughput_measured_count = 0

    for row in rows:
        extension_counts[row["extension"]] += 1
        category_counts[row["category"]] += 1
        if float(row["latency_cycles"]) > 0:
            latency_measured_count += 1
        if float(row["throughput_cycles"]) > 0:
            throughput_measured_count += 1

    print(f"\n{'=' * 60}")
    print(f"Zen 4 Master Denormalized CSV — Summary")
    print(f"{'=' * 60}")
    print(f"Total rows: {total_row_count}")
    print()

    print(f"Rows per extension:")
    for extension_name in sorted(extension_counts.keys()):
        print(f"  {extension_name:30s} {extension_counts[extension_name]:5d}")
    print()

    print(f"Rows per category:")
    for category_name in sorted(category_counts.keys()):
        print(f"  {category_name:30s} {category_counts[category_name]:5d}")
    print()

    print(f"Latency measured:   {latency_measured_count:5d} / {total_row_count}"
          f"  ({100 * latency_measured_count / total_row_count:.1f}%)")
    print(f"Throughput measured: {throughput_measured_count:5d} / {total_row_count}"
          f"  ({100 * throughput_measured_count / total_row_count:.1f}%)")
    print(f"{'=' * 60}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    csv_pattern = str(RESULTS_DIR / "zen4_*.csv")
    csv_files = sorted(glob.glob(csv_pattern))

    # Exclude our own output if it exists from a previous run
    csv_files = [
        filepath for filepath in csv_files
        if "master_denormalized" not in filepath
    ]

    if not csv_files:
        print(f"ERROR: no CSV files found matching {csv_pattern}", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(csv_files)} source CSV files:")
    for filepath in csv_files:
        print(f"  {os.path.basename(filepath)}")

    all_normalized_rows = []

    for filepath in csv_files:
        header_line, data_lines = skip_metadata_preamble(filepath)
        if header_line is None:
            continue

        schema_columns = detect_schema(header_line)
        print(f"\n  {os.path.basename(filepath)}: "
              f"schema={schema_columns}, rows={len(data_lines)}")

        parsed_row_count = 0
        for data_line in data_lines:
            raw_row = parse_row_with_schema(schema_columns, data_line, filepath)
            if raw_row is None:
                continue
            normalized_row = normalize_row(raw_row, filepath)
            if normalized_row is None:
                continue
            all_normalized_rows.append(normalized_row)
            parsed_row_count += 1

        print(f"    -> parsed {parsed_row_count} rows")

    print(f"\nTotal rows before dedup: {len(all_normalized_rows)}")

    # Deduplicate
    deduplicated_rows = deduplicate_rows(all_normalized_rows)
    print(f"Total rows after dedup:  {len(deduplicated_rows)}")

    # Sort by: extension, category, mnemonic, width_bits
    deduplicated_rows.sort(key=lambda row: (
        row["extension"],
        row["category"],
        row["mnemonic"],
        int(row["width_bits"]),
    ))

    # Write master CSV
    with open(MASTER_CSV, "w", newline="") as output_file:
        csv_writer = csv.DictWriter(output_file, fieldnames=UNIFIED_COLUMNS)
        csv_writer.writeheader()
        csv_writer.writerows(deduplicated_rows)

    print(f"\nWrote: {MASTER_CSV}")

    # Print summary
    print_summary(deduplicated_rows)

    # Generate gap report
    missing_count = generate_gap_report(deduplicated_rows, GAP_REPORT)
    print(f"\nGap report: {GAP_REPORT}")
    print(f"  Missing mnemonics: {missing_count}")


if __name__ == "__main__":
    main()
