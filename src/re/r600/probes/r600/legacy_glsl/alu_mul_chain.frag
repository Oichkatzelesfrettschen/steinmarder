// Probe: MUL_IEEE throughput - long dependency chain
// Expected: 1 MUL per cycle per slot, 5 slots = 5 MUL/cycle
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v_coord;
void main() {
    vec4 a = v_coord;
    // 64 chained multiplies to measure throughput
    for (int i = 0; i < 64; i++) {
        a = a * vec4(0.99, 0.99, 0.99, 0.99);
    }
    fragColor = a;
}
