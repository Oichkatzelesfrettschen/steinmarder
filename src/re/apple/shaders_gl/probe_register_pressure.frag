#version 410 core

in vec2 uv;
out vec4 fragColor;

void main() {
    vec4 acc0 = vec4(uv, uv.x + uv.y, uv.x - uv.y);
    vec4 acc1 = acc0.yzwx * 1.173 + vec4(0.1, 0.2, 0.3, 0.4);
    vec4 acc2 = acc1.zwxy * 0.947 + acc0 * 0.713;
    vec4 acc3 = acc2.wxyz * 1.061 + acc1 * 0.511;
    vec4 acc4 = acc3 + acc2 * 0.333 + acc1 * 0.125;
    vec4 acc5 = sin(acc4 * 3.14159) + cos(acc3 * 1.61803);
    vec4 acc6 = mix(acc5, acc4, 0.37) + sqrt(abs(acc2) + 0.001);
    fragColor = fract(acc6);
}
