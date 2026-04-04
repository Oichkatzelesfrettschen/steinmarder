__kernel void depchain_i8_add(__global float *out)
{
    const uint gid = get_global_id(0);
    char acc = (char)(((int)(gid & 0x7f) - 31));

    for (int i = 0; i < 512; ++i) {
        acc = (char)(acc + (char)3);
    }

    out[gid] = (float)acc;
}
