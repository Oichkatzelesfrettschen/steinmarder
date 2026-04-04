__kernel void depchain_uint24_mul(__global float *out)
{
    const uint gid = get_global_id(0);
    uint acc = (gid & 0x00fffffdu) + 3u;

    for (int i = 0; i < 512; ++i) {
        acc = mad24(acc, 3u, 7u);
    }

    out[gid] = convert_float(acc & 0x00ffffffu) * (1.0f / 16777216.0f);
}
