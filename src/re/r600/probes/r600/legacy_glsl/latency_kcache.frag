// r600 TeraScale-2 Latency Probe: Constant Cache (kcache)
// steinmarder methodology: dependent UBO reads to measure kcache latency
//
// Strategy: Read from a UBO in a dependent chain — each read index
// depends on the previous value. The kcache on Evergreen holds constants
// fetched from memory; dependent reads measure the load-use latency.
//
// Expected: ~4 cycles if kcache is fast (similar to register read).

#version 140

uniform block {
    vec4 data[64];
} ubo;

varying vec2 v_texcoord;

void main()
{
    // Start with an index derived from fragment position
    int idx = int(mod(gl_FragCoord.x + gl_FragCoord.y, 64.0));
    vec4 accum = vec4(0.0);

    // 32-deep dependent UBO read chain
    // Each index depends on the previous read value
    vec4 v;

    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.w * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.w * 63.0, 64.0));

    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.w * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.w * 63.0, 64.0));

    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.w * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.w * 63.0, 64.0));

    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.w * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.x * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.y * 63.0, 64.0));
    v = ubo.data[idx]; accum += v; idx = int(mod(v.z * 63.0, 64.0));
    v = ubo.data[idx]; accum += v;

    gl_FragColor = accum / 32.0;
}
