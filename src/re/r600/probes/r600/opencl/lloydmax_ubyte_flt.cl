/*
 * lloydmax_ubyte_flt.cl — INT8 Lloyd-Max quantized vector distance
 *
 * Exploits TeraScale-2's UBYTE[0-3]_FLT instruction: a single-cycle op that
 * extracts one byte from a DWORD and converts it to float. This effectively
 * gives us 4x throughput on INT8 data compared to scalar float loads.
 *
 * The kernel implements a simplified Lloyd-Max distance computation:
 *   1. Load packed INT8x4 weights from a codebook (1 DWORD = 4 weights)
 *   2. Unpack via UBYTE_FLT (4 ops, 1 cycle each, all in vec slots)
 *   3. Scale to [0,1] range (multiply by 1/255)
 *   4. Compute dot product with float activations
 *
 * Expected ISA (per 4-element block):
 *   Bundle 0: UBYTE0_FLT R0.x, UBYTE1_FLT R0.y, UBYTE2_FLT R0.z, UBYTE3_FLT R0.w
 *             (4 vec slots, 1 cycle — ALL 4 bytes unpacked simultaneously)
 *   Bundle 1: MUL_IEEE R0.x *= 1/255, R0.y *= 1/255, R0.z *= 1/255, R0.w *= 1/255
 *   Bundle 2: DOT4_IEEE result += R0 . activations
 *
 * Compare to scalar float path: 4 loads + 4 multiplies + 3 adds = 11 ops
 * UBYTE_FLT path: 4 unpack + 4 scale + 1 DOT4 = 9 ops, but in only 3 bundles
 *
 * Build: RUSTICL_ENABLE=r600 clang -cl-std=CL1.2 lloydmax_ubyte_flt.cl
 * Run:   RUSTICL_ENABLE=r600 R600_DEBUG=cs ./lloydmax_test
 */

/* Codebook decode: unpack 4 INT8 values from a UINT32 and convert to float.
 * The GLSL equivalent is unpackUnorm4x8() which NIR lowers to:
 *   extract_u8(packed, 0) -> u2f32 -> UBYTE0_FLT
 *   extract_u8(packed, 1) -> u2f32 -> UBYTE1_FLT
 *   extract_u8(packed, 2) -> u2f32 -> UBYTE2_FLT
 *   extract_u8(packed, 3) -> u2f32 -> UBYTE3_FLT
 */

__kernel void lloydmax_ubyte_dot(
    __global const uint *packed_weights,   /* INT8x4 packed codebook vectors */
    __global const float4 *activations,    /* FP32 activation vectors */
    __global float *results,               /* FP32 dot product results */
    const float inv_scale,                 /* 1.0f/255.0f for normalization */
    const uint vector_len_div4             /* number of packed DWORDs per vector */
)
{
    uint gid = get_global_id(0);
    float acc = 0.0f;

    uint base_w = gid * vector_len_div4;

    for (uint i = 0; i < vector_len_div4; i++) {
        uint packed = packed_weights[base_w + i];

        /* These 4 extractions + u2f should emit UBYTE[0-3]_FLT */
        float w0 = (float)(packed & 0xFFu) * inv_scale;
        float w1 = (float)((packed >> 8) & 0xFFu) * inv_scale;
        float w2 = (float)((packed >> 16) & 0xFFu) * inv_scale;
        float w3 = (float)((packed >> 24) & 0xFFu) * inv_scale;

        float4 act = activations[gid * vector_len_div4 + i];

        /* This should emit DOT4_IEEE in a single VLIW bundle */
        acc += w0 * act.x + w1 * act.y + w2 * act.z + w3 * act.w;
    }

    results[gid] = acc;
}

/* Variant 2: Pure UBYTE throughput test (no dot product, just unpack + store).
 * Measures raw UBYTE_FLT throughput without ALU bottleneck. */
__kernel void ubyte_unpack_throughput(
    __global const uint *packed_input,
    __global float4 *unpacked_output,
    const uint count
)
{
    uint gid = get_global_id(0);
    if (gid >= count) return;

    uint packed = packed_input[gid];

    /* 4x UBYTE_FLT: should pack into a single VLIW bundle
     * Each fills one vec slot (x,y,z,w) */
    float4 result;
    result.x = (float)(packed & 0xFFu);
    result.y = (float)((packed >> 8) & 0xFFu);
    result.z = (float)((packed >> 16) & 0xFFu);
    result.w = (float)((packed >> 24) & 0xFFu);

    unpacked_output[gid] = result;
}

/* Variant 3: Codebook lookup — classic Lloyd-Max vector quantization.
 * Given a query vector and a codebook of packed INT8 centroids,
 * find the nearest centroid by minimum L2 distance. */
__kernel void lloydmax_nearest_centroid(
    __global const float4 *query,          /* FP32 query vectors */
    __global const uint *codebook,         /* INT8x4 packed centroids */
    __global uint *nearest_idx,            /* output: index of nearest centroid */
    const uint num_centroids,              /* codebook size */
    const float inv_scale                  /* 1/255 */
)
{
    uint gid = get_global_id(0);
    float4 q = query[gid];

    float min_dist = INFINITY;
    uint min_idx = 0;

    for (uint c = 0; c < num_centroids; c++) {
        uint packed = codebook[c];

        /* UBYTE_FLT unpack */
        float4 centroid;
        centroid.x = (float)(packed & 0xFFu) * inv_scale;
        centroid.y = (float)((packed >> 8) & 0xFFu) * inv_scale;
        centroid.z = (float)((packed >> 16) & 0xFFu) * inv_scale;
        centroid.w = (float)((packed >> 24) & 0xFFu) * inv_scale;

        /* L2 distance (squared) */
        float4 diff = q - centroid;
        float dist = dot(diff, diff);

        if (dist < min_dist) {
            min_dist = dist;
            min_idx = c;
        }
    }

    nearest_idx[gid] = min_idx;
}
