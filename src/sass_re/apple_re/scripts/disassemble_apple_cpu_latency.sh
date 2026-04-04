#!/bin/sh
set -eu

BIN="${1:-build/bin/sm_apple_cpu_latency}"
LLVM_OBJDUMP="${LLVM_OBJDUMP:-}"

if [ ! -f "$BIN" ]; then
    echo "missing binary: $BIN" >&2
    exit 1
fi

if [ -z "$LLVM_OBJDUMP" ]; then
    if command -v llvm-objdump >/dev/null 2>&1; then
        LLVM_OBJDUMP="$(command -v llvm-objdump)"
    elif [ -x /opt/homebrew/opt/llvm/bin/llvm-objdump ]; then
        LLVM_OBJDUMP=/opt/homebrew/opt/llvm/bin/llvm-objdump
    fi
fi

echo "== otool =="
otool -tvV "$BIN" | sed -n '1,220p'

echo
echo "== llvm-objdump =="
if [ -n "$LLVM_OBJDUMP" ]; then
    "$LLVM_OBJDUMP" -d --macho "$BIN" | sed -n '1,220p'
else
    echo "llvm-objdump not found"
fi
