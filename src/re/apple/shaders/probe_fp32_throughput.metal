#include <metal_stdlib>
using namespace metal;

kernel void probe_fadd8_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        float delta = float(gid + 1u) * 1.0e-7f + 0.00001f;
        float acc0 = float(gid + 0u + 1u) * 1.0e-6f + 1.0f;
        float acc1 = float(gid + 1u + 1u) * 1.0e-6f + 1.0f;
        float acc2 = float(gid + 2u + 1u) * 1.0e-6f + 1.0f;
        float acc3 = float(gid + 3u + 1u) * 1.0e-6f + 1.0f;
        float acc4 = float(gid + 4u + 1u) * 1.0e-6f + 1.0f;
        float acc5 = float(gid + 5u + 1u) * 1.0e-6f + 1.0f;
        float acc6 = float(gid + 6u + 1u) * 1.0e-6f + 1.0f;
        float acc7 = float(gid + 7u + 1u) * 1.0e-6f + 1.0f;

        for (uint outer = 0u; outer < 100000u; outer++) {
            acc0 += delta;
            acc1 += delta;
            acc2 += delta;
            acc3 += delta;
            acc4 += delta;
            acc5 += delta;
            acc6 += delta;
            acc7 += delta;
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3 + acc4 + acc5 + acc6 + acc7);
    }
}

kernel void probe_fmul8_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        float step = float(gid + 1u) * 1.0e-8f + 1.000001f;
        float acc0 = float(gid + 0u + 1u) * 1.0e-6f + 1.0f;
        float acc1 = float(gid + 1u + 1u) * 1.0e-6f + 1.0f;
        float acc2 = float(gid + 2u + 1u) * 1.0e-6f + 1.0f;
        float acc3 = float(gid + 3u + 1u) * 1.0e-6f + 1.0f;
        float acc4 = float(gid + 4u + 1u) * 1.0e-6f + 1.0f;
        float acc5 = float(gid + 5u + 1u) * 1.0e-6f + 1.0f;
        float acc6 = float(gid + 6u + 1u) * 1.0e-6f + 1.0f;
        float acc7 = float(gid + 7u + 1u) * 1.0e-6f + 1.0f;

        for (uint outer = 0u; outer < 100000u; outer++) {
            acc0 *= step;
            acc1 *= step;
            acc2 *= step;
            acc3 *= step;
            acc4 *= step;
            acc5 *= step;
            acc6 *= step;
            acc7 *= step;
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3 + acc4 + acc5 + acc6 + acc7);
    }
}

kernel void probe_fma8_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        float step  = float(gid + 1u) * 1.0e-8f + 1.000001f;
        float delta = float(gid + 1u) * 1.0e-7f + 0.00001f;
        float acc0 = float(gid + 0u + 1u) * 1.0e-6f + 1.0f;
        float acc1 = float(gid + 1u + 1u) * 1.0e-6f + 1.0f;
        float acc2 = float(gid + 2u + 1u) * 1.0e-6f + 1.0f;
        float acc3 = float(gid + 3u + 1u) * 1.0e-6f + 1.0f;
        float acc4 = float(gid + 4u + 1u) * 1.0e-6f + 1.0f;
        float acc5 = float(gid + 5u + 1u) * 1.0e-6f + 1.0f;
        float acc6 = float(gid + 6u + 1u) * 1.0e-6f + 1.0f;
        float acc7 = float(gid + 7u + 1u) * 1.0e-6f + 1.0f;

        for (uint outer = 0u; outer < 100000u; outer++) {
            acc0 = fma(acc0, step, delta);
            acc1 = fma(acc1, step, delta);
            acc2 = fma(acc2, step, delta);
            acc3 = fma(acc3, step, delta);
            acc4 = fma(acc4, step, delta);
            acc5 = fma(acc5, step, delta);
            acc6 = fma(acc6, step, delta);
            acc7 = fma(acc7, step, delta);
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3 + acc4 + acc5 + acc6 + acc7);
    }
}
