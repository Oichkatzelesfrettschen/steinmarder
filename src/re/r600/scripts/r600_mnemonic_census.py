#!/usr/bin/env python3
"""
r600_mnemonic_census.py — opcode emission frontier tracker.

Scans R600_DEBUG ISA disassembly output, blessed findings, and source
files to identify which Evergreen ISA mnemonics have been observed in
compiled shaders vs the full opcode inventory.

Reports:
- Observed opcodes with hit counts
- Known opcodes never observed (the "dark" frontier)
- Coverage percentage by category (ALU, CF, TEX, VTX, LDS, MEM)

Usage:
    python3 r600_mnemonic_census.py                      # auto-discover
    python3 r600_mnemonic_census.py --csv census.csv
    python3 r600_mnemonic_census.py path/to/*.stderr
"""
from __future__ import annotations

import argparse
import csv
import re
import sys
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESULTS_DIR = ROOT / "results"
FINDINGS_DIR = ROOT / "results" / "x130e_terascale"

# ── Evergreen ISA reference opcode inventory ──────────────────────────────
# Source: AMD_Evergreen-Family_ISA.pdf chapters 3-8, cross-referenced with
# Mesa src/gallium/drivers/r600/ alu_op_table and cf_native_opcodes.
#
# Categories: ALU (vec+trans), CF, TEX, VTX, LDS, MEM (RAT/scratch/ring)

KNOWN_OPCODES: dict[str, str] = {}

def _register(category: str, opcodes: list[str]) -> None:
    for op in opcodes:
        KNOWN_OPCODES[op] = category

# ALU vector slot opcodes (Chapter 3)
_register("ALU_VEC", [
    "ADD", "MUL", "MUL_IEEE", "MAX", "MIN", "MAX_DX10", "MIN_DX10",
    "SETE", "SETGT", "SETGE", "SETNE", "SETE_DX10", "SETGT_DX10",
    "SETGE_DX10", "SETNE_DX10",
    "FRACT", "TRUNC", "CEIL", "RNDNE", "FLOOR",
    "MULADD", "MULADD_IEEE", "MULADD_IEEE_D2", "MULADD_64",
    "CNDE", "CNDGT", "CNDGE", "CNDE_INT", "CNDGT_INT", "CNDGE_INT",
    "DOT4", "DOT4_IEEE", "DOT4_e",
    "CUBE",
    "MOV",
    "NOP",
    "PRED_SETE", "PRED_SETGT", "PRED_SETGE", "PRED_SETNE",
    "PRED_SETE_INT", "PRED_SETGT_INT", "PRED_SETGE_INT", "PRED_SETNE_INT",
    "PRED_SETGT_UINT", "PRED_SETGE_UINT",
    "PRED_SETE_PUSH", "PRED_SETGT_PUSH", "PRED_SETGE_PUSH", "PRED_SETNE_PUSH",
    "PRED_SETE_PUSH_INT", "PRED_SETGT_PUSH_INT", "PRED_SETGE_PUSH_INT",
    "PRED_SETNE_PUSH_INT",
    "KILLGT", "KILLGE", "KILLE", "KILLNE",
    "KILLGT_INT", "KILLGE_INT", "KILLE_INT", "KILLNE_INT",
    "KILLGT_UINT", "KILLGE_UINT",
    "GROUP_BARRIER", "GROUP_SEQ_BEGIN", "GROUP_SEQ_END",
    "SET_MODE",
    "MOVA", "MOVA_INT", "MOVA_FLOOR",
    "INTERP_XY", "INTERP_ZW", "INTERP_X", "INTERP_Z", "INTERP_LOAD_P0",
    # Integer ALU
    "ADD_INT", "SUB_INT", "MUL_UINT24", "MULLO_INT", "MULHI_INT",
    "MULLO_UINT", "MULHI_UINT",
    "AND_INT", "OR_INT", "XOR_INT", "NOT_INT",
    "LSHL_INT", "LSHR_INT", "ASHR_INT",
    "SETE_INT", "SETGT_INT", "SETGE_INT", "SETNE_INT",
    "SETGT_UINT", "SETGE_UINT",
    "MAX_INT", "MIN_INT", "MAX_UINT", "MIN_UINT",
    "BFE_INT", "BFE_UINT", "BFI_INT",
    "FLT_TO_INT", "FLT_TO_UINT", "INT_TO_FLT", "UINT_TO_FLT",
    "FLT_TO_INT_FLOOR", "FLT_TO_INT_RPI",
    # Byte extract/convert (Evergreen additions)
    "UBYTE0_FLT", "UBYTE1_FLT", "UBYTE2_FLT", "UBYTE3_FLT",
    "FLT_TO_UINT4",
    # Bit operations
    "BIT_ALIGN_INT", "BYTE_ALIGN_INT",
    "BCNT_INT", "FFBH_UINT", "FFBL_INT",
    # OP3 (three-source)
    "MULADD_UINT24",
    # Miscellaneous
    "LDEXP_64", "FRACT_64", "ADD_64", "MUL_64",
    "FLT32_TO_FLT64", "FLT64_TO_FLT32",
    "SETE_64", "SETGT_64", "SETGE_64", "SETNE_64",
])

# ALU transcendental slot opcodes (t-slot, Chapter 3)
_register("ALU_TRANS", [
    "RECIP_IEEE", "RECIP_FF", "RECIP_CLAMPED", "RECIP_INT", "RECIP_UINT",
    "RECIPSQRT_IEEE", "RECIPSQRT_FF", "RECIPSQRT_CLAMPED",
    "SQRT_IEEE", "SQRT_FF",
    "SIN", "COS",
    "LOG_IEEE", "LOG_CLAMPED",
    "EXP_IEEE",
    "MULLO_INT", "MULHI_INT", "MULLO_UINT", "MULHI_UINT",
    "MUL_LIT", "MUL_LIT_D2", "MUL_LIT_M2", "MUL_LIT_M4",
    "FLT_TO_INT", "INT_TO_FLT", "UINT_TO_FLT", "FLT_TO_UINT",
    "RECIP_64", "RECIPSQRT_64", "SQRT_64",
])

# Control Flow opcodes (Chapter 4)
_register("CF", [
    "ALU", "ALU_PUSH_BEFORE", "ALU_POP_AFTER", "ALU_POP2_AFTER",
    "ALU_EXT", "ALU_CONTINUE", "ALU_BREAK", "ALU_ELSE_AFTER",
    "TEX", "VTX", "VTX_TC", "GDS",
    "LOOP_START", "LOOP_START_DX10", "LOOP_START_NO_AL",
    "LOOP_END", "LOOP_CONTINUE", "LOOP_BREAK",
    "JUMP", "PUSH", "PUSH_ELSE", "ELSE", "POP",
    "CALL", "CALL_FS", "RETURN",
    "EMIT_VERTEX", "EMIT_CUT_VERTEX", "CUT_VERTEX",
    "KILL",
    "NOP",
    "WAIT_ACK", "HALT",
    "MEM_STREAM0_BUF0", "MEM_STREAM0_BUF1", "MEM_STREAM0_BUF2", "MEM_STREAM0_BUF3",
    "MEM_STREAM1_BUF0", "MEM_STREAM1_BUF1", "MEM_STREAM1_BUF2", "MEM_STREAM1_BUF3",
    "MEM_STREAM2_BUF0", "MEM_STREAM2_BUF1", "MEM_STREAM2_BUF2", "MEM_STREAM2_BUF3",
    "MEM_STREAM3_BUF0", "MEM_STREAM3_BUF1", "MEM_STREAM3_BUF2", "MEM_STREAM3_BUF3",
    "MEM_RING", "MEM_RING1", "MEM_RING2", "MEM_RING3",
    "MEM_SCRATCH", "MEM_REDUCTION", "MEM_EXPORT",
    "EXPORT", "EXPORT_DONE",
])

# MEM RAT opcodes (Chapter 5)
_register("MEM_RAT", [
    "MEM_RAT", "MEM_RAT_CACHELESS", "MEM_RAT_NOCACHE",
    # RAT instruction sub-opcodes
    "STORE_TYPED", "STORE_RAW", "STORE_RAW_FDENORM",
    "CMPXCHG_INT", "CMPXCHG_FLT", "CMPXCHG_FDENORM",
    "ADD_RTN", "SUB_RTN", "RSUB_RTN",
    "MIN_INT_RTN", "MAX_INT_RTN", "MIN_UINT_RTN", "MAX_UINT_RTN",
    "MIN_FLT_RTN", "MAX_FLT_RTN",
    "AND_RTN", "OR_RTN", "XOR_RTN",
    "INC_UINT_RTN", "DEC_UINT_RTN",
    "NOP_RTN",
    "WRITE_IND", "WRITE_IND_ACK",
])

# TEX opcodes (Chapter 6)
_register("TEX", [
    "TEX_SAMPLE", "TEX_SAMPLE_L", "TEX_SAMPLE_LB", "TEX_SAMPLE_LZ",
    "TEX_SAMPLE_G", "TEX_SAMPLE_G_L", "TEX_SAMPLE_G_LB", "TEX_SAMPLE_G_LZ",
    "TEX_SAMPLE_C", "TEX_SAMPLE_C_L", "TEX_SAMPLE_C_LB", "TEX_SAMPLE_C_LZ",
    "TEX_SAMPLE_C_G", "TEX_SAMPLE_C_G_L", "TEX_SAMPLE_C_G_LB",
    "TEX_LD", "TEX_GET_TEXTURE_RESINFO", "TEX_GET_NUMBER_OF_SAMPLES",
    "TEX_GET_COMP_TEX_LOD", "TEX_GET_GRADIENTS_H", "TEX_GET_GRADIENTS_V",
    "TEX_SET_OFFSETS", "TEX_KEEP_GRADIENTS",
    "TEX_SET_GRADIENTS_H", "TEX_SET_GRADIENTS_V",
    "TEX_GET_BUFFER_RESINFO",
    "TEX_GATHER4", "TEX_GATHER4_O", "TEX_GATHER4_C", "TEX_GATHER4_C_O",
])

# VTX opcodes (Chapter 7)
_register("VTX", [
    "VFETCH", "VFETCH_SEM",
    "VTX_FETCH", "VTX_READ_8", "VTX_READ_16", "VTX_READ_32",
    "VTX_READ_64", "VTX_READ_128",
])

# LDS opcodes (Chapter 8)
_register("LDS", [
    "LDS_ADD", "LDS_SUB", "LDS_RSUB",
    "LDS_INC", "LDS_DEC",
    "LDS_MIN_INT", "LDS_MAX_INT", "LDS_MIN_UINT", "LDS_MAX_UINT",
    "LDS_AND", "LDS_OR", "LDS_XOR",
    "LDS_WRXCHG", "LDS_WRXCHG2", "LDS_WRXCHG2_ST",
    "LDS_CMP_STORE", "LDS_CMP_STORE_SPF",
    "LDS_BYTE_WRITE", "LDS_SHORT_WRITE", "LDS_WRITE", "LDS_WRITE_REL",
    "LDS_BYTE_READ", "LDS_UBYTE_READ", "LDS_SHORT_READ", "LDS_USHORT_READ",
    "LDS_READ", "LDS_READ_REL", "LDS_READ_RET", "LDS_READ2_RET",
    "LDS_ADD_RET", "LDS_SUB_RET", "LDS_RSUB_RET",
    "LDS_INC_RET", "LDS_DEC_RET",
    "LDS_MIN_INT_RET", "LDS_MAX_INT_RET", "LDS_MIN_UINT_RET", "LDS_MAX_UINT_RET",
    "LDS_AND_RET", "LDS_OR_RET", "LDS_XOR_RET",
    "LDS_WRXCHG_RET", "LDS_WRXCHG2_RET", "LDS_WRXCHG2_ST_RET",
    "LDS_CMP_XCHG_RET", "LDS_CMP_XCHG_SPF_RET",
])


# ── ISA mnemonic extraction from text ─────────────────────────────────────

# R600_DEBUG ISA dump: slot-assigned ALU instruction
RE_ALU_SLOT = re.compile(
    r"(?:^\s+\d{4}\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+)"
    r"\s+\d+\s+[xyzwt]:\s+(\S+)"
)

# R600_DEBUG ISA dump: CF-level instruction
RE_CF_INST = re.compile(
    r"^(?:\d{4}\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+)\s+"
    r"(ALU\S*|TEX|VTX\S*|MEM_RAT\S*|EXPORT\S*|LOOP\S*|JUMP|PUSH|POP|ELSE|CALL\S*|RETURN|NOP|WAIT_ACK|HALT|GDS|KILL|MEM_SCRATCH|MEM_RING\S*|MEM_STREAM\S*|EMIT\S*|CUT\S*)"
)

# R600_DEBUG ISA dump: VTX/TEX fetch instruction
RE_FETCH = re.compile(
    r"(?:^\s+\d{4}\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+(?:\s+[0-9a-fA-F]+)?)\s+"
    r"(VFETCH|TEX_SAMPLE\S*|TEX_LD\S*|TEX_GATHER\S*|TEX_GET\S*|TEX_SET\S*|MEM_RAT\s+\S+)"
)

# Markdown / findings: backtick-quoted ISA mnemonics
RE_MD_OPCODE = re.compile(
    r"(?:^|\s)`?([A-Z][A-Z0-9_]{2,})`?"
    r"(?:\s|$|[,;.)\]])"
)

# ISA lines in markdown code blocks
RE_MD_ISA = re.compile(
    r"[xyzwt]:\s+(\S+)"
)


def extract_opcodes_r600debug(text: str) -> Counter[str]:
    """Extract ISA mnemonics from R600_DEBUG stderr output."""
    counts: Counter[str] = Counter()
    for line in text.splitlines():
        m = RE_ALU_SLOT.match(line)
        if m:
            counts[m.group(1)] += 1
            continue
        m = RE_CF_INST.match(line)
        if m:
            raw = m.group(1)
            # Normalize: "ALU_PUSH_BEFORE 4" → "ALU_PUSH_BEFORE"
            counts[raw.split()[0]] += 1
            continue
        m = RE_FETCH.match(line)
        if m:
            raw = m.group(1)
            if raw.startswith("MEM_RAT"):
                # "MEM_RAT WRITE_IND_ACK" → record both
                parts = raw.split()
                counts["MEM_RAT"] += 1
                if len(parts) > 1:
                    counts[parts[1]] += 1
            else:
                counts[raw] += 1
    return counts


def extract_opcodes_markdown(text: str) -> Counter[str]:
    """Extract ISA mnemonics mentioned in markdown findings."""
    counts: Counter[str] = Counter()
    in_code = False
    for line in text.splitlines():
        if line.strip().startswith("```"):
            in_code = not in_code
        if in_code:
            m = RE_MD_ISA.search(line)
            if m:
                op = m.group(1)
                if op in KNOWN_OPCODES:
                    counts[op] += 1
    return counts


# ── file scanning ─────────────────────────────────────────────────────────

def scan_files(paths: list[Path]) -> Counter[str]:
    """Scan all given files and extract observed opcodes."""
    total: Counter[str] = Counter()
    for path in paths:
        text = path.read_text(errors="replace")
        if path.suffix == ".stderr" or path.suffix == ".log":
            total.update(extract_opcodes_r600debug(text))
        elif path.suffix == ".md":
            total.update(extract_opcodes_markdown(text))
        else:
            # Try both approaches
            total.update(extract_opcodes_r600debug(text))
    return total


def discover_files(results_dir: Path) -> list[Path]:
    """Find all files that might contain ISA disassembly."""
    paths: list[Path] = []
    for pattern in ("**/*.stderr", "**/*.log", "**/*.stdout"):
        paths.extend(results_dir.glob(pattern))
    # Also scan blessed findings
    findings = results_dir / "x130e_terascale"
    if findings.exists():
        paths.extend(findings.rglob("*.md"))
    return sorted(set(paths))


# ── reporting ─────────────────────────────────────────────────────────────

def print_census(observed: Counter[str]) -> None:
    # Split observed into known vs unknown
    known_observed = {op: count for op, count in observed.items()
                      if op in KNOWN_OPCODES}
    unknown_observed = {op: count for op, count in observed.items()
                        if op not in KNOWN_OPCODES}
    never_seen = {op: cat for op, cat in KNOWN_OPCODES.items()
                  if op not in observed}

    # Category stats
    cat_total: Counter[str] = Counter()
    cat_observed: Counter[str] = Counter()
    for op, cat in KNOWN_OPCODES.items():
        cat_total[cat] += 1
        if op in observed:
            cat_observed[cat] += 1

    print(f"\n{'=' * 60}")
    print(f"  r600 ISA Mnemonic Census")
    print(f"{'=' * 60}")
    print(f"  Reference inventory:  {len(KNOWN_OPCODES)} opcodes")
    print(f"  Observed (known):     {len(known_observed)}")
    print(f"  Observed (unknown):   {len(unknown_observed)}")
    print(f"  Never observed:       {len(never_seen)}")
    print(f"  Coverage:             {len(known_observed)/len(KNOWN_OPCODES)*100:.1f}%")

    print(f"\n  Coverage by category:")
    print(f"  {'Category':<16s} {'Observed':>8s} {'Total':>8s} {'Coverage':>10s}")
    print(f"  {'-'*44}")
    for cat in sorted(cat_total):
        obs = cat_observed.get(cat, 0)
        tot = cat_total[cat]
        pct = obs / tot * 100 if tot else 0
        bar = "#" * int(pct / 5) + "." * (20 - int(pct / 5))
        print(f"  {cat:<16s} {obs:>8d} {tot:>8d} {pct:>8.0f}%  {bar}")

    print(f"\n  Observed opcodes (by frequency):")
    for op, count in sorted(known_observed.items(), key=lambda x: -x[1]):
        cat = KNOWN_OPCODES[op]
        print(f"    {op:<28s} {count:>6d}  ({cat})")

    if unknown_observed:
        print(f"\n  Unknown opcodes (not in reference, by frequency):")
        for op, count in sorted(unknown_observed.items(), key=lambda x: -x[1]):
            print(f"    {op:<28s} {count:>6d}")

    # Show the "dark" frontier by category
    print(f"\n  Never-observed opcodes (the dark frontier):")
    by_cat: dict[str, list[str]] = {}
    for op, cat in sorted(never_seen.items()):
        by_cat.setdefault(cat, []).append(op)
    for cat in sorted(by_cat):
        ops = by_cat[cat]
        if len(ops) <= 10:
            print(f"    {cat}: {', '.join(ops)}")
        else:
            print(f"    {cat} ({len(ops)}): {', '.join(ops[:8])}, ...")


def write_csv(observed: Counter[str], path: Path) -> None:
    fieldnames = ["opcode", "category", "observed_count", "status"]
    rows = []
    for op in sorted(KNOWN_OPCODES):
        cat = KNOWN_OPCODES[op]
        count = observed.get(op, 0)
        status = "observed" if count > 0 else "never_seen"
        rows.append({
            "opcode": op,
            "category": cat,
            "observed_count": count,
            "status": status,
        })
    # Also add unknown opcodes
    for op, count in sorted(observed.items()):
        if op not in KNOWN_OPCODES:
            rows.append({
                "opcode": op,
                "category": "UNKNOWN",
                "observed_count": count,
                "status": "unknown",
            })
    with path.open("w", newline="") as fp:
        writer = csv.DictWriter(fp, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ── main ──────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Track r600 ISA opcode emission frontier."
    )
    parser.add_argument("files", nargs="*", type=Path,
                        help="Explicit files to scan for ISA mnemonics")
    parser.add_argument("--results-dir", type=Path, default=RESULTS_DIR,
                        help="Auto-discover ISA output in results directory")
    parser.add_argument("--csv", type=Path, default=None,
                        help="Write census to CSV")
    args = parser.parse_args()

    if args.files:
        paths = args.files
    else:
        paths = discover_files(args.results_dir)

    if not paths:
        print("No ISA files found.", file=sys.stderr)
        return 1

    print(f"Scanning {len(paths)} files...")
    observed = scan_files(paths)
    print_census(observed)

    if args.csv:
        write_csv(observed, args.csv)
        print(f"\nCSV written to {args.csv}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
