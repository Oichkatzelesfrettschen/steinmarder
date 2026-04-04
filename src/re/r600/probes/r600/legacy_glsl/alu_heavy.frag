// Probe: Pure ALU workload (no texture)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec4 a = v_coord;
    for (int i = 0; i < 128; i++) {
        a = a * vec4(0.999) + vec4(0.001);
    }
    fragColor = a;
}
