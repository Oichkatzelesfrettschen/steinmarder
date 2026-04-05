#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_dq.c -- AVX-512DQ extension probe on Zen 4.
 *
 * Measures vpmullq (native 64x64->64 integer multiply), vcvtqq2pd / vcvttpd2qq
 * (INT64 <-> FP64 converts), vrangeps, vreduceps, and vfpclassps.
 *
 * vpmullq is the key instruction: DQ provides native 64-bit SIMD integer
 * multiply that was previously assumed missing.
 */

/* --- vpmullq zmm latency (4-deep dependency chain) --- */
static double vpmullq_latency(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi64(0x0000000100000003ULL);
    __m512i multiplier  = _mm512_set1_epi64(0x0000000000000005ULL);
    __asm__ volatile("" : "+x"(multiplier));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_mullo_epi64(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi64(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi64(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi64(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

/* --- vpmullq zmm throughput (8 independent accumulators) --- */
static double vpmullq_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(1);
    __m512i acc1 = _mm512_set1_epi64(2);
    __m512i acc2 = _mm512_set1_epi64(3);
    __m512i acc3 = _mm512_set1_epi64(4);
    __m512i acc4 = _mm512_set1_epi64(5);
    __m512i acc5 = _mm512_set1_epi64(6);
    __m512i acc6 = _mm512_set1_epi64(7);
    __m512i acc7 = _mm512_set1_epi64(8);
    __m512i multiplier = _mm512_set1_epi64(0x0000000000000005ULL);
    __asm__ volatile("" : "+x"(multiplier));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_mullo_epi64(acc0, multiplier);
        acc1 = _mm512_mullo_epi64(acc1, multiplier);
        acc2 = _mm512_mullo_epi64(acc2, multiplier);
        acc3 = _mm512_mullo_epi64(acc3, multiplier);
        acc4 = _mm512_mullo_epi64(acc4, multiplier);
        acc5 = _mm512_mullo_epi64(acc5, multiplier);
        acc6 = _mm512_mullo_epi64(acc6, multiplier);
        acc7 = _mm512_mullo_epi64(acc7, multiplier);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vcvtqq2pd zmm throughput (INT64 -> FP64) --- */
static double vcvtqq2pd_throughput(uint64_t iterations)
{
    __m512i src0 = _mm512_set1_epi64(12345);
    __m512i src1 = _mm512_set1_epi64(67890);
    __m512i src2 = _mm512_set1_epi64(11111);
    __m512i src3 = _mm512_set1_epi64(22222);
    __m512i src4 = _mm512_set1_epi64(33333);
    __m512i src5 = _mm512_set1_epi64(44444);
    __m512i src6 = _mm512_set1_epi64(55555);
    __m512i src7 = _mm512_set1_epi64(66666);
    __asm__ volatile("" : "+x"(src0), "+x"(src1), "+x"(src2), "+x"(src3),
                          "+x"(src4), "+x"(src5), "+x"(src6), "+x"(src7));

    __m512d dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        dst0 = _mm512_cvtepi64_pd(src0);
        dst1 = _mm512_cvtepi64_pd(src1);
        dst2 = _mm512_cvtepi64_pd(src2);
        dst3 = _mm512_cvtepi64_pd(src3);
        dst4 = _mm512_cvtepi64_pd(src4);
        dst5 = _mm512_cvtepi64_pd(src5);
        dst6 = _mm512_cvtepi64_pd(src6);
        dst7 = _mm512_cvtepi64_pd(src7);
        __asm__ volatile("" : "+x"(dst0), "+x"(dst1), "+x"(dst2), "+x"(dst3),
                              "+x"(dst4), "+x"(dst5), "+x"(dst6), "+x"(dst7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink = _mm512_reduce_add_pd(dst0) + _mm512_reduce_add_pd(dst4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vcvttpd2qq zmm throughput (FP64 -> INT64 truncate) --- */
static double vcvttpd2qq_throughput(uint64_t iterations)
{
    __m512d src0 = _mm512_set1_pd(12345.678);
    __m512d src1 = _mm512_set1_pd(67890.123);
    __m512d src2 = _mm512_set1_pd(11111.222);
    __m512d src3 = _mm512_set1_pd(22222.333);
    __m512d src4 = _mm512_set1_pd(33333.444);
    __m512d src5 = _mm512_set1_pd(44444.555);
    __m512d src6 = _mm512_set1_pd(55555.666);
    __m512d src7 = _mm512_set1_pd(66666.777);
    __asm__ volatile("" : "+x"(src0), "+x"(src1), "+x"(src2), "+x"(src3),
                          "+x"(src4), "+x"(src5), "+x"(src6), "+x"(src7));

    __m512i dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        dst0 = _mm512_cvttpd_epi64(src0);
        dst1 = _mm512_cvttpd_epi64(src1);
        dst2 = _mm512_cvttpd_epi64(src2);
        dst3 = _mm512_cvttpd_epi64(src3);
        dst4 = _mm512_cvttpd_epi64(src4);
        dst5 = _mm512_cvttpd_epi64(src5);
        dst6 = _mm512_cvttpd_epi64(src6);
        dst7 = _mm512_cvttpd_epi64(src7);
        __asm__ volatile("" : "+x"(dst0), "+x"(dst1), "+x"(dst2), "+x"(dst3),
                              "+x"(dst4), "+x"(dst5), "+x"(dst6), "+x"(dst7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(dst0) +
                              _mm512_reduce_add_epi64(dst4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vrangeps zmm throughput (FP32 range restriction) --- */
static double vrangeps_throughput(uint64_t iterations)
{
    __m512 acc0 = _mm512_set1_ps(3.14f);
    __m512 acc1 = _mm512_set1_ps(2.71f);
    __m512 acc2 = _mm512_set1_ps(1.41f);
    __m512 acc3 = _mm512_set1_ps(1.73f);
    __m512 acc4 = _mm512_set1_ps(0.57f);
    __m512 acc5 = _mm512_set1_ps(2.23f);
    __m512 acc6 = _mm512_set1_ps(1.61f);
    __m512 acc7 = _mm512_set1_ps(0.69f);
    __m512 bound = _mm512_set1_ps(1.0f);
    __asm__ volatile("" : "+x"(bound));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* imm8=0b0010: min of abs values (clamp-like) */
        acc0 = _mm512_range_ps(acc0, bound, 0x02);
        acc1 = _mm512_range_ps(acc1, bound, 0x02);
        acc2 = _mm512_range_ps(acc2, bound, 0x02);
        acc3 = _mm512_range_ps(acc3, bound, 0x02);
        acc4 = _mm512_range_ps(acc4, bound, 0x02);
        acc5 = _mm512_range_ps(acc5, bound, 0x02);
        acc6 = _mm512_range_ps(acc6, bound, 0x02);
        acc7 = _mm512_range_ps(acc7, bound, 0x02);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc0) + _mm512_reduce_add_ps(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vreduceps zmm throughput (FP32 reduce / extract significand) --- */
static double vreduceps_throughput(uint64_t iterations)
{
    __m512 acc0 = _mm512_set1_ps(3.14f);
    __m512 acc1 = _mm512_set1_ps(2.71f);
    __m512 acc2 = _mm512_set1_ps(1.41f);
    __m512 acc3 = _mm512_set1_ps(1.73f);
    __m512 acc4 = _mm512_set1_ps(0.57f);
    __m512 acc5 = _mm512_set1_ps(2.23f);
    __m512 acc6 = _mm512_set1_ps(1.61f);
    __m512 acc7 = _mm512_set1_ps(0.69f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* imm8=0x04: round to nearest, 4 bits of precision */
        acc0 = _mm512_reduce_ps(acc0, 0x04);
        acc1 = _mm512_reduce_ps(acc1, 0x04);
        acc2 = _mm512_reduce_ps(acc2, 0x04);
        acc3 = _mm512_reduce_ps(acc3, 0x04);
        acc4 = _mm512_reduce_ps(acc4, 0x04);
        acc5 = _mm512_reduce_ps(acc5, 0x04);
        acc6 = _mm512_reduce_ps(acc6, 0x04);
        acc7 = _mm512_reduce_ps(acc7, 0x04);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc0) + _mm512_reduce_add_ps(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vfpclassps zmm -> k throughput (FP classification to mask) --- */
static double vfpclassps_throughput(uint64_t iterations)
{
    __m512 src0 = _mm512_set1_ps(0.0f);
    __m512 src1 = _mm512_set1_ps(1.0f / 0.0f);   /* +Inf */
    __m512 src2 = _mm512_set1_ps(0.0f / 0.0f);    /* NaN */
    __m512 src3 = _mm512_set1_ps(1.0e-40f);       /* denorm */
    __m512 src4 = _mm512_set1_ps(-0.0f);
    __m512 src5 = _mm512_set1_ps(42.0f);
    __m512 src6 = _mm512_set1_ps(-1.0f / 0.0f);   /* -Inf */
    __m512 src7 = _mm512_set1_ps(-3.14f);
    __asm__ volatile("" : "+x"(src0), "+x"(src1), "+x"(src2), "+x"(src3),
                          "+x"(src4), "+x"(src5), "+x"(src6), "+x"(src7));

    __mmask16 mask_result = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* imm8=0x99: detect QNaN|+Inf|-Inf|+zero|denorm */
        __mmask16 k0 = _mm512_fpclass_ps_mask(src0, 0x99);
        __mmask16 k1 = _mm512_fpclass_ps_mask(src1, 0x99);
        __mmask16 k2 = _mm512_fpclass_ps_mask(src2, 0x99);
        __mmask16 k3 = _mm512_fpclass_ps_mask(src3, 0x99);
        __mmask16 k4 = _mm512_fpclass_ps_mask(src4, 0x99);
        __mmask16 k5 = _mm512_fpclass_ps_mask(src5, 0x99);
        __mmask16 k6 = _mm512_fpclass_ps_mask(src6, 0x99);
        __mmask16 k7 = _mm512_fpclass_ps_mask(src7, 0x99);
        mask_result = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;
        __asm__ volatile("" : "+r"(mask_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = (int)mask_result;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_dq\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double vpmullq_lat       = vpmullq_latency(cfg.iterations);
    double vpmullq_tp        = vpmullq_throughput(cfg.iterations);
    double vcvtqq2pd_tp      = vcvtqq2pd_throughput(cfg.iterations);
    double vcvttpd2qq_tp     = vcvttpd2qq_throughput(cfg.iterations);
    double vrangeps_tp       = vrangeps_throughput(cfg.iterations);
    double vreduceps_tp      = vreduceps_throughput(cfg.iterations);
    double vfpclassps_tp     = vfpclassps_throughput(cfg.iterations);

    printf("=== AVX-512DQ: vpmullq (native 64-bit integer multiply) ===\n");
    printf("vpmullq_latency_cycles=%.4f\n", vpmullq_lat);
    printf("vpmullq_throughput_cycles=%.4f\n", vpmullq_tp);
    printf("vpmullq_ops_per_cycle=%.2f\n", 1.0 / vpmullq_tp);
    printf("\n");

    printf("=== AVX-512DQ: INT64 <-> FP64 conversions ===\n");
    printf("vcvtqq2pd_throughput_cycles=%.4f\n", vcvtqq2pd_tp);
    printf("vcvttpd2qq_throughput_cycles=%.4f\n", vcvttpd2qq_tp);
    printf("\n");

    printf("=== AVX-512DQ: FP32 range/reduce/classify ===\n");
    printf("vrangeps_throughput_cycles=%.4f\n", vrangeps_tp);
    printf("vreduceps_throughput_cycles=%.4f\n", vreduceps_tp);
    printf("vfpclassps_throughput_cycles=%.4f\n", vfpclassps_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_dq,vpmullq_lat=%.4f,vpmullq_tp=%.4f,"
               "vcvtqq2pd_tp=%.4f,vcvttpd2qq_tp=%.4f,"
               "vrangeps_tp=%.4f,vreduceps_tp=%.4f,vfpclassps_tp=%.4f\n",
               vpmullq_lat, vpmullq_tp, vcvtqq2pd_tp, vcvttpd2qq_tp,
               vrangeps_tp, vreduceps_tp, vfpclassps_tp);
    return 0;
}
