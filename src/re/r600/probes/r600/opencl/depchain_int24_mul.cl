static inline int sign_extend24(int value)
{
    return (value << 8) >> 8;
}

__kernel void depchain_int24_mul(__global float *out)
{
    const uint gid = get_global_id(0);
    int acc = sign_extend24((int)(gid * 13u) - 0x00300000);

    for (int i = 0; i < 512; ++i) {
        acc = sign_extend24(mad24(acc, 3, -7));
    }

    out[gid] = convert_float(acc) * (1.0f / 8388608.0f);
}
