#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_depchain_int.c — Integer dependency chain latency across ALL widths.
 *
 * Translated from r600 depchain_*.cl and SASS int*_tile probes.
 * Measures latency of dependent chains for: INT8 add (via vpaddb),
 * INT16 add (via vpaddw), INT32 add (via vpaddd), INT64 add (via vpaddq),
 * INT32 mul (via vpmulld), INT32 imul/widening (via vpmuldq), and
 * INT8 SAD (via vpsadbw / _mm512_sad_epu8).
 * Each uses a 4-deep dependent chain per iteration.
 *
 * Uses asm volatile barriers between each op to enforce serial dependency
 * and prevent the compiler from breaking the chain or constant-folding.
 */

static double depchain_int8_add(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi8(1);
    __m512i operand = _mm512_set1_epi8(1);
    __asm__ volatile("" : "+x"(operand));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_add_epi8(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi8(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi8(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi8(acc, operand);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double depchain_int16_add(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi16(1);
    __m512i operand = _mm512_set1_epi16(1);
    __asm__ volatile("" : "+x"(operand));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_add_epi16(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi16(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi16(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi16(acc, operand);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double depchain_int32_add(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi32(1);
    __m512i operand = _mm512_set1_epi32(1);
    __asm__ volatile("" : "+x"(operand));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_add_epi32(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi32(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi32(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi32(acc, operand);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double depchain_int64_add(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi64(1);
    __m512i operand = _mm512_set1_epi64(1);
    __asm__ volatile("" : "+x"(operand));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_add_epi64(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi64(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi64(acc, operand);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_add_epi64(acc, operand);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink_val = _mm512_reduce_add_epi64(acc);
    (void)sink_val;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double depchain_int32_mul(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi32(3);
    __m512i factor = _mm512_set1_epi32(1);
    __asm__ volatile("" : "+x"(factor));  /* hide the constant from the compiler */

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_mullo_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_mullo_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_mullo_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_mullo_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double depchain_int32_imul_widening(uint64_t iterations)
{
    /* vpmuldq: signed 32x32->64 widening multiply (even lanes) */
    __m512i acc = _mm512_set1_epi64(3);
    __m512i factor = _mm512_set1_epi64(1);
    __asm__ volatile("" : "+x"(factor));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_mul_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_mul_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_mul_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_mul_epi32(acc, factor);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink_val = _mm512_reduce_add_epi64(acc);
    (void)sink_val;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double depchain_int8_sad(uint64_t iterations)
{
    /* vpsadbw: sum of absolute differences of uint8 -> uint64 per group of 8 */
    __m512i acc = _mm512_set1_epi8(10);
    __m512i reference = _mm512_set1_epi8(9);
    __asm__ volatile("" : "+x"(reference));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_sad_epu8(acc, reference);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_sad_epu8(acc, reference);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_sad_epu8(acc, reference);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_sad_epu8(acc, reference);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink_val = _mm512_reduce_add_epi64(acc);
    (void)sink_val;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    sm_zen_print_header("depchain_int", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double int8_add_latency    = depchain_int8_add(cfg.iterations);
    double int16_add_latency   = depchain_int16_add(cfg.iterations);
    double int32_add_latency   = depchain_int32_add(cfg.iterations);
    double int64_add_latency   = depchain_int64_add(cfg.iterations);
    double int32_mul_latency   = depchain_int32_mul(cfg.iterations);
    double int32_imul_latency  = depchain_int32_imul_widening(cfg.iterations);
    double int8_sad_latency    = depchain_int8_sad(cfg.iterations);

    printf("vpaddb_zmm_latency_cycles=%.4f\n", int8_add_latency);
    printf("vpaddw_zmm_latency_cycles=%.4f\n", int16_add_latency);
    printf("vpaddd_zmm_latency_cycles=%.4f\n", int32_add_latency);
    printf("vpaddq_zmm_latency_cycles=%.4f\n", int64_add_latency);
    printf("vpmulld_zmm_latency_cycles=%.4f\n", int32_mul_latency);
    printf("vpmuldq_zmm_latency_cycles=%.4f\n", int32_imul_latency);
    printf("vpsadbw_zmm_latency_cycles=%.4f\n", int8_sad_latency);

    printf("\n--- Summary (cycles per dependent op) ---\n");
    printf("INT8  add (vpaddb):  %.2f cy\n", int8_add_latency);
    printf("INT16 add (vpaddw):  %.2f cy\n", int16_add_latency);
    printf("INT32 add (vpaddd):  %.2f cy\n", int32_add_latency);
    printf("INT64 add (vpaddq):  %.2f cy\n", int64_add_latency);
    printf("INT32 mul (vpmulld): %.2f cy\n", int32_mul_latency);
    printf("INT32 imul(vpmuldq): %.2f cy\n", int32_imul_latency);
    printf("INT8  SAD (vpsadbw): %.2f cy\n", int8_sad_latency);

    if (cfg.csv)
        printf("csv,probe=depchain_int,vpaddb=%.4f,vpaddw=%.4f,vpaddd=%.4f,vpaddq=%.4f,vpmulld=%.4f,vpmuldq=%.4f,vpsadbw=%.4f\n",
               int8_add_latency, int16_add_latency, int32_add_latency, int64_add_latency,
               int32_mul_latency, int32_imul_latency, int8_sad_latency);

    return 0;
}
