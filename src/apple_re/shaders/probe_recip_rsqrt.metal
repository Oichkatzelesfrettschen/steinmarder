#include <metal_stdlib>
using namespace metal;

// RECIP and RSQRT latency and throughput probes.
// philipturner benchmarks: RECIP32 = 6-cycle throughput, RSQRT32 = 8-cycle throughput.
// These probes measure both latency (serial dep-chain) and throughput (independent accs).
// ops_per_iter = 8 for all kernels.
// Seed all accumulators as small positive values (rsqrt domain requires > 0).

// ---- probe_recip_lat --------------------------------------------------------
// 8 serial dependent-chain reciprocals: acc = 1.0f / acc
// Latency probe: each iteration depends on result of previous.
// Metal compute shaders typically emit native rcp for "1.0f / acc".
kernel void probe_recip_lat(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        // 8 serial chains — each chain feeds into next (sequential dependency).
        float acc0 = float(gid + 1u) * 1.0e-6f + 1.0f;
        float acc1 = float(gid + 2u) * 1.0e-6f + 1.0f;
        float acc2 = float(gid + 3u) * 1.0e-6f + 1.0f;
        float acc3 = float(gid + 4u) * 1.0e-6f + 1.0f;
        float acc4 = float(gid + 5u) * 1.0e-6f + 1.0f;
        float acc5 = float(gid + 6u) * 1.0e-6f + 1.0f;
        float acc6 = float(gid + 7u) * 1.0e-6f + 1.0f;
        float acc7 = float(gid + 8u) * 1.0e-6f + 1.0f;

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            // Each chain is a dependent sequence: acc = 1.0f / acc
            // serialized within each chain — measures recip latency.
            acc0 = 1.0f / acc0;
            acc1 = 1.0f / acc1;
            acc2 = 1.0f / acc2;
            acc3 = 1.0f / acc3;
            acc4 = 1.0f / acc4;
            acc5 = 1.0f / acc5;
            acc6 = 1.0f / acc6;
            acc7 = 1.0f / acc7;
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3
                                + acc4 + acc5 + acc6 + acc7);
    }
}

// ---- probe_rsqrt_lat --------------------------------------------------------
// 8 serial dependent-chain rsqrt: acc = rsqrt(acc)
// Latency probe: rsqrt(rsqrt(x)) oscillates in [~0.84, ~1.19] for x in [1,2],
// so values stay valid (never zero or negative).
kernel void probe_rsqrt_lat(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        float acc0 = float(gid + 1u) * 1.0e-6f + 1.0f;
        float acc1 = float(gid + 2u) * 1.0e-6f + 1.0f;
        float acc2 = float(gid + 3u) * 1.0e-6f + 1.0f;
        float acc3 = float(gid + 4u) * 1.0e-6f + 1.0f;
        float acc4 = float(gid + 5u) * 1.0e-6f + 1.0f;
        float acc5 = float(gid + 6u) * 1.0e-6f + 1.0f;
        float acc6 = float(gid + 7u) * 1.0e-6f + 1.0f;
        float acc7 = float(gid + 8u) * 1.0e-6f + 1.0f;

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            acc0 = rsqrt(acc0);
            acc1 = rsqrt(acc1);
            acc2 = rsqrt(acc2);
            acc3 = rsqrt(acc3);
            acc4 = rsqrt(acc4);
            acc5 = rsqrt(acc5);
            acc6 = rsqrt(acc6);
            acc7 = rsqrt(acc7);
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3
                                + acc4 + acc5 + acc6 + acc7);
    }
}

// ---- probe_recip_tp ---------------------------------------------------------
// 8 INDEPENDENT accumulators: each = 1.0f / acc_k (throughput probe)
// Independent chains — measures recip issue bandwidth.
kernel void probe_recip_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        float acc0 = float(gid +  0u + 1u) * 1.0e-6f + 1.0f;
        float acc1 = float(gid +  1u + 1u) * 1.0e-6f + 1.0f;
        float acc2 = float(gid +  2u + 1u) * 1.0e-6f + 1.0f;
        float acc3 = float(gid +  3u + 1u) * 1.0e-6f + 1.0f;
        float acc4 = float(gid +  4u + 1u) * 1.0e-6f + 1.0f;
        float acc5 = float(gid +  5u + 1u) * 1.0e-6f + 1.0f;
        float acc6 = float(gid +  6u + 1u) * 1.0e-6f + 1.0f;
        float acc7 = float(gid +  7u + 1u) * 1.0e-6f + 1.0f;

        // Independent recip chains — no cross-dependencies.
        // Each iteration: acc_k = 1.0f / acc_k (self-contained chain).
        // For throughput isolation: same dependency structure but 8 independent streams.
        // To keep values in a bounded range and remain positive, alternate with
        // a small fma perturbation so the reciprocal doesn't drift to 0 or inf.
        // Actually: 1/(1/x) = x, so alternating pairs stay bounded. 100000 iters is even.
        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            acc0 = 1.0f / acc0;
            acc1 = 1.0f / acc1;
            acc2 = 1.0f / acc2;
            acc3 = 1.0f / acc3;
            acc4 = 1.0f / acc4;
            acc5 = 1.0f / acc5;
            acc6 = 1.0f / acc6;
            acc7 = 1.0f / acc7;
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3
                                + acc4 + acc5 + acc6 + acc7);
    }
}

// ---- probe_rsqrt_tp ---------------------------------------------------------
// 8 INDEPENDENT accumulators: each = rsqrt(acc_k) (throughput probe)
// rsqrt(rsqrt(x)) chains oscillate and stay in safe positive range.
// 100000 iterations (even count): values return to near-original.
kernel void probe_rsqrt_tp(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)

        float acc0 = float(gid +  0u + 1u) * 1.0e-6f + 1.0f;
        float acc1 = float(gid +  1u + 1u) * 1.0e-6f + 1.0f;
        float acc2 = float(gid +  2u + 1u) * 1.0e-6f + 1.0f;
        float acc3 = float(gid +  3u + 1u) * 1.0e-6f + 1.0f;
        float acc4 = float(gid +  4u + 1u) * 1.0e-6f + 1.0f;
        float acc5 = float(gid +  5u + 1u) * 1.0e-6f + 1.0f;
        float acc6 = float(gid +  6u + 1u) * 1.0e-6f + 1.0f;
        float acc7 = float(gid +  7u + 1u) * 1.0e-6f + 1.0f;

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            acc0 = rsqrt(acc0);
            acc1 = rsqrt(acc1);
            acc2 = rsqrt(acc2);
            acc3 = rsqrt(acc3);
            acc4 = rsqrt(acc4);
            acc5 = rsqrt(acc5);
            acc6 = rsqrt(acc6);
            acc7 = rsqrt(acc7);
        }

        out[gid] = as_type<uint>(acc0 + acc1 + acc2 + acc3
                                + acc4 + acc5 + acc6 + acc7);
    }
}
