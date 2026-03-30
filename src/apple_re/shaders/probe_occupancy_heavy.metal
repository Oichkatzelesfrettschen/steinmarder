#include <metal_stdlib>

using namespace metal;

kernel void probe_simdgroup_reduce(device uint *out [[buffer(0)]],
                                   uint gid [[thread_position_in_grid]],
                                   uint tid [[thread_index_in_threadgroup]],
                                   uint lane [[thread_index_in_simdgroup]],
                                   uint simd_group [[simdgroup_index_in_threadgroup]]) {
    uint x = gid ^ (tid << 4) ^ (lane << 9) ^ (simd_group << 15);

    #pragma unroll 32
    for (uint i = 0; i < 32; ++i) {
        x = x * 1664525u + 1013904223u;
        x ^= (x >> ((i & 7u) + 1u));
        x += ((tid + i) * 2654435761u);
        x ^= ((lane + 1u) * (simd_group + 3u));
    }

    out[gid] = x;
}
