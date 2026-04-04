#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$ROOT_DIR/../.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_metal_probes_$(date +%Y%m%d_%H%M%S)}"
ITERS="${ITERS:-800}"
WIDTH="${WIDTH:-1024}"
TYPE_SURFACE_ITERS="${TYPE_SURFACE_ITERS:-256}"
FS_PROBE_EVERY="${FS_PROBE_EVERY:-128}"
SUDO_MODE="${SUDO_MODE:-keepalive}"
SUDO_INVOKE="sudo -n"

if [ "$SUDO_MODE" = "keepalive" ] || [ "$SUDO_MODE" = "cache" ]; then
    SUDO_INVOKE="sudo -A"
fi

HOST_OUT_DIR="$OUT_DIR/metal_host"
METAL_OUT_DIR="$OUT_DIR/metal_probe_variants"
HOST_BIN="$HOST_OUT_DIR/sm_apple_metal_probe_host"

mkdir -p "$OUT_DIR" "$HOST_OUT_DIR" "$METAL_OUT_DIR" "$OUT_DIR/disassembly"

cat > "$OUT_DIR/metal_variant_matrix.csv" <<'EOF'
variant,shader,kernel,iters_correctness,iters_timing,iters_trace,notes
baseline,probe_simdgroup_reduce.metal,probe_simdgroup_reduce,1,200,1000,starter variant
threadgroup_heavy,probe_threadgroup_heavy.metal,probe_threadgroup_heavy,1,160,800,threadgroup memory pressure
threadgroup_minimal,probe_threadgroup_minimal.metal,probe_threadgroup_minimal,1,192,900,minimal threadgroup pressure
occupancy_heavy,probe_occupancy_heavy.metal,probe_occupancy_heavy,1,160,800,arithmetic occupancy pressure
register_pressure,probe_register_pressure.metal,probe_register_pressure,1,200,800,register pressure variant
dd_lat,probe_dd_lat.metal,probe_dd_lat,1,200,800,double-double dep chain latency (float2 hi/lo)
dd_tput,probe_dd_tput.metal,probe_dd_tput,1,200,800,double-double throughput 4 independent pairs
EOF
cp "$OUT_DIR/metal_variant_matrix.csv" "$OUT_DIR/metal_variant_matrix_published.csv"

python3 "$(dirname -- "$0")/classify_metal_type_surface.py" \
    --out "$OUT_DIR/metal_type_surface_matrix.csv" \
    --json-out "$OUT_DIR/metal_type_surface_matrix.json" \
    --timing-out "$OUT_DIR/metal_frontier_timing.csv" \
    --build-dir "$OUT_DIR/metal_type_surface_build" \
    --width "$WIDTH" \
    --iters "$TYPE_SURFACE_ITERS" >/dev/null 2>&1 || true

if [ ! -x "$HOST_BIN" ]; then
    "$(dirname -- "$0")/build_metal_probe_host.sh" "$ROOT_DIR/host/metal_probe_host.m" "$HOST_OUT_DIR" >/dev/null 2>&1 || true
fi

for variant in baseline threadgroup_heavy threadgroup_minimal occupancy_heavy register_pressure dd_lat dd_tput; do
    shader="probe_simdgroup_reduce.metal"
    case "$variant" in
        threadgroup_heavy) shader="probe_threadgroup_heavy.metal" ;;
        threadgroup_minimal) shader="probe_threadgroup_minimal.metal" ;;
        occupancy_heavy) shader="probe_occupancy_heavy.metal" ;;
        register_pressure) shader="probe_register_pressure.metal" ;;
        dd_lat)  shader="probe_dd_lat.metal" ;;
        dd_tput) shader="probe_dd_tput.metal" ;;
    esac

    variant_dir="$METAL_OUT_DIR/$variant"
    mkdir -p "$variant_dir"
    "$(dirname -- "$0")/compile_metal_probe.sh" "$ROOT_DIR/shaders/$shader" "$variant_dir" >/dev/null 2>&1 || true
done

echo 'variant,iters,width,elapsed_ns,ns_per_iter,ns_per_element,checksum' > "$OUT_DIR/metal_correctness.csv"
echo 'variant,iters,width,elapsed_ns,ns_per_iter,ns_per_element,checksum' > "$OUT_DIR/metal_timing.csv"

for variant in baseline threadgroup_heavy threadgroup_minimal occupancy_heavy register_pressure dd_lat dd_tput; do
    metallib="$(find "$METAL_OUT_DIR/$variant" -maxdepth 1 -type f -name '*.metallib' | head -n 1)"
    [ -z "$metallib" ] && continue

    kernel_name="probe_simdgroup_reduce"
    case "$variant" in
        threadgroup_heavy)   kernel_name="probe_threadgroup_heavy" ;;
        threadgroup_minimal) kernel_name="probe_threadgroup_minimal" ;;
        occupancy_heavy)     kernel_name="probe_occupancy_heavy" ;;
        register_pressure)   kernel_name="probe_register_pressure" ;;
        dd_lat)              kernel_name="probe_dd_lat" ;;
        dd_tput)             kernel_name="probe_dd_tput" ;;
    esac

    row="$("$HOST_BIN" --metallib "$metallib" --kernel "$kernel_name" --width "$WIDTH" --iters 1 --csv 2>/dev/null | tail -n 1)"
    [ -n "$row" ] && printf '%s,%s\n' "$variant" "$row" >> "$OUT_DIR/metal_correctness.csv"

    row="$("$HOST_BIN" --metallib "$metallib" --kernel "$kernel_name" --width "$WIDTH" --iters "$ITERS" --csv 2>/dev/null | tail -n 1)"
    [ -n "$row" ] && printf '%s,%s\n' "$variant" "$row" >> "$OUT_DIR/metal_timing.csv"
done

baseline_metallib="$(find "$METAL_OUT_DIR/baseline" -maxdepth 1 -type f -name '*.metallib' | head -n 1)"
if [ -x "$HOST_BIN" ] && [ -n "$baseline_metallib" ]; then
    "$HOST_BIN" --metallib "$baseline_metallib" --kernel probe_simdgroup_reduce --iters 50000 --width "$WIDTH" --fs-probe-every "$FS_PROBE_EVERY" >/dev/null 2>&1 &
    host_pid="$!"
    echo "$host_pid" > "$OUT_DIR/host_capture_pid.txt"
    sleep 0.2
    if command -v fs_usage >/dev/null 2>&1; then
        $SUDO_INVOKE fs_usage -w -f filesys -t 1 "$host_pid" > "$OUT_DIR/fs_usage_gpu_host.txt" 2>&1 || true
    fi
    if command -v sample >/dev/null 2>&1; then
        sample "$host_pid" 1 1 -file "$OUT_DIR/sample_gpu_host.txt" >/dev/null 2>&1 || true
    fi
    if command -v spindump >/dev/null 2>&1; then
        $SUDO_INVOKE spindump "$host_pid" 1 1 -o "$OUT_DIR/spindump_gpu_host.txt" >/dev/null 2>&1 || true
    fi
    if command -v leaks >/dev/null 2>&1; then
        $SUDO_INVOKE leaks "$host_pid" > "$OUT_DIR/gpu_host_leaks.txt" 2>&1 || true
    fi
    if command -v vmmap >/dev/null 2>&1; then
        $SUDO_INVOKE vmmap "$host_pid" > "$OUT_DIR/gpu_host_vmmap.txt" 2>&1 || true
    fi
    if command -v powermetrics >/dev/null 2>&1; then
        $SUDO_INVOKE powermetrics --samplers gpu_power -n 1 > "$OUT_DIR/powermetrics_gpu.txt" 2>&1 || true
    fi
    wait "$host_pid" >/dev/null 2>&1 || true
fi

if command -v xctrace >/dev/null 2>&1 && [ -x "$HOST_BIN" ]; then
    for variant in baseline threadgroup_heavy threadgroup_minimal occupancy_heavy register_pressure dd_lat dd_tput; do
        metallib="$(find "$METAL_OUT_DIR/$variant" -maxdepth 1 -type f -name '*.metallib' | head -n 1)"
        [ -z "$metallib" ] && continue

        kernel_name="probe_simdgroup_reduce"
        case "$variant" in
            threadgroup_heavy)   kernel_name="probe_threadgroup_heavy" ;;
            threadgroup_minimal) kernel_name="probe_threadgroup_minimal" ;;
            occupancy_heavy)     kernel_name="probe_occupancy_heavy" ;;
            register_pressure)   kernel_name="probe_register_pressure" ;;
            dd_lat)              kernel_name="probe_dd_lat" ;;
            dd_tput)             kernel_name="probe_dd_tput" ;;
        esac

        stage_dir="$(mktemp -d /tmp/steinmarder_next42_${variant}.XXXXXX)"
        cp "$HOST_BIN" "$stage_dir/sm_apple_metal_probe_host"
        cp "$metallib" "$stage_dir/$variant.metallib"
        echo "$stage_dir" > "$OUT_DIR/gpu_${variant}_stage_dir.txt"
        xctrace record --template 'Metal System Trace' --output "$OUT_DIR/gpu_${variant}.trace" --launch -- \
            "$stage_dir/sm_apple_metal_probe_host" \
            --metallib "$stage_dir/$variant.metallib" \
            --kernel "$kernel_name" \
            --iters 800 \
            --width "$WIDTH" >/dev/null 2>&1 || true
    done
fi

if find "$OUT_DIR" -maxdepth 1 -name 'gpu_*.trace' | grep -q . 2>/dev/null; then
    python3 "$(dirname -- "$0")/extract_xctrace_metrics.py" "$OUT_DIR" >/dev/null 2>&1 || true
    if [ -f "$OUT_DIR/xctrace_metric_row_counts.csv" ] && [ -f "$OUT_DIR/xctrace_trace_health.csv" ]; then
        python3 "$(dirname -- "$0")/analyze_xctrace_row_deltas.py" \
            "$OUT_DIR/xctrace_metric_row_counts.csv" \
            "$OUT_DIR/xctrace_trace_health.csv" \
            "$OUT_DIR/xctrace_row_deltas.csv" \
            "$OUT_DIR/xctrace_row_delta_summary.md" \
            "$OUT_DIR/xctrace_row_density.csv" >/dev/null 2>&1 || true
        cat > "$OUT_DIR/counter_latency_report.md" <<'EOF'
# Counter vs Latency Report

- baseline trace: gpu_baseline.trace
- comparison traces: gpu_threadgroup_heavy.trace, gpu_threadgroup_minimal.trace, gpu_occupancy_heavy.trace, gpu_register_pressure.trace
- timing csv: metal_timing.csv
- density csv: xctrace_row_density.csv
EOF
    fi
fi

cat > "$OUT_DIR/metal_notes.md" <<'EOF'
# Next42 Metal Probe Draft

- variants: baseline, threadgroup_heavy, threadgroup_minimal, occupancy_heavy, register_pressure, dd_lat, dd_tput
- dd_lat: double-double dep chain (float2 hi/lo, 8 steps/iter); measures AGX FP32 dep-chain latency for Knuth 2-sum
- dd_tput: double-double throughput (4 independent float2 pairs, 32 steps/iter); measures peak DD issue bandwidth
- expected artifacts: metal_variant_matrix.csv, metal_variant_matrix_published.csv, metal_type_surface_matrix.csv, metal_frontier_timing.csv, metal_correctness.csv, metal_timing.csv, gpu_*.trace, fs_usage_gpu_host.txt, gpu_host_leaks.txt, gpu_host_vmmap.txt
EOF

printf '%s\n' "$OUT_DIR"
