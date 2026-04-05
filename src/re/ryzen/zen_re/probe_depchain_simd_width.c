#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>
#include <math.h>

/*
 * probe_depchain_simd_width.c - SIMD width scaling latency probes for Zen 4
 * (Ryzen 9 7945HX).
 *
 * Isolates the effect of SIMD register width on dependency chain latency.
 * Same operation at scalar, 128-bit, 256-bit, and 512-bit widths.
 *
 * Key question: does latency change with SIMD width on Zen 4?
 * Expected: no change for 128/256 (same 256-bit execution unit),
 * possible +0-1 cycle for 512-bit (double-pumped on Zen 4).
 *
 * Operations: FP32 FMA, FP64 FMA, INT32 ADD at all widths.
 *
 * Uses __asm__ volatile("" : "+x"(acc)) compiler opacity fences to
 * prevent constant-folding while preserving true data dependencies.
 */

#define WARMUP_ITERS 5000
#define CHAIN_DEPTH  4

typedef struct {
    const char *datatype;
    const char *operation;
    int simd_bits;
    int element_count;
    double latency_cycles;
    const char *instruction;
} WidthProbeResult;

/* ===== FP32 FMA at all widths ===== */

static double depchain_fp32_fma_scalar(uint64_t iterations)
{
    __m128 accumulator = _mm_set_ss(1.0f);
    __m128 multiplier = _mm_set_ss(1.0f);
    __m128 addend = _mm_set_ss(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm_cvtss_f32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_fp32_fma_128(uint64_t iterations)
{
    __m128 accumulator = _mm_set1_ps(1.0f);
    __m128 multiplier = _mm_set1_ps(1.0f);
    __m128 addend = _mm_set1_ps(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = accumulator[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_fp32_fma_256(uint64_t iterations)
{
    __m256 accumulator = _mm256_set1_ps(1.0f);
    __m256 multiplier = _mm256_set1_ps(1.0f);
    __m256 addend = _mm256_set1_ps(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = accumulator[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_fp32_fma_512(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1.0f);
    __m512 multiplier = _mm512_set1_ps(1.0f);
    __m512 addend = _mm512_set1_ps(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ===== FP64 FMA at all widths ===== */

static double depchain_fp64_fma_scalar(uint64_t iterations)
{
    __m128d accumulator = _mm_set_sd(1.0);
    __m128d multiplier = _mm_set_sd(1.0);
    __m128d addend = _mm_set_sd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_sd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm_cvtsd_f64(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_fp64_fma_128(uint64_t iterations)
{
    __m128d accumulator = _mm_set1_pd(1.0);
    __m128d multiplier = _mm_set1_pd(1.0);
    __m128d addend = _mm_set1_pd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = accumulator[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_fp64_fma_256(uint64_t iterations)
{
    __m256d accumulator = _mm256_set1_pd(1.0);
    __m256d multiplier = _mm256_set1_pd(1.0);
    __m256d addend = _mm256_set1_pd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = accumulator[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_fp64_fma_512(uint64_t iterations)
{
    __m512d accumulator = _mm512_set1_pd(1.0);
    __m512d multiplier = _mm512_set1_pd(1.0);
    __m512d addend = _mm512_set1_pd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm512_reduce_add_pd(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ===== INT32 ADD at all widths ===== */

static double depchain_int32_add_128(uint64_t iterations)
{
    __m128i accumulator = _mm_set1_epi32(1);
    __m128i addend = _mm_set1_epi32(1);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm_extract_epi32(accumulator, 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_int32_add_256(uint64_t iterations)
{
    __m256i accumulator = _mm256_set1_epi32(1);
    __m256i addend = _mm256_set1_epi32(1);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm256_extract_epi32(accumulator, 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

static double depchain_int32_add_512(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi32(1);
    __m512i addend = _mm512_set1_epi32(1);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_epi32(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm512_reduce_add_epi32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 100000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    sm_zen_print_header("depchain_simd_width", &probe_config, &detected_features);
    printf("\n");

    if (!detected_features.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    uint64_t total_iterations = probe_config.iterations;

    WidthProbeResult results_table[] = {
        {"FP32", "fma",  32,  1, 0.0, "vfmadd231ss xmm"},
        {"FP32", "fma", 128,  4, 0.0, "vfmadd231ps xmm"},
        {"FP32", "fma", 256,  8, 0.0, "vfmadd231ps ymm"},
        {"FP32", "fma", 512, 16, 0.0, "vfmadd231ps zmm"},
        {"FP64", "fma",  64,  1, 0.0, "vfmadd231sd xmm"},
        {"FP64", "fma", 128,  2, 0.0, "vfmadd231pd xmm"},
        {"FP64", "fma", 256,  4, 0.0, "vfmadd231pd ymm"},
        {"FP64", "fma", 512,  8, 0.0, "vfmadd231pd zmm"},
        {"INT32", "add", 128,  4, 0.0, "vpaddd xmm"},
        {"INT32", "add", 256,  8, 0.0, "vpaddd ymm"},
        {"INT32", "add", 512, 16, 0.0, "vpaddd zmm"},
    };
    int result_count = sizeof(results_table) / sizeof(results_table[0]);

    results_table[0].latency_cycles  = depchain_fp32_fma_scalar(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[1].latency_cycles  = depchain_fp32_fma_128(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[2].latency_cycles  = depchain_fp32_fma_256(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[3].latency_cycles  = depchain_fp32_fma_512(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[4].latency_cycles  = depchain_fp64_fma_scalar(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[5].latency_cycles  = depchain_fp64_fma_128(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[6].latency_cycles  = depchain_fp64_fma_256(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[7].latency_cycles  = depchain_fp64_fma_512(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[8].latency_cycles  = depchain_int32_add_128(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[9].latency_cycles  = depchain_int32_add_256(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[10].latency_cycles = depchain_int32_add_512(total_iterations);

    printf("%-8s %5s %10s %10s %14s  %s\n",
           "datatype", "op", "simd_bits", "elements", "latency_cycles", "instruction");
    printf("%-8s %5s %10s %10s %14s  %s\n",
           "--------", "--", "---------", "--------", "--------------", "-----------");
    for (int row_index = 0; row_index < result_count; row_index++) {
        printf("%-8s %5s %10d %10d %14.4f  %s\n",
               results_table[row_index].datatype,
               results_table[row_index].operation,
               results_table[row_index].simd_bits,
               results_table[row_index].element_count,
               results_table[row_index].latency_cycles,
               results_table[row_index].instruction);
    }

    printf("\n--- Width Scaling Analysis ---\n");
    printf("FP32 FMA: scalar=%.2f  128=%.2f  256=%.2f  512=%.2f\n",
           results_table[0].latency_cycles,
           results_table[1].latency_cycles,
           results_table[2].latency_cycles,
           results_table[3].latency_cycles);
    printf("FP64 FMA: scalar=%.2f  128=%.2f  256=%.2f  512=%.2f\n",
           results_table[4].latency_cycles,
           results_table[5].latency_cycles,
           results_table[6].latency_cycles,
           results_table[7].latency_cycles);
    printf("INT32 ADD: 128=%.2f  256=%.2f  512=%.2f\n",
           results_table[8].latency_cycles,
           results_table[9].latency_cycles,
           results_table[10].latency_cycles);

    double fp32_fma_512_vs_256 = results_table[3].latency_cycles - results_table[2].latency_cycles;
    double fp64_fma_512_vs_256 = results_table[7].latency_cycles - results_table[6].latency_cycles;
    double int32_add_512_vs_256 = results_table[10].latency_cycles - results_table[9].latency_cycles;

    printf("\n512 vs 256 delta (cycles): FP32_FMA=%+.2f  FP64_FMA=%+.2f  INT32_ADD=%+.2f\n",
           fp32_fma_512_vs_256, fp64_fma_512_vs_256, int32_add_512_vs_256);

    if (fabs(fp32_fma_512_vs_256) < 0.5 && fabs(fp64_fma_512_vs_256) < 0.5 &&
        fabs(int32_add_512_vs_256) < 0.5) {
        printf("CONCLUSION: 512-bit latency is SAME as 256-bit on this Zen 4.\n");
        printf("  (Double-pump does NOT add latency, only affects throughput.)\n");
    } else {
        printf("CONCLUSION: 512-bit latency differs from 256-bit.\n");
        printf("  FP32 FMA: %+.2f cy, FP64 FMA: %+.2f cy, INT32 ADD: %+.2f cy\n",
               fp32_fma_512_vs_256, fp64_fma_512_vs_256, int32_add_512_vs_256);
    }

    if (probe_config.csv) {
        printf("\ncsv,probe=depchain_simd_width");
        for (int row_index = 0; row_index < result_count; row_index++) {
            printf(",%s_%s_%dbit=%.4f",
                   results_table[row_index].datatype,
                   results_table[row_index].operation,
                   results_table[row_index].simd_bits,
                   results_table[row_index].latency_cycles);
        }
        printf("\n");
    }

    return 0;
}
