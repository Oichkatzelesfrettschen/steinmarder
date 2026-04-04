// Probe: Scalar operations (only uses x-slot, wastes y/z/w/t)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    float a = v_coord.x;
    for (int i = 0; i < 64; i++) {
        a = a * 0.99 + 0.01;
    }
    fragColor = vec4(a, a, a, 1.0);
}
