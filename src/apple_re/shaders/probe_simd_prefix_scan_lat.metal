#include <metal_stdlib>
using namespace metal;

// D3: AGX simd_prefix_inclusive_sum dep-chain latency.
//
// simd_prefix_inclusive_sum(x) computes an inclusive prefix sum across all 32 threads
// in the simdgroup. Each thread i gets the sum of all threads 0..i.
//
// Dep chain: the result of one prefix sum feeds into the next call.
// Scale by 1e-6 after each call to prevent overflow (32 × x would grow otherwise).
//
// ops_per_iter = 8 (8 calls per inner iteration)

kernel void probe_simd_prefix_sum_lat(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;

    for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
        #pragma clang fp reassociate(off)
        {
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_inclusive_sum(acc) * 1.0e-6f + 1.0f;
        }
    }

    out[gid] = acc;
}

// D3b: simd_prefix_exclusive_sum for comparison.
kernel void probe_simd_prefix_exclusive_sum_lat(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;

    for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
        #pragma clang fp reassociate(off)
        {
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_prefix_exclusive_sum(acc) * 1.0e-6f + 1.0f;
        }
    }

    out[gid] = acc;
}
