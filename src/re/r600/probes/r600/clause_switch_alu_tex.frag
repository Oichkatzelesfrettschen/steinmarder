// r600 TeraScale-2 Latency Probe: ALU→TEX Clause Switching Overhead
// steinmarder methodology: interleaved ALU and TEX operations
//
// Evergreen executes ALU and TEX in separate clauses. Switching between
// them incurs a clause switch penalty. This probe forces alternating
// ALU→TEX→ALU→TEX transitions to measure the per-switch overhead.
//
// Compare:
//   A) All ALU then all TEX (2 clause switches total)
//   B) Interleaved ALU-TEX-ALU-TEX (32 clause switches)
// Difference / 30 = per-clause-switch overhead
//
// Expected: ~10 cycles per switch?

#version 130

uniform sampler2D tex;
varying vec2 v_texcoord;

void main()
{
    vec2 coord = v_texcoord;
    vec4 accum = vec4(0.0);
    vec4 t;

    // Interleaved: 1 ALU op, then 1 TEX op, repeat.
    // Each TEX depends on the ALU result, forcing a clause switch.

    // Round 1: ALU then TEX
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 2
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 3
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 4
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 5
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 6
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 7
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 8
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001);
    accum += t;

    // Round 9-16
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;
    accum = accum * 1.001 + vec4(0.001);
    t = texture2D(tex, coord + accum.xy * 0.00001); accum += t;

    gl_FragColor = accum / 16.0;
}
