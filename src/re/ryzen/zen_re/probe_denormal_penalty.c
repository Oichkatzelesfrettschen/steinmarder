#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>
#include <math.h>
#include <float.h>

/*
 * probe_denormal_penalty.c -- Data-dependent denormal/subnormal penalty on Zen 4.
 *
 * AMD Software Optimization Guide documents that denormal operands may add
 * "possibly more than 100 clock cycles" via a microcode assist.  This probe
 * measures the EXACT cost for:
 *   - VADDPS zmm: normal*normal, normal*denormal, denormal*denormal
 *   - VMULPS zmm: same three combinations
 *   - VFMADD231PS zmm: same three combinations
 *   - VADDPD zmm: normal vs denormal (FP64)
 *   - Each scenario repeated with FTZ/DAZ ON and OFF
 *
 * Uses 4-deep dependency chains per iteration with asm volatile fences
 * to prevent constant-folding while preserving data dependencies.
 */

#define WARMUP_ITERS 5000
#define CHAIN_DEPTH  4

/* FP32 smallest subnormal: ~1.4e-45 */
static const float DENORMAL_F32 = 1.0e-45f;
static const float NORMAL_F32   = 1.0f;

/* FP64 smallest subnormal: ~5e-324 */
static const double DENORMAL_F64 = 5.0e-324;
static const double NORMAL_F64   = 1.0;

typedef struct {
    const char *test_name;
    const char *instruction;
    const char *operand_class;
    const char *ftz_daz_state;
    double      latency_cycles;
} DenormalResult;

/* ====================================================================
 *  FP32 VADDPS zmm measurements
 * ==================================================================== */

static double measure_addps_normal_normal(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(NORMAL_F32);
    __m512 addend      = _mm512_set1_ps(0.0001f);

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

static double measure_addps_normal_denormal(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(NORMAL_F32);
    __m512 addend      = _mm512_set1_ps(DENORMAL_F32);

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

static double measure_addps_denormal_denormal(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(DENORMAL_F32);
    __m512 addend      = _mm512_set1_ps(DENORMAL_F32);

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
 *  FP32 VMULPS zmm measurements
 * ==================================================================== */

static double measure_mulps_normal_normal(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(NORMAL_F32);
    __m512 multiplier  = _mm512_set1_ps(1.0000001f);

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

static double measure_mulps_normal_denormal(uint64_t iterations)
{
    /* Multiplying normal * denormal -- result is denormal or zero */
    __m512 accumulator = _mm512_set1_ps(DENORMAL_F32);
    __m512 multiplier  = _mm512_set1_ps(NORMAL_F32);

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

static double measure_mulps_denormal_denormal(uint64_t iterations)
{
    /* den * den underflows to zero, but the INPUT is what causes the assist */
    __m512 accumulator = _mm512_set1_ps(DENORMAL_F32);
    __m512 multiplier  = _mm512_set1_ps(DENORMAL_F32);

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
 *  FP32 VFMADD231PS zmm measurements
 * ==================================================================== */

static double measure_fmaddps_normal_normal(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(NORMAL_F32);
    __m512 factor_a    = _mm512_set1_ps(1.0000001f);
    __m512 factor_b    = _mm512_set1_ps(0.0000001f);

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

static double measure_fmaddps_normal_denormal(uint64_t iterations)
{
    /* acc is denormal; a*b is normal -- result touches denormal path */
    __m512 accumulator = _mm512_set1_ps(DENORMAL_F32);
    __m512 factor_a    = _mm512_set1_ps(1.0000001f);
    __m512 factor_b    = _mm512_set1_ps(0.0000001f);

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

static double measure_fmaddps_denormal_denormal(uint64_t iterations)
{
    /* All three operands denormal */
    __m512 accumulator = _mm512_set1_ps(DENORMAL_F32);
    __m512 factor_a    = _mm512_set1_ps(DENORMAL_F32);
    __m512 factor_b    = _mm512_set1_ps(DENORMAL_F32);

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
 *  FP64 VADDPD zmm measurements
 * ==================================================================== */

static double measure_addpd_normal(uint64_t iterations)
{
    __m512d accumulator = _mm512_set1_pd(NORMAL_F64);
    __m512d addend      = _mm512_set1_pd(0.0001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_add_pd(accumulator, addend);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm512_reduce_add_pd(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double measure_addpd_denormal(uint64_t iterations)
{
    __m512d accumulator = _mm512_set1_pd(DENORMAL_F64);
    __m512d addend      = _mm512_set1_pd(DENORMAL_F64);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int depth = 0; depth < CHAIN_DEPTH; depth++) {
            accumulator = _mm512_add_pd(accumulator, addend);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm512_reduce_add_pd(accumulator);
    (void)sink_value;
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

    sm_zen_print_header("denormal_penalty", &probe_config, &detected_features);
    printf("\n");

    if (!detected_features.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    uint64_t measurement_iterations = probe_config.iterations * 200;

    /* Maximum result slots: 3 instructions * 3 operand classes * 2 ftz states
     * + 1 instruction * 2 operand classes * 2 ftz states = 22 */
    DenormalResult results_table[22];
    int result_index = 0;

    /* ---- Phase 1: FTZ/DAZ OFF (default IEEE 754 behavior) ---- */
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);

    printf("Phase 1: FTZ=OFF DAZ=OFF (IEEE 754 denormal handling)\n");
    printf("  measurement_iterations=%lu\n\n", (unsigned long)measurement_iterations);

    results_table[result_index++] = (DenormalResult){
        "addps_zmm", "vaddps zmm", "normal+normal", "OFF",
        measure_addps_normal_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addps_zmm", "vaddps zmm", "normal+denormal", "OFF",
        measure_addps_normal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addps_zmm", "vaddps zmm", "denormal+denormal", "OFF",
        measure_addps_denormal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "mulps_zmm", "vmulps zmm", "normal*normal", "OFF",
        measure_mulps_normal_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "mulps_zmm", "vmulps zmm", "normal*denormal", "OFF",
        measure_mulps_normal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "mulps_zmm", "vmulps zmm", "denormal*denormal", "OFF",
        measure_mulps_denormal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "normal_fma", "OFF",
        measure_fmaddps_normal_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "denormal_addend", "OFF",
        measure_fmaddps_normal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "all_denormal", "OFF",
        measure_fmaddps_denormal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addpd_zmm", "vaddpd zmm", "normal+normal", "OFF",
        measure_addpd_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addpd_zmm", "vaddpd zmm", "denormal+denormal", "OFF",
        measure_addpd_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* ---- Phase 2: FTZ/DAZ ON (flush denormals to zero) ---- */
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    printf("Phase 2: FTZ=ON DAZ=ON (denormals flushed to zero)\n\n");

    results_table[result_index++] = (DenormalResult){
        "addps_zmm", "vaddps zmm", "normal+normal", "ON",
        measure_addps_normal_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addps_zmm", "vaddps zmm", "normal+denormal", "ON",
        measure_addps_normal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addps_zmm", "vaddps zmm", "denormal+denormal", "ON",
        measure_addps_denormal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "mulps_zmm", "vmulps zmm", "normal*normal", "ON",
        measure_mulps_normal_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "mulps_zmm", "vmulps zmm", "normal*denormal", "ON",
        measure_mulps_normal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "mulps_zmm", "vmulps zmm", "denormal*denormal", "ON",
        measure_mulps_denormal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "normal_fma", "ON",
        measure_fmaddps_normal_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "denormal_addend", "ON",
        measure_fmaddps_normal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "fmaddps_zmm", "vfmadd231ps zmm", "all_denormal", "ON",
        measure_fmaddps_denormal_denormal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addpd_zmm", "vaddpd zmm", "normal+normal", "ON",
        measure_addpd_normal(measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (DenormalResult){
        "addpd_zmm", "vaddpd zmm", "denormal+denormal", "ON",
        measure_addpd_denormal(measurement_iterations)};

    /* Restore default */
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);

    /* ---- Print results ---- */
    printf("%-16s %-20s %-22s %-6s %14s\n",
           "test", "instruction", "operand_class", "ftz", "latency_cyc");
    printf("%-16s %-20s %-22s %-6s %14s\n",
           "----", "-----------", "-------------", "---", "-----------");
    for (int row = 0; row < result_index; row++) {
        printf("%-16s %-20s %-22s %-6s %14.4f\n",
               results_table[row].test_name,
               results_table[row].instruction,
               results_table[row].operand_class,
               results_table[row].ftz_daz_state,
               results_table[row].latency_cycles);
    }

    /* ---- Penalty analysis ---- */
    printf("\n--- denormal penalty (cycles above normal baseline) ---\n");
    for (int ftz_phase = 0; ftz_phase < 2; ftz_phase++) {
        int phase_offset = ftz_phase * 11;
        const char *ftz_label = ftz_phase == 0 ? "OFF" : "ON";
        double addps_baseline   = results_table[phase_offset + 0].latency_cycles;
        double mulps_baseline   = results_table[phase_offset + 3].latency_cycles;
        double fmaddps_baseline = results_table[phase_offset + 6].latency_cycles;
        double addpd_baseline   = results_table[phase_offset + 9].latency_cycles;

        printf("  [FTZ=%s] addps  normal+denormal: +%.2f cyc  denormal+denormal: +%.2f cyc\n",
               ftz_label,
               results_table[phase_offset + 1].latency_cycles - addps_baseline,
               results_table[phase_offset + 2].latency_cycles - addps_baseline);
        printf("  [FTZ=%s] mulps  normal*denormal: +%.2f cyc  denormal*denormal: +%.2f cyc\n",
               ftz_label,
               results_table[phase_offset + 4].latency_cycles - mulps_baseline,
               results_table[phase_offset + 5].latency_cycles - mulps_baseline);
        printf("  [FTZ=%s] fmadd  denormal_addend: +%.2f cyc  all_denormal:      +%.2f cyc\n",
               ftz_label,
               results_table[phase_offset + 7].latency_cycles - fmaddps_baseline,
               results_table[phase_offset + 8].latency_cycles - fmaddps_baseline);
        printf("  [FTZ=%s] addpd  denormal+denormal: +%.2f cyc\n",
               ftz_label,
               results_table[phase_offset + 10].latency_cycles - addpd_baseline);
    }

    /* ---- CSV output ---- */
    if (probe_config.csv) {
        printf("\ncsv_header,test,instruction,operand_class,ftz_daz,latency_cycles\n");
        for (int row = 0; row < result_index; row++) {
            printf("%s,%s,%s,%s,%.4f\n",
                   results_table[row].test_name,
                   results_table[row].instruction,
                   results_table[row].operand_class,
                   results_table[row].ftz_daz_state,
                   results_table[row].latency_cycles);
        }
    }

    return 0;
}
