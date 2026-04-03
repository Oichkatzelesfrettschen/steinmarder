#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_full_stack_$(date +%Y%m%d_%H%M%S)}"
SUDO_INVOKE="sudo -n"

mkdir -p "$OUT_DIR"

echo "timestamp_utc,step,status,artifact" > "$OUT_DIR/step_status.csv"

run_step() {
    name="$1"
    cmd="$2"
    artifact="$3"
    if eval "$cmd" > "$OUT_DIR/${name}.stdout.txt" 2> "$OUT_DIR/${name}.stderr.txt"; then
        printf '%s,%s,ok,%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$name" "$artifact" >> "$OUT_DIR/step_status.csv"
    else
        printf '%s,%s,fail,%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$name" "$artifact" >> "$OUT_DIR/step_status.csv"
    fi
}

run_step cpu_suite "\"$ROOT_DIR/scripts/run_next42_cpu_suite.sh\" \"$OUT_DIR/cpu\"" "$OUT_DIR/cpu"
run_step metal_suite "\"$ROOT_DIR/scripts/run_next42_metal_suite.sh\" \"$OUT_DIR/metal\"" "$OUT_DIR/metal"
run_step neural_raw "\"$ROOT_DIR/scripts/run_next42_neural_raw_probes.sh\" \"$OUT_DIR/neural\"" "$OUT_DIR/neural"
run_step opengl_suite "\"$ROOT_DIR/scripts/run_next42_opengl_probe_suite.sh\" \"$OUT_DIR/opengl\"" "$OUT_DIR/opengl"
run_step moltenvk_suite "\"$ROOT_DIR/scripts/run_next42_moltenvk_probe_suite.sh\" \"$OUT_DIR/moltenvk\"" "$OUT_DIR/moltenvk"

vm_stat > "$OUT_DIR/vm_stat.txt" 2>&1 || true
iostat -Id disk0 1 2 > "$OUT_DIR/iostat_disk0.txt" 2>&1 || true
diskutil list > "$OUT_DIR/diskutil_list.txt" 2>&1 || true
df -h > "$OUT_DIR/df_h.txt" 2>&1 || true
mount > "$OUT_DIR/mount.txt" 2>&1 || true
sysctl vm.swapusage > "$OUT_DIR/vm_swapusage.txt" 2>&1 || true
if command -v powermetrics >/dev/null 2>&1; then
    $SUDO_INVOKE powermetrics --samplers cpu_power,gpu_power -n 1 > "$OUT_DIR/powermetrics.txt" 2>&1 || true
fi

cat > "$OUT_DIR/full_stack_notes.md" <<'EOF'
# Next42 Full Stack Draft

- child suites: cpu, metal, neural, opengl, moltenvk
- sidecar telemetry: vm_stat, iostat, diskutil, df, mount, vm.swapusage, optional powermetrics
- purpose: keep CPU/GPU/RAM/IO/DISK context attached to the probe suites instead of treating the compute lanes in isolation
EOF

printf '%s\n' "$OUT_DIR"
