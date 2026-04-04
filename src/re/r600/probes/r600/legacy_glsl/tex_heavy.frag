// Probe: Pure texture workload (minimal ALU)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 v_texcoord;
uniform sampler2D tex0;
void main() {
    vec4 sum = vec4(0.0);
    for (int i = 0; i < 32; i++) {
        sum += texture(tex0, v_texcoord + vec2(float(i % 8) * 0.125, float(i / 8) * 0.25));
    }
    fragColor = sum / 32.0;
}
