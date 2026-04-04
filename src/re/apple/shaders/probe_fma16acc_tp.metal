#include <metal_stdlib>
using namespace metal;

// FMA throughput re-probe with 16 INDEPENDENT accumulators.
// Each accumulator runs fma(acc_k, step_k, delta_k) once per outer iteration.
// step_k and delta_k are runtime-variable per accumulator (seeded from gid + k).
// 16 chains should fully saturate any hidden serialization present in the 8-chain probe.
// ops_per_iter = 16
kernel void probe_fma16acc_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        // Seed 16 independent step and delta values from gid+k to prevent
        // the compiler from folding them into a single constant.
        float step0  = float(gid +  0u + 1u) * 1.0e-8f + 1.000001f;
        float step1  = float(gid +  1u + 1u) * 1.0e-8f + 1.000001f;
        float step2  = float(gid +  2u + 1u) * 1.0e-8f + 1.000001f;
        float step3  = float(gid +  3u + 1u) * 1.0e-8f + 1.000001f;
        float step4  = float(gid +  4u + 1u) * 1.0e-8f + 1.000001f;
        float step5  = float(gid +  5u + 1u) * 1.0e-8f + 1.000001f;
        float step6  = float(gid +  6u + 1u) * 1.0e-8f + 1.000001f;
        float step7  = float(gid +  7u + 1u) * 1.0e-8f + 1.000001f;
        float step8  = float(gid +  8u + 1u) * 1.0e-8f + 1.000001f;
        float step9  = float(gid +  9u + 1u) * 1.0e-8f + 1.000001f;
        float step10 = float(gid + 10u + 1u) * 1.0e-8f + 1.000001f;
        float step11 = float(gid + 11u + 1u) * 1.0e-8f + 1.000001f;
        float step12 = float(gid + 12u + 1u) * 1.0e-8f + 1.000001f;
        float step13 = float(gid + 13u + 1u) * 1.0e-8f + 1.000001f;
        float step14 = float(gid + 14u + 1u) * 1.0e-8f + 1.000001f;
        float step15 = float(gid + 15u + 1u) * 1.0e-8f + 1.000001f;

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
            acc0  = fma(acc0,  step0,  delta0);
            acc1  = fma(acc1,  step1,  delta1);
            acc2  = fma(acc2,  step2,  delta2);
            acc3  = fma(acc3,  step3,  delta3);
            acc4  = fma(acc4,  step4,  delta4);
            acc5  = fma(acc5,  step5,  delta5);
            acc6  = fma(acc6,  step6,  delta6);
            acc7  = fma(acc7,  step7,  delta7);
            acc8  = fma(acc8,  step8,  delta8);
            acc9  = fma(acc9,  step9,  delta9);
            acc10 = fma(acc10, step10, delta10);
            acc11 = fma(acc11, step11, delta11);
            acc12 = fma(acc12, step12, delta12);
            acc13 = fma(acc13, step13, delta13);
            acc14 = fma(acc14, step14, delta14);
            acc15 = fma(acc15, step15, delta15);
        }

        // Sum all 16 accumulators — forces all live, prevents DCE.
        float partial_lo = acc0  + acc1  + acc2  + acc3
                         + acc4  + acc5  + acc6  + acc7;
        float partial_hi = acc8  + acc9  + acc10 + acc11
                         + acc12 + acc13 + acc14 + acc15;
        out[gid] = as_type<uint>(partial_lo + partial_hi);
    }
}
