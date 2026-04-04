#!/bin/sh
set -eu

if [ $# -ge 1 ]; then
    out_dir=$1
else
    out_dir=build/zen_re_variant_sweep
fi

mkdir -p "$out_dir"

bench=./build/bin/nerf_simd_test
if [ ! -x "$bench" ]; then
    echo "error: $bench not built" >&2
    exit 1
fi

repeats=${SM_NERF_VARIANT_REPEATS:-4}
cases=${SM_NERF_VARIANT_CASES:-"64x64x64x1 96x96x64x1 128x72x96x1 64x64x64x6 96x96x64x6 128x72x96x6"}
summary="$out_dir/raw.csv"
aggregate="$out_dir/aggregate.csv"

echo "case_id,iter,variant,width,height,steps,threads,render_ms,total_phase_ms,hashgrid_ns_per_step,mlp_ns_per_step,composite_ns_per_step" > "$summary"

case_id=0
for spec in $cases; do
    width=$(printf "%s" "$spec" | cut -d x -f 1)
    height=$(printf "%s" "$spec" | cut -d x -f 2)
    steps=$(printf "%s" "$spec" | cut -d x -f 3)
    threads=$(printf "%s" "$spec" | cut -d x -f 4)
    iter=1
    while [ "$iter" -le "$repeats" ]; do
        for variant in generic prepacked; do
            out="$out_dir/case${case_id}_${variant}_$(printf '%02d' "$iter").stdout"
            if [ "$variant" = "generic" ]; then
                env OMP_NUM_THREADS="$threads" \
                    SM_NERF_PHASE_TIMING=1 \
                    SM_NERF_TEST_W="$width" \
                    SM_NERF_TEST_H="$height" \
                    SM_NERF_TEST_STEPS="$steps" \
                    SM_NERF_TEST_WRITE_PPM=0 \
                    SM_NERF_HASHGRID_PREFETCH=0 \
                    "$bench" > "$out" 2>&1
            else
                env OMP_NUM_THREADS="$threads" \
                    SM_NERF_PHASE_TIMING=1 \
                    SM_NERF_TEST_W="$width" \
                    SM_NERF_TEST_H="$height" \
                    SM_NERF_TEST_STEPS="$steps" \
                    SM_NERF_TEST_WRITE_PPM=0 \
                    SM_NERF_HASHGRID_PREFETCH=0 \
                    SM_NERF_MLP_VARIANT=prepacked \
                    "$bench" > "$out" 2>&1
            fi

            python3 - "$out" "$case_id" "$iter" "$variant" "$width" "$height" "$steps" "$threads" >> "$summary" <<'PY'
import pathlib, re, sys
text = pathlib.Path(sys.argv[1]).read_text()
render = re.search(r"Rendered in ([0-9.]+) ms", text)
phase1 = re.search(r"\[PHASE\] volume_integration rays=(\d+) attempted_steps=(\d+) completed_steps=(\d+) early_terminated=(\d+) total_ms=([0-9.]+)", text)
phase3 = re.search(r"\[PHASE\] volume_integration adaptive_ns_per_step=([0-9.]+) hashgrid_ns_per_step=([0-9.]+) mlp_ns_per_step=([0-9.]+) composite_ns_per_step=([0-9.]+) other_ns_per_step=([0-9.]+)", text)
print(",".join([
    sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8],
    render.group(1), phase1.group(5), phase3.group(2), phase3.group(3), phase3.group(4)
]))
PY
        done
        iter=$((iter + 1))
    done
    case_id=$((case_id + 1))
done

python3 - "$summary" > "$aggregate" <<'PY'
import csv, sys
from collections import defaultdict
rows = list(csv.DictReader(open(sys.argv[1], newline="")))
acc = defaultdict(lambda: {"count": 0, "render_ms": 0.0, "total_phase_ms": 0.0, "hashgrid_ns_per_step": 0.0, "mlp_ns_per_step": 0.0, "composite_ns_per_step": 0.0})
for row in rows:
    key = (row["case_id"], row["variant"], row["width"], row["height"], row["steps"], row["threads"])
    slot = acc[key]
    slot["count"] += 1
    for metric in ["render_ms", "total_phase_ms", "hashgrid_ns_per_step", "mlp_ns_per_step", "composite_ns_per_step"]:
        slot[metric] += float(row[metric])
print("case_id,variant,width,height,steps,threads,avg_render_ms,avg_total_phase_ms,avg_hashgrid_ns_per_step,avg_mlp_ns_per_step,avg_composite_ns_per_step")
for key in sorted(acc):
    slot = acc[key]
    n = slot["count"]
    print(",".join([
        key[0], key[1], key[2], key[3], key[4], key[5],
        f"{slot['render_ms']/n:.3f}",
        f"{slot['total_phase_ms']/n:.3f}",
        f"{slot['hashgrid_ns_per_step']/n:.3f}",
        f"{slot['mlp_ns_per_step']/n:.3f}",
        f"{slot['composite_ns_per_step']/n:.3f}",
    ]))
PY

echo "wrote $summary"
echo "wrote $aggregate"
