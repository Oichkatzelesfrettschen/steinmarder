// Probe: SIN/COS t-slot throughput
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    float a = v_coord.x;
    for (int i = 0; i < 32; i++) {
        a = sin(a) + 0.001;
    }
    fragColor = vec4(a);
}
