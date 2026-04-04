#version 130
// steinmarder latency probe: MULLO_INT dependent chain (trans-only, expected multi-cycle)
uniform int iseed;
void main() {
    int v = iseed + int(gl_FragCoord.x);
    // 32-deep dependent chain of integer multiply (MULLO_INT → trans slot)
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    v = v * 3; v = v * 3; v = v * 3; v = v * 3;
    float f = float(v) * 0.0000001;
    gl_FragColor = vec4(f, f, f, 1.0);
}
