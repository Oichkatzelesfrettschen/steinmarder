#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$ROOT_DIR/../.." && pwd)"

BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
GRID="${GRID:-128}"
WARMUP="${WARMUP:-5}"
STEPS="${STEPS:-30}"
OUT_ROOT="${OUT_ROOT:-$ROOT_DIR/results/$(date +%Y%m%d_%H%M%S)_cuda_decision}"

RUN_NCU=1
RUN_NSYS=1
RUN_DISASM=1
RUN_SUMMARY=1
STEP_NO=0
TOTAL_STEPS=9

STATUS_CSV="$OUT_ROOT/step_status.csv"
COMMANDS_LOG="$OUT_ROOT/commands.log"
MANIFEST_JSON="$OUT_ROOT/run_manifest.json"
SUMMARY_MD="$OUT_ROOT/summary.md"
NCU_BASE_DIR="$OUT_ROOT/02_ncu/baseline"
NCU_EXT_DIR="$OUT_ROOT/02_ncu/extended"

usage() {
    cat <<EOF
usage: $0 [--grid N] [--warmup N] [--steps N] [--out DIR] [--build-dir DIR]
          [--skip-ncu] [--skip-nsys] [--skip-disasm] [--skip-summary]
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --grid)
            GRID="${2:-}"
            shift 2
            ;;
        --warmup)
            WARMUP="${2:-}"
            shift 2
            ;;
        --steps)
            STEPS="${2:-}"
            shift 2
            ;;
        --out)
            OUT_ROOT="${2:-}"
            STATUS_CSV="$OUT_ROOT/step_status.csv"
            COMMANDS_LOG="$OUT_ROOT/commands.log"
            MANIFEST_JSON="$OUT_ROOT/run_manifest.json"
            SUMMARY_MD="$OUT_ROOT/summary.md"
            NCU_BASE_DIR="$OUT_ROOT/02_ncu/baseline"
            NCU_EXT_DIR="$OUT_ROOT/02_ncu/extended"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --skip-ncu)
            RUN_NCU=0
            shift
            ;;
        --skip-nsys)
            RUN_NSYS=0
            shift
            ;;
        --skip-disasm)
            RUN_DISASM=0
            shift
            ;;
        --skip-summary)
            RUN_SUMMARY=0
            shift
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

mkdir -p \
    "$OUT_ROOT/00_build" \
    "$OUT_ROOT/01_baseline" \
    "$NCU_BASE_DIR" \
    "$NCU_EXT_DIR" \
    "$OUT_ROOT/03_nsys" \
    "$OUT_ROOT/04_disasm" \
    "$OUT_ROOT/05_summary" \
    "$OUT_ROOT/steps"

echo "timestamp_utc,step,status,name,artifact_dir" > "$STATUS_CSV"
: > "$COMMANDS_LOG"

mark_step() {
    step="$1"
    status="$2"
    name="$3"
    artifact="$4"
    printf '%s,%s,%s,%s,%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$step" "$status" "$name" "$artifact" >> "$STATUS_CSV"
}

run_step() {
    name="$1"
    artifact="$2"
    cmd="$3"

    STEP_NO=$((STEP_NO + 1))
    step_label="$(printf '%02d_%s' "$STEP_NO" "$name")"
    step_dir="$OUT_ROOT/steps/$step_label"
    mkdir -p "$step_dir"

    {
        echo "### STEP $STEP_NO: $name"
        echo "$cmd"
        echo
    } >> "$COMMANDS_LOG"

    printf '[%02d/%02d] %s\n' "$STEP_NO" "$TOTAL_STEPS" "$name"

    if eval "$cmd" > "$step_dir/stdout.log" 2> "$step_dir/stderr.log"; then
        mark_step "$STEP_NO" "ok" "$name" "$artifact"
        return 0
    fi

    mark_step "$STEP_NO" "fail" "$name" "$artifact"
    return 0
}

write_summary_files() {
    launch_csv="$OUT_ROOT/05_summary/cuda_launch_health.csv"
    variant_csv="$OUT_ROOT/05_summary/cuda_variant_matrix.csv"
    occupancy_csv="$OUT_ROOT/05_summary/cuda_occupancy_vs_bw.csv"
    benchmark_notes="$OUT_ROOT/05_summary/cuda_benchmark_notes.md"
    decision_summary="$OUT_ROOT/05_summary/cuda_decision_summary.md"

    cp "$STATUS_CSV" "$launch_csv"
    if [ -s "$OUT_ROOT/01_baseline/all_${GRID}.csv" ]; then
        cp "$OUT_ROOT/01_baseline/all_${GRID}.csv" "$variant_csv"
    else
        cat > "$variant_csv" <<EOF_VARIANT
status,message
missing,Baseline CSV was not produced; rerun the baseline sweep to populate this file.
EOF_VARIANT
    fi

    cat > "$occupancy_csv" <<EOF_OCC
kernel,grid,ncu_metrics_csv,ncu_rep,bw_reference_csv,source_pass
EOF_OCC

    found_metrics=0
    for metrics in "$NCU_BASE_DIR"/*_metrics.csv "$NCU_EXT_DIR"/*_metrics.csv; do
        [ -e "$metrics" ] || continue
        base="$(basename "$metrics")"
        stem="${base%_metrics.csv}"
        stem2="${stem%_*}"
        stem3="${stem2%_*}"
        grid_value="${stem3##*_}"
        kernel_name="${stem3%_*}"
        source_pass="$(basename "$(dirname "$metrics")")"
        printf '%s,%s,%s,%s,%s,%s\n' \
            "$kernel_name" "$grid_value" "$metrics" "${metrics%_metrics.csv}.ncu-rep" \
            "$OUT_ROOT/01_baseline/all_${GRID}.csv" "$source_pass" >> "$occupancy_csv"
        found_metrics=1
    done
    if [ "$found_metrics" -eq 0 ]; then
        printf 'none,%s,,,,no_ncu_metrics_found\n' "$GRID" >> "$occupancy_csv"
    fi

    ncu_count="$(find "$OUT_ROOT/02_ncu" -type f -name '*_metrics.csv' | wc -l | tr -d ' ')"
    nsys_count="$(find "$OUT_ROOT/03_nsys" -type f -name '*.nsys-rep' | wc -l | tr -d ' ')"
    sass_count="$(find "$OUT_ROOT/04_disasm" -type f -name '*.sass' | wc -l | tr -d ' ')"

    cat > "$benchmark_notes" <<EOF_NOTES
# CUDA benchmark notes

- run root: \`$OUT_ROOT\`
- build dir: \`$BUILD_DIR\`
- grid: \`$GRID\`
- warmup steps: \`$WARMUP\`
- timing steps: \`$STEPS\`
- Nsight Compute metrics files: \`$ncu_count\`
- Nsight Systems traces: \`$nsys_count\`
- SASS/disassembly files: \`$sass_count\`

## Pass mapping

- pass 0: build, smoke, and spill gate
- pass 1: baseline benchmark sweep and kernel catalog
- pass 2: Nsight Compute baseline sweep
- pass 3: Nsight Compute extended matrix sweep
- pass 4: Nsight Systems timeline capture
- pass 5: disassembly and resource usage bundle
- pass 6: summary ledgers and review notes

## Review pointers

- \`00_build/build.log\`
- \`00_build/spills.txt\`
- \`01_baseline/all_${GRID}.csv\`
- \`02_ncu/baseline/\`
- \`02_ncu/extended/\`
- \`03_nsys/\`
- \`04_disasm/\`
EOF_NOTES

    cat > "$decision_summary" <<EOF_DECISION
# CUDA Decision Summary

This tranche is operational and now has a repeatable launcher:
\`src/cuda_lbm/scripts/run_cuda_decision_tranche.sh\`

## Decision buckets

- compiler-sufficient: pending review of the generated baseline and NCU sidecars
- manual-preferred: pending review of the NCU/NSYS/disassembly bundle
- research-only: pending review of the occupancy-versus-bandwidth cross-reference

## Summary artifacts

- \`cuda_launch_health.csv\`
- \`cuda_variant_matrix.csv\`
- \`cuda_occupancy_vs_bw.csv\`
- \`cuda_benchmark_notes.md\`
- \`run_manifest.json\`

## Next action

Use the summary files plus the per-pass sidecars to decide which CUDA LBM
kernel families stay handwritten and which can be treated as compiler-sufficient.
EOF_DECISION

    cat > "$MANIFEST_JSON" <<EOF_MANIFEST
{
  "run_root": "$OUT_ROOT",
  "build_dir": "$BUILD_DIR",
  "grid": "$GRID",
  "warmup": "$WARMUP",
  "steps": "$STEPS",
  "passes": {
    "build": "00_build",
    "baseline": "01_baseline",
    "ncu_baseline": "02_ncu/baseline",
    "ncu_extended": "02_ncu/extended",
    "nsys": "03_nsys",
    "disasm": "04_disasm",
    "summary": "05_summary"
  }
}
EOF_MANIFEST

    cat > "$SUMMARY_MD" <<EOF_SUMMARY
# CUDA Decision Tranche

- run root: \`$OUT_ROOT\`
- build dir: \`$BUILD_DIR\`
- grid: \`$GRID\`
- warmup steps: \`$WARMUP\`
- timing steps: \`$STEPS\`

The full launcher is now available at:
\`src/cuda_lbm/scripts/run_cuda_decision_tranche.sh\`
EOF_SUMMARY
}

run_step "configure_build" "$OUT_ROOT/00_build" \
    "cd \"$REPO_ROOT\" && cmake -S . -B \"$BUILD_DIR\" -DCMAKE_BUILD_TYPE=Release > \"$OUT_ROOT/00_build/cmake.log\" 2>&1"

run_step "compile_build" "$OUT_ROOT/00_build" \
    "cd \"$REPO_ROOT\" && cmake --build \"$BUILD_DIR\" --target lbm_bench lbm_test > \"$OUT_ROOT/00_build/build.log\" 2>&1"

run_step "spill_and_smoke" "$OUT_ROOT/00_build" \
    "cd \"$REPO_ROOT\" && sh \"$SCRIPT_DIR/check_spills.sh\" \"$OUT_ROOT/00_build/build.log\" > \"$OUT_ROOT/00_build/spills.txt\" 2>&1 && \"$BUILD_DIR/bin/lbm_test\" > \"$OUT_ROOT/00_build/lbm_test.log\" 2>&1 && \"$BUILD_DIR/bin/lbm_bench\" --validate > \"$OUT_ROOT/00_build/lbm_validate.log\" 2>&1 && \"$BUILD_DIR/bin/lbm_bench\" --regression > \"$OUT_ROOT/00_build/lbm_regression.log\" 2>&1"

run_step "baseline_sweep" "$OUT_ROOT/01_baseline" \
    "cd \"$REPO_ROOT\" && \"$BUILD_DIR/bin/lbm_bench\" --all --grid \"$GRID\" --output table > \"$OUT_ROOT/01_baseline/all_${GRID}_table.log\" 2>&1 && \"$BUILD_DIR/bin/lbm_bench\" --all --grid \"$GRID\" --output csv | awk 'BEGIN{flag=0} /^kernel,/{flag=1} flag' > \"$OUT_ROOT/01_baseline/all_${GRID}.csv\" && \"$BUILD_DIR/bin/lbm_bench\" --list > \"$OUT_ROOT/01_baseline/kernel_list.txt\" 2>&1 && \"$BUILD_DIR/bin/lbm_bench\" --occupancy > \"$OUT_ROOT/01_baseline/occupancy.txt\" 2>&1"

if [ "$RUN_NCU" -eq 1 ]; then
    run_step "ncu_baseline" "$NCU_BASE_DIR" \
        "sh \"$SCRIPT_DIR/profile_ncu_all.sh\" \"$NCU_BASE_DIR\" > \"$NCU_BASE_DIR/profile_ncu_all.log\" 2>&1"

    run_step "ncu_extended" "$NCU_EXT_DIR" \
        "fail=0; for k in int8_soa fp16_soa fp16_soa_h2 int16_soa bf16_soa fp32_soa_cs fp32_soa_fused fp64_soa int8_soa_lloydmax int8_soa_lloydmax_pipe q16_soa q16_soa_lm q32_soa int8_soa_mrt_aa; do sh \"$SCRIPT_DIR/profile_ncu.sh\" \"$k\" \"$GRID\" \"$NCU_EXT_DIR\" >> \"$NCU_EXT_DIR/profile_ncu_extended.log\" 2>&1 || fail=1; done; [ \"$fail\" -eq 0 ]"
else
    STEP_NO=$((STEP_NO + 1))
    mark_step "$STEP_NO" "skipped" "ncu_baseline" "$NCU_BASE_DIR"
    STEP_NO=$((STEP_NO + 1))
    mark_step "$STEP_NO" "skipped" "ncu_extended" "$NCU_EXT_DIR"
fi

if [ "$RUN_NSYS" -eq 1 ]; then
    run_step "nsys_trace" "$OUT_ROOT/03_nsys" \
        "sh \"$SCRIPT_DIR/profile_nsys.sh\" --grid \"$GRID\" --output \"$OUT_ROOT/03_nsys\" > \"$OUT_ROOT/03_nsys/profile_nsys.log\" 2>&1"
else
    STEP_NO=$((STEP_NO + 1))
    mark_step "$STEP_NO" "skipped" "nsys_trace" "$OUT_ROOT/03_nsys"
fi

if [ "$RUN_DISASM" -eq 1 ]; then
    run_step "disasm_bundle" "$OUT_ROOT/04_disasm" \
        "cd \"$REPO_ROOT\" && fail=0; if ! cuobjdump --dump-sass \"$BUILD_DIR/bin/lbm_bench\" > \"$OUT_ROOT/04_disasm/lbm_bench.sass\" 2> \"$OUT_ROOT/04_disasm/cuobjdump_sass.log\"; then fail=1; fi; if ! cuobjdump --dump-resource-usage \"$BUILD_DIR/bin/lbm_bench\" > \"$OUT_ROOT/04_disasm/lbm_bench_resources.txt\" 2> \"$OUT_ROOT/04_disasm/cuobjdump_resources.log\"; then fail=1; fi; cubin_path=\$(find \"$BUILD_DIR\" -type f -name '*.cubin' | head -n 1 || true); if [ -n \"\$cubin_path\" ] && command -v nvdisasm >/dev/null 2>&1; then nvdisasm --print-code \"\$cubin_path\" > \"$OUT_ROOT/04_disasm/lbm_bench_raw.sass\" 2> \"$OUT_ROOT/04_disasm/nvdisasm.log\" || fail=1; else printf '%s\n' '# nvdisasm raw disassembly was not available for this run.' '# Inspect the cuobjdump sidecars and the embedded cubin search path instead.' > \"$OUT_ROOT/04_disasm/lbm_bench_raw.sass\"; fi; [ \"\$fail\" -eq 0 ]"
else
    STEP_NO=$((STEP_NO + 1))
    mark_step "$STEP_NO" "skipped" "disasm_bundle" "$OUT_ROOT/04_disasm"
fi

if [ "$RUN_SUMMARY" -eq 1 ]; then
    run_step "summary_files" "$OUT_ROOT/05_summary" "write_summary_files"
else
    STEP_NO=$((STEP_NO + 1))
    mark_step "$STEP_NO" "skipped" "summary_files" "$OUT_ROOT/05_summary"
fi

echo "CUDA decision tranche complete."
echo "Run root: $OUT_ROOT"
