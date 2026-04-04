#include <metal_stdlib>
using namespace metal;

// D2: AGX simd_shuffle_down dep-chain latency.
//
// simd_shuffle_down(value, delta) moves each lane's value delta positions down
// (lane N gets the value from lane N+delta; last delta lanes get lane 0's value).
// Delta = 1 measures the cost of a single-step inter-lane shift.
//
// This dep chain: each shuffle result feeds directly into the next shuffle —
// measuring the serialized cross-lane shuffle latency.
//
// ops_per_iter = 8
// Inner loop hardcoded to 100000.

kernel void probe_simd_shuffle_down_lat(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;

    for (uint iter_idx = 0u; iter_idx < 100000u; iter_idx++) {
        #pragma clang fp reassociate(off)
        {
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_down(acc, 1u) * 1.0e-6f + 1.0f;
        }
    }

    out[gid] = acc;
}

// D2b: simd_shuffle_xor — XOR-based butterfly (same cycle cost, different reach).
kernel void probe_simd_shuffle_xor_lat(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;

    for (uint iter_idx = 0u; iter_idx < 100000u; iter_idx++) {
        #pragma clang fp reassociate(off)
        {
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
            acc = simd_shuffle_xor(acc, 1u) * 1.0e-6f + 1.0f;
        }
    }

    out[gid] = acc;
}
