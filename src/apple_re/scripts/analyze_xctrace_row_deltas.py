#!/usr/bin/env python3
"""Compute schema row-count deltas and elapsed-normalized density changes."""

from __future__ import annotations

import csv
import pathlib
import sys
from collections import defaultdict


def load_counts(path: pathlib.Path) -> dict[str, dict[str, int]]:
    by_trace: dict[str, dict[str, int]] = defaultdict(dict)
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            trace = row.get("trace", "").strip()
            schema = row.get("schema", "").strip()
            rows = row.get("rows", "").strip()
            if not trace or not schema:
                continue
            try:
                by_trace[trace][schema] = int(rows)
            except ValueError:
                continue
    return by_trace


def load_trace_durations(path: pathlib.Path) -> dict[str, float]:
    durations: dict[str, float] = {}
    if not path.exists():
        return durations
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            trace = row.get("trace", "").strip()
            duration = row.get("duration_s", "").strip()
            if not trace or not duration:
                continue
            try:
                durations[trace] = float(duration)
            except ValueError:
                continue
    return durations


def write_deltas(
    baseline_name: str,
    counts: dict[str, dict[str, int]],
    out_csv: pathlib.Path,
) -> list[tuple[str, str, int, int, int, str]]:
    baseline = counts.get(baseline_name, {})
    rows_out: list[tuple[str, str, int, int, int, str]] = []

    for trace_name, schema_counts in sorted(counts.items()):
        if trace_name == baseline_name:
            continue
        all_schemas = sorted(set(baseline) | set(schema_counts))
        for schema in all_schemas:
            base_rows = baseline.get(schema, 0)
            variant_rows = schema_counts.get(schema, 0)
            delta_rows = variant_rows - base_rows
            if base_rows == 0:
                delta_pct = "n/a"
            else:
                delta_pct = f"{(100.0 * delta_rows) / float(base_rows):.2f}%"
            rows_out.append((trace_name, schema, base_rows, variant_rows, delta_rows, delta_pct))

    with out_csv.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            ["variant_trace", "schema", "baseline_rows", "variant_rows", "delta_rows", "delta_pct"]
        )
        writer.writerows(rows_out)
    return rows_out


def write_density(
    baseline_name: str,
    counts: dict[str, dict[str, int]],
    durations: dict[str, float],
    out_csv: pathlib.Path,
) -> list[tuple[str, str, float, float, float]]:
    baseline_counts = counts.get(baseline_name, {})
    baseline_duration = durations.get(baseline_name)
    if not baseline_duration or baseline_duration <= 0.0:
        return []

    rows: list[tuple[str, str, float, float, float]] = []
    for trace_name, schema_counts in sorted(counts.items()):
        if trace_name == baseline_name:
            continue
        trace_duration = durations.get(trace_name)
        if not trace_duration or trace_duration <= 0.0:
            continue
        for schema in sorted(set(baseline_counts) | set(schema_counts)):
            base_rows = baseline_counts.get(schema, 0)
            variant_rows = schema_counts.get(schema, 0)
            variant_density = variant_rows / trace_duration
            base_density = base_rows / baseline_duration
            delta_density = variant_density - base_density
            rows.append((trace_name, schema, base_density, variant_density, delta_density))
    with out_csv.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "variant_trace",
                "schema",
                "baseline_density_rps",
                "variant_density_rps",
                "delta_density_rps",
            ]
        )
        writer.writerows(rows)
    return rows


def write_summary(
    baseline_name: str,
    rows_out: list[tuple[str, str, int, int, int, str]],
    out_md: pathlib.Path,
    density_rows: list[tuple[str, str, float, float, float]],
) -> None:
    metal_rows = [r for r in rows_out if r[1].startswith("metal-")]
    metal_rows.sort(key=lambda r: abs(r[4]), reverse=True)
    top = metal_rows[:20]

    lines = [
        "# xctrace Row Delta Summary",
        "",
        f"- baseline_trace: `{baseline_name}`",
        f"- compared_traces: `{len({r[0] for r in rows_out})}`",
        "",
        "Top absolute deltas on metal schemas:",
    ]
    if not top:
        lines.append("- none")
    else:
        for trace_name, schema, base_rows, variant_rows, delta_rows, delta_pct in top:
            lines.append(
                f"- `{trace_name}` `{schema}`: baseline={base_rows}, variant={variant_rows}, delta={delta_rows} ({delta_pct})"
            )
    if density_rows:
        density_rows_sorted = sorted(density_rows, key=lambda r: abs(r[4]), reverse=True)
        lines.extend([
            "",
            "Top density deltas (rows/sec) on metal schemas:",
        ])
        for trace_name, schema, base_density, variant_density, delta_density in density_rows_sorted[:10]:
            lines.append(
                f"- `{trace_name}` `{schema}`: baseline={base_density:.1f} r/s, variant={variant_density:.1f} r/s, delta={delta_density:.1f} r/s"
            )
    out_md.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 6:
        print(
            "usage: analyze_xctrace_row_deltas.py <xctrace_metric_row_counts.csv> <xctrace_trace_health.csv> <out_deltas.csv> <out_summary.md> <out_density.csv>",
            file=sys.stderr,
        )
        return 2

    in_csv = pathlib.Path(sys.argv[1]).resolve()
    health_csv = pathlib.Path(sys.argv[2]).resolve()
    out_csv = pathlib.Path(sys.argv[3]).resolve()
    out_md = pathlib.Path(sys.argv[4]).resolve()
    out_density_csv = pathlib.Path(sys.argv[5]).resolve()

    if not in_csv.exists():
        print(f"missing input csv: {in_csv}", file=sys.stderr)
        return 1

    counts = load_counts(in_csv)
    durations = load_trace_durations(health_csv)
    baseline_name = "gpu_baseline.trace"
    rows_out = write_deltas(baseline_name, counts, out_csv)
    density_rows = write_density(baseline_name, counts, durations, out_density_csv)
    write_summary(baseline_name, rows_out, out_md, density_rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
