#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_neural_suite_$(date +%Y%m%d_%H%M%S)}"
SUDO_MODE="${SUDO_MODE:-none}"
ITERS="${ITERS:-500000}"

exec "$ROOT_DIR/scripts/run_apple_tranche1.sh" \
  --phase F \
  --out "$OUT_DIR" \
  --sudo "$SUDO_MODE" \
  --iters "$ITERS"
