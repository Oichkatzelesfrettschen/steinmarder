__kernel void depchain_mul(__global float *out)
{
    const uint gid = get_global_id(0);
    float acc = 1.001f + (float)gid * 0.0001f;

    for (int i = 0; i < 512; ++i) {
        acc = acc * 1.0009765625f;
    }

    out[gid] = acc;
}
