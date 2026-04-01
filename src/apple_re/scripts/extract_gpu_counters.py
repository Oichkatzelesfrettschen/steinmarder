#!/usr/bin/env python3
"""Extract and summarize Metal GPU counter data from xctrace XML exports.

Usage:
    python3 extract_gpu_counters.py <run_dir>
    python3 extract_gpu_counters.py <run_dir> --schema metal-gpu-counter-intervals
    python3 extract_gpu_counters.py <run_dir> --list-schemas

Produces:
    <run_dir>/gpu_counter_summary.csv
    <run_dir>/gpu_counter_analysis.md

IMPORTANT: Prior blessed runs (r5, r6, r7) captured zero hardware counter rows.
The xctrace recordings used the 'Metal Application' template which includes the
metal-gpu-counter-intervals TABLE SCHEMA but never populates it with data.

To capture actual hardware GPU counter data, record with:
    xcrun xctrace record --template 'Metal GPU Counters' \\
        --output <output.trace> -- <your_binary>

Or use the 'GPU' template which includes Metal GPU Counters plus other instruments.

After recording, export the counter table:
    xcrun xctrace export --input <output.trace> \\
        --xpath '//trace-toc[1]/run[1]/data[1]/table[@schema="metal-gpu-counter-intervals"]' \\
        --output gpu_counters.xml

Then run this script on the directory containing gpu_counters.xml.
"""

from __future__ import annotations

import csv
import pathlib
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict


COUNTER_SCHEMAS = [
    "metal-gpu-counter-intervals",
    "metal-shader-profiler-intervals",
]


def find_counter_xmls(run_dir: pathlib.Path, schema: str | None) -> list[pathlib.Path]:
    patterns = [f"*{s}*" for s in (COUNTER_SCHEMAS if schema is None else [schema])]
    found: list[pathlib.Path] = []
    for pat in patterns:
        found.extend(sorted(run_dir.glob(f"**/{pat}.xml")))
    return found


def parse_counter_xml(path: pathlib.Path) -> tuple[list[str], list[dict[str, str]]]:
    """Return (column_mnemonics, rows). Rows is empty if table has no data."""
    tree = ET.parse(path)
    root = tree.getroot()
    schema_el = root.find(".//schema")
    columns: list[str] = []
    if schema_el is not None:
        for col in schema_el.findall("col"):
            mnemonic = col.findtext("mnemonic", "")
            columns.append(mnemonic)
    rows: list[dict[str, str]] = []
    for row_el in root.findall(".//row"):
        row: dict[str, str] = {}
        for cell in row_el:
            row[cell.tag] = (cell.text or "").strip()
        if row:
            rows.append(row)
    return columns, rows


def list_schemas(run_dir: pathlib.Path) -> None:
    xmls = find_counter_xmls(run_dir, None)
    if not xmls:
        print("No GPU counter or shader-profiler XML files found.")
        return
    print(f"Found {len(xmls)} GPU counter XML files in {run_dir}:")
    for path in xmls:
        cols, rows = parse_counter_xml(path)
        print(f"  [{len(rows):4d} rows, {len(cols)} cols] {path.relative_to(run_dir)}")


def write_csv(out_path: pathlib.Path, counter_data: dict[str, list[dict[str, str]]]) -> None:
    all_keys: list[str] = ["source_file", "name", "value", "percent_value", "label", "gpu", "start", "duration"]
    with out_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=all_keys, extrasaction="ignore")
        writer.writeheader()
        for source_file, rows in counter_data.items():
            for row in rows:
                row["source_file"] = source_file
                writer.writerow(row)


def write_analysis(
    out_path: pathlib.Path,
    run_dir: pathlib.Path,
    counter_data: dict[str, list[dict[str, str]]],
    empty_files: list[pathlib.Path],
) -> None:
    total_rows = sum(len(v) for v in counter_data.values())
    lines = [
        "# Metal GPU Counter Analysis",
        "",
        f"- run_dir: `{run_dir}`",
        f"- total_counter_rows: {total_rows}",
        f"- files_with_data: {len(counter_data)}",
        f"- schema-only_files (0 rows): {len(empty_files)}",
        "",
    ]
    if empty_files:
        lines += [
            "## Schema-only captures (0 data rows)",
            "",
            "These files contain only the table schema, no counter values.",
            "This means the xctrace recording was made with a template that does",
            "**not** enable hardware GPU counter sampling.",
            "",
            "**Fix**: Record with `xcrun xctrace record --template 'Metal GPU Counters'`",
            "instead of `--template 'Metal Application'`.",
            "",
            "Empty files:",
        ]
        for path in empty_files:
            lines.append(f"- `{path.name}`")
        lines.append("")
    if counter_data:
        lines += [
            "## Counter values found",
            "",
            "| File | Counter Name | Value | Percent | Label |",
            "|------|-------------|-------|---------|-------|",
        ]
        for source_file, rows in counter_data.items():
            for row in rows[:20]:
                lines.append(
                    f"| {source_file} | {row.get('name','')} | {row.get('value','')} "
                    f"| {row.get('percent-value','')} | {row.get('label','')} |"
                )
        lines.append("")
    else:
        lines += [
            "## No hardware counter data captured",
            "",
            "All GPU counter XML exports in this run directory are schema-only.",
            "",
            "### How to capture hardware GPU counters",
            "",
            "1. Use the `Metal GPU Counters` or `GPU` template:",
            "   ```sh",
            "   xcrun xctrace record \\",
            "     --template 'Metal GPU Counters' \\",
            "     --output gpu_counters.trace \\",
            "     -- ./sm_apple_metal_probe_host",
            "   ```",
            "",
            "2. Export the counter table:",
            "   ```sh",
            "   xcrun xctrace export \\",
            "     --input gpu_counters.trace \\",
            "     --xpath '//trace-toc[1]/run[1]/data[1]/table[@schema=\"metal-gpu-counter-intervals\"]' \\",
            "     --output gpu_counter_data.xml",
            "   ```",
            "",
            "3. Run this script on the directory containing gpu_counter_data.xml.",
            "",
            "### Available templates with GPU counter support",
            "- `Metal GPU Counters` — dedicated GPU counter template",
            "- `GPU` — GPU + Metal + counter bundle",
            "",
            "### Counter families exposed by Metal GPU Counters on Apple silicon",
            "- Vertex cycles, fragment cycles, tiling cycles",
            "- ALU utilization (vertex, fragment, compute)",
            "- Texture unit utilization",
            "- Memory read/write bandwidth",
            "- L1 tile cache hit rate, L2 cache hit rate",
            "- SIMD utilization",
            "- Fragment overflow (spill to memory)",
            "",
        ]
    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("run_dir", help="Run directory containing xctrace XML exports")
    parser.add_argument("--schema", help="Specific schema name to look for")
    parser.add_argument("--list-schemas", action="store_true", help="List found XML files and row counts, then exit")
    args = parser.parse_args()

    run_dir = pathlib.Path(args.run_dir).resolve()
    if not run_dir.is_dir():
        print(f"error: {run_dir} is not a directory", file=sys.stderr)
        return 1

    if args.list_schemas:
        list_schemas(run_dir)
        return 0

    xmls = find_counter_xmls(run_dir, args.schema)
    if not xmls:
        print("No GPU counter XML files found. Run with --list-schemas to debug.")
        return 1

    counter_data: dict[str, list[dict[str, str]]] = {}
    empty_files: list[pathlib.Path] = []
    for xml_path in xmls:
        cols, rows = parse_counter_xml(xml_path)
        if rows:
            counter_data[xml_path.name] = rows
        else:
            empty_files.append(xml_path)

    out_csv = run_dir / "gpu_counter_summary.csv"
    out_md = run_dir / "gpu_counter_analysis.md"
    write_csv(out_csv, counter_data)
    write_analysis(out_md, run_dir, counter_data, empty_files)
    print(f"wrote {out_csv}")
    print(f"wrote {out_md}")
    total = sum(len(v) for v in counter_data.values())
    print(f"total counter rows: {total} (0 = schema-only captures, see gpu_counter_analysis.md)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
