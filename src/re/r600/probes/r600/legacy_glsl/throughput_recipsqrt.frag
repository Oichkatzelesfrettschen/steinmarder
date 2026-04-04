#version 130
// steinmarder throughput probe: RECIPSQRT independent chains (trans slot)
uniform float seed;
void main() {
    float a = abs(seed + gl_FragCoord.x * 0.001) + 1.0;
    float b = abs(seed + gl_FragCoord.y * 0.001) + 1.0;
    float c = a + 0.5;
    float d = b + 0.5;
    // 4 independent chains of inversesqrt (8 each = 32 total)
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    a=inversesqrt(a)+1.0; b=inversesqrt(b)+1.0; c=inversesqrt(c)+1.0; d=inversesqrt(d)+1.0;
    gl_FragColor = vec4(a, b, c+d, 1.0);
}
