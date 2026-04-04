#include <metal_stdlib>

using namespace metal;

// T6: AGX L1 global memory latency — pointer-chase within the output buffer.
//
// The output buffer is 1024 floats = 4 KB, which fits entirely in L1.
// Each thread's address depends on the previously loaded value, forming a
// serial dep chain that the compiler cannot hoist or cache.
//
// Using 'volatile device float*' prevents the compiler from promoting loads
// to registers.  The address mask keeps every access within the buffer.
//
// 8 dependent loads per outer iteration.
// ops_per_iter = 8

kernel void probe_global_l1_lat(device float   *out   [[buffer(0)]],
                                constant uint  &iters [[buffer(1)]],
                                uint gid [[thread_position_in_grid]]) {
    volatile device float *vbuf = (volatile device float *)out;
    const uint buf_mask = 1023u; // 1024 floats

    uint idx = gid & buf_mask;
    float acc = 0.0f;

    for (uint iter_idx = 0; iter_idx < iters; ++iter_idx) {
        float v;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
        v = vbuf[idx]; acc += v; idx = (idx + uint(acc)) & buf_mask;
    }

    out[gid] = acc;
}
