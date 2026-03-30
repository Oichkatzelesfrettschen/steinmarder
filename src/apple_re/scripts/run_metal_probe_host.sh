#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
HOST_OUT_DIR="${1:-$ROOT_DIR/results/metal_probe_host}"
HOST_BIN="$HOST_OUT_DIR/sm_apple_metal_probe_host"
METAL_OUT_DIR="${2:-$ROOT_DIR/results/metal_probe}"
METALLIB="$METAL_OUT_DIR/probe_simdgroup_reduce.metallib"
WIDTH="${WIDTH:-1024}"
ITERS="${ITERS:-100}"

if [ ! -x "$HOST_BIN" ]; then
    "$ROOT_DIR/scripts/build_metal_probe_host.sh" "$ROOT_DIR/host/metal_probe_host.m" "$HOST_OUT_DIR"
fi

if [ ! -f "$METALLIB" ]; then
    "$ROOT_DIR/scripts/compile_metal_probe.sh" "$ROOT_DIR/shaders/probe_simdgroup_reduce.metal" "$METAL_OUT_DIR"
fi

"$HOST_BIN" --metallib "$METALLIB" --width "$WIDTH" --iters "$ITERS" --csv
