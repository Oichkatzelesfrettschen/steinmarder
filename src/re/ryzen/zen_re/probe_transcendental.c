#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_transcendental.c — Scalar and SIMD transcendental throughput and latency.
 *
 * Translated from Apple probe_recip_rsqrt.metal, probe_transcendental_tp.metal,
 * and r600 alu_sin_cos.frag (RECIPSQRT_IEEE, SIN, COS equivalents).
 *
 * Measures throughput AND latency of:
 *   vrcp14ps zmm   — reciprocal approximation (14-bit)
 *   vrsqrt14ps zmm — reciprocal sqrt approximation (14-bit)
 *   vsqrtps zmm    — exact sqrt (full precision)
 *   sqrtss          — scalar exact sqrt
 *   divss           — scalar divide
 *
 * Throughput: 8 independent accumulators to saturate execution ports.
 * Latency: 4-deep dependent chain with asm barriers.
 */

/* ---- Throughput benchmarks (8-wide independent streams) ---- */

static double throughput_vrcp14ps(uint64_t iterations)
{
    __m512 accumulator_0 = _mm512_set1_ps(1.5f);
    __m512 accumulator_1 = _mm512_set1_ps(2.5f);
    __m512 accumulator_2 = _mm512_set1_ps(3.5f);
    __m512 accumulator_3 = _mm512_set1_ps(4.5f);
    __m512 accumulator_4 = _mm512_set1_ps(5.5f);
    __m512 accumulator_5 = _mm512_set1_ps(6.5f);
    __m512 accumulator_6 = _mm512_set1_ps(7.5f);
    __m512 accumulator_7 = _mm512_set1_ps(8.5f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        accumulator_0 = _mm512_rcp14_ps(accumulator_0);
        accumulator_1 = _mm512_rcp14_ps(accumulator_1);
        accumulator_2 = _mm512_rcp14_ps(accumulator_2);
        accumulator_3 = _mm512_rcp14_ps(accumulator_3);
        accumulator_4 = _mm512_rcp14_ps(accumulator_4);
        accumulator_5 = _mm512_rcp14_ps(accumulator_5);
        accumulator_6 = _mm512_rcp14_ps(accumulator_6);
        accumulator_7 = _mm512_rcp14_ps(accumulator_7);
        __asm__ volatile("" : "+x"(accumulator_0), "+x"(accumulator_1),
                              "+x"(accumulator_2), "+x"(accumulator_3),
                              "+x"(accumulator_4), "+x"(accumulator_5),
                              "+x"(accumulator_6), "+x"(accumulator_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(accumulator_0) + _mm512_reduce_add_ps(accumulator_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_vrsqrt14ps(uint64_t iterations)
{
    __m512 accumulator_0 = _mm512_set1_ps(1.5f);
    __m512 accumulator_1 = _mm512_set1_ps(2.5f);
    __m512 accumulator_2 = _mm512_set1_ps(3.5f);
    __m512 accumulator_3 = _mm512_set1_ps(4.5f);
    __m512 accumulator_4 = _mm512_set1_ps(5.5f);
    __m512 accumulator_5 = _mm512_set1_ps(6.5f);
    __m512 accumulator_6 = _mm512_set1_ps(7.5f);
    __m512 accumulator_7 = _mm512_set1_ps(8.5f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        accumulator_0 = _mm512_rsqrt14_ps(accumulator_0);
        accumulator_1 = _mm512_rsqrt14_ps(accumulator_1);
        accumulator_2 = _mm512_rsqrt14_ps(accumulator_2);
        accumulator_3 = _mm512_rsqrt14_ps(accumulator_3);
        accumulator_4 = _mm512_rsqrt14_ps(accumulator_4);
        accumulator_5 = _mm512_rsqrt14_ps(accumulator_5);
        accumulator_6 = _mm512_rsqrt14_ps(accumulator_6);
        accumulator_7 = _mm512_rsqrt14_ps(accumulator_7);
        __asm__ volatile("" : "+x"(accumulator_0), "+x"(accumulator_1),
                              "+x"(accumulator_2), "+x"(accumulator_3),
                              "+x"(accumulator_4), "+x"(accumulator_5),
                              "+x"(accumulator_6), "+x"(accumulator_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(accumulator_0) + _mm512_reduce_add_ps(accumulator_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_vsqrtps(uint64_t iterations)
{
    __m512 accumulator_0 = _mm512_set1_ps(1.5f);
    __m512 accumulator_1 = _mm512_set1_ps(2.5f);
    __m512 accumulator_2 = _mm512_set1_ps(3.5f);
    __m512 accumulator_3 = _mm512_set1_ps(4.5f);
    __m512 accumulator_4 = _mm512_set1_ps(5.5f);
    __m512 accumulator_5 = _mm512_set1_ps(6.5f);
    __m512 accumulator_6 = _mm512_set1_ps(7.5f);
    __m512 accumulator_7 = _mm512_set1_ps(8.5f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        accumulator_0 = _mm512_sqrt_ps(accumulator_0);
        accumulator_1 = _mm512_sqrt_ps(accumulator_1);
        accumulator_2 = _mm512_sqrt_ps(accumulator_2);
        accumulator_3 = _mm512_sqrt_ps(accumulator_3);
        accumulator_4 = _mm512_sqrt_ps(accumulator_4);
        accumulator_5 = _mm512_sqrt_ps(accumulator_5);
        accumulator_6 = _mm512_sqrt_ps(accumulator_6);
        accumulator_7 = _mm512_sqrt_ps(accumulator_7);
        __asm__ volatile("" : "+x"(accumulator_0), "+x"(accumulator_1),
                              "+x"(accumulator_2), "+x"(accumulator_3),
                              "+x"(accumulator_4), "+x"(accumulator_5),
                              "+x"(accumulator_6), "+x"(accumulator_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(accumulator_0) + _mm512_reduce_add_ps(accumulator_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_sqrtss(uint64_t iterations)
{
    __m128 accumulator_0 = _mm_set_ss(1.5f);
    __m128 accumulator_1 = _mm_set_ss(2.5f);
    __m128 accumulator_2 = _mm_set_ss(3.5f);
    __m128 accumulator_3 = _mm_set_ss(4.5f);
    __m128 accumulator_4 = _mm_set_ss(5.5f);
    __m128 accumulator_5 = _mm_set_ss(6.5f);
    __m128 accumulator_6 = _mm_set_ss(7.5f);
    __m128 accumulator_7 = _mm_set_ss(8.5f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        accumulator_0 = _mm_sqrt_ss(accumulator_0);
        accumulator_1 = _mm_sqrt_ss(accumulator_1);
        accumulator_2 = _mm_sqrt_ss(accumulator_2);
        accumulator_3 = _mm_sqrt_ss(accumulator_3);
        accumulator_4 = _mm_sqrt_ss(accumulator_4);
        accumulator_5 = _mm_sqrt_ss(accumulator_5);
        accumulator_6 = _mm_sqrt_ss(accumulator_6);
        accumulator_7 = _mm_sqrt_ss(accumulator_7);
        __asm__ volatile("" : "+x"(accumulator_0), "+x"(accumulator_1),
                              "+x"(accumulator_2), "+x"(accumulator_3),
                              "+x"(accumulator_4), "+x"(accumulator_5),
                              "+x"(accumulator_6), "+x"(accumulator_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm_cvtss_f32(accumulator_0) + _mm_cvtss_f32(accumulator_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_divss(uint64_t iterations)
{
    __m128 accumulator_0 = _mm_set_ss(100.0f);
    __m128 accumulator_1 = _mm_set_ss(200.0f);
    __m128 accumulator_2 = _mm_set_ss(300.0f);
    __m128 accumulator_3 = _mm_set_ss(400.0f);
    __m128 accumulator_4 = _mm_set_ss(500.0f);
    __m128 accumulator_5 = _mm_set_ss(600.0f);
    __m128 accumulator_6 = _mm_set_ss(700.0f);
    __m128 accumulator_7 = _mm_set_ss(800.0f);
    __m128 divisor = _mm_set_ss(1.0001f);
    __asm__ volatile("" : "+x"(divisor));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        accumulator_0 = _mm_div_ss(accumulator_0, divisor);
        accumulator_1 = _mm_div_ss(accumulator_1, divisor);
        accumulator_2 = _mm_div_ss(accumulator_2, divisor);
        accumulator_3 = _mm_div_ss(accumulator_3, divisor);
        accumulator_4 = _mm_div_ss(accumulator_4, divisor);
        accumulator_5 = _mm_div_ss(accumulator_5, divisor);
        accumulator_6 = _mm_div_ss(accumulator_6, divisor);
        accumulator_7 = _mm_div_ss(accumulator_7, divisor);
        __asm__ volatile("" : "+x"(accumulator_0), "+x"(accumulator_1),
                              "+x"(accumulator_2), "+x"(accumulator_3),
                              "+x"(accumulator_4), "+x"(accumulator_5),
                              "+x"(accumulator_6), "+x"(accumulator_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm_cvtss_f32(accumulator_0) + _mm_cvtss_f32(accumulator_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ---- Latency benchmarks (4-deep dependent chain) ---- */

static double latency_vrcp14ps(uint64_t iterations)
{
    __m512 acc = _mm512_set1_ps(2.0f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_rcp14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_rcp14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_rcp14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_rcp14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double latency_vrsqrt14ps(uint64_t iterations)
{
    __m512 acc = _mm512_set1_ps(2.0f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_rsqrt14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_rsqrt14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_rsqrt14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_rsqrt14_ps(acc);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double latency_vsqrtps(uint64_t iterations)
{
    __m512 acc = _mm512_set1_ps(2.0f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_sqrt_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_sqrt_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_sqrt_ps(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_sqrt_ps(acc);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double latency_sqrtss(uint64_t iterations)
{
    __m128 acc = _mm_set_ss(2.0f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm_sqrt_ss(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm_sqrt_ss(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm_sqrt_ss(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm_sqrt_ss(acc);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm_cvtss_f32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double latency_divss(uint64_t iterations)
{
    __m128 acc = _mm_set_ss(1000000.0f);
    __m128 divisor = _mm_set_ss(1.0001f);
    __asm__ volatile("" : "+x"(divisor));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm_div_ss(acc, divisor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm_div_ss(acc, divisor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm_div_ss(acc, divisor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm_div_ss(acc, divisor);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm_cvtss_f32(acc);
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

    sm_zen_print_header("transcendental", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    /* Throughput measurements */
    double rcp14_throughput   = throughput_vrcp14ps(cfg.iterations);
    double rsqrt14_throughput = throughput_vrsqrt14ps(cfg.iterations);
    double sqrtps_throughput  = throughput_vsqrtps(cfg.iterations);
    double sqrtss_throughput  = throughput_sqrtss(cfg.iterations);
    double divss_throughput   = throughput_divss(cfg.iterations);

    /* Latency measurements */
    double rcp14_lat   = latency_vrcp14ps(cfg.iterations);
    double rsqrt14_lat = latency_vrsqrt14ps(cfg.iterations);
    double sqrtps_lat  = latency_vsqrtps(cfg.iterations);
    double sqrtss_lat  = latency_sqrtss(cfg.iterations);
    double divss_lat   = latency_divss(cfg.iterations);

    printf("--- Throughput (cycles per op, 8 independent streams) ---\n");
    printf("vrcp14ps_zmm_tp=%.4f\n", rcp14_throughput);
    printf("vrsqrt14ps_zmm_tp=%.4f\n", rsqrt14_throughput);
    printf("vsqrtps_zmm_tp=%.4f\n", sqrtps_throughput);
    printf("sqrtss_tp=%.4f\n", sqrtss_throughput);
    printf("divss_tp=%.4f\n", divss_throughput);

    printf("\n--- Latency (cycles per dependent op, 4-deep chain) ---\n");
    printf("vrcp14ps_zmm_lat=%.4f\n", rcp14_lat);
    printf("vrsqrt14ps_zmm_lat=%.4f\n", rsqrt14_lat);
    printf("vsqrtps_zmm_lat=%.4f\n", sqrtps_lat);
    printf("sqrtss_lat=%.4f\n", sqrtss_lat);
    printf("divss_lat=%.4f\n", divss_lat);

    printf("\n--- Summary ---\n");
    printf("vrcp14ps zmm:   tp=%.2f cy  lat=%.2f cy\n", rcp14_throughput, rcp14_lat);
    printf("vrsqrt14ps zmm: tp=%.2f cy  lat=%.2f cy\n", rsqrt14_throughput, rsqrt14_lat);
    printf("vsqrtps zmm:    tp=%.2f cy  lat=%.2f cy\n", sqrtps_throughput, sqrtps_lat);
    printf("sqrtss:         tp=%.2f cy  lat=%.2f cy\n", sqrtss_throughput, sqrtss_lat);
    printf("divss:          tp=%.2f cy  lat=%.2f cy\n", divss_throughput, divss_lat);

    if (cfg.csv)
        printf("csv,probe=transcendental,rcp14_tp=%.4f,rcp14_lat=%.4f,rsqrt14_tp=%.4f,rsqrt14_lat=%.4f,sqrtps_tp=%.4f,sqrtps_lat=%.4f,sqrtss_tp=%.4f,sqrtss_lat=%.4f,divss_tp=%.4f,divss_lat=%.4f\n",
               rcp14_throughput, rcp14_lat, rsqrt14_throughput, rsqrt14_lat,
               sqrtps_throughput, sqrtps_lat, sqrtss_throughput, sqrtss_lat,
               divss_throughput, divss_lat);

    return 0;
}
