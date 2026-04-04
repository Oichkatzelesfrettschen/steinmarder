#include <metal_stdlib>
using namespace metal;

kernel void probe_float32_surface(device uint *out [[buffer(0)]],
                                  uint gid [[thread_position_in_grid]]) {
    float a = float((gid & 255u) + 1u) * 0.25f;
    float b = float(((gid * 17u) & 255u) + 3u) * 0.125f;
    float c = fma(a, 0.75f, b);
    out[gid] = as_type<uint>(c);
}
