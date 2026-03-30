#!/bin/sh
set -eu

REPO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
VENV_DIR="$REPO_ROOT/.venv"
PYTHON_BIN="${PYTHON_BIN:-/opt/homebrew/bin/python3.12}"
REQ_FILE="$REPO_ROOT/src/apple_re/requirements-neural.txt"

if [ ! -x "$PYTHON_BIN" ]; then
    echo "missing python interpreter: $PYTHON_BIN" >&2
    exit 1
fi

echo "Using Python: $PYTHON_BIN"
rm -rf "$VENV_DIR"
"$PYTHON_BIN" -m venv "$VENV_DIR"
"$VENV_DIR/bin/python" -m pip install --upgrade pip setuptools wheel
"$VENV_DIR/bin/python" -m pip install -r "$REQ_FILE"

echo "Neural lane environment ready:"
echo "  $VENV_DIR/bin/python"
