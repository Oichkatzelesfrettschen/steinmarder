#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_dot_product.c — Dot product patterns at multiple widths.
 *
 * Translated from Apple probe_dd_lat.metal, r600 alu_dot_product.frag,
 * and SASS dp4a probes.
 *
 * Measures:
 *   1. Manual 16-wide float dot via vmulps zmm + reduce_add
 *   2. 4-wide float dot via vdpps xmm (SSE4.1 dpps)
 *   3. 8-wide float dot via vdpps ymm (two 4-wide dpps in each 128-bit lane)
 *   4. vpdpbusd zmm — int8 dot product (4 uint8 * int8 -> int32, 16 groups)
 *   5. Manual 32-element float dot (two 16-wide vmulps + reduce)
 *
 * Reports cycles per dot product operation at each width.
 */

/*
 * 16-wide float dot: vmulps zmm, zmm -> reduce_add_ps
 * Latency: one multiply + full 16-lane reduction.
 */
static double dot16_float_latency(uint64_t iterations)
{
    __m512 vector_a = _mm512_setr_ps(
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f);
    __m512 vector_b = _mm512_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(vector_a), "+x"(vector_b));
    float accumulated_result = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        __m512 product = _mm512_mul_ps(vector_a, vector_b);
        float dot_result = _mm512_reduce_add_ps(product);
        accumulated_result += dot_result;
        /* Feed back to create dependency chain */
        vector_a = _mm512_set1_ps(accumulated_result);
        __asm__ volatile("" : "+x"(vector_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_result;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations);
}

/* Throughput: 4 independent 16-wide dots */
static double dot16_float_throughput(uint64_t iterations)
{
    __m512 vec_a0 = _mm512_set1_ps(1.0f);
    __m512 vec_a1 = _mm512_set1_ps(2.0f);
    __m512 vec_a2 = _mm512_set1_ps(3.0f);
    __m512 vec_a3 = _mm512_set1_ps(4.0f);
    __m512 vec_b  = _mm512_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(vec_a0), "+x"(vec_a1), "+x"(vec_a2), "+x"(vec_a3), "+x"(vec_b));
    float accumulated_sum = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        __m512 prod_0 = _mm512_mul_ps(vec_a0, vec_b);
        __m512 prod_1 = _mm512_mul_ps(vec_a1, vec_b);
        __m512 prod_2 = _mm512_mul_ps(vec_a2, vec_b);
        __m512 prod_3 = _mm512_mul_ps(vec_a3, vec_b);
        float dot_0 = _mm512_reduce_add_ps(prod_0);
        float dot_1 = _mm512_reduce_add_ps(prod_1);
        float dot_2 = _mm512_reduce_add_ps(prod_2);
        float dot_3 = _mm512_reduce_add_ps(prod_3);
        accumulated_sum += dot_0 + dot_1 + dot_2 + dot_3;
        __asm__ volatile("" : "+x"(vec_a0), "+x"(vec_a1), "+x"(vec_a2), "+x"(vec_a3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_sum;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

/*
 * 4-wide float dot: vdpps xmm (SSE4.1 dpps instruction)
 * dpps computes a 4-element dot product in a single instruction.
 */
static double dot4_dpps_latency(uint64_t iterations)
{
    __m128 vector_a = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 vector_b = _mm_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(vector_a), "+x"(vector_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        /* 0xFF: use all 4 input elements, write result to all 4 output lanes */
        vector_a = _mm_dp_ps(vector_a, vector_b, 0xFF);
        __asm__ volatile("" : "+x"(vector_a));
        vector_a = _mm_dp_ps(vector_a, vector_b, 0xFF);
        __asm__ volatile("" : "+x"(vector_a));
        vector_a = _mm_dp_ps(vector_a, vector_b, 0xFF);
        __asm__ volatile("" : "+x"(vector_a));
        vector_a = _mm_dp_ps(vector_a, vector_b, 0xFF);
        __asm__ volatile("" : "+x"(vector_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm_cvtss_f32(vector_a);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double dot4_dpps_throughput(uint64_t iterations)
{
    __m128 vec_a0 = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 vec_a1 = _mm_setr_ps(5.0f, 6.0f, 7.0f, 8.0f);
    __m128 vec_a2 = _mm_setr_ps(9.0f, 10.0f, 11.0f, 12.0f);
    __m128 vec_a3 = _mm_setr_ps(13.0f, 14.0f, 15.0f, 16.0f);
    __m128 vec_a4 = _mm_setr_ps(1.0f, 3.0f, 5.0f, 7.0f);
    __m128 vec_a5 = _mm_setr_ps(2.0f, 4.0f, 6.0f, 8.0f);
    __m128 vec_a6 = _mm_setr_ps(1.5f, 2.5f, 3.5f, 4.5f);
    __m128 vec_a7 = _mm_setr_ps(5.5f, 6.5f, 7.5f, 8.5f);
    __m128 vec_b  = _mm_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(vec_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        vec_a0 = _mm_dp_ps(vec_a0, vec_b, 0xFF);
        vec_a1 = _mm_dp_ps(vec_a1, vec_b, 0xFF);
        vec_a2 = _mm_dp_ps(vec_a2, vec_b, 0xFF);
        vec_a3 = _mm_dp_ps(vec_a3, vec_b, 0xFF);
        vec_a4 = _mm_dp_ps(vec_a4, vec_b, 0xFF);
        vec_a5 = _mm_dp_ps(vec_a5, vec_b, 0xFF);
        vec_a6 = _mm_dp_ps(vec_a6, vec_b, 0xFF);
        vec_a7 = _mm_dp_ps(vec_a7, vec_b, 0xFF);
        __asm__ volatile("" : "+x"(vec_a0), "+x"(vec_a1), "+x"(vec_a2), "+x"(vec_a3),
                              "+x"(vec_a4), "+x"(vec_a5), "+x"(vec_a6), "+x"(vec_a7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm_cvtss_f32(vec_a0) + _mm_cvtss_f32(vec_a4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/*
 * 8-wide float dot: vdpps ymm does two independent 4-wide dpps in each
 * 128-bit lane, then we hadd the two results. Latency = dpps + extract + add.
 */
static double dot8_dpps_ymm_latency(uint64_t iterations)
{
    __m256 vector_a = _mm256_setr_ps(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f);
    __m256 vector_b = _mm256_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(vector_a), "+x"(vector_b));
    float accumulated_result = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        /* dpps in each 128-bit lane */
        __m256 lane_dots = _mm256_dp_ps(vector_a, vector_b, 0xFF);
        /* Extract high 128, add to low to combine the two 4-wide dots */
        __m128 low_lane  = _mm256_castps256_ps128(lane_dots);
        __m128 high_lane = _mm256_extractf128_ps(lane_dots, 1);
        __m128 combined  = _mm_add_ss(low_lane, high_lane);
        float dot_result = _mm_cvtss_f32(combined);
        accumulated_result += dot_result;
        vector_a = _mm256_set1_ps(accumulated_result);
        __asm__ volatile("" : "+x"(vector_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_result;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations);
}

/*
 * vpdpbusd zmm — INT8 dot product (4 uint8 * int8 -> int32, 16 groups)
 * Each vpdpbusd computes 16 parallel 4-element int8 dot products.
 * Latency: dependent chain of 4 vpdpbusd.
 */
static double dot_int8_vpdpbusd_latency(uint64_t iterations)
{
    __m512i accumulator = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi32(0x01010101);  /* four uint8 = 1 */
    __m512i operand_b = _mm512_set1_epi32(0x01010101);  /* four int8  = 1 */
    __asm__ volatile("" : "+x"(operand_a), "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double dot_int8_vpdpbusd_throughput(uint64_t iterations)
{
    __m512i acc_0 = _mm512_setzero_si512();
    __m512i acc_1 = _mm512_setzero_si512();
    __m512i acc_2 = _mm512_setzero_si512();
    __m512i acc_3 = _mm512_setzero_si512();
    __m512i acc_4 = _mm512_setzero_si512();
    __m512i acc_5 = _mm512_setzero_si512();
    __m512i acc_6 = _mm512_setzero_si512();
    __m512i acc_7 = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi32(0x01020304);
    __m512i operand_b = _mm512_set1_epi32(0x01010101);
    __asm__ volatile("" : "+x"(operand_a), "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc_0 = _mm512_dpbusd_epi32(acc_0, operand_a, operand_b);
        acc_1 = _mm512_dpbusd_epi32(acc_1, operand_a, operand_b);
        acc_2 = _mm512_dpbusd_epi32(acc_2, operand_a, operand_b);
        acc_3 = _mm512_dpbusd_epi32(acc_3, operand_a, operand_b);
        acc_4 = _mm512_dpbusd_epi32(acc_4, operand_a, operand_b);
        acc_5 = _mm512_dpbusd_epi32(acc_5, operand_a, operand_b);
        acc_6 = _mm512_dpbusd_epi32(acc_6, operand_a, operand_b);
        acc_7 = _mm512_dpbusd_epi32(acc_7, operand_a, operand_b);
        __asm__ volatile("" : "+x"(acc_0), "+x"(acc_1), "+x"(acc_2), "+x"(acc_3),
                              "+x"(acc_4), "+x"(acc_5), "+x"(acc_6), "+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc_0) + _mm512_reduce_add_epi32(acc_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/*
 * 32-element float dot: two vmulps zmm + reduce each, sum.
 * Measures cost of doing a 32-wide dot at 512-bit SIMD width.
 */
static double dot32_float_latency(uint64_t iterations)
{
    __m512 vec_a_lo = _mm512_set1_ps(1.0f);
    __m512 vec_a_hi = _mm512_set1_ps(2.0f);
    __m512 vec_b_lo = _mm512_set1_ps(0.05f);
    __m512 vec_b_hi = _mm512_set1_ps(0.05f);
    __asm__ volatile("" : "+x"(vec_a_lo), "+x"(vec_a_hi), "+x"(vec_b_lo), "+x"(vec_b_hi));
    float accumulated_result = 0.0f;

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        __m512 prod_lo = _mm512_mul_ps(vec_a_lo, vec_b_lo);
        __m512 prod_hi = _mm512_mul_ps(vec_a_hi, vec_b_hi);
        __m512 combined_product = _mm512_add_ps(prod_lo, prod_hi);
        float dot_result = _mm512_reduce_add_ps(combined_product);
        accumulated_result += dot_result;
        vec_a_lo = _mm512_set1_ps(accumulated_result);
        __asm__ volatile("" : "+x"(vec_a_lo));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = accumulated_result;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    sm_zen_print_header("dot_product", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double dot16_lat  = dot16_float_latency(cfg.iterations);
    double dot16_tp   = dot16_float_throughput(cfg.iterations);
    double dot4_lat   = dot4_dpps_latency(cfg.iterations);
    double dot4_tp    = dot4_dpps_throughput(cfg.iterations);
    double dot8_lat   = dot8_dpps_ymm_latency(cfg.iterations);
    double int8_lat   = dot_int8_vpdpbusd_latency(cfg.iterations);
    double int8_tp    = dot_int8_vpdpbusd_throughput(cfg.iterations);
    double dot32_lat  = dot32_float_latency(cfg.iterations);

    printf("--- Float dot product ---\n");
    printf("dot4_dpps_xmm_lat=%.4f\n", dot4_lat);
    printf("dot4_dpps_xmm_tp=%.4f\n", dot4_tp);
    printf("dot8_dpps_ymm_lat=%.4f\n", dot8_lat);
    printf("dot16_mulps_reduce_lat=%.4f\n", dot16_lat);
    printf("dot16_mulps_reduce_tp=%.4f\n", dot16_tp);
    printf("dot32_2x_mulps_reduce_lat=%.4f\n", dot32_lat);

    printf("\n--- INT8 dot product (vpdpbusd, 16x 4-element dots per zmm) ---\n");
    printf("vpdpbusd_zmm_lat=%.4f\n", int8_lat);
    printf("vpdpbusd_zmm_tp=%.4f\n", int8_tp);

    printf("\n--- Summary (cycles per dot product) ---\n");
    printf(" 4-elem float (dpps xmm):    lat=%.2f cy  tp=%.2f cy\n", dot4_lat, dot4_tp);
    printf(" 8-elem float (dpps ymm):    lat=%.2f cy\n", dot8_lat);
    printf("16-elem float (mulps+reduce):lat=%.2f cy  tp=%.2f cy\n", dot16_lat, dot16_tp);
    printf("32-elem float (2x mul+reduce):lat=%.2f cy\n", dot32_lat);
    printf("int8 4x16 (vpdpbusd zmm):   lat=%.2f cy  tp=%.2f cy\n", int8_lat, int8_tp);
    printf("  -> vpdpbusd: %d int8 dot products per instruction\n", 16);

    if (cfg.csv)
        printf("csv,probe=dot_product,dot4_lat=%.4f,dot4_tp=%.4f,dot8_lat=%.4f,dot16_lat=%.4f,dot16_tp=%.4f,dot32_lat=%.4f,int8_lat=%.4f,int8_tp=%.4f\n",
               dot4_lat, dot4_tp, dot8_lat, dot16_lat, dot16_tp, dot32_lat, int8_lat, int8_tp);

    return 0;
}
