#include <metal_stdlib>

using namespace metal;

kernel void probe_simdgroup_reduce(device uint *out [[buffer(0)]],
                                   uint gid [[thread_position_in_grid]],
                                   uint tid [[thread_index_in_threadgroup]],
                                   uint lane [[thread_index_in_simdgroup]],
                                   uint simd_group [[simdgroup_index_in_threadgroup]]) {
    uint value = gid ^ (lane << 8) ^ (simd_group << 16);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    out[gid] = value + tid;
}
