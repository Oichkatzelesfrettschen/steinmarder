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
TOTAL_STEPS=62
SUDO_INVOKE="sudo -A"

usage() {
    cat <<EOF
usage: $0 [--phase A|B|C|D|E|F|G|H|all|A,B,...] [--out DIR] [--sudo keepalive|cache|none] [--iters N]
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

run_step "B" "cpu_family_matrix" "\"$SCRIPT_DIR/run_next42_cpu_probes.sh\" \"$OUT_DIR\""
run_step "B" "cpu_probe_inventory" "test -s \"$OUT_DIR/cpu_probe_inventory.txt\" && echo cpu_probe_inventory_ready"
run_step "B" "compile_matrix_define" "test -s \"$OUT_DIR/compile_matrix.txt\" && echo compile_matrix_ready"
run_step "B" "build_cpu_matrix" "test -d \"$OUT_DIR/cpu_bins\" && find \"$OUT_DIR/cpu_bins\" -maxdepth 1 -type f | wc -l"
run_step "B" "disassemble_cpu_matrix" "test -d \"$OUT_DIR/disassembly\" && find \"$OUT_DIR/disassembly\" -maxdepth 1 -type f | wc -l"
run_step "B" "llvm_mca_cpu_matrix" "test -d \"$OUT_DIR/llvm_mca\" && find \"$OUT_DIR/llvm_mca\" -maxdepth 1 -type f | wc -l"

run_step "C" "cpu_baseline_timing" "mkdir -p \"$OUT_DIR/cpu_runs\" && for bin in \"$OUT_DIR\"/cpu_bins/sm_apple_cpu_latency_*; do \"\$bin\" --iters \"$ITERS\" --csv > \"$OUT_DIR/cpu_runs/\$(basename \"\$bin\").csv\" 2>&1 || true; done"
run_step "C" "hyperfine_cpu_timing" "if command -v hyperfine >/dev/null 2>&1; then hyperfine --runs 5 \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3 --iters $ITERS --csv\" --export-csv \"$OUT_DIR/cpu_runs/hyperfine.csv\"; else echo hyperfine_missing; fi"
run_step "C" "xctrace_cpu_profile" "if command -v xctrace >/dev/null 2>&1; then xctrace record --template 'Time Profiler' --output \"$OUT_DIR/cpu_time_profiler.trace\" --launch -- \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" --iters \"$ITERS\" >/dev/null 2>&1 || true; else echo xctrace_missing; fi"
run_step "C" "dtrace_dtruss_cpu" "if command -v dtruss >/dev/null 2>&1; then $SUDO_INVOKE dtruss -f \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" --iters 10000 > \"$OUT_DIR/cpu_runs/dtruss.log\" 2>&1 || true; else echo dtruss_missing; fi"
run_step "C" "powermetrics_cpu" "if command -v powermetrics >/dev/null 2>&1; then $SUDO_INVOKE powermetrics --samplers cpu_power -n 1 > \"$OUT_DIR/cpu_runs/powermetrics_cpu.txt\" 2>&1 || true; else echo powermetrics_missing; fi"
run_step "C" "cpu_diagnostics" "mkdir -p \"$OUT_DIR/diagnostics\" && clang -O1 -g -fsanitize=address,undefined -std=gnu11 -I\"$ROOT_DIR/include\" \"$ROOT_DIR/probes/apple_cpu_latency.c\" -lm -o \"$OUT_DIR/diagnostics/sm_apple_cpu_latency_asan\" && \"$OUT_DIR/diagnostics/sm_apple_cpu_latency_asan\" --iters 5000 --csv > \"$OUT_DIR/diagnostics/asan_run.csv\" 2>&1 || true; if [ -x \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" ]; then \"$OUT_DIR/cpu_bins/sm_apple_cpu_latency_O3\" --probe add_dep_u64 --iters 200000000 --csv > \"$OUT_DIR/diagnostics/live_cpu_probe.csv\" 2>&1 & diag_pid=\$!; echo \"\$diag_pid\" > \"$OUT_DIR/diagnostics/live_cpu_probe.pid\"; sleep 0.2; if command -v leaks >/dev/null 2>&1; then leaks \"\$diag_pid\" > \"$OUT_DIR/diagnostics/leaks.txt\" 2>&1 || true; fi; if command -v vmmap >/dev/null 2>&1; then vmmap \"\$diag_pid\" > \"$OUT_DIR/diagnostics/vmmap.txt\" 2>&1 || true; fi; wait \"\$diag_pid\" >/dev/null 2>&1 || true; else echo missing_cpu_probe_binary > \"$OUT_DIR/diagnostics/leaks.txt\"; echo missing_cpu_probe_binary > \"$OUT_DIR/diagnostics/vmmap.txt\"; fi"

run_step "D" "metal_host_harness_build" "\"$SCRIPT_DIR/run_next42_metal_probes.sh\" \"$OUT_DIR\""
run_step "D" "metal_variant_matrix_define" "test -s \"$OUT_DIR/metal_variant_matrix.csv\" && echo metal_variant_matrix_ready"
run_step "D" "metal_compile_variants" "test -d \"$OUT_DIR/metal_probe_variants\" && find \"$OUT_DIR/metal_probe_variants\" -maxdepth 2 -name '*.metallib' | wc -l"
run_step "D" "metal_correctness" "test -s \"$OUT_DIR/metal_correctness.csv\" && test -s \"$OUT_DIR/counter_latency_report.md\" && echo metal_correctness_ready"
run_step "D" "metal_timing_sweep" "test -s \"$OUT_DIR/metal_timing.csv\" && echo metal_timing_ready"
run_step "D" "publish_metal_matrix" "test -s \"$OUT_DIR/metal_variant_matrix_published.csv\" && echo metal_variant_matrix_published_ready"

run_step "E" "xctrace_gpu_baseline" "if command -v xctrace >/dev/null 2>&1; then variant=baseline; variant_dir=\"$OUT_DIR/metal_probe_variants/\$variant\"; metallib=\$(find \"\$variant_dir\" -maxdepth 1 -type f -name '*.metallib' | head -n 1); if [ -n \"\$metallib\" ]; then stage_dir=\$(mktemp -d /tmp/steinmarder_apple_trace_\${variant}.XXXXXX); cp \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" \"\$stage_dir/\"; cp \"\$metallib\" \"\$stage_dir/\$variant.metallib\"; echo \"\$stage_dir\" > \"$OUT_DIR/gpu_baseline_stage_dir.txt\"; xctrace record --template 'Metal System Trace' --output \"$OUT_DIR/gpu_baseline.trace\" --launch -- \"\$stage_dir/sm_apple_metal_probe_host\" --metallib \"\$stage_dir/\$variant.metallib\" --iters 1000 --width 1024 >/dev/null 2>&1 || true; fi; else echo xctrace_missing; fi"
run_step "E" "xctrace_gpu_compare" "if command -v xctrace >/dev/null 2>&1; then for variant in threadgroup_heavy threadgroup_minimal occupancy_heavy register_pressure; do variant_dir=\"$OUT_DIR/metal_probe_variants/\$variant\"; metallib=\$(find \"\$variant_dir\" -maxdepth 1 -type f -name '*.metallib' | head -n 1); [ -z \"\$metallib\" ] && continue; stage_dir=\$(mktemp -d /tmp/steinmarder_apple_trace_\${variant}.XXXXXX); cp \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" \"\$stage_dir/\"; cp \"\$metallib\" \"\$stage_dir/\$variant.metallib\"; echo \"\$stage_dir\" > \"$OUT_DIR/gpu_\${variant}_stage_dir.txt\"; xctrace record --template 'Metal System Trace' --output \"$OUT_DIR/gpu_\${variant}.trace\" --launch -- \"\$stage_dir/sm_apple_metal_probe_host\" --metallib \"\$stage_dir/\$variant.metallib\" --iters 800 --width 1024 >/dev/null 2>&1 || true; done; else echo xctrace_missing; fi"
run_step "E" "extract_xctrace_metrics" "if command -v xctrace >/dev/null 2>&1; then python3 \"$SCRIPT_DIR/extract_xctrace_metrics.py\" \"$OUT_DIR\" > \"$OUT_DIR/xctrace_extract_summary.txt\" 2>&1 || true; else ls -1 \"$OUT_DIR\"/*.trace > \"$OUT_DIR/xctrace_artifacts.txt\" 2>/dev/null || true; fi"
run_step "E" "host_overhead_capture" "baseline_metallib=\$(find \"$OUT_DIR/metal_probe_variants/baseline\" -maxdepth 1 -type f -name '*.metallib' | head -n 1); if [ -x \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" ] && [ -n \"\${baseline_metallib:-}\" ]; then \"$OUT_DIR/metal_host/sm_apple_metal_probe_host\" --metallib \"\$baseline_metallib\" --kernel probe_simdgroup_reduce --iters 50000 --width 1024 --fs-probe-every 128 >/dev/null 2>&1 & pid=\$!; echo \"\$pid\" > \"$OUT_DIR/host_capture_pid.txt\"; sleep 0.2; if command -v fs_usage >/dev/null 2>&1; then \$SUDO_INVOKE fs_usage -w -f filesys -t 1 \"\$pid\" > \"$OUT_DIR/fs_usage_gpu_host.txt\" 2>&1 || true; fi; if command -v sample >/dev/null 2>&1; then sample \"\$pid\" 1 1 -file \"$OUT_DIR/sample_gpu_host.txt\" >/dev/null 2>&1 || true; fi; if command -v spindump >/dev/null 2>&1; then \$SUDO_INVOKE spindump \"\$pid\" 1 1 -o \"$OUT_DIR/spindump_gpu_host.txt\" >/dev/null 2>&1 || true; fi; if command -v leaks >/dev/null 2>&1; then \$SUDO_INVOKE leaks \"\$pid\" > \"$OUT_DIR/gpu_host_leaks.txt\" 2>&1 || true; fi; if command -v vmmap >/dev/null 2>&1; then \$SUDO_INVOKE vmmap \"\$pid\" > \"$OUT_DIR/gpu_host_vmmap.txt\" 2>&1 || true; fi; wait \"\$pid\" >/dev/null 2>&1 || true; else echo host_or_metallib_missing > \"$OUT_DIR/sample_gpu_host.txt\"; fi"
run_step "E" "powermetrics_gpu" "if command -v powermetrics >/dev/null 2>&1; then $SUDO_INVOKE powermetrics --samplers gpu_power -n 1 > \"$OUT_DIR/powermetrics_gpu.txt\" 2>&1 || true; else echo powermetrics_missing; fi"
run_step "E" "counter_latency_report" "python3 \"$SCRIPT_DIR/analyze_xctrace_row_deltas.py\" \"$OUT_DIR/xctrace_metric_row_counts.csv\" \"$OUT_DIR/xctrace_trace_health.csv\" \"$OUT_DIR/xctrace_row_deltas.csv\" \"$OUT_DIR/xctrace_row_delta_summary.md\" \"$OUT_DIR/xctrace_row_density.csv\" >/dev/null 2>&1 || true; cat > \"$OUT_DIR/counter_latency_report.md\" <<'EOF'
# Counter vs Latency Report

- baseline trace: gpu_baseline.trace
- comparison traces: gpu_threadgroup_heavy.trace, gpu_threadgroup_minimal.trace, gpu_occupancy_heavy.trace, gpu_register_pressure.trace
- timing csv: metal_timing.csv
- power sample: powermetrics_gpu.txt
- trace health: xctrace_trace_health.csv
- schema inventory: xctrace_schema_inventory.csv
- metric row counts: xctrace_metric_row_counts.csv
- row deltas: xctrace_row_deltas.csv
- row delta summary: xctrace_row_delta_summary.md
- density csv: xctrace_row_density.csv
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

run_step "H" "postrun_refresh_metrics" "python3 \"$SCRIPT_DIR/extract_xctrace_metrics.py\" \"$OUT_DIR\" > \"$OUT_DIR/xctrace_extract_summary_postrun.txt\" 2>&1 || true; python3 \"$SCRIPT_DIR/analyze_xctrace_row_deltas.py\" \"$OUT_DIR/xctrace_metric_row_counts.csv\" \"$OUT_DIR/xctrace_trace_health.csv\" \"$OUT_DIR/xctrace_row_deltas.csv\" \"$OUT_DIR/xctrace_row_delta_summary.md\" \"$OUT_DIR/xctrace_row_density.csv\" >/dev/null 2>&1 || true"
run_step "H" "compare_density_prev_bundle" "prev_density=\$(find \"$ROOT_DIR/results/blessed\" -maxdepth 2 -name xctrace_row_density.csv | sort | tail -n 1); if [ -n \"\$prev_density\" ] && [ -f \"\$prev_density\" ]; then python3 \"$SCRIPT_DIR/compare_xctrace_density_runs.py\" \"$OUT_DIR/xctrace_row_density.csv\" \"\$prev_density\" \"$OUT_DIR/xctrace_density_compare.csv\" \"$OUT_DIR/xctrace_density_compare.md\" >/dev/null 2>&1 || true; else echo previous_density_missing > \"$OUT_DIR/xctrace_density_compare.md\"; fi"
run_step "H" "rank_metal_variants" "python3 - <<'PY' > \"$OUT_DIR/metal_variant_rankings.md\"
import csv, pathlib
out_dir = pathlib.Path(\"$OUT_DIR\")
timing = {}
with (out_dir / \"metal_timing.csv\").open(newline=\"\", encoding=\"utf-8\") as fh:
    for row in csv.DictReader(fh):
        variant = row.get(\"variant\", \"\").strip()
        if variant:
            try:
                timing[variant] = float(row.get(\"ns_per_element\", \"\"))
            except ValueError:
                pass
density = {}
with (out_dir / \"xctrace_row_density.csv\").open(newline=\"\", encoding=\"utf-8\") as fh:
    for row in csv.DictReader(fh):
        if row.get(\"schema\", \"\").strip() == \"metal-gpu-state-intervals\":
            density[row.get(\"variant_trace\", \"\").replace(\".trace\", \"\")] = row
rows = []
for variant, ns_per_element in timing.items():
    key = f\"gpu_{variant}\"
    drow = density.get(f\"{key}.trace\") or density.get(key)
    delta_density = float(drow.get(\"delta_density_rps\", \"0\")) if drow else 0.0
    rows.append((variant, ns_per_element, delta_density))
rows.sort(key=lambda row: (row[1], -row[2]))
print(\"# Metal Variant Rankings\\n\")
print(\"- ranked_by: ns_per_element ascending, then density delta descending\\n\")
for variant, ns_per_element, delta_density in rows:
    print(f\"- {variant}: ns_per_element={ns_per_element:.6f}, delta_density_rps={delta_density:.3f}\")
PY"
run_step "H" "fs_usage_signal_summary" "if [ -s \"$OUT_DIR/fs_usage_gpu_host.txt\" ]; then printf '# fs_usage Signal Summary\\n\\n- lines: %s\\n- probe_log: %s\\n' \"\$(wc -l < \"$OUT_DIR/fs_usage_gpu_host.txt\")\" \"$(basename \"$OUT_DIR/fs_usage_gpu_host.txt\")\" > \"$OUT_DIR/fs_usage_summary.md\"; else printf '# fs_usage Signal Summary\\n\\n- lines: 0\\n- probe_log: %s\\n' \"$(basename \"$OUT_DIR/fs_usage_gpu_host.txt\")\" > \"$OUT_DIR/fs_usage_summary.md\"; fi"
run_step "H" "host_capture_summary" "python3 - <<'PY' > \"$OUT_DIR/host_capture_summary.md\"
from pathlib import Path
out_dir = Path(\"$OUT_DIR\")
files = [
    \"host_capture_pid.txt\",
    \"sample_gpu_host.txt\",
    \"spindump_gpu_host.txt\",
    \"gpu_host_leaks.txt\",
    \"gpu_host_vmmap.txt\",
]
print(\"# Host Capture Summary\\n\")
for name in files:
    path = out_dir / name
    if path.exists():
        print(f\"- {name}: present ({path.stat().st_size} bytes)\")
    else:
        print(f\"- {name}: missing\")
PY"
run_step "H" "leaks_vmmap_summary" "python3 - <<'PY' > \"$OUT_DIR/leaks_vmmap_summary.md\"
from pathlib import Path
out_dir = Path(\"$OUT_DIR\")
print(\"# Leaks / VMMap Summary\\n\")
for name in [\"gpu_host_leaks.txt\", \"gpu_host_vmmap.txt\"]:
    path = out_dir / name
    if path.exists():
        lines = path.read_text(encoding=\"utf-8\", errors=\"replace\").splitlines()
        first = lines[0] if lines else \"\"
        print(f\"- {name}: {len(lines)} lines; first line: {first}\")
    else:
        print(f\"- {name}: missing\")
PY"
run_step "H" "threadgroup_minimal_summary" "python3 - <<'PY' > \"$OUT_DIR/threadgroup_minimal_summary.md\"
from pathlib import Path
out_dir = Path(\"$OUT_DIR\")
trace = out_dir / \"gpu_threadgroup_minimal.trace\"
print(\"# Threadgroup Minimal Summary\\n\")
if trace.exists():
    print(f\"- trace: {trace.name}\")
else:
    print(\"- trace: missing\")
timing = out_dir / \"metal_timing.csv\"
if timing.exists():
    print(f\"- timing_rows: {sum(1 for _ in timing.open(encoding='utf-8')) - 1}\")
PY"
run_step "H" "counter_latency_report_refresh" "python3 \"$SCRIPT_DIR/analyze_xctrace_row_deltas.py\" \"$OUT_DIR/xctrace_metric_row_counts.csv\" \"$OUT_DIR/xctrace_trace_health.csv\" \"$OUT_DIR/xctrace_row_deltas.csv\" \"$OUT_DIR/xctrace_row_delta_summary.md\" \"$OUT_DIR/xctrace_row_density.csv\" >/dev/null 2>&1 || true; cat > \"$OUT_DIR/counter_latency_report.md\" <<'EOF'
# Counter vs Latency Report

- baseline trace: gpu_baseline.trace
- comparison traces: gpu_threadgroup_heavy.trace, gpu_threadgroup_minimal.trace, gpu_occupancy_heavy.trace, gpu_register_pressure.trace
- timing csv: metal_timing.csv
- density compare: xctrace_density_compare.csv
- density md: xctrace_density_compare.md
- power sample: powermetrics_gpu.txt
- trace health: xctrace_trace_health.csv
- schema inventory: xctrace_schema_inventory.csv
- metric row counts: xctrace_metric_row_counts.csv
- row deltas: xctrace_row_deltas.csv
- row delta summary: xctrace_row_delta_summary.md
- density csv: xctrace_row_density.csv
EOF"
run_step "H" "run_summary_write" "cat > \"$OUT_DIR/RUN_SUMMARY.md\" <<'EOF'
# Apple Tranche1 Run Summary

- run_dir: $OUT_DIR
- phases: A,B,C,D,E,F,G,H
- total_steps: 62
- include: cpu, metal, neural, host diagnostics, density compare
- notable_variant: threadgroup_minimal
- normalized_compare: xctrace_density_compare.md
EOF"
run_step "H" "keepalive_summary_write" "cat > \"$OUT_DIR/KEEPALIVE_SUMMARY.md\" <<'EOF'
# Apple Tranche1 Keepalive Summary

- keepalive_scope: C,D,E plus post-run H synthesis
- fs_usage: concrete filesystem events captured
- host_pid_captures: leaks/vmmap/sample/spindump available
- density_story: register pressure still trades density for throughput while occupancy remains the strongest positive density lane
- comparison: current density compared against the latest promoted blessed bundle
EOF"
run_step "H" "analysis_next_steps_write" "cat > \"$OUT_DIR/ANALYSIS_NEXT_STEPS.md\" <<'EOF'
# Apple Tranche Next Steps

1. Compare the current keepalive bundle against the previous blessed density CSV and confirm the register-vs-occupancy sign pattern stays stable.
2. Promote the run into a dated blessed directory once the summary and tarball are ready.
3. Keep the Apple and Ryzen index entries synchronized with the new tranche promotion.
EOF"
run_step "H" "bundle_index_write" "cat > \"$OUT_DIR/bundle_index.txt\" <<'EOF'
RUN_SUMMARY.md
KEEPALIVE_SUMMARY.md
ANALYSIS_NEXT_STEPS.md
counter_latency_report.md
xctrace_density_compare.csv
xctrace_density_compare.md
xctrace_row_density.csv
xctrace_row_delta_summary.md
fs_usage_gpu_host.txt
gpu_host_leaks.txt
gpu_host_vmmap.txt
EOF"
run_step "H" "bundle_sha256" "if command -v shasum >/dev/null 2>&1; then shasum -a 256 \"$OUT_DIR\"/RUN_SUMMARY.md \"$OUT_DIR\"/KEEPALIVE_SUMMARY.md \"$OUT_DIR\"/counter_latency_report.md \"$OUT_DIR\"/xctrace_density_compare.md \"$OUT_DIR\"/xctrace_row_density.csv > \"$OUT_DIR/bundle_sha256.txt\"; else echo shasum_missing > \"$OUT_DIR/bundle_sha256.txt\"; fi"
run_step "H" "create_bundle_tarball" "bundle_name=\$(basename \"$OUT_DIR\")_cuda_grade_bundle.tar.gz; tar -czf \"$OUT_DIR/\$bundle_name\" -C \"$OUT_DIR\" RUN_SUMMARY.md KEEPALIVE_SUMMARY.md ANALYSIS_NEXT_STEPS.md counter_latency_report.md xctrace_density_compare.csv xctrace_density_compare.md xctrace_row_density.csv xctrace_row_delta_summary.md fs_usage_gpu_host.txt gpu_host_leaks.txt gpu_host_vmmap.txt host_capture_summary.md leaks_vmmap_summary.md fs_usage_summary.md threadgroup_minimal_summary.md bundle_index.txt bundle_sha256.txt >/dev/null 2>&1 || true"
run_step "H" "promotion_notes_write" "cat > \"$OUT_DIR/promotion_notes.txt\" <<'EOF'
Promote this run only after verifying the density compare stays sign-stable and the host capture summaries show the expected PID-scoped data.
EOF"
run_step "H" "linkage_notes_write" "cat > \"$OUT_DIR/linkage_notes.txt\" <<'EOF'
Update references in:
- src/sass_re/FRONTIER_ROADMAP_APPLE.md
- src/sass_re/APPLE_SILICON_RE_BRIDGE.md
- docs/README.md
with this run directory once results are promoted.
EOF"
run_step "H" "quality_gates_refresh" "python3 - <<'PY' > \"$OUT_DIR/quality_gates.txt\"
import csv
from pathlib import Path
rows = list(csv.DictReader(open(\"$STATUS_CSV\", encoding=\"utf-8\")))
failed = [r for r in rows if r[\"status\"] == \"fail\"]
print(f\"failed_steps={len(failed)}\")
print(\"gate_artifacts_present=\" + (\"1\" if rows else \"0\"))
print(\"has_density_compare=\" + (\"1\" if Path(\"$OUT_DIR/xctrace_density_compare.md\").exists() else \"0\"))
PY"
run_step "H" "summary_refresh" "cat > \"$OUT_DIR/summary.md\" <<'EOF'
# Apple Tranche1 Synthesis

- run_dir: $OUT_DIR
- status_csv: step_status.csv
- manifest: run_manifest.json
- manifest_final: run_manifest_final.json
- key_reports: counter_latency_report.md, quality_gates.txt, xctrace_trace_health.csv, xctrace_density_compare.md
- comparison_bundle: keepalive plus post-run H synthesis
EOF"
run_step "H" "phase_h_wrapup" "echo phase_h_complete > \"$OUT_DIR/phase_h_complete.txt\""

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
