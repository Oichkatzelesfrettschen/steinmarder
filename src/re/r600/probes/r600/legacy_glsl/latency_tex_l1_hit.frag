// r600 TeraScale-2 Latency Probe: TEX L1 Cache Hit
// steinmarder methodology: dependent texture reads from same coordinate
//
// Strategy: Read the same texcoord N times in a dependent chain.
// Each read depends on the previous result (add small offset from previous
// color value). Since the same texel is accessed repeatedly, all reads
// after the first should hit the L1 texture cache.
//
// Compare frametime of this shader vs a no-TEX baseline to measure
// TEX L1 hit latency.

#version 130

uniform sampler2D tex;
varying vec2 v_texcoord;

void main()
{
    vec2 coord = v_texcoord;
    vec4 accum = vec4(0.0);

    // 32-deep dependent texture read chain (same texel, L1 hot)
    // Each read adds a tiny offset from the previous result to create
    // a true data dependency, but the offset is small enough that
    // the texture coordinate stays on the same texel.
    vec4 t;

    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;

    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;

    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;

    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t; coord += t.xy * 0.00001;
    t = texture2D(tex, coord); accum += t;

    gl_FragColor = accum / 32.0;
}
