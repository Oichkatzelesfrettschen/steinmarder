#include <metal_stdlib>

using namespace metal;

kernel void probe_simdgroup_reduce(device uint *out [[buffer(0)]],
                                   uint gid [[thread_position_in_grid]],
                                   uint tid [[thread_index_in_threadgroup]],
                                   uint lane [[thread_index_in_simdgroup]],
                                   uint simd_group [[simdgroup_index_in_threadgroup]]) {
    uint value = gid;
    uint accum0 = value ^ 0x9e3779b9u;
    uint accum1 = (lane << 4) | simd_group;
    uint accum2 = tid | (gid << 1);
    for (uint i = 0; i < 24; ++i) {
        accum0 ^= (accum1 + i) * 0x85ebca6bu;
        accum1 += (accum2 ^ (i << 2));
        accum2 = (accum0 << 5) | (accum2 >> 3);
        value += accum0 ^ accum1;
    }
    float fuse = float(accum0) * 0.0000001f;
    accum0 += uint((fuse + float(accum2)) * 1.41421356f);
    out[gid] = accum0 + accum1 + accum2;
}
