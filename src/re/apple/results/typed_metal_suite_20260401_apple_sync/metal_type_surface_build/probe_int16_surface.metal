#include <metal_stdlib>
using namespace metal;

kernel void probe_int16_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    short a = short((gid & 0x7fffu) - 1024u);
    short b = short(((gid * 5u) & 0x7fffu) - 256u);
    short c = short(a + b);
    out[gid] = uint(ushort(c));
}
