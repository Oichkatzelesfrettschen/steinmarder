#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
SRC="${1:-$ROOT_DIR/shaders/probe_simdgroup_reduce.metal}"
OUT_DIR="${2:-$ROOT_DIR/results/metal_probe}"
AIR="$OUT_DIR/$(basename "${SRC%.metal}").air"
LIB="$OUT_DIR/$(basename "${SRC%.metal}").metallib"
METAL_XCRUN="xcrun --toolchain Metal"

mkdir -p "$OUT_DIR"

if ! xcrun --toolchain Metal --find metal >/dev/null 2>&1; then
    echo "metal compiler not found. Select full Xcode with xcode-select first." >&2
    exit 1
fi

echo "Compiling $SRC"
$METAL_XCRUN metal -std=metal3.1 -c "$SRC" -o "$AIR"
$METAL_XCRUN metallib "$AIR" -o "$LIB"

echo "air: $AIR"
echo "metallib: $LIB"
