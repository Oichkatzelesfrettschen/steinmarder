#version 130
// steinmarder latency probe: DOT4_IEEE dependent chain (multi-slot)
uniform float seed;
void main() {
    vec4 v = vec4(seed + gl_FragCoord.x * 0.001, 0.5, 0.5, 0.5);
    vec4 k = vec4(1.0000001, 0.9999999, 1.0000001, 0.9999999);
    // 32-deep dependent chain of dot products
    float d;
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    d = dot(v, k); v = vec4(d, d, d, d);
    gl_FragColor = vec4(d, d, d, 1.0);
}
