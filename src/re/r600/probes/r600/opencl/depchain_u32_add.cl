__kernel void depchain_u32_add(__global float *out)
{
    const uint gid = get_global_id(0);
    uint acc = (uint)(((uint)(gid & 0xffffffffu) + 1u));

    for (int i = 0; i < 512; ++i) {
        acc = (uint)(acc + (uint)3);
    }

    out[gid] = convert_float(acc);
}
