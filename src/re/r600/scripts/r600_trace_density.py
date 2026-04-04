#!/usr/bin/env python3
"""
r600_trace_density.py — trace event density analysis for r600 driver.

Parses R600_DEBUG stderr output (COMPUTE_TRACE, GFX_FLUSH_TRACE,
WINSYS_TRACE) and optionally trace-cmd text output to compute event
density metrics: events per shader compilation, per CS submission,
and per flush cycle.

This tells you whether the driver is spending time compiling, flushing,
or actually dispatching, and helps identify stalls and hot paths.

Usage:
    python3 r600_trace_density.py path/to/clinfo.stderr
    python3 r600_trace_density.py --results-dir path/to/results/
    python3 r600_trace_density.py --trace-cmd path/to/trace.txt
    python3 r600_trace_density.py --csv density.csv --results-dir results/
"""
from __future__ import annotations

import argparse
import csv
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESULTS_DIR = ROOT / "results"


# ── R600_DEBUG trace event parsing ────────────────────────────────────────

@dataclass
class TraceEvent:
    event_type: str      # COMPUTE_TRACE, GFX_FLUSH_TRACE, WINSYS_TRACE, ISA, etc.
    detail: str          # the text after the prefix
    line_no: int = 0


# R600_DEBUG prefix patterns
R600_TRACE_RE = re.compile(
    r"^(COMPUTE_TRACE|GFX_FLUSH_TRACE|WINSYS_TRACE|FETCH_TRACE|TEX_TRACE):\s*(.*)"
)

# ISA disassembly markers
ISA_CF_RE = re.compile(r"^\d{4}\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+(ALU|TEX|VTX|MEM_RAT|CF_|EXPORT|LOOP|CALL|JUMP|ELSE|POP|PUSH|NOP)")
ISA_ALU_RE = re.compile(r"^\s+\d{4}\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+\d+\s+[xyzwt]:\s+(\S+)")
ISA_FETCH_RE = re.compile(r"^\s+\d{4}\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+(VFETCH|TEX_SAMPLE|TEX_LD|MEM_RAT\S*)")

# GFX_FLUSH detail parsing
FLUSH_CDW_RE = re.compile(r"cdw=(\d+)")
FLUSH_RELOCS_RE = re.compile(r"relocs=(\d+)")
FLUSH_RING_RE = re.compile(r"ring=(\d+)")

# COMPUTE_TRACE detail parsing
SHADER_TYPE_RE = re.compile(r"type=(\d+)")
ISA_DWORDS_RE = re.compile(r"(\d+)\s+dwords")
ISA_GPRS_RE = re.compile(r"(\d+)\s+gprs")


def parse_r600_debug(text: str) -> list[TraceEvent]:
    """Parse R600_DEBUG stderr output into structured events."""
    events: list[TraceEvent] = []
    for line_no, line in enumerate(text.splitlines(), 1):
        m = R600_TRACE_RE.match(line)
        if m:
            events.append(TraceEvent(m.group(1), m.group(2), line_no))
            continue
        # Check for ISA lines
        m = ISA_ALU_RE.match(line)
        if m:
            events.append(TraceEvent("ISA_ALU", m.group(1), line_no))
            continue
        m = ISA_CF_RE.match(line)
        if m:
            events.append(TraceEvent("ISA_CF", m.group(1), line_no))
            continue
        m = ISA_FETCH_RE.match(line)
        if m:
            events.append(TraceEvent("ISA_FETCH", m.group(1), line_no))
    return events


# ── trace-cmd text parsing ──────────────��─────────────────────────────────

TRACECMD_RE = re.compile(
    r"^\s*(\S+)-(\d+)\s+\[(\d+)\]\s+([\d.]+):\s+(\S+):\s*(.*)"
)


@dataclass
class TraceCmdEvent:
    process: str
    pid: int
    cpu: int
    timestamp: float
    event_name: str
    detail: str


def parse_tracecmd(text: str) -> list[TraceCmdEvent]:
    """Parse trace-cmd report text output."""
    events: list[TraceCmdEvent] = []
    for line in text.splitlines():
        m = TRACECMD_RE.match(line)
        if m:
            events.append(TraceCmdEvent(
                process=m.group(1),
                pid=int(m.group(2)),
                cpu=int(m.group(3)),
                timestamp=float(m.group(4)),
                event_name=m.group(5),
                detail=m.group(6),
            ))
    return events


# ── analysis ──────────────────────────────────────────────────────────────

@dataclass
class R600DensityReport:
    source: str
    total_events: int = 0
    event_counts: dict[str, int] = field(default_factory=Counter)
    shader_compilations: int = 0
    cs_submissions: int = 0
    gfx_flushes: int = 0
    total_cdw: int = 0
    total_relocs: int = 0
    isa_alu_opcodes: dict[str, int] = field(default_factory=Counter)
    isa_cf_opcodes: dict[str, int] = field(default_factory=Counter)
    isa_fetch_opcodes: dict[str, int] = field(default_factory=Counter)
    total_gprs: int = 0
    total_isa_dwords: int = 0
    shader_count: int = 0


def analyze_r600_events(events: list[TraceEvent], source: str) -> R600DensityReport:
    """Compute density metrics from parsed R600_DEBUG events."""
    report = R600DensityReport(source=source)
    report.total_events = len(events)

    for ev in events:
        report.event_counts[ev.event_type] += 1

        if ev.event_type == "COMPUTE_TRACE":
            if "pipe_shader_create" in ev.detail:
                report.shader_compilations += 1
            if "kernel ISA" in ev.detail:
                report.shader_count += 1
                m = ISA_DWORDS_RE.search(ev.detail)
                if m:
                    report.total_isa_dwords += int(m.group(1))

        elif ev.event_type == "WINSYS_TRACE":
            if "CS_SUBMIT" in ev.detail:
                report.cs_submissions += 1
                m = FLUSH_CDW_RE.search(ev.detail)
                if m:
                    report.total_cdw += int(m.group(1))
                m = FLUSH_RELOCS_RE.search(ev.detail)
                if m:
                    report.total_relocs += int(m.group(1))

        elif ev.event_type == "GFX_FLUSH_TRACE":
            report.gfx_flushes += 1
            m = FLUSH_CDW_RE.search(ev.detail)
            if m:
                report.total_cdw += int(m.group(1))

        elif ev.event_type == "ISA_ALU":
            report.isa_alu_opcodes[ev.detail] += 1

        elif ev.event_type == "ISA_CF":
            report.isa_cf_opcodes[ev.detail] += 1

        elif ev.event_type == "ISA_FETCH":
            report.isa_fetch_opcodes[ev.detail] += 1

    return report


@dataclass
class TraceCmdDensityReport:
    source: str
    total_events: int = 0
    event_counts: dict[str, int] = field(default_factory=Counter)
    duration_s: float = 0.0
    events_per_second: float = 0.0
    per_cpu: dict[int, int] = field(default_factory=Counter)
    per_process: dict[str, int] = field(default_factory=Counter)


def analyze_tracecmd_events(
    events: list[TraceCmdEvent], source: str,
) -> TraceCmdDensityReport:
    """Compute density metrics from parsed trace-cmd events."""
    report = TraceCmdDensityReport(source=source)
    report.total_events = len(events)
    if not events:
        return report

    for ev in events:
        report.event_counts[ev.event_name] += 1
        report.per_cpu[ev.cpu] += 1
        report.per_process[ev.process] += 1

    timestamps = [ev.timestamp for ev in events]
    report.duration_s = max(timestamps) - min(timestamps)
    if report.duration_s > 0:
        report.events_per_second = report.total_events / report.duration_s

    return report


# ── output ────────────────────────────────────���───────────────────────────

def print_r600_report(report: R600DensityReport) -> None:
    print(f"\n{'=' * 60}")
    print(f"  R600_DEBUG Trace Density: {report.source}")
    print(f"{'=' * 60}")
    print(f"  Total events:         {report.total_events}")
    print(f"  Shader compilations:  {report.shader_compilations}")
    print(f"  Compiled shaders:     {report.shader_count}")
    print(f"  CS submissions:       {report.cs_submissions}")
    print(f"  GFX flushes:          {report.gfx_flushes}")
    print(f"  Total CDW emitted:    {report.total_cdw}")
    print(f"  Total relocations:    {report.total_relocs}")
    print(f"  Total ISA dwords:     {report.total_isa_dwords}")

    if report.cs_submissions:
        print(f"  Avg CDW/submit:       {report.total_cdw / report.cs_submissions:.0f}")
    if report.shader_count:
        print(f"  Avg ISA dwords/shader:{report.total_isa_dwords / report.shader_count:.0f}")

    print(f"\n  Event type breakdown:")
    for etype, count in sorted(report.event_counts.items(), key=lambda x: -x[1]):
        print(f"    {etype:<24s} {count:>6d}")

    if report.isa_alu_opcodes:
        print(f"\n  ALU opcode histogram (top 15):")
        for op, count in sorted(report.isa_alu_opcodes.items(), key=lambda x: -x[1])[:15]:
            print(f"    {op:<24s} {count:>6d}")

    if report.isa_cf_opcodes:
        print(f"\n  CF opcode histogram:")
        for op, count in sorted(report.isa_cf_opcodes.items(), key=lambda x: -x[1]):
            print(f"    {op:<24s} {count:>6d}")

    if report.isa_fetch_opcodes:
        print(f"\n  Fetch opcode histogram:")
        for op, count in sorted(report.isa_fetch_opcodes.items(), key=lambda x: -x[1]):
            print(f"    {op:<24s} {count:>6d}")


def print_tracecmd_report(report: TraceCmdDensityReport) -> None:
    print(f"\n{'=' * 60}")
    print(f"  trace-cmd Density: {report.source}")
    print(f"{'=' * 60}")
    print(f"  Total events:     {report.total_events}")
    print(f"  Duration:         {report.duration_s:.3f}s")
    print(f"  Events/second:    {report.events_per_second:.0f}")

    print(f"\n  Per-event breakdown:")
    for name, count in sorted(report.event_counts.items(), key=lambda x: -x[1])[:20]:
        rate = count / report.duration_s if report.duration_s else 0
        print(f"    {name:<36s} {count:>6d}  ({rate:>8.0f}/s)")

    print(f"\n  Per-CPU distribution:")
    for cpu in sorted(report.per_cpu):
        print(f"    CPU {cpu}: {report.per_cpu[cpu]:>6d}")

    if len(report.per_process) <= 10:
        print(f"\n  Per-process:")
        for proc, count in sorted(report.per_process.items(), key=lambda x: -x[1]):
            print(f"    {proc:<24s} {count:>6d}")


def write_r600_csv(reports: list[R600DensityReport], path: Path) -> None:
    fieldnames = [
        "source", "total_events", "shader_compilations", "compiled_shaders",
        "cs_submissions", "gfx_flushes", "total_cdw", "total_relocs",
        "total_isa_dwords", "unique_alu_opcodes", "unique_cf_opcodes",
        "unique_fetch_opcodes",
    ]
    with path.open("w", newline="") as fp:
        writer = csv.DictWriter(fp, fieldnames=fieldnames)
        writer.writeheader()
        for r in reports:
            writer.writerow({
                "source": r.source,
                "total_events": r.total_events,
                "shader_compilations": r.shader_compilations,
                "compiled_shaders": r.shader_count,
                "cs_submissions": r.cs_submissions,
                "gfx_flushes": r.gfx_flushes,
                "total_cdw": r.total_cdw,
                "total_relocs": r.total_relocs,
                "total_isa_dwords": r.total_isa_dwords,
                "unique_alu_opcodes": len(r.isa_alu_opcodes),
                "unique_cf_opcodes": len(r.isa_cf_opcodes),
                "unique_fetch_opcodes": len(r.isa_fetch_opcodes),
            })


# ── file discovery ──────────��─────────────────────────────────────────────

def find_stderr_files(results_dir: Path) -> list[Path]:
    """Find R600_DEBUG stderr files in result directories."""
    candidates: list[Path] = []
    for path in sorted(results_dir.rglob("*.stderr")):
        candidates.append(path)
    return candidates


def find_tracecmd_files(results_dir: Path) -> list[Path]:
    """Find trace-cmd text output files."""
    candidates: list[Path] = []
    for pattern in ("*tracecmd*", "*trace_cmd*", "*trace-cmd*", "*.trace"):
        for path in results_dir.rglob(pattern):
            if path.is_file() and path.stat().st_size > 0:
                candidates.append(path)
    return sorted(set(candidates))


# ���─ main ──────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analyze R600_DEBUG trace event density."
    )
    parser.add_argument("files", nargs="*", type=Path,
                        help="Explicit stderr or trace files to analyze")
    parser.add_argument("--results-dir", type=Path, default=RESULTS_DIR,
                        help="Auto-discover stderr files in results directory")
    parser.add_argument("--trace-cmd", type=Path, default=None,
                        help="Explicit trace-cmd text output file")
    parser.add_argument("--csv", type=Path, default=None,
                        help="Write R600_DEBUG density to CSV")
    parser.add_argument("--top", type=int, default=5,
                        help="Number of top files to report (default: 5)")
    args = parser.parse_args()

    # Collect files
    stderr_files: list[Path] = []
    tracecmd_files: list[Path] = []

    if args.files:
        stderr_files.extend(args.files)
    else:
        stderr_files = find_stderr_files(args.results_dir)

    if args.trace_cmd:
        tracecmd_files.append(args.trace_cmd)
    elif not args.files:
        tracecmd_files = find_tracecmd_files(args.results_dir)

    # Analyze R600_DEBUG stderr files
    r600_reports: list[R600DensityReport] = []
    for path in stderr_files:
        text = path.read_text(errors="replace")
        events = parse_r600_debug(text)
        if events:
            rel = path.relative_to(args.results_dir) if path.is_relative_to(args.results_dir) else path
            report = analyze_r600_events(events, str(rel))
            r600_reports.append(report)

    # Sort by event count, show top N
    r600_reports.sort(key=lambda r: r.total_events, reverse=True)

    if r600_reports:
        print(f"Found {len(r600_reports)} files with R600_DEBUG trace events")
        for report in r600_reports[:args.top]:
            print_r600_report(report)

        # Aggregate summary
        total_events = sum(r.total_events for r in r600_reports)
        total_compiles = sum(r.shader_compilations for r in r600_reports)
        total_submits = sum(r.cs_submissions for r in r600_reports)
        all_alu = Counter()
        for r in r600_reports:
            all_alu.update(r.isa_alu_opcodes)

        print(f"\n{'=' * 60}")
        print(f"  Aggregate across {len(r600_reports)} files")
        print(f"{'=' * 60}")
        print(f"  Total events:         {total_events}")
        print(f"  Total compilations:   {total_compiles}")
        print(f"  Total CS submissions: {total_submits}")
        print(f"  Unique ALU opcodes:   {len(all_alu)}")
        if all_alu:
            print(f"\n  Combined ALU histogram (top 20):")
            for op, count in all_alu.most_common(20):
                print(f"    {op:<24s} {count:>6d}")

    # Analyze trace-cmd files
    for path in tracecmd_files:
        text = path.read_text(errors="replace")
        events = parse_tracecmd(text)
        if events:
            rel = path.relative_to(args.results_dir) if path.is_relative_to(args.results_dir) else path
            report = analyze_tracecmd_events(events, str(rel))
            print_tracecmd_report(report)

    if args.csv and r600_reports:
        write_r600_csv(r600_reports, args.csv)
        print(f"\nCSV written to {args.csv}")

    if not r600_reports and not tracecmd_files:
        print("No trace data found.", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
