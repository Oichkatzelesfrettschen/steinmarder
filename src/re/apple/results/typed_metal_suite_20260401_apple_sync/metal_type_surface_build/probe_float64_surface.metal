#include <metal_stdlib>
using namespace metal;

kernel void probe_float64_surface(device uint *out [[buffer(0)]],
                                  uint gid [[thread_position_in_grid]]) {
    double a = double((gid & 255u) + 1u) * 0.25;
    double b = double(((gid * 17u) & 255u) + 3u) * 0.125;
    double c = fma(a, 0.75, b);
    ulong bits = as_type<ulong>(c);
    out[gid] = uint(bits ^ (bits >> 32));
}
