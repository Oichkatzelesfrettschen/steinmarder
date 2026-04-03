#!/usr/bin/env python3
"""Extract and summarize Metal GPU counter data from xctrace XML exports.

Usage:
    python3 extract_gpu_counters.py <run_dir>
    python3 extract_gpu_counters.py <run_dir> --schema metal-gpu-counter-intervals
    python3 extract_gpu_counters.py <run_dir> --list-schemas

Produces:
    <run_dir>/gpu_counter_summary.csv
    <run_dir>/gpu_hardware_counters.csv
    <run_dir>/gpu_hardware_counters.md
    <run_dir>/gpu_counter_analysis.md
"""

from __future__ import annotations

import csv
import os
import pathlib
import statistics
import sys
import xml.etree.ElementTree as ET
from concurrent.futures import ProcessPoolExecutor


COUNTER_SCHEMAS = [
    "metal-gpu-counter-intervals",
    "metal-shader-profiler-intervals",
]

FOCUS_COUNTERS = [
    "ALU Utilization",
    "F16 Utilization",
    "F32 Utilization",
    "Fragment Occupancy",
    "Compute Occupancy",
    "GPU Read Bandwidth",
    "GPU Write Bandwidth",
    "LLC Utilization",
    "Top Performance Limiter",
    "Total Occupancy",
]

RAW_SUMMARY_LIMIT = 50000

COUNTER_NAME_ALIASES = {
    "GPU Last Level Cache Limiter": "LLC Limiter",
    "GPU Last Level Cache Utilization": "LLC Utilization",
    "Fragment Input Interpolation Limiter": "Fragment Interp Limiter",
    "Fragment Input Interpolation Utilization": "Fragment Interp Utilization",
    "Threadgroup/Imageblock Load Limiter": "TG/IB Load Limiter",
    "Threadgroup/Imageblock Load Utilization": "TG/IB Load Utilization",
    "Threadgroup/Imageblock Store Limiter": "TG/IB Store Limiter",
    "Threadgroup/Imageblock Store Utilization": "TG/IB Store Utilization",
    "Partial Renders Count": "Partial Renders",
}


def find_counter_xmls(run_dir: pathlib.Path, schema: str | None) -> list[pathlib.Path]:
    schemas = COUNTER_SCHEMAS if schema is None else [schema]
    found: list[pathlib.Path] = []
    for item in sorted(run_dir.iterdir()):
        if not item.is_file() or item.suffix != ".xml":
            continue
        if any(f"_{candidate}" in item.name for candidate in schemas):
            found.append(item)
    if found:
        return found
    patterns = [f"*{s}*" for s in schemas]
    for pat in patterns:
        found.extend(sorted(run_dir.glob(f"**/{pat}.xml")))
    return found


def iter_counter_rows(path: pathlib.Path):
    columns: list[str] = []
    id_cache: dict[str, str] = {}
    current_col_mnemonic: str | None = None
    schema_seen = False
    row_depth = 0

    context = ET.iterparse(path, events=("start", "end"))
    for event, elem in context:
        tag = elem.tag
        if event == "start":
            if tag == "schema":
                schema_seen = True
            elif tag == "row":
                row_depth += 1
            continue

        if tag == "mnemonic" and schema_seen:
            current_col_mnemonic = (elem.text or "").strip()
        elif tag == "col" and schema_seen:
            columns.append(current_col_mnemonic or "")
            current_col_mnemonic = None
            elem.clear()
        elif tag == "schema":
            schema_seen = False
            elem.clear()
        elif tag == "row":
            row: dict[str, str] = {}
            for index, cell in enumerate(list(elem)):
                key = columns[index] if index < len(columns) else cell.tag
                fmt_value = (cell.attrib.get("fmt") or "").strip()
                text_value = (cell.text or "").strip()
                ref_id = (cell.attrib.get("ref") or "").strip()
                cell_id = (cell.attrib.get("id") or "").strip()
                if key in {"name", "label", "gpu"}:
                    value = fmt_value or text_value
                else:
                    value = text_value or fmt_value
                if not value and ref_id:
                    value = id_cache.get(ref_id, "")
                row[key] = value
                if cell_id and value:
                    id_cache[cell_id] = value
            if row:
                yield columns, row
            row_depth = max(0, row_depth - 1)
            elem.clear()
        else:
            if row_depth == 0:
                elem.clear()


def parse_counter_xml(path: pathlib.Path) -> tuple[list[str], list[dict[str, str]]]:
    columns: list[str] = []
    rows: list[dict[str, str]] = []
    for parsed_columns, row in iter_counter_rows(path):
        columns = parsed_columns
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


def percentile(values: list[float], q: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    idx = (len(ordered) - 1) * q
    low = int(idx)
    high = min(low + 1, len(ordered) - 1)
    frac = idx - low
    if low == high:
        return ordered[low]
    return ordered[low] * (1.0 - frac) + ordered[high] * frac


def detect_schema(path: pathlib.Path, columns: list[str]) -> str:
    for schema in COUNTER_SCHEMAS:
        if schema in path.name:
            return schema
    if "percent-value" in columns or "counter-id" in columns:
        return "metal-gpu-counter-intervals"
    return "unknown"


def extract_variant(path: pathlib.Path, schema: str) -> str:
    name = path.stem
    prefix = "gpu_counter_"
    suffix = f"_{schema}"
    if name.startswith(prefix) and name.endswith(suffix):
        return name[len(prefix):-len(suffix)]
    if schema != "unknown" and name.endswith(f"_{schema}"):
        return name[:-(len(schema) + 1)]
    return "unknown"


def parse_float(value: str) -> float | None:
    cleaned = (value or "").strip().replace("%", "")
    if not cleaned:
        return None
    try:
        return float(cleaned)
    except ValueError:
        return None


def normalize_row(source_file: str, schema: str, variant: str, row: dict[str, str]) -> dict[str, str]:
    return {
        "source_file": source_file,
        "schema": schema,
        "variant": variant,
        "name": row.get("name", ""),
        "value": row.get("value", ""),
        "percent_value": row.get("percent-value", ""),
        "label": row.get("label", ""),
        "gpu": row.get("gpu", ""),
        "start": row.get("start", ""),
        "duration": row.get("duration", ""),
        "is_percentage": row.get("is-percentage", ""),
        "color": row.get("color", ""),
        "group_index": row.get("group-index", ""),
        "ring_buffer_index": row.get("ring-buffer-index", ""),
    }


def write_raw_csv(out_path: pathlib.Path, rows: list[dict[str, str]]) -> None:
    fieldnames = [
        "source_file",
        "schema",
        "variant",
        "name",
        "value",
        "percent_value",
        "label",
        "gpu",
        "start",
        "duration",
        "is_percentage",
        "color",
        "group_index",
        "ring_buffer_index",
    ]
    with out_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def build_hardware_counter_rows_from_grouped(grouped: dict[tuple[str, str], list[float]]) -> list[dict[str, str]]:
    summary_rows: list[dict[str, str]] = []
    for (variant, counter), values in sorted(grouped.items()):
        summary_rows.append({
            "counter": counter,
            "variant": variant,
            "n_samples": str(len(values)),
            "min_pct": f"{min(values):.3f}",
            "p50_pct": f"{statistics.median(values):.3f}",
            "p90_pct": f"{percentile(values, 0.90):.3f}",
            "p99_pct": f"{percentile(values, 0.99):.3f}",
            "max_pct": f"{max(values):.3f}",
            "avg_pct": f"{statistics.fmean(values):.3f}",
        })
    return summary_rows


def summarize_xml(path_str: str, raw_limit: int) -> dict[str, object]:
    path = pathlib.Path(path_str)
    schema = detect_schema(path, [])
    variant = extract_variant(path, schema)
    row_count = 0
    raw_rows_sample: list[dict[str, str]] = []
    grouped: dict[tuple[str, str], list[float]] = {}
    for _, row in iter_counter_rows(path):
        row_count += 1
        normalized = normalize_row(path.name, schema, variant, row)
        if len(raw_rows_sample) < raw_limit:
            raw_rows_sample.append(normalized)
        if normalized["schema"] != "metal-gpu-counter-intervals":
            continue
        name = COUNTER_NAME_ALIASES.get(normalized.get("name", ""), normalized.get("name", ""))
        percent = parse_float(normalized.get("percent_value", ""))
        if percent is None and normalized.get("is_percentage", "") in {"1", "true", "True", "YES", "Yes"}:
            percent = parse_float(normalized.get("value", ""))
        if percent is not None and name:
            grouped.setdefault((variant, name), []).append(percent)
    return {
        "row_count": row_count,
        "raw_rows_sample": raw_rows_sample,
        "hardware_rows": build_hardware_counter_rows_from_grouped(grouped),
    }


def write_hardware_counter_csv(out_path: pathlib.Path, rows: list[dict[str, str]]) -> None:
    fieldnames = [
        "counter",
        "variant",
        "n_samples",
        "min_pct",
        "p50_pct",
        "p90_pct",
        "p99_pct",
        "max_pct",
        "avg_pct",
    ]
    with out_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def write_hardware_counter_md(out_path: pathlib.Path, run_dir: pathlib.Path, rows: list[dict[str, str]]) -> None:
    variants = sorted({row["variant"] for row in rows})
    lines = [
        "# Metal GPU Hardware Counter Summary",
        "",
        f"- run_dir: `{run_dir}`",
        f"- variants_with_data: {len(variants)}",
        f"- counter_series: {len(rows)}",
        "",
    ]
    if not rows:
        lines += [
            "No hardware counter rows were found in this run directory.",
            "",
            "Use `xcrun xctrace record --instrument 'Metal GPU Counters'` and export",
            "`metal-gpu-counter-intervals` XML to populate this summary.",
            "",
        ]
        out_path.write_text("\n".join(lines), encoding="utf-8")
        return

    for variant in variants:
        lines += [
            f"## {variant}",
            "",
            "| Counter | Samples | P90% | P99% | Max% | Avg% |",
            "|---------|---------|------|------|------|------|",
        ]
        variant_rows = [row for row in rows if row["variant"] == variant]
        focus_rows = [row for row in variant_rows if row["counter"] in FOCUS_COUNTERS]
        display_rows = focus_rows if focus_rows else variant_rows[:12]
        for row in display_rows:
            lines.append(
                f"| {row['counter']} | {row['n_samples']} | {row['p90_pct']} | {row['p99_pct']} | {row['max_pct']} | {row['avg_pct']} |"
            )
        lines.append("")
    out_path.write_text("\n".join(lines), encoding="utf-8")


def write_analysis(
    out_path: pathlib.Path,
    run_dir: pathlib.Path,
    total_raw_rows: int,
    hardware_rows: list[dict[str, str]],
    empty_files: list[pathlib.Path],
) -> None:
    lines = [
        "# Metal GPU Counter Analysis",
        "",
        f"- run_dir: `{run_dir}`",
        f"- total_raw_counter_rows: {total_raw_rows}",
        f"- hardware_counter_series: {len(hardware_rows)}",
        f"- schema_only_files (0 rows): {len(empty_files)}",
        "",
    ]
    if empty_files:
        lines.append("## Schema-only captures")
        lines.append("")
        for path in empty_files:
            lines.append(f"- `{path.name}`")
        lines.append("")
    if hardware_rows:
        lines += [
            "## Hardware summary outputs",
            "",
            "- `gpu_counter_summary.csv` preserves normalized raw rows.",
            "- `gpu_hardware_counters.csv` provides blessed per-counter percentile summaries.",
            "- `gpu_hardware_counters.md` gives a quick human-readable variant view.",
            "",
        ]
    else:
        lines += [
            "## No hardware counter data captured",
            "",
            "All exported counter XMLs were schema-only or lacked usable percentage values.",
            "",
            "Use a working capture path such as:",
            "",
            "```sh",
            "xcrun xctrace record \\",
            "  --instrument 'Metal GPU Counters' \\",
            "  --output gpu_counters.trace \\",
            "  --time-limit 20s \\",
            "  -- ./sm_apple_metal_probe_host --metallib <variant.metallib> --iters 5000000 --width 1024",
            "```",
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

    raw_rows_sample: list[dict[str, str]] = []
    total_raw_rows = 0
    empty_files: list[pathlib.Path] = []
    hardware_rows: list[dict[str, str]] = []
    max_workers = min(len(xmls), max(1, min(4, os.cpu_count() or 1)))
    per_file_raw_limit = max(1, RAW_SUMMARY_LIMIT // max(1, len(xmls)))
    with ProcessPoolExecutor(max_workers=max_workers) as pool:
        for xml_path, result in zip(
            xmls,
            pool.map(summarize_xml, [str(path) for path in xmls], [per_file_raw_limit] * len(xmls)),
        ):
            row_count = int(result["row_count"])
            total_raw_rows += row_count
            if row_count == 0:
                empty_files.append(xml_path)
            for row in result["raw_rows_sample"]:
                if len(raw_rows_sample) >= RAW_SUMMARY_LIMIT:
                    break
                raw_rows_sample.append(row)
            hardware_rows.extend(result["hardware_rows"])

    out_raw_csv = run_dir / "gpu_counter_summary.csv"
    out_hardware_csv = run_dir / "gpu_hardware_counters.csv"
    out_hardware_md = run_dir / "gpu_hardware_counters.md"
    out_analysis = run_dir / "gpu_counter_analysis.md"

    write_raw_csv(out_raw_csv, raw_rows_sample)
    write_hardware_counter_csv(out_hardware_csv, hardware_rows)
    write_hardware_counter_md(out_hardware_md, run_dir, hardware_rows)
    write_analysis(out_analysis, run_dir, total_raw_rows, hardware_rows, empty_files)

    print(f"wrote {out_raw_csv}")
    print(f"wrote {out_hardware_csv}")
    print(f"wrote {out_hardware_md}")
    print(f"wrote {out_analysis}")
    print(f"total raw counter rows: {total_raw_rows}")
    print(f"hardware counter series: {len(hardware_rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
