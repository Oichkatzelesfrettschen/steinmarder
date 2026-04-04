#version 130
// steinmarder throughput probe: MUL_IEEE independent chains
// 4 independent chains → measures throughput (IPC), not latency
uniform float seed;
void main() {
    float a = seed + gl_FragCoord.x * 0.001;
    float b = seed + gl_FragCoord.y * 0.001;
    float c = a + 0.5;
    float d = b + 0.5;
    // 4 independent 16-deep chains (64 total ops, all independent)
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    a=a*1.0000001; b=b*1.0000001; c=c*1.0000001; d=d*1.0000001;
    gl_FragColor = vec4(a, b, c+d, 1.0);
}
