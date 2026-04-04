static inline int4 sign_extend24_int4(int4 value)
{
    return (value << 8) >> 8;
}

__kernel void throughput_ivec4_mad24(__global float4 *out)
{
    const int gid = (int)get_global_id(0);
    int4 acc = sign_extend24_int4((int4)(gid, gid + 1, gid + 2, gid + 3) * 257 - 0x00300000);
    const int4 a = (int4)(3, 5, 7, 11);
    const int4 b = (int4)(-17, 19, -23, 29);

    for (int i = 0; i < 256; ++i) {
        acc = sign_extend24_int4(mad24(acc, a, b));
    }

    out[gid] = convert_float4(acc) * (1.0f / 8388608.0f);
}
