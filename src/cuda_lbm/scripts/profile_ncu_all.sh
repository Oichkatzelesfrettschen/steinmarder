#!/bin/sh
# Batch Nsight Compute profiling: all physics-valid SoA kernels at 128^3.
# Calls profile_ncu.sh for each kernel.

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTDIR="${1:-results}"

KERNELS="
int8_soa
fp8_e4m3_soa
fp8_e5m2_soa
fp16_soa_h2
int16_soa
fp16_soa
bf16_soa
fp32_soa_cs
fp32_soa_fused
fp64_soa
"

echo "=== Batch NCU Profiling: All Physics-Valid SoA Kernels at 128^3 ==="

FAIL=0
for k in $KERNELS; do
    echo ""
    echo "--- $k ---"
    sh "$SCRIPT_DIR/profile_ncu.sh" "$k" 128 "$OUTDIR" || FAIL=1
done

echo ""
echo "=== Batch profiling complete. Results in: $OUTDIR ==="
exit "$FAIL"
