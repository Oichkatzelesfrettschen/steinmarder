#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
HOST="${HOST:-x130e}"
OUT_DIR="${1:-$ROOT_DIR/results/terascale_trace_stack_$(date +%Y%m%d_%H%M%S)}"
REMOTE_MESA="${REMOTE_MESA:-/home/eirikr/workspaces/mesa/mesa-26-debug}"
REMOTE_BUILD="${REMOTE_BUILD:-$REMOTE_MESA/build-terakan-distcc}"
REMOTE_ICD="${REMOTE_ICD:-$REMOTE_BUILD/src/amd/terascale/vulkan/terascale_devenv_icd.x86_64.json}"

mkdir -p "$OUT_DIR"
STATUS_CSV="$OUT_DIR/step_status.csv"
MANIFEST_JSON="$OUT_DIR/run_manifest.json"

echo "timestamp_utc,step,status,artifact" > "$STATUS_CSV"

step_no=0

run_remote() {
    name="$1"
    command="$2"
    step_no=$((step_no + 1))
    step_dir="$OUT_DIR/$(printf '%02d_%s' "$step_no" "$name")"
    mkdir -p "$step_dir"
    printf '[%02d] %s\n' "$step_no" "$name"
    if ssh "$HOST" "$command" > "$step_dir/stdout.log" 2> "$step_dir/stderr.log"; then
        status="ok"
    else
        status="fail"
    fi
    printf '%s,%s,%s,%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$name" "$status" "$step_dir" >> "$STATUS_CSV"
}

run_remote "machine_stamp" "id; echo; uname -a; echo; lscpu || true; echo; free -h || true"
run_remote "dri_inventory" "ls -l /dev/dri; echo; getfacl /dev/dri/renderD128 /dev/dri/card0 2>/dev/null || true"
run_remote "tool_inventory" "command -v perf || true; command -v trace-cmd || true; command -v radeontop || true; command -v AMDuProfCLI || true; command -v iostat || true; command -v vmstat || true"
run_remote "vulkan_summary" "export VK_ICD_FILENAMES='$REMOTE_ICD'; vulkaninfo --summary"
run_remote "perf_stat_capability" "perf stat -e cycles,instructions,cache-misses -a -- sleep 1"
run_remote "tracecmd_radeon_list" "trace-cmd list -e 2>/dev/null | rg 'radeon|drm|amdgpu|sched' || true"
run_remote "radeontop_probe" "timeout 3 radeontop -d - -l 1 2>/dev/null | sed -n '1,40p' || true"
run_remote "vm_io_probe" "vmstat 1 3 || true; echo; iostat -xz 1 2 || true"

python3 - <<PY > "$MANIFEST_JSON"
import json
payload = {
    "host": "$HOST",
    "remote_mesa": "$REMOTE_MESA",
    "remote_build": "$REMOTE_BUILD",
    "remote_icd": "$REMOTE_ICD",
    "status_csv": "$STATUS_CSV",
}
print(json.dumps(payload, indent=2))
PY

echo "TeraScale trace stack capture complete: $OUT_DIR"
