#include <metal_stdlib>
using namespace metal;

// FP16 throughput probes — 8 independent accumulators each.
// Tests Rosenzweig's claim that AGX has MORE 16-bit ALUs than 32-bit ALUs,
// predicting fp16 throughput < fp32 throughput.
// Prior fp32 baselines: fadd_tp=1.090ns, fmul_tp=0.751ns, fma_tp=1.764ns
// ops_per_iter = 8 for each kernel.

// ---- probe_fadd16_tp --------------------------------------------------------
// 8 independent half-precision FADD accumulators.
kernel void probe_fadd16_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        half delta = half(float(gid + 1u) * 1.0e-5f + 0.001f);

        half acc0 = half(float(gid +  0u + 1u) * 1.0e-4f + 1.0f);
        half acc1 = half(float(gid +  1u + 1u) * 1.0e-4f + 1.0f);
        half acc2 = half(float(gid +  2u + 1u) * 1.0e-4f + 1.0f);
        half acc3 = half(float(gid +  3u + 1u) * 1.0e-4f + 1.0f);
        half acc4 = half(float(gid +  4u + 1u) * 1.0e-4f + 1.0f);
        half acc5 = half(float(gid +  5u + 1u) * 1.0e-4f + 1.0f);
        half acc6 = half(float(gid +  6u + 1u) * 1.0e-4f + 1.0f);
        half acc7 = half(float(gid +  7u + 1u) * 1.0e-4f + 1.0f);

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            acc0 += delta;
            acc1 += delta;
            acc2 += delta;
            acc3 += delta;
            acc4 += delta;
            acc5 += delta;
            acc6 += delta;
            acc7 += delta;
        }

        float accumulated_sum = float(acc0 + acc1 + acc2 + acc3
                                    + acc4 + acc5 + acc6 + acc7);
        out[gid] = as_type<uint>(accumulated_sum);
    }
}

// ---- probe_fmul16_tp --------------------------------------------------------
// 8 independent half-precision FMUL accumulators.
kernel void probe_fmul16_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        half step = half(float(gid + 1u) * 1.0e-6f + 1.0001f);

        half acc0 = half(float(gid +  0u + 1u) * 1.0e-4f + 1.0f);
        half acc1 = half(float(gid +  1u + 1u) * 1.0e-4f + 1.0f);
        half acc2 = half(float(gid +  2u + 1u) * 1.0e-4f + 1.0f);
        half acc3 = half(float(gid +  3u + 1u) * 1.0e-4f + 1.0f);
        half acc4 = half(float(gid +  4u + 1u) * 1.0e-4f + 1.0f);
        half acc5 = half(float(gid +  5u + 1u) * 1.0e-4f + 1.0f);
        half acc6 = half(float(gid +  6u + 1u) * 1.0e-4f + 1.0f);
        half acc7 = half(float(gid +  7u + 1u) * 1.0e-4f + 1.0f);

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            acc0 *= step;
            acc1 *= step;
            acc2 *= step;
            acc3 *= step;
            acc4 *= step;
            acc5 *= step;
            acc6 *= step;
            acc7 *= step;
        }

        float accumulated_sum = float(acc0 + acc1 + acc2 + acc3
                                    + acc4 + acc5 + acc6 + acc7);
        out[gid] = as_type<uint>(accumulated_sum);
    }
}

// ---- probe_fma16_tp ---------------------------------------------------------
// 8 independent half-precision FMA accumulators.
kernel void probe_fma16_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        half step  = half(float(gid + 1u) * 1.0e-6f + 1.0001f);
        half delta = half(float(gid + 1u) * 1.0e-5f + 0.001f);

        half acc0 = half(float(gid +  0u + 1u) * 1.0e-4f + 1.0f);
        half acc1 = half(float(gid +  1u + 1u) * 1.0e-4f + 1.0f);
        half acc2 = half(float(gid +  2u + 1u) * 1.0e-4f + 1.0f);
        half acc3 = half(float(gid +  3u + 1u) * 1.0e-4f + 1.0f);
        half acc4 = half(float(gid +  4u + 1u) * 1.0e-4f + 1.0f);
        half acc5 = half(float(gid +  5u + 1u) * 1.0e-4f + 1.0f);
        half acc6 = half(float(gid +  6u + 1u) * 1.0e-4f + 1.0f);
        half acc7 = half(float(gid +  7u + 1u) * 1.0e-4f + 1.0f);

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            acc0 = fma(acc0, step, delta);
            acc1 = fma(acc1, step, delta);
            acc2 = fma(acc2, step, delta);
            acc3 = fma(acc3, step, delta);
            acc4 = fma(acc4, step, delta);
            acc5 = fma(acc5, step, delta);
            acc6 = fma(acc6, step, delta);
            acc7 = fma(acc7, step, delta);
        }

        float accumulated_sum = float(acc0 + acc1 + acc2 + acc3
                                    + acc4 + acc5 + acc6 + acc7);
        out[gid] = as_type<uint>(accumulated_sum);
    }
}
