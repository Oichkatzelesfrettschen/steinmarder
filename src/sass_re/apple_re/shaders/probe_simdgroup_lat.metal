#include <metal_stdlib>

using namespace metal;

// T7: AGX simdgroup_sum cross-lane reduction latency dep chain.
//
// Each thread starts with a unique seed value.  A serial chain is built where
// the result of each simd_sum feeds directly into the next call.  The
// multiply-by-1e-6 after each sum keeps values from overflowing when 32 lanes
// accumulate repeatedly.
//
// 8 simd_sum calls per outer iteration.
// ops_per_iter = 8

kernel void probe_simdgroup_sum_lat(device float   *out   [[buffer(0)]],
                                    constant uint  &iters [[buffer(1)]],
                                    uint gid [[thread_position_in_grid]]) {
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;

    for (uint iter_idx = 0; iter_idx < iters; ++iter_idx) {
#pragma clang fp reassociate(off)
        {
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
            acc = simd_sum(acc) * 1.0e-6f + 1.0f;
        }
    }

    out[gid] = acc;
}
