#include <metal_stdlib>
using namespace metal;

// D1: AGX simd_broadcast dep-chain latency.
//
// simd_broadcast(value, lane_id) delivers one lane's value to all 32 threads.
// This dep chain: each thread feeds the result of a broadcast back into the
// next broadcast — measuring the serialized cross-lane broadcast cost.
//
// ops_per_iter = 8 (8 broadcast calls per outer iteration)
// Broadcasting from lane 0 each time.
// Inner loop hardcoded to 100000 (consistent with other dep-chain probes).

kernel void probe_simd_broadcast_lat(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;

    for (uint iter_idx = 0u; iter_idx < 100000u; iter_idx++) {
        #pragma clang fp reassociate(off)
        {
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
            acc = simd_broadcast(acc, 0) * 1.0e-6f + 1.0f;
        }
    }

    out[gid] = acc;
}
