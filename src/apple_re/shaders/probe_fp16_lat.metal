#include <metal_stdlib>
using namespace metal;

kernel void probe_fadd16_lat(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        half acc   = half(gid + 1u) * 0.0001h + 1.0h;
        half delta = half((gid ^ 0x55u) + 1u) * 0.0001h + 0.0001h;

        for (uint outer = 0u; outer < 100000u; outer++) {
            acc += delta;
            acc += delta;
            acc += delta;
            acc += delta;
            acc += delta;
            acc += delta;
            acc += delta;
            acc += delta;
        }

        out[gid] = as_type<uint>(float(acc));
    }
}

kernel void probe_fmul16_lat(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        half acc  = half(gid + 1u) * 0.0001h + 1.0h;
        half step = half((gid ^ 0x33u) + 1u) * 0.00001h + 1.0001h;

        for (uint outer = 0u; outer < 100000u; outer++) {
            acc *= step;
            acc *= step;
            acc *= step;
            acc *= step;
            acc *= step;
            acc *= step;
            acc *= step;
            acc *= step;
        }

        out[gid] = as_type<uint>(float(acc));
    }
}

kernel void probe_fma16_lat(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        half acc   = half(gid + 1u) * 0.0001h + 1.0h;
        half step  = half((gid ^ 0x33u) + 1u) * 0.00001h + 1.0001h;
        half delta = half((gid ^ 0x55u) + 1u) * 0.0001h + 0.0001h;

        for (uint outer = 0u; outer < 100000u; outer++) {
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
        }

        out[gid] = as_type<uint>(float(acc));
    }
}
