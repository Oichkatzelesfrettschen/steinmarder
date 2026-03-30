#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
SRC="${1:-$ROOT_DIR/host/metal_probe_host.m}"
OUT_DIR="${2:-$ROOT_DIR/results/metal_probe_host}"
OUT_BIN="$OUT_DIR/sm_apple_metal_probe_host"

mkdir -p "$OUT_DIR"

if ! command -v xcrun >/dev/null 2>&1; then
    echo "xcrun not found; install Xcode and select the toolchain first." >&2
    exit 1
fi

echo "Compiling host harness: $SRC"
xcrun --sdk macosx clang \
    -fobjc-arc \
    -framework Foundation \
    -framework Metal \
    "$SRC" \
    -o "$OUT_BIN"

echo "host_bin: $OUT_BIN"
