// Probe: GPR pressure with 4 live vec4 registers
// More GPRs = fewer wavefronts = less latency hiding
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec4 r[4];
    r[0] = v_coord;
    for (int i = 1; i < 4; i++) {
        r[i] = r[i-1] * vec4(0.99) + vec4(0.01);
    }
    vec4 sum = vec4(0.0);
    for (int i = 0; i < 4; i++) {
        sum += r[i];
    }
    fragColor = sum / float(4);
}
