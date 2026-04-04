#version 130
// steinmarder latency probe: RECIPSQRT_IEEE dependent chain (trans slot)
uniform float seed;
void main() {
    float v = abs(seed + gl_FragCoord.x * 0.001) + 1.0;
    // 32-deep dependent chain of inversesqrt (RECIPSQRT_IEEE, t-slot only)
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0; v = inversesqrt(v) + 1.0;
    gl_FragColor = vec4(v, v, v, 1.0);
}
