#!/bin/sh
# Full Zen 4 probe suite — runs all 10 probes and collects CSV output.
#
# Usage: ./probe_zen4_full.sh [cpu] [output_dir]
#   cpu:        core to pin to (default: 0)
#   output_dir: directory for CSV results (default: results/zen4_$(date +%Y%m%d_%H%M%S))

set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/../../../../.." && pwd)"
BIN="${ROOT}/build/bin"
CPU="${1:-0}"
OUTDIR="${2:-${ROOT}/src/re/ryzen/zen_re/results/zen4_$(date +%Y%m%d_%H%M%S)}"

mkdir -p "$OUTDIR"

echo "=== Zen 4 Probe Suite ==="
echo "CPU: $CPU"
echo "Output: $OUTDIR"
echo "Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo ""

# Collect machine info
echo "=== Machine Info ===" > "$OUTDIR/machine_info.txt"
lscpu | grep -E "Model name|CPU family|Model:|Stepping|CPU.s.:|Core|Thread|L1|L2|L3" >> "$OUTDIR/machine_info.txt" 2>/dev/null
uname -a >> "$OUTDIR/machine_info.txt"
echo "" >> "$OUTDIR/machine_info.txt"

run_probe() {
    local name="$1"
    shift
    local bin="${BIN}/zen_${name}"
    if [ ! -x "$bin" ]; then
        echo "SKIP: $name (not built)"
        return
    fi
    echo "--- $name ---"
    "$bin" "$@" 2>&1 | tee "$OUTDIR/${name}.txt"
    echo ""
}

# Original Zen probes (Zen 3/4 portable)
run_probe probe_fma          --iterations 50000 --cpu "$CPU" --size 1048576 --csv
run_probe probe_load_use     --iterations 50000 --cpu "$CPU" --size 1048576 --csv
run_probe probe_permute      --iterations 50000 --cpu "$CPU" --size 1048576 --csv
run_probe probe_gather       --iterations 50000 --cpu "$CPU" --size 1048576 --csv
run_probe probe_branch       --iterations 50000 --cpu "$CPU" --size 1048576 --csv
run_probe probe_prefetch     --iterations 50000 --cpu "$CPU" --size 1048576 --csv

# Zen 4 cache hierarchy sweep
run_probe probe_cache_hierarchy --cpu "$CPU"

# AVX-512 probes (Zen 4 only)
run_probe probe_avx512_fma           --iterations 100000 --cpu "$CPU" --csv
run_probe probe_avx512_gather        --iterations 50000  --cpu "$CPU" --size 1048576 --csv
run_probe probe_avx512_vnni          --iterations 50000  --cpu "$CPU" --csv
run_probe probe_avx512_bf16          --iterations 100000 --cpu "$CPU" --csv
run_probe probe_avx512_bandwidth     --iterations 5000   --cpu "$CPU"
run_probe probe_avx512_throttle      --cpu "$CPU" --csv
run_probe probe_avx512_port_contention --iterations 50000 --cpu "$CPU" --size 262144 --csv
run_probe probe_avx512_shuffle        --iterations 50000  --cpu "$CPU" --csv
run_probe probe_avx512_convert        --iterations 50000  --cpu "$CPU" --csv
run_probe probe_avx512_mask            --iterations 50000  --cpu "$CPU" --csv

# Collect all CSV lines into a single summary
echo "=== Summary CSV ===" > "$OUTDIR/summary.csv"
grep "^csv," "$OUTDIR"/*.txt >> "$OUTDIR/summary.csv" 2>/dev/null || true

echo ""
echo "=== Complete ==="
echo "Results in: $OUTDIR"
echo "Summary: $OUTDIR/summary.csv"
cat "$OUTDIR/summary.csv"
