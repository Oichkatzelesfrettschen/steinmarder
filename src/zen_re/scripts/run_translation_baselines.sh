#!/bin/sh
# Run the first CPU translation targets through the perf wrapper.

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)"
OUTDIR="${1:-${ROOT}/build/zen_re_translation_baselines}"

mkdir -p "$OUTDIR"

run_target() {
    name="$1"
    bin="${ROOT}/build/bin/${name}"
    if [ ! -x "$bin" ]; then
        echo "${name},missing_binary" >> "$OUTDIR/status.csv"
        return
    fi
    echo "=== ${name} ==="
    perf stat \
        -x , \
        -r 3 \
        -e cycles,instructions,branches,branch-misses,cache-references,cache-misses \
        "$bin" \
        > "$OUTDIR/${name}.stdout" 2> "$OUTDIR/${name}_perf_stat.csv" || true
    echo "${name},ok" >> "$OUTDIR/status.csv"
}

: > "$OUTDIR/status.csv"

if [ -f "${ROOT}/models/nerf_hashgrid.bin" ] && [ -f "${ROOT}/models/occupancy_grid.bin" ]; then
    run_target nerf_simd_test
else
    echo "nerf_simd_test,missing_models" >> "$OUTDIR/status.csv"
fi

run_target test_render_smoke

python3 "$SCRIPT_DIR/summarize_perf_stat.py" "$OUTDIR" > "$OUTDIR/summary.csv"
echo "Saved: $OUTDIR/status.csv"
echo "Saved: $OUTDIR/summary.csv"
