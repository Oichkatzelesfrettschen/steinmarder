#!/bin/sh
set -eu

if [ $# -ge 1 ]; then
    out_dir=$1
    shift
else
    out_dir=build/zen_re_phase_compare
fi

mkdir -p "$out_dir"

bench=./build/bin/nerf_simd_test
if [ ! -x "$bench" ]; then
    echo "error: $bench not built" >&2
    exit 1
fi

width=${SM_NERF_TEST_W:-64}
height=${SM_NERF_TEST_H:-64}
steps=${SM_NERF_TEST_STEPS:-64}
repeats=${SM_NERF_PHASE_REPEATS:-10}
threads=${OMP_NUM_THREADS:-1}

summary="$out_dir/summary.csv"
echo "mode,iter,render_ms,total_phase_ms,adaptive_ms,hashgrid_ms,mlp_ms,composite_ms,writeback_ms,other_ms,steps_completed,hashgrid_ns_per_step,mlp_ns_per_step,composite_ns_per_step" > "$summary"

if [ $# -eq 0 ]; then
    set -- off immediate corners4
fi

for mode_name in "$@"; do
    case "$mode_name" in
        off) mode_value=0 ;;
        immediate) mode_value=1 ;;
        ahead) mode_value=2 ;;
        corners4) mode_value=3 ;;
        *)
            echo "error: unsupported mode $mode_name" >&2
            exit 1
            ;;
    esac

    iter=1
    while [ "$iter" -le "$repeats" ]; do
        out="$out_dir/${mode_name}_$(printf '%02d' "$iter").stdout"
        env OMP_NUM_THREADS="$threads" \
            SM_NERF_PHASE_TIMING=1 \
            SM_NERF_TEST_W="$width" \
            SM_NERF_TEST_H="$height" \
            SM_NERF_TEST_STEPS="$steps" \
            SM_NERF_TEST_WRITE_PPM=0 \
            SM_NERF_HASHGRID_PREFETCH="$mode_value" \
            "$bench" > "$out" 2>&1

        python3 - "$out" "$mode_name" "$iter" >> "$summary" <<'PY'
import re, sys, pathlib
text = pathlib.Path(sys.argv[1]).read_text()
mode = sys.argv[2]
it = sys.argv[3]
render_ms = re.search(r"Rendered in ([0-9.]+) ms", text).group(1)
m1 = re.search(r"\[PHASE\] volume_integration rays=(\d+) attempted_steps=(\d+) completed_steps=(\d+) early_terminated=(\d+) total_ms=([0-9.]+)", text)
m2 = re.search(r"\[PHASE\] volume_integration adaptive_ms=([0-9.]+) hashgrid_ms=([0-9.]+) mlp_ms=([0-9.]+) composite_ms=([0-9.]+) writeback_ms=([0-9.]+) other_ms=([0-9.]+)", text)
m3 = re.search(r"\[PHASE\] volume_integration adaptive_ns_per_step=([0-9.]+) hashgrid_ns_per_step=([0-9.]+) mlp_ns_per_step=([0-9.]+) composite_ns_per_step=([0-9.]+) other_ns_per_step=([0-9.]+)", text)
print(",".join([
    mode,
    it,
    render_ms,
    m1.group(5),
    m2.group(1),
    m2.group(2),
    m2.group(3),
    m2.group(4),
    m2.group(5),
    m2.group(6),
    m1.group(3),
    m3.group(2),
    m3.group(3),
    m3.group(4),
]))
PY
        iter=$((iter + 1))
    done
done

echo "wrote $summary"
