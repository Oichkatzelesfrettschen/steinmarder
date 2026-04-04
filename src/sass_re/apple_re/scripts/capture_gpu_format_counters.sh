#!/usr/bin/env bash
# capture_gpu_format_counters.sh — Record per-format Metal GPU traces/counters
# from the typed Metal type-surface build.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

OUTPUT_DIR="${1:-$ROOT_DIR/results/typed_metal_format_counters_$(date +%Y%m%d_%H%M%S)}"
SOURCE_DIR="$OUTPUT_DIR"
WIDTH=2048
ITERS=400000
TRACE_ITERS=768
TIME_LIMIT="20s"
FORMATS="float32,float16,bfloat16,int8,int16,int32,int64,uint8,uint16,uint32,uint64,int4,uint4,fp4,tf32,fp8_e4m3,fp8_e5m2,mxfp8,mxint8,mxfp6,mxfp4"
HOST_BIN=""

shift || true
while [[ $# -gt 0 ]]; do
    case "$1" in
        --source-dir) SOURCE_DIR="$2"; shift 2 ;;
        --width) WIDTH="$2"; shift 2 ;;
        --iters) ITERS="$2"; shift 2 ;;
        --trace-iters) TRACE_ITERS="$2"; shift 2 ;;
        --time-limit) TIME_LIMIT="$2"; shift 2 ;;
        --formats) FORMATS="$2"; shift 2 ;;
        --host-bin) HOST_BIN="$2"; shift 2 ;;
        *) echo "unknown arg: $1" >&2; exit 1 ;;
    esac
done

case ",$FORMATS," in
    *,float32,*) ;;
    *) FORMATS="float32,$FORMATS" ;;
esac

mkdir -p "$OUTPUT_DIR"

if [[ -z "$HOST_BIN" ]]; then
    for candidate in \
        "$SOURCE_DIR/metal_host/sm_apple_metal_probe_host" \
        "$ROOT_DIR/results/metal_type_surface_host/sm_apple_metal_probe_host" \
        "$ROOT_DIR/results/metal_probe_host/sm_apple_metal_probe_host"; do
        if [[ -x "$candidate" ]]; then
            HOST_BIN="$candidate"
            break
        fi
    done
fi

if [[ -z "$HOST_BIN" ]]; then
    echo "Metal probe host binary not found." >&2
    exit 1
fi

TYPE_SURFACE_CSV="$SOURCE_DIR/metal_type_surface_matrix.csv"
TIMING_CSV="$SOURCE_DIR/metal_frontier_timing.csv"
BUILD_DIR="$SOURCE_DIR/metal_type_surface_build"

if [[ ! -f "$TYPE_SURFACE_CSV" || ! -f "$TIMING_CSV" || ! -d "$BUILD_DIR" ]]; then
    echo "Missing typed Metal artifacts under $SOURCE_DIR." >&2
    echo "Expected: metal_type_surface_matrix.csv, metal_frontier_timing.csv, metal_type_surface_build/" >&2
    exit 1
fi

FORMAT_LIST_FILE="$OUTPUT_DIR/format_probe_manifest.csv"
echo "format,bit_width,kernel_name,classification_mode,support_tier,metallib" > "$FORMAT_LIST_FILE"

IFS=',' read -r -a FORMAT_ARRAY <<< "$FORMATS"
for fmt in "${FORMAT_ARRAY[@]}"; do
    fmt="$(printf '%s' "$fmt" | xargs)"
    [[ -z "$fmt" ]] && continue
    row="$(python3 - <<PY
import csv
from pathlib import Path
fmt = ${fmt@Q}
rows = list(csv.DictReader(Path(${TYPE_SURFACE_CSV@Q}).open()))
for row in rows:
    if row["format"] == fmt and row["compile_status"] == "compiled":
        print(",".join([
            row["format"],
            row["bit_width"],
            row["kernel_name"],
            row["classification_mode"],
            row["support_tier"],
            str(Path(${BUILD_DIR@Q}) / f"{row['kernel_name']}.metallib"),
        ]))
        break
PY
)"
    [[ -n "$row" ]] && printf '%s\n' "$row" >> "$FORMAT_LIST_FILE"
done

COUNTER_SCHEMAS=("metal-gpu-counter-intervals" "metal-shader-profiler-intervals")

tail -n +2 "$FORMAT_LIST_FILE" | while IFS=',' read -r format bit_width kernel_name classification_mode support_tier metallib; do
    [[ -f "$metallib" ]] || continue
    echo "=== Format capture: $format ($kernel_name) ==="
    stage_dir="$(mktemp -d "/tmp/steinmarder_gpu_format_${format}.XXXXXX")"
    cp "$HOST_BIN" "$stage_dir/sm_apple_metal_probe_host"
    cp "$metallib" "$stage_dir/$format.metallib"
    printf '%s\n' "$stage_dir" > "$OUTPUT_DIR/gpu_${format}_stage_dir.txt"

    counter_trace="$OUTPUT_DIR/gpu_${format}.trace"
    set +e
    xcrun xctrace record \
        --instrument 'Metal GPU Counters' \
        --output "$counter_trace" \
        --time-limit "$TIME_LIMIT" \
        --launch -- "$stage_dir/sm_apple_metal_probe_host" \
            --metallib "$stage_dir/$format.metallib" \
            --kernel "$kernel_name" \
            --iters "$ITERS" \
            --width "$WIDTH" \
        >/dev/null 2>&1
    counter_status=$?
    set -o errexit
    if [[ $counter_status -ne 0 && ! -d "$counter_trace" ]]; then
        echo "WARNING: no Metal GPU Counters trace for $format" >&2
    fi

    for schema in "${COUNTER_SCHEMAS[@]}"; do
        out_xml="$OUTPUT_DIR/gpu_counter_${format}_${schema}.xml"
        if [[ -d "$counter_trace" ]]; then
            xcrun xctrace export \
                --input "$counter_trace" \
                --xpath "//trace-toc[1]/run[1]/data[1]/table[@schema=\"${schema}\"]" \
                --output "$out_xml" \
                >/dev/null 2>&1 || true
        fi
    done

    trace_bundle="$OUTPUT_DIR/gpu_fmt_${format}.trace"
    set +e
    xctrace record --template 'Metal System Trace' \
        --output "$trace_bundle" \
        --launch -- "$stage_dir/sm_apple_metal_probe_host" \
            --metallib "$stage_dir/$format.metallib" \
            --kernel "$kernel_name" \
            --iters "$TRACE_ITERS" \
            --width "$WIDTH" \
        >/dev/null 2>&1
    trace_status=$?
    set -e
    if [[ $trace_status -ne 0 && ! -d "$trace_bundle" ]]; then
        echo "WARNING: no Metal System Trace bundle for $format" >&2
    fi
done

python3 "$SCRIPT_DIR/extract_gpu_counters.py" "$OUTPUT_DIR" --schema metal-gpu-counter-intervals >/dev/null 2>&1 || true
python3 "$SCRIPT_DIR/analyze_gpu_counter_deltas.py" \
    "$OUTPUT_DIR/gpu_hardware_counters.csv" \
    --baseline float32 \
    --csv-out "$OUTPUT_DIR/gpu_hardware_counter_deltas.csv" \
    --md-out "$OUTPUT_DIR/gpu_hardware_counter_deltas.md" \
    >/dev/null 2>&1 || true
python3 "$SCRIPT_DIR/extract_xctrace_metrics.py" "$OUTPUT_DIR" >/dev/null 2>&1 || true
if [[ -f "$OUTPUT_DIR/xctrace_metric_row_counts.csv" && -f "$OUTPUT_DIR/xctrace_trace_health.csv" ]]; then
    python3 "$SCRIPT_DIR/analyze_xctrace_row_deltas.py" \
        "$OUTPUT_DIR/xctrace_metric_row_counts.csv" \
        "$OUTPUT_DIR/xctrace_trace_health.csv" \
        "$OUTPUT_DIR/xctrace_row_deltas.csv" \
        "$OUTPUT_DIR/xctrace_row_delta_summary.md" \
        "$OUTPUT_DIR/xctrace_row_density.csv" \
        "gpu_fmt_float32.trace" >/dev/null 2>&1 || true
fi

cat > "$OUTPUT_DIR/gpu_format_capture_notes.md" <<'EOF'
# Per-Format AGX Capture

- source lane: typed Metal type-surface build
- traces:
  - `gpu_<format>.trace` via `Metal GPU Counters`
  - `gpu_fmt_<format>.trace` via `Metal System Trace`
- exported counters:
  - `gpu_counter_<format>_metal-gpu-counter-intervals.xml`
  - `gpu_counter_<format>_metal-shader-profiler-intervals.xml`
- baseline for counter deltas: `float32`
EOF

printf '%s\n' "$OUTPUT_DIR"
