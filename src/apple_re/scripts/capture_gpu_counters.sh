#!/usr/bin/env bash
# capture_gpu_counters.sh — Record and export Metal hardware GPU counters.
#
# Fills Critical Gap A from APPLE_TRACK_GAP_ANALYSIS.md:
# Prior blessed runs (r5–r7) used the 'Metal Application' xctrace template,
# which includes the metal-gpu-counter-intervals schema but NEVER populates
# it with actual data. Hardware counters require the 'Metal GPU Counters'
# or 'GPU' template.
#
# Usage:
#   ./capture_gpu_counters.sh <output_dir> [--iters N] [--variant NAME]
#
# What this captures:
#   metal-gpu-counter-intervals: ALU utilization (vertex/fragment/compute),
#     texture unit utilization, memory bandwidth, L1/L2 tile cache hit rates,
#     SIMD utilization, fragment overflow, tiling cycles, render cycles.
#   metal-shader-profiler-intervals: per-shader timing with GPU-side profiler.
#
# Output artifacts:
#   <output_dir>/gpu_counters_<variant>.trace   — raw xctrace bundle (excluded from git)
#   <output_dir>/gpu_counter_<variant>_*.xml    — exported counter table XML
#   <output_dir>/gpu_counter_summary.csv        — parsed counter values
#   <output_dir>/gpu_counter_analysis.md        — interpretation

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"
METAL_HOST_SRC="$REPO_ROOT/src/apple_re/metal_host"
SHADERS_DIR="$REPO_ROOT/src/apple_re/shaders"

OUTPUT_DIR="${1:-/tmp/gpu_counters_$(date +%Y%m%d_%H%M%S)}"
ITERS=200000
VARIANT="all"

shift || true
while [[ $# -gt 0 ]]; do
    case "$1" in
        --iters) ITERS="$2"; shift 2 ;;
        --variant) VARIANT="$2"; shift 2 ;;
        *) echo "unknown arg: $1"; exit 1 ;;
    esac
done

mkdir -p "$OUTPUT_DIR"
echo "Output dir: $OUTPUT_DIR"
echo "Iters: $ITERS"
echo "Variant: $VARIANT"

# Locate the Metal probe host binary.
METAL_HOST=""
for candidate in \
    "$OUTPUT_DIR/../../*/metal_host/sm_apple_metal_probe_host" \
    "$REPO_ROOT/src/apple_re/results/"*"/metal_host/sm_apple_metal_probe_host" \
    "/tmp/sm_apple_metal_probe_host"; do
    if [[ -x "$(echo $candidate | head -1)" ]]; then
        METAL_HOST="$(echo $candidate | head -1)"
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
    VARIANTS=(baseline occupancy_heavy register_pressure threadgroup_heavy threadgroup_minimal ffma_lat ffma_tput tgsm_lat)
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

    # Record with Metal GPU Counters template. This template enables the
    # hardware GPU counter instruments that populate metal-gpu-counter-intervals.
    xcrun xctrace record \
        --template 'Metal GPU Counters' \
        --output "$trace_file" \
        --env "METAL_PROBE_VARIANT=$variant" \
        --env "METAL_PROBE_ITERS=$ITERS" \
        -- "$METAL_HOST" \
            --variant "$variant" \
            --iters "$ITERS" \
            --width 1024 \
        2>&1 || {
        echo "WARNING: xctrace record failed for $variant — skipping"
        continue
    }

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
python3 "$SCRIPT_DIR/extract_gpu_counters.py" "$OUTPUT_DIR" && \
    echo "GPU counter analysis complete." || \
    echo "Extraction ran (check gpu_counter_analysis.md for details)."

echo ""
echo "=== Summary ==="
echo "Trace files (excluded from git): $OUTPUT_DIR/gpu_counters_*.trace"
echo "Counter XML exports:             $OUTPUT_DIR/gpu_counter_*.xml"
echo "Parsed CSV:                      $OUTPUT_DIR/gpu_counter_summary.csv"
echo "Analysis doc:                    $OUTPUT_DIR/gpu_counter_analysis.md"
