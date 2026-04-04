// Probe: Mixed vec4 + transcendental (should fill all 5 VLIW5 slots)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec4 a = v_coord + vec4(0.001);
    for (int i = 0; i < 32; i++) {
        // vec4 MAD fills x/y/z/w, inversesqrt fills t-slot
        float rsq = inversesqrt(a.x * a.x + a.y * a.y + a.z * a.z);
        a = a * vec4(rsq * 0.99) + vec4(0.01);
    }
    fragColor = a;
}
