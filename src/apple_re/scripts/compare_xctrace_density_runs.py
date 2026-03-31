#!/usr/bin/env python3
"""Compare elapsed-normalized xctrace density across successive Apple tranches."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


def load_density(path: Path) -> dict[tuple[str, str], dict[str, str]]:
    rows: dict[tuple[str, str], dict[str, str]] = {}
    if not path.exists():
        return rows
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            key = (row.get("variant_trace", "").strip(), row.get("schema", "").strip())
            if key[0] and key[1]:
                rows[key] = row
    return rows


def as_float(value: str) -> float | None:
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("current_density_csv", type=Path)
    parser.add_argument("previous_density_csv", type=Path)
    parser.add_argument("out_csv", type=Path)
    parser.add_argument("out_md", type=Path)
    args = parser.parse_args()

    current = load_density(args.current_density_csv.resolve())
    previous = load_density(args.previous_density_csv.resolve())

    rows = []
    for key, cur in sorted(current.items()):
        prev = previous.get(key)
        if prev is None:
            continue
        cur_delta = as_float(cur.get("delta_density_rps", ""))
        prev_delta = as_float(prev.get("delta_density_rps", ""))
        cur_variant = as_float(cur.get("variant_density_rps", ""))
        prev_variant = as_float(prev.get("variant_density_rps", ""))
        cur_base = as_float(cur.get("baseline_density_rps", ""))
        prev_base = as_float(prev.get("baseline_density_rps", ""))
        if None in {cur_delta, prev_delta, cur_variant, prev_variant, cur_base, prev_base}:
            continue
        rows.append(
            {
                "variant_trace": key[0],
                "schema": key[1],
                "current_baseline_density_rps": f"{cur_base:.6f}",
                "previous_baseline_density_rps": f"{prev_base:.6f}",
                "current_variant_density_rps": f"{cur_variant:.6f}",
                "previous_variant_density_rps": f"{prev_variant:.6f}",
                "current_delta_density_rps": f"{cur_delta:.6f}",
                "previous_delta_density_rps": f"{prev_delta:.6f}",
                "delta_of_delta_rps": f"{(cur_delta - prev_delta):.6f}",
                "sign_stable": "yes" if (cur_delta == 0 or prev_delta == 0 or (cur_delta > 0) == (prev_delta > 0)) else "no",
            }
        )

    with args.out_csv.open("w", newline="", encoding="utf-8") as fh:
        if rows:
            writer = csv.DictWriter(
                fh,
                fieldnames=[
                    "variant_trace",
                    "schema",
                    "current_baseline_density_rps",
                    "previous_baseline_density_rps",
                    "current_variant_density_rps",
                    "previous_variant_density_rps",
                    "current_delta_density_rps",
                    "previous_delta_density_rps",
                    "delta_of_delta_rps",
                    "sign_stable",
                ],
            )
            writer.writeheader()
            writer.writerows(rows)
        else:
            fh.write("variant_trace,schema,current_baseline_density_rps,previous_baseline_density_rps,current_variant_density_rps,previous_variant_density_rps,current_delta_density_rps,previous_delta_density_rps,delta_of_delta_rps,sign_stable\n")

    rows.sort(key=lambda row: abs(float(row["delta_of_delta_rps"])), reverse=True)
    lines = [
        "# Apple xctrace Density Comparison",
        "",
        f"- current: `{args.current_density_csv}`",
        f"- previous: `{args.previous_density_csv}`",
        f"- compared_rows: `{len(rows)}`",
        "",
        "Top absolute delta-of-delta rows:",
    ]
    if not rows:
        lines.append("- none")
    else:
        for row in rows[:12]:
            lines.append(
                f"- `{row['variant_trace']}` `{row['schema']}`: delta-of-delta={row['delta_of_delta_rps']} r/s, sign_stable={row['sign_stable']}"
            )
    args.out_md.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"compared_rows={len(rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
