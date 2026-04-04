#!/bin/sh
# Thin AMD uProf wrapper.
# Usage: profile_uprof.sh <binary_name> [iterations] [output_dir]
#
# Set UPROF_ARGS to the current AMDuProfCLI collection arguments for your
# installed uProf version. Example:
#   UPROF_ARGS='collect --config tbp --output out' ./profile_uprof.sh zen_probe_fma

set -eu

TARGET_NAME="${1:?Usage: profile_uprof.sh <binary_name> [iterations] [output_dir]}"
ITERATIONS="${2:-1000000}"
OUTDIR="${3:-results}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="${SCRIPT_DIR}/../../../build/bin/${TARGET_NAME}"

if [ ! -x "$BIN" ]; then
    echo "ERROR: target not found at $BIN" >&2
    exit 1
fi

UPROF_BIN="${UPROF_BIN:-}"
if [ -z "$UPROF_BIN" ]; then
    if command -v AMDuProfCLI >/dev/null 2>&1; then
        UPROF_BIN="$(command -v AMDuProfCLI)"
    elif command -v AMDuProfPcm >/dev/null 2>&1; then
        UPROF_BIN="$(command -v AMDuProfPcm)"
    else
        echo "ERROR: AMD uProf CLI not found (checked AMDuProfCLI and AMDuProfPcm)" >&2
        exit 1
    fi
fi

if [ -z "${UPROF_ARGS:-}" ]; then
    echo "ERROR: set UPROF_ARGS to the current AMD uProf collection arguments for your installed version" >&2
    exit 1
fi

mkdir -p "$OUTDIR"

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
LOG_FILE="${OUTDIR}/${TARGET_NAME}_${TIMESTAMP}_uprof.log"

echo "=== AMD uProf: ${TARGET_NAME} ==="
echo "Using: $UPROF_BIN $UPROF_ARGS -- $BIN --iterations $ITERATIONS"

# shellcheck disable=SC2086
"$UPROF_BIN" $UPROF_ARGS -- "$BIN" --iterations "$ITERATIONS" >"$LOG_FILE" 2>&1

echo "Saved: $LOG_FILE"
