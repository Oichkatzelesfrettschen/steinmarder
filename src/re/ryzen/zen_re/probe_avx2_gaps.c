#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_avx2_gaps.c -- Fill the 17 missing AVX2 mnemonics from the
 * Zen 4 coverage gap report.
 *
 * Covers: VEXTRACTI128, VINSERTI128, VPALIGNR, VPAND, VPANDN, VPBLENDD,
 * VPHADDD, VPHADDW, VPHSUBD, VPHSUBW, VPMASKMOVD, VPMASKMOVQ,
 * VPMOVMSKB, VPOR, VPSHUFHW, VPSHUFLW, VPXOR.
 *
 * Output: one CSV line per instruction form:
 *   mnemonic,operands,extension,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -msse4.1 -msse4.2 -mssse3 -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_avx2_gaps.c -lm \
 *     -o ../../../../build/bin/zen_probe_avx2_gaps
 */

/* ========================================================================
 * YMM MACRO TEMPLATES
 * ======================================================================== */

#define YMM_LAT_BINARY(func_name, intrinsic, init_val)                         \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i accumulator = init_val;                                            \
    __m256i source_operand = init_val;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define YMM_TP_BINARY(func_name, intrinsic, init_fn, init_arg)                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m256i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m256i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m256i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m256i source_operand = init_fn(init_arg);                                \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, source_operand);                              \
        acc_1 = intrinsic(acc_1, source_operand);                              \
        acc_2 = intrinsic(acc_2, source_operand);                              \
        acc_3 = intrinsic(acc_3, source_operand);                              \
        acc_4 = intrinsic(acc_4, source_operand);                              \
        acc_5 = intrinsic(acc_5, source_operand);                              \
        acc_6 = intrinsic(acc_6, source_operand);                              \
        acc_7 = intrinsic(acc_7, source_operand);                              \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) +                 \
                              _mm256_extract_epi32(acc_4, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* ========================================================================
 * VPAND / VPANDN / VPOR / VPXOR -- ymm logical
 * ======================================================================== */

YMM_LAT_BINARY(avx2_vpand_ymm_lat, _mm256_and_si256, _mm256_set1_epi32(0x55AA55AA))
YMM_TP_BINARY(avx2_vpand_ymm_tp, _mm256_and_si256, _mm256_set1_epi32, 1)

YMM_LAT_BINARY(avx2_vpandn_ymm_lat, _mm256_andnot_si256, _mm256_set1_epi32(0x55AA55AA))
YMM_TP_BINARY(avx2_vpandn_ymm_tp, _mm256_andnot_si256, _mm256_set1_epi32, 1)

YMM_LAT_BINARY(avx2_vpor_ymm_lat, _mm256_or_si256, _mm256_set1_epi32(0x55AA55AA))
YMM_TP_BINARY(avx2_vpor_ymm_tp, _mm256_or_si256, _mm256_set1_epi32, 1)

YMM_LAT_BINARY(avx2_vpxor_ymm_lat, _mm256_xor_si256, _mm256_set1_epi32(0x55AA55AA))
YMM_TP_BINARY(avx2_vpxor_ymm_tp, _mm256_xor_si256, _mm256_set1_epi32, 1)

/* ========================================================================
 * VPHADDD / VPHADDW / VPHSUBD / VPHSUBW -- ymm horizontal add/sub
 * ======================================================================== */

YMM_LAT_BINARY(avx2_vphaddd_ymm_lat, _mm256_hadd_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(avx2_vphaddd_ymm_tp, _mm256_hadd_epi32, _mm256_set1_epi32, 1)

YMM_LAT_BINARY(avx2_vphaddw_ymm_lat, _mm256_hadd_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(avx2_vphaddw_ymm_tp, _mm256_hadd_epi16, _mm256_set1_epi16, 1)

YMM_LAT_BINARY(avx2_vphsubd_ymm_lat, _mm256_hsub_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(avx2_vphsubd_ymm_tp, _mm256_hsub_epi32, _mm256_set1_epi32, 1)

YMM_LAT_BINARY(avx2_vphsubw_ymm_lat, _mm256_hsub_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(avx2_vphsubw_ymm_tp, _mm256_hsub_epi16, _mm256_set1_epi16, 1)

/* ========================================================================
 * VPBLENDD ymm -- immediate dword blend
 * ======================================================================== */

static double avx2_vpblendd_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi32(1);
    __m256i source_operand = _mm256_set1_epi32(2);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_blend_epi32(accumulator, source_operand, 0x55);
        accumulator = _mm256_blend_epi32(accumulator, source_operand, 0x55);
        accumulator = _mm256_blend_epi32(accumulator, source_operand, 0x55);
        accumulator = _mm256_blend_epi32(accumulator, source_operand, 0x55);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double avx2_vpblendd_ymm_tp(uint64_t iteration_count)
{
    __m256i acc_0 = _mm256_set1_epi32(1), acc_1 = _mm256_set1_epi32(2);
    __m256i acc_2 = _mm256_set1_epi32(3), acc_3 = _mm256_set1_epi32(4);
    __m256i acc_4 = _mm256_set1_epi32(5), acc_5 = _mm256_set1_epi32(6);
    __m256i acc_6 = _mm256_set1_epi32(7), acc_7 = _mm256_set1_epi32(8);
    __m256i source_operand = _mm256_set1_epi32(0xFF);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm256_blend_epi32(acc_0, source_operand, 0x55);
        acc_1 = _mm256_blend_epi32(acc_1, source_operand, 0x55);
        acc_2 = _mm256_blend_epi32(acc_2, source_operand, 0x55);
        acc_3 = _mm256_blend_epi32(acc_3, source_operand, 0x55);
        acc_4 = _mm256_blend_epi32(acc_4, source_operand, 0x55);
        acc_5 = _mm256_blend_epi32(acc_5, source_operand, 0x55);
        acc_6 = _mm256_blend_epi32(acc_6, source_operand, 0x55);
        acc_7 = _mm256_blend_epi32(acc_7, source_operand, 0x55);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * VPALIGNR ymm -- byte align
 * ======================================================================== */

static double avx2_vpalignr_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi32(0x12345678);
    __m256i source_operand = _mm256_set1_epi32(0xABCDEF01);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_alignr_epi8(accumulator, source_operand, 4);
        accumulator = _mm256_alignr_epi8(accumulator, source_operand, 4);
        accumulator = _mm256_alignr_epi8(accumulator, source_operand, 4);
        accumulator = _mm256_alignr_epi8(accumulator, source_operand, 4);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double avx2_vpalignr_ymm_tp(uint64_t iteration_count)
{
    __m256i acc_0 = _mm256_set1_epi32(1), acc_1 = _mm256_set1_epi32(2);
    __m256i acc_2 = _mm256_set1_epi32(3), acc_3 = _mm256_set1_epi32(4);
    __m256i acc_4 = _mm256_set1_epi32(5), acc_5 = _mm256_set1_epi32(6);
    __m256i acc_6 = _mm256_set1_epi32(7), acc_7 = _mm256_set1_epi32(8);
    __m256i source_operand = _mm256_set1_epi32(0xFF);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm256_alignr_epi8(acc_0, source_operand, 4);
        acc_1 = _mm256_alignr_epi8(acc_1, source_operand, 4);
        acc_2 = _mm256_alignr_epi8(acc_2, source_operand, 4);
        acc_3 = _mm256_alignr_epi8(acc_3, source_operand, 4);
        acc_4 = _mm256_alignr_epi8(acc_4, source_operand, 4);
        acc_5 = _mm256_alignr_epi8(acc_5, source_operand, 4);
        acc_6 = _mm256_alignr_epi8(acc_6, source_operand, 4);
        acc_7 = _mm256_alignr_epi8(acc_7, source_operand, 4);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * VPSHUFHW / VPSHUFLW ymm -- shuffle high/low words
 * ======================================================================== */

static double avx2_vpshufhw_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi32(0x12345678);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_shufflehi_epi16(accumulator, 0x1B);
        accumulator = _mm256_shufflehi_epi16(accumulator, 0x1B);
        accumulator = _mm256_shufflehi_epi16(accumulator, 0x1B);
        accumulator = _mm256_shufflehi_epi16(accumulator, 0x1B);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double avx2_vpshufhw_ymm_tp(uint64_t iteration_count)
{
    __m256i acc_0 = _mm256_set1_epi32(1), acc_1 = _mm256_set1_epi32(2);
    __m256i acc_2 = _mm256_set1_epi32(3), acc_3 = _mm256_set1_epi32(4);
    __m256i acc_4 = _mm256_set1_epi32(5), acc_5 = _mm256_set1_epi32(6);
    __m256i acc_6 = _mm256_set1_epi32(7), acc_7 = _mm256_set1_epi32(8);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm256_shufflehi_epi16(acc_0, 0x1B);
        acc_1 = _mm256_shufflehi_epi16(acc_1, 0x1B);
        acc_2 = _mm256_shufflehi_epi16(acc_2, 0x1B);
        acc_3 = _mm256_shufflehi_epi16(acc_3, 0x1B);
        acc_4 = _mm256_shufflehi_epi16(acc_4, 0x1B);
        acc_5 = _mm256_shufflehi_epi16(acc_5, 0x1B);
        acc_6 = _mm256_shufflehi_epi16(acc_6, 0x1B);
        acc_7 = _mm256_shufflehi_epi16(acc_7, 0x1B);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double avx2_vpshuflw_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi32(0x12345678);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_shufflelo_epi16(accumulator, 0x1B);
        accumulator = _mm256_shufflelo_epi16(accumulator, 0x1B);
        accumulator = _mm256_shufflelo_epi16(accumulator, 0x1B);
        accumulator = _mm256_shufflelo_epi16(accumulator, 0x1B);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double avx2_vpshuflw_ymm_tp(uint64_t iteration_count)
{
    __m256i acc_0 = _mm256_set1_epi32(1), acc_1 = _mm256_set1_epi32(2);
    __m256i acc_2 = _mm256_set1_epi32(3), acc_3 = _mm256_set1_epi32(4);
    __m256i acc_4 = _mm256_set1_epi32(5), acc_5 = _mm256_set1_epi32(6);
    __m256i acc_6 = _mm256_set1_epi32(7), acc_7 = _mm256_set1_epi32(8);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm256_shufflelo_epi16(acc_0, 0x1B);
        acc_1 = _mm256_shufflelo_epi16(acc_1, 0x1B);
        acc_2 = _mm256_shufflelo_epi16(acc_2, 0x1B);
        acc_3 = _mm256_shufflelo_epi16(acc_3, 0x1B);
        acc_4 = _mm256_shufflelo_epi16(acc_4, 0x1B);
        acc_5 = _mm256_shufflelo_epi16(acc_5, 0x1B);
        acc_6 = _mm256_shufflelo_epi16(acc_6, 0x1B);
        acc_7 = _mm256_shufflelo_epi16(acc_7, 0x1B);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * VEXTRACTI128 / VINSERTI128 -- cross-lane insert/extract 128-bit
 * ======================================================================== */

static double avx2_vextracti128_lat(uint64_t iteration_count)
{
    __m256i ymm_source = _mm256_set1_epi32(0x12345678);
    __m128i xmm_result;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        xmm_result = _mm256_extracti128_si256(ymm_source, 1);
        ymm_source = _mm256_set_m128i(xmm_result, xmm_result);
        xmm_result = _mm256_extracti128_si256(ymm_source, 0);
        ymm_source = _mm256_set_m128i(xmm_result, xmm_result);
        xmm_result = _mm256_extracti128_si256(ymm_source, 1);
        ymm_source = _mm256_set_m128i(xmm_result, xmm_result);
        xmm_result = _mm256_extracti128_si256(ymm_source, 0);
        ymm_source = _mm256_set_m128i(xmm_result, xmm_result);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(xmm_result, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double avx2_vextracti128_tp(uint64_t iteration_count)
{
    __m256i ymm_source = _mm256_set1_epi32(0x12345678);
    __m128i result_0, result_1, result_2, result_3;
    __m128i result_4, result_5, result_6, result_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_0 = _mm256_extracti128_si256(ymm_source, 0);
        result_1 = _mm256_extracti128_si256(ymm_source, 1);
        result_2 = _mm256_extracti128_si256(ymm_source, 0);
        result_3 = _mm256_extracti128_si256(ymm_source, 1);
        result_4 = _mm256_extracti128_si256(ymm_source, 0);
        result_5 = _mm256_extracti128_si256(ymm_source, 1);
        result_6 = _mm256_extracti128_si256(ymm_source, 0);
        result_7 = _mm256_extracti128_si256(ymm_source, 1);
        __asm__ volatile("" : "+x"(result_0), "+x"(result_1),
                              "+x"(result_2), "+x"(result_3));
        __asm__ volatile("" : "+x"(result_4), "+x"(result_5),
                              "+x"(result_6), "+x"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(result_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double avx2_vinserti128_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi32(1);
    __m128i insert_source = _mm_set1_epi32(0xABCD);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_inserti128_si256(accumulator, insert_source, 0);
        accumulator = _mm256_inserti128_si256(accumulator, insert_source, 1);
        accumulator = _mm256_inserti128_si256(accumulator, insert_source, 0);
        accumulator = _mm256_inserti128_si256(accumulator, insert_source, 1);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double avx2_vinserti128_tp(uint64_t iteration_count)
{
    __m256i acc_0 = _mm256_set1_epi32(1), acc_1 = _mm256_set1_epi32(2);
    __m256i acc_2 = _mm256_set1_epi32(3), acc_3 = _mm256_set1_epi32(4);
    __m256i acc_4 = _mm256_set1_epi32(5), acc_5 = _mm256_set1_epi32(6);
    __m256i acc_6 = _mm256_set1_epi32(7), acc_7 = _mm256_set1_epi32(8);
    __m128i insert_source = _mm_set1_epi32(0xABCD);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm256_inserti128_si256(acc_0, insert_source, 0);
        acc_1 = _mm256_inserti128_si256(acc_1, insert_source, 1);
        acc_2 = _mm256_inserti128_si256(acc_2, insert_source, 0);
        acc_3 = _mm256_inserti128_si256(acc_3, insert_source, 1);
        acc_4 = _mm256_inserti128_si256(acc_4, insert_source, 0);
        acc_5 = _mm256_inserti128_si256(acc_5, insert_source, 1);
        acc_6 = _mm256_inserti128_si256(acc_6, insert_source, 0);
        acc_7 = _mm256_inserti128_si256(acc_7, insert_source, 1);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * VPMOVMSKB ymm -- move byte mask to GPR
 * ======================================================================== */

static double avx2_vpmovmskb_ymm_tp(uint64_t iteration_count)
{
    __m256i ymm_source = _mm256_set1_epi32(0x80408020);
    int result_0, result_1, result_2, result_3;
    int result_4, result_5, result_6, result_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_0 = _mm256_movemask_epi8(ymm_source);
        result_1 = _mm256_movemask_epi8(ymm_source);
        result_2 = _mm256_movemask_epi8(ymm_source);
        result_3 = _mm256_movemask_epi8(ymm_source);
        result_4 = _mm256_movemask_epi8(ymm_source);
        result_5 = _mm256_movemask_epi8(ymm_source);
        result_6 = _mm256_movemask_epi8(ymm_source);
        result_7 = _mm256_movemask_epi8(ymm_source);
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = result_0 ^ result_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * VPMASKMOVD / VPMASKMOVQ -- masked load/store
 * ======================================================================== */

static double avx2_vpmaskmovd_load_tp(uint64_t iteration_count)
{
    int32_t __attribute__((aligned(32))) memory_buffer[8] = {1,2,3,4,5,6,7,8};
    __m256i load_mask = _mm256_set_epi32(-1, 0, -1, 0, -1, 0, -1, 0);
    __m256i result_0, result_1, result_2, result_3;
    __m256i result_4, result_5, result_6, result_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_0 = _mm256_maskload_epi32(memory_buffer, load_mask);
        result_1 = _mm256_maskload_epi32(memory_buffer, load_mask);
        result_2 = _mm256_maskload_epi32(memory_buffer, load_mask);
        result_3 = _mm256_maskload_epi32(memory_buffer, load_mask);
        result_4 = _mm256_maskload_epi32(memory_buffer, load_mask);
        result_5 = _mm256_maskload_epi32(memory_buffer, load_mask);
        result_6 = _mm256_maskload_epi32(memory_buffer, load_mask);
        result_7 = _mm256_maskload_epi32(memory_buffer, load_mask);
        __asm__ volatile("" : "+x"(result_0), "+x"(result_1),
                              "+x"(result_2), "+x"(result_3));
        __asm__ volatile("" : "+x"(result_4), "+x"(result_5),
                              "+x"(result_6), "+x"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(result_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double avx2_vpmaskmovq_load_tp(uint64_t iteration_count)
{
    int64_t __attribute__((aligned(32))) memory_buffer[4] = {1,2,3,4};
    __m256i load_mask = _mm256_set_epi64x(-1LL, 0LL, -1LL, 0LL);
    __m256i result_0, result_1, result_2, result_3;
    __m256i result_4, result_5, result_6, result_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_0 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        result_1 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        result_2 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        result_3 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        result_4 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        result_5 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        result_6 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        result_7 = _mm256_maskload_epi64((long long const *)memory_buffer, load_mask);
        __asm__ volatile("" : "+x"(result_0), "+x"(result_1),
                              "+x"(result_2), "+x"(result_3));
        __asm__ volatile("" : "+x"(result_4), "+x"(result_5),
                              "+x"(result_6), "+x"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(result_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}


/* ========================================================================
 * PROBE TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    const char *isa_extension;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} Avx2GapProbeEntry;

static const Avx2GapProbeEntry avx2_gap_probe_table[] = {
    /* Logic */
    {"VPAND",       "ymm,ymm,ymm",   "AVX2","avx2_logic",  avx2_vpand_ymm_lat,     avx2_vpand_ymm_tp},
    {"VPANDN",      "ymm,ymm,ymm",   "AVX2","avx2_logic",  avx2_vpandn_ymm_lat,    avx2_vpandn_ymm_tp},
    {"VPOR",        "ymm,ymm,ymm",   "AVX2","avx2_logic",  avx2_vpor_ymm_lat,      avx2_vpor_ymm_tp},
    {"VPXOR",       "ymm,ymm,ymm",   "AVX2","avx2_logic",  avx2_vpxor_ymm_lat,     avx2_vpxor_ymm_tp},

    /* Horizontal add/sub */
    {"VPHADDD",     "ymm,ymm,ymm",   "AVX2","avx2_hadd",   avx2_vphaddd_ymm_lat,   avx2_vphaddd_ymm_tp},
    {"VPHADDW",     "ymm,ymm,ymm",   "AVX2","avx2_hadd",   avx2_vphaddw_ymm_lat,   avx2_vphaddw_ymm_tp},
    {"VPHSUBD",     "ymm,ymm,ymm",   "AVX2","avx2_hsub",   avx2_vphsubd_ymm_lat,   avx2_vphsubd_ymm_tp},
    {"VPHSUBW",     "ymm,ymm,ymm",   "AVX2","avx2_hsub",   avx2_vphsubw_ymm_lat,   avx2_vphsubw_ymm_tp},

    /* Blend */
    {"VPBLENDD",    "ymm,ymm,ymm,$0x55","AVX2","avx2_blend",avx2_vpblendd_ymm_lat,  avx2_vpblendd_ymm_tp},

    /* Align */
    {"VPALIGNR",    "ymm,ymm,ymm,$4",  "AVX2","avx2_shuf",  avx2_vpalignr_ymm_lat,  avx2_vpalignr_ymm_tp},

    /* Shuffle */
    {"VPSHUFHW",    "ymm,ymm,$0x1B",   "AVX2","avx2_shuf",  avx2_vpshufhw_ymm_lat,  avx2_vpshufhw_ymm_tp},
    {"VPSHUFLW",    "ymm,ymm,$0x1B",   "AVX2","avx2_shuf",  avx2_vpshuflw_ymm_lat,  avx2_vpshuflw_ymm_tp},

    /* Cross-lane insert / extract */
    {"VEXTRACTI128","xmm,ymm,$n",      "AVX2","avx2_lane",  avx2_vextracti128_lat,  avx2_vextracti128_tp},
    {"VINSERTI128", "ymm,ymm,xmm,$n",  "AVX2","avx2_lane",  avx2_vinserti128_lat,   avx2_vinserti128_tp},

    /* Movemask */
    {"VPMOVMSKB",   "r32,ymm",         "AVX2","avx2_mask",  NULL,                   avx2_vpmovmskb_ymm_tp},

    /* Masked load */
    {"VPMASKMOVD",  "ymm,[m256],ymm",  "AVX2","avx2_mask",  NULL,                   avx2_vpmaskmovd_load_tp},
    {"VPMASKMOVQ",  "ymm,[m256],ymm",  "AVX2","avx2_mask",  NULL,                   avx2_vpmaskmovq_load_tp},

    {NULL, NULL, NULL, NULL, NULL, NULL}
};


/* ========================================================================
 * MAIN
 * ======================================================================== */

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 100000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    int csv_mode = probe_config.csv;

    if (!csv_mode) {
        sm_zen_print_header("avx2_gaps", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n",
               "missing AVX2 (ymm) gap-fill");
    }

    /* Print CSV header */
    printf("mnemonic,operands,extension,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; avx2_gap_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const Avx2GapProbeEntry *current_entry = &avx2_gap_probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; avx2_gap_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const Avx2GapProbeEntry *current_entry = &avx2_gap_probe_table[entry_idx];

        double measured_latency = -1.0;
        double measured_throughput = -1.0;

        if (current_entry->measure_latency)
            measured_latency = current_entry->measure_latency(probe_config.iterations);
        if (current_entry->measure_throughput)
            measured_throughput = current_entry->measure_throughput(probe_config.iterations);

        printf("%s,%s,%s,%.4f,%.4f,%s\n",
               current_entry->mnemonic,
               current_entry->operand_description,
               current_entry->isa_extension,
               measured_latency,
               measured_throughput,
               current_entry->instruction_category);
    }

    return 0;
}
