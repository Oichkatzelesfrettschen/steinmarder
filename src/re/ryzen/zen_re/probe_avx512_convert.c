#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_convert.c — FP32 <-> BF16 conversion throughput on Zen 4.
 *
 * Measures the cost of:
 *   1. vcvtne2ps2bf16 zmm — convert two __m512 -> one __m512bh (32 BF16 values)
 *   2. vcvtneps2bf16 ymm  — convert __m512 -> __m256bh (16 BF16 values)
 *   3. Manual BF16->FP32 expansion via slli+cvtepu16 (the reverse path)
 *
 * These measure the cost of converting weights to BF16 at prepack time
 * and activations at runtime in the NeRF MLP pipeline.
 */

/* --- vcvtne2ps2bf16 zmm throughput --- */
static double cvtne2ps2bf16_zmm_throughput(uint64_t iterations)
{
    __m512 src_lo0 = _mm512_set1_ps(1.0f);
    __m512 src_hi0 = _mm512_set1_ps(2.0f);
    __m512 src_lo1 = _mm512_set1_ps(3.0f);
    __m512 src_hi1 = _mm512_set1_ps(4.0f);
    __m512 src_lo2 = _mm512_set1_ps(5.0f);
    __m512 src_hi2 = _mm512_set1_ps(6.0f);
    __m512 src_lo3 = _mm512_set1_ps(7.0f);
    __m512 src_hi3 = _mm512_set1_ps(8.0f);
    __m512bh acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7;

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_cvtne2ps_pbh(src_hi0, src_lo0);
        acc1 = _mm512_cvtne2ps_pbh(src_hi1, src_lo1);
        acc2 = _mm512_cvtne2ps_pbh(src_hi2, src_lo2);
        acc3 = _mm512_cvtne2ps_pbh(src_hi3, src_lo3);
        acc4 = _mm512_cvtne2ps_pbh(src_lo0, src_hi0);
        acc5 = _mm512_cvtne2ps_pbh(src_lo1, src_hi1);
        acc6 = _mm512_cvtne2ps_pbh(src_lo2, src_hi2);
        acc7 = _mm512_cvtne2ps_pbh(src_lo3, src_hi3);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7)
                         : : );
        /* Feed results back as sources to prevent dead code elimination */
        src_lo0 = (__m512)acc0;
        src_hi0 = (__m512)acc1;
        src_lo1 = (__m512)acc2;
        src_hi1 = (__m512)acc3;
        src_lo2 = (__m512)acc4;
        src_hi2 = (__m512)acc5;
        src_lo3 = (__m512)acc6;
        src_hi3 = (__m512)acc7;
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = ((float *)&acc0)[0] + ((float *)&acc4)[0];
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- vcvtneps2bf16 ymm throughput (512 -> 256) --- */
static double cvtneps2bf16_ymm_throughput(uint64_t iterations)
{
    __m512 src0 = _mm512_set1_ps(1.0f);
    __m512 src1 = _mm512_set1_ps(2.0f);
    __m512 src2 = _mm512_set1_ps(3.0f);
    __m512 src3 = _mm512_set1_ps(4.0f);
    __m512 src4 = _mm512_set1_ps(5.0f);
    __m512 src5 = _mm512_set1_ps(6.0f);
    __m512 src6 = _mm512_set1_ps(7.0f);
    __m512 src7 = _mm512_set1_ps(8.0f);
    __m256bh acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7;

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_cvtneps_pbh(src0);
        acc1 = _mm512_cvtneps_pbh(src1);
        acc2 = _mm512_cvtneps_pbh(src2);
        acc3 = _mm512_cvtneps_pbh(src3);
        acc4 = _mm512_cvtneps_pbh(src4);
        acc5 = _mm512_cvtneps_pbh(src5);
        acc6 = _mm512_cvtneps_pbh(src6);
        acc7 = _mm512_cvtneps_pbh(src7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
        /* Widen results back to prevent the loop from being optimized away */
        src0 = _mm512_castps256_ps512((__m256)acc0);
        src1 = _mm512_castps256_ps512((__m256)acc1);
        src2 = _mm512_castps256_ps512((__m256)acc2);
        src3 = _mm512_castps256_ps512((__m256)acc3);
        src4 = _mm512_castps256_ps512((__m256)acc4);
        src5 = _mm512_castps256_ps512((__m256)acc5);
        src6 = _mm512_castps256_ps512((__m256)acc6);
        src7 = _mm512_castps256_ps512((__m256)acc7);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = ((float *)&acc0)[0] + ((float *)&acc4)[0];
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- Manual BF16->FP32 expansion: cvtepu16->epi32 + slli 16 --- */
static double bf16_to_fp32_manual_throughput(uint64_t iterations)
{
    /* Simulate 8 independent BF16->FP32 expansions */
    __m256i bf16_0 = _mm256_set1_epi16(0x3F80); /* bf16 1.0 */
    __m256i bf16_1 = _mm256_set1_epi16(0x4000); /* bf16 2.0 */
    __m256i bf16_2 = _mm256_set1_epi16(0x4040); /* bf16 3.0 */
    __m256i bf16_3 = _mm256_set1_epi16(0x4080); /* bf16 4.0 */
    __m256i bf16_4 = _mm256_set1_epi16(0x40A0); /* bf16 5.0 */
    __m256i bf16_5 = _mm256_set1_epi16(0x40C0); /* bf16 6.0 */
    __m256i bf16_6 = _mm256_set1_epi16(0x40E0); /* bf16 7.0 */
    __m256i bf16_7 = _mm256_set1_epi16(0x4100); /* bf16 8.0 */

    __m512 fp32_0, fp32_1, fp32_2, fp32_3;
    __m512 fp32_4, fp32_5, fp32_6, fp32_7;

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* BF16->FP32: zero-extend u16->u32, then shift left 16 to place in FP32 position */
        fp32_0 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_0), 16);
        fp32_1 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_1), 16);
        fp32_2 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_2), 16);
        fp32_3 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_3), 16);
        fp32_4 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_4), 16);
        fp32_5 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_5), 16);
        fp32_6 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_6), 16);
        fp32_7 = (__m512)_mm512_slli_epi32(_mm512_cvtepu16_epi32(bf16_7), 16);
        __asm__ volatile("" : "+x"(fp32_0), "+x"(fp32_1), "+x"(fp32_2), "+x"(fp32_3),
                              "+x"(fp32_4), "+x"(fp32_5), "+x"(fp32_6), "+x"(fp32_7));
        /* Feed back to prevent hoisting */
        bf16_0 = _mm512_cvtepi32_epi16((__m512i)fp32_0);
        bf16_1 = _mm512_cvtepi32_epi16((__m512i)fp32_1);
        bf16_2 = _mm512_cvtepi32_epi16((__m512i)fp32_2);
        bf16_3 = _mm512_cvtepi32_epi16((__m512i)fp32_3);
        bf16_4 = _mm512_cvtepi32_epi16((__m512i)fp32_4);
        bf16_5 = _mm512_cvtepi32_epi16((__m512i)fp32_5);
        bf16_6 = _mm512_cvtepi32_epi16((__m512i)fp32_6);
        bf16_7 = _mm512_cvtepi32_epi16((__m512i)fp32_7);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(fp32_0) + _mm512_reduce_add_ps(fp32_4);
    (void)sink;
    /* Each iteration does 8 expand ops (cvtepu16+slli pair = 1 logical expand) */
    return (double)(t1 - t0) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_convert\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double cvtne2_tp  = cvtne2ps2bf16_zmm_throughput(cfg.iterations);
    double cvtne_tp   = cvtneps2bf16_ymm_throughput(cfg.iterations);
    double expand_tp  = bf16_to_fp32_manual_throughput(cfg.iterations);

    printf("vcvtne2ps2bf16_zmm_throughput_cycles=%.4f\n", cvtne2_tp);
    printf("vcvtneps2bf16_ymm_throughput_cycles=%.4f\n", cvtne_tp);
    printf("bf16_to_fp32_expand_throughput_cycles=%.4f\n", expand_tp);
    printf("bf16_values_per_cvtne2=32\n");
    printf("bf16_values_per_cvtne=16\n");
    printf("fp32_values_per_expand=16\n");
    printf("cvtne2_bf16_per_cycle=%.1f\n", 32.0 / cvtne2_tp);
    printf("cvtne_bf16_per_cycle=%.1f\n", 16.0 / cvtne_tp);
    printf("expand_fp32_per_cycle=%.1f\n", 16.0 / expand_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_convert,cvtne2_tp=%.4f,cvtne_tp=%.4f,"
               "expand_tp=%.4f,cvtne2_per_cyc=%.1f,expand_per_cyc=%.1f\n",
               cvtne2_tp, cvtne_tp, expand_tp,
               32.0 / cvtne2_tp, 16.0 / expand_tp);
    return 0;
}
