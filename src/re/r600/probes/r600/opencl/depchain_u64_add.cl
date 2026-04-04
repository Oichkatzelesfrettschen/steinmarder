__kernel void depchain_u64_add(__global float *out)
{
    const uint gid = get_global_id(0);
    ulong acc = (ulong)(((ulong)(gid & 0xfffffffffffffffful) + 1ul));

    for (int i = 0; i < 512; ++i) {
        acc = (ulong)(acc + (ulong)3ul);
    }

    out[gid] = convert_float(acc);
}
