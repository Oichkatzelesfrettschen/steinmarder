#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
HOST="${HOST:-x130e}"
PHASE="${PHASE:-all}"
OUT_DIR="${1:-$ROOT_DIR/results/terascale_probe_tranche_$(date +%Y%m%d_%H%M%S)}"

REMOTE_MESA="${REMOTE_MESA:-/home/eirikr/workspaces/mesa/mesa-26-debug}"
REMOTE_BUILD="${REMOTE_BUILD:-$REMOTE_MESA/build-terakan-distcc}"
REMOTE_ICD="${REMOTE_ICD:-$REMOTE_BUILD/src/amd/terascale/vulkan/terascale_devenv_icd.x86_64.json}"

STATUS_CSV="$OUT_DIR/step_status.csv"
COMMANDS_LOG="$OUT_DIR/commands.log"
MANIFEST_JSON="$OUT_DIR/run_manifest.json"

mkdir -p "$OUT_DIR/steps"
echo "timestamp_utc,step,status,phase,name,artifact_dir" > "$STATUS_CSV"
: > "$COMMANDS_LOG"

STEP_NO=0

phase_enabled() {
    target="$1"
    [ "$PHASE" = "all" ] && return 0
    [ "$PHASE" = "$target" ]
}

mark_step() {
    step="$1"
    status="$2"
    phase="$3"
    name="$4"
    artifact="$5"
    printf '%s,%s,%s,%s,%s,%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$step" "$status" "$phase" "$name" "$artifact" >> "$STATUS_CSV"
}

run_step() {
    phase="$1"
    name="$2"
    cmd="$3"
    STEP_NO=$((STEP_NO + 1))
    step_label="$(printf '%02d_%s' "$STEP_NO" "$name")"
    step_dir="$OUT_DIR/steps/$step_label"
    mkdir -p "$step_dir"

    {
        echo "### STEP $STEP_NO ($phase): $name"
        echo "$cmd"
        echo
    } >> "$COMMANDS_LOG"

    if ! phase_enabled "$phase"; then
        mark_step "$STEP_NO" "skipped_phase" "$phase" "$name" "$step_dir"
        return 0
    fi

    printf '[%02d] (%s) %s\n' "$STEP_NO" "$phase" "$name"
    if eval "$cmd" > "$step_dir/stdout.log" 2> "$step_dir/stderr.log"; then
        mark_step "$STEP_NO" "ok" "$phase" "$name" "$step_dir"
    else
        mark_step "$STEP_NO" "fail" "$phase" "$name" "$step_dir"
    fi
}

run_step "inventory" "emit_local_probe_manifest" \
    "python3 \"$SCRIPT_DIR/emit_terascale_probe_manifest.py\" emit --format json > \"$OUT_DIR/local_probe_manifest.json\""

run_step "inventory" "count_probe_families" \
    "python3 \"$SCRIPT_DIR/emit_terascale_probe_manifest.py\" emit --format csv --header > \"$OUT_DIR/local_probe_manifest.csv\""

run_step "remote" "remote_machine_stamp" \
    "ssh \"$HOST\" 'id; echo; uname -a; echo; test -d \"$REMOTE_MESA\" && git -C \"$REMOTE_MESA\" rev-parse --short HEAD'"

run_step "opengl" "probe_tree_inventory" \
    "ssh \"$HOST\" 'find /home/eirikr/workspaces/research/TerakanMesa/re-toolkit/probes -maxdepth 2 -type f | sort'"

run_step "opengl" "gl_probe_tooling" \
    "ssh \"$HOST\" 'command -v glmark2; command -v piglit; command -v shader_runner || true; command -v glxinfo || true'"

run_step "vulkan" "vulkaninfo_summary" \
    "ssh \"$HOST\" 'export VK_ICD_FILENAMES=\"$REMOTE_ICD\"; vulkaninfo --summary'"

run_step "vulkan" "vkmark_list_scenes" \
    "ssh \"$HOST\" 'export VK_ICD_FILENAMES=\"$REMOTE_ICD\"; vkmark --list-scenes'"

run_step "vulkan" "vkmark_list_devices" \
    "ssh \"$HOST\" 'export VK_ICD_FILENAMES=\"$REMOTE_ICD\"; export VK_LOADER_DEBUG=error,warn,driver; vkmark --list-devices'"

run_step "opencl" "opencl_inventory" \
    "ssh \"$HOST\" 'command -v clinfo || true; command -v clpeak || true; clinfo 2>/dev/null | sed -n \"1,120p\"; echo; timeout 20 clpeak 2>/dev/null | sed -n \"1,120p\" || true'"

run_step "system" "trace_tool_inventory" \
    "ssh \"$HOST\" 'command -v perf; command -v trace-cmd || true; command -v bpftrace || true; command -v radeontop || true; command -v AMDuProfCLI || true'"

python3 - <<PY > "$MANIFEST_JSON"
import json
from pathlib import Path

out_dir = Path(r"$OUT_DIR")
payload = {
    "host": "$HOST",
    "phase": "$PHASE",
    "remote_mesa": "$REMOTE_MESA",
    "remote_build": "$REMOTE_BUILD",
    "remote_icd": "$REMOTE_ICD",
    "status_csv": str(out_dir / "step_status.csv"),
    "commands_log": str(out_dir / "commands.log"),
    "local_probe_manifest": str(out_dir / "local_probe_manifest.json"),
}
print(json.dumps(payload, indent=2))
PY

echo "TeraScale probe tranche complete: $OUT_DIR"
