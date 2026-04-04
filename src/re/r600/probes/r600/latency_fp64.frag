// r600 TeraScale-2 Latency Probe: FP64 (Native Double Precision)
// steinmarder methodology: dependent double-precision chain
//
// The Evergreen ISA has native FP64: ADD_64, MUL_64, FMA_64.
// Each uses 2 ALU slots (vec pair), so theoretical latency is 2 cycles.
// PV forwarding may still apply.
//
// This probe measures: does the hardware pipeline FP64 ops at 2 cycles,
// or is there additional stall overhead beyond the 2-slot consumption?
//
// Requires GLSL 1.50+ (GL 3.2) or ARB_gpu_shader_fp64.

#version 150
#extension GL_ARB_gpu_shader_fp64 : enable

out vec4 fragColor;

void main()
{
    // Seed from fragment position to prevent constant folding
    double x = double(gl_FragCoord.x) * 0.001;
    double y = double(gl_FragCoord.y) * 0.001;

    // 32-deep dependent FP64 multiply chain
    // Each multiply depends on the previous result.
    // MUL_64 uses 2 vec slots per op. With PV forwarding,
    // expected latency is 2 cycles per multiply.
    double v = x + y;

    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;

    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;

    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;

    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;
    v = v * 1.0000001LF; v = v * 0.9999999LF;

    // Also test FP64 ADD chain (might have different latency)
    double a = v;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;

    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;

    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;

    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;
    a = a + 0.0000001LF; a = a + 0.0000001LF;

    // Output as FP32 (truncate double to float for framebuffer)
    fragColor = vec4(float(v), float(a), 0.0, 1.0);
}
