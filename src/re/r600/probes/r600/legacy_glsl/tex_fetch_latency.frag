// Probe: Texture fetch latency (dependent reads)
#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 v_texcoord;
uniform sampler2D tex0;
void main() {
    vec4 c = texture(tex0, v_texcoord);
    // Dependent texture reads - each read depends on previous result
    for (int i = 0; i < 8; i++) {
        c = texture(tex0, c.xy * 0.5 + 0.25);
    }
    fragColor = c;
}
