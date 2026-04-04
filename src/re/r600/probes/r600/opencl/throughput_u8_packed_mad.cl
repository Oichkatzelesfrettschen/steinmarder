__kernel void throughput_u8_packed_mad(__global float4 *out)
{
    const uint gid = get_global_id(0);
    uint packed = gid * 0x01020408u + 0x11223344u;
    const uint4 a = (uint4)(3u, 5u, 7u, 11u);
    const uint4 b = (uint4)(1u, 2u, 3u, 4u);

    for (int i = 0; i < 256; ++i) {
        uint4 lanes = convert_uint4(as_uchar4(packed));
        lanes = mad24(lanes, a, b) & (uint4)(0xffu);
        packed = as_uint(convert_uchar4(lanes));
    }

    out[gid] = convert_float4(convert_uint4(as_uchar4(packed))) * (1.0f / 255.0f);
}
