#version 410 core

in vec2 uv;
out vec4 fragColor;

uniform sampler2D tex0;

void main() {
    vec2 stride0 = vec2(1.0 / 1024.0, 0.0);
    vec2 stride1 = vec2(0.0, 1.0 / 1024.0);
    vec4 s0 = texture(tex0, uv);
    vec4 s1 = texture(tex0, uv + stride0 * 7.0);
    vec4 s2 = texture(tex0, uv + stride1 * 11.0);
    vec4 s3 = texture(tex0, uv + stride0 * 13.0 + stride1 * 5.0);
    fragColor = (s0 + s1 + s2 + s3) * 0.25;
}
