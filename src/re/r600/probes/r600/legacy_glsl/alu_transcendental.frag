// Probe: Transcendental slot (t-slot) throughput
// RSQ, RCP, SIN, COS, LOG, EXP - all must use t-slot
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    float a = v_coord.x + 0.001;
    for (int i = 0; i < 32; i++) {
        a = inversesqrt(a) * 1.001;  // RSQ on t-slot
    }
    fragColor = vec4(a);
}
