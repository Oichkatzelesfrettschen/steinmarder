#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$ROOT_DIR/../.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_cpu_cache_probes_$(date +%Y%m%d_%H%M%S)}"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
BINARY="${BINARY:-$BUILD_DIR/bin/sm_apple_cpu_cache_pressure}"
SUDO_INVOKE="${SUDO_INVOKE:-sudo -n}"

mkdir -p "$OUT_DIR/cache_bins" "$OUT_DIR/cache_runs" "$OUT_DIR/cache_diagnostics"

cat > "$OUT_DIR/cache_probe_families.txt" <<'EOF'
stream_l1_hot
stream_l2_boundary
pointer_chase_llc
reuse_stride_hotset
EOF

cat > "$OUT_DIR/cache_probe_inventory.txt" <<'EOF'
probes/apple_cpu_cache_pressure.c
scripts/run_next42_cpu_cache_probes.sh
EOF

cat > "$OUT_DIR/cache_family_matrix.csv" <<'EOF'
label,family
stream_l1_hot,stream
stream_l2_boundary,stream
pointer_chase_llc,pointer
reuse_stride_hotset,reuse
EOF

if [ ! -x "$BINARY" ] && [ -d "$BUILD_DIR" ]; then
    cmake --build "$BUILD_DIR" --target sm_apple_cpu_cache_pressure >/dev/null 2>&1 || true
fi

if [ ! -x "$BINARY" ]; then
    clang -O3 -std=gnu11 -I"$ROOT_DIR/include" "$ROOT_DIR/probes/apple_cpu_cache_pressure.c" -o "$OUT_DIR/cache_bins/sm_apple_cpu_cache_pressure" >/dev/null 2>&1 || true
    BINARY="$OUT_DIR/cache_bins/sm_apple_cpu_cache_pressure"
fi

if [ ! -x "$BINARY" ]; then
    cat > "$OUT_DIR/cache_interpretation.md" <<'EOF'
# Cache Pressure Interpretation

- binary: missing
- expected artifacts: cache_probe_families.txt, cache_pressure.csv, cache_trace_health.csv, cache_interpretation.md
EOF
    printf '%s\n' "$OUT_DIR"
    exit 0
fi

python3 - "$OUT_DIR" "$BINARY" <<'PY'
from __future__ import annotations

import csv
import math
import subprocess
import sys
from pathlib import Path

out_dir = Path(sys.argv[1])
binary = sys.argv[2]

touch_budget_bytes = int(out_dir.joinpath("cache_touch_budget.txt").read_text(encoding="utf-8").strip()) if out_dir.joinpath("cache_touch_budget.txt").exists() else 64 * 1024 * 1024
min_passes = 8
max_passes = 256

families = [
    ("stream_l1_hot", "stream", [16, 24, 32, 48, 64, 96, 128, 192, 256], [64, 128, 256, 512]),
    ("stream_l2_boundary", "stream", [192, 256, 384, 512, 768, 1024, 1536, 2048], [64, 128, 256, 512]),
    ("pointer_chase_llc", "pointer", [128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192], [64, 128, 256]),
    ("reuse_stride_hotset", "reuse", [64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072], [64, 128, 256]),
]

pressure_csv = out_dir / "cache_pressure.csv"
with pressure_csv.open("w", newline="", encoding="utf-8") as fh:
    writer = csv.DictWriter(
        fh,
        fieldnames=[
            "label",
            "family",
            "size_kib",
            "size_bytes",
            "stride_bytes",
            "passes",
            "accesses",
            "elapsed_ns",
            "ns_per_access",
            "accesses_per_sec",
            "approx_gaccesses_per_sec",
        ],
    )
    writer.writeheader()

    rows: list[dict[str, str]] = []
    for label, family, sizes_kib, strides in families:
        for size_kib in sizes_kib:
            size_bytes = size_kib * 1024
            passes = max(min_passes, min(max_passes, math.ceil(touch_budget_bytes / max(size_bytes, 1))))
            for stride_bytes in strides:
                proc = subprocess.run(
                    [
                        binary,
                        "--family",
                        family,
                        "--size-kib",
                        str(size_kib),
                        "--stride-bytes",
                        str(stride_bytes),
                        "--passes",
                        str(passes),
                        "--csv",
                    ],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                if proc.returncode != 0 or not proc.stdout.strip():
                    continue
                parsed = list(csv.DictReader(proc.stdout.splitlines()))
                if not parsed:
                    continue
                row = parsed[-1]
                out_row = {
                    "label": label,
                    "family": family,
                    "size_kib": row.get("size_kib", str(size_kib)),
                    "size_bytes": row.get("size_bytes", str(size_bytes)),
                    "stride_bytes": row.get("stride_bytes", str(stride_bytes)),
                    "passes": row.get("passes", str(passes)),
                    "accesses": row.get("accesses", "0"),
                    "elapsed_ns": row.get("elapsed_ns", "0"),
                    "ns_per_access": row.get("ns_per_access", "0"),
                    "accesses_per_sec": row.get("accesses_per_sec", "0"),
                    "approx_gaccesses_per_sec": row.get("approx_gaccesses_per_sec", "0"),
                }
                writer.writerow(out_row)
                rows.append(out_row)

    if not rows:
        writer.writerow(
            {
                "label": "draft",
                "family": "missing",
                "size_kib": "0",
                "size_bytes": "0",
                "stride_bytes": "0",
                "passes": "0",
                "accesses": "0",
                "elapsed_ns": "0",
                "ns_per_access": "0",
                "accesses_per_sec": "0",
                "approx_gaccesses_per_sec": "0",
            }
        )

trace_path = out_dir / "gpu_cache_time_profiler.trace"
trace_path.unlink(missing_ok=True)

representative = (
    "pointer",
    4096,
    64,
    max(min_passes, min(max_passes, math.ceil(touch_budget_bytes / (4096 * 1024)))),
)
subprocess.run(
    [
        binary,
        "--family",
        representative[0],
        "--size-kib",
        str(representative[1]),
        "--stride-bytes",
        str(representative[2]),
        "--passes",
        str(representative[3]),
        "--csv",
    ],
    capture_output=True,
    text=True,
    check=False,
)
PY

if command -v xctrace >/dev/null 2>&1; then
    xctrace record --template 'Time Profiler' --output "$OUT_DIR/gpu_cache_time_profiler.trace" --launch -- \
        "$BINARY" --family pointer --size-kib 4096 --stride-bytes 64 --passes 32 --csv >/dev/null 2>&1 || true
fi

if command -v powermetrics >/dev/null 2>&1; then
    if $SUDO_INVOKE true >/dev/null 2>&1; then
        $SUDO_INVOKE powermetrics --samplers cpu_power -n 1 > "$OUT_DIR/powermetrics_cache.txt" 2>&1 || true
    else
        echo powermetrics_sudo_unavailable > "$OUT_DIR/powermetrics_cache.txt"
    fi
else
    echo powermetrics_missing > "$OUT_DIR/powermetrics_cache.txt"
fi

if command -v xctrace >/dev/null 2>&1; then
    python3 "$ROOT_DIR/scripts/extract_xctrace_metrics.py" "$OUT_DIR" > "$OUT_DIR/cache_xctrace_extract_summary.txt" 2>&1 || true
fi

python3 - "$OUT_DIR" <<'PY'
from __future__ import annotations

import csv
from pathlib import Path
import sys

out_dir = Path(sys.argv[1])
trace_health = out_dir / "xctrace_trace_health.csv"
cache_trace_health = out_dir / "cache_trace_health.csv"

if trace_health.exists():
    rows = list(csv.DictReader(trace_health.open(encoding="utf-8")))
    if rows:
        rows = [row for row in rows if row.get("trace") == "gpu_cache_time_profiler.trace"] or rows
    with cache_trace_health.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=list(rows[0].keys()) if rows else ["trace", "template_name", "duration_s", "end_reason", "target_process_name", "target_pid", "target_exit_status", "target_termination_reason", "schema_count"])
        writer.writeheader()
        writer.writerows(rows)
else:
    with cache_trace_health.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(
            fh,
            fieldnames=["trace", "template_name", "duration_s", "end_reason", "target_process_name", "target_pid", "target_exit_status", "target_termination_reason", "schema_count"],
        )
        writer.writeheader()
        writer.writerow(
            {
                "trace": "gpu_cache_time_profiler.trace",
                "template_name": "missing",
                "duration_s": "0",
                "end_reason": "missing",
                "target_process_name": "missing",
                "target_pid": "0",
                "target_exit_status": "0",
                "target_termination_reason": "missing",
                "schema_count": "0",
            }
        )
PY

python3 - "$OUT_DIR" <<'PY' > "$OUT_DIR/cache_interpretation.md"
from __future__ import annotations

import csv
import math
from pathlib import Path
import sys

out_dir = Path(sys.argv[1])
pressure = out_dir / "cache_pressure.csv"
print("# Cache Pressure Interpretation\n")

if not pressure.exists():
    print("- cache_pressure.csv missing")
    raise SystemExit(0)

rows = list(csv.DictReader(pressure.open(encoding="utf-8")))
if not rows:
    print("- no rows captured")
    raise SystemExit(0)

by_label: dict[str, list[dict[str, str]]] = {}
for row in rows:
    by_label.setdefault(row["label"], []).append(row)

for label, label_rows in sorted(by_label.items()):
    values = []
    for row in label_rows:
        try:
            values.append((float(row["size_kib"]), float(row["stride_bytes"]), float(row["ns_per_access"])))
        except (TypeError, ValueError):
            pass
    if not values:
        continue
    min_ns = min(v[2] for v in values)
    knee = None
    for size_kib, stride_bytes, ns_per_access in sorted(values, key=lambda item: (item[0], item[1])):
        if ns_per_access >= min_ns * 1.15:
            knee = (size_kib, stride_bytes, ns_per_access)
            break
    print(f"- {label}: min_ns_per_access={min_ns:.6f}")
    if knee:
        print(f"  observed_knee: size_kib={knee[0]:.0f}, stride_bytes={knee[1]:.0f}, ns_per_access={knee[2]:.6f}")
    else:
        print("  observed_knee: not crossed in this sweep")
PY

printf '%s\n' "$OUT_DIR"
