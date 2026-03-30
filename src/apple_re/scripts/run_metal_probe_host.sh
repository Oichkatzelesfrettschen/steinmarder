#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
HOST_OUT_DIR="${1:-$ROOT_DIR/results/metal_probe_host}"
HOST_BIN="$HOST_OUT_DIR/sm_apple_metal_probe_host"
METAL_OUT_DIR="${2:-$ROOT_DIR/results/metal_probe}"
METALLIB="${3:-$METAL_OUT_DIR/probe_simdgroup_reduce.metallib}"
WIDTH="${WIDTH:-1024}"
ITERS="${ITERS:-100}"
KERNEL="${KERNEL:-probe_simdgroup_reduce}"
FS_PROBE_EVERY="${FS_PROBE_EVERY:-0}"

if [ ! -x "$HOST_BIN" ]; then
    "$ROOT_DIR/scripts/build_metal_probe_host.sh" "$ROOT_DIR/host/metal_probe_host.m" "$HOST_OUT_DIR"
fi

if [ ! -f "$METALLIB" ]; then
    candidate="$(find "$METAL_OUT_DIR" -maxdepth 1 -type f -name '*.metallib' | head -n 1)"
    if [ -n "$candidate" ]; then
        METALLIB="$candidate"
    fi
fi

if [ ! -f "$METALLIB" ]; then
    "$ROOT_DIR/scripts/compile_metal_probe.sh" "$ROOT_DIR/shaders/probe_simdgroup_reduce.metal" "$METAL_OUT_DIR"
    METALLIB="$METAL_OUT_DIR/probe_simdgroup_reduce.metallib"
fi

if [ "$FS_PROBE_EVERY" -gt 0 ] 2>/dev/null; then
    "$HOST_BIN" --metallib "$METALLIB" --kernel "$KERNEL" --width "$WIDTH" --iters "$ITERS" --fs-probe-every "$FS_PROBE_EVERY" --csv
else
    "$HOST_BIN" --metallib "$METALLIB" --kernel "$KERNEL" --width "$WIDTH" --iters "$ITERS" --csv
fi
