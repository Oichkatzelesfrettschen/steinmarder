#!/usr/bin/env python3
"""Extract structured metrics from xctrace artifacts for tranche reports."""

from __future__ import annotations

import argparse
import csv
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


INTERESTING_SCHEMAS = {
    # Generic timeline/profile context
    "time-profile",
    "time-sample",
    "process-info",
    "thread-info",
    "gcd-perf-event",
    "dyld-library-load",
    "life-cycle-period",
    # Metal/GPU execution path
    "metal-application-encoders-list",
    "metal-command-buffer-completed",
    "metal-driver-event-per-thread-intervals",
    "metal-driver-intervals",
    "metal-gpu-counter-intervals",
    "metal-gpu-intervals",
    "metal-gpu-state-intervals",
    "metal-shader-profiler-intervals",
}


def _run_xctrace(args: list[str]) -> str:
    try:
        proc = subprocess.run(
            ["xctrace", "export", *args],
            capture_output=True,
            text=True,
            check=False,
            timeout=180,
        )
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError(f"xctrace export timed out: {' '.join(args)}") from exc
    if proc.returncode != 0:
        stderr = proc.stderr.strip()
        raise RuntimeError(f"xctrace export failed ({proc.returncode}): {stderr}")
    return proc.stdout


def _safe_name(schema: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", schema)


def _is_interesting_schema(schema: str) -> bool:
    return schema in INTERESTING_SCHEMAS


def _parse_toc(trace_name: str, toc_xml: str) -> tuple[dict[str, str], list[str]]:
    root = ET.fromstring(toc_xml)
    run = root.find("./run")
    info = run.find("./info") if run is not None else None
    summary = info.find("./summary") if info is not None else None
    target_process = info.find("./target/process") if info is not None else None
    tables = run.findall("./data/table") if run is not None else []

    row = {
        "trace": trace_name,
        "template_name": (summary.findtext("./template-name") if summary is not None else "") or "",
        "duration_s": (summary.findtext("./duration") if summary is not None else "") or "",
        "end_reason": (summary.findtext("./end-reason") if summary is not None else "") or "",
        "target_process_name": (target_process.get("name") if target_process is not None else "") or "",
        "target_pid": (target_process.get("pid") if target_process is not None else "") or "",
        "target_exit_status": (target_process.get("return-exit-status") if target_process is not None else "") or "",
        "target_termination_reason": (target_process.get("termination-reason") if target_process is not None else "") or "",
        "schema_count": str(len(tables)),
    }

    schemas = sorted(
        {
            table.get("schema", "")
            for table in tables
            if table.get("schema")
        }
    )
    return row, schemas


def _count_rows(query_xml: str) -> int:
    root = ET.fromstring(query_xml)
    count = 0
    for node in root.findall("./node"):
        count += len(node.findall(".//row"))
    return count


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("out_dir", type=Path, help="Tranche output directory")
    args = parser.parse_args()

    out_dir = args.out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    trace_paths = sorted(out_dir.glob("*.trace"))
    (out_dir / "xctrace_artifacts.txt").write_text(
        "".join(f"{trace}\n" for trace in trace_paths),
        encoding="utf-8",
    )

    trace_health_rows: list[dict[str, str]] = []
    schema_inventory_rows: list[dict[str, str]] = []
    row_count_rows: list[dict[str, str]] = []

    for trace_path in trace_paths:
        trace_name = trace_path.name
        trace_stem = trace_path.stem

        toc_xml = _run_xctrace(["--input", str(trace_path), "--toc"])
        toc_path = out_dir / f"{trace_stem}_toc.xml"
        toc_path.write_text(toc_xml, encoding="utf-8")

        trace_row, schemas = _parse_toc(trace_name, toc_xml)
        trace_health_rows.append(trace_row)

        for schema in schemas:
            schema_inventory_rows.append(
                {"trace": trace_name, "schema": schema}
            )
            if not _is_interesting_schema(schema):
                continue

            xpath = f'/trace-toc/run[@number="1"]/data/table[@schema="{schema}"]'
            export_xml = _run_xctrace(["--input", str(trace_path), "--xpath", xpath])
            export_name = f"xctrace_{trace_stem}_{_safe_name(schema)}.xml"
            export_path = out_dir / export_name
            export_path.write_text(export_xml, encoding="utf-8")
            row_count_rows.append(
                {
                    "trace": trace_name,
                    "schema": schema,
                    "rows": str(_count_rows(export_xml)),
                    "export_file": export_name,
                }
            )

    health_csv = out_dir / "xctrace_trace_health.csv"
    with health_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "trace",
                "template_name",
                "duration_s",
                "end_reason",
                "target_process_name",
                "target_pid",
                "target_exit_status",
                "target_termination_reason",
                "schema_count",
            ],
        )
        writer.writeheader()
        writer.writerows(trace_health_rows)

    schema_csv = out_dir / "xctrace_schema_inventory.csv"
    with schema_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["trace", "schema"])
        writer.writeheader()
        writer.writerows(schema_inventory_rows)

    row_counts_csv = out_dir / "xctrace_metric_row_counts.csv"
    with row_counts_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f, fieldnames=["trace", "schema", "rows", "export_file"]
        )
        writer.writeheader()
        writer.writerows(row_count_rows)

    summary_md = out_dir / "xctrace_export_summary.md"
    with summary_md.open("w", encoding="utf-8") as f:
        f.write("# xctrace Export Summary\n\n")
        f.write(f"- traces_found: {len(trace_paths)}\n")
        f.write(f"- trace_health_csv: {health_csv.name}\n")
        f.write(f"- schema_inventory_csv: {schema_csv.name}\n")
        f.write(f"- metric_row_counts_csv: {row_counts_csv.name}\n")
        f.write(f"- per-schema_xml_exports: {len(row_count_rows)}\n")

    print(f"traces_found={len(trace_paths)}")
    print(f"schema_exports={len(row_count_rows)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"error={exc}", file=sys.stderr)
        raise SystemExit(1)
