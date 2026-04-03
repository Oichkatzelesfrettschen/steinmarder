#include <metal_stdlib>
using namespace metal;

// Diagnostic: probe whether AGX rsqrt/recip units are pipelined.
// Uses dep-feedback (acc_k = func(acc_k)) with N=2 chains.
// Theory: if unit is pipelined with T < L, N=2 chains gives T cycles/op.
// If T=L (non-pipelined), N=2 gives L/2 cycles/op (< L) — still an improvement.
// N=2 requires L/T <= 2 → T >= L/2 to see TP.

// --- 2-chain rsqrt (precise) ---
kernel void probe_rsqrt_2chain(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc0 = float(gid + 1u) * 1.0e-6f + 1.5f;
    float acc1 = float(gid + 2u) * 1.0e-6f + 2.0f;
    for (uint32_t i = 0u; i < 100000u; i++) {
        #pragma clang fp reassociate(off)
        {
            acc0 = metal::precise::rsqrt(acc0);
            acc1 = metal::precise::rsqrt(acc1);
            acc0 = metal::precise::rsqrt(acc0);
            acc1 = metal::precise::rsqrt(acc1);
            acc0 = metal::precise::rsqrt(acc0);
            acc1 = metal::precise::rsqrt(acc1);
            acc0 = metal::precise::rsqrt(acc0);
            acc1 = metal::precise::rsqrt(acc1);
        }
    }
    out[gid] = acc0 + acc1;
}

// --- 4-chain rsqrt (precise) ---
kernel void probe_rsqrt_4chain(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc0 = float(gid + 1u) * 1.0e-6f + 1.5f;
    float acc1 = float(gid + 2u) * 1.0e-6f + 2.0f;
    float acc2 = float(gid + 3u) * 1.0e-6f + 2.5f;
    float acc3 = float(gid + 4u) * 1.0e-6f + 3.0f;
    for (uint32_t i = 0u; i < 100000u; i++) {
        #pragma clang fp reassociate(off)
        {
            acc0 = metal::precise::rsqrt(acc0);
            acc1 = metal::precise::rsqrt(acc1);
            acc2 = metal::precise::rsqrt(acc2);
            acc3 = metal::precise::rsqrt(acc3);
            acc0 = metal::precise::rsqrt(acc0);
            acc1 = metal::precise::rsqrt(acc1);
            acc2 = metal::precise::rsqrt(acc2);
            acc3 = metal::precise::rsqrt(acc3);
        }
    }
    out[gid] = acc0 + acc1 + acc2 + acc3;
}

// --- 2-chain fast divide (hardware rcp path) ---
kernel void probe_recip_fast_2chain(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc0 = float(gid + 1u) * 1.0e-6f + 1.5f;
    float acc1 = float(gid + 2u) * 1.0e-6f + 2.0f;
    for (uint32_t i = 0u; i < 100000u; i++) {
        #pragma clang fp reassociate(off)
        {
            acc0 = metal::fast::divide(1.0f, acc0);
            acc1 = metal::fast::divide(1.0f, acc1);
            acc0 = metal::fast::divide(1.0f, acc0);
            acc1 = metal::fast::divide(1.0f, acc1);
            acc0 = metal::fast::divide(1.0f, acc0);
            acc1 = metal::fast::divide(1.0f, acc1);
            acc0 = metal::fast::divide(1.0f, acc0);
            acc1 = metal::fast::divide(1.0f, acc1);
        }
    }
    out[gid] = acc0 + acc1;
}

// --- 4-chain fast divide ---
kernel void probe_recip_fast_4chain(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc0 = float(gid + 1u) * 1.0e-6f + 1.5f;
    float acc1 = float(gid + 2u) * 1.0e-6f + 2.0f;
    float acc2 = float(gid + 3u) * 1.0e-6f + 2.5f;
    float acc3 = float(gid + 4u) * 1.0e-6f + 3.0f;
    for (uint32_t i = 0u; i < 100000u; i++) {
        #pragma clang fp reassociate(off)
        {
            acc0 = metal::fast::divide(1.0f, acc0);
            acc1 = metal::fast::divide(1.0f, acc1);
            acc2 = metal::fast::divide(1.0f, acc2);
            acc3 = metal::fast::divide(1.0f, acc3);
            acc0 = metal::fast::divide(1.0f, acc0);
            acc1 = metal::fast::divide(1.0f, acc1);
            acc2 = metal::fast::divide(1.0f, acc2);
            acc3 = metal::fast::divide(1.0f, acc3);
        }
    }
    out[gid] = acc0 + acc1 + acc2 + acc3;
}
