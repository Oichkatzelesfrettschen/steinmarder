#include "nerf_simd.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <cpuid.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif

/* Portable prefetch macro for reducing cache misses on random hashgrid lookups */
#if defined(__GNUC__) || defined(__clang__)
#define SM_PREFETCH(addr) __builtin_prefetch((const void*)(addr), 0, 0)
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#include <intrin.h>
#define SM_PREFETCH(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#else
#define SM_PREFETCH(addr) ((void)0)
#endif

/* Detect AVX2 and AVX-512 at runtime */
static void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    #ifdef _MSC_VER
        int regs[4];
        __cpuidex(regs, leaf, subleaf);
        *eax = regs[0];
        *ebx = regs[1];
        *ecx = regs[2];
        *edx = regs[3];
    #else
        __cpuid_count(leaf, subleaf, *eax, *ebx, *ecx, *edx);
    #endif
}

CPUFeatures sm_detect_cpu_features(void) {
    CPUFeatures features = {0};
    
    uint32_t eax, ebx, ecx, edx;
    
    /* Check leaf 1 for AVX (ECX bit 28) */
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    bool has_avx = (ecx & (1 << 28)) != 0;
    
    /* Check leaf 7 for AVX2 (EBX bit 5) and AVX-512 (EBX bits 16-17) */
    cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    features.has_avx2 = has_avx && ((ebx & (1 << 5)) != 0);
    features.has_avx512f = (ebx & (1 << 16)) != 0;
    
    if (features.has_avx2) {
        fprintf(stderr, "✓ CPU supports AVX2\n");
    } else {
        fprintf(stderr, "ℹ CPU does NOT support AVX2 (will use scalar fallback)\n");
    }
    
    if (features.has_avx512f) {
        fprintf(stderr, "✓ CPU supports AVX-512F\n");
    }
    
    return features;
}

/* Half-float utilities (for fp16 exported weights/tables) */
static float sm_half_to_float(uint16_t h) {
    uint16_t exp = h & 0x7C00u;
    uint16_t mant = h & 0x03FFu;
    uint32_t sign = ((uint32_t)h & 0x8000u) << 16;
    uint32_t f_exp;
    uint32_t f_mant;
    if (exp == 0x7C00u) {
        /* NaN or Inf */
        f_exp = 0xFFu;
        f_mant = (uint32_t)mant << 13;
    } else if (exp != 0) {
        /* Normalized */
        f_exp = (exp >> 10) + (127u - 15u);
        f_mant = (uint32_t)mant << 13;
    } else if (mant != 0) {
        /* Subnormal */
        uint32_t sig = mant;
        int shift = -1;
        do {
            shift++;
            sig <<= 1;
        } while ((sig & 0x0400u) == 0u);
        sig &= 0x03FFu;
        f_exp = (uint32_t)(127 - 15 - shift);
        f_mant = sig << 13;
    } else {
        /* Zero */
        f_exp = 0;
        f_mant = 0;
    }

    uint32_t bits = sign | (f_exp << 23) | f_mant;
    float out;
    memcpy(&out, &bits, sizeof(out));
    return out;
}

static bool sm_read_half_block(FILE *f, float *dst, uint32_t count) {
    size_t bytes = (size_t)count * sizeof(uint16_t);
    uint16_t *tmp = (uint16_t*)malloc(bytes);
    if (!tmp) {
        return false;
    }
    if (fread(tmp, sizeof(uint16_t), count, f) != count) {
        free(tmp);
        return false;
    }
    for (uint32_t i = 0; i < count; i++) {
        dst[i] = sm_half_to_float(tmp[i]);
    }
    free(tmp);
    return true;
}

static void *sm_aligned_calloc(size_t alignment, size_t count, size_t elem_size) {
    void *ptr = NULL;
    size_t size = count * elem_size;
    size_t padded_size = ((size + alignment - 1u) / alignment) * alignment;
#if defined(_ISOC11_SOURCE)
    ptr = aligned_alloc(alignment, padded_size);
    if (ptr) memset(ptr, 0, padded_size);
#else
    if (posix_memalign(&ptr, alignment, padded_size) != 0) {
        ptr = NULL;
    } else {
        memset(ptr, 0, padded_size);
    }
#endif
    return ptr;
}

static void sm_nerf_prepare_mlp_prepack(NeRFData *data) {
    uint32_t in_dim = data->config.mlp_in_dim;
    uint32_t hidden_dim = data->config.mlp_hidden_dim;
    uint32_t out_dim = data->config.mlp_out_dim;
    const float *w0 = data->mlp_weights;
    const float *b0 = data->mlp_biases;
    const float *w1 = w0 + (size_t)hidden_dim * in_dim;
    const float *b1 = b0 + hidden_dim;
    const float *w_out = w1 + (size_t)hidden_dim * hidden_dim;
    const float *b_out = b1 + hidden_dim;
    size_t w0_elems = (size_t)in_dim * hidden_dim;
    size_t w1_elems = (size_t)hidden_dim * hidden_dim;
    size_t wout_t_elems = (size_t)out_dim * hidden_dim;

    data->mlp_prepacked_ready = 0;
    if (in_dim != 27 || hidden_dim != 64 || out_dim != 4 || data->config.mlp_num_layers != 2) {
        return;
    }

    data->mlp_w0_aligned = (float*)sm_aligned_calloc(32u, w0_elems, sizeof(float));
    data->mlp_w1_aligned = (float*)sm_aligned_calloc(32u, w1_elems, sizeof(float));
    data->mlp_wout_t_aligned = (float*)sm_aligned_calloc(32u, wout_t_elems, sizeof(float));
    data->mlp_b0_aligned = (float*)sm_aligned_calloc(32u, hidden_dim, sizeof(float));
    data->mlp_b1_aligned = (float*)sm_aligned_calloc(32u, hidden_dim, sizeof(float));
    data->mlp_bout_aligned = (float*)sm_aligned_calloc(32u, out_dim, sizeof(float));
    if (!data->mlp_w0_aligned || !data->mlp_w1_aligned || !data->mlp_wout_t_aligned ||
        !data->mlp_b0_aligned || !data->mlp_b1_aligned || !data->mlp_bout_aligned) {
        free(data->mlp_w0_aligned);
        free(data->mlp_w1_aligned);
        free(data->mlp_wout_t_aligned);
        free(data->mlp_b0_aligned);
        free(data->mlp_b1_aligned);
        free(data->mlp_bout_aligned);
        data->mlp_w0_aligned = NULL;
        data->mlp_w1_aligned = NULL;
        data->mlp_wout_t_aligned = NULL;
        data->mlp_b0_aligned = NULL;
        data->mlp_b1_aligned = NULL;
        data->mlp_bout_aligned = NULL;
        return;
    }

    memcpy(data->mlp_w0_aligned, w0, w0_elems * sizeof(float));
    memcpy(data->mlp_w1_aligned, w1, w1_elems * sizeof(float));
    memcpy(data->mlp_b0_aligned, b0, hidden_dim * sizeof(float));
    memcpy(data->mlp_b1_aligned, b1, hidden_dim * sizeof(float));
    memcpy(data->mlp_bout_aligned, b_out, out_dim * sizeof(float));
    for (uint32_t h = 0; h < hidden_dim; h++) {
        for (uint32_t o = 0; o < out_dim; o++) {
            data->mlp_wout_t_aligned[(size_t)o * hidden_dim + h] =
                w_out[(size_t)h * out_dim + o];
        }
    }
    data->mlp_prepacked_ready = 1;
}

#ifdef __AVX512BF16__
/* Convert IEEE 754 float to BF16 by truncating the lower 16 mantissa bits. */
static inline uint16_t sm_float_to_bf16(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (uint16_t)(bits >> 16);
}

/* Prepare pair-interleaved BF16 weight arrays for vdpbf16ps inference.
 *
 * Weight layout for vdpbf16ps: each 512-bit ZMM holds 16 FP32 accumulators.
 * The instruction multiplies 16 BF16 *pairs* (32 BF16 values packed as 16 x
 * uint32) against a broadcast pair, accumulating into the 16 FP32 lanes.
 *
 * For W1 [64 inputs -> 64 outputs] we iterate inputs in pairs (hp, hp+1).
 * Each pair-row stores, for 64 outputs:
 *   bf16(w[hp][0]), bf16(w[hp+1][0]),  bf16(w[hp][1]), bf16(w[hp+1][1]), ...
 * That is: low 16 bits = w[hp][h], high 16 bits = w[hp+1][h] for each output h.
 *
 * For W0 [27 inputs -> 64 outputs] we pad to 28 inputs (14 pairs) with zeros.
 */
static void sm_nerf_prepare_mlp_bf16(NeRFData *data) {
    uint32_t in_dim = data->config.mlp_in_dim;
    uint32_t hidden_dim = data->config.mlp_hidden_dim;
    uint32_t out_dim = data->config.mlp_out_dim;

    data->mlp_bf16_ready = 0;

    if (in_dim != 27 || hidden_dim != 64 || out_dim != 4 || data->config.mlp_num_layers != 2) {
        return;
    }
    if (!data->mlp_prepacked_ready) {
        return;
    }

    /* W0: 27 inputs padded to 28 -> 14 pairs, each pair has 64 outputs.
     * Storage: 14 pairs * 64 outputs * 2 bf16 per pair = 14 * 128 uint16_t = 1792 uint16_t.
     * Byte layout per pair-row: [bf16(w[hp][0]), bf16(w[hp+1][0]), bf16(w[hp][1]), ...] */
    uint32_t w0_padded_inputs = 28;
    uint32_t w0_num_pairs = w0_padded_inputs / 2;  /* 14 */
    size_t w0_bf16_elems = (size_t)w0_num_pairs * hidden_dim * 2;
    data->mlp_w0_bf16 = (uint16_t*)sm_aligned_calloc(64u, w0_bf16_elems, sizeof(uint16_t));

    /* W1: 64 inputs -> 32 pairs, each pair has 64 outputs.
     * Storage: 32 * 64 * 2 = 4096 uint16_t. */
    uint32_t w1_num_pairs = hidden_dim / 2;  /* 32 */
    size_t w1_bf16_elems = (size_t)w1_num_pairs * hidden_dim * 2;
    data->mlp_w1_bf16 = (uint16_t*)sm_aligned_calloc(64u, w1_bf16_elems, sizeof(uint16_t));

    if (!data->mlp_w0_bf16 || !data->mlp_w1_bf16) {
        free(data->mlp_w0_bf16);
        free(data->mlp_w1_bf16);
        data->mlp_w0_bf16 = NULL;
        data->mlp_w1_bf16 = NULL;
        return;
    }

    /* Pack W0 into pair-interleaved BF16.
     * Source layout (mlp_w0_aligned): [input_idx][hidden_idx], row-major.
     * For pair p (inputs 2p and 2p+1), output h:
     *   offset = p * (hidden_dim * 2) + h * 2
     *   [offset+0] = bf16(w0[2p][h])
     *   [offset+1] = bf16(w0[2p+1][h]) */
    for (uint32_t pair_idx = 0; pair_idx < w0_num_pairs; pair_idx++) {
        uint32_t input_a = pair_idx * 2;
        uint32_t input_b = pair_idx * 2 + 1;
        for (uint32_t h = 0; h < hidden_dim; h++) {
            size_t dst_offset = (size_t)pair_idx * hidden_dim * 2 + (size_t)h * 2;
            float val_a = (input_a < in_dim) ? data->mlp_w0_aligned[input_a * hidden_dim + h] : 0.0f;
            float val_b = (input_b < in_dim) ? data->mlp_w0_aligned[input_b * hidden_dim + h] : 0.0f;
            data->mlp_w0_bf16[dst_offset + 0] = sm_float_to_bf16(val_a);
            data->mlp_w0_bf16[dst_offset + 1] = sm_float_to_bf16(val_b);
        }
    }

    /* Pack W1 into pair-interleaved BF16.
     * Source layout (mlp_w1_aligned): [input_idx][hidden_idx], row-major. */
    for (uint32_t pair_idx = 0; pair_idx < w1_num_pairs; pair_idx++) {
        uint32_t input_a = pair_idx * 2;
        uint32_t input_b = pair_idx * 2 + 1;
        for (uint32_t h = 0; h < hidden_dim; h++) {
            size_t dst_offset = (size_t)pair_idx * hidden_dim * 2 + (size_t)h * 2;
            data->mlp_w1_bf16[dst_offset + 0] = sm_float_to_bf16(data->mlp_w1_aligned[input_a * hidden_dim + h]);
            data->mlp_w1_bf16[dst_offset + 1] = sm_float_to_bf16(data->mlp_w1_aligned[input_b * hidden_dim + h]);
        }
    }

    data->mlp_bf16_ready = 1;
    fprintf(stderr, "[NeRF] BF16 pair-interleaved weights ready (W0: %u pairs, W1: %u pairs)\n",
            w0_num_pairs, w1_num_pairs);
}
#endif /* __AVX512BF16__ */

SMNeRFMLPVariant sm_nerf_mlp_variant(void) {
    static int cached = -1;
    const char *env;
    if (cached != -1) return (SMNeRFMLPVariant)cached;
    cached = SM_NERF_MLP_VARIANT_GENERIC;
    env = getenv("SM_NERF_MLP_VARIANT");
    if (!env || !env[0]) return (SMNeRFMLPVariant)cached;
    if (strcmp(env, "prepacked") == 0 || strcmp(env, "1") == 0) {
        cached = SM_NERF_MLP_VARIANT_PREPACKED;
    } else if (strcmp(env, "bf16") == 0 || strcmp(env, "2") == 0) {
        cached = SM_NERF_MLP_VARIANT_BF16;
    }
    return (SMNeRFMLPVariant)cached;
}

const char *sm_nerf_mlp_variant_name(SMNeRFMLPVariant variant) {
    switch (variant) {
        case SM_NERF_MLP_VARIANT_PREPACKED:
            return "prepacked";
        case SM_NERF_MLP_VARIANT_BF16:
            return "bf16";
        case SM_NERF_MLP_VARIANT_GENERIC:
        default:
            return "generic";
    }
}

#ifdef __AVX2__
static float sm_hsum256_ps(__m256 v) {
    float lanes[8];
    _mm256_storeu_ps(lanes, v);
    return lanes[0] + lanes[1] + lanes[2] + lanes[3] +
           lanes[4] + lanes[5] + lanes[6] + lanes[7];
}

static void sm_mlp_inference_single_prepacked_data(
    const float features_in[27],
    const NeRFData *nerf_data,
    float rgb_out[3],
    float *sigma_out
) {
    float hidden[64];
    float hidden2[64];
    float out_acc[4];
    int h;

    for (h = 0; h < 64; h += 8) {
        _mm256_store_ps(&hidden[h], _mm256_load_ps(&nerf_data->mlp_b0_aligned[h]));
    }
    for (int i = 0; i < 27; i++) {
        __m256 fvec = _mm256_set1_ps(features_in[i]);
        const float *w0_row = &nerf_data->mlp_w0_aligned[(size_t)i * 64];
        for (h = 0; h < 64; h += 8) {
            __m256 acc = _mm256_load_ps(&hidden[h]);
            __m256 wvec = _mm256_load_ps(&w0_row[h]);
            acc = _mm256_fmadd_ps(wvec, fvec, acc);
            _mm256_store_ps(&hidden[h], acc);
        }
    }
    {
        __m256 zero = _mm256_setzero_ps();
        for (h = 0; h < 64; h += 8) {
            __m256 v = _mm256_load_ps(&hidden[h]);
            _mm256_store_ps(&hidden[h], _mm256_max_ps(v, zero));
        }
    }

    for (h = 0; h < 64; h += 8) {
        _mm256_store_ps(&hidden2[h], _mm256_load_ps(&nerf_data->mlp_b1_aligned[h]));
    }
    for (int hp = 0; hp < 64; hp++) {
        __m256 fvec = _mm256_set1_ps(hidden[hp]);
        const float *w1_row = &nerf_data->mlp_w1_aligned[(size_t)hp * 64];
        for (h = 0; h < 64; h += 8) {
            __m256 acc = _mm256_load_ps(&hidden2[h]);
            __m256 wvec = _mm256_load_ps(&w1_row[h]);
            acc = _mm256_fmadd_ps(wvec, fvec, acc);
            _mm256_store_ps(&hidden2[h], acc);
        }
    }
    {
        __m256 zero = _mm256_setzero_ps();
        for (h = 0; h < 64; h += 8) {
            __m256 v = _mm256_load_ps(&hidden2[h]);
            _mm256_store_ps(&hidden2[h], _mm256_max_ps(v, zero));
        }
    }

    for (int o = 0; o < 4; o++) {
        __m256 acc0 = _mm256_setzero_ps();
        const float *wcol = &nerf_data->mlp_wout_t_aligned[(size_t)o * 64];
        for (h = 0; h < 64; h += 8) {
            __m256 hv = _mm256_load_ps(&hidden2[h]);
            __m256 wv = _mm256_load_ps(&wcol[h]);
            acc0 = _mm256_fmadd_ps(hv, wv, acc0);
        }
        out_acc[o] = nerf_data->mlp_bout_aligned[o] + sm_hsum256_ps(acc0);
    }

    rgb_out[0] = 1.0f / (1.0f + expf(-out_acc[0]));
    rgb_out[1] = 1.0f / (1.0f + expf(-out_acc[1]));
    rgb_out[2] = 1.0f / (1.0f + expf(-out_acc[2]));
    if (out_acc[3] > 20.0f) *sigma_out = out_acc[3];
    else if (out_acc[3] < -20.0f) *sigma_out = 0.0f;
    else *sigma_out = logf(1.0f + expf(out_acc[3]));
}
#endif

#ifdef __AVX512BF16__
/* BF16 single-ray MLP inference using AVX-512 vdpbf16ps.
 *
 * Layer 0 (W0, 27->64): 28 inputs (padded), 14 input-pairs, 64 outputs.
 *   Each vdpbf16ps processes 1 pair across 16 outputs (one ZMM accumulator).
 *   4 ZMM accumulators cover all 64 outputs.  14 pairs * 4 ZMMs = 56 vdpbf16ps.
 *
 * Layer 1 (W1, 64->64): 32 input-pairs, 64 outputs.
 *   32 pairs * 4 ZMMs = 128 vdpbf16ps.
 *
 * Output layer (W_out, 64->4): kept as FP32 AVX2 (only 4% of work). */
static void sm_mlp_inference_single_bf16(
    const float features_in[27],
    const NeRFData *nerf_data,
    float rgb_out[3],
    float *sigma_out
) {
    float hidden[64] __attribute__((aligned(64)));
    float hidden2[64] __attribute__((aligned(64)));
    float out_acc[4];
    int h;

    /* === Layer 0: Input -> Hidden (BF16 path) ===
     * W0 is pair-interleaved BF16: [14 pairs][64 outputs][2 bf16].
     * Pad input 27 to 28 (28th = 0.0f). */
    float padded_input[28] __attribute__((aligned(64)));
    memcpy(padded_input, features_in, 27 * sizeof(float));
    padded_input[27] = 0.0f;

    /* Initialize accumulators from bias */
    __m512 acc_zmm_0 = _mm512_load_ps(&nerf_data->mlp_b0_aligned[0]);
    __m512 acc_zmm_1 = _mm512_load_ps(&nerf_data->mlp_b0_aligned[16]);
    __m512 acc_zmm_2 = _mm512_load_ps(&nerf_data->mlp_b0_aligned[32]);
    __m512 acc_zmm_3 = _mm512_load_ps(&nerf_data->mlp_b0_aligned[48]);

    for (int pair_idx = 0; pair_idx < 14; pair_idx++) {
        /* Pack two consecutive activations as a BF16 pair, broadcast to all 16 positions.
         * Low 16 bits = bf16(input[2*pair_idx]), high 16 bits = bf16(input[2*pair_idx+1]). */
        uint16_t bf16_a = sm_float_to_bf16(padded_input[pair_idx * 2]);
        uint16_t bf16_b = sm_float_to_bf16(padded_input[pair_idx * 2 + 1]);
        uint32_t activation_pair = ((uint32_t)bf16_b << 16) | (uint32_t)bf16_a;
        __m512bh act_pairs = (__m512bh)_mm512_set1_epi32((int)activation_pair);

        /* Each pair-row is 64 outputs * 2 bf16 = 128 uint16_t = 256 bytes = 4 ZMM loads. */
        const uint16_t *weight_row = &nerf_data->mlp_w0_bf16[(size_t)pair_idx * 64 * 2];

        __m512bh wt_0 = (__m512bh)_mm512_load_si512(&weight_row[0]);
        __m512bh wt_1 = (__m512bh)_mm512_load_si512(&weight_row[32]);
        __m512bh wt_2 = (__m512bh)_mm512_load_si512(&weight_row[64]);
        __m512bh wt_3 = (__m512bh)_mm512_load_si512(&weight_row[96]);

        acc_zmm_0 = _mm512_dpbf16_ps(acc_zmm_0, act_pairs, wt_0);
        acc_zmm_1 = _mm512_dpbf16_ps(acc_zmm_1, act_pairs, wt_1);
        acc_zmm_2 = _mm512_dpbf16_ps(acc_zmm_2, act_pairs, wt_2);
        acc_zmm_3 = _mm512_dpbf16_ps(acc_zmm_3, act_pairs, wt_3);
    }

    /* ReLU */
    __m512 zero_zmm = _mm512_setzero_ps();
    _mm512_store_ps(&hidden[0],  _mm512_max_ps(acc_zmm_0, zero_zmm));
    _mm512_store_ps(&hidden[16], _mm512_max_ps(acc_zmm_1, zero_zmm));
    _mm512_store_ps(&hidden[32], _mm512_max_ps(acc_zmm_2, zero_zmm));
    _mm512_store_ps(&hidden[48], _mm512_max_ps(acc_zmm_3, zero_zmm));

    /* === Layer 1: Hidden -> Hidden (BF16 path) ===
     * W1 is pair-interleaved BF16: [32 pairs][64 outputs][2 bf16]. */
    acc_zmm_0 = _mm512_load_ps(&nerf_data->mlp_b1_aligned[0]);
    acc_zmm_1 = _mm512_load_ps(&nerf_data->mlp_b1_aligned[16]);
    acc_zmm_2 = _mm512_load_ps(&nerf_data->mlp_b1_aligned[32]);
    acc_zmm_3 = _mm512_load_ps(&nerf_data->mlp_b1_aligned[48]);

    for (int pair_idx = 0; pair_idx < 32; pair_idx++) {
        uint16_t bf16_a = sm_float_to_bf16(hidden[pair_idx * 2]);
        uint16_t bf16_b = sm_float_to_bf16(hidden[pair_idx * 2 + 1]);
        uint32_t activation_pair = ((uint32_t)bf16_b << 16) | (uint32_t)bf16_a;
        __m512bh act_pairs = (__m512bh)_mm512_set1_epi32((int)activation_pair);

        const uint16_t *weight_row = &nerf_data->mlp_w1_bf16[(size_t)pair_idx * 64 * 2];

        __m512bh wt_0 = (__m512bh)_mm512_load_si512(&weight_row[0]);
        __m512bh wt_1 = (__m512bh)_mm512_load_si512(&weight_row[32]);
        __m512bh wt_2 = (__m512bh)_mm512_load_si512(&weight_row[64]);
        __m512bh wt_3 = (__m512bh)_mm512_load_si512(&weight_row[96]);

        acc_zmm_0 = _mm512_dpbf16_ps(acc_zmm_0, act_pairs, wt_0);
        acc_zmm_1 = _mm512_dpbf16_ps(acc_zmm_1, act_pairs, wt_1);
        acc_zmm_2 = _mm512_dpbf16_ps(acc_zmm_2, act_pairs, wt_2);
        acc_zmm_3 = _mm512_dpbf16_ps(acc_zmm_3, act_pairs, wt_3);
    }

    /* ReLU */
    _mm512_store_ps(&hidden2[0],  _mm512_max_ps(acc_zmm_0, zero_zmm));
    _mm512_store_ps(&hidden2[16], _mm512_max_ps(acc_zmm_1, zero_zmm));
    _mm512_store_ps(&hidden2[32], _mm512_max_ps(acc_zmm_2, zero_zmm));
    _mm512_store_ps(&hidden2[48], _mm512_max_ps(acc_zmm_3, zero_zmm));

    /* === Output layer: Hidden2 -> Output (FP32 AVX2, only 4 outputs) === */
    for (int o = 0; o < 4; o++) {
        __m256 acc256_0 = _mm256_setzero_ps();
        const float *wcol = &nerf_data->mlp_wout_t_aligned[(size_t)o * 64];
        for (h = 0; h < 64; h += 8) {
            __m256 hv = _mm256_load_ps(&hidden2[h]);
            __m256 wv = _mm256_load_ps(&wcol[h]);
            acc256_0 = _mm256_fmadd_ps(hv, wv, acc256_0);
        }
        out_acc[o] = nerf_data->mlp_bout_aligned[o] + sm_hsum256_ps(acc256_0);
    }

    /* Sigmoid for RGB, softplus for sigma (matches prepacked path exactly) */
    rgb_out[0] = 1.0f / (1.0f + expf(-out_acc[0]));
    rgb_out[1] = 1.0f / (1.0f + expf(-out_acc[1]));
    rgb_out[2] = 1.0f / (1.0f + expf(-out_acc[2]));
    if (out_acc[3] > 20.0f) *sigma_out = out_acc[3];
    else if (out_acc[3] < -20.0f) *sigma_out = 0.0f;
    else *sigma_out = logf(1.0f + expf(out_acc[3]));
}
#endif /* __AVX512BF16__ */


/* ===== SIMD Utilities ===== */

#ifdef __AVX2__
#include <immintrin.h>
#include <x86intrin.h>

static inline uint64_t sm_rdtsc(void) {
    return __builtin_ia32_rdtsc();
}

static inline __m256 sm_sigmoid_avx2(__m256 x) {
    /* sigmoid(x) = 1 / (1 + exp(-x)) - Fast approximation */
    const __m256 one = _mm256_set1_ps(1.0f);
    
    /* Fast sigmoid approximation: clamp(0.5 + 0.2 * x, 0, 1) */
    __m256 sigmoid_approx = _mm256_mul_ps(x, _mm256_set1_ps(0.2f));
    sigmoid_approx = _mm256_add_ps(sigmoid_approx, _mm256_set1_ps(0.5f));
    sigmoid_approx = _mm256_max_ps(sigmoid_approx, _mm256_set1_ps(0.0f));
    sigmoid_approx = _mm256_min_ps(sigmoid_approx, one);
    
    return sigmoid_approx;
}

static inline __m256 sm_relu_avx2(__m256 x) {
    return _mm256_max_ps(x, _mm256_set1_ps(0.0f));
}

#else  /* Scalar fallback for CPUs without AVX2 */

static inline uint64_t sm_rdtsc(void) {
    /* Fallback: not available on older CPUs */
    return 0;
}

static inline float sm_sigmoid_scalar(float x) {
    /* sigmoid(x) ≈ clamp(0.5 + 0.2 * x, 0, 1) */
    float result = 0.5f + 0.2f * x;
    if (result < 0.0f) return 0.0f;
    if (result > 1.0f) return 1.0f;
    return result;
}

static inline float sm_relu_scalar(float x) {
    return x > 0.0f ? x : 0.0f;
}

#endif

/* ===== Hash Function ===== */

static inline uint32_t sm_hash_ijk(
    int32_t i, int32_t j, int32_t k,
    uint32_t hash_size
) {
    /* Cast to unsigned for hash */
    uint32_t ui = (uint32_t)i;
    uint32_t uj = (uint32_t)j;
    uint32_t uk = (uint32_t)k;
    
    /* Spatial hash with prime multipliers */
    uint32_t hash = (ui * 73856093u) ^ (uj * 19349663u) ^ (uk * 83492791u);
    return hash % hash_size;
}

static inline uint32_t sm_hash_position(
    const Vec3 *pos,
    float level_scale,
    uint32_t hash_size
) {
    /* Scale position and convert to integer coordinates */
    int32_t x = (int32_t)floorf(pos->x * level_scale);
    int32_t y = (int32_t)floorf(pos->y * level_scale);
    int32_t z = (int32_t)floorf(pos->z * level_scale);
    
    return sm_hash_ijk(x, y, z, hash_size);
}

/* ===== NeRF Data Loading ===== */

NeRFData* sm_nerf_data_load(const char *hashgrid_path, const char *occ_path) {
    FILE *f_hash = fopen(hashgrid_path, "rb");
    if (!f_hash) {
        fprintf(stderr, "ERROR: Cannot open hashgrid file: %s\n", hashgrid_path);
        return NULL;
    }

    FILE *f_occ = fopen(occ_path, "rb");
    if (!f_occ) {
        fprintf(stderr, "ERROR: Cannot open occupancy file: %s\n", occ_path);
        fclose(f_hash);
        return NULL;
    }

    NeRFData *data = (NeRFData*)malloc(sizeof(NeRFData));
    if (!data) {
        fclose(f_hash);
        fclose(f_occ);
        return NULL;
    }
    memset(data, 0, sizeof(NeRFData));

    /* Read header (15 u32 = 60 bytes) */
    uint32_t header[15];
    if (fread(header, sizeof(uint32_t), 15, f_hash) != 15) {
        fprintf(stderr, "ERROR: Failed to read NeRF header\n");
        fclose(f_hash);
        fclose(f_occ);
        free(data);
        return NULL;
    }

    data->config.magic = header[0];
    data->config.version = header[1];
    data->config.num_levels = header[2];
    data->config.features_per_entry = header[3];
    data->config.hashmap_size = header[4];
    data->config.base_res = header[5];
    memcpy(&data->config.per_level_scale, &header[6], sizeof(float));
    data->config.mlp_in_dim = header[7];
    data->config.mlp_hidden_dim = header[8];
    data->config.mlp_num_layers = header[9];
    data->config.mlp_out_dim = header[10];
    memcpy(&data->config.scale, &header[11], sizeof(float));
    memcpy(&data->config.center.x, &header[12], sizeof(float));
    memcpy(&data->config.center.y, &header[13], sizeof(float));
    memcpy(&data->config.center.z, &header[14], sizeof(float));

    bool fp16_format = data->config.version >= 2;

    printf("[NeRF] Loaded config: levels=%u, hash_size=%u, mlp=%u->%u->%u (v%u, %s weights)\n",
           data->config.num_levels, data->config.hashmap_size,
           data->config.mlp_in_dim, data->config.mlp_hidden_dim, data->config.mlp_out_dim,
           data->config.version, fp16_format ? "fp16" : "fp32");

    /* Hashgrid: stored as fp16 tables in version >= 2 */
    uint32_t grid_elems = data->config.num_levels * data->config.hashmap_size *
                          data->config.features_per_entry;
    size_t grid_bytes = (size_t)grid_elems * sizeof(uint16_t);
    uint16_t *grid_raw = (uint16_t*)malloc(grid_bytes);
    if (!grid_raw) {
        fclose(f_hash);
        fclose(f_occ);
        free(data);
        return NULL;
    }
    if (fread(grid_raw, sizeof(uint16_t), grid_elems, f_hash) != grid_elems) {
        fprintf(stderr, "ERROR: Failed to read hashgrid data\n");
        free(grid_raw);
        fclose(f_hash);
        fclose(f_occ);
        free(data);
        return NULL;
    }
    // Check if compact FP16 storage is requested (halves hashgrid memory).
    // SM_NERF_HASHGRID_FP16=1 keeps data as 16-bit, converts on lookup.
    const char *compact_env = getenv("SM_NERF_HASHGRID_FP16");
    int use_compact = (compact_env && compact_env[0] == '1' && fp16_format);

    if (use_compact) {
        // Compact mode: keep raw FP16 bits, convert on-the-fly during lookup.
        // Memory: 2 bytes/entry instead of 4 bytes/entry (50% reduction).
        data->hashgrid_fp16 = (uint16_t*)malloc((size_t)grid_elems * sizeof(uint16_t));
        if (!data->hashgrid_fp16) {
            free(grid_raw); fclose(f_hash); fclose(f_occ); free(data);
            return NULL;
        }
        memcpy(data->hashgrid_fp16, grid_raw, grid_elems * sizeof(uint16_t));
        data->hashgrid_data = NULL;
        data->hashgrid_compact = 1;
        fprintf(stderr, "[NeRF] hashgrid: compact FP16 mode (%u entries, %.1f MB saved)\n",
                grid_elems, (float)grid_elems * 2.0f / (1024.0f * 1024.0f));
    } else {
        // Standard mode: expand to FP32 for maximum lookup speed.
        data->hashgrid_data = (float*)malloc((size_t)grid_elems * sizeof(float));
        if (!data->hashgrid_data) {
            free(grid_raw); fclose(f_hash); fclose(f_occ); free(data);
            return NULL;
        }
        for (uint32_t i = 0; i < grid_elems; i++) {
            if (fp16_format) {
                data->hashgrid_data[i] = sm_half_to_float(grid_raw[i]);
            } else {
                float v = (float)grid_raw[i] / 32767.5f - 1.0f;
                if (v > 1.0f) v = 1.0f;
                if (v < -1.0f) v = -1.0f;
                data->hashgrid_data[i] = v;
            }
        }
        data->hashgrid_fp16 = NULL;
        data->hashgrid_compact = 0;
    }
    free(grid_raw);

    /* MLP sizes
     * WEIGHT LAYOUT NOTE: this CPU inference path stores W0 as
     * [in_dim][hidden_dim] = [27][64] (row-major, outer index = input feature).
     * The GPU kernel in src/sass_re/instant_ngp/mlp_forward.cu stores W0 as
     * [hidden_dim][in_dim] = [64][27] (outer index = neuron).
     * These are TRANSPOSED from each other — GPU-trained checkpoints cannot
     * be loaded here without transposing W0 and W1 first.
     * Set env var SM_NERF_GPU_WEIGHTS=1 to emit a reminder at load time. */
    if (getenv("SM_NERF_GPU_WEIGHTS")) {
        fprintf(stderr,
            "[NeRF] WARNING: SM_NERF_GPU_WEIGHTS set. GPU checkpoints store W0 as\n"
            "  [hidden][in]=[64][27]. This CPU code expects [in][hidden]=[27][64].\n"
            "  Transpose W0 and W1 before loading, or results will be wrong.\n");
    }
    uint32_t w0_size = data->config.mlp_hidden_dim * data->config.mlp_in_dim;
    uint32_t b0_size = data->config.mlp_hidden_dim;
    uint32_t w1_size = data->config.mlp_hidden_dim * data->config.mlp_hidden_dim;
    uint32_t b1_size = data->config.mlp_hidden_dim;
    uint32_t w_out_size = data->config.mlp_out_dim * data->config.mlp_hidden_dim;
    uint32_t b_out_size = data->config.mlp_out_dim;

    uint32_t total_weight_elems = w0_size + w1_size + w_out_size;
    uint32_t total_bias_elems = b0_size + b1_size + b_out_size;

    data->mlp_weights = (float*)malloc((size_t)total_weight_elems * sizeof(float));
    data->mlp_biases = (float*)malloc((size_t)total_bias_elems * sizeof(float));
    if (!data->mlp_weights || !data->mlp_biases) {
        free(data->hashgrid_data);
        free(data->mlp_weights);
        free(data->mlp_biases);
        fclose(f_hash);
        fclose(f_occ);
        free(data);
        return NULL;
    }

    if (fp16_format) {
        uint32_t w_off = 0;
        uint32_t b_off = 0;
        if (!sm_read_half_block(f_hash, data->mlp_weights + w_off, w0_size)) goto load_fail;
        w_off += w0_size;
        if (!sm_read_half_block(f_hash, data->mlp_biases + b_off, b0_size)) goto load_fail;
        b_off += b0_size;
        if (!sm_read_half_block(f_hash, data->mlp_weights + w_off, w1_size)) goto load_fail;
        w_off += w1_size;
        if (!sm_read_half_block(f_hash, data->mlp_biases + b_off, b1_size)) goto load_fail;
        b_off += b1_size;
        if (!sm_read_half_block(f_hash, data->mlp_weights + w_off, w_out_size)) goto load_fail;
        w_off += w_out_size;
        if (!sm_read_half_block(f_hash, data->mlp_biases + b_off, b_out_size)) goto load_fail;
    } else {
        size_t weights_bytes = (size_t)total_weight_elems * sizeof(float);
        size_t biases_bytes = (size_t)total_bias_elems * sizeof(float);
        if (fread(data->mlp_weights, 1, weights_bytes, f_hash) != weights_bytes) goto load_fail;
        if (fread(data->mlp_biases, 1, biases_bytes, f_hash) != biases_bytes) goto load_fail;
    }

    /* Load occupancy grid (version >=2 includes a 16-byte header) */
    uint32_t occ_dim = 64;
    float occ_threshold = 0.0f;
    if (fp16_format) {
        uint32_t occ_magic = 0;
        if (fread(&occ_magic, sizeof(uint32_t), 1, f_occ) != 1) goto load_fail;
        if (occ_magic != 0x31474F4Eu) {
            fprintf(stderr, "ERROR: Occupancy grid magic mismatch (0x%08X)\n", occ_magic);
            goto load_fail;
        }
        if (fread(&occ_dim, sizeof(uint32_t), 1, f_occ) != 1) goto load_fail;
        float voxel_size = 0.0f;
        if (fread(&voxel_size, sizeof(float), 1, f_occ) != 1) goto load_fail;
        if (fread(&occ_threshold, sizeof(float), 1, f_occ) != 1) goto load_fail;
        (void)voxel_size;  /* Currently unused */
    }

    size_t occ_count = (size_t)occ_dim * occ_dim * occ_dim;
    data->occupancy_grid = (uint8_t*)malloc(occ_count);
    if (!data->occupancy_grid) goto load_fail;
    if (fread(data->occupancy_grid, 1, occ_count, f_occ) != occ_count) {
        fprintf(stderr, "ERROR: Failed to read occupancy grid\n");
        goto load_fail;
    }

    fclose(f_hash);
    fclose(f_occ);

    printf("[NeRF] Loaded %.2f KB hashgrid, %u weights, %u biases, occupancy %u^3 (thr=%.4f)\n",
           (double)(grid_elems * sizeof(float)) / 1024.0,
           total_weight_elems, total_bias_elems, occ_dim, occ_threshold);

    sm_nerf_prepare_mlp_prepack(data);
#ifdef __AVX512BF16__
    sm_nerf_prepare_mlp_bf16(data);
#endif

    return data;

load_fail:
    fclose(f_hash);
    fclose(f_occ);
    sm_nerf_data_free(data);
    return NULL;
}

void sm_nerf_data_free(NeRFData *data) {
    if (!data) return;
    free(data->hashgrid_data);
    free(data->hashgrid_fp16);
    free(data->mlp_weights);
    free(data->mlp_biases);
    free(data->mlp_w0_aligned);
    free(data->mlp_w1_aligned);
    free(data->mlp_wout_t_aligned);
    free(data->mlp_b0_aligned);
    free(data->mlp_b1_aligned);
    free(data->mlp_bout_aligned);
#ifdef __AVX512BF16__
    free(data->mlp_w0_bf16);
    free(data->mlp_w1_bf16);
#endif
    free(data->occupancy_grid);
    free(data);
}

/* ===== FP16 hashgrid read helper ===== */

/* Read a hashgrid feature value, converting from FP16 if in compact mode.
 * In compact mode, hashgrid_data is NULL and hashgrid_fp16 is non-NULL.
 * The caller passes both pointers; exactly one should be non-NULL. */
static inline float hashgrid_read(
    const float *hashgrid_f32,
    const uint16_t *hashgrid_fp16,
    uint32_t index
) {
    if (hashgrid_f32) return hashgrid_f32[index];
    return sm_half_to_float(hashgrid_fp16[index]);
}

enum {
    SM_HASHGRID_PREFETCH_OFF = 0,
    SM_HASHGRID_PREFETCH_IMMEDIATE = 1,
    SM_HASHGRID_PREFETCH_NEXT_RAY = 2,
    SM_HASHGRID_PREFETCH_IMMEDIATE_4 = 3
};

static int sm_hashgrid_prefetch_mode(void) {
    static int cached_mode = -1;
    if (cached_mode >= 0) {
        return cached_mode;
    }

    cached_mode = SM_HASHGRID_PREFETCH_IMMEDIATE;
    {
        const char *env = getenv("SM_NERF_HASHGRID_PREFETCH");
        if (env) {
            if (strcmp(env, "0") == 0 ||
                strcmp(env, "off") == 0 ||
                strcmp(env, "OFF") == 0 ||
                strcmp(env, "none") == 0 ||
                strcmp(env, "NONE") == 0) {
                cached_mode = SM_HASHGRID_PREFETCH_OFF;
            } else if (strcmp(env, "1") == 0 ||
                       strcmp(env, "on") == 0 ||
                       strcmp(env, "ON") == 0 ||
                       strcmp(env, "immediate") == 0 ||
                       strcmp(env, "IMMEDIATE") == 0) {
                cached_mode = SM_HASHGRID_PREFETCH_IMMEDIATE;
            } else if (strcmp(env, "2") == 0 ||
                       strcmp(env, "ahead") == 0 ||
                       strcmp(env, "AHEAD") == 0 ||
                       strcmp(env, "next") == 0 ||
                       strcmp(env, "NEXT") == 0) {
                cached_mode = SM_HASHGRID_PREFETCH_NEXT_RAY;
            } else if (strcmp(env, "3") == 0 ||
                       strcmp(env, "corners4") == 0 ||
                       strcmp(env, "CORNERS4") == 0 ||
                       strcmp(env, "half") == 0 ||
                       strcmp(env, "HALF") == 0) {
                cached_mode = SM_HASHGRID_PREFETCH_IMMEDIATE_4;
            }
        }
    }
    return cached_mode;
}

/* ===== Batched Hashgrid Lookup with Trilinear Interpolation ===== */

void sm_hashgrid_lookup_batch(
    const Vec3 positions[SIMD_BATCH_SIZE],
    const NeRFConfig *config,
    const float *hashgrid_data,
    float features_out[SIMD_BATCH_SIZE][24]
) {
    /* For each level, look up features per position with trilinear interpolation.
     * Output array is features_out[SIMD_BATCH_SIZE][24], so we must ensure
     * batch_levels * features_per_entry <= 24 to avoid out-of-bounds writes. */
    uint32_t fpe = config->features_per_entry > 0 ? config->features_per_entry : 2;
    uint32_t max_levels = 24 / fpe;   /* max levels that fit in the [24] output */
    uint32_t batch_levels = config->num_levels < max_levels ? config->num_levels : max_levels;
    for (uint32_t level = 0; level < batch_levels; level++) {
        /* Match Python: res = int(base_res * (per_level_scale ** l)) */
        float res = (float)(int)(config->base_res * powf(config->per_level_scale, (float)level));
        uint32_t level_offset = level * config->hashmap_size * config->features_per_entry;
        
        for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
            Vec3 p = positions[ray];
            
            /* Scale position to grid coordinates */
            float gx = p.x * res;
            float gy = p.y * res;
            float gz = p.z * res;
            
            /* Get integer corner and fractional weights */
            float fx = floorf(gx);
            float fy = floorf(gy);
            float fz = floorf(gz);
            int32_t i0 = (int32_t)fx;
            int32_t j0 = (int32_t)fy;
            int32_t k0 = (int32_t)fz;
            
            /* Interpolation weights */
            float wx = gx - fx;
            float wy = gy - fy;
            float wz = gz - fz;
            
            /* Hash all 8 corners once (independent of feature index) */
            uint32_t h000 = sm_hash_ijk(i0,   j0,   k0,   config->hashmap_size);
            uint32_t h001 = sm_hash_ijk(i0,   j0,   k0+1, config->hashmap_size);
            uint32_t h010 = sm_hash_ijk(i0,   j0+1, k0,   config->hashmap_size);
            uint32_t h011 = sm_hash_ijk(i0,   j0+1, k0+1, config->hashmap_size);
            uint32_t h100 = sm_hash_ijk(i0+1, j0,   k0,   config->hashmap_size);
            uint32_t h101 = sm_hash_ijk(i0+1, j0,   k0+1, config->hashmap_size);
            uint32_t h110 = sm_hash_ijk(i0+1, j0+1, k0,   config->hashmap_size);
            uint32_t h111 = sm_hash_ijk(i0+1, j0+1, k0+1, config->hashmap_size);

            /* Precompute base addresses for each corner (stride by features_per_entry) */
            uint32_t fpe = config->features_per_entry;
            uint32_t base000 = level_offset + h000 * fpe;
            uint32_t base001 = level_offset + h001 * fpe;
            uint32_t base010 = level_offset + h010 * fpe;
            uint32_t base011 = level_offset + h011 * fpe;
            uint32_t base100 = level_offset + h100 * fpe;
            uint32_t base101 = level_offset + h101 * fpe;
            uint32_t base110 = level_offset + h110 * fpe;
            uint32_t base111 = level_offset + h111 * fpe;

            {
                int prefetch_mode = sm_hashgrid_prefetch_mode();
                if (prefetch_mode == SM_HASHGRID_PREFETCH_IMMEDIATE) {
                    /* Baseline behavior: prefetch the same ray's 8 corners. */
                    SM_PREFETCH(&hashgrid_data[base000]);
                    SM_PREFETCH(&hashgrid_data[base001]);
                    SM_PREFETCH(&hashgrid_data[base010]);
                    SM_PREFETCH(&hashgrid_data[base011]);
                    SM_PREFETCH(&hashgrid_data[base100]);
                    SM_PREFETCH(&hashgrid_data[base101]);
                    SM_PREFETCH(&hashgrid_data[base110]);
                    SM_PREFETCH(&hashgrid_data[base111]);
                } else if (prefetch_mode == SM_HASHGRID_PREFETCH_IMMEDIATE_4) {
                    /* Reduced variant: only touch the 4 base corners first. */
                    SM_PREFETCH(&hashgrid_data[base000]);
                    SM_PREFETCH(&hashgrid_data[base001]);
                    SM_PREFETCH(&hashgrid_data[base010]);
                    SM_PREFETCH(&hashgrid_data[base011]);
                } else if (prefetch_mode == SM_HASHGRID_PREFETCH_NEXT_RAY && ray + 1 < SIMD_BATCH_SIZE) {
                    /* Probe whether moving the prefetch earlier helps Zen more
                     * than the current same-iteration placement. */
                    Vec3 next_p = positions[ray + 1];
                    float next_gx = next_p.x * res;
                    float next_gy = next_p.y * res;
                    float next_gz = next_p.z * res;
                    int32_t next_i0 = (int32_t)floorf(next_gx);
                    int32_t next_j0 = (int32_t)floorf(next_gy);
                    int32_t next_k0 = (int32_t)floorf(next_gz);
                    uint32_t next_h000 = sm_hash_ijk(next_i0,   next_j0,   next_k0,   config->hashmap_size);
                    uint32_t next_h001 = sm_hash_ijk(next_i0,   next_j0,   next_k0+1, config->hashmap_size);
                    uint32_t next_h010 = sm_hash_ijk(next_i0,   next_j0+1, next_k0,   config->hashmap_size);
                    uint32_t next_h011 = sm_hash_ijk(next_i0,   next_j0+1, next_k0+1, config->hashmap_size);
                    uint32_t next_h100 = sm_hash_ijk(next_i0+1, next_j0,   next_k0,   config->hashmap_size);
                    uint32_t next_h101 = sm_hash_ijk(next_i0+1, next_j0,   next_k0+1, config->hashmap_size);
                    uint32_t next_h110 = sm_hash_ijk(next_i0+1, next_j0+1, next_k0,   config->hashmap_size);
                    uint32_t next_h111 = sm_hash_ijk(next_i0+1, next_j0+1, next_k0+1, config->hashmap_size);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h000 * fpe]);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h001 * fpe]);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h010 * fpe]);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h011 * fpe]);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h100 * fpe]);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h101 * fpe]);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h110 * fpe]);
                    SM_PREFETCH(&hashgrid_data[level_offset + next_h111 * fpe]);
                }
            }

            /* For each feature, do trilinear interpolation over 8 corners */
            for (uint32_t f = 0; f < fpe; f++) {
                /* Look up feature values at each corner */
                float v000 = hashgrid_data[base000 + f];
                float v001 = hashgrid_data[base001 + f];
                float v010 = hashgrid_data[base010 + f];
                float v011 = hashgrid_data[base011 + f];
                float v100 = hashgrid_data[base100 + f];
                float v101 = hashgrid_data[base101 + f];
                float v110 = hashgrid_data[base110 + f];
                float v111 = hashgrid_data[base111 + f];
                
                /* Trilinear interpolation */
                /* First lerp in Z */
                float v00 = v000 * (1.0f - wz) + v001 * wz;
                float v01 = v010 * (1.0f - wz) + v011 * wz;
                float v10 = v100 * (1.0f - wz) + v101 * wz;
                float v11 = v110 * (1.0f - wz) + v111 * wz;
                
                /* Then lerp in Y */
                float v0 = v00 * (1.0f - wy) + v01 * wy;
                float v1 = v10 * (1.0f - wy) + v11 * wy;
                
                /* Finally lerp in X */
                float val = v0 * (1.0f - wx) + v1 * wx;

                /* Bug fix: was `level * 2` — hardcoded stride breaks when
                 * features_per_entry != 2. Use config->features_per_entry. */
                features_out[ray][level * config->features_per_entry + f] = val;
            }
        }
    }
}
/* ===== Batched Occupancy Lookup ===== */

void sm_occupancy_lookup_batch(
    const Vec3 positions[SIMD_BATCH_SIZE],
    const NeRFConfig *config,
    const uint8_t *occ_grid,
    uint8_t occupancy_out[SIMD_BATCH_SIZE]
) {
    for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        /* Transform position to occupancy grid coordinates [0, 64) */
        Vec3 p = positions[ray];
        
        /* Normalize to [0, 1] relative to bounds */
        float bounds = config->scale;
        Vec3 normalized;
        normalized.x = (p.x - config->center.x) / bounds + 0.5f;
        normalized.y = (p.y - config->center.y) / bounds + 0.5f;
        normalized.z = (p.z - config->center.z) / bounds + 0.5f;
        
        /* Clamp to [0, 1] */
        normalized.x = fmaxf(0.0f, fminf(1.0f, normalized.x));
        normalized.y = fmaxf(0.0f, fminf(1.0f, normalized.y));
        normalized.z = fmaxf(0.0f, fminf(1.0f, normalized.z));
        
        /* Scale to [0, 63] */
        uint32_t xi = (uint32_t)(normalized.x * 63.0f);
        uint32_t yi = (uint32_t)(normalized.y * 63.0f);
        uint32_t zi = (uint32_t)(normalized.z * 63.0f);
        
        /* Linear index into 64^3 grid */
        uint32_t idx = zi * 64 * 64 + yi * 64 + xi;
        occupancy_out[ray] = occ_grid[idx];
    }
}

/* ===== Batched MLP Inference (cache-friendly loop order) ===== */

void sm_mlp_inference_batch(
    const float features_in[SIMD_BATCH_SIZE][27],
    const NeRFConfig *config,
    const float *mlp_weights,
    const float *mlp_biases,
    float rgb_out[SIMD_BATCH_SIZE][3],
    float sigma_out[SIMD_BATCH_SIZE]
) {
    uint32_t in_dim = config->mlp_in_dim;
    uint32_t hidden_dim = config->mlp_hidden_dim;
    uint32_t out_dim = config->mlp_out_dim;
    
    /* Guard: batch hidden arrays are sized for hidden_dim up to 128.
     * Wider networks are not supported by this fast path. */
    if (hidden_dim > 128) {
        fprintf(stderr, "[NeRF] ERROR: hidden_dim=%u exceeds batch inference limit (128)\n", hidden_dim);
        return;
    }
    /* Hidden layer output [8 rays][hidden_dim] */
    float hidden[SIMD_BATCH_SIZE][128];
    
    /* Get weight pointers — layout is [in_dim][hidden_dim] (row-major, CPU convention).
     * NOTE: the GPU kernel (mlp_forward.cu) uses [hidden_dim][in_dim] — TRANSPOSED.
     * Weights loaded from .smb files use this CPU [in][hidden] layout. */
    const float *w0 = mlp_weights;
    const float *b0 = mlp_biases;
    const float *w1 = w0 + hidden_dim * in_dim;
    const float *b1 = b0 + hidden_dim;
    const float *w_out = w1 + hidden_dim * hidden_dim;
    const float *b_out = b1 + hidden_dim;
    
    /* Layer 0: Input -> Hidden (27 -> 64)
     * Restructured: iterate inputs OUTER so w0[i * hidden_dim + h] scans
     * contiguously over h in the inner loop — reads full cache lines. */
    for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        for (uint32_t h = 0; h < hidden_dim; h++) {
            hidden[ray][h] = b0[h];
        }
    }
    for (uint32_t i = 0; i < in_dim; i++) {
        const float *w0_row = &w0[i * hidden_dim];
        for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
            __m256 vfi = _mm256_set1_ps(features_in[ray][i]);
            for (uint32_t h = 0; h < hidden_dim; h += 8) {
                __m256 vh = _mm256_loadu_ps(&hidden[ray][h]);
                __m256 vw = _mm256_loadu_ps(&w0_row[h]);
                vh = _mm256_fmadd_ps(vw, vfi, vh);
                _mm256_storeu_ps(&hidden[ray][h], vh);
            }
        }
    }
    for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        __m256 vzero = _mm256_setzero_ps();
        for (uint32_t h = 0; h < hidden_dim; h += 8) {
            __m256 vh = _mm256_loadu_ps(&hidden[ray][h]);
            _mm256_storeu_ps(&hidden[ray][h], _mm256_max_ps(vh, vzero));
        }
    }
    
    /* Layer 1: Hidden -> Hidden (64 -> 64) with ReLU — same cache-friendly pattern */
    float hidden2[SIMD_BATCH_SIZE][128];
    for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        for (uint32_t h = 0; h < hidden_dim; h++) {
            hidden2[ray][h] = b1[h];
        }
    }
    for (uint32_t h_prev = 0; h_prev < hidden_dim; h_prev++) {
        const float *w1_row = &w1[h_prev * hidden_dim];
        for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
            __m256 vval = _mm256_set1_ps(hidden[ray][h_prev]);
            for (uint32_t h = 0; h < hidden_dim; h += 8) {
                __m256 vh = _mm256_loadu_ps(&hidden2[ray][h]);
                __m256 vw = _mm256_loadu_ps(&w1_row[h]);
                vh = _mm256_fmadd_ps(vw, vval, vh);
                _mm256_storeu_ps(&hidden2[ray][h], vh);
            }
        }
    }
    for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        __m256 vzero = _mm256_setzero_ps();
        for (uint32_t h = 0; h < hidden_dim; h += 8) {
            __m256 vh = _mm256_loadu_ps(&hidden2[ray][h]);
            _mm256_storeu_ps(&hidden2[ray][h], _mm256_max_ps(vh, vzero));
        }
    }
    
    /* Output layer: Hidden -> Output — scan w_out rows contiguously.
     * mlp_num_layers > 2 not yet implemented; warn if mismatch. */
    if (config->mlp_num_layers != 2) {
        fprintf(stderr, "[NeRF] WARNING: mlp_num_layers=%u but inference is hardcoded for 2 hidden layers\n",
                config->mlp_num_layers);
    }
    float out_acc[SIMD_BATCH_SIZE][4];
    for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        for (uint32_t o = 0; o < out_dim && o < 4; o++) {
            out_acc[ray][o] = b_out[o];
        }
    }
    for (uint32_t h = 0; h < hidden_dim; h++) {
        const float *wout_row = &w_out[h * out_dim]; /* contiguous row */
        for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
            float hv = hidden2[ray][h];
            for (uint32_t o = 0; o < out_dim && o < 4; o++) {
                out_acc[ray][o] += wout_row[o] * hv;
            }
        }
    }
    /* Apply activations */
    for (uint32_t ray = 0; ray < SIMD_BATCH_SIZE; ray++) {
        for (uint32_t o = 0; o < out_dim; o++) {
            float val = out_acc[ray][o];
            if (o < 3) {
                float sigmoid = 1.0f / (1.0f + expf(-val));
                rgb_out[ray][o] = sigmoid;
            } else {
                float sigma;
                if (val > 20.0f) {
                    sigma = val;
                } else if (val < -20.0f) {
                    sigma = 0.0f;
                } else {
                    sigma = logf(1.0f + expf(val));
                }
                sigma_out[ray] = fminf(50.0f, sigma);
            }
        }
    }
}

/* Single-ray MLP inference optimized for single-lane execution.
 * Uses weight access patterns that favor the stored layout [in_dim, hidden_dim]
 * and [hidden_dim, hidden_dim] to improve cache locality. */
void sm_mlp_inference_single(
    const float features_in[27],
    const NeRFConfig *config,
    const float *mlp_weights,
    const float *mlp_biases,
    float rgb_out[3],
    float *sigma_out
) {
    uint32_t in_dim = config->mlp_in_dim;
    uint32_t hidden_dim = config->mlp_hidden_dim;
    uint32_t out_dim = config->mlp_out_dim;

    /* Guard: hidden[] and hidden2[] are stack arrays of size 128.
     * Also out_acc[] is size 16. Reject configs that would overflow. */
    if (hidden_dim > 128) {
        fprintf(stderr, "[NeRF] ERROR: hidden_dim=%u exceeds single inference limit (128)\n", hidden_dim);
        return;
    }
    if (out_dim > 16) {
        fprintf(stderr, "[NeRF] ERROR: mlp_out_dim=%u exceeds single inference limit (16)\n", out_dim);
        return;
    }

    /* Warn if the saved model has a depth we can't handle correctly. */
    if (config->mlp_num_layers != 2) {
        fprintf(stderr, "[NeRF] WARNING: mlp_num_layers=%u but single inference is hardcoded for 2 hidden layers\n",
                config->mlp_num_layers);
    }

    /* Pointers into packed weights/biases (same layout as batch version) */
    const float *w0 = mlp_weights; /* [in_dim][hidden_dim] */
    const float *b0 = mlp_biases;  /* [hidden_dim] */
    const float *w1 = w0 + (size_t)hidden_dim * in_dim; /* [hidden_dim][hidden_dim] */
    const float *b1 = b0 + hidden_dim; /* [hidden_dim] */
    const float *w_out = w1 + (size_t)hidden_dim * hidden_dim; /* [hidden_dim][out_dim] */
    const float *b_out = b1 + hidden_dim; /* [out_dim] */

    /* Layer 0: Input -> Hidden
     * Compute hidden = b0 + sum_i features_in[i] * w0[i]
     * We iterate inputs outer so w0 row is contiguous over hidden_dim. */
    float hidden[128]; /* allocate a bit more, but only use hidden_dim */

#ifdef __AVX2__
    /* Vectorized init: load biases into hidden */
    uint32_t h = 0;
    for (; h + 7 < hidden_dim; h += 8) {
        __m256 v = _mm256_loadu_ps(&b0[h]);
        _mm256_storeu_ps(&hidden[h], v);
    }
    for (; h < hidden_dim; ++h) hidden[h] = b0[h];

    /* Vectorized input->hidden: for each input, fused multiply-add across hidden dim */
    for (uint32_t i = 0; i < in_dim; i++) {
        float fi = features_in[i];
        __m256 fvec = _mm256_set1_ps(fi);
        const float *w0_row = &w0[(size_t)i * hidden_dim];
        uint32_t hh = 0;
        for (; hh + 7 < hidden_dim; hh += 8) {
            __m256 wvec = _mm256_loadu_ps(&w0_row[hh]);
            __m256 acc = _mm256_loadu_ps(&hidden[hh]);
            acc = _mm256_add_ps(_mm256_mul_ps(wvec, fvec), acc);
            _mm256_storeu_ps(&hidden[hh], acc);
        }
        for (; hh < hidden_dim; ++hh) hidden[hh] += w0_row[hh] * fi;
    }

    /* ReLU */
    for (uint32_t hh = 0; hh < hidden_dim; ++hh) {
        hidden[hh] = hidden[hh] > 0.0f ? hidden[hh] : 0.0f;
    }
#else
    for (uint32_t h = 0; h < hidden_dim; h++) hidden[h] = b0[h];

    for (uint32_t i = 0; i < in_dim; i++) {
        float fi = features_in[i];
        const float *w0_row = &w0[(size_t)i * hidden_dim];
        for (uint32_t h = 0; h < hidden_dim; h++) {
            hidden[h] += w0_row[h] * fi;
        }
    }
    for (uint32_t h = 0; h < hidden_dim; h++) {
        hidden[h] = hidden[h] > 0.0f ? hidden[h] : 0.0f;
    }
#endif

    /* Layer 1: Hidden -> Hidden
     * Compute hidden2 = b1 + sum_hprev hidden[hprev] * w1[hprev]
     * Vectorize over hidden dimension when AVX2 is available. */
    float hidden2[128];
#ifdef __AVX2__
    uint32_t hh = 0;
    for (; hh + 7 < hidden_dim; hh += 8) {
        __m256 v = _mm256_loadu_ps(&b1[hh]);
        _mm256_storeu_ps(&hidden2[hh], v);
    }
    for (; hh < hidden_dim; ++hh) hidden2[hh] = b1[hh];

    for (uint32_t hprev = 0; hprev < hidden_dim; hprev++) {
        float val = hidden[hprev];
        __m256 val_vec = _mm256_set1_ps(val);
        const float *w1_row = &w1[(size_t)hprev * hidden_dim];
        uint32_t h2 = 0;
        for (; h2 + 7 < hidden_dim; h2 += 8) {
            __m256 wvec = _mm256_loadu_ps(&w1_row[h2]);
            __m256 acc = _mm256_loadu_ps(&hidden2[h2]);
            acc = _mm256_add_ps(_mm256_mul_ps(wvec, val_vec), acc);
            _mm256_storeu_ps(&hidden2[h2], acc);
        }
        for (; h2 < hidden_dim; ++h2) hidden2[h2] += w1_row[h2] * val;
    }

    for (uint32_t h3 = 0; h3 < hidden_dim; ++h3) hidden2[h3] = hidden2[h3] > 0.0f ? hidden2[h3] : 0.0f;
#else
    for (uint32_t h = 0; h < hidden_dim; h++) hidden2[h] = b1[h];

    for (uint32_t hprev = 0; hprev < hidden_dim; hprev++) {
        float val = hidden[hprev];
        const float *w1_row = &w1[(size_t)hprev * hidden_dim];
        for (uint32_t h = 0; h < hidden_dim; h++) {
            hidden2[h] += w1_row[h] * val;
        }
    }
    for (uint32_t h = 0; h < hidden_dim; h++) {
        hidden2[h] = hidden2[h] > 0.0f ? hidden2[h] : 0.0f;
    }
#endif

    /* Output layer: hidden2 -> out
     * We accumulate per-output in a small array for cache-friendly writes. */
    float out_acc[16];
    for (uint32_t o = 0; o < out_dim; o++) out_acc[o] = b_out[o];

    for (uint32_t h = 0; h < hidden_dim; h++) {
        const float *wout_row = &w_out[(size_t)h * out_dim];
        float hv = hidden2[h];
        for (uint32_t o = 0; o < out_dim; o++) {
            out_acc[o] += wout_row[o] * hv;
        }
    }

    /* Apply activations: first 3 are RGB (sigmoid), last is sigma (softplus-like).
     * Fix: initialise sigma_out to 0 before the loop so it is never left
     * unwritten when out_dim < 4 (e.g. RGB-only checkpoint). */
    *sigma_out = 0.0f;
    for (uint32_t o = 0; o < out_dim; o++) {
        float val = out_acc[o];
        if (o < 3) {
            rgb_out[o] = 1.0f / (1.0f + expf(-val));
        } else {
            float sigma;
            if (val > 20.0f) sigma = val;
            else if (val < -20.0f) sigma = 0.0f;
            else sigma = logf(1.0f + expf(val));
            *sigma_out = sigma;
        }
    }
}

/* ===== Adaptive Sampling ===== */

float sm_adaptive_step_size(
    const Vec3 pos,
    const uint8_t *occ_grid,
    const NeRFConfig *config,
    float base_step
) {
    /* Quantize to occupancy grid */
    float bounds = config->scale;
    Vec3 normalized;
    normalized.x = (pos.x - config->center.x) / bounds + 0.5f;
    normalized.y = (pos.y - config->center.y) / bounds + 0.5f;
    normalized.z = (pos.z - config->center.z) / bounds + 0.5f;
    
    normalized.x = fmaxf(0.0f, fminf(1.0f, normalized.x));
    normalized.y = fmaxf(0.0f, fminf(1.0f, normalized.y));
    normalized.z = fmaxf(0.0f, fminf(1.0f, normalized.z));
    
    uint32_t xi = (uint32_t)(normalized.x * 63.0f);
    uint32_t yi = (uint32_t)(normalized.y * 63.0f);
    uint32_t zi = (uint32_t)(normalized.z * 63.0f);
    
    /* Clamp to valid range */
    if (xi >= 64) xi = 63;
    if (yi >= 64) yi = 63;
    if (zi >= 64) zi = 63;
    
    uint32_t idx = zi * 64 * 64 + yi * 64 + xi;
    uint8_t occupancy = occ_grid[idx];
    
    /* Threshold: if LOW occupancy (empty space), use LARGER step */
    const uint8_t OCCUPANCY_THRESHOLD = 32;  /* Out of 255 */
    
    if (occupancy < OCCUPANCY_THRESHOLD) {
        return base_step * 3.0f;  /* Skip empty space faster */
    }
    return base_step;  /* Fine sampling in occupied regions */
}

bool sm_ray_should_terminate(float accumulated_alpha) {
    /* Stop when near-full opacity reached */
    if (accumulated_alpha > 0.99f) return true;
    
    return false;
}

typedef struct {
    uint64_t rays_started;
    uint64_t steps_attempted;
    uint64_t steps_completed;
    uint64_t rays_terminated_early;
    uint64_t ns_adaptive;
    uint64_t ns_hashgrid;
    uint64_t ns_mlp;
    uint64_t ns_composite;
    uint64_t ns_writeback;
    uint64_t ns_other;
} NeRFPhaseTimingSlot;

#define SM_PHASE_TIMING_SLOTS 256

static NeRFPhaseTimingSlot g_nerf_phase_timing[SM_PHASE_TIMING_SLOTS];
static int g_nerf_phase_timing_enabled = -1;

static uint64_t sm_now_ns_monotonic(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int sm_nerf_phase_timing_enabled(void) {
    if (g_nerf_phase_timing_enabled >= 0) {
        return g_nerf_phase_timing_enabled;
    }
    g_nerf_phase_timing_enabled = 0;
    {
        const char *env = getenv("SM_NERF_PHASE_TIMING");
        if (env && env[0] && strcmp(env, "0") != 0) {
            g_nerf_phase_timing_enabled = 1;
        }
    }
    return g_nerf_phase_timing_enabled;
}

static NeRFPhaseTimingSlot *sm_nerf_phase_slot(void) {
    unsigned int slot = 0;
#ifdef _OPENMP
    if (omp_in_parallel()) {
        slot = (unsigned int)omp_get_thread_num();
    }
#endif
    if (slot >= SM_PHASE_TIMING_SLOTS) {
        slot = SM_PHASE_TIMING_SLOTS - 1;
    }
    return &g_nerf_phase_timing[slot];
}

void sm_nerf_phase_timing_reset(void) {
    memset(g_nerf_phase_timing, 0, sizeof(g_nerf_phase_timing));
}

NeRFPhaseTiming sm_nerf_phase_timing_snapshot(void) {
    NeRFPhaseTiming total = {0};
    int i;
    for (i = 0; i < SM_PHASE_TIMING_SLOTS; i++) {
        total.rays_started += g_nerf_phase_timing[i].rays_started;
        total.steps_attempted += g_nerf_phase_timing[i].steps_attempted;
        total.steps_completed += g_nerf_phase_timing[i].steps_completed;
        total.rays_terminated_early += g_nerf_phase_timing[i].rays_terminated_early;
        total.ns_adaptive += g_nerf_phase_timing[i].ns_adaptive;
        total.ns_hashgrid += g_nerf_phase_timing[i].ns_hashgrid;
        total.ns_mlp += g_nerf_phase_timing[i].ns_mlp;
        total.ns_composite += g_nerf_phase_timing[i].ns_composite;
        total.ns_writeback += g_nerf_phase_timing[i].ns_writeback;
        total.ns_other += g_nerf_phase_timing[i].ns_other;
    }
    return total;
}

void sm_nerf_phase_timing_report(const char *label) {
    NeRFPhaseTiming total = sm_nerf_phase_timing_snapshot();
    double total_ms = (double)(total.ns_adaptive + total.ns_hashgrid + total.ns_mlp +
                               total.ns_composite + total.ns_writeback + total.ns_other) / 1000000.0;
    double step_count = total.steps_completed ? (double)total.steps_completed : 1.0;
    const char *tag = label ? label : "phase";
    printf("[PHASE] %s rays=%" PRIu64 " attempted_steps=%" PRIu64 " completed_steps=%" PRIu64
           " early_terminated=%" PRIu64 " total_ms=%.3f\n",
           tag,
           total.rays_started,
           total.steps_attempted,
           total.steps_completed,
           total.rays_terminated_early,
           total_ms);
    printf("[PHASE] %s adaptive_ms=%.3f hashgrid_ms=%.3f mlp_ms=%.3f composite_ms=%.3f writeback_ms=%.3f other_ms=%.3f\n",
           tag,
           (double)total.ns_adaptive / 1000000.0,
           (double)total.ns_hashgrid / 1000000.0,
           (double)total.ns_mlp / 1000000.0,
           (double)total.ns_composite / 1000000.0,
           (double)total.ns_writeback / 1000000.0,
           (double)total.ns_other / 1000000.0);
    printf("[PHASE] %s adaptive_ns_per_step=%.2f hashgrid_ns_per_step=%.2f mlp_ns_per_step=%.2f composite_ns_per_step=%.2f other_ns_per_step=%.2f\n",
           tag,
           (double)total.ns_adaptive / step_count,
           (double)total.ns_hashgrid / step_count,
           (double)total.ns_mlp / step_count,
           (double)total.ns_composite / step_count,
           (double)total.ns_other / step_count);
}

/* ===== Volume Integration (Main Rendering Kernel) ===== */

void sm_volume_integrate_batch(
    const RayBatch *batch,
    const NeRFConfig *config,
    const NeRFData *nerf_data,
    NeRFFramebuffer *output_fb,
    uint32_t num_steps,
    float density_scale,
    float bounds_max
) {
    /* Early return if num_steps is zero — avoids division by zero in base_step
     * and a degenerate ray-march with no samples. */
    if (num_steps == 0) return;

    /* Process each ray independently within batch */
    /* Precompute level scales via iterative multiply (avoids powf per level). */
    float level_scales[20];
    uint32_t num_levels_clamped = config->num_levels < 20 ? config->num_levels : 20;
    {
        float scale = (float)config->base_res;
        for (uint32_t l = 0; l < num_levels_clamped; l++) {
            level_scales[l] = scale;
            scale *= config->per_level_scale;
        }
    }
    
    /* Parallelize ray loop with OpenMP if available */
    #pragma omp parallel for schedule(dynamic, 1) if(_OPENMP)
    for (int ray_idx_signed = 0; ray_idx_signed < (int)batch->count; ray_idx_signed++) {
        uint32_t ray_idx = (uint32_t)ray_idx_signed;
        NeRFPhaseTimingSlot *phase_slot = NULL;
        uint64_t phase_t0 = 0;
        uint64_t phase_t1 = 0;
        uint64_t ray_start_ns = 0;
        uint64_t ray_adaptive_ns = 0;
        uint64_t ray_hashgrid_ns = 0;
        uint64_t ray_mlp_ns = 0;
        uint64_t ray_composite_ns = 0;
        uint64_t ray_writeback_ns = 0;
        if (!batch->active[ray_idx]) continue;
        if (sm_nerf_phase_timing_enabled()) {
            phase_slot = sm_nerf_phase_slot();
            phase_slot->rays_started++;
            ray_start_ns = sm_now_ns_monotonic();
        }
        
        Vec3 origin = batch->origin[ray_idx];
        Vec3 direction = batch->direction[ray_idx];
        float t_min = batch->tmin[ray_idx];
        float t_max = batch->tmax[ray_idx];
        uint32_t pixel_id = batch->pixel_id[ray_idx];
        
        /* Initialize accumulation */
        float accumulated_rgb[3] = {0.0f, 0.0f, 0.0f};
        float accumulated_alpha = 0.0f;
        float base_step = (bounds_max * 2.0f) / (float)num_steps;
        float dir_len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

        /* Ray marching loop — t is accumulated so adaptive step_size actually
         * advances the ray position, not just the alpha weighting. */
        float t = t_min;
        for (uint32_t step = 0; step < num_steps; step++) {
            if (phase_slot) {
                phase_slot->steps_attempted++;
            }
            if (t > t_max) break;
            
            Vec3 pos;
            pos.x = origin.x + direction.x * t;
            pos.y = origin.y + direction.y * t;
            pos.z = origin.z + direction.z * t;
            
            /* Adaptive step size — used both for alpha computation AND to advance t. */
            if (phase_slot) phase_t0 = sm_now_ns_monotonic();
            float step_size = sm_adaptive_step_size(pos, nerf_data->occupancy_grid, config, base_step);
            if (phase_slot) {
                phase_t1 = sm_now_ns_monotonic();
                ray_adaptive_ns += phase_t1 - phase_t0;
            }
            
            /* Create feature vector for this ray step */
            float feat[27];
            memset(feat, 0, sizeof(feat));

            /* Normalize position to scene bounds [-1, 1] then to [0, 1] like GPU shader */
            Vec3 norm_pos;
            norm_pos.x = (pos.x - config->center.x) / config->scale;
            norm_pos.y = (pos.y - config->center.y) / config->scale;
            norm_pos.z = (pos.z - config->center.z) / config->scale;
            
            /* Convert to [0, 1] and clamp */
            Vec3 pn;
            pn.x = fmaxf(0.0f, fminf(1.0f, norm_pos.x * 0.5f + 0.5f));
            pn.y = fmaxf(0.0f, fminf(1.0f, norm_pos.y * 0.5f + 0.5f));
            pn.z = fmaxf(0.0f, fminf(1.0f, norm_pos.z * 0.5f + 0.5f));

            /* Hashgrid features per level with trilinear interpolation */
            if (phase_slot) phase_t0 = sm_now_ns_monotonic();
            for (uint32_t level = 0; level < num_levels_clamped; level++) {
                /* Use precomputed level scale */
                float res = level_scales[level];
                uint32_t level_offset = level * config->hashmap_size * config->features_per_entry;
                
                /* Scale position to grid coordinates */
                float gx = pn.x * res;
                float gy = pn.y * res;
                float gz = pn.z * res;
                
                /* Get integer corner and fractional weights */
                float fx = floorf(gx);
                float fy = floorf(gy);
                float fz = floorf(gz);
                int32_t i0 = (int32_t)fx;
                int32_t j0 = (int32_t)fy;
                int32_t k0 = (int32_t)fz;
                
                /* Interpolation weights */
                float wx = gx - fx;
                float wy = gy - fy;
                float wz = gz - fz;
                
                /* For each feature, do trilinear interpolation over 8 corners */
                for (uint32_t f = 0; f < config->features_per_entry; f++) {
                    /* Hash all 8 corners */
                    uint32_t h000 = sm_hash_ijk(i0,   j0,   k0,   config->hashmap_size);
                    uint32_t h001 = sm_hash_ijk(i0,   j0,   k0+1, config->hashmap_size);
                    uint32_t h010 = sm_hash_ijk(i0,   j0+1, k0,   config->hashmap_size);
                    uint32_t h011 = sm_hash_ijk(i0,   j0+1, k0+1, config->hashmap_size);
                    uint32_t h100 = sm_hash_ijk(i0+1, j0,   k0,   config->hashmap_size);
                    uint32_t h101 = sm_hash_ijk(i0+1, j0,   k0+1, config->hashmap_size);
                    uint32_t h110 = sm_hash_ijk(i0+1, j0+1, k0,   config->hashmap_size);
                    uint32_t h111 = sm_hash_ijk(i0+1, j0+1, k0+1, config->hashmap_size);
                    
                    {
                        int prefetch_mode = sm_hashgrid_prefetch_mode();
                        /* Keep the current behavior as the default and let the
                         * benchmark lane disable it without recompiling. */
                        if (prefetch_mode == SM_HASHGRID_PREFETCH_IMMEDIATE ||
                            prefetch_mode == SM_HASHGRID_PREFETCH_NEXT_RAY) {
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h000 * config->features_per_entry]);
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h100 * config->features_per_entry]);
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h010 * config->features_per_entry]);
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h110 * config->features_per_entry]);
                        } else if (prefetch_mode == SM_HASHGRID_PREFETCH_IMMEDIATE_4) {
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h000 * config->features_per_entry]);
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h001 * config->features_per_entry]);
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h010 * config->features_per_entry]);
                            SM_PREFETCH(&nerf_data->hashgrid_data[level_offset + h011 * config->features_per_entry]);
                        }
                    }

                    /* Look up feature values at each corner */
                    float v000 = nerf_data->hashgrid_data[level_offset + h000 * config->features_per_entry + f];
                    float v001 = nerf_data->hashgrid_data[level_offset + h001 * config->features_per_entry + f];
                    float v010 = nerf_data->hashgrid_data[level_offset + h010 * config->features_per_entry + f];
                    float v011 = nerf_data->hashgrid_data[level_offset + h011 * config->features_per_entry + f];
                    float v100 = nerf_data->hashgrid_data[level_offset + h100 * config->features_per_entry + f];
                    float v101 = nerf_data->hashgrid_data[level_offset + h101 * config->features_per_entry + f];
                    float v110 = nerf_data->hashgrid_data[level_offset + h110 * config->features_per_entry + f];
                    float v111 = nerf_data->hashgrid_data[level_offset + h111 * config->features_per_entry + f];
                    
                    /* Trilinear interpolation */
                    float v00 = v000 * (1.0f - wz) + v001 * wz;
                    float v01 = v010 * (1.0f - wz) + v011 * wz;
                    float v10 = v100 * (1.0f - wz) + v101 * wz;
                    float v11 = v110 * (1.0f - wz) + v111 * wz;
                    float v0 = v00 * (1.0f - wy) + v01 * wy;
                    float v1 = v10 * (1.0f - wy) + v11 * wy;
                    float val = v0 * (1.0f - wx) + v1 * wx;
                    
                    /* Bug fix: was hardcoded `level * 2` which is wrong when
                     * features_per_entry != 2. Use features_per_entry as stride. */
                    feat[level * config->features_per_entry + f] = val;
                }
            }
            if (phase_slot) {
                phase_t1 = sm_now_ns_monotonic();
                ray_hashgrid_ns += phase_t1 - phase_t0;
            }

            /* View direction encoded */
            if (dir_len > 1e-6f) {
                feat[24] = direction.x / dir_len * 0.5f + 0.5f;  /* Normalize to [0, 1] */
                feat[25] = direction.y / dir_len * 0.5f + 0.5f;
                feat[26] = direction.z / dir_len * 0.5f + 0.5f;
            }
            
            /* MLP inference for this ray step - use single-ray fast path */
            float rgb_step_scalar[3];
            float sigma_step_scalar = 0.0f;

            if (phase_slot) phase_t0 = sm_now_ns_monotonic();
#ifdef __AVX512BF16__
            if (sm_nerf_mlp_variant() == SM_NERF_MLP_VARIANT_BF16 &&
                nerf_data->mlp_bf16_ready) {
                sm_mlp_inference_single_bf16(
                    feat,
                    nerf_data,
                    rgb_step_scalar,
                    &sigma_step_scalar
                );
            } else
#endif
#ifdef __AVX2__
            if (sm_nerf_mlp_variant() == SM_NERF_MLP_VARIANT_PREPACKED &&
                nerf_data->mlp_prepacked_ready) {
                sm_mlp_inference_single_prepacked_data(
                    feat,
                    nerf_data,
                    rgb_step_scalar,
                    &sigma_step_scalar
                );
            } else
#endif
            {
                sm_mlp_inference_single(
                    feat,
                    config,
                    nerf_data->mlp_weights,
                    nerf_data->mlp_biases,
                    rgb_step_scalar,
                    &sigma_step_scalar
                );
            }
            if (phase_slot) {
                phase_t1 = sm_now_ns_monotonic();
                ray_mlp_ns += phase_t1 - phase_t0;
            }

            /* Volume compositing */
            if (phase_slot) phase_t0 = sm_now_ns_monotonic();
            float sigma = sigma_step_scalar * density_scale;
            float alpha = 1.0f - expf(-sigma * step_size);
            
            /* Accumulate color */
            float weight = alpha * (1.0f - accumulated_alpha);
            accumulated_rgb[0] += rgb_step_scalar[0] * weight;
            accumulated_rgb[1] += rgb_step_scalar[1] * weight;
            accumulated_rgb[2] += rgb_step_scalar[2] * weight;
            accumulated_alpha += weight;
            
            /* Advance ray position by the adaptive step (fixes the bug where t
             * was always t_min + step*base_step, making empty-space skip ineffective). */
            t += step_size;
            if (phase_slot) {
                phase_t1 = sm_now_ns_monotonic();
                ray_composite_ns += phase_t1 - phase_t0;
                phase_slot->steps_completed++;
            }

            /* Early termination */
            if (sm_ray_should_terminate(accumulated_alpha)) {
                if (phase_slot) {
                    phase_slot->rays_terminated_early++;
                }
                break;
            }
        }
        
        /* Write to framebuffer */
        uint32_t px = pixel_id % output_fb->width;
        uint32_t py = pixel_id / output_fb->width;
        
        if (px < output_fb->width && py < output_fb->height) {
            if (phase_slot) phase_t0 = sm_now_ns_monotonic();
            uint32_t fb_idx = py * output_fb->width + px;
            /* Thread-safe write: each pixel written by one thread */
            output_fb->pixels[fb_idx].rgb.x = accumulated_rgb[0];  /* R */
            output_fb->pixels[fb_idx].rgb.y = accumulated_rgb[1];  /* G */
            output_fb->pixels[fb_idx].rgb.z = accumulated_rgb[2];  /* B */
            output_fb->pixels[fb_idx].alpha = accumulated_alpha;
            if (phase_slot) {
                phase_t1 = sm_now_ns_monotonic();
                ray_writeback_ns += phase_t1 - phase_t0;
            }
        }
        if (phase_slot) {
            uint64_t ray_total_ns = sm_now_ns_monotonic() - ray_start_ns;
            uint64_t ray_accounted_ns = ray_adaptive_ns + ray_hashgrid_ns + ray_mlp_ns + ray_composite_ns + ray_writeback_ns;
            phase_slot->ns_adaptive += ray_adaptive_ns;
            phase_slot->ns_hashgrid += ray_hashgrid_ns;
            phase_slot->ns_mlp += ray_mlp_ns;
            phase_slot->ns_composite += ray_composite_ns;
            phase_slot->ns_writeback += ray_writeback_ns;
            if (ray_total_ns > ray_accounted_ns) {
                phase_slot->ns_other += ray_total_ns - ray_accounted_ns;
            }
        }
    }  /* End of parallel region */
}

/* ===== Profiling Utilities ===== */

void sm_perf_start(uint64_t *start_cycle) {
    *start_cycle = sm_rdtsc();
}

void sm_perf_end(uint64_t start_cycle, PerfCounter *counter) {
    uint64_t end_cycle = sm_rdtsc();
    counter->total_cycles += (end_cycle - start_cycle);
    counter->sample_count++;
}

void sm_perf_report(const char *name, const PerfCounter *counter) {
    if (counter->sample_count == 0) return;
    
    double avg_cycles = (double)counter->total_cycles / counter->sample_count;
    double avg_us = avg_cycles / 3000.0;  /* ~3 GHz CPU */
    
    printf("[PERF] %s: %.2f cycles/sample, %.2f µs/sample (%" PRIu64 " samples)\n",
           name, avg_cycles, avg_us, counter->sample_count);
}
