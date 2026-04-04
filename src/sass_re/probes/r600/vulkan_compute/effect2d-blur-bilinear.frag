#version 420 core

// Optimized blur shader: 2D bilinear merge of 5x3 box filter
// Original: 15 texture taps, 252 dwords, 18 GPRs, 81 ALU, 70.4% VLIW util
// Optimized: 6 bilinear taps, 142 dwords, 6 GPRs, 39 ALU, 65.0% VLIW util
// Result: blur FPS 414 -> 910 (+120%), composite vkmark 791 -> 906 (+14.5%)
//
// Technique: bilinear HW interpolation at half-texel offsets merges adjacent samples.
// For a box filter (all weights equal), sampling at the midpoint between 2 texels
// returns their average. Sampling at the center of a 2x2 block returns the average
// of all 4 texels. This replaces ALU multiply-accumulate with fixed-function TEX HW.
//
// Deploy: glslangValidator -V effect2d-blur-bilinear.frag -o effect2d-blur.frag.spv
//         sudo cp effect2d-blur.frag.spv /usr/share/vkmark/shaders/effect2d-blur.frag.spv

layout(std140, binding = 0) uniform block {
    float TextureStepX;
    float TextureStepY;
};

layout(binding = 1) uniform sampler2D Texture0;

layout(location = 0) in vec2 in_texcoord;

layout(location = 0) out vec4 frag_color;

void main(void)
{
    // 2D bilinear merge of 5x3 box filter: 15 taps -> 6 bilinear taps
    //
    // Original grid (5 columns x 3 rows, all weights = 1/15):
    //   (-2,-1) (-1,-1) (0,-1) (1,-1) (2,-1)
    //   (-2, 0) (-1, 0) (0, 0) (1, 0) (2, 0)
    //   (-2,+1) (-1,+1) (0,+1) (1,+1) (2,+1)
    //
    // Column merge: (-2,-1) with (-1,*) -> sample at x = -1.5*TSX
    //               (1,*) with (2,*) -> sample at x = +1.5*TSX
    //               (0,*) stays at x = 0
    //
    // Row merge: y=-1 with y=0 -> sample at y = -0.5*TSY
    //            y=+1 stays at y = +1.0*TSY
    //
    // Level A (merged rows y=-1, y=0):
    //   tap0: (-1.5, -0.5) covers 4 texels, weight 4
    //   tap1: ( 0.0, -0.5) covers 2 texels, weight 2
    //   tap2: (+1.5, -0.5) covers 4 texels, weight 4
    //
    // Level B (row y=+1 alone):
    //   tap3: (-1.5, +1.0) covers 2 texels, weight 2
    //   tap4: ( 0.0, +1.0) covers 1 texel,  weight 1
    //   tap5: (+1.5, +1.0) covers 2 texels, weight 2
    //
    // Total texels: 4+2+4+2+1+2 = 15 (exact)

    float yA = -0.5 * TextureStepY;
    float yB =  1.0 * TextureStepY;
    float xL = -1.5 * TextureStepX;
    float xR =  1.5 * TextureStepX;

    vec3 sum = vec3(0.0);

    // Level A: 2-row merge (y=-1, y=0)
    sum += texture(Texture0, in_texcoord + vec2(xL, yA)).rgb * 4.0;
    sum += texture(Texture0, in_texcoord + vec2(0.0, yA)).rgb * 2.0;
    sum += texture(Texture0, in_texcoord + vec2(xR, yA)).rgb * 4.0;

    // Level B: row y=+1
    sum += texture(Texture0, in_texcoord + vec2(xL, yB)).rgb * 2.0;
    sum += texture(Texture0, in_texcoord + vec2(0.0, yB)).rgb;
    sum += texture(Texture0, in_texcoord + vec2(xR, yB)).rgb * 2.0;

    frag_color = vec4(sum / 15.0, 1.0);
}
