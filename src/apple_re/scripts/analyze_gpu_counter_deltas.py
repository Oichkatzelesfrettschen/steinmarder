#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from pathlib import Path

FOCUS_COUNTERS = [
    "ALU Utilization",
    "Compute Occupancy",
    "F16 Utilization",
    "F32 Utilization",
    "Fragment Occupancy",
    "GPU Read Bandwidth",
    "GPU Write Bandwidth",
    "LLC Utilization",
    "Top Performance Limiter",
    "Total Occupancy",
]


def load_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


def parse_float(text: str) -> float:
    return float((text or "0").strip())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("summary_csv", help="Path to gpu_hardware_counters.csv")
    parser.add_argument("--baseline", default="baseline")
    parser.add_argument("--csv-out")
    parser.add_argument("--md-out")
    args = parser.parse_args()

    summary_csv = Path(args.summary_csv).resolve()
    csv_out = Path(args.csv_out).resolve() if args.csv_out else summary_csv.with_name("gpu_hardware_counter_deltas.csv")
    md_out = Path(args.md_out).resolve() if args.md_out else summary_csv.with_name("gpu_hardware_counter_deltas.md")

    rows = load_csv(summary_csv)
    index = {(row["variant"], row["counter"]): row for row in rows}
    variants = sorted({row["variant"] for row in rows if row["variant"] != args.baseline})

    out_rows: list[dict[str, str]] = []
    for variant in variants:
        for counter in FOCUS_COUNTERS:
            baseline_row = index.get((args.baseline, counter))
            variant_row = index.get((variant, counter))
            if baseline_row is None or variant_row is None:
                continue
            baseline_avg = parse_float(baseline_row["avg_pct"])
            variant_avg = parse_float(variant_row["avg_pct"])
            baseline_peak = parse_float(baseline_row["max_pct"])
            variant_peak = parse_float(variant_row["max_pct"])
            baseline_p99 = parse_float(baseline_row["p99_pct"])
            variant_p99 = parse_float(variant_row["p99_pct"])
            avg_ratio = ""
            if baseline_avg > 0.0:
                avg_ratio = f"{variant_avg / baseline_avg:.3f}"
            out_rows.append({
                "baseline_variant": args.baseline,
                "variant": variant,
                "counter": counter,
                "baseline_avg_pct": f"{baseline_avg:.3f}",
                "variant_avg_pct": f"{variant_avg:.3f}",
                "avg_delta_pct": f"{variant_avg - baseline_avg:.3f}",
                "avg_ratio": avg_ratio,
                "baseline_p99_pct": f"{baseline_p99:.3f}",
                "variant_p99_pct": f"{variant_p99:.3f}",
                "p99_delta_pct": f"{variant_p99 - baseline_p99:.3f}",
                "baseline_peak_pct": f"{baseline_peak:.3f}",
                "variant_peak_pct": f"{variant_peak:.3f}",
                "peak_delta_pct": f"{variant_peak - baseline_peak:.3f}",
            })

    fieldnames = [
        "baseline_variant",
        "variant",
        "counter",
        "baseline_avg_pct",
        "variant_avg_pct",
        "avg_delta_pct",
        "avg_ratio",
        "baseline_p99_pct",
        "variant_p99_pct",
        "p99_delta_pct",
        "baseline_peak_pct",
        "variant_peak_pct",
        "peak_delta_pct",
    ]
    with csv_out.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(out_rows)

    lines = [
        "# GPU Hardware Counter Deltas",
        "",
        f"- source: `{summary_csv}`",
        f"- baseline_variant: `{args.baseline}`",
        f"- compared_variants: {len(variants)}",
        "",
    ]
    if not out_rows:
        lines += [
            "No variant deltas were produced yet.",
            "",
            "Capture at least one non-baseline variant into the same summary CSV to populate this report.",
            "",
        ]
    else:
        current_variant = None
        for row in out_rows:
            variant = row["variant"]
            if variant != current_variant:
                current_variant = variant
                lines += [
                    f"## {variant} vs {args.baseline}",
                    "",
                    "| Counter | Avg delta | Avg ratio | P99 delta | Peak delta |",
                    "|---------|-----------|-----------|-----------|------------|",
                ]
            lines.append(
                f"| {row['counter']} | {row['avg_delta_pct']} | {row['avg_ratio'] or 'n/a'} | {row['p99_delta_pct']} | {row['peak_delta_pct']} |"
            )
        lines.append("")

    md_out.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {csv_out}")
    print(f"wrote {md_out}")
    print(f"delta_rows: {len(out_rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
