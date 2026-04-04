// Probe: MULADD_IEEE (FMA) throughput
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec4 a = v_coord;
    vec4 b = vec4(0.01);
    for (int i = 0; i < 64; i++) {
        a = a * vec4(0.99) + b;
    }
    fragColor = a;
}
