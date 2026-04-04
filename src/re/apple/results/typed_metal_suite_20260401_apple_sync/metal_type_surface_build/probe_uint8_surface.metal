#include <metal_stdlib>
using namespace metal;

kernel void probe_uint8_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    uchar a = uchar(gid & 0xffu);
    uchar b = uchar((gid * 7u) & 0xffu);
    uchar c = uchar(a + b);
    out[gid] = uint(c);
}
