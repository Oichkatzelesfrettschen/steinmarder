__kernel void depchain_u16_add(__global float *out)
{
    const uint gid = get_global_id(0);
    ushort acc = (ushort)(((uint)(gid & 0xffffu) + 1u));

    for (int i = 0; i < 512; ++i) {
        acc = (ushort)(acc + (ushort)3);
    }

    out[gid] = convert_float(acc);
}
