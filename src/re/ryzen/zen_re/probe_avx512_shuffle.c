#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_shuffle.c — Cross-lane shuffle/permute throughput at 512-bit.
 *
 * Measures vpermps zmm, vpermi2d zmm, vpermb zmm (AVX512-VBMI),
 * and compares against the 256-bit vpermps ymm baseline.
 *
 * Critical for data layout transposition in the BF16 pair-interleaved
 * weight repack path of the NeRF MLP kernel.
 */

/* --- vpermps zmm latency (dependent chain) --- */
static double vpermps_zmm_latency(uint64_t iterations)
{
    __m512 data = _mm512_set_ps(16.0f,15.0f,14.0f,13.0f,12.0f,11.0f,10.0f,9.0f,
                                 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f,1.0f);
    /* Non-trivial scatter pattern that can't be simplified */
    __m512i control = _mm512_set_epi32(5,12,3,10,7,14,1,8,13,6,11,0,9,2,15,4);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        data = _mm512_permutexvar_ps(control, data);
        __asm__ volatile("" : "+x"(data));
        data = _mm512_permutexvar_ps(control, data);
        __asm__ volatile("" : "+x"(data));
        data = _mm512_permutexvar_ps(control, data);
        __asm__ volatile("" : "+x"(data));
        data = _mm512_permutexvar_ps(control, data);
        __asm__ volatile("" : "+x"(data));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(data);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 4);
}

/* --- vpermps zmm throughput (independent streams) --- */
static double vpermps_zmm_throughput(uint64_t iterations)
{
    __m512 acc0 = _mm512_set_ps(16.0f,15.0f,14.0f,13.0f,12.0f,11.0f,10.0f,9.0f,
                                 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f,1.0f);
    __m512 acc1 = _mm512_set_ps(1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f,
                                 9.0f,10.0f,11.0f,12.0f,13.0f,14.0f,15.0f,16.0f);
    __m512 acc2 = _mm512_set1_ps(1.5f);
    __m512 acc3 = _mm512_set1_ps(2.5f);
    __m512 acc4 = _mm512_set1_ps(3.5f);
    __m512 acc5 = _mm512_set1_ps(4.5f);
    __m512 acc6 = _mm512_set1_ps(5.5f);
    __m512 acc7 = _mm512_set1_ps(6.5f);
    __m512i ctrl = _mm512_set_epi32(0,7,2,5,4,1,6,3,12,15,10,9,8,11,14,13);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_permutexvar_ps(ctrl, acc0);
        acc1 = _mm512_permutexvar_ps(ctrl, acc1);
        acc2 = _mm512_permutexvar_ps(ctrl, acc2);
        acc3 = _mm512_permutexvar_ps(ctrl, acc3);
        acc4 = _mm512_permutexvar_ps(ctrl, acc4);
        acc5 = _mm512_permutexvar_ps(ctrl, acc5);
        acc6 = _mm512_permutexvar_ps(ctrl, acc6);
        acc7 = _mm512_permutexvar_ps(ctrl, acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc0) + _mm512_reduce_add_ps(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- vpermi2d zmm throughput (2-source integer permute) --- */
static double vpermi2d_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi32(1);
    __m512i acc1 = _mm512_set1_epi32(2);
    __m512i acc2 = _mm512_set1_epi32(3);
    __m512i acc3 = _mm512_set1_epi32(4);
    __m512i acc4 = _mm512_set1_epi32(5);
    __m512i acc5 = _mm512_set1_epi32(6);
    __m512i acc6 = _mm512_set1_epi32(7);
    __m512i acc7 = _mm512_set1_epi32(8);
    /* Indices: low 5 bits select from 32 total elements (16 from a, 16 from b) */
    __m512i idx = _mm512_set_epi32(31,17,3,29,15,1,27,13,25,11,0,23,9,21,7,19);
    __m512i source_b = _mm512_set_epi32(32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        __m512i r0 = _mm512_permutex2var_epi32(acc0, idx, source_b);
        __m512i r1 = _mm512_permutex2var_epi32(acc1, idx, source_b);
        __m512i r2 = _mm512_permutex2var_epi32(acc2, idx, source_b);
        __m512i r3 = _mm512_permutex2var_epi32(acc3, idx, source_b);
        __m512i r4 = _mm512_permutex2var_epi32(acc4, idx, source_b);
        __m512i r5 = _mm512_permutex2var_epi32(acc5, idx, source_b);
        __m512i r6 = _mm512_permutex2var_epi32(acc6, idx, source_b);
        __m512i r7 = _mm512_permutex2var_epi32(acc7, idx, source_b);
        acc0 = r0; acc1 = r1; acc2 = r2; acc3 = r3;
        acc4 = r4; acc5 = r5; acc6 = r6; acc7 = r7;
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc0) + _mm512_reduce_add_epi32(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- vpermb zmm throughput (byte-granularity, AVX512-VBMI) --- */
static double vpermb_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi8(1);
    __m512i acc1 = _mm512_set1_epi8(2);
    __m512i acc2 = _mm512_set1_epi8(3);
    __m512i acc3 = _mm512_set1_epi8(4);
    __m512i acc4 = _mm512_set1_epi8(5);
    __m512i acc5 = _mm512_set1_epi8(6);
    __m512i acc6 = _mm512_set1_epi8(7);
    __m512i acc7 = _mm512_set1_epi8(8);
    /* Byte permute index: select from 64 bytes */
    __m512i ctrl = _mm512_set_epi8(
        63,0,61,2,59,4,57,6,55,8,53,10,51,12,49,14,
        47,16,45,18,43,20,41,22,39,24,37,26,35,28,33,30,
        31,32,29,34,27,36,25,38,23,40,21,42,19,44,17,46,
        15,48,13,50,11,52,9,54,7,56,5,58,3,60,1,62);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_permutexvar_epi8(ctrl, acc0);
        acc1 = _mm512_permutexvar_epi8(ctrl, acc1);
        acc2 = _mm512_permutexvar_epi8(ctrl, acc2);
        acc3 = _mm512_permutexvar_epi8(ctrl, acc3);
        acc4 = _mm512_permutexvar_epi8(ctrl, acc4);
        acc5 = _mm512_permutexvar_epi8(ctrl, acc5);
        acc6 = _mm512_permutexvar_epi8(ctrl, acc6);
        acc7 = _mm512_permutexvar_epi8(ctrl, acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) + _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- vpermps ymm throughput (256-bit baseline comparison) --- */
static double vpermps_ymm_throughput(uint64_t iterations)
{
    __m256 acc0 = _mm256_set_ps(8.0f,7.0f,6.0f,5.0f,4.0f,3.0f,2.0f,1.0f);
    __m256 acc1 = _mm256_set_ps(1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f);
    __m256 acc2 = _mm256_set1_ps(1.5f);
    __m256 acc3 = _mm256_set1_ps(2.5f);
    __m256 acc4 = _mm256_set1_ps(3.5f);
    __m256 acc5 = _mm256_set1_ps(4.5f);
    __m256 acc6 = _mm256_set1_ps(5.5f);
    __m256 acc7 = _mm256_set1_ps(6.5f);
    __m256i ctrl = _mm256_set_epi32(0,7,2,5,4,1,6,3);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm256_permutevar8x32_ps(acc0, ctrl);
        acc1 = _mm256_permutevar8x32_ps(acc1, ctrl);
        acc2 = _mm256_permutevar8x32_ps(acc2, ctrl);
        acc3 = _mm256_permutevar8x32_ps(acc3, ctrl);
        acc4 = _mm256_permutevar8x32_ps(acc4, ctrl);
        acc5 = _mm256_permutevar8x32_ps(acc5, ctrl);
        acc6 = _mm256_permutevar8x32_ps(acc6, ctrl);
        acc7 = _mm256_permutevar8x32_ps(acc7, ctrl);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    __m256 sum = _mm256_add_ps(_mm256_add_ps(acc0, acc1), _mm256_add_ps(acc2, acc3));
    sum = _mm256_add_ps(sum, _mm256_add_ps(_mm256_add_ps(acc4, acc5), _mm256_add_ps(acc6, acc7)));
    volatile float sink;
    _mm_store_ss((float *)&sink, _mm256_castps256_ps128(sum));
    return (double)(t1 - t0) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_shuffle\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double vpermps_zmm_lat  = vpermps_zmm_latency(cfg.iterations);
    double vpermps_zmm_tp   = vpermps_zmm_throughput(cfg.iterations);
    double vpermi2d_zmm_tp  = vpermi2d_zmm_throughput(cfg.iterations);
    double vpermb_zmm_tp    = vpermb_zmm_throughput(cfg.iterations);
    double vpermps_ymm_tp   = vpermps_ymm_throughput(cfg.iterations);

    printf("vpermps_zmm_latency_cycles=%.4f\n", vpermps_zmm_lat);
    printf("vpermps_zmm_throughput_cycles=%.4f\n", vpermps_zmm_tp);
    printf("vpermi2d_zmm_throughput_cycles=%.4f\n", vpermi2d_zmm_tp);
    printf("vpermb_zmm_throughput_cycles=%.4f\n", vpermb_zmm_tp);
    printf("vpermps_ymm_throughput_cycles=%.4f\n", vpermps_ymm_tp);
    printf("zmm_vs_ymm_permute_ratio=%.3f\n", vpermps_zmm_tp / vpermps_ymm_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_shuffle,vpermps_zmm_lat=%.4f,vpermps_zmm_tp=%.4f,"
               "vpermi2d_zmm_tp=%.4f,vpermb_zmm_tp=%.4f,vpermps_ymm_tp=%.4f,"
               "zmm_ymm_ratio=%.3f\n",
               vpermps_zmm_lat, vpermps_zmm_tp, vpermi2d_zmm_tp,
               vpermb_zmm_tp, vpermps_ymm_tp, vpermps_zmm_tp / vpermps_ymm_tp);
    return 0;
}
