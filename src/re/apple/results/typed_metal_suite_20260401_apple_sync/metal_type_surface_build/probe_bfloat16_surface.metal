#include <metal_stdlib>
using namespace metal;

kernel void probe_bfloat16_surface(device uint *out [[buffer(0)]],
                                   uint gid [[thread_position_in_grid]]) {
    bfloat a = bfloat(float((gid & 255u) + 1u) * 0.25f);
    bfloat b = bfloat(float(((gid * 9u) & 255u) + 5u) * 0.125f);
    bfloat c = a * bfloat(0.5f) + b;
    out[gid] = as_type<uint>(float(c));
}
