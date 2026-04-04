#!/bin/sh
# Run all Zen probes through the perf wrapper and summarize the results.

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTDIR="${1:-${SCRIPT_DIR}/../../../build/zen_re_probe_runs}"
ITERATIONS="${2:-200000}"

mkdir -p "$OUTDIR"

for probe in \
    zen_probe_fma \
    zen_probe_load_use \
    zen_probe_permute \
    zen_probe_gather \
    zen_probe_branch \
    zen_probe_prefetch
do
    sh "$SCRIPT_DIR/profile_perf.sh" "$probe" "$ITERATIONS" "$OUTDIR/$probe"
done

python3 "$SCRIPT_DIR/summarize_perf_stat.py" "$OUTDIR" > "$OUTDIR/summary.csv"
echo "Saved: $OUTDIR/summary.csv"
