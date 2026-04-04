#include <metal_stdlib>
using namespace metal;

kernel void probe_int8_surface(device uint *out [[buffer(0)]],
                               uint gid [[thread_position_in_grid]]) {
    char a = char((gid & 0x7fu) - 64u);
    char b = char(((gid * 3u) & 0x7fu) - 32u);
    char c = char(a + b);
    out[gid] = uint(uchar(c));
}
