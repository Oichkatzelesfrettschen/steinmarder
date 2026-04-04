#!/usr/bin/env python3

from __future__ import annotations

import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TABLES = ROOT / "tables"

REQUESTED_FAMILIES = ["int", "uint", "bf", "fp", "tf", "mx"]
REQUESTED_WIDTHS = ["4", "8", "16", "32", "64"]

SLOT_OVERRIDES: dict[tuple[str, str], list[str]] = {
    ("tf", "32"): ["tf32"],
}

DOMAIN_ORDER = ["cpu", "metal", "gpu", "neural"]


def load_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


def write_csv(path: Path, rows: list[dict[str, str]], fieldnames: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def joined(values: list[str]) -> str:
    return "|".join(v for v in values if v)


def build_slot_formats(taxonomy_rows: list[dict[str, str]]) -> dict[tuple[str, str], list[str]]:
    slot_formats: dict[tuple[str, str], list[str]] = {}
    for family in REQUESTED_FAMILIES:
        for width in REQUESTED_WIDTHS:
            key = (family, width)
            if key in SLOT_OVERRIDES:
                slot_formats[key] = SLOT_OVERRIDES[key]
                continue
            formats = [
                row["format"]
                for row in taxonomy_rows
                if row["family"] == family and row["bit_width"] == width
            ]
            slot_formats[key] = formats
    return slot_formats


def collect_boundary(boundary_rows: list[dict[str, str]], lane: str, formats: list[str]) -> list[dict[str, str]]:
    if not formats:
        return []
    return [row for row in boundary_rows if row["lane"] == lane and row["format"] in formats]


def collect_timing(timing_rows: list[dict[str, str]], lane: str, formats: list[str], backends: set[str] | None = None) -> list[dict[str, str]]:
    if not formats:
        return []
    rows = [row for row in timing_rows if row["lane"] == lane and row["format"] in formats]
    if backends is not None:
        rows = [row for row in rows if row["backend"] in backends]
    return rows


def cpu_state(boundary: list[dict[str, str]], timing: list[dict[str, str]], formats: list[str]) -> tuple[str, str]:
    if timing:
        backends = sorted({row["backend"] for row in timing})
        return "measured", f"timing_backends={joined(backends)}"
    if boundary:
        tiers = sorted({row["support_tier"] for row in boundary})
        return "classified_only", f"support_tiers={joined(tiers)}"
    if formats:
        return "gap", "taxonomy exists but no CPU boundary/timing rows yet"
    return "not_modeled", "no requested CPU slot modeled in taxonomy"


def metal_state(boundary: list[dict[str, str]], timing: list[dict[str, str]], formats: list[str]) -> tuple[str, str]:
    if timing:
        backends = sorted({row["backend"] for row in timing})
        if "agx_frontier_proxy" in backends:
            return "proxy_timed", f"timing_backends={joined(backends)}"
        return "timed_surface", f"timing_backends={joined(backends)}"
    if boundary:
        tiers = sorted({row["support_tier"] for row in boundary})
        return "classified_only", f"support_tiers={joined(tiers)}"
    if formats:
        return "gap", "taxonomy exists but no Metal classifier/timing rows yet"
    return "not_modeled", "no requested Metal slot modeled in taxonomy"


def gpu_state(metal_formats: list[str], timing_rows: list[dict[str, str]]) -> tuple[str, str]:
    if not metal_formats:
        return "not_modeled", "no requested GPU slot modeled in taxonomy"
    gpu_rows = [
        row for row in timing_rows
        if row["lane"] == "metal"
        and row["format"] in metal_formats
        and row["backend"] in {
            "agx_trace",
            "agx_counters",
            "agx_counter_deltas",
            "agx_format_trace",
            "agx_format_counters",
            "agx_format_counter_deltas",
        }
    ]
    if gpu_rows:
        backends = sorted({row["backend"] for row in gpu_rows})
        return "gpu_traced", f"gpu_backends={joined(backends)}"
    return "gap", "no per-format GPU trace/counter rows yet"


def neural_state(boundary: list[dict[str, str]], timing: list[dict[str, str]], formats: list[str]) -> tuple[str, str]:
    if timing:
        backends = sorted({row["backend"] for row in timing})
        return "timed_backend", f"timing_backends={joined(backends)}"
    if boundary:
        tiers = sorted({row["support_tier"] for row in boundary})
        return "classified_only", f"support_tiers={joined(tiers)}"
    if formats:
        return "gap", "taxonomy exists but no neural boundary/timing rows yet"
    return "not_modeled", "no requested neural slot modeled in taxonomy"


def next_action(family: str, width: str, states: dict[str, str], formats: list[str]) -> str:
    if family == "tf" and width != "32":
        return "Keep unmodeled unless a non-TF32 Apple tensor-float surface appears."
    if family == "bf" and width != "16":
        return "Add only if a non-bfloat16 Apple bfloat variant is actually exposed."
    if not formats:
        return "No active slot to build yet; keep visible as an explicit gap."
    if states["metal"] in {"gap", "classified_only"}:
        return "Expand Metal type-surface kernels and sync the resulting rows."
    if states["gpu"] == "gap":
        return "Capture per-format GPU trace/counter rows for this format instead of relying on float32-only AGX telemetry."
    if states["cpu"] == "gap":
        return "Add CPU reference probes or software proxies for this slot."
    if states["neural"] == "gap":
        return "Add the format to the typed neural matrix as a direct or proxy backend row."
    return "Tighten support honesty and deepen timing/counter fidelity rather than opening a new slot."


def build_rows() -> list[dict[str, str]]:
    taxonomy_rows = load_csv(TABLES / "table_apple_type_taxonomy.csv")
    boundary_rows = load_csv(TABLES / "table_apple_type_boundary_matrix.csv")
    timing_rows = load_csv(TABLES / "table_apple_type_timings.csv")

    slot_formats = build_slot_formats(taxonomy_rows)
    rows: list[dict[str, str]] = []

    for family in REQUESTED_FAMILIES:
        for width in REQUESTED_WIDTHS:
            formats = slot_formats[(family, width)]
            taxonomy_matches = [
                row for row in taxonomy_rows
                if row["format"] in formats
            ]
            observed_widths = sorted({row["bit_width"] for row in taxonomy_matches})

            cpu_boundary = collect_boundary(boundary_rows, "cpu", formats)
            cpu_timing = collect_timing(timing_rows, "cpu", formats)
            metal_boundary = collect_boundary(boundary_rows, "metal", formats)
            metal_timing = collect_timing(
                timing_rows,
                "metal",
                formats,
                {"agx_type_surface", "agx_frontier_proxy"},
            )
            neural_boundary = collect_boundary(boundary_rows, "neural", formats)
            neural_timing = collect_timing(timing_rows, "neural", formats)

            states = {}
            cpu_status, cpu_notes = cpu_state(cpu_boundary, cpu_timing, formats)
            metal_status, metal_notes = metal_state(metal_boundary, metal_timing, formats)
            gpu_status, gpu_notes = gpu_state(formats, timing_rows)
            neural_status, neural_notes = neural_state(neural_boundary, neural_timing, formats)
            states["cpu"] = cpu_status
            states["metal"] = metal_status
            states["gpu"] = gpu_status
            states["neural"] = neural_status

            rows.append({
                "family": family,
                "requested_width": width,
                "format_candidates": joined(formats),
                "observed_payload_widths": joined(observed_widths),
                "cpu_status": cpu_status,
                "cpu_notes": cpu_notes,
                "metal_status": metal_status,
                "metal_notes": metal_notes,
                "gpu_status": gpu_status,
                "gpu_notes": gpu_notes,
                "neural_status": neural_status,
                "neural_notes": neural_notes,
                "next_action": next_action(family, width, states, formats),
            })
    return rows


def main() -> int:
    rows = build_rows()
    out_path = TABLES / "table_apple_full_scope_gap_matrix.csv"
    write_csv(out_path, rows, [
        "family",
        "requested_width",
        "format_candidates",
        "observed_payload_widths",
        "cpu_status",
        "cpu_notes",
        "metal_status",
        "metal_notes",
        "gpu_status",
        "gpu_notes",
        "neural_status",
        "neural_notes",
        "next_action",
    ])
    print(f"wrote {len(rows)} rows to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
