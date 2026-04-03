#include <metal_stdlib>

using namespace metal;

// INT64 dependent multiply chain probe.
// 32 dependent muls per iteration.
// Metal MSL `long` multiply compiles to AIR `mul i64`.
// If AGX has no native 64-bit multiply, the backend expands to a multi-instruction
// sequence (e.g., 2× MUL32 + shift + add), which would show as >2× int32 latency.
// ops_per_iter = 32
kernel void probe_int64_mul_lat(device uint *out     [[buffer(0)]],
                                 constant uint &iters [[buffer(1)]],
                                 uint gid             [[thread_position_in_grid]]) {
    long acc = (long)(gid + 1u);
    const long mul_factor = 1664525L;

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

    out[gid] = uint(acc) ^ uint(acc >> 32);
}
