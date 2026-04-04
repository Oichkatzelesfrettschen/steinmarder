// Probe: DOT product throughput (uses x+y+z slots)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec3 a = v_coord.xyz;
    vec3 b = vec3(0.577);
    float d = 0.0;
    for (int i = 0; i < 64; i++) {
        d += dot(a, b);
        a = a * 0.99 + vec3(d * 0.001);
    }
    fragColor = vec4(d);
}
