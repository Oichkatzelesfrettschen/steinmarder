#!/bin/sh
set -eu

if [ $# -ge 1 ]; then
    out_dir=$1
else
    out_dir=build/zen_re_mlp_matrix
fi

mkdir -p "$out_dir"
bench=./build/bin/nerf_mlp_bench
if [ ! -x "$bench" ]; then
    echo "error: $bench not built" >&2
    exit 1
fi

summary="$out_dir/summary.csv"
echo "mode,pattern,us_per_iter,raw" > "$summary"

for mode in single batch fixed prepacked; do
    for pattern in zeros ramp alt random; do
        raw=$("$bench" --csv --iterations 12000 --mode "$mode" --pattern "$pattern" 2>/dev/null | tail -n 1)
        us=$(printf "%s\n" "$raw" | sed -n 's/.*us_per_iter=\([0-9.]*\).*/\1/p')
        echo "$mode,$pattern,$us,\"$raw\"" >> "$summary"
    done
done

echo "wrote $summary"
