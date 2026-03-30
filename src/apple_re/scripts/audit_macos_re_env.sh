#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
VENV_PYTHON="$ROOT_DIR/../../.venv/bin/python"

echo "== Apple RE environment audit =="
echo "root: $ROOT_DIR"
echo

echo "-- Xcode selection --"
xcode-select -p 2>/dev/null || true
if xcrun --toolchain Metal --find metal >/dev/null 2>&1; then
    xcrun --toolchain Metal --find metal
else
    echo "metal: not found"
fi
if xcrun --toolchain Metal --find metallib >/dev/null 2>&1; then
    xcrun --toolchain Metal --find metallib
else
    echo "metallib: not found"
fi
command -v xctrace >/dev/null 2>&1 && xctrace version || echo "xctrace: not found"
echo

echo "-- Homebrew core packages --"
for pkg in llvm lld libomp cmake ninja ccache graphviz openblas; do
    if brew list --versions "$pkg" >/dev/null 2>&1; then
        brew list --versions "$pkg"
    else
        echo "$pkg: missing"
    fi
done
echo

echo "-- Python ML stack --"
if [ -x "$VENV_PYTHON" ]; then
    "$VENV_PYTHON" "$ROOT_DIR/scripts/check_python_ml_stack.py"
elif command -v python3 >/dev/null 2>&1; then
    python3 "$ROOT_DIR/scripts/check_python_ml_stack.py"
else
    echo "python3: missing"
fi
