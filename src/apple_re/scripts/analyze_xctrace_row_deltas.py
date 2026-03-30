#!/usr/bin/env python3
"""Compute schema row-count deltas between baseline and variant xctrace outputs."""

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


def write_summary(
    baseline_name: str,
    rows_out: list[tuple[str, str, int, int, int, str]],
    out_md: pathlib.Path,
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
    out_md.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: analyze_xctrace_row_deltas.py <xctrace_metric_row_counts.csv> <out_deltas.csv> <out_summary.md>",
            file=sys.stderr,
        )
        return 2

    in_csv = pathlib.Path(sys.argv[1]).resolve()
    out_csv = pathlib.Path(sys.argv[2]).resolve()
    out_md = pathlib.Path(sys.argv[3]).resolve()

    if not in_csv.exists():
        print(f"missing input csv: {in_csv}", file=sys.stderr)
        return 1

    counts = load_counts(in_csv)
    baseline_name = "gpu_baseline.trace"
    rows_out = write_deltas(baseline_name, counts, out_csv)
    write_summary(baseline_name, rows_out, out_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
