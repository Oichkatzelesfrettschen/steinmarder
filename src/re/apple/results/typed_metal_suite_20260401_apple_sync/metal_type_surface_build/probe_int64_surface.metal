#include <metal_stdlib>
using namespace metal;

kernel void probe_int64_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    long a = long(gid) * 13l + 7l;
    long b = long(gid) ^ 0x55aa55aal;
    long c = (a + b) ^ (a << 1);
    ulong bits = as_type<ulong>(c);
    out[gid] = uint(bits ^ (bits >> 32));
}
