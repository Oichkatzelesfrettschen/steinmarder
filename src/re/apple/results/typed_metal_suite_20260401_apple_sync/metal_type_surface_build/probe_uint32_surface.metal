#include <metal_stdlib>
using namespace metal;

kernel void probe_uint32_surface(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    uint a = gid * 13u + 7u;
    uint b = gid ^ 0x55aa55aau;
    out[gid] = (a + b) ^ (a << 1);
}
