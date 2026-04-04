#!/usr/bin/env python3
"""Summarize perf stat CSV outputs emitted by profile_perf.sh."""

from __future__ import annotations

import csv
import sys
from pathlib import Path


def parse_count(raw: str) -> str:
    raw = raw.strip()
    return "" if raw == "<not counted>" else raw


def parse_file(path: Path) -> dict[str, str]:
    metrics: dict[str, str] = {"file": str(path)}
    with path.open(newline="") as fh:
        reader = csv.reader(fh)
        for row in reader:
            if len(row) < 3:
                continue
            count = parse_count(row[0])
            event = row[2].replace(":u", "")
            if event:
                metrics[event] = count
    return metrics


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: summarize_perf_stat.py <results_dir>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    files = sorted(root.rglob("*_perf_stat.csv"))
    rows = [parse_file(path) for path in files]
    fields = [
        "file",
        "cycles",
        "instructions",
        "branches",
        "branch-misses",
        "cache-references",
        "cache-misses",
        "L1-dcache-loads",
        "L1-dcache-load-misses",
        "dTLB-loads",
        "dTLB-load-misses",
    ]
    writer = csv.DictWriter(sys.stdout, fieldnames=fields)
    writer.writeheader()
    for row in rows:
        writer.writerow(row)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
