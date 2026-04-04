#include <metal_stdlib>
using namespace metal;

kernel void probe_uint64_surface(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    ulong a = ulong(gid) * 13ul + 7ul;
    ulong b = ulong(gid) ^ 0x55aa55aaul;
    ulong c = (a + b) ^ (a << 1);
    out[gid] = uint(c ^ (c >> 32));
}
