#include <metal_stdlib>

using namespace metal;

// INT32 dependent add chain probe.
// 32 dependent adds per iteration — each result feeds the next.
// Measures AGX INT32 add latency (cycles/op).
// Compare with probe_int64_lat.metal to determine if AGX has native 64-bit integer ALU.
// ops_per_iter = 32
kernel void probe_int32_lat(device uint *out     [[buffer(0)]],
                             constant uint &iters [[buffer(1)]],
                             uint gid             [[thread_position_in_grid]]) {
    uint acc = gid + 1u;
    const uint step = 2654435761u;  // Knuth multiplicative hash constant (odd, no even factors)

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

    out[gid] = acc;
}
