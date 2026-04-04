__kernel void depchain_i64_add(__global float *out)
{
    const uint gid = get_global_id(0);
    long acc = (long)(((long)(gid & 0x7fffffffffffffffl) - 31l));

    for (int i = 0; i < 512; ++i) {
        acc = (long)(acc + (long)3ul);
    }

    out[gid] = (float)acc;
}
