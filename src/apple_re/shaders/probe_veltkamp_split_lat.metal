#include <metal_stdlib>
using namespace metal;

kernel void probe_veltkamp_split_lat(
    device uint* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    {
        #pragma clang fp reassociate(off)
        const float C_split = 8193.0f;  // 2^13 + 1

        float a_hi = float(gid + 1u) * 1.0e-4f + 1.0f;
        float a_lo = float(gid ^ 0xA5A5A5A5u) * 1.0e-10f;
        float step = float((gid * 2654435761u + 1u) | 1u) * 1.0e-9f + 1.000001f;

        for (uint outer = 0u; outer < 100000u; outer++) {
            // Step 0
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
            // Step 1
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
            // Step 2
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
            // Step 3
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
            // Step 4
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
            // Step 5
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
            // Step 6
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
            // Step 7
            {
                float gamma   = C_split * a_hi;
                float a_hi_hi = gamma - (gamma - a_hi);
                float a_hi_lo = a_hi - a_hi_hi;
                float prod_hi  = a_hi * step;
                float prod_err = fma(a_hi, step, -prod_hi);
                a_hi = prod_hi + a_hi_lo * step;
                a_lo = prod_err + a_lo * step;
            }
        }

        out[gid] = as_type<uint>(a_hi + a_lo);
    }
}
