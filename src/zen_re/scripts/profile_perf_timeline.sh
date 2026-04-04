#!/bin/sh
# CPU-side timeline capture analogous to the CUDA nsys wrapper.
# Usage: profile_perf_timeline.sh <binary_name> [iterations] [output_dir]

set -eu

TARGET_NAME="${1:?Usage: profile_perf_timeline.sh <binary_name> [iterations] [output_dir]}"
ITERATIONS="${2:-1000000}"
OUTDIR="${3:-results}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="${SCRIPT_DIR}/../../../build/bin/${TARGET_NAME}"

if [ ! -x "$BIN" ]; then
    echo "ERROR: target not found at $BIN" >&2
    exit 1
fi

command -v perf >/dev/null 2>&1 || { echo "ERROR: perf not in PATH" >&2; exit 1; }

mkdir -p "$OUTDIR"

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
SCHED_FILE="${OUTDIR}/${TARGET_NAME}_${TIMESTAMP}_sched.data"
TIMECHART_FILE="${OUTDIR}/${TARGET_NAME}_${TIMESTAMP}_timechart.svg"

echo "=== perf sched record: ${TARGET_NAME} ==="
perf sched record -o "$SCHED_FILE" "$BIN" --iterations "$ITERATIONS" >/dev/null 2>&1
perf timechart -i "$SCHED_FILE" -o "$TIMECHART_FILE" >/dev/null 2>&1 || true

echo "Saved: $SCHED_FILE"
echo "Saved: $TIMECHART_FILE"
