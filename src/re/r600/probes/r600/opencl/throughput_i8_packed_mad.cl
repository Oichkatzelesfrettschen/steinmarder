__kernel void throughput_i8_packed_mad(__global float4 *out)
{
    const int gid = (int)get_global_id(0);
    int packed = gid * 0x00010203 + 0x10203040;
    const int4 a = (int4)(3, -5, 7, -11);
    const int4 b = (int4)(1, -2, 3, -4);

    for (int i = 0; i < 256; ++i) {
        int4 lanes = convert_int4(as_char4(packed));
        lanes = clamp(mad24(lanes, a, b), (int4)(-128), (int4)(127));
        packed = as_int(convert_char4(lanes));
    }

    out[gid] = convert_float4(convert_int4(as_char4(packed))) * (1.0f / 128.0f);
}
