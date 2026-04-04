#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import statistics
import xml.etree.ElementTree as ET
from pathlib import Path

SELECTED_GPU_COUNTERS = [
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

SELECTED_GPU_COUNTER_DELTA_COUNTERS = [
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

CPU_PROBE_MAP = {
    "add_dep_u32": {
        "format": "uint32",
        "bit_width": "32",
        "support_tier": "native",
        "boundary_kind": "typed_reference",
        "metric": "dep_chain_ns_per_op",
        "notes": "native_aarch64_w_register_add_dep_chain",
    },
    "add_dep_i64": {
        "format": "int64",
        "bit_width": "64",
        "support_tier": "native",
        "boundary_kind": "typed_reference",
        "metric": "dep_chain_ns_per_op",
        "notes": "native_aarch64_x_register_add_dep_chain",
    },
    "add_dep_i16": {
        "format": "int16",
        "bit_width": "16",
        "support_tier": "compiler_lowered",
        "boundary_kind": "typed_lowering_reference",
        "metric": "dep_chain_ns_per_op",
        "notes": "aarch64_add_plus_sxth_lowering_chain",
    },
    "add_dep_i8": {
        "format": "int8",
        "bit_width": "8",
        "support_tier": "compiler_lowered",
        "boundary_kind": "typed_lowering_reference",
        "metric": "dep_chain_ns_per_op",
        "notes": "aarch64_add_plus_sxtb_lowering_chain",
    },
    "add_dep_u16": {
        "format": "uint16",
        "bit_width": "16",
        "support_tier": "compiler_lowered",
        "boundary_kind": "typed_lowering_reference",
        "metric": "dep_chain_ns_per_op",
        "notes": "aarch64_add_plus_uxth_lowering_chain",
    },
    "add_dep_u8": {
        "format": "uint8",
        "bit_width": "8",
        "support_tier": "compiler_lowered",
        "boundary_kind": "typed_lowering_reference",
        "metric": "dep_chain_ns_per_op",
        "notes": "aarch64_add_plus_uxtb_lowering_chain",
    },
    "fadd_dep_f32": {
        "format": "float32",
        "bit_width": "32",
        "support_tier": "native",
        "boundary_kind": "typed_reference",
        "metric": "dep_chain_ns_per_op",
        "notes": "native_aarch64_scalar_fadd_dep_chain",
    },
    "bf16_f32_roundtrip_proxy": {
        "format": "bfloat16",
        "bit_width": "16",
        "support_tier": "host_proxy",
        "boundary_kind": "typed_proxy_reference",
        "metric": "roundtrip_proxy_ns_per_op",
        "notes": "host_bfloat16_quantize_roundtrip_proxy",
    },
}


def load_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh))


def write_csv(path: Path, rows: list[dict[str, str]], fieldnames: list[str]) -> None:
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def relative_artifact_path(sass_re_dir: Path, artifact: Path) -> str:
    return os.path.relpath(artifact, sass_re_dir).replace(os.sep, "/")


def format_bits(taxonomy_rows: list[dict[str, str]]) -> dict[str, str]:
    out: dict[str, str] = {}
    for row in taxonomy_rows:
        out[row["format"]] = row["bit_width"]
    return out


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


def trace_variant_name(trace_name: str) -> str:
    name = trace_name
    if name.startswith("gpu_"):
        name = name[len("gpu_"):]
    if name.endswith(".trace"):
        name = name[:-len(".trace")]
    return name


def metric_safe_name(schema: str) -> str:
    return schema.replace("-", "_")


def slugify_metric(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", value.lower()).strip("_")


def canonical_format_name(name: str, taxonomy_bits: dict[str, str]) -> str | None:
    candidate = (name or "").strip()
    while True:
        if candidate.startswith("fmt_"):
            candidate = candidate[len("fmt_"):]
            continue
        if candidate.startswith("trace_"):
            candidate = candidate[len("trace_"):]
            continue
        break
    return candidate if candidate in taxonomy_bits else None


def command_buffer_gap_rows(run_dir: Path, sass_re_dir: Path, taxonomy_bits: dict[str, str]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for xml_path in sorted(run_dir.glob("xctrace_*_metal-command-buffer-completed.xml")):
        try:
            root = ET.fromstring(xml_path.read_text())
        except ET.ParseError:
            continue
        timestamps: list[int] = []
        for start_time in root.findall(".//start-time"):
            try:
                timestamps.append(int((start_time.text or "").strip()))
            except ValueError:
                continue
        if len(timestamps) < 2:
            continue
        timestamps.sort()
        gaps = [curr - prev for prev, curr in zip(timestamps, timestamps[1:]) if curr >= prev]
        if not gaps:
            continue
        variant = trace_variant_name(xml_path.name.replace("xctrace_", "").replace("_metal-command-buffer-completed.xml", ""))
        format_name = canonical_format_name(variant, taxonomy_bits)
        backend = "agx_trace"
        metric = f"{variant}_command_buffer_completion_gap_ns"
        bit_width = taxonomy_bits.get("float32", "32")
        notes = f"source=metal-command-buffer-completed, completions={len(timestamps)}"
        if format_name is not None:
            backend = "agx_format_trace"
            metric = "command_buffer_completion_gap_ns"
            bit_width = taxonomy_bits.get(format_name, "")
            notes = f"format_variant={variant}, " + notes
        artifact = relative_artifact_path(sass_re_dir, xml_path)
        rows.append({
            "lane": "metal",
            "backend": backend,
            "format": format_name or "float32",
            "bit_width": bit_width,
            "metric": metric,
            "unit": "ns",
            "median": str(statistics.median(gaps)),
            "p90": str(percentile([float(v) for v in gaps], 0.9)),
            "warmup_excluded": "yes",
            "artifact": artifact,
            "notes": notes,
        })
    return rows


def replace_rows(
    existing_rows: list[dict[str, str]],
    new_rows: list[dict[str, str]],
    key_fields: tuple[str, ...],
) -> list[dict[str, str]]:
    incoming = {tuple(row.get(field, "") for field in key_fields): row for row in new_rows}
    loose_incoming: set[tuple[str, ...]] = set()
    if "bit_width" in key_fields:
        bit_index = key_fields.index("bit_width")
        for row in new_rows:
            key = list(tuple(row.get(field, "") for field in key_fields))
            key[bit_index] = ""
            loose_incoming.add(tuple(key))
    kept: list[dict[str, str]] = []
    for row in existing_rows:
        key = tuple(row.get(field, "") for field in key_fields)
        if key in incoming or key in loose_incoming:
            continue
        kept.append(row)
    kept.extend(new_rows)
    return kept


def normalize_boundary_rows(
    run_dir: Path,
    sass_re_dir: Path,
    taxonomy_bits: dict[str, str],
) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    metal_csv = run_dir / "metal_type_surface_matrix.csv"
    if metal_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, metal_csv)
        for row in load_csv(metal_csv):
            status = "classified"
            if row["compile_status"] == "not_attempted":
                status = "planned"
            elif row["compile_status"] == "failed":
                status = "classified_failed"
            rows.append({
                "lane": "metal",
                "backend": "air_type_surface",
                "format": row["format"],
                "bit_width": row["bit_width"],
                "support_tier": row["support_tier"],
                "boundary_kind": "type_surface",
                "probe_or_model": "classify_metal_type_surface",
                "workload_id": "compile_and_air_classification",
                "artifact": artifact,
                "status": status,
            })

    neural_csv = run_dir / "neural_backend_matrix.csv"
    if neural_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, neural_csv)
        for row in load_csv(neural_csv):
            backend = row["backend"]
            support_tier = row["support_tier"]
            if support_tier == "placement_only":
                boundary_kind = "compute_unit_boundary"
            elif backend.endswith("_proxy") or row["workload"] == "typed_proxy" or support_tier == "framework_proxy":
                boundary_kind = "typed_model_boundary"
            else:
                boundary_kind = "backend_boundary"
            rows.append({
                "lane": "neural",
                "backend": backend,
                "format": row["format"],
                "bit_width": taxonomy_bits.get(row["format"], ""),
                "support_tier": support_tier,
                "boundary_kind": boundary_kind,
                "probe_or_model": "neural_typed_matrix",
                "workload_id": row["workload"],
                "artifact": artifact,
                "status": row["status"],
            })
    cpu_run_dir = run_dir / "cpu_runs"
    if cpu_run_dir.exists():
        artifact = relative_artifact_path(sass_re_dir, cpu_run_dir / "all_probes.csv") if (cpu_run_dir / "all_probes.csv").exists() else relative_artifact_path(sass_re_dir, cpu_run_dir)
        for probe_name, meta in CPU_PROBE_MAP.items():
            probe_csv = cpu_run_dir / f"{probe_name}.csv"
            if not probe_csv.exists():
                continue
            rows.append({
                "lane": "cpu",
                "backend": "apple_cpu_reference",
                "format": meta["format"],
                "bit_width": meta["bit_width"],
                "support_tier": meta["support_tier"],
                "boundary_kind": meta["boundary_kind"],
                "probe_or_model": "apple_cpu_latency",
                "workload_id": probe_name,
                "artifact": relative_artifact_path(sass_re_dir, probe_csv),
                "status": "measured",
            })
    return rows


def normalize_timing_rows(
    run_dir: Path,
    sass_re_dir: Path,
    taxonomy_bits: dict[str, str],
) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    metal_timing_csv = run_dir / "metal_timing.csv"
    if metal_timing_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, metal_timing_csv)
        for row in load_csv(metal_timing_csv):
            variant = row.get("variant", "").strip()
            prefix = f"{variant}_" if variant else ""
            notes = []
            if variant:
                notes.append(f"variant={variant}")
            if row.get("width"):
                notes.append(f"width={row['width']}")
            if row.get("iters"):
                notes.append(f"iters={row['iters']}")
            if row.get("checksum"):
                notes.append(f"checksum={row['checksum']}")
            note_text = ", ".join(notes)
            if row.get("ns_per_iter"):
                rows.append({
                    "lane": "metal",
                    "backend": "agx_dispatch",
                    "format": "float32",
                    "bit_width": taxonomy_bits.get("float32", "32"),
                    "metric": f"{prefix}cpu_wall_time_ns_per_iter",
                    "unit": "ns",
                    "median": row["ns_per_iter"],
                    "p90": "",
                    "warmup_excluded": "no",
                    "artifact": artifact,
                    "notes": note_text,
                })
            if row.get("ns_per_element"):
                rows.append({
                    "lane": "metal",
                    "backend": "agx_dispatch",
                    "format": "float32",
                    "bit_width": taxonomy_bits.get("float32", "32"),
                    "metric": f"{prefix}cpu_wall_time_ns_per_element",
                    "unit": "ns",
                    "median": row["ns_per_element"],
                    "p90": "",
                    "warmup_excluded": "no",
                    "artifact": artifact,
                    "notes": note_text,
                })

    frontier_timing_csv = run_dir / "metal_frontier_timing.csv"
    if frontier_timing_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, frontier_timing_csv)
        for row in load_csv(frontier_timing_csv):
            backend = "agx_type_surface"
            if row.get("classification_mode", "").strip() == "frontier_proxy":
                backend = "agx_frontier_proxy"
            notes = []
            if row.get("classification_mode"):
                notes.append(f"classification_mode={row['classification_mode']}")
            if row.get("kernel_name"):
                notes.append(f"kernel={row['kernel_name']}")
            if row.get("measurement_backend"):
                notes.append(f"measurement_backend={row['measurement_backend']}")
            if row.get("width"):
                notes.append(f"width={row['width']}")
            if row.get("iters"):
                notes.append(f"iters={row['iters']}")
            if row.get("checksum"):
                notes.append(f"checksum={row['checksum']}")
            if row.get("support_tier"):
                notes.append(f"support_tier={row['support_tier']}")
            if row.get("notes"):
                notes.append(row["notes"])
            note_text = ", ".join(notes)
            if row.get("ns_per_iter"):
                rows.append({
                    "lane": "metal",
                    "backend": backend,
                    "format": row["format"],
                    "bit_width": row["bit_width"] or taxonomy_bits.get(row["format"], ""),
                    "metric": "type_surface_cpu_wall_ns_per_iter",
                    "unit": "ns",
                    "median": row["ns_per_iter"],
                    "p90": "",
                    "warmup_excluded": "no",
                    "artifact": artifact,
                    "notes": note_text,
                })
            if row.get("ns_per_element"):
                rows.append({
                    "lane": "metal",
                    "backend": backend,
                    "format": row["format"],
                    "bit_width": row["bit_width"] or taxonomy_bits.get(row["format"], ""),
                    "metric": "type_surface_cpu_wall_ns_per_element",
                    "unit": "ns",
                    "median": row["ns_per_element"],
                    "p90": "",
                    "warmup_excluded": "no",
                    "artifact": artifact,
                    "notes": note_text,
                })

    trace_health_csv = run_dir / "xctrace_trace_health.csv"
    if trace_health_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, trace_health_csv)
        for row in load_csv(trace_health_csv):
            variant = trace_variant_name(row["trace"])
            format_name = canonical_format_name(variant, taxonomy_bits)
            notes = f"template={row.get('template_name', '')}, end_reason={row.get('end_reason', '')}, target_exit_status={row.get('target_exit_status', '')}"
            backend = "agx_trace"
            metric = f"{variant}_trace_duration_s"
            bit_width = taxonomy_bits.get("float32", "32")
            if format_name is not None:
                backend = "agx_format_trace"
                metric = "trace_duration_s"
                bit_width = taxonomy_bits.get(format_name, "")
                notes = f"format_variant={variant}, {notes}"
            if row.get("duration_s"):
                rows.append({
                    "lane": "metal",
                    "backend": backend,
                    "format": format_name or "float32",
                    "bit_width": bit_width,
                    "metric": metric,
                    "unit": "s",
                    "median": row["duration_s"],
                    "p90": "",
                    "warmup_excluded": "no",
                    "artifact": artifact,
                    "notes": notes,
                })

    row_count_csv = run_dir / "xctrace_metric_row_counts.csv"
    if row_count_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, row_count_csv)
        for row in load_csv(row_count_csv):
            variant = trace_variant_name(row["trace"])
            schema = metric_safe_name(row["schema"])
            format_name = canonical_format_name(variant, taxonomy_bits)
            backend = "agx_trace"
            metric = f"{variant}_{schema}_rows"
            bit_width = taxonomy_bits.get("float32", "32")
            notes = f"export_file={row.get('export_file', '')}"
            if format_name is not None:
                backend = "agx_format_trace"
                metric = f"{schema}_rows"
                bit_width = taxonomy_bits.get(format_name, "")
                notes = f"format_variant={variant}, {notes}"
            rows.append({
                "lane": "metal",
                "backend": backend,
                "format": format_name or "float32",
                "bit_width": bit_width,
                "metric": metric,
                "unit": "rows",
                "median": row["rows"],
                "p90": "",
                "warmup_excluded": "no",
                "artifact": artifact,
                "notes": notes,
            })

    density_csv = run_dir / "xctrace_row_density.csv"
    if density_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, density_csv)
        for row in load_csv(density_csv):
            variant = trace_variant_name(row["variant_trace"])
            schema = metric_safe_name(row["schema"])
            format_name = canonical_format_name(variant, taxonomy_bits)
            backend = "agx_trace"
            density_metric = f"{variant}_{schema}_density_rps"
            delta_metric = f"{variant}_{schema}_delta_density_rps"
            bit_width = taxonomy_bits.get("float32", "32")
            density_notes = f"baseline_density_rps={row.get('baseline_density_rps', '')}"
            delta_notes = f"baseline_density_rps={row.get('baseline_density_rps', '')}, variant_density_rps={row.get('variant_density_rps', '')}"
            if format_name is not None:
                backend = "agx_format_trace"
                density_metric = f"{schema}_density_rps"
                delta_metric = f"{schema}_delta_density_rps"
                bit_width = taxonomy_bits.get(format_name, "")
                density_notes = f"format_variant={variant}, {density_notes}"
                delta_notes = f"format_variant={variant}, {delta_notes}"
            rows.append({
                "lane": "metal",
                "backend": backend,
                "format": format_name or "float32",
                "bit_width": bit_width,
                "metric": density_metric,
                "unit": "rows_per_s",
                "median": row["variant_density_rps"],
                "p90": "",
                "warmup_excluded": "yes",
                "artifact": artifact,
                "notes": density_notes,
            })
            rows.append({
                "lane": "metal",
                "backend": backend,
                "format": format_name or "float32",
                "bit_width": bit_width,
                "metric": delta_metric,
                "unit": "rows_per_s",
                "median": row["delta_density_rps"],
                "p90": "",
                "warmup_excluded": "yes",
                "artifact": artifact,
                "notes": delta_notes,
            })

    rows.extend(command_buffer_gap_rows(run_dir, sass_re_dir, taxonomy_bits))

    hardware_counter_csv = run_dir / "gpu_hardware_counters.csv"
    if hardware_counter_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, hardware_counter_csv)
        for row in load_csv(hardware_counter_csv):
            counter_name = row.get("counter", "")
            if counter_name not in SELECTED_GPU_COUNTERS:
                continue
            variant = row.get("variant", "").strip() or "unknown"
            format_name = canonical_format_name(variant, taxonomy_bits)
            backend = "agx_counters"
            bit_width = taxonomy_bits.get("float32", "32")
            counter_slug = slugify_metric(counter_name)
            sample_count = row.get("n_samples", "")
            peak_notes = []
            if sample_count:
                peak_notes.append(f"n_samples={sample_count}")
            if row.get("p90_pct"):
                peak_notes.append(f"p90_pct={row['p90_pct']}")
            if row.get("p50_pct"):
                peak_notes.append(f"p50_pct={row['p50_pct']}")
            peak_notes_text = ", ".join(peak_notes)
            peak_metric = f"{variant}_{counter_slug}_peak"
            avg_metric = f"{variant}_{counter_slug}_avg"
            avg_notes = f"n_samples={sample_count}, p50_pct={row.get('p50_pct', '')}"
            if format_name is not None:
                backend = "agx_format_counters"
                bit_width = taxonomy_bits.get(format_name, "")
                peak_metric = f"{counter_slug}_peak"
                avg_metric = f"{counter_slug}_avg"
                peak_notes_text = f"format_variant={variant}, {peak_notes_text}" if peak_notes_text else f"format_variant={variant}"
                avg_notes = f"format_variant={variant}, {avg_notes}"
            rows.append({
                "lane": "metal",
                "backend": backend,
                "format": format_name or "float32",
                "bit_width": bit_width,
                "metric": peak_metric,
                "unit": "pct",
                "median": row.get("max_pct", ""),
                "p90": row.get("p99_pct", ""),
                "warmup_excluded": "yes",
                "artifact": artifact,
                "notes": peak_notes_text,
            })
            rows.append({
                "lane": "metal",
                "backend": backend,
                "format": format_name or "float32",
                "bit_width": bit_width,
                "metric": avg_metric,
                "unit": "pct",
                "median": row.get("avg_pct", ""),
                "p90": row.get("p90_pct", ""),
                "warmup_excluded": "yes",
                "artifact": artifact,
                "notes": avg_notes,
            })

    hardware_counter_delta_csv = run_dir / "gpu_hardware_counter_deltas.csv"
    if hardware_counter_delta_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, hardware_counter_delta_csv)
        for row in load_csv(hardware_counter_delta_csv):
            counter_name = row.get("counter", "")
            if counter_name not in SELECTED_GPU_COUNTER_DELTA_COUNTERS:
                continue
            variant = row.get("variant", "").strip() or "unknown"
            baseline = row.get("baseline_variant", "").strip() or "baseline"
            format_name = canonical_format_name(variant, taxonomy_bits)
            baseline_format = canonical_format_name(baseline, taxonomy_bits)
            backend = "agx_counter_deltas"
            bit_width = taxonomy_bits.get("float32", "32")
            counter_slug = slugify_metric(counter_name)
            avg_ratio = row.get("avg_ratio", "").strip()
            avg_metric = f"{variant}_vs_{baseline}_{counter_slug}_avg_ratio"
            p99_metric = f"{variant}_vs_{baseline}_{counter_slug}_p99_delta_pct"
            avg_notes = f"baseline_avg_pct={row.get('baseline_avg_pct', '')}, variant_avg_pct={row.get('variant_avg_pct', '')}"
            p99_notes = f"baseline_p99_pct={row.get('baseline_p99_pct', '')}, variant_p99_pct={row.get('variant_p99_pct', '')}"
            if format_name is not None:
                backend = "agx_format_counter_deltas"
                bit_width = taxonomy_bits.get(format_name, "")
                baseline_label = baseline_format or baseline
                avg_metric = f"vs_{baseline_label}_{counter_slug}_avg_ratio"
                p99_metric = f"vs_{baseline_label}_{counter_slug}_p99_delta_pct"
                avg_notes = f"format_variant={variant}, {avg_notes}"
                p99_notes = f"format_variant={variant}, {p99_notes}"
            if avg_ratio:
                rows.append({
                    "lane": "metal",
                    "backend": backend,
                    "format": format_name or "float32",
                    "bit_width": bit_width,
                    "metric": avg_metric,
                    "unit": "ratio",
                    "median": avg_ratio,
                    "p90": "",
                    "warmup_excluded": "yes",
                    "artifact": artifact,
                    "notes": avg_notes,
                })
            rows.append({
                "lane": "metal",
                "backend": backend,
                "format": format_name or "float32",
                "bit_width": bit_width,
                "metric": p99_metric,
                "unit": "pct",
                "median": row.get("p99_delta_pct", ""),
                "p90": "",
                "warmup_excluded": "yes",
                "artifact": artifact,
                "notes": p99_notes,
            })

    timing_csv = run_dir / "neural_timing_matrix.csv"
    if timing_csv.exists():
        artifact = relative_artifact_path(sass_re_dir, timing_csv)
        for row in load_csv(timing_csv):
            rows.append({
                "lane": "neural",
                "backend": row["backend"],
                "format": row["format"],
                "bit_width": taxonomy_bits.get(row["format"], ""),
                "metric": f"{row['workload']}_ms",
                "unit": "ms",
                "median": row["median_ms"],
                "p90": row["p90_ms"],
                "warmup_excluded": "yes" if int(row.get("warmup_runs", "0") or "0") > 0 else "no",
                "artifact": artifact,
                "notes": row.get("notes", ""),
            })
    cpu_run_dir = run_dir / "cpu_runs"
    if cpu_run_dir.exists():
        for probe_name, meta in CPU_PROBE_MAP.items():
            probe_csv = cpu_run_dir / f"{probe_name}.csv"
            if not probe_csv.exists():
                continue
            probe_rows = load_csv(probe_csv)
            if not probe_rows:
                continue
            row = probe_rows[0]
            notes = [meta["notes"]]
            if row.get("iters"):
                notes.append(f"iters={row['iters']}")
            if row.get("unroll"):
                notes.append(f"unroll={row['unroll']}")
            if row.get("elapsed_ns"):
                notes.append(f"elapsed_ns={row['elapsed_ns']}")
            if row.get("approx_gops"):
                notes.append(f"approx_gops={row['approx_gops']}")
            rows.append({
                "lane": "cpu",
                "backend": "apple_cpu_reference",
                "format": meta["format"],
                "bit_width": meta["bit_width"],
                "metric": meta["metric"],
                "unit": "ns",
                "median": row.get("ns_per_op", ""),
                "p90": "",
                "warmup_excluded": "no",
                "artifact": relative_artifact_path(sass_re_dir, probe_csv),
                "notes": ", ".join(notes),
            })
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-dir", required=True, help="Path to src/apple_re/results/... run directory")
    parser.add_argument("--sass-re-dir", default=str(Path(__file__).resolve().parents[1]))
    parser.add_argument("--taxonomy-table")
    parser.add_argument("--boundary-table")
    parser.add_argument("--timing-table")
    args = parser.parse_args()

    sass_re_dir = Path(args.sass_re_dir).resolve()
    run_dir = Path(args.run_dir).resolve()
    taxonomy_table = Path(args.taxonomy_table) if args.taxonomy_table else sass_re_dir / "tables" / "table_apple_type_taxonomy.csv"
    boundary_table = Path(args.boundary_table) if args.boundary_table else sass_re_dir / "tables" / "table_apple_type_boundary_matrix.csv"
    timing_table = Path(args.timing_table) if args.timing_table else sass_re_dir / "tables" / "table_apple_type_timings.csv"

    taxonomy_rows = load_csv(taxonomy_table)
    boundary_rows = load_csv(boundary_table)
    timing_rows = load_csv(timing_table)
    taxonomy_bits = format_bits(taxonomy_rows)

    new_boundary_rows = normalize_boundary_rows(run_dir, sass_re_dir, taxonomy_bits)
    new_timing_rows = normalize_timing_rows(run_dir, sass_re_dir, taxonomy_bits)

    boundary_key = ("lane", "backend", "format", "bit_width", "boundary_kind", "probe_or_model", "workload_id")
    timing_key = ("lane", "backend", "format", "bit_width", "metric")

    merged_boundary = replace_rows(boundary_rows, new_boundary_rows, boundary_key)
    merged_timing = replace_rows(timing_rows, new_timing_rows, timing_key)

    boundary_fields = list(boundary_rows[0].keys()) if boundary_rows else [
        "lane", "backend", "format", "bit_width", "support_tier", "boundary_kind", "probe_or_model", "workload_id", "artifact", "status",
    ]
    timing_fields = list(timing_rows[0].keys()) if timing_rows else [
        "lane", "backend", "format", "bit_width", "metric", "unit", "median", "p90", "warmup_excluded", "artifact", "notes",
    ]

    write_csv(boundary_table, merged_boundary, boundary_fields)
    write_csv(timing_table, merged_timing, timing_fields)

    print(json.dumps({
        "run_dir": str(run_dir),
        "boundary_updates": len(new_boundary_rows),
        "timing_updates": len(new_timing_rows),
        "boundary_table": str(boundary_table),
        "timing_table": str(timing_table),
    }, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
