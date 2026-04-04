// Probe: Texture bandwidth (independent reads)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 v_texcoord;
uniform sampler2D tex0;
void main() {
    vec4 sum = vec4(0.0);
    // Independent reads - all can be issued in parallel
    for (int i = 0; i < 16; i++) {
        vec2 offset = vec2(float(i) * 0.0625, 0.0);
        sum += texture(tex0, v_texcoord + offset);
    }
    fragColor = sum / 16.0;
}
