#include <metal_stdlib>
using namespace metal;

// FP32 FADD throughput re-probe with 16 independent accumulators.
// Prior 8-accumulator probe: fadd_tp = 1.090 ns/op.
// If 16-acc gives a lower ns/op, the 8-acc probe was under-saturating the ALU unit.
// ops_per_iter = 16.
kernel void probe_fadd16acc_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        // All deltas seeded from gid to prevent constant folding across accumulators.
        float delta0  = float(gid +  0u + 1u) * 1.0e-7f + 0.00001f;
        float delta1  = float(gid +  1u + 1u) * 1.0e-7f + 0.00001f;
        float delta2  = float(gid +  2u + 1u) * 1.0e-7f + 0.00001f;
        float delta3  = float(gid +  3u + 1u) * 1.0e-7f + 0.00001f;
        float delta4  = float(gid +  4u + 1u) * 1.0e-7f + 0.00001f;
        float delta5  = float(gid +  5u + 1u) * 1.0e-7f + 0.00001f;
        float delta6  = float(gid +  6u + 1u) * 1.0e-7f + 0.00001f;
        float delta7  = float(gid +  7u + 1u) * 1.0e-7f + 0.00001f;
        float delta8  = float(gid +  8u + 1u) * 1.0e-7f + 0.00001f;
        float delta9  = float(gid +  9u + 1u) * 1.0e-7f + 0.00001f;
        float delta10 = float(gid + 10u + 1u) * 1.0e-7f + 0.00001f;
        float delta11 = float(gid + 11u + 1u) * 1.0e-7f + 0.00001f;
        float delta12 = float(gid + 12u + 1u) * 1.0e-7f + 0.00001f;
        float delta13 = float(gid + 13u + 1u) * 1.0e-7f + 0.00001f;
        float delta14 = float(gid + 14u + 1u) * 1.0e-7f + 0.00001f;
        float delta15 = float(gid + 15u + 1u) * 1.0e-7f + 0.00001f;

        float acc0  = float(gid +  0u + 1u) * 1.0e-6f + 1.0f;
        float acc1  = float(gid +  1u + 1u) * 1.0e-6f + 1.0f;
        float acc2  = float(gid +  2u + 1u) * 1.0e-6f + 1.0f;
        float acc3  = float(gid +  3u + 1u) * 1.0e-6f + 1.0f;
        float acc4  = float(gid +  4u + 1u) * 1.0e-6f + 1.0f;
        float acc5  = float(gid +  5u + 1u) * 1.0e-6f + 1.0f;
        float acc6  = float(gid +  6u + 1u) * 1.0e-6f + 1.0f;
        float acc7  = float(gid +  7u + 1u) * 1.0e-6f + 1.0f;
        float acc8  = float(gid +  8u + 1u) * 1.0e-6f + 1.0f;
        float acc9  = float(gid +  9u + 1u) * 1.0e-6f + 1.0f;
        float acc10 = float(gid + 10u + 1u) * 1.0e-6f + 1.0f;
        float acc11 = float(gid + 11u + 1u) * 1.0e-6f + 1.0f;
        float acc12 = float(gid + 12u + 1u) * 1.0e-6f + 1.0f;
        float acc13 = float(gid + 13u + 1u) * 1.0e-6f + 1.0f;
        float acc14 = float(gid + 14u + 1u) * 1.0e-6f + 1.0f;
        float acc15 = float(gid + 15u + 1u) * 1.0e-6f + 1.0f;

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            acc0  += delta0;
            acc1  += delta1;
            acc2  += delta2;
            acc3  += delta3;
            acc4  += delta4;
            acc5  += delta5;
            acc6  += delta6;
            acc7  += delta7;
            acc8  += delta8;
            acc9  += delta9;
            acc10 += delta10;
            acc11 += delta11;
            acc12 += delta12;
            acc13 += delta13;
            acc14 += delta14;
            acc15 += delta15;
        }

        float partial_lo = acc0  + acc1  + acc2  + acc3
                         + acc4  + acc5  + acc6  + acc7;
        float partial_hi = acc8  + acc9  + acc10 + acc11
                         + acc12 + acc13 + acc14 + acc15;
        out[gid] = as_type<uint>(partial_lo + partial_hi);
    }
}
