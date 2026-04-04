__kernel void depchain_add(__global float *out)
{
    const uint gid = get_global_id(0);
    float acc = (float)gid * 0.125f;

    for (int i = 0; i < 512; ++i) {
        acc = acc + 1.0f;
    }

    out[gid] = acc;
}
