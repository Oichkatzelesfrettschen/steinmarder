#include <metal_stdlib>
using namespace metal;

kernel void probe_int32_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    int a = int(gid) * 13 + 7;
    int b = int(gid) ^ 0x55aa55aa;
    int c = (a + b) ^ (a << 1);
    out[gid] = as_type<uint>(c);
}
