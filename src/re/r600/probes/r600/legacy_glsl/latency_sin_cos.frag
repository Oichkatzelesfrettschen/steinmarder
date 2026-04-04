#version 130
// steinmarder latency probe: SIN/COS dependent chain (trans slot)
uniform float seed;
void main() {
    float v = seed + gl_FragCoord.x * 0.001;
    // 32-deep dependent chain alternating sin/cos
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    v = sin(v); v = cos(v); v = sin(v); v = cos(v);
    gl_FragColor = vec4(v, v, v, 1.0);
}
