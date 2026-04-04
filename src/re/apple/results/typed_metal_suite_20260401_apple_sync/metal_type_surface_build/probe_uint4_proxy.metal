#include <metal_stdlib>
using namespace metal;

inline float quantize_tf32_proxy(float x) {
    uint bits = as_type<uint>(x);
    bits &= 0xffffe000u;
    return as_type<float>(bits);
}

inline float quantize_minifloat_proxy(float x, uint exp_bits, uint mant_bits, int exp_bias) {
    if (x == 0.0f) {
        return 0.0f;
    }
    float sign = x < 0.0f ? -1.0f : 1.0f;
    float ax = fabs(x);
    int exponent = 0;
    float mant = frexp(ax, exponent);
    (void)mant;
    int normal_exp = exponent - 1;
    int min_normal = 1 - exp_bias;
    int max_normal = int((1u << exp_bits) - 2u) - exp_bias;
    if (normal_exp < min_normal) {
        return 0.0f;
    }
    if (normal_exp > max_normal) {
        normal_exp = max_normal;
    }
    float normalized = ldexp(ax, -normal_exp);
    float frac = clamp(normalized - 1.0f, 0.0f, 1.0f);
    float steps = float(1u << mant_bits);
    float qfrac = floor(frac * steps + 0.5f) / steps;
    qfrac = min(qfrac, (steps - 1.0f) / steps);
    return sign * ldexp(1.0f + qfrac, normal_exp);
}

inline float quantize_mxint8_proxy(float x, float scale) {
    float normalized = clamp(x / scale, -127.0f, 127.0f);
    float q = floor(normalized + (normalized >= 0.0f ? 0.5f : -0.5f));
    return q * scale;
}

inline float threadgroup_scale_proxy(uint gid,
                                     uint lid,
                                     uint tg_size,
                                     threadgroup float *scratch) {
    float base = float((gid & 255u) + 1u) * 0.0625f;
    float alt = float(((gid * 17u) & 255u) + 3u) * 0.03125f;
    float x = (float(int(gid & 1u) * 2 - 1) * base) + alt;
    scratch[lid] = fabs(x);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = tg_size >> 1; stride > 0; stride >>= 1) {
        if (lid < stride) {
            scratch[lid] = max(scratch[lid], scratch[lid + stride]);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    return max(scratch[0], 1.0f);
}kernel void probe_uint4_proxy(device uint *out [[buffer(0)]],
                              uint gid [[thread_position_in_grid]]) {
    uint raw = (gid * 7u) & 0x1fu;
    uint q = min(raw, 15u);
    out[gid] = q & 0xfu;
}
