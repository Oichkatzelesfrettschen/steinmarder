#include <metal_stdlib>
using namespace metal;

kernel void probe_float16_surface(device uint *out [[buffer(0)]],
                                  uint gid [[thread_position_in_grid]]) {
    half a = half(float((gid & 127u) + 1u) * 0.5f);
    half b = half(float(((gid * 5u) & 127u) + 3u) * 0.25f);
    half c = a * half(0.75f) + b;
    out[gid] = as_type<uint>(float(c));
}
