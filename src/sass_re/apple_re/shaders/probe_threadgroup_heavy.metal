#include <metal_stdlib>

using namespace metal;

kernel void probe_simdgroup_reduce(device uint *out [[buffer(0)]],
                                   uint gid [[thread_position_in_grid]],
                                   uint tid [[thread_index_in_threadgroup]],
                                   uint lane [[thread_index_in_simdgroup]],
                                   uint simd_group [[simdgroup_index_in_threadgroup]]) {
    threadgroup uint scratch[256];
    const uint local_idx = tid & 255u;
    const uint seed = gid ^ (lane << 7) ^ (simd_group << 13);

    scratch[local_idx] = seed + local_idx;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint offset = 128; offset > 0; offset >>= 1) {
        if (local_idx < offset) {
            scratch[local_idx] += scratch[local_idx + offset];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    out[gid] = scratch[local_idx & 127u] ^ tid;
}
