#include <metal_stdlib>

using namespace metal;

// Independent FMA throughput probe.
// 8 fully independent accumulators — no cross-dependencies between chains.
// This measures FP32 FMA *throughput* (issue bandwidth), not latency.
// Compare with probe_ffma_lat.metal (single dependent chain) for latency.
kernel void probe_ffma_tput(device float *out [[buffer(0)]],
                            constant uint &iters [[buffer(1)]],
                            uint gid [[thread_position_in_grid]]) {
    float base = float(gid) * 0.00001f + 1.0f;
    // 8 independent chains — the GPU can issue all 8 in parallel if it has
    // enough issue slots. Throughput = ops / (elapsed_cycles * active_threads).
    float a0 = base + 0.0f;
    float a1 = base + 1.0f;
    float a2 = base + 2.0f;
    float a3 = base + 3.0f;
    float a4 = base + 4.0f;
    float a5 = base + 5.0f;
    float a6 = base + 6.0f;
    float a7 = base + 7.0f;

    const float mul_a = 1.00000012f;
    const float add_b = 0.99999994f;

    for (uint i = 0; i < iters; ++i) {
        a0 = fma(a0, mul_a, add_b);
        a1 = fma(a1, mul_a, add_b);
        a2 = fma(a2, mul_a, add_b);
        a3 = fma(a3, mul_a, add_b);
        a4 = fma(a4, mul_a, add_b);
        a5 = fma(a5, mul_a, add_b);
        a6 = fma(a6, mul_a, add_b);
        a7 = fma(a7, mul_a, add_b);
        a0 = fma(a0, mul_a, add_b);
        a1 = fma(a1, mul_a, add_b);
        a2 = fma(a2, mul_a, add_b);
        a3 = fma(a3, mul_a, add_b);
        a4 = fma(a4, mul_a, add_b);
        a5 = fma(a5, mul_a, add_b);
        a6 = fma(a6, mul_a, add_b);
        a7 = fma(a7, mul_a, add_b);
        a0 = fma(a0, mul_a, add_b);
        a1 = fma(a1, mul_a, add_b);
        a2 = fma(a2, mul_a, add_b);
        a3 = fma(a3, mul_a, add_b);
        a4 = fma(a4, mul_a, add_b);
        a5 = fma(a5, mul_a, add_b);
        a6 = fma(a6, mul_a, add_b);
        a7 = fma(a7, mul_a, add_b);
        a0 = fma(a0, mul_a, add_b);
        a1 = fma(a1, mul_a, add_b);
        a2 = fma(a2, mul_a, add_b);
        a3 = fma(a3, mul_a, add_b);
        a4 = fma(a4, mul_a, add_b);
        a5 = fma(a5, mul_a, add_b);
        a6 = fma(a6, mul_a, add_b);
        a7 = fma(a7, mul_a, add_b);
    }

    // Sink all accumulators to prevent DCE.
    out[gid] = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
}
