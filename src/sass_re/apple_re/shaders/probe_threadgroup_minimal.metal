#include <metal_stdlib>

using namespace metal;

kernel void probe_simdgroup_reduce(device uint *out [[buffer(0)]],
                                   uint gid [[thread_position_in_grid]],
                                   uint tid [[thread_index_in_threadgroup]],
                                   uint lane [[thread_index_in_simdgroup]],
                                   uint simd_group [[simdgroup_index_in_threadgroup]]) {
    threadgroup uint scratch[32];
    const uint local_idx = tid & 31u;
    uint x = gid ^ (lane << 3) ^ (simd_group << 11);

    scratch[local_idx] = x + local_idx;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    x ^= scratch[(local_idx + 1u) & 31u];
    x += (lane + 1u) * 0x9e3779b9u;

    #pragma unroll 12
    for (uint i = 0; i < 12; ++i) {
        x ^= (x >> 7) ^ (tid + i * 13u);
        x = (x << 5) | (x >> 27);
        x += (simd_group + 1u) * (0x85ebca6bu ^ (i * 17u));
    }

    out[gid] = x ^ scratch[(local_idx + lane) & 31u];
}
