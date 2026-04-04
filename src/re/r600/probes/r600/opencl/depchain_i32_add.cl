__kernel void depchain_i32_add(__global float *out)
{
    const uint gid = get_global_id(0);
    int acc = (int)(((int)(gid & 0x7fffffff) - 31));

    for (int i = 0; i < 512; ++i) {
        acc = (int)(acc + (int)3);
    }

    out[gid] = (float)acc;
}
