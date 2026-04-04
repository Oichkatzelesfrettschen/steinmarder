// Probe: Full vec4 operations (should fill all 4 general slots)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec4 a = v_coord;
    for (int i = 0; i < 64; i++) {
        a = a * vec4(0.99, 0.98, 0.97, 0.96) + vec4(0.01, 0.02, 0.03, 0.04);
    }
    fragColor = a;
}
