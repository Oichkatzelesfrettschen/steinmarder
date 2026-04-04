#version 410 core

in vec2 uv;
out vec4 fragColor;

void main() {
    ivec2 p = ivec2(uv * 255.0);
    int qx = (p.x & 15) - 8;
    int qy = (p.y & 15) - 8;
    int mix0 = qx * 7 + qy * 13;
    int mix1 = (mix0 << 2) ^ (mix0 >> 1) ^ 0x55;
    float v = float(mix1 & 255) / 255.0;
    fragColor = vec4(v, fract(v * 1.7), fract(v * 2.3), 1.0);
}
