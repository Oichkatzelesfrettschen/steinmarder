#include <metal_stdlib>
using namespace metal;

// B4 v4: rsqrt TP with N=4 independent inputs per iteration.
// Reduces register pressure from ~66 (v3, N=16) to ~19 live registers.
// With L=12 cyc, TP=8 cyc: need N >= L/TP = 1.5 → N=4 is sufficient.
// Issue time per iter: 4 × 8 = 32 cycles. Per op: 32/4 = 8 cyc/op.
// ops_per_iter = 4

kernel void probe_rsqrt_tp_v4(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float bo = float(gid + 1u) * 1.0e-8f;
    float b0 = 0.50f + bo, b1 = 0.54f + bo;
    float b2 = 0.58f + bo, b3 = 0.62f + bo;
    float acc = 0.0f;

    for (uint32_t outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
        #pragma clang fp reassociate(off)
        {
            float delta = float(outer_iter) * 1.0e-6f;
            float r0 = metal::precise::rsqrt(b0 + delta);
            float r1 = metal::precise::rsqrt(b1 + delta);
            float r2 = metal::precise::rsqrt(b2 + delta);
            float r3 = metal::precise::rsqrt(b3 + delta);
            acc += (r0 + r1) + (r2 + r3);
        }
    }
    out[gid] = acc;
}
