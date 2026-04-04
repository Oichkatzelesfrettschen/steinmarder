#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

exec "$ROOT_DIR/scripts/run_next42_metal_probes.sh" "$@"
