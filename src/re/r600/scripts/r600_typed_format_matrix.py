#!/usr/bin/env python3
"""
r600_typed_format_matrix.py — backend × dtype × op capability matrix.

Scans suite_results.csv files from OpenCL and Vulkan probe suites plus the
probe source corpus to build a matrix of (backend, data_type, operation) →
{ok, fail, untested}.  This tells you at a glance which measurement lanes
are lit, which are blocked, and which probes exist but haven't been run.

Usage:
    python3 r600_typed_format_matrix.py                        # auto-discover
    python3 r600_typed_format_matrix.py --results-dir path/    # explicit
    python3 r600_typed_format_matrix.py --csv matrix.csv       # write CSV
"""
from __future__ import annotations

import argparse
import csv
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESULTS_DIR = ROOT / "results"
PROBE_DIR = ROOT / "probes" / "r600"

# ── dtype extraction from probe names and mode fields ─────────────────────

DTYPE_FROM_MODE = {
    "scalar_f32": "f32",
    "vec4_f32": "vec4_f32",
    "scalar_f64": "f64",
    "vec4_f64": "vec4_f64",
    "scalar_i32": "i32",
    "vec4_i32": "vec4_i32",
}

# Probe name patterns → (dtype, operation)
PROBE_PATTERNS: list[tuple[re.Pattern[str], str, str]] = [
    # Integer dep-chains by width
    (re.compile(r"depchain_(u8)_(add|mul)"), "u8", "depchain"),
    (re.compile(r"depchain_(i8)_(add|mul)"), "i8", "depchain"),
    (re.compile(r"depchain_(u16)_(add|mul)"), "u16", "depchain"),
    (re.compile(r"depchain_(i16)_(add|mul)"), "i16", "depchain"),
    (re.compile(r"depchain_(u32)_(add|mul)"), "u32", "depchain"),
    (re.compile(r"depchain_(i32)_(add|mul)"), "i32", "depchain"),
    (re.compile(r"depchain_(u64)_(add|mul)"), "u64", "depchain"),
    (re.compile(r"depchain_(i64)_(add|mul)"), "i64", "depchain"),
    (re.compile(r"depchain_(uint24)_mul"), "uint24", "depchain"),
    (re.compile(r"depchain_(int24)_mul"), "int24", "depchain"),
    (re.compile(r"depchain_add$"), "f32", "depchain"),
    (re.compile(r"depchain_mul$"), "f32", "depchain"),
    # Throughput probes
    (re.compile(r"throughput_vec4_(mad|fma)"), "vec4_f32", "throughput"),
    (re.compile(r"throughput_uvec4_mad24"), "uvec4_u24", "throughput"),
    (re.compile(r"throughput_ivec4_mad24"), "ivec4_i24", "throughput"),
    (re.compile(r"throughput_u8_packed_mad"), "u8_packed", "throughput"),
    (re.compile(r"throughput_i8_packed_mad"), "i8_packed", "throughput"),
    (re.compile(r"throughput_mul_ieee"), "f32", "throughput"),
    (re.compile(r"throughput_recipsqrt"), "f32", "throughput_trans"),
    # Occupancy / GPR pressure
    (re.compile(r"occupancy_gpr_(\d+)"), "f32", "occupancy"),
    (re.compile(r"gpr_pressure_(\d+)"), "f32", "gpr_pressure"),
    # Atomics
    (re.compile(r"atomic_cas"), "u32", "atomic"),
    # Special opcodes
    (re.compile(r"lloydmax"), "u8_packed", "lloydmax"),
    (re.compile(r"vtx_fetch"), "f32", "vtx_fetch"),
    (re.compile(r"mul_uint24"), "uint24", "mul_uint24"),
    # Legacy GLSL latency probes
    (re.compile(r"latency_fp64"), "f64", "latency"),
    (re.compile(r"latency_tex"), "f32", "tex_latency"),
    (re.compile(r"latency_kcache"), "f32", "kcache_latency"),
    (re.compile(r"latency_mullo_int"), "i32", "latency"),
    (re.compile(r"latency_(add|mul|muladd|recipsqrt|sin_cos|dot4)"), "f32", "latency"),
    # Legacy GLSL ALU probes
    (re.compile(r"alu_(dot_product|normalize|heavy|mad_chain|mul_chain)"), "f32", "alu"),
    (re.compile(r"alu_(sin_cos|transcendental)"), "f32", "alu_trans"),
    # Legacy GLSL texture probes
    (re.compile(r"tex_(bandwidth|fetch|heavy)"), "f32", "texture"),
    # Legacy GLSL misc
    (re.compile(r"clause_switch"), "f32", "clause_switch"),
    (re.compile(r"vliw5_"), "f32", "vliw5"),
]


def classify_probe(probe_name: str, mode: str) -> tuple[str, str]:
    """Return (dtype, operation) for a probe."""
    for pattern, dtype, op in PROBE_PATTERNS:
        if pattern.search(probe_name):
            return dtype, op
    # Fallback: extract from mode if available
    dtype = DTYPE_FROM_MODE.get(mode, "unknown")
    op = probe_name.split("_")[0] if probe_name else "unknown"
    return dtype, op


# ── result collection ─────────────────────────────────────────────────────

@dataclass
class ProbeResult:
    lane: str
    probe_group: str
    probe_name: str
    dtype: str
    operation: str
    status: str
    profile: str = ""
    kernel_ns: float = 0.0
    suite_dir: str = ""


def load_suite_results(results_dir: Path) -> list[ProbeResult]:
    """Load all suite_results.csv files from result directories."""
    results: list[ProbeResult] = []
    for csv_path in sorted(results_dir.glob("*/suite_results.csv")):
        suite_name = csv_path.parent.name
        with csv_path.open(newline="") as fp:
            reader = csv.DictReader(fp)
            for row in reader:
                lane = row.get("lane", "")
                probe_name = row.get("probe_name", "")
                mode = row.get("mode", "")
                status = row.get("status", "")
                if not lane or not probe_name:
                    continue
                dtype, operation = classify_probe(probe_name, mode)
                kernel_ns = 0.0
                for key in ("kernel_ns", "dispatch_ns"):
                    val = row.get(key, "").strip()
                    if val:
                        try:
                            kernel_ns = float(val)
                        except ValueError:
                            pass
                        break
                results.append(ProbeResult(
                    lane=lane,
                    probe_group=row.get("probe_group", ""),
                    probe_name=probe_name,
                    dtype=dtype,
                    operation=operation,
                    status=status,
                    profile=row.get("profile", ""),
                    kernel_ns=kernel_ns,
                    suite_dir=suite_name,
                ))
    return results


# ── probe corpus inventory ────────────────────────────────────────────────

LANE_MAP = {
    "opencl": "opencl",
    "vulkan_compute": "vulkan",
    "opengl_compute": "opengl",
    "shader_tests": "shader_runner",
    "legacy_glsl": "opengl",
}


def inventory_probe_corpus(probe_dir: Path) -> list[tuple[str, str, str]]:
    """Return (lane, dtype, operation) for every probe source file."""
    inventory: list[tuple[str, str, str]] = []
    for ext in ("*.cl", "*.comp", "*.frag", "*.shader_test"):
        for path in probe_dir.rglob(ext):
            rel_parts = path.relative_to(probe_dir).parts
            lane_dir = rel_parts[0] if len(rel_parts) > 1 else ""
            lane = LANE_MAP.get(lane_dir, lane_dir)
            if not lane:
                continue  # skip root-level / unclassifiable files
            dtype, operation = classify_probe(path.stem, "")
            inventory.append((lane, dtype, operation))
    return inventory


# ── matrix assembly ───────────────────────────────────────────────────────

@dataclass
class CellStatus:
    best: str = "untested"  # ok > fail > untested
    count_ok: int = 0
    count_fail: int = 0
    best_ns: float = 0.0
    has_source: bool = False

    def merge_result(self, status: str, kernel_ns: float) -> None:
        if status == "ok":
            self.count_ok += 1
            if self.best != "ok" or (kernel_ns and (not self.best_ns or kernel_ns < self.best_ns)):
                self.best_ns = kernel_ns
            self.best = "ok"
        elif status == "fail":
            self.count_fail += 1
            if self.best == "untested":
                self.best = "fail"


def build_matrix(
    results: list[ProbeResult],
    corpus: list[tuple[str, str, str]],
) -> dict[tuple[str, str, str], CellStatus]:
    """Build (lane, dtype, operation) → CellStatus matrix."""
    matrix: dict[tuple[str, str, str], CellStatus] = defaultdict(CellStatus)

    for lane, dtype, operation in corpus:
        matrix[(lane, dtype, operation)].has_source = True

    for r in results:
        cell = matrix[(r.lane, r.dtype, r.operation)]
        cell.merge_result(r.status, r.kernel_ns)

    return dict(matrix)


# ── output ────────────────────────────────────────────────────────────────

DTYPE_ORDER = [
    "f32", "vec4_f32", "f64", "vec4_f64",
    "i8", "u8", "i8_packed", "u8_packed",
    "i16", "u16", "int24", "uint24",
    "i32", "u32", "ivec4_i24", "uvec4_u24",
    "i64", "u64", "unknown",
]

LANE_ORDER = ["opencl", "vulkan", "opengl", "shader_runner"]


def dtype_sort_key(dtype: str) -> int:
    try:
        return DTYPE_ORDER.index(dtype)
    except ValueError:
        return len(DTYPE_ORDER)


def format_cell(cell: CellStatus) -> str:
    sym = {"ok": "OK", "fail": "FAIL", "untested": "--"}[cell.best]
    if cell.best == "ok" and cell.best_ns:
        return f"{sym} ({cell.best_ns/1e6:.1f}ms)"
    if cell.count_ok or cell.count_fail:
        return f"{sym} ({cell.count_ok}ok/{cell.count_fail}f)"
    if cell.has_source:
        return "SOURCE"
    return sym


def print_matrix(matrix: dict[tuple[str, str, str], CellStatus]) -> None:
    """Print human-readable matrix to stdout."""
    # Collect all operations and dtypes
    all_ops = sorted({k[2] for k in matrix})
    all_dtypes = sorted({k[1] for k in matrix}, key=dtype_sort_key)
    all_lanes = sorted({k[0] for k in matrix},
                       key=lambda l: LANE_ORDER.index(l) if l in LANE_ORDER else 99)

    for lane in all_lanes:
        print(f"\n{'=' * 72}")
        print(f"  Lane: {lane}")
        print(f"{'=' * 72}")

        # Header
        col_w = max(12, max((len(op) for op in all_ops), default=12) + 2)
        header = f"{'dtype':<16s}" + "".join(f"{op:<{col_w}s}" for op in all_ops)
        print(header)
        print("-" * len(header))

        for dtype in all_dtypes:
            row_cells = []
            has_any = False
            for op in all_ops:
                key = (lane, dtype, op)
                if key in matrix:
                    has_any = True
                    row_cells.append(format_cell(matrix[key]))
                else:
                    row_cells.append("")
            if has_any:
                row = f"{dtype:<16s}" + "".join(f"{c:<{col_w}s}" for c in row_cells)
                print(row)


def write_csv(matrix: dict[tuple[str, str, str], CellStatus], path: Path) -> None:
    """Write matrix to CSV."""
    fieldnames = [
        "lane", "dtype", "operation", "status", "count_ok", "count_fail",
        "best_ns", "has_source",
    ]
    rows = []
    for (lane, dtype, op), cell in sorted(matrix.items()):
        rows.append({
            "lane": lane,
            "dtype": dtype,
            "operation": op,
            "status": cell.best,
            "count_ok": cell.count_ok,
            "count_fail": cell.count_fail,
            "best_ns": f"{cell.best_ns:.0f}" if cell.best_ns else "",
            "has_source": "yes" if cell.has_source else "no",
        })
    with path.open("w", newline="") as fp:
        writer = csv.DictWriter(fp, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ── summary stats ─────────────────────────────────────────────────────────

def print_summary(matrix: dict[tuple[str, str, str], CellStatus]) -> None:
    print(f"\n{'=' * 72}")
    print("  Summary")
    print(f"{'=' * 72}")
    total = len(matrix)
    ok = sum(1 for c in matrix.values() if c.best == "ok")
    fail = sum(1 for c in matrix.values() if c.best == "fail")
    untested = sum(1 for c in matrix.values() if c.best == "untested")
    source_only = sum(1 for c in matrix.values() if c.best == "untested" and c.has_source)
    print(f"  Total cells:    {total}")
    print(f"  OK:             {ok}")
    print(f"  FAIL:           {fail}")
    print(f"  Untested:       {untested} ({source_only} have source)")
    print(f"  Coverage:       {(ok + fail) / total * 100:.0f}% tested, "
          f"{ok / max(ok + fail, 1) * 100:.0f}% pass rate")


# ── main ──────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build backend × dtype × op capability matrix from probe results."
    )
    parser.add_argument("--results-dir", type=Path, default=RESULTS_DIR,
                        help="Directory containing suite result subdirectories")
    parser.add_argument("--probe-dir", type=Path, default=PROBE_DIR,
                        help="Directory containing probe source files")
    parser.add_argument("--csv", type=Path, default=None,
                        help="Write matrix to CSV file")
    args = parser.parse_args()

    results = load_suite_results(args.results_dir)
    corpus = inventory_probe_corpus(args.probe_dir)
    matrix = build_matrix(results, corpus)

    print_matrix(matrix)
    print_summary(matrix)

    if args.csv:
        write_csv(matrix, args.csv)
        print(f"\nCSV written to {args.csv}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
