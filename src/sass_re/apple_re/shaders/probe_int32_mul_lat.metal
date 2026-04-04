#include <metal_stdlib>

using namespace metal;

// INT32 dependent multiply chain probe.
// 32 dependent muls per iteration.
// Measures AGX INT32 multiply latency — likely distinct from add latency.
// Compare with probe_int64_mul_lat.metal.
// ops_per_iter = 32
kernel void probe_int32_mul_lat(device uint *out     [[buffer(0)]],
                                 constant uint &iters [[buffer(1)]],
                                 uint gid             [[thread_position_in_grid]]) {
    uint acc = gid + 1u;
    const uint mul_factor = 1664525u;  // LCG multiplier (odd, well-distributed)

    for (uint i = 0; i < iters; ++i) {
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
        acc *= mul_factor; acc *= mul_factor; acc *= mul_factor; acc *= mul_factor;
    }

    out[gid] = acc;
}
