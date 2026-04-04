#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path


def parse_ns(row: dict[str, str]) -> float:
    for key in ("dispatch_ns", "kernel_ns"):
        value = row.get(key, "").strip()
        if value:
            return float(value)
    return 0.0


def load_rows(path: Path) -> dict[str, dict[str, str]]:
    rows: dict[str, dict[str, str]] = {}
    with path.open(newline="") as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            if row.get("status") != "ok":
                continue
            rows[row["probe_group"]] = row
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vulkan-csv", required=True)
    parser.add_argument("--opencl-csv", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    vk_rows = load_rows(Path(args.vulkan_csv))
    cl_rows = load_rows(Path(args.opencl_csv))
    out_path = Path(args.output)

    fieldnames = [
        "probe_group",
        "vk_probe_name",
        "cl_probe_name",
        "vk_mode",
        "cl_mode",
        "vk_time_ns",
        "cl_time_ns",
        "vk_vs_cl_ratio",
        "vk_checksum",
        "cl_checksum",
    ]

    with out_path.open("w", newline="") as fp:
        writer = csv.DictWriter(fp, fieldnames=fieldnames)
        writer.writeheader()
        for group in sorted(set(vk_rows) & set(cl_rows)):
            vk = vk_rows[group]
            cl = cl_rows[group]
            vk_time = parse_ns(vk)
            cl_time = parse_ns(cl)
            ratio = vk_time / cl_time if cl_time else 0.0
            writer.writerow(
                {
                    "probe_group": group,
                    "vk_probe_name": vk["probe_name"],
                    "cl_probe_name": cl["probe_name"],
                    "vk_mode": vk["mode"],
                    "cl_mode": cl["mode"],
                    "vk_time_ns": f"{vk_time:.0f}",
                    "cl_time_ns": f"{cl_time:.0f}",
                    "vk_vs_cl_ratio": f"{ratio:.6f}",
                    "vk_checksum": vk["checksum"],
                    "cl_checksum": cl["checksum"],
                }
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
