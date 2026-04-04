__kernel void depchain_i16_add(__global float *out)
{
    const uint gid = get_global_id(0);
    short acc = (short)(((int)(gid & 0x7fff) - 31));

    for (int i = 0; i < 512; ++i) {
        acc = (short)(acc + (short)3);
    }

    out[gid] = (float)acc;
}
