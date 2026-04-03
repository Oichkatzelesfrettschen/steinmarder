#!/usr/bin/env bash
# capture_gpu_counters.sh — Record and export Metal hardware GPU counters.
#
# Usage:
#   ./capture_gpu_counters.sh <output_dir> [--source-dir DIR] [--iters N]
#                             [--variant NAME] [--width N] [--time-limit 20s]
#
# What this captures:
#   metal-gpu-counter-intervals: hardware counter windows sampled by xctrace
#   metal-shader-profiler-intervals: per-shader timing with GPU-side profiler
#
# Output artifacts:
#   <output_dir>/gpu_counters_<variant>.trace   — raw xctrace bundle (excluded from git)
#   <output_dir>/gpu_counter_<variant>_*.xml    — exported counter table XML
#   <output_dir>/gpu_counter_summary.csv        — normalized raw counter rows
#   <output_dir>/gpu_hardware_counters.csv      — blessed percentile summary
#   <output_dir>/gpu_hardware_counters.md       — human-readable summary
#   <output_dir>/gpu_counter_analysis.md        — capture notes

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"

OUTPUT_DIR="${1:-/tmp/gpu_counters_$(date +%Y%m%d_%H%M%S)}"
SOURCE_DIR="$OUTPUT_DIR"
ITERS=5000000
VARIANT="all"
WIDTH=1024
TIME_LIMIT="20s"

shift || true
while [[ $# -gt 0 ]]; do
    case "$1" in
        --source-dir) SOURCE_DIR="$2"; shift 2 ;;
        --iters) ITERS="$2"; shift 2 ;;
        --variant) VARIANT="$2"; shift 2 ;;
        --width) WIDTH="$2"; shift 2 ;;
        --time-limit) TIME_LIMIT="$2"; shift 2 ;;
        *) echo "unknown arg: $1"; exit 1 ;;
    esac
done

mkdir -p "$OUTPUT_DIR"
echo "Output dir: $OUTPUT_DIR"
echo "Source dir: $SOURCE_DIR"
echo "Iters: $ITERS"
echo "Variant: $VARIANT"
echo "Width: $WIDTH"
echo "Time limit: $TIME_LIMIT"

METAL_HOST=""
for candidate in \
    "$SOURCE_DIR/metal_host/sm_apple_metal_probe_host" \
    "$REPO_ROOT/src/apple_re/results/"*"/metal_host/sm_apple_metal_probe_host" \
    "/tmp/sm_apple_metal_probe_host"; do
    if [[ -x "$candidate" ]]; then
        METAL_HOST="$candidate"
        break
    fi
done

if [[ -z "$METAL_HOST" ]]; then
    echo "Metal probe host binary not found. Build it first with:"
    echo "  src/apple_re/scripts/build_metal_probe_host.sh"
    echo "Then re-run this script."
    exit 1
fi

echo "Metal host: $METAL_HOST"

# Variants to capture. For each variant we record a separate trace so that
# per-variant counter values can be compared directly.
if [[ "$VARIANT" == "all" ]]; then
    VARIANTS=(baseline threadgroup_heavy threadgroup_minimal occupancy_heavy register_pressure)
else
    VARIANTS=("$VARIANT")
fi

COUNTER_SCHEMAS=(
    "metal-gpu-counter-intervals"
    "metal-shader-profiler-intervals"
)

for variant in "${VARIANTS[@]}"; do
    echo ""
    echo "=== Recording variant: $variant ==="
    trace_file="$OUTPUT_DIR/gpu_counters_${variant}.trace"
    metallib="$(find "$SOURCE_DIR/metal_probe_variants/$variant" -maxdepth 1 -type f -name '*.metallib' | head -n 1)"
    if [[ -z "$metallib" ]]; then
        echo "WARNING: no metallib found for $variant under $SOURCE_DIR/metal_probe_variants/$variant — skipping"
        continue
    fi
    stage_dir="$(mktemp -d "/tmp/steinmarder_gpu_counters_${variant}.XXXXXX")"
    cp "$METAL_HOST" "$stage_dir/sm_apple_metal_probe_host"
    cp "$metallib" "$stage_dir/$variant.metallib"
    printf '%s\n' "$stage_dir" > "$OUTPUT_DIR/gpu_counter_${variant}_stage_dir.txt"

    # Use the instrument form, which is the working capture route for Apple
    # hardware counters. The trace contains the counter windows; we summarize
    # them after export.
    set +e
    xcrun xctrace record \
        --instrument 'Metal GPU Counters' \
        --output "$trace_file" \
        --time-limit "$TIME_LIMIT" \
        --launch -- "$stage_dir/sm_apple_metal_probe_host" \
            --metallib "$stage_dir/$variant.metallib" \
            --kernel probe_simdgroup_reduce \
            --iters "$ITERS" \
            --width "$WIDTH" \
        2>&1
    record_status=$?
    set -e
    if [[ $record_status -ne 0 && ! -d "$trace_file" ]]; then
        echo "WARNING: xctrace record failed for $variant and no trace was saved — skipping"
        continue
    fi
    if [[ $record_status -ne 0 ]]; then
        echo "WARNING: xctrace record returned status $record_status for $variant, but trace was saved; continuing with export"
    fi

    echo "Recorded: $trace_file"

    # Export counter tables from the trace.
    for schema in "${COUNTER_SCHEMAS[@]}"; do
        out_xml="$OUTPUT_DIR/gpu_counter_${variant}_${schema}.xml"
        xcrun xctrace export \
            --input "$trace_file" \
            --xpath "//trace-toc[1]/run[1]/data[1]/table[@schema=\"${schema}\"]" \
            --output "$out_xml" \
            2>/dev/null || {
            echo "  schema $schema: export failed or no data"
            continue
        }
        row_count=$(python3 -c "
import xml.etree.ElementTree as ET
root = ET.parse('$out_xml').getroot()
rows = list(root.iter('row'))
print(len(rows))
" 2>/dev/null || echo "0")
        echo "  exported $schema: $row_count rows → $out_xml"
    done
done

echo ""
echo "=== Extracting counter values ==="
extract_ok=0
for attempt in 1 2 3; do
    if python3 "$SCRIPT_DIR/extract_gpu_counters.py" "$OUTPUT_DIR" --schema metal-gpu-counter-intervals; then
        echo "GPU counter analysis complete."
        extract_ok=1
        break
    fi
    echo "Extraction attempt $attempt failed; retrying after export flush..."
    sleep 2
done
if [[ $extract_ok -ne 1 ]]; then
    echo "Extraction ran (check gpu_counter_analysis.md for details)."
fi

echo ""
echo "=== Summary ==="
echo "Trace files (excluded from git): $OUTPUT_DIR/gpu_counters_*.trace"
echo "Counter XML exports:             $OUTPUT_DIR/gpu_counter_*.xml"
echo "Normalized raw CSV:              $OUTPUT_DIR/gpu_counter_summary.csv"
echo "Hardware summary CSV:            $OUTPUT_DIR/gpu_hardware_counters.csv"
echo "Hardware summary doc:            $OUTPUT_DIR/gpu_hardware_counters.md"
echo "Analysis doc:                    $OUTPUT_DIR/gpu_counter_analysis.md"
