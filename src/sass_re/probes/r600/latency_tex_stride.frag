// r600 TeraScale-2 Latency Probe: TEX L1 Cache Miss (Stride Sweep)
// steinmarder methodology: dependent texture reads with strided coordinates
//
// Strategy: Each texture read depends on the previous result, but the
// coordinate jumps by a large stride. This forces L1 cache misses on
// every read (if stride > L1 line size).
//
// Compare against latency_tex_l1_hit.frag to isolate L1 miss penalty.
// Expected: 100-400 cycles (L1 miss → L2/DRAM fetch).

#version 130

uniform sampler2D tex;
varying vec2 v_texcoord;

void main()
{
    vec2 coord = v_texcoord;
    vec4 accum = vec4(0.0);

    // 32-deep dependent texture read chain with large strides
    // The stride 0.03125 (1/32) on a 256x256 texture = 8 texels,
    // which should miss L1 if the cache line is < 8 texels.
    // Each read's coordinate depends on the previous result.
    vec4 t;
    float stride = 0.03125;

    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);

    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);

    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);

    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.z * stride + 0.5, t.w * stride + 0.5);
    t = texture2D(tex, coord); accum += t; coord = vec2(t.x * stride + 0.5, t.y * stride + 0.5);
    t = texture2D(tex, coord); accum += t;

    gl_FragColor = accum / 32.0;
}
