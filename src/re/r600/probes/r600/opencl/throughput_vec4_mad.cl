__kernel void throughput_vec4_mad(__global float4 *out)
{
    const uint gid = get_global_id(0);
    float4 acc = (float4)(1.0f, 2.0f, 3.0f, 4.0f) + (float)gid * 0.001f;
    const float4 a = (float4)(1.0001f, 0.9999f, 1.0002f, 0.9998f);
    const float4 b = (float4)(0.5f, 0.25f, 0.125f, 0.0625f);

    for (int i = 0; i < 256; ++i) {
        acc = mad(acc, a, b);
    }

    out[gid] = acc;
}
