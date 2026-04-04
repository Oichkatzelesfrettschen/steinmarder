#include <metal_stdlib>
using namespace metal;

// RECIP throughput probe v2 — 16 independent accumulator chains.
//
// The 8-chain probe measured ~8 cycles vs. philip's claimed 6-cycle TP.
// With 16 chains: if T=6 cyc, need ceil(12_lat/6_tp) = 2 chains to saturate.
// 16 chains gives 8× headroom — should clearly show 6-cycle throughput if present.
//
// 1.0f/acc is the native rcp instruction on AGX.
// 100000 iterations (even) — 1/(1/x)=x, values stay bounded.
// ops_per_iter = 16

kernel void probe_recip_tp_v2(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

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
            acc0  = 1.0f / acc0;
            acc1  = 1.0f / acc1;
            acc2  = 1.0f / acc2;
            acc3  = 1.0f / acc3;
            acc4  = 1.0f / acc4;
            acc5  = 1.0f / acc5;
            acc6  = 1.0f / acc6;
            acc7  = 1.0f / acc7;
            acc8  = 1.0f / acc8;
            acc9  = 1.0f / acc9;
            acc10 = 1.0f / acc10;
            acc11 = 1.0f / acc11;
            acc12 = 1.0f / acc12;
            acc13 = 1.0f / acc13;
            acc14 = 1.0f / acc14;
            acc15 = 1.0f / acc15;
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3
                                + acc4 + acc5 + acc6 + acc7
                                + acc8 + acc9 + acc10 + acc11
                                + acc12 + acc13 + acc14 + acc15);
    }
}
