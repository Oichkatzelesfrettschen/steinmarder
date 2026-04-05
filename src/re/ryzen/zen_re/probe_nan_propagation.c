#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>
#include <math.h>
#include <float.h>
#include <string.h>

/*
 * probe_nan_propagation.c -- NaN/Inf data-dependent latency on Zen 4.
 *
 * Tests whether special FP values (NaN, Inf, signaling NaN) change the
 * execution latency of arithmetic instructions.  On some microarchitectures
 * special-value handling takes a slower path through microcode.
 *
 * Tests:
 *   - VADDPS zmm:      normal, +Inf, -Inf, qNaN, sNaN
 *   - VMULPS zmm:      normal, +Inf, -Inf, qNaN, sNaN
 *   - VDIVPS zmm:      normal, 0/0 (NaN-producing), x/0 (Inf-producing)
 *   - VSQRTPS zmm:     normal, negative (NaN-producing), zero
 *   - VFMADD231PS zmm: NaN in position a, b, or c
 *   - VCMPPS zmm:      normal vs normal, NaN vs normal (unordered)
 *
 * 4-deep dependency chains with asm volatile opacity fences.
 */

#define WARMUP_ITERS 5000
#define CHAIN_DEPTH  4

typedef struct {
    const char *test_name;
    const char *instruction;
    const char *operand_class;
    double      latency_cycles;
} NanResult;

/* Build a signaling NaN (sNaN) for FP32: exponent=0xFF, bit22=0, mantissa!=0 */
static inline float make_snan_f32(void)
{
    uint32_t snan_bits = 0x7FA00000u; /* sNaN with payload */
    float snan_value;
    memcpy(&snan_value, &snan_bits, sizeof(float));
    return snan_value;
}

/* ====================================================================
 *  VADDPS zmm
 * ==================================================================== */

static double measure_addps_with_value(float operand_value, uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(operand_value);
    __m512 addend      = _mm512_set1_ps(operand_value);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_add_ps(accumulator, addend);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_add_ps(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_ps(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_ps(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_ps(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ====================================================================
 *  VMULPS zmm
 * ==================================================================== */

static double measure_mulps_with_value(float operand_value, uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(operand_value);
    __m512 multiplier  = _mm512_set1_ps(operand_value);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_mul_ps(accumulator, multiplier);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ====================================================================
 *  VDIVPS zmm -- special: 0/0 produces NaN, x/0 produces Inf
 * ==================================================================== */

static double measure_divps_normal(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1000.0f);
    __m512 divisor     = _mm512_set1_ps(1.0000001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_div_ps(accumulator, divisor);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_divps_zero_by_zero(uint64_t iterations)
{
    /* 0/0 = NaN on every iteration */
    __m512 accumulator = _mm512_set1_ps(0.0f);
    __m512 divisor     = _mm512_set1_ps(0.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_div_ps(accumulator, divisor);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_divps_x_by_zero(uint64_t iterations)
{
    /* x/0 = Inf on every iteration */
    __m512 accumulator = _mm512_set1_ps(1.0f);
    __m512 divisor     = _mm512_set1_ps(0.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_div_ps(accumulator, divisor);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ====================================================================
 *  VSQRTPS zmm
 * ==================================================================== */

static double measure_sqrtps_normal(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(100.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_sqrt_ps(accumulator);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_sqrtps_negative(uint64_t iterations)
{
    /* sqrt(negative) = NaN */
    __m512 accumulator = _mm512_set1_ps(-1.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_sqrt_ps(accumulator);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_sqrtps_zero(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(0.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_sqrt_ps(accumulator);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ====================================================================
 *  VFMADD231PS zmm -- NaN in each position
 * ==================================================================== */

static double measure_fmaddps_nan_in_a(uint64_t iterations)
{
    float nan_value   = nanf("");
    __m512 factor_a   = _mm512_set1_ps(nan_value);
    __m512 factor_b   = _mm512_set1_ps(1.0f);
    __m512 accumulator = _mm512_set1_ps(1.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_fmaddps_nan_in_b(uint64_t iterations)
{
    float nan_value   = nanf("");
    __m512 factor_a   = _mm512_set1_ps(1.0f);
    __m512 factor_b   = _mm512_set1_ps(nan_value);
    __m512 accumulator = _mm512_set1_ps(1.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_fmaddps_nan_in_c(uint64_t iterations)
{
    /* NaN only in c (accumulator); a*b is normal */
    float nan_value     = nanf("");
    __m512 factor_a     = _mm512_set1_ps(1.0000001f);
    __m512 factor_b     = _mm512_set1_ps(0.0000001f);
    __m512 accumulator  = _mm512_set1_ps(nan_value);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_fmaddps_normal(uint64_t iterations)
{
    __m512 factor_a     = _mm512_set1_ps(1.0000001f);
    __m512 factor_b     = _mm512_set1_ps(0.9999999f);
    __m512 accumulator  = _mm512_set1_ps(1.0f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(factor_a, factor_b, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ====================================================================
 *  VCMPPS zmm -- unordered comparison with NaN
 * ==================================================================== */

static double measure_cmpps_normal(uint64_t iterations)
{
    __m512 operand_a = _mm512_set1_ps(1.0f);
    __m512 operand_b = _mm512_set1_ps(2.0f);
    volatile __mmask16 result_mask = 0;

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_LT_OQ);
            __asm__ volatile("" : "+k"(result_mask));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_LT_OQ);
        __asm__ volatile("" : "+k"(result_mask));
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_LT_OQ);
        __asm__ volatile("" : "+k"(result_mask));
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_LT_OQ);
        __asm__ volatile("" : "+k"(result_mask));
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_LT_OQ);
        __asm__ volatile("" : "+k"(result_mask));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile __mmask16 sink = result_mask;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_cmpps_nan(uint64_t iterations)
{
    float nan_value = nanf("");
    __m512 operand_a = _mm512_set1_ps(nan_value);
    __m512 operand_b = _mm512_set1_ps(1.0f);
    volatile __mmask16 result_mask = 0;

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_UNORD_Q);
            __asm__ volatile("" : "+k"(result_mask));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_UNORD_Q);
        __asm__ volatile("" : "+k"(result_mask));
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_UNORD_Q);
        __asm__ volatile("" : "+k"(result_mask));
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_UNORD_Q);
        __asm__ volatile("" : "+k"(result_mask));
        result_mask = _mm512_cmp_ps_mask(operand_a, operand_b, _CMP_UNORD_Q);
        __asm__ volatile("" : "+k"(result_mask));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile __mmask16 sink = result_mask;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ====================================================================
 *  Main
 * ==================================================================== */

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 30000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    sm_zen_print_header("nan_propagation", &probe_config, &detected_features);
    printf("\n");

    if (!detected_features.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    uint64_t measurement_iterations = probe_config.iterations * 200;
    /* Div and sqrt are slower; use fewer iterations */
    uint64_t slow_measurement_iterations = probe_config.iterations * 20;

    printf("measurement_iterations=%lu (fast), %lu (div/sqrt)\n\n",
           (unsigned long)measurement_iterations,
           (unsigned long)slow_measurement_iterations);

    float positive_inf = INFINITY;
    float negative_inf = -INFINITY;
    float quiet_nan    = nanf("");
    float signaling_nan = make_snan_f32();

    NanResult results_table[24];
    int result_index = 0;

    /* --- VADDPS zmm --- */
    results_table[result_index++] = (NanResult){
        "addps_zmm", "vaddps zmm", "normal",
        measure_addps_with_value(1.0f, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "addps_zmm", "vaddps zmm", "+Inf",
        measure_addps_with_value(positive_inf, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "addps_zmm", "vaddps zmm", "-Inf",
        measure_addps_with_value(negative_inf, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "addps_zmm", "vaddps zmm", "qNaN",
        measure_addps_with_value(quiet_nan, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "addps_zmm", "vaddps zmm", "sNaN",
        measure_addps_with_value(signaling_nan, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- VMULPS zmm --- */
    results_table[result_index++] = (NanResult){
        "mulps_zmm", "vmulps zmm", "normal",
        measure_mulps_with_value(1.0000001f, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "mulps_zmm", "vmulps zmm", "+Inf",
        measure_mulps_with_value(positive_inf, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "mulps_zmm", "vmulps zmm", "-Inf",
        measure_mulps_with_value(negative_inf, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "mulps_zmm", "vmulps zmm", "qNaN",
        measure_mulps_with_value(quiet_nan, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "mulps_zmm", "vmulps zmm", "sNaN",
        measure_mulps_with_value(signaling_nan, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- VDIVPS zmm --- */
    results_table[result_index++] = (NanResult){
        "divps_zmm", "vdivps zmm", "normal",
        measure_divps_normal(slow_measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "divps_zmm", "vdivps zmm", "0/0_NaN",
        measure_divps_zero_by_zero(slow_measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "divps_zmm", "vdivps zmm", "x/0_Inf",
        measure_divps_x_by_zero(slow_measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- VSQRTPS zmm --- */
    results_table[result_index++] = (NanResult){
        "sqrtps_zmm", "vsqrtps zmm", "normal",
        measure_sqrtps_normal(slow_measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "sqrtps_zmm", "vsqrtps zmm", "negative_NaN",
        measure_sqrtps_negative(slow_measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "sqrtps_zmm", "vsqrtps zmm", "zero",
        measure_sqrtps_zero(slow_measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- VFMADD231PS zmm --- */
    results_table[result_index++] = (NanResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "normal",
        measure_fmaddps_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "NaN_in_a",
        measure_fmaddps_nan_in_a(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "NaN_in_b",
        measure_fmaddps_nan_in_b(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "NaN_in_c",
        measure_fmaddps_nan_in_c(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- VCMPPS zmm --- */
    results_table[result_index++] = (NanResult){
        "cmpps_zmm", "vcmpps zmm", "normal_ordered",
        measure_cmpps_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (NanResult){
        "cmpps_zmm", "vcmpps zmm", "NaN_unordered",
        measure_cmpps_nan(measurement_iterations)};

    /* ---- Print results ---- */
    printf("%-14s %-20s %-18s %14s\n",
           "test", "instruction", "operand_class", "latency_cyc");
    printf("%-14s %-20s %-18s %14s\n",
           "----", "-----------", "-------------", "-----------");
    for (int row = 0; row < result_index; row++) {
        printf("%-14s %-20s %-18s %14.4f\n",
               results_table[row].test_name,
               results_table[row].instruction,
               results_table[row].operand_class,
               results_table[row].latency_cycles);
    }

    /* ---- Penalty analysis ---- */
    printf("\n--- special-value penalty (cycles above normal baseline) ---\n");
    /* addps: baseline index 0 */
    printf("  addps: +Inf=%+.2f  -Inf=%+.2f  qNaN=%+.2f  sNaN=%+.2f\n",
           results_table[1].latency_cycles - results_table[0].latency_cycles,
           results_table[2].latency_cycles - results_table[0].latency_cycles,
           results_table[3].latency_cycles - results_table[0].latency_cycles,
           results_table[4].latency_cycles - results_table[0].latency_cycles);
    /* mulps: baseline index 5 */
    printf("  mulps: +Inf=%+.2f  -Inf=%+.2f  qNaN=%+.2f  sNaN=%+.2f\n",
           results_table[6].latency_cycles - results_table[5].latency_cycles,
           results_table[7].latency_cycles - results_table[5].latency_cycles,
           results_table[8].latency_cycles - results_table[5].latency_cycles,
           results_table[9].latency_cycles - results_table[5].latency_cycles);
    /* divps: baseline index 10 */
    printf("  divps: 0/0_NaN=%+.2f  x/0_Inf=%+.2f\n",
           results_table[11].latency_cycles - results_table[10].latency_cycles,
           results_table[12].latency_cycles - results_table[10].latency_cycles);
    /* sqrtps: baseline index 13 */
    printf("  sqrtps: negative_NaN=%+.2f  zero=%+.2f\n",
           results_table[14].latency_cycles - results_table[13].latency_cycles,
           results_table[15].latency_cycles - results_table[13].latency_cycles);
    /* fmadd: baseline index 16 */
    printf("  fmadd: NaN_in_a=%+.2f  NaN_in_b=%+.2f  NaN_in_c=%+.2f\n",
           results_table[17].latency_cycles - results_table[16].latency_cycles,
           results_table[18].latency_cycles - results_table[16].latency_cycles,
           results_table[19].latency_cycles - results_table[16].latency_cycles);
    /* cmpps: baseline index 20 */
    printf("  cmpps: NaN_unordered=%+.2f\n",
           results_table[21].latency_cycles - results_table[20].latency_cycles);

    /* ---- CSV output ---- */
    if (probe_config.csv) {
        printf("\ncsv_header,test,instruction,operand_class,latency_cycles\n");
        for (int row = 0; row < result_index; row++) {
            printf("%s,%s,%s,%.4f\n",
                   results_table[row].test_name,
                   results_table[row].instruction,
                   results_table[row].operand_class,
                   results_table[row].latency_cycles);
        }
    }

    return 0;
}
