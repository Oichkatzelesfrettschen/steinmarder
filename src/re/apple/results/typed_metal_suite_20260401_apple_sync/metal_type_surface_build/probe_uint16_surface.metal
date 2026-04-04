#include <metal_stdlib>
using namespace metal;

kernel void probe_uint16_surface(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    ushort a = ushort(gid & 0xffffu);
    ushort b = ushort((gid * 7u) & 0xffffu);
    ushort c = ushort(a + b);
    out[gid] = uint(c);
}
