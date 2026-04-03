#include <metal_stdlib>
using namespace metal;

// B5 redesign: AGX reciprocal throughput with independent per-iteration inputs.
//
// Prior result: probe_recip_tp (8 chains, dep-feedback) = 6.27 ns (~8.1 cyc).
// Philip says RECIP32 TP = 6 cycles. The 8-chain dep-feedback design
// (acc_k = 1/acc_k) alternates between value and reciprocal — bounded, no
// fixed point — but the dep chain through each acc_k (latency = 11 cyc)
// combined with 8 chains gives L/N = 11/8 = 1.4 cyc, floored at TP=6 →
// should give 6 cycles. Yet we measure 8.1. Discrepancy suggests the AGX
// special unit's instruction window, measured at ~8 ops, is not wide enough
// to overlap across loop iterations with only 8 chains.
//
// Fix: 16 independent recip ops per iteration, inputs derived from loop counter.
//   x_k(iter) = float(iter) * 1e-6f + base_k   (always positive, near 0.5)
//   r_k = 1.0f / x_k   (AGX emits hardware rcp via arcp flag on fast division)
//
// With 16 independent ops and TP=6:
//   Issue time = 16 × 6 = 96 cycles per iteration
//   ns_per_op = 96 / (1.3 GHz × 16) = 4.62 ns ≈ 6 cycles ← expected result
//
// ops_per_iter = 16

kernel void probe_recip_tp_v3(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float base_offset = float(gid + 1u) * 1.0e-8f;

    // 16 distinct base values in [0.50, 0.65]; all positive, same order of magnitude.
    float b0  = 0.500f + base_offset;
    float b1  = 0.510f + base_offset;
    float b2  = 0.520f + base_offset;
    float b3  = 0.530f + base_offset;
    float b4  = 0.540f + base_offset;
    float b5  = 0.550f + base_offset;
    float b6  = 0.560f + base_offset;
    float b7  = 0.570f + base_offset;
    float b8  = 0.580f + base_offset;
    float b9  = 0.590f + base_offset;
    float b10 = 0.600f + base_offset;
    float b11 = 0.610f + base_offset;
    float b12 = 0.620f + base_offset;
    float b13 = 0.630f + base_offset;
    float b14 = 0.640f + base_offset;
    float b15 = 0.650f + base_offset;

    float acc = 0.0f;

    for (uint32_t outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
        #pragma clang fp reassociate(off)
        {
            float iter_delta = float(outer_iter) * 1.0e-6f;

            float x0  = b0  + iter_delta;
            float x1  = b1  + iter_delta;
            float x2  = b2  + iter_delta;
            float x3  = b3  + iter_delta;
            float x4  = b4  + iter_delta;
            float x5  = b5  + iter_delta;
            float x6  = b6  + iter_delta;
            float x7  = b7  + iter_delta;
            float x8  = b8  + iter_delta;
            float x9  = b9  + iter_delta;
            float x10 = b10 + iter_delta;
            float x11 = b11 + iter_delta;
            float x12 = b12 + iter_delta;
            float x13 = b13 + iter_delta;
            float x14 = b14 + iter_delta;
            float x15 = b15 + iter_delta;

            // 16 independent reciprocals.
            // Metal emits `fdiv arcp` which the AGX backend lowers to hardware rcp.
            float r0  = 1.0f / x0;
            float r1  = 1.0f / x1;
            float r2  = 1.0f / x2;
            float r3  = 1.0f / x3;
            float r4  = 1.0f / x4;
            float r5  = 1.0f / x5;
            float r6  = 1.0f / x6;
            float r7  = 1.0f / x7;
            float r8  = 1.0f / x8;
            float r9  = 1.0f / x9;
            float r10 = 1.0f / x10;
            float r11 = 1.0f / x11;
            float r12 = 1.0f / x12;
            float r13 = 1.0f / x13;
            float r14 = 1.0f / x14;
            float r15 = 1.0f / x15;

            // Tree reduce (4 levels) — keeps acc dep-chain off the critical rsqrt path.
            float t0 = r0  + r1;
            float t1 = r2  + r3;
            float t2 = r4  + r5;
            float t3 = r6  + r7;
            float t4 = r8  + r9;
            float t5 = r10 + r11;
            float t6 = r12 + r13;
            float t7 = r14 + r15;
            float u0 = t0 + t1;
            float u1 = t2 + t3;
            float u2 = t4 + t5;
            float u3 = t6 + t7;
            float v0 = u0 + u1;
            float v1 = u2 + u3;
            acc += v0 + v1;
        }
    }

    out[gid] = acc;
}
