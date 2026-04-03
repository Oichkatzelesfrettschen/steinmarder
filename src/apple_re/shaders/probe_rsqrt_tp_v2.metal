#include <metal_stdlib>
using namespace metal;

// RSQRT throughput probe v2 — 16 independent accumulator chains.
//
// The original 8-chain probe measured latency == dep-chain (9.39 ns) because
// AGX's RSQRT unit has ~12-cycle latency and the 8 chains did not hide it
// (8 × T_issue < 12-cycle pipeline depth → latency-bound at 8 chains).
//
// With 16 chains: if philip's claim T=8 cyc throughput holds, we need
// ceil(12/8) = 2 chains to hide latency; 16 chains gives >6× headroom.
// Each chain is a serial dep chain (acc_k = rsqrt(acc_k)).
// 16 chains × 1 register each = 16 regs — well within 128 GPR budget.
//
// ops_per_iter = 16

kernel void probe_rsqrt_tp_v2(
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
            acc0  = rsqrt(acc0);
            acc1  = rsqrt(acc1);
            acc2  = rsqrt(acc2);
            acc3  = rsqrt(acc3);
            acc4  = rsqrt(acc4);
            acc5  = rsqrt(acc5);
            acc6  = rsqrt(acc6);
            acc7  = rsqrt(acc7);
            acc8  = rsqrt(acc8);
            acc9  = rsqrt(acc9);
            acc10 = rsqrt(acc10);
            acc11 = rsqrt(acc11);
            acc12 = rsqrt(acc12);
            acc13 = rsqrt(acc13);
            acc14 = rsqrt(acc14);
            acc15 = rsqrt(acc15);
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3
                                + acc4 + acc5 + acc6 + acc7
                                + acc8 + acc9 + acc10 + acc11
                                + acc12 + acc13 + acc14 + acc15);
    }
}
