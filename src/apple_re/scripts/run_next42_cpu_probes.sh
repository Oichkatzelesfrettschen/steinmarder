#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$ROOT_DIR/../.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_cpu_probes_$(date +%Y%m%d_%H%M%S)}"
ITERS="${ITERS:-500000}"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
BINARY="${BINARY:-$BUILD_DIR/bin/sm_apple_cpu_latency}"

mkdir -p "$OUT_DIR/cpu_runs" "$OUT_DIR/disassembly" "$OUT_DIR/llvm_mca" "$OUT_DIR/diagnostics"

cat > "$OUT_DIR/cpu_probe_families.txt" <<'EOF'
integer_add_sub
floating_add_mul_fma
load_store_chain
shuffle_lane_cross
atomics
transcendentals
EOF

find "$ROOT_DIR/probes" -type f -name '*.c' | sort > "$OUT_DIR/cpu_probe_inventory.txt"

cat > "$OUT_DIR/compile_matrix.txt" <<'EOF'
O0:-O0
O2:-O2
O3:-O3
Ofast:-Ofast
EOF

mkdir -p "$OUT_DIR/cpu_bins"
for opt in O0:-O0 O2:-O2 O3:-O3 Ofast:-Ofast; do
    name="${opt%%:*}"
    flag="${opt##*:}"
    clang "$flag" -std=gnu11 -I"$ROOT_DIR/include" "$ROOT_DIR/probes/apple_cpu_latency.c" -lm -o "$OUT_DIR/cpu_bins/sm_apple_cpu_latency_${name}" 2>/dev/null || true
done

if [ ! -x "$BINARY" ] && [ -d "$BUILD_DIR" ]; then
    cmake --build "$BUILD_DIR" --target sm_apple_cpu_latency >/dev/null 2>&1 || true
fi

if [ ! -x "$BINARY" ]; then
    printf 'missing_cpu_binary=%s\n' "$BINARY" > "$OUT_DIR/diagnostics/build_status.txt"
    cat > "$OUT_DIR/cpu_notes.md" <<'EOF'
# Next42 CPU Probe Draft

- binary: missing
- expected artifacts: cpu_probe_families.txt, cpu_probe_inventory.txt, compile_matrix.txt, cpu_runs/*.csv, disassembly/*.txt, llvm_mca/*.txt
EOF
    printf '%s\n' "$OUT_DIR"
    exit 0
fi

for probe in add_dep_u64 fadd_dep_f64 fmadd_dep_f64 load_store_chain_u64 shuffle_lane_cross_u64 atomic_add_relaxed_u64 transcendental_sin_cos_f64; do
    "$BINARY" --iters "$ITERS" --probe "$probe" --csv > "$OUT_DIR/cpu_runs/$probe.csv" 2>&1 || true
done

"$BINARY" --iters "$ITERS" --csv > "$OUT_DIR/cpu_runs/all_probes.csv" 2>&1 || true

cat > "$OUT_DIR/diagnostics/asan_run.csv" <<'EOF'
probe,status,notes
draft,not_run,this draft script leaves sanitizer validation to the full tranche runner
EOF

cat > "$OUT_DIR/diagnostics/leaks.txt" <<'EOF'
CPU lane draft does not target a persistent process yet.
EOF

cat > "$OUT_DIR/diagnostics/vmmap.txt" <<'EOF'
CPU lane draft does not target a persistent process yet.
EOF

for bin in "$OUT_DIR"/cpu_bins/sm_apple_cpu_latency_*; do
    [ -e "$bin" ] || continue
    base="$(basename "$bin")"
    otool -tvV "$bin" > "$OUT_DIR/disassembly/${base}.otool.txt" 2>/dev/null || true
    if command -v llvm-objdump >/dev/null 2>&1; then
        llvm-objdump -d --macho "$bin" > "$OUT_DIR/disassembly/${base}.objdump.txt" 2>/dev/null || true
    elif [ -x /opt/homebrew/opt/llvm/bin/llvm-objdump ]; then
        /opt/homebrew/opt/llvm/bin/llvm-objdump -d --macho "$bin" > "$OUT_DIR/disassembly/${base}.objdump.txt" 2>/dev/null || true
    fi
done

for opt in O0:-O0 O2:-O2 O3:-O3 Ofast:-Ofast; do
    name="${opt%%:*}"
    flag="${opt##*:}"
    asm="$OUT_DIR/llvm_mca/apple_cpu_latency_${name}.s"
    report="$OUT_DIR/llvm_mca/apple_cpu_latency_${name}.mca.txt"
    clang "$flag" -std=gnu11 -S -I"$ROOT_DIR/include" "$ROOT_DIR/probes/apple_cpu_latency.c" -o "$asm" 2>/dev/null || true
    if [ -f "$asm" ]; then
        if command -v llvm-mca >/dev/null 2>&1; then
            llvm-mca "$asm" > "$report" 2>&1 || true
        elif [ -x /opt/homebrew/opt/llvm/bin/llvm-mca ]; then
            /opt/homebrew/opt/llvm/bin/llvm-mca "$asm" > "$report" 2>&1 || true
        fi
    fi
done

if command -v hyperfine >/dev/null 2>&1; then
    hyperfine --runs 5 \
        --export-csv "$OUT_DIR/cpu_runs/hyperfine.csv" \
        "$BINARY --iters $ITERS --probe add_dep_u64 --csv >/dev/null" \
        "$BINARY --iters $ITERS --probe load_store_chain_u64 --csv >/dev/null" \
        "$BINARY --iters $ITERS --probe atomic_add_relaxed_u64 --csv >/dev/null" \
        >/dev/null 2>&1 || true
fi

if [ ! -f "$OUT_DIR/cpu_runs/hyperfine.csv" ]; then
    cat > "$OUT_DIR/cpu_runs/hyperfine.csv" <<'EOF'
command,mean,stddev,median,user,system,min,max
draft,not_run,not_run,not_run,not_run,not_run,not_run,not_run
EOF
fi

cat > "$OUT_DIR/cpu_notes.md" <<'EOF'
# Next42 CPU Probe Draft

- families: integer add/sub, floating add/mul/FMA, load/store, shuffle, atomics, transcendentals
- binary: sm_apple_cpu_latency
- expected artifacts: cpu_probe_families.txt, cpu_probe_inventory.txt, compile_matrix.txt, cpu_bins/*, cpu_runs/*.csv, disassembly/*.txt, llvm_mca/*.txt
EOF

printf '%s\n' "$OUT_DIR"
