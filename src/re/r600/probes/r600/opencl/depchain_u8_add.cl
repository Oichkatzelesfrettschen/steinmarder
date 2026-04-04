__kernel void depchain_u8_add(__global float *out)
{
    const uint gid = get_global_id(0);
    uchar acc = (uchar)(((uint)(gid & 0xffu) + 1u));

    for (int i = 0; i < 512; ++i) {
        acc = (uchar)(acc + (uchar)3);
    }

    out[gid] = convert_float(acc);
}
