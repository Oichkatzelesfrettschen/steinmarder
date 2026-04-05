#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_reduction.c — Horizontal reduction patterns.
 *
 * Translated from Apple probe_simd_reduce_lat.metal and
 * SASS probe_bar_red_predicate.cu.
 *
 * Measures:
 *   1. _mm512_reduce_add_ps  — full 16-lane float reduction (compiler built-in)
 *   2. _mm512_reduce_max_ps  — full 16-lane float max reduction
 *   3. Manual tree reduction  — vshufps + vaddps pairs (4 stages for 16->1)
 *   4. Cross-lane vpermps reduction using vpermps + vaddps
 *
 * Compares Zen 4 reduction cost against Apple AGX single-cycle simd_reduce.
 * All measured as latency (dependent chain of reductions).
 */

/* Latency of _mm512_reduce_add_ps (compiler-generated sequence) */
static double reduce_add_ps_latency(uint64_t iterations)
{
    __m512 source_vector = _mm512_setr_ps(
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    __asm__ volatile("" : "+x"(source_vector));
    float accumulated_scalar = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        float reduced_value = _mm512_reduce_add_ps(source_vector);
        accumulated_scalar += reduced_value;
        /* Feed the scalar result back to create dependency */
        source_vector = _mm512_set1_ps(accumulated_scalar);
        __asm__ volatile("" : "+x"(source_vector));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_scalar;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations);
}

/* Latency of _mm512_reduce_max_ps */
static double reduce_max_ps_latency(uint64_t iterations)
{
    __m512 source_vector = _mm512_setr_ps(
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    __asm__ volatile("" : "+x"(source_vector));
    float accumulated_scalar = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        float max_value = _mm512_reduce_max_ps(source_vector);
        accumulated_scalar += max_value;
        source_vector = _mm512_set1_ps(accumulated_scalar);
        __asm__ volatile("" : "+x"(source_vector));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_scalar;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations);
}

/*
 * Manual tree reduction using extract256 + vaddps ymm, then 128-bit
 * shuffles down to scalar. This is the canonical efficient pattern:
 *   Stage 1: extract high 256, add to low 256  (16->8)
 *   Stage 2: extract high 128, add to low 128  (8->4)
 *   Stage 3: movehdup + addps                  (4->2)
 *   Stage 4: movehl + addss                    (2->1)
 */
static double manual_tree_reduction_latency(uint64_t iterations)
{
    __m512 source_vector = _mm512_setr_ps(
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    __asm__ volatile("" : "+x"(source_vector));
    float accumulated_scalar = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        /* Stage 1: 512->256 */
        __m256 low_half  = _mm512_castps512_ps256(source_vector);
        __m256 high_half = _mm512_extractf32x8_ps(source_vector, 1);
        __m256 sum_256   = _mm256_add_ps(low_half, high_half);

        /* Stage 2: 256->128 */
        __m128 low_128  = _mm256_castps256_ps128(sum_256);
        __m128 high_128 = _mm256_extractf128_ps(sum_256, 1);
        __m128 sum_128  = _mm_add_ps(low_128, high_128);

        /* Stage 3: 128->64 (pairs) */
        __m128 shuf_stage3 = _mm_movehdup_ps(sum_128);
        __m128 sum_64      = _mm_add_ps(sum_128, shuf_stage3);

        /* Stage 4: 64->32 (scalar) */
        __m128 shuf_stage4 = _mm_movehl_ps(sum_64, sum_64);
        __m128 sum_scalar  = _mm_add_ss(sum_64, shuf_stage4);

        float reduced_value = _mm_cvtss_f32(sum_scalar);
        accumulated_scalar += reduced_value;
        source_vector = _mm512_set1_ps(accumulated_scalar);
        __asm__ volatile("" : "+x"(source_vector));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_scalar;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations);
}

/*
 * Cross-lane vpermps-based reduction: uses vpermps to rotate lanes
 * and add, rather than extract-based narrowing.
 *   Step 1: vpermps to swap 256-bit halves, vaddps (16->8 unique in low)
 *   Step 2: vpermps to swap 128-bit quarters, vaddps (8->4)
 *   Step 3: vpermps to swap 64-bit pairs, vaddps (4->2)
 *   Step 4: vpermps to swap 32-bit elements, vaddps (2->1)
 * All done at 512-bit width.
 */
static double vpermps_reduction_latency(uint64_t iterations)
{
    __m512 source_vector = _mm512_setr_ps(
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    __asm__ volatile("" : "+x"(source_vector));

    /* Permutation indices: swap high/low 256 */
    __m512i perm_swap_256 = _mm512_setr_epi32(8,9,10,11,12,13,14,15, 0,1,2,3,4,5,6,7);
    /* Swap high/low 128 within each 256 */
    __m512i perm_swap_128 = _mm512_setr_epi32(4,5,6,7,0,1,2,3, 12,13,14,15,8,9,10,11);
    /* Swap high/low 64 within each 128 */
    __m512i perm_swap_64  = _mm512_setr_epi32(2,3,0,1,6,7,4,5, 10,11,8,9,14,15,12,13);
    /* Swap adjacent 32-bit elements */
    __m512i perm_swap_32  = _mm512_setr_epi32(1,0,3,2,5,4,7,6, 9,8,11,10,13,12,15,14);

    float accumulated_scalar = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        __m512 permuted_data;

        /* Stage 1: swap 256-bit halves, add */
        permuted_data = _mm512_permutexvar_ps(perm_swap_256, source_vector);
        source_vector = _mm512_add_ps(source_vector, permuted_data);

        /* Stage 2: swap 128-bit quarters, add */
        permuted_data = _mm512_permutexvar_ps(perm_swap_128, source_vector);
        source_vector = _mm512_add_ps(source_vector, permuted_data);

        /* Stage 3: swap 64-bit pairs, add */
        permuted_data = _mm512_permutexvar_ps(perm_swap_64, source_vector);
        source_vector = _mm512_add_ps(source_vector, permuted_data);

        /* Stage 4: swap adjacent 32-bit, add */
        permuted_data = _mm512_permutexvar_ps(perm_swap_32, source_vector);
        source_vector = _mm512_add_ps(source_vector, permuted_data);

        /* Extract scalar result and feed back */
        float reduced_value = _mm512_cvtss_f32(source_vector);
        accumulated_scalar += reduced_value;
        source_vector = _mm512_set1_ps(accumulated_scalar);
        __asm__ volatile("" : "+x"(source_vector));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_scalar;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations);
}

/* Throughput: how many independent reductions per cycle */
static double reduce_add_ps_throughput(uint64_t iterations)
{
    __m512 vec_0 = _mm512_set1_ps(1.0f);
    __m512 vec_1 = _mm512_set1_ps(2.0f);
    __m512 vec_2 = _mm512_set1_ps(3.0f);
    __m512 vec_3 = _mm512_set1_ps(4.0f);
    __asm__ volatile("" : "+x"(vec_0), "+x"(vec_1), "+x"(vec_2), "+x"(vec_3));
    float accumulated_sum = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        float reduced_0 = _mm512_reduce_add_ps(vec_0);
        float reduced_1 = _mm512_reduce_add_ps(vec_1);
        float reduced_2 = _mm512_reduce_add_ps(vec_2);
        float reduced_3 = _mm512_reduce_add_ps(vec_3);
        accumulated_sum += reduced_0 + reduced_1 + reduced_2 + reduced_3;
        __asm__ volatile("" : "+x"(vec_0), "+x"(vec_1), "+x"(vec_2), "+x"(vec_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_sum;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    sm_zen_print_header("reduction", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double reduce_add_lat       = reduce_add_ps_latency(cfg.iterations);
    double reduce_max_lat       = reduce_max_ps_latency(cfg.iterations);
    double manual_tree_lat      = manual_tree_reduction_latency(cfg.iterations);
    double vpermps_reduce_lat   = vpermps_reduction_latency(cfg.iterations);
    double reduce_add_tp        = reduce_add_ps_throughput(cfg.iterations);

    printf("--- Latency (cycles per full 16-lane reduction) ---\n");
    printf("reduce_add_ps_lat=%.4f\n", reduce_add_lat);
    printf("reduce_max_ps_lat=%.4f\n", reduce_max_lat);
    printf("manual_tree_lat=%.4f\n", manual_tree_lat);
    printf("vpermps_reduce_lat=%.4f\n", vpermps_reduce_lat);

    printf("\n--- Throughput (cycles per reduction, 4 independent) ---\n");
    printf("reduce_add_ps_tp=%.4f\n", reduce_add_tp);

    printf("\n--- Summary ---\n");
    printf("reduce_add_ps:     lat=%.2f cy  tp=%.2f cy\n", reduce_add_lat, reduce_add_tp);
    printf("reduce_max_ps:     lat=%.2f cy\n", reduce_max_lat);
    printf("manual tree:       lat=%.2f cy  (extract + add, 4 stages)\n", manual_tree_lat);
    printf("vpermps + vaddps:  lat=%.2f cy  (cross-lane permute, 4 stages)\n", vpermps_reduce_lat);
    printf("\nNote: Apple AGX does 16-lane simd_reduce in 1 cycle.\n");
    printf("Zen 4 requires multi-stage shuffles (~%.0fx slower).\n",
           reduce_add_lat);

    if (cfg.csv)
        printf("csv,probe=reduction,reduce_add_lat=%.4f,reduce_max_lat=%.4f,manual_tree_lat=%.4f,vpermps_lat=%.4f,reduce_add_tp=%.4f\n",
               reduce_add_lat, reduce_max_lat, manual_tree_lat, vpermps_reduce_lat, reduce_add_tp);

    return 0;
}
