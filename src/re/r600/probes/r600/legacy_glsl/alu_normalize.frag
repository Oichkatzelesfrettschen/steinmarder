// Probe: normalize() pattern - DOT3 + RSQ + 3xMUL
// This is the key bottleneck identified in ISA analysis
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec3 a = v_coord.xyz;
    for (int i = 0; i < 32; i++) {
        a = normalize(a + vec3(0.001));
    }
    fragColor = vec4(a, 1.0);
}
