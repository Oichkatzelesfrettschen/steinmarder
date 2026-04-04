#!/bin/sh
# Fast smoke run for the Zen probe binaries.

set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)"
CPU="${1:-0}"

run() {
    echo "=== $1 ==="
    "${ROOT}/build/bin/$1" --iterations 20000 --cpu "$CPU" --size 1048576 --csv
}

run zen_probe_fma
run zen_probe_load_use
run zen_probe_permute
run zen_probe_gather
run zen_probe_branch
run zen_probe_prefetch
