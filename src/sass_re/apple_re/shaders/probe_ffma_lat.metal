#include <metal_stdlib>

using namespace metal;

// Dependent FMA chain probe.
// All 32 iterations have a data dependency: each fma result feeds the next.
// This measures FP32 FMA *latency*, not throughput.
// Compare with probe_ffma_tput.metal (independent accumulators) for throughput.
kernel void probe_ffma_lat(device float *out [[buffer(0)]],
                           constant uint &iters [[buffer(1)]],
                           uint gid [[thread_position_in_grid]]) {
    float acc = float(gid) * 0.00001f + 1.0f;
    const float mul_a = 1.00000012f;
    const float add_b = 0.99999994f;

    for (uint i = 0; i < iters; ++i) {
        // 32 dependent FMA ops per iteration — no loop unroll to preserve the
        // dependency chain and prevent the compiler from vectorizing or reordering.
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
        acc = fma(acc, mul_a, add_b);
    }

    out[gid] = acc;
}
