#include <metal_stdlib>

using namespace metal;

// INT64 dependent add chain probe.
// 32 dependent adds per iteration — each result feeds the next.
// Measures AGX INT64 add latency (cycles/op).
// If AGX lowers i64 to i32 pairs, this should be ~2× slower than probe_int32_lat.
// If AGX has native 64-bit integer ALU, latency should match probe_int32_lat.
// ops_per_iter = 32
kernel void probe_int64_lat(device uint *out     [[buffer(0)]],
                             constant uint &iters [[buffer(1)]],
                             uint gid             [[thread_position_in_grid]]) {
    long acc = (long)(gid + 1u);
    const long step = 2654435761L;

    for (uint i = 0; i < iters; ++i) {
        acc += step; acc += step; acc += step; acc += step;
        acc += step; acc += step; acc += step; acc += step;
        acc += step; acc += step; acc += step; acc += step;
        acc += step; acc += step; acc += step; acc += step;
        acc += step; acc += step; acc += step; acc += step;
        acc += step; acc += step; acc += step; acc += step;
        acc += step; acc += step; acc += step; acc += step;
        acc += step; acc += step; acc += step; acc += step;
    }

    // Fold 64-bit result to 32-bit to stay compatible with host harness (uint output)
    out[gid] = uint(acc) ^ uint(acc >> 32);
}
