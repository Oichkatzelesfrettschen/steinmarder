#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_vbmi2.c -- AVX-512 VBMI2 extension probe on Zen 4.
 *
 * Measures vpcompressb/w (conditional byte/word compress), vpexpandb
 * (conditional byte expand), and vpshldvd/vpshrdvd/vpshldvw (variable-count
 * concatenate-and-shift).
 *
 * vpcompressb is tested with multiple mask densities (100%, 50%, 25%,
 * single-bit) to detect data-dependent throughput variation.
 */

/* --- vpcompressb zmm throughput with given mask density --- */
static double vpcompressb_throughput(uint64_t iterations, __mmask64 compress_mask)
{
    __m512i acc0 = _mm512_set1_epi8(1);
    __m512i acc1 = _mm512_set1_epi8(2);
    __m512i acc2 = _mm512_set1_epi8(3);
    __m512i acc3 = _mm512_set1_epi8(4);
    __m512i acc4 = _mm512_set1_epi8(5);
    __m512i acc5 = _mm512_set1_epi8(6);
    __m512i acc6 = _mm512_set1_epi8(7);
    __m512i acc7 = _mm512_set1_epi8(8);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_maskz_compress_epi8(compress_mask, acc0);
        acc1 = _mm512_maskz_compress_epi8(compress_mask, acc1);
        acc2 = _mm512_maskz_compress_epi8(compress_mask, acc2);
        acc3 = _mm512_maskz_compress_epi8(compress_mask, acc3);
        acc4 = _mm512_maskz_compress_epi8(compress_mask, acc4);
        acc5 = _mm512_maskz_compress_epi8(compress_mask, acc5);
        acc6 = _mm512_maskz_compress_epi8(compress_mask, acc6);
        acc7 = _mm512_maskz_compress_epi8(compress_mask, acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpcompressw zmm throughput (word compress, all-1s mask) --- */
static double vpcompressw_throughput(uint64_t iterations)
{
    __mmask32 compress_mask = 0xFFFFFFFFu;
    __m512i acc0 = _mm512_set1_epi16(1);
    __m512i acc1 = _mm512_set1_epi16(2);
    __m512i acc2 = _mm512_set1_epi16(3);
    __m512i acc3 = _mm512_set1_epi16(4);
    __m512i acc4 = _mm512_set1_epi16(5);
    __m512i acc5 = _mm512_set1_epi16(6);
    __m512i acc6 = _mm512_set1_epi16(7);
    __m512i acc7 = _mm512_set1_epi16(8);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_maskz_compress_epi16(compress_mask, acc0);
        acc1 = _mm512_maskz_compress_epi16(compress_mask, acc1);
        acc2 = _mm512_maskz_compress_epi16(compress_mask, acc2);
        acc3 = _mm512_maskz_compress_epi16(compress_mask, acc3);
        acc4 = _mm512_maskz_compress_epi16(compress_mask, acc4);
        acc5 = _mm512_maskz_compress_epi16(compress_mask, acc5);
        acc6 = _mm512_maskz_compress_epi16(compress_mask, acc6);
        acc7 = _mm512_maskz_compress_epi16(compress_mask, acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpexpandb zmm throughput (byte expand, 50% mask) --- */
static double vpexpandb_throughput(uint64_t iterations)
{
    __mmask64 expand_mask = 0xAAAAAAAAAAAAAAAAULL; /* 50% density */
    __m512i acc0 = _mm512_set1_epi8(1);
    __m512i acc1 = _mm512_set1_epi8(2);
    __m512i acc2 = _mm512_set1_epi8(3);
    __m512i acc3 = _mm512_set1_epi8(4);
    __m512i acc4 = _mm512_set1_epi8(5);
    __m512i acc5 = _mm512_set1_epi8(6);
    __m512i acc6 = _mm512_set1_epi8(7);
    __m512i acc7 = _mm512_set1_epi8(8);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_maskz_expand_epi8(expand_mask, acc0);
        acc1 = _mm512_maskz_expand_epi8(expand_mask, acc1);
        acc2 = _mm512_maskz_expand_epi8(expand_mask, acc2);
        acc3 = _mm512_maskz_expand_epi8(expand_mask, acc3);
        acc4 = _mm512_maskz_expand_epi8(expand_mask, acc4);
        acc5 = _mm512_maskz_expand_epi8(expand_mask, acc5);
        acc6 = _mm512_maskz_expand_epi8(expand_mask, acc6);
        acc7 = _mm512_maskz_expand_epi8(expand_mask, acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpshldvd zmm throughput (variable double-shift left, 32-bit) --- */
static double vpshldvd_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi32(0x12345678);
    __m512i acc1 = _mm512_set1_epi32(0x23456789);
    __m512i acc2 = _mm512_set1_epi32(0x3456789A);
    __m512i acc3 = _mm512_set1_epi32(0x456789AB);
    __m512i acc4 = _mm512_set1_epi32(0x56789ABC);
    __m512i acc5 = _mm512_set1_epi32(0x6789ABCD);
    __m512i acc6 = _mm512_set1_epi32(0x789ABCDE);
    __m512i acc7 = _mm512_set1_epi32(0x89ABCDEF);
    __m512i operand_b = _mm512_set1_epi32(0xDEADBEEF);
    __m512i shift_count = _mm512_set1_epi32(7);
    __asm__ volatile("" : "+x"(operand_b), "+x"(shift_count));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_shldv_epi32(acc0, operand_b, shift_count);
        acc1 = _mm512_shldv_epi32(acc1, operand_b, shift_count);
        acc2 = _mm512_shldv_epi32(acc2, operand_b, shift_count);
        acc3 = _mm512_shldv_epi32(acc3, operand_b, shift_count);
        acc4 = _mm512_shldv_epi32(acc4, operand_b, shift_count);
        acc5 = _mm512_shldv_epi32(acc5, operand_b, shift_count);
        acc6 = _mm512_shldv_epi32(acc6, operand_b, shift_count);
        acc7 = _mm512_shldv_epi32(acc7, operand_b, shift_count);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc0) + _mm512_reduce_add_epi32(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpshrdvd zmm throughput (variable double-shift right, 32-bit) --- */
static double vpshrdvd_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi32(0x12345678);
    __m512i acc1 = _mm512_set1_epi32(0x23456789);
    __m512i acc2 = _mm512_set1_epi32(0x3456789A);
    __m512i acc3 = _mm512_set1_epi32(0x456789AB);
    __m512i acc4 = _mm512_set1_epi32(0x56789ABC);
    __m512i acc5 = _mm512_set1_epi32(0x6789ABCD);
    __m512i acc6 = _mm512_set1_epi32(0x789ABCDE);
    __m512i acc7 = _mm512_set1_epi32(0x89ABCDEF);
    __m512i operand_b = _mm512_set1_epi32(0xDEADBEEF);
    __m512i shift_count = _mm512_set1_epi32(7);
    __asm__ volatile("" : "+x"(operand_b), "+x"(shift_count));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_shrdv_epi32(acc0, operand_b, shift_count);
        acc1 = _mm512_shrdv_epi32(acc1, operand_b, shift_count);
        acc2 = _mm512_shrdv_epi32(acc2, operand_b, shift_count);
        acc3 = _mm512_shrdv_epi32(acc3, operand_b, shift_count);
        acc4 = _mm512_shrdv_epi32(acc4, operand_b, shift_count);
        acc5 = _mm512_shrdv_epi32(acc5, operand_b, shift_count);
        acc6 = _mm512_shrdv_epi32(acc6, operand_b, shift_count);
        acc7 = _mm512_shrdv_epi32(acc7, operand_b, shift_count);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc0) + _mm512_reduce_add_epi32(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpshldvw zmm throughput (variable double-shift left, 16-bit) --- */
static double vpshldvw_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi16(0x1234);
    __m512i acc1 = _mm512_set1_epi16(0x2345);
    __m512i acc2 = _mm512_set1_epi16(0x3456);
    __m512i acc3 = _mm512_set1_epi16(0x4567);
    __m512i acc4 = _mm512_set1_epi16(0x5678);
    __m512i acc5 = _mm512_set1_epi16(0x6789);
    __m512i acc6 = _mm512_set1_epi16(0x789A);
    __m512i acc7 = _mm512_set1_epi16((short)0x89AB);
    __m512i operand_b = _mm512_set1_epi16((short)0xBEEF);
    __m512i shift_count = _mm512_set1_epi16(5);
    __asm__ volatile("" : "+x"(operand_b), "+x"(shift_count));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_shldv_epi16(acc0, operand_b, shift_count);
        acc1 = _mm512_shldv_epi16(acc1, operand_b, shift_count);
        acc2 = _mm512_shldv_epi16(acc2, operand_b, shift_count);
        acc3 = _mm512_shldv_epi16(acc3, operand_b, shift_count);
        acc4 = _mm512_shldv_epi16(acc4, operand_b, shift_count);
        acc5 = _mm512_shldv_epi16(acc5, operand_b, shift_count);
        acc6 = _mm512_shldv_epi16(acc6, operand_b, shift_count);
        acc7 = _mm512_shldv_epi16(acc7, operand_b, shift_count);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
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

    printf("probe=avx512_vbmi2\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    /* vpcompressb with different mask densities */
    __mmask64 mask_all    = 0xFFFFFFFFFFFFFFFFULL;  /* 64 of 64 = 100% */
    __mmask64 mask_half   = 0xAAAAAAAAAAAAAAAAULL;  /* 32 of 64 = 50%  */
    __mmask64 mask_quarter= 0x8888888888888888ULL;  /* 16 of 64 = 25%  */
    __mmask64 mask_single = 0x0000000000000001ULL;  /*  1 of 64 ~  2%  */

    double compressb_all     = vpcompressb_throughput(cfg.iterations, mask_all);
    double compressb_half    = vpcompressb_throughput(cfg.iterations, mask_half);
    double compressb_quarter = vpcompressb_throughput(cfg.iterations, mask_quarter);
    double compressb_single  = vpcompressb_throughput(cfg.iterations, mask_single);
    double compressw_tp      = vpcompressw_throughput(cfg.iterations);
    double expandb_tp        = vpexpandb_throughput(cfg.iterations);
    double shldvd_tp         = vpshldvd_throughput(cfg.iterations);
    double shrdvd_tp         = vpshrdvd_throughput(cfg.iterations);
    double shldvw_tp         = vpshldvw_throughput(cfg.iterations);

    printf("=== AVX-512 VBMI2: vpcompressb (mask density sweep) ===\n");
    printf("vpcompressb_100pct_throughput_cycles=%.4f\n", compressb_all);
    printf("vpcompressb_50pct_throughput_cycles=%.4f\n", compressb_half);
    printf("vpcompressb_25pct_throughput_cycles=%.4f\n", compressb_quarter);
    printf("vpcompressb_single_throughput_cycles=%.4f\n", compressb_single);
    printf("vpcompressb_density_ratio_100vs25=%.3f\n",
           compressb_all / compressb_quarter);
    printf("\n");

    printf("=== AVX-512 VBMI2: vpcompressw / vpexpandb ===\n");
    printf("vpcompressw_throughput_cycles=%.4f\n", compressw_tp);
    printf("vpexpandb_throughput_cycles=%.4f\n", expandb_tp);
    printf("\n");

    printf("=== AVX-512 VBMI2: concatenate-and-shift ===\n");
    printf("vpshldvd_throughput_cycles=%.4f\n", shldvd_tp);
    printf("vpshrdvd_throughput_cycles=%.4f\n", shrdvd_tp);
    printf("vpshldvw_throughput_cycles=%.4f\n", shldvw_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_vbmi2,"
               "compressb_100=%.4f,compressb_50=%.4f,compressb_25=%.4f,compressb_1=%.4f,"
               "compressw=%.4f,expandb=%.4f,"
               "shldvd=%.4f,shrdvd=%.4f,shldvw=%.4f\n",
               compressb_all, compressb_half, compressb_quarter, compressb_single,
               compressw_tp, expandb_tp, shldvd_tp, shrdvd_tp, shldvw_tp);
    return 0;
}
