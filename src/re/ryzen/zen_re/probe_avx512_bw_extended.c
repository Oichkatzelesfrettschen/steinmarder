#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_bw_extended.c -- Fill remaining AVX-512BW gaps on Zen 4.
 *
 * Measures vpshufb (byte shuffle LUT), vpmaxub/vpminub (unsigned byte
 * max/min), vpmaxsb/vpminsb (signed byte max/min), vpackuswb/vpackusdw
 * (saturating pack), and vpcmpub (unsigned byte compare to mask).
 *
 * These are critical for image processing, quantization, and UTF-8 string ops.
 */

/* --- vpshufb zmm throughput (byte shuffle -- the LUT workhorse) --- */
static double vpshufb_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi8(0x10);
    __m512i acc1 = _mm512_set1_epi8(0x20);
    __m512i acc2 = _mm512_set1_epi8(0x30);
    __m512i acc3 = _mm512_set1_epi8(0x40);
    __m512i acc4 = _mm512_set1_epi8(0x50);
    __m512i acc5 = _mm512_set1_epi8(0x60);
    __m512i acc6 = _mm512_set1_epi8(0x70);
    __m512i acc7 = _mm512_set1_epi8(0x01);
    /* Shuffle control: within each 128-bit lane, pick bytes 0..15 in reverse */
    __m512i shuffle_control = _mm512_set_epi8(
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    __asm__ volatile("" : "+x"(shuffle_control));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_shuffle_epi8(acc0, shuffle_control);
        acc1 = _mm512_shuffle_epi8(acc1, shuffle_control);
        acc2 = _mm512_shuffle_epi8(acc2, shuffle_control);
        acc3 = _mm512_shuffle_epi8(acc3, shuffle_control);
        acc4 = _mm512_shuffle_epi8(acc4, shuffle_control);
        acc5 = _mm512_shuffle_epi8(acc5, shuffle_control);
        acc6 = _mm512_shuffle_epi8(acc6, shuffle_control);
        acc7 = _mm512_shuffle_epi8(acc7, shuffle_control);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpmaxub zmm throughput (unsigned byte max) --- */
static double vpmaxub_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi8(100);
    __m512i acc1 = _mm512_set1_epi8(101);
    __m512i acc2 = _mm512_set1_epi8(102);
    __m512i acc3 = _mm512_set1_epi8(103);
    __m512i acc4 = _mm512_set1_epi8(104);
    __m512i acc5 = _mm512_set1_epi8(105);
    __m512i acc6 = _mm512_set1_epi8(106);
    __m512i acc7 = _mm512_set1_epi8(107);
    __m512i operand_b = _mm512_set1_epi8(50);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_max_epu8(acc0, operand_b);
        acc1 = _mm512_max_epu8(acc1, operand_b);
        acc2 = _mm512_max_epu8(acc2, operand_b);
        acc3 = _mm512_max_epu8(acc3, operand_b);
        acc4 = _mm512_max_epu8(acc4, operand_b);
        acc5 = _mm512_max_epu8(acc5, operand_b);
        acc6 = _mm512_max_epu8(acc6, operand_b);
        acc7 = _mm512_max_epu8(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpminub zmm throughput (unsigned byte min) --- */
static double vpminub_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi8(100);
    __m512i acc1 = _mm512_set1_epi8(101);
    __m512i acc2 = _mm512_set1_epi8(102);
    __m512i acc3 = _mm512_set1_epi8(103);
    __m512i acc4 = _mm512_set1_epi8(104);
    __m512i acc5 = _mm512_set1_epi8(105);
    __m512i acc6 = _mm512_set1_epi8(106);
    __m512i acc7 = _mm512_set1_epi8(107);
    __m512i operand_b = _mm512_set1_epi8((char)200);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_min_epu8(acc0, operand_b);
        acc1 = _mm512_min_epu8(acc1, operand_b);
        acc2 = _mm512_min_epu8(acc2, operand_b);
        acc3 = _mm512_min_epu8(acc3, operand_b);
        acc4 = _mm512_min_epu8(acc4, operand_b);
        acc5 = _mm512_min_epu8(acc5, operand_b);
        acc6 = _mm512_min_epu8(acc6, operand_b);
        acc7 = _mm512_min_epu8(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpmaxsb zmm throughput (signed byte max) --- */
static double vpmaxsb_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi8(50);
    __m512i acc1 = _mm512_set1_epi8(51);
    __m512i acc2 = _mm512_set1_epi8(52);
    __m512i acc3 = _mm512_set1_epi8(53);
    __m512i acc4 = _mm512_set1_epi8(54);
    __m512i acc5 = _mm512_set1_epi8(55);
    __m512i acc6 = _mm512_set1_epi8(56);
    __m512i acc7 = _mm512_set1_epi8(57);
    __m512i operand_b = _mm512_set1_epi8(-10);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_max_epi8(acc0, operand_b);
        acc1 = _mm512_max_epi8(acc1, operand_b);
        acc2 = _mm512_max_epi8(acc2, operand_b);
        acc3 = _mm512_max_epi8(acc3, operand_b);
        acc4 = _mm512_max_epi8(acc4, operand_b);
        acc5 = _mm512_max_epi8(acc5, operand_b);
        acc6 = _mm512_max_epi8(acc6, operand_b);
        acc7 = _mm512_max_epi8(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpminsb zmm throughput (signed byte min) --- */
static double vpminsb_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi8(50);
    __m512i acc1 = _mm512_set1_epi8(51);
    __m512i acc2 = _mm512_set1_epi8(52);
    __m512i acc3 = _mm512_set1_epi8(53);
    __m512i acc4 = _mm512_set1_epi8(54);
    __m512i acc5 = _mm512_set1_epi8(55);
    __m512i acc6 = _mm512_set1_epi8(56);
    __m512i acc7 = _mm512_set1_epi8(57);
    __m512i operand_b = _mm512_set1_epi8(127);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_min_epi8(acc0, operand_b);
        acc1 = _mm512_min_epi8(acc1, operand_b);
        acc2 = _mm512_min_epi8(acc2, operand_b);
        acc3 = _mm512_min_epi8(acc3, operand_b);
        acc4 = _mm512_min_epi8(acc4, operand_b);
        acc5 = _mm512_min_epi8(acc5, operand_b);
        acc6 = _mm512_min_epi8(acc6, operand_b);
        acc7 = _mm512_min_epi8(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpackuswb zmm throughput (pack int16 -> uint8 with saturation) --- */
static double vpackuswb_throughput(uint64_t iterations)
{
    __m512i src_a0 = _mm512_set1_epi16(100);
    __m512i src_a1 = _mm512_set1_epi16(101);
    __m512i src_a2 = _mm512_set1_epi16(102);
    __m512i src_a3 = _mm512_set1_epi16(103);
    __m512i src_a4 = _mm512_set1_epi16(104);
    __m512i src_a5 = _mm512_set1_epi16(105);
    __m512i src_a6 = _mm512_set1_epi16(106);
    __m512i src_a7 = _mm512_set1_epi16(107);
    __m512i src_b = _mm512_set1_epi16(200);
    __asm__ volatile("" : "+x"(src_a0), "+x"(src_a1), "+x"(src_a2), "+x"(src_a3),
                          "+x"(src_a4), "+x"(src_a5), "+x"(src_a6), "+x"(src_a7),
                          "+x"(src_b));

    __m512i dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        dst0 = _mm512_packus_epi16(src_a0, src_b);
        dst1 = _mm512_packus_epi16(src_a1, src_b);
        dst2 = _mm512_packus_epi16(src_a2, src_b);
        dst3 = _mm512_packus_epi16(src_a3, src_b);
        dst4 = _mm512_packus_epi16(src_a4, src_b);
        dst5 = _mm512_packus_epi16(src_a5, src_b);
        dst6 = _mm512_packus_epi16(src_a6, src_b);
        dst7 = _mm512_packus_epi16(src_a7, src_b);
        __asm__ volatile("" : "+x"(dst0), "+x"(dst1), "+x"(dst2), "+x"(dst3),
                              "+x"(dst4), "+x"(dst5), "+x"(dst6), "+x"(dst7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(dst0) +
                              _mm512_reduce_add_epi64(dst4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpackusdw zmm throughput (pack int32 -> uint16 with saturation) --- */
static double vpackusdw_throughput(uint64_t iterations)
{
    __m512i src_a0 = _mm512_set1_epi32(30000);
    __m512i src_a1 = _mm512_set1_epi32(30001);
    __m512i src_a2 = _mm512_set1_epi32(30002);
    __m512i src_a3 = _mm512_set1_epi32(30003);
    __m512i src_a4 = _mm512_set1_epi32(30004);
    __m512i src_a5 = _mm512_set1_epi32(30005);
    __m512i src_a6 = _mm512_set1_epi32(30006);
    __m512i src_a7 = _mm512_set1_epi32(30007);
    __m512i src_b = _mm512_set1_epi32(60000);
    __asm__ volatile("" : "+x"(src_a0), "+x"(src_a1), "+x"(src_a2), "+x"(src_a3),
                          "+x"(src_a4), "+x"(src_a5), "+x"(src_a6), "+x"(src_a7),
                          "+x"(src_b));

    __m512i dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        dst0 = _mm512_packus_epi32(src_a0, src_b);
        dst1 = _mm512_packus_epi32(src_a1, src_b);
        dst2 = _mm512_packus_epi32(src_a2, src_b);
        dst3 = _mm512_packus_epi32(src_a3, src_b);
        dst4 = _mm512_packus_epi32(src_a4, src_b);
        dst5 = _mm512_packus_epi32(src_a5, src_b);
        dst6 = _mm512_packus_epi32(src_a6, src_b);
        dst7 = _mm512_packus_epi32(src_a7, src_b);
        __asm__ volatile("" : "+x"(dst0), "+x"(dst1), "+x"(dst2), "+x"(dst3),
                              "+x"(dst4), "+x"(dst5), "+x"(dst6), "+x"(dst7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(dst0) +
                              _mm512_reduce_add_epi64(dst4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpcmpub zmm -> k throughput (unsigned byte compare to mask) --- */
static double vpcmpub_throughput(uint64_t iterations)
{
    __m512i src0 = _mm512_set1_epi8(100);
    __m512i src1 = _mm512_set1_epi8(101);
    __m512i src2 = _mm512_set1_epi8(102);
    __m512i src3 = _mm512_set1_epi8(103);
    __m512i src4 = _mm512_set1_epi8(104);
    __m512i src5 = _mm512_set1_epi8(105);
    __m512i src6 = _mm512_set1_epi8(106);
    __m512i src7 = _mm512_set1_epi8(107);
    __m512i threshold = _mm512_set1_epi8(99);
    __asm__ volatile("" : "+x"(src0), "+x"(src1), "+x"(src2), "+x"(src3),
                          "+x"(src4), "+x"(src5), "+x"(src6), "+x"(src7),
                          "+x"(threshold));

    __mmask64 mask_result = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* _MM_CMPINT_NLE = 6 (greater than, unsigned) */
        __mmask64 k0 = _mm512_cmp_epu8_mask(src0, threshold, _MM_CMPINT_NLE);
        __mmask64 k1 = _mm512_cmp_epu8_mask(src1, threshold, _MM_CMPINT_NLE);
        __mmask64 k2 = _mm512_cmp_epu8_mask(src2, threshold, _MM_CMPINT_NLE);
        __mmask64 k3 = _mm512_cmp_epu8_mask(src3, threshold, _MM_CMPINT_NLE);
        __mmask64 k4 = _mm512_cmp_epu8_mask(src4, threshold, _MM_CMPINT_NLE);
        __mmask64 k5 = _mm512_cmp_epu8_mask(src5, threshold, _MM_CMPINT_NLE);
        __mmask64 k6 = _mm512_cmp_epu8_mask(src6, threshold, _MM_CMPINT_NLE);
        __mmask64 k7 = _mm512_cmp_epu8_mask(src7, threshold, _MM_CMPINT_NLE);
        mask_result = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;
        __asm__ volatile("" : "+r"(mask_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = (long long)mask_result;
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

    printf("probe=avx512_bw_extended\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double vpshufb_tp   = vpshufb_throughput(cfg.iterations);
    double vpmaxub_tp   = vpmaxub_throughput(cfg.iterations);
    double vpminub_tp   = vpminub_throughput(cfg.iterations);
    double vpmaxsb_tp   = vpmaxsb_throughput(cfg.iterations);
    double vpminsb_tp   = vpminsb_throughput(cfg.iterations);
    double vpackuswb_tp = vpackuswb_throughput(cfg.iterations);
    double vpackusdw_tp = vpackusdw_throughput(cfg.iterations);
    double vpcmpub_tp   = vpcmpub_throughput(cfg.iterations);

    printf("=== AVX-512BW: vpshufb zmm (byte shuffle LUT) ===\n");
    printf("vpshufb_throughput_cycles=%.4f\n", vpshufb_tp);
    printf("\n");

    printf("=== AVX-512BW: byte max/min ===\n");
    printf("vpmaxub_throughput_cycles=%.4f\n", vpmaxub_tp);
    printf("vpminub_throughput_cycles=%.4f\n", vpminub_tp);
    printf("vpmaxsb_throughput_cycles=%.4f\n", vpmaxsb_tp);
    printf("vpminsb_throughput_cycles=%.4f\n", vpminsb_tp);
    printf("\n");

    printf("=== AVX-512BW: saturating pack ===\n");
    printf("vpackuswb_throughput_cycles=%.4f\n", vpackuswb_tp);
    printf("vpackusdw_throughput_cycles=%.4f\n", vpackusdw_tp);
    printf("\n");

    printf("=== AVX-512BW: unsigned byte compare to mask ===\n");
    printf("vpcmpub_throughput_cycles=%.4f\n", vpcmpub_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_bw_extended,"
               "vpshufb=%.4f,vpmaxub=%.4f,vpminub=%.4f,"
               "vpmaxsb=%.4f,vpminsb=%.4f,"
               "vpackuswb=%.4f,vpackusdw=%.4f,vpcmpub=%.4f\n",
               vpshufb_tp, vpmaxub_tp, vpminub_tp,
               vpmaxsb_tp, vpminsb_tp,
               vpackuswb_tp, vpackusdw_tp, vpcmpub_tp);
    return 0;
}
