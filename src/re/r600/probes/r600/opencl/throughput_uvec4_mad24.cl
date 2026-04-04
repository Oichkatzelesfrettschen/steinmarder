__kernel void throughput_uvec4_mad24(__global float4 *out)
{
    const uint gid = get_global_id(0);
    uint4 acc = ((uint4)(gid, gid + 1u, gid + 2u, gid + 3u) * 257u) & (uint4)(0x00ffffffu);
    const uint4 a = (uint4)(3u, 5u, 7u, 11u);
    const uint4 b = (uint4)(17u, 19u, 23u, 29u);

    for (int i = 0; i < 256; ++i) {
        acc = mad24(acc, a, b) & (uint4)(0x00ffffffu);
    }

    out[gid] = convert_float4(acc) * (1.0f / 16777216.0f);
}
