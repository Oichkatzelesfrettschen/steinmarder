#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$ROOT_DIR/../.." && pwd)"

PHASES="all"
OUT_DIR="$ROOT_DIR/results/tranche1_$(date +%Y%m%d_%H%M%S)"
SUDO_MODE="keepalive"
ITERS="${ITERS:-500000}"
KEEPALIVE_PID=""
STEP_NO=0
TOTAL_STEPS=42
SUDO_INVOKE="sudo -A"

usage() {
    cat <<EOF
usage: $0 [--phase A|B|C|D|E|F|G|all|A,B,...] [--out DIR] [--sudo keepalive|cache|none] [--iters N]
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --phase)
            PHASES="${2:-}"
            shift 2
            ;;
        --out)
            OUT_DIR="${2:-}"
            shift 2
            ;;
        --sudo)
            SUDO_MODE="${2:-}"
            shift 2
            ;;
        --iters)
            ITERS="${2:-}"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [ "$SUDO_MODE" = "keepalive" ]; then
    SUDO_INVOKE="sudo -A"
elif [ "$SUDO_MODE" = "cache" ]; then
    SUDO_INVOKE="sudo -n"
else
    SUDO_INVOKE="sudo -n"
fi

mkdir -p "$OUT_DIR/steps"

COMMANDS_LOG="$OUT_DIR/commands.log"
STATUS_CSV="$OUT_DIR/step_status.csv"
VERSIONS_JSON="$OUT_DIR/versions.json"
MANIFEST_JSON="$OUT_DIR/run_manifest.json"
SUMMARY_MD="$OUT_DIR/summary.md"
KEEPALIVE_PID_FILE="$OUT_DIR/sudo_keepalive.pid"

echo "timestamp_utc,step,status,phase,name,artifact_dir" > "$STATUS_CSV"
: > "$COMMANDS_LOG"

phase_enabled() {
    target="$1"
    if [ "$PHASES" = "all" ]; then
        return 0
    fi
    old_ifs="$IFS"
    IFS=","
    for token in $PHASES; do
        if [ "$token" = "$target" ]; then
            IFS="$old_ifs"
            return 0
        fi
    done
    IFS="$old_ifs"
    return 1
}

choose_sudo_prime_cmd() {
    if [ -n "${SUDO_ASKPASS:-}" ] && [ -x "${SUDO_ASKPASS}" ]; then
        echo "sudo -A -v"
        return 0
    fi
    echo "sudo -v"
}

sudo_prime_once() {
    if [ "$SUDO_MODE" = "none" ]; then
        echo "sudo_disabled"
        return 0
    fi
    prime_cmd="$(choose_sudo_prime_cmd)"
    echo "sudo_prime_cmd=$prime_cmd"
    eval "$prime_cmd"
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
    step_label="$(printf "%02d_%s" "$STEP_NO" "$name")"
    step_dir="$OUT_DIR/steps/$step_label"
    mkdir -p "$step_dir"

    if ! phase_enabled "$phase"; then
        mark_step "$STEP_NO" "skipped_phase" "$phase" "$name" "$step_dir"
        return 0
    fi

    printf '[%02d/%02d] (%s) %s\n' "$STEP_NO" "$TOTAL_STEPS" "$phase" "$name"
    {
        echo "### STEP $STEP_NO ($phase): $name"
        echo "$cmd"
    } >> "$COMMANDS_LOG"

    if eval "$cmd" > "$step_dir/stdout.log" 2> "$step_dir/stderr.log"; then
        mark_step "$STEP_NO" "ok" "$phase" "$name" "$step_dir"
    else
        mark_step "$STEP_NO" "fail" "$phase" "$name" "$step_dir"
    fi
}

start_sudo_keepalive() {
    if [ "$SUDO_MODE" = "none" ]; then
        return 0
    fi
    sudo_prime_once
    (
        while true; do
            sudo -n true >/dev/null 2>&1 || exit 0
            sleep 50
        done
    ) &
    KEEPALIVE_PID="$!"
    printf '%s\n' "$KEEPALIVE_PID" > "$KEEPALIVE_PID_FILE"
}

stop_sudo_keepalive() {
    if [ -n "$KEEPALIVE_PID" ]; then
        kill "$KEEPALIVE_PID" >/dev/null 2>&1 || true
        KEEPALIVE_PID=""
    fi
    if [ "$SUDO_MODE" = "keepalive" ]; then
        sudo -K >/dev/null 2>&1 || true
    fi
}

cleanup() {
    stop_sudo_keepalive
}
trap cleanup EXIT INT TERM

run_step "A" "sudo_prime" "sudo_prime_once"
run_step "A" "sudo_keepalive_start" "if [ \"$SUDO_MODE\" = \"keepalive\" ] || [ \"$SUDO_MODE\" = \"cache\" ]; then echo keepalive_requested; else echo keepalive_disabled; fi"
if phase_enabled "A" && { [ "$SUDO_MODE" = "keepalive" ] || [ "$SUDO_MODE" = "cache" ]; }; then
    start_sudo_keepalive
fi

run_step "A" "machine_stamp" "{
  echo \"uname=$(uname -a)\";
  echo \"xcode_select=$(xcode-select -p 2>/dev/null || true)\";
  echo \"xcodebuild=$(xcodebuild -version 2>/dev/null | tr '\n' ';')\";
  echo \"metal=$(xcrun --toolchain Metal --find metal 2>/dev/null || true)\";
  echo \"metallib=$(xcrun --toolchain Metal --find metallib 2>/dev/null || true)\";
} > \"$VERSIONS_JSON\""
run_step "A" "tool_matrix" "for t in xcodebuild xcrun xctrace dtrace dtruss sample spindump powermetrics leaks vmmap fs_usage lldb ncu nsys nvcc uprof valgrind gdb hyperfine; do command -v \"\$t\" >/dev/null 2>&1 && echo \"\$t,found,\$(command -v \"\$t\")\" || echo \"\$t,missing,\"; done > \"$OUT_DIR/tool_matrix.csv\""
run_step "A" "package_inventory" "{
  command -v brew >/dev/null 2>&1 && brew list --versions > \"$OUT_DIR/brew_versions.txt\" || true;
  command -v port >/dev/null 2>&1 && port installed > \"$OUT_DIR/port_installed.txt\" || true;
}"
run_step "A" "capabilities_json" "python3 \"$SCRIPT_DIR/apple_capability_report.py\" > \"$OUT_DIR/capabilities.json\""

run_step "B" "cpu_family_matrix" "cat > \"$OUT_DIR/cpu_probe_families.txt\" <<'EOF'
integer_add_sub
floating_add_mul_fma
load_store_chain
shuffle_lane_cross
atomics
transcendentals
EOF"
run_step "B" "cpu_probe_inventory" "find \"$ROOT_DIR/probes\" -type f -name '*.c' | sort > \"$OUT_DIR/cpu_probe_inventory.txt\""
run_step "B" "compile_matrix_define" "cat > \"$OUT_DIR/compile_matrix.txt\" <<'EOF'
O0:-O0
O2:-O2
O3:-O3
Ofast:-Ofast
EOF"
run_step "B" "build_cpu_matrix" "mkdir -p \"$OUT_DIR/cpu_bins\" && for row in O0:-O0 O2:-O2 O3:-O3 Ofast:-Ofast; do name=\"\${row%%:*}\"; flag=\"\${row##*:}\"; clang \"\$flag\" -std=gnu11 -I\"$ROOT_DIR/include\" \"$ROOT_DIR/probes/apple_cpu_latency.c\" -o \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_\${name}\"; done"
run_step "B" "disassemble_cpu_matrix" "mkdir -p \"$OUT_DIR/disassembly\" && for bin in \"$OUT_DIR\"/cpu_bins/sm_apple_cpu_latency_*; do base=\$(basename \"\$bin\"); otool -tvV \"\$bin\" > \"$OUT_DIR/disassembly/\${base}.otool.txt\" 2>/dev/null || true; if command -v llvm-objdump >/dev/null 2>&1; then llvm-objdump -d --macho \"\$bin\" > \"$OUT_DIR/disassembly/\${base}.objdump.txt\" 2>/dev/null || true; elif [ -x /opt/homebrew/opt/llvm/bin/llvm-objdump ]; then /opt/homebrew/opt/llvm/bin/llvm-objdump -d --macho \"\$bin\" > \"$OUT_DIR/disassembly/\${base}.objdump.txt\" 2>/dev/null || true; fi; done"
run_step "B" "llvm_mca_cpu_matrix" "mkdir -p \"$OUT_DIR/llvm_mca\" && for row in O0:-O0 O2:-O2 O3:-O3; do name=\"\${row%%:*}\"; flag=\"\${row##*:}\"; clang \"\$flag\" -std=gnu11 -S -I\"$ROOT_DIR/include\" \"$ROOT_DIR/probes/apple_cpu_latency.c\" -o \"$OUT_DIR/llvm_mca/apple_cpu_latency_\${name}.s\" 2>/dev/null || true; if command -v llvm-mca >/dev/null 2>&1; then llvm-mca \"$OUT_DIR/llvm_mca/apple_cpu_latency_\${name}.s\" > \"$OUT_DIR/llvm_mca/apple_cpu_latency_\${name}.mca.txt\" 2>&1 || true; elif [ -x /opt/homebrew/opt/llvm/bin/llvm-mca ]; then /opt/homebrew/opt/llvm/bin/llvm-mca \"$OUT_DIR/llvm_mca/apple_cpu_latency_\${name}.s\" > \"$OUT_DIR/llvm_mca/apple_cpu_latency_\${name}.mca.txt\" 2>&1 || true; fi; done"

run_step "C" "cpu_baseline_timing" "mkdir -p \"$OUT_DIR/cpu_runs\" && for bin in \"$OUT_DIR\"/cpu_bins/sm_apple_cpu_latency_*; do \"\$bin\" --iters \"$ITERS\" --csv > \"$OUT_DIR/cpu_runs/\$(basename \"\$bin\").csv\" 2>&1 || true; done"
run_step "C" "hyperfine_cpu_timing" "if command -v hyperfine >/dev/null 2>&1; then hyperfine --runs 5 \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3 --iters $ITERS --csv\" --export-csv \"$OUT_DIR/cpu_runs/hyperfine.csv\"; else echo hyperfine_missing; fi"
run_step "C" "xctrace_cpu_profile" "if command -v xctrace >/dev/null 2>&1; then xctrace record --template 'Time Profiler' --output \"$OUT_DIR/cpu_time_profiler.trace\" --launch -- \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" --iters \"$ITERS\" >/dev/null 2>&1 || true; else echo xctrace_missing; fi"
run_step "C" "dtrace_dtruss_cpu" "if command -v dtruss >/dev/null 2>&1; then $SUDO_INVOKE dtruss -f \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" --iters 10000 > \"$OUT_DIR/cpu_runs/dtruss.log\" 2>&1 || true; else echo dtruss_missing; fi"
run_step "C" "powermetrics_cpu" "if command -v powermetrics >/dev/null 2>&1; then $SUDO_INVOKE powermetrics --samplers cpu_power -n 1 > \"$OUT_DIR/cpu_runs/powermetrics_cpu.txt\" 2>&1 || true; else echo powermetrics_missing; fi"
run_step "C" "cpu_diagnostics" "mkdir -p \"$OUT_DIR/diagnostics\" && clang -O1 -g -fsanitize=address,undefined -std=gnu11 -I\"$ROOT_DIR/include\" \"$ROOT_DIR/probes/apple_cpu_latency.c\" -o \"$OUT_DIR/diagnostics/sm_apple_cpu_latency_asan\" && \"$OUT_DIR/diagnostics/sm_apple_cpu_latency_asan\" --iters 5000 --csv > \"$OUT_DIR/diagnostics/asan_run.csv\" 2>&1 || true; if [ -x \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" ]; then \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" --iters 200000000 --csv > \"$OUT_DIR/diagnostics/live_cpu_probe.csv\" 2>&1 & diag_pid=\$!; echo \"\$diag_pid\" > \"$OUT_DIR/diagnostics/live_cpu_probe.pid\"; sleep 0.2; if command -v leaks >/dev/null 2>&1; then leaks \"\$diag_pid\" > \"$OUT_DIR/diagnostics/leaks.txt\" 2>&1 || true; fi; if command -v vmmap >/dev/null 2>&1; then vmmap \"\$diag_pid\" > \"$OUT_DIR/diagnostics/vmmap.txt\" 2>&1 || true; fi; wait \"\$diag_pid\" >/dev/null 2>&1 || true; else echo missing_cpu_probe_binary > \"$OUT_DIR/diagnostics/leaks.txt\"; echo missing_cpu_probe_binary > \"$OUT_DIR/diagnostics/vmmap.txt\"; fi"

run_step "D" "metal_host_harness_build" "\"$SCRIPT_DIR/build_metal_probe_host.sh\" \"$ROOT_DIR/host/metal_probe_host.m\" \"$OUT_DIR/metal_host\""
run_step "D" "metal_variant_matrix_define" "cat > \"$OUT_DIR/metal_variant_matrix.csv\" <<'EOF'
variant,shader,notes
baseline,probe_simdgroup_reduce.metal,starter variant
EOF"
run_step "D" "metal_compile_variants" "\"$SCRIPT_DIR/compile_metal_probe.sh\" \"$ROOT_DIR/shaders/probe_simdgroup_reduce.metal\" \"$OUT_DIR/metal_probe\""
run_step "D" "metal_correctness" "WIDTH=1024 ITERS=1 \"$SCRIPT_DIR/run_metal_probe_host.sh\" \"$OUT_DIR/metal_host\" \"$OUT_DIR/metal_probe\" > \"$OUT_DIR/metal_correctness.csv\" 2>&1 || true"
run_step "D" "metal_timing_sweep" "WIDTH=1024 ITERS=200 \"$SCRIPT_DIR/run_metal_probe_host.sh\" \"$OUT_DIR/metal_host\" \"$OUT_DIR/metal_probe\" > \"$OUT_DIR/metal_timing.csv\" 2>&1 || true"
run_step "D" "publish_metal_matrix" "cat \"$OUT_DIR/metal_variant_matrix.csv\" > \"$OUT_DIR/metal_variant_matrix_published.csv\""

run_step "E" "xctrace_gpu_baseline" "if command -v xctrace >/dev/null 2>&1; then stage_dir=\$(mktemp -d /tmp/steinmarder_apple_trace_baseline.XXXXXX); cp \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" \"\$stage_dir/\"; cp \"$OUT_DIR/metal_probe/probe_simdgroup_reduce.metallib\" \"\$stage_dir/\"; echo \"\$stage_dir\" > \"$OUT_DIR/gpu_baseline_stage_dir.txt\"; xctrace record --template 'Metal System Trace' --output \"$OUT_DIR/gpu_baseline.trace\" --launch -- \"\$stage_dir/sm_apple_metal_probe_host\" --metallib \"\$stage_dir/probe_simdgroup_reduce.metallib\" --iters 1000 --width 1024 >/dev/null 2>&1 || true; else echo xctrace_missing; fi"
run_step "E" "xctrace_gpu_compare" "if command -v xctrace >/dev/null 2>&1; then stage_dir=\$(mktemp -d /tmp/steinmarder_apple_trace_compare.XXXXXX); cp \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" \"\$stage_dir/\"; cp \"$OUT_DIR/metal_probe/probe_simdgroup_reduce.metallib\" \"\$stage_dir/\"; echo \"\$stage_dir\" > \"$OUT_DIR/gpu_compare_stage_dir.txt\"; xctrace record --template 'Time Profiler' --output \"$OUT_DIR/gpu_compare.trace\" --launch -- \"\$stage_dir/sm_apple_metal_probe_host\" --metallib \"\$stage_dir/probe_simdgroup_reduce.metallib\" --iters 20000 --width 1024 >/dev/null 2>&1 || true; else echo xctrace_missing; fi"
run_step "E" "extract_xctrace_metrics" "if command -v xctrace >/dev/null 2>&1; then python3 \"$SCRIPT_DIR/extract_xctrace_metrics.py\" \"$OUT_DIR\" > \"$OUT_DIR/xctrace_extract_summary.txt\" 2>&1 || true; else ls -1 \"$OUT_DIR\"/*.trace > \"$OUT_DIR/xctrace_artifacts.txt\" 2>/dev/null || true; fi"
run_step "E" "host_overhead_capture" "if [ -x \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" ] && [ -f \"$OUT_DIR/metal_probe/probe_simdgroup_reduce.metallib\" ]; then \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" --metallib \"$OUT_DIR/metal_probe/probe_simdgroup_reduce.metallib\" --iters 50000 --width 1024 >/dev/null 2>&1 & pid=\$!; echo \"\$pid\" > \"$OUT_DIR/host_capture_pid.txt\"; sleep 0.2; if command -v fs_usage >/dev/null 2>&1; then $SUDO_INVOKE fs_usage -w -f filesys -t 1 \"\$pid\" > \"$OUT_DIR/fs_usage_gpu_host.txt\" 2>&1 || true; fi; if command -v sample >/dev/null 2>&1; then sample \"\$pid\" 1 1 -file \"$OUT_DIR/sample_gpu_host.txt\" >/dev/null 2>&1 || true; fi; if command -v spindump >/dev/null 2>&1; then $SUDO_INVOKE spindump \"\$pid\" 1 1 -o \"$OUT_DIR/spindump_gpu_host.txt\" >/dev/null 2>&1 || true; fi; wait \"\$pid\" >/dev/null 2>&1 || true; else echo host_or_metallib_missing > \"$OUT_DIR/sample_gpu_host.txt\"; fi"
run_step "E" "powermetrics_gpu" "if command -v powermetrics >/dev/null 2>&1; then $SUDO_INVOKE powermetrics --samplers gpu_power -n 1 > \"$OUT_DIR/powermetrics_gpu.txt\" 2>&1 || true; else echo powermetrics_missing; fi"
run_step "E" "counter_latency_report" "cat > \"$OUT_DIR/counter_latency_report.md\" <<'EOF'
# Counter vs Latency Report

- baseline trace: gpu_baseline.trace
- comparison trace: gpu_compare.trace
- timing csv: metal_timing.csv
- power sample: powermetrics_gpu.txt
- trace health: xctrace_trace_health.csv
- schema inventory: xctrace_schema_inventory.csv
- metric row counts: xctrace_metric_row_counts.csv
EOF"

run_step "F" "venv_rebuild" "\"$SCRIPT_DIR/bootstrap_neural_lane.sh\""
run_step "F" "neural_probe_all" "\"$REPO_ROOT/.venv/bin/python\" \"$SCRIPT_DIR/neural_lane_probe.py\" > \"$OUT_DIR/neural_probe.json\" 2>&1 || true"
run_step "F" "model_family_define" "cat > \"$OUT_DIR/neural_model_family.json\" <<'EOF'
{
  \"models\": [\"tiny_linear\", \"tiny_mlp\", \"tiny_conv\"],
  \"batch_sizes\": [1, 4, 16],
  \"dtypes\": [\"float32\", \"float16\"]
}
EOF"
run_step "F" "coreml_placement_sweep" "if [ -x \"$REPO_ROOT/.venv/bin/python\" ]; then \"$REPO_ROOT/.venv/bin/python\" \"$SCRIPT_DIR/neural_lane_probe.py\" > \"$OUT_DIR/coreml_placement_sweep.json\" 2>&1 || true; else echo missing_venv_python; fi"
run_step "F" "torch_cpu_vs_mps" "if [ -x \"$REPO_ROOT/.venv/bin/python\" ]; then \"$REPO_ROOT/.venv/bin/python\" - <<'PY' > \"$OUT_DIR/torch_cpu_vs_mps.json\" 2>&1
import json, time
import torch
res = {\"torch\": torch.__version__, \"mps\": bool(torch.backends.mps.is_available())}
x = torch.randn(512, 512)
t0 = time.time(); y = x @ x.T; res[\"cpu_ms\"] = (time.time() - t0) * 1000.0
if torch.backends.mps.is_available():
    xm = x.to(\"mps\")
    t1 = time.time(); ym = xm @ xm.T; _ = ym.cpu(); res[\"mps_ms\"] = (time.time() - t1) * 1000.0
print(json.dumps(res, indent=2))
PY
else echo missing_venv_python; fi"
run_step "F" "mlx_jax_checks" "if [ -x \"$REPO_ROOT/.venv/bin/python\" ]; then \"$REPO_ROOT/.venv/bin/python\" - <<'PY' > \"$OUT_DIR/mlx_jax_checks.json\" 2>&1
import json
out = {}
try:
    import mlx.core as mx
    out[\"mlx\"] = {\"ok\": True, \"default_device\": str(mx.default_device())}
except Exception as exc:
    out[\"mlx\"] = {\"ok\": False, \"error\": f\"{type(exc).__name__}: {exc}\"}
try:
    import jax
    out[\"jax\"] = {\"ok\": True, \"version\": jax.__version__, \"devices\": [str(d) for d in jax.devices()]}
except Exception as exc:
    out[\"jax\"] = {\"ok\": False, \"error\": f\"{type(exc).__name__}: {exc}\"}
print(json.dumps(out, indent=2))
PY
else echo missing_venv_python; fi"

run_step "G" "consolidate_manifest" "python3 - <<'PY' > \"$MANIFEST_JSON\"
import csv, json, pathlib
status_path = pathlib.Path(\"$STATUS_CSV\")
rows = list(csv.DictReader(status_path.open()))
summary = {
    \"run_dir\": \"$OUT_DIR\",
    \"total_steps\": len(rows),
    \"ok\": sum(1 for r in rows if r[\"status\"] == \"ok\"),
    \"fail\": sum(1 for r in rows if r[\"status\"] == \"fail\"),
    \"skipped_phase\": sum(1 for r in rows if r[\"status\"] == \"skipped_phase\"),
    \"steps\": rows,
}
print(json.dumps(summary, indent=2))
PY"
run_step "G" "quality_gates" "python3 - <<'PY' > \"$OUT_DIR/quality_gates.txt\"
import csv
rows = list(csv.DictReader(open(\"$STATUS_CSV\")))
failed = [r for r in rows if r[\"status\"] == \"fail\"]
print(f\"failed_steps={len(failed)}\")
print(\"gate_artifacts_present=\" + (\"1\" if rows else \"0\"))
PY"
run_step "G" "rerun_failed_snapshot" "awk -F, 'NR>1 && \$3==\"fail\" {print \$2\",\"\$4\",\"\$5}' \"$STATUS_CSV\" > \"$OUT_DIR/failed_steps.csv\""
run_step "G" "synthesis_report" "cat > \"$SUMMARY_MD\" <<'EOF'
# Apple Tranche1 Synthesis

- run_dir: $OUT_DIR
- status_csv: step_status.csv
- manifest: run_manifest.json
- manifest_final: run_manifest_final.json
- capabilities: capabilities.json
- key reports: counter_latency_report.md, quality_gates.txt, xctrace_trace_health.csv
EOF"
run_step "G" "linkage_notes" "cat > \"$OUT_DIR/linkage_notes.txt\" <<'EOF'
Update references in:
- src/sass_re/FRONTIER_ROADMAP_APPLE.md
- src/sass_re/APPLE_SILICON_RE_BRIDGE.md
- docs/README.md
with this run directory once results are promoted.
EOF"
run_step "G" "sudo_teardown" "if [ \"$SUDO_MODE\" = \"keepalive\" ]; then echo \"keepalive_pid=$KEEPALIVE_PID\"; sudo -K >/dev/null 2>&1 || true; elif [ \"$SUDO_MODE\" = \"cache\" ]; then echo \"keepalive_pid=$KEEPALIVE_PID\"; echo sudo_cache_preserved; else echo sudo_disabled; fi"

python3 - <<PY > "$OUT_DIR/run_manifest_final.json"
import csv
import json
import pathlib

status_path = pathlib.Path("$STATUS_CSV")
rows = list(csv.DictReader(status_path.open()))
summary = {
    "run_dir": "$OUT_DIR",
    "total_steps": len(rows),
    "ok": sum(1 for r in rows if r["status"] == "ok"),
    "fail": sum(1 for r in rows if r["status"] == "fail"),
    "skipped_phase": sum(1 for r in rows if r["status"] == "skipped_phase"),
    "steps": rows,
}
print(json.dumps(summary, indent=2))
PY

cp "$OUT_DIR/run_manifest_final.json" "$MANIFEST_JSON"

printf 'Completed tranche runner. Artifacts: %s\n' "$OUT_DIR"
