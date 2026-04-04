#include <metal_stdlib>
using namespace metal;

kernel void probe_imul_lat(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        uint acc = gid + 1u;
        uint offset = gid ^ 0xDEADBEEFu;
        uint step = (gid * 6364136223846793005u + 1442695040888963407u) | 1u;

        for (uint outer = 0u; outer < 100000u; outer++) {
            acc = acc * step + offset;
            acc = acc * step + offset;
            acc = acc * step + offset;
            acc = acc * step + offset;
            acc = acc * step + offset;
            acc = acc * step + offset;
            acc = acc * step + offset;
            acc = acc * step + offset;
        }

        out[gid] = acc;
    }
}
