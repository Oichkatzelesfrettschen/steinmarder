#!/bin/sh
set -eu

REPO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
VENV_DIR="$REPO_ROOT/.venv"
PYTHON_BIN="${PYTHON_BIN:-/opt/homebrew/bin/python3.12}"
REQ_FILE="$REPO_ROOT/src/apple_re/requirements-neural.txt"
STATE_FILE="$VENV_DIR/.apple_neural_bootstrap_state"
FORCE_REBUILD="${FORCE_NEURAL_VENV_REBUILD:-0}"

if [ ! -x "$PYTHON_BIN" ]; then
    echo "missing python interpreter: $PYTHON_BIN" >&2
    exit 1
fi

REQ_HASH="$("$PYTHON_BIN" - "$REQ_FILE" <<'PY'
import hashlib
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
print(hashlib.sha256(path.read_bytes()).hexdigest())
PY
)"
PY_VERSION="$("$PYTHON_BIN" - <<'PY'
import sys
print(f"{sys.version_info[0]}.{sys.version_info[1]}.{sys.version_info[2]}")
PY
)"
STATE_VALUE="python=$PYTHON_BIN
version=$PY_VERSION
requirements_sha256=$REQ_HASH"

echo "Using Python: $PYTHON_BIN"

if [ "$FORCE_REBUILD" != "1" ] && \
   [ -x "$VENV_DIR/bin/python" ] && \
   [ -x "$VENV_DIR/bin/pip" ] && \
   [ -f "$STATE_FILE" ] && \
   [ "$(cat "$STATE_FILE")" = "$STATE_VALUE" ]; then
    echo "Reusing existing neural lane environment at $VENV_DIR"
    echo "Neural lane environment ready:"
    echo "  $VENV_DIR/bin/python"
    exit 0
fi

rm -rf "$VENV_DIR"
"$PYTHON_BIN" -m venv "$VENV_DIR"
"$VENV_DIR/bin/python" -m pip install --upgrade pip setuptools wheel
"$VENV_DIR/bin/python" -m pip install -r "$REQ_FILE"
printf '%s\n' "$STATE_VALUE" > "$STATE_FILE"

echo "Neural lane environment ready:"
echo "  $VENV_DIR/bin/python"
