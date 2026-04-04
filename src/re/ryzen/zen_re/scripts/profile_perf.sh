#!/bin/sh
# Single-target Linux perf profiling for Zen probes and translation targets.
# Usage: profile_perf.sh <binary_name> [iterations] [output_dir]

set -eu

TARGET_NAME="${1:?Usage: profile_perf.sh <binary_name> [iterations] [output_dir]}"
ITERATIONS="${2:-1000000}"
OUTDIR="${3:-results}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="${SCRIPT_DIR}/../../../build/bin/${TARGET_NAME}"

if [ ! -x "$BIN" ]; then
    echo "ERROR: target not found at $BIN" >&2
    echo "Build with: cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build" >&2
    exit 1
fi

command -v perf >/dev/null 2>&1 || { echo "ERROR: perf not in PATH" >&2; exit 1; }

mkdir -p "$OUTDIR"

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
STAT_FILE="${OUTDIR}/${TARGET_NAME}_${TIMESTAMP}_perf_stat.csv"
RECORD_FILE="${OUTDIR}/${TARGET_NAME}_${TIMESTAMP}.perf.data"
SCRIPT_FILE="${OUTDIR}/${TARGET_NAME}_${TIMESTAMP}_perf_script.txt"

echo "=== perf stat: ${TARGET_NAME} ==="
perf stat \
    -x , \
    -r 5 \
    -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,dTLB-loads,dTLB-load-misses \
    "$BIN" --iterations "$ITERATIONS" --csv \
    2> "$STAT_FILE"

echo "Saved: $STAT_FILE"

echo "=== perf record: ${TARGET_NAME} ==="
perf record \
    --call-graph dwarf \
    -F 999 \
    -o "$RECORD_FILE" \
    "$BIN" --iterations "$ITERATIONS" >/dev/null 2>&1

perf script -i "$RECORD_FILE" > "$SCRIPT_FILE" || true

echo "Saved: $RECORD_FILE"
echo "Saved: $SCRIPT_FILE"
