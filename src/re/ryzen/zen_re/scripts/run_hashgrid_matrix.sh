#!/bin/sh
set -eu

if [ $# -ge 1 ]; then
    out_dir=$1
else
    out_dir=build/zen_re_hashgrid_matrix
fi

mkdir -p "$out_dir"

bench=./build/bin/nerf_hashgrid_bench
if [ ! -x "$bench" ]; then
    echo "error: $bench not built" >&2
    exit 1
fi

summary="$out_dir/summary.csv"
context="$out_dir/context.txt"

{
    date -Iseconds
    uname -a
    lscpu
} > "$context" 2>&1 || true

echo "prefetch,pattern,logical_rays,cold_cache_bytes,us_per_iter,raw" > "$summary"

for prefetch in 1 0; do
    for pattern in coherent linear random; do
        for logical_rays in 1 2 4 8; do
            for cold_cache_bytes in 0 32768 524288 4194304; do
                raw=$(env SM_NERF_HASHGRID_PREFETCH=$prefetch "$bench" \
                    --csv \
                    --iterations 2000 \
                    --pattern "$pattern" \
                    --logical-rays "$logical_rays" \
                    --cold-cache-bytes "$cold_cache_bytes" \
                    2>/dev/null | tail -n 1)
                us_per_iter=$(printf "%s\n" "$raw" | sed -n 's/.*us_per_iter=\([0-9.]*\).*/\1/p')
                echo "$prefetch,$pattern,$logical_rays,$cold_cache_bytes,$us_per_iter,\"$raw\"" >> "$summary"
            done
        done
    done
done

echo "wrote $summary"
