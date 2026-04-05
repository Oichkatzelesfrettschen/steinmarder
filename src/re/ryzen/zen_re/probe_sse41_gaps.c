#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <smmintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_sse41_gaps.c -- Fill the 17 missing SSE4.1 mnemonics from the
 * Zen 4 coverage gap report.
 *
 * Covers: BLENDPD, BLENDPS, BLENDVPD, BLENDVPS, EXTRACTPS, INSERTPS,
 * PBLENDVB, PBLENDW, PINSRB, PINSRD, PINSRQ, PINSRW (xmm form),
 * PTEST, ROUNDPD, ROUNDPS, ROUNDSD, ROUNDSS.
 *
 * Output: one CSV line per instruction form:
 *   mnemonic,operands,extension,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -msse4.1 -msse4.2 -mssse3 -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_sse41_gaps.c -lm \
 *     -o ../../../../build/bin/zen_probe_sse41_gaps
 */

/* ========================================================================
 * BLENDPD / BLENDPS -- immediate blend
 * ======================================================================== */

static double sse41_blendpd_lat(uint64_t iteration_count)
{
    __m128d accumulator = _mm_set1_pd(1.0);
    __m128d source_operand = _mm_set1_pd(2.0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_blend_pd(accumulator, source_operand, 0x1);
        accumulator = _mm_blend_pd(accumulator, source_operand, 0x1);
        accumulator = _mm_blend_pd(accumulator, source_operand, 0x1);
        accumulator = _mm_blend_pd(accumulator, source_operand, 0x1);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128d sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse41_blendpd_tp(uint64_t iteration_count)
{
    __m128d acc_0 = _mm_set1_pd(1.0), acc_1 = _mm_set1_pd(2.0);
    __m128d acc_2 = _mm_set1_pd(3.0), acc_3 = _mm_set1_pd(4.0);
    __m128d acc_4 = _mm_set1_pd(5.0), acc_5 = _mm_set1_pd(6.0);
    __m128d acc_6 = _mm_set1_pd(7.0), acc_7 = _mm_set1_pd(8.0);
    __m128d source_operand = _mm_set1_pd(0.5);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_blend_pd(acc_0, source_operand, 0x1);
        acc_1 = _mm_blend_pd(acc_1, source_operand, 0x1);
        acc_2 = _mm_blend_pd(acc_2, source_operand, 0x1);
        acc_3 = _mm_blend_pd(acc_3, source_operand, 0x1);
        acc_4 = _mm_blend_pd(acc_4, source_operand, 0x1);
        acc_5 = _mm_blend_pd(acc_5, source_operand, 0x1);
        acc_6 = _mm_blend_pd(acc_6, source_operand, 0x1);
        acc_7 = _mm_blend_pd(acc_7, source_operand, 0x1);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128d sink = acc_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double sse41_blendps_lat(uint64_t iteration_count)
{
    __m128 accumulator = _mm_set1_ps(1.0f);
    __m128 source_operand = _mm_set1_ps(2.0f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_blend_ps(accumulator, source_operand, 0x5);
        accumulator = _mm_blend_ps(accumulator, source_operand, 0x5);
        accumulator = _mm_blend_ps(accumulator, source_operand, 0x5);
        accumulator = _mm_blend_ps(accumulator, source_operand, 0x5);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse41_blendps_tp(uint64_t iteration_count)
{
    __m128 acc_0 = _mm_set1_ps(1.0f), acc_1 = _mm_set1_ps(2.0f);
    __m128 acc_2 = _mm_set1_ps(3.0f), acc_3 = _mm_set1_ps(4.0f);
    __m128 acc_4 = _mm_set1_ps(5.0f), acc_5 = _mm_set1_ps(6.0f);
    __m128 acc_6 = _mm_set1_ps(7.0f), acc_7 = _mm_set1_ps(8.0f);
    __m128 source_operand = _mm_set1_ps(0.5f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_blend_ps(acc_0, source_operand, 0x5);
        acc_1 = _mm_blend_ps(acc_1, source_operand, 0x5);
        acc_2 = _mm_blend_ps(acc_2, source_operand, 0x5);
        acc_3 = _mm_blend_ps(acc_3, source_operand, 0x5);
        acc_4 = _mm_blend_ps(acc_4, source_operand, 0x5);
        acc_5 = _mm_blend_ps(acc_5, source_operand, 0x5);
        acc_6 = _mm_blend_ps(acc_6, source_operand, 0x5);
        acc_7 = _mm_blend_ps(acc_7, source_operand, 0x5);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = acc_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * BLENDVPD / BLENDVPS / PBLENDVB -- variable blend
 * ======================================================================== */

#define SSE41_BLENDV_LATTHRU(name_lat, name_tp, type, intrinsic, init, mask_init) \
static double name_lat(uint64_t iteration_count) {                             \
    type accumulator = init; type source_operand = init; type blend_mask = mask_init; \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand, blend_mask);       \
        accumulator = intrinsic(accumulator, source_operand, blend_mask);       \
        accumulator = intrinsic(accumulator, source_operand, blend_mask);       \
        accumulator = intrinsic(accumulator, source_operand, blend_mask);       \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile type sink = accumulator; (void)sink;                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}                                                                              \
static double name_tp(uint64_t iteration_count) {                              \
    type acc_0=init,acc_1=init,acc_2=init,acc_3=init;                         \
    type acc_4=init,acc_5=init,acc_6=init,acc_7=init;                         \
    type source_operand = init; type blend_mask = mask_init;                   \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, source_operand, blend_mask);                  \
        acc_1 = intrinsic(acc_1, source_operand, blend_mask);                  \
        acc_2 = intrinsic(acc_2, source_operand, blend_mask);                  \
        acc_3 = intrinsic(acc_3, source_operand, blend_mask);                  \
        acc_4 = intrinsic(acc_4, source_operand, blend_mask);                  \
        acc_5 = intrinsic(acc_5, source_operand, blend_mask);                  \
        acc_6 = intrinsic(acc_6, source_operand, blend_mask);                  \
        acc_7 = intrinsic(acc_7, source_operand, blend_mask);                  \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile type sink = acc_0; (void)sink;                                    \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

SSE41_BLENDV_LATTHRU(sse41_blendvpd_lat, sse41_blendvpd_tp, __m128d,
    _mm_blendv_pd, _mm_set1_pd(1.0),
    _mm_castsi128_pd(_mm_set_epi64x(-1LL, 0LL)))

SSE41_BLENDV_LATTHRU(sse41_blendvps_lat, sse41_blendvps_tp, __m128,
    _mm_blendv_ps, _mm_set1_ps(1.0f),
    _mm_castsi128_ps(_mm_set_epi32(-1, 0, -1, 0)))

/* PBLENDVB -- integer */
static double sse41_pblendvb_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi32(0x01020304);
    __m128i source_operand = _mm_set1_epi32(0x05060708);
    __m128i blend_mask = _mm_set_epi8(-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_blendv_epi8(accumulator, source_operand, blend_mask);
        accumulator = _mm_blendv_epi8(accumulator, source_operand, blend_mask);
        accumulator = _mm_blendv_epi8(accumulator, source_operand, blend_mask);
        accumulator = _mm_blendv_epi8(accumulator, source_operand, blend_mask);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse41_pblendvb_tp(uint64_t iteration_count)
{
    __m128i acc_0=_mm_set1_epi32(1),acc_1=_mm_set1_epi32(2),acc_2=_mm_set1_epi32(3),acc_3=_mm_set1_epi32(4);
    __m128i acc_4=_mm_set1_epi32(5),acc_5=_mm_set1_epi32(6),acc_6=_mm_set1_epi32(7),acc_7=_mm_set1_epi32(8);
    __m128i source_operand = _mm_set1_epi32(0xFF);
    __m128i blend_mask = _mm_set_epi8(-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_blendv_epi8(acc_0, source_operand, blend_mask);
        acc_1 = _mm_blendv_epi8(acc_1, source_operand, blend_mask);
        acc_2 = _mm_blendv_epi8(acc_2, source_operand, blend_mask);
        acc_3 = _mm_blendv_epi8(acc_3, source_operand, blend_mask);
        acc_4 = _mm_blendv_epi8(acc_4, source_operand, blend_mask);
        acc_5 = _mm_blendv_epi8(acc_5, source_operand, blend_mask);
        acc_6 = _mm_blendv_epi8(acc_6, source_operand, blend_mask);
        acc_7 = _mm_blendv_epi8(acc_7, source_operand, blend_mask);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(acc_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * EXTRACTPS / INSERTPS
 * ======================================================================== */

static double sse41_extractps_tp(uint64_t iteration_count)
{
    __m128 xmm_source = _mm_set1_ps(1.5f);
    int r0,r1,r2,r3,r4,r5,r6,r7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        r0 = _mm_extract_ps(xmm_source, 0); r1 = _mm_extract_ps(xmm_source, 1);
        r2 = _mm_extract_ps(xmm_source, 2); r3 = _mm_extract_ps(xmm_source, 3);
        r4 = _mm_extract_ps(xmm_source, 0); r5 = _mm_extract_ps(xmm_source, 1);
        r6 = _mm_extract_ps(xmm_source, 2); r7 = _mm_extract_ps(xmm_source, 3);
        __asm__ volatile("" : "+r"(r0),"+r"(r1),"+r"(r2),"+r"(r3));
        __asm__ volatile("" : "+r"(r4),"+r"(r5),"+r"(r6),"+r"(r7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = r0 ^ r4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double sse41_insertps_lat(uint64_t iteration_count)
{
    __m128 accumulator = _mm_set1_ps(1.0f);
    __m128 source_operand = _mm_set1_ps(2.0f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_insert_ps(accumulator, source_operand, 0x10);
        accumulator = _mm_insert_ps(accumulator, source_operand, 0x10);
        accumulator = _mm_insert_ps(accumulator, source_operand, 0x10);
        accumulator = _mm_insert_ps(accumulator, source_operand, 0x10);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse41_insertps_tp(uint64_t iteration_count)
{
    __m128 acc_0=_mm_set1_ps(1.f),acc_1=_mm_set1_ps(2.f),acc_2=_mm_set1_ps(3.f),acc_3=_mm_set1_ps(4.f);
    __m128 acc_4=_mm_set1_ps(5.f),acc_5=_mm_set1_ps(6.f),acc_6=_mm_set1_ps(7.f),acc_7=_mm_set1_ps(8.f);
    __m128 source_operand = _mm_set1_ps(0.5f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_insert_ps(acc_0, source_operand, 0x10);
        acc_1 = _mm_insert_ps(acc_1, source_operand, 0x10);
        acc_2 = _mm_insert_ps(acc_2, source_operand, 0x10);
        acc_3 = _mm_insert_ps(acc_3, source_operand, 0x10);
        acc_4 = _mm_insert_ps(acc_4, source_operand, 0x10);
        acc_5 = _mm_insert_ps(acc_5, source_operand, 0x10);
        acc_6 = _mm_insert_ps(acc_6, source_operand, 0x10);
        acc_7 = _mm_insert_ps(acc_7, source_operand, 0x10);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = acc_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * PBLENDW, PINSRB/D/Q/W, PTEST, ROUNDxx -- inline asm binary patterns
 * ======================================================================== */

#define SSE41_XMM_INSERT_LAT_TP(name_lat, name_tp, intrinsic, type_val, imm_seq)  \
static double name_lat(uint64_t iteration_count) {                             \
    __m128i accumulator = _mm_set1_epi32(1);                                   \
    type_val insert_value = 0x42;                                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, insert_value, 0);                 \
        accumulator = intrinsic(accumulator, insert_value, 1);                 \
        accumulator = intrinsic(accumulator, insert_value, 0);                 \
        accumulator = intrinsic(accumulator, insert_value, 1);                 \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}                                                                              \
static double name_tp(uint64_t iteration_count) {                              \
    __m128i acc_0=_mm_set1_epi32(1),acc_1=_mm_set1_epi32(2);                  \
    __m128i acc_2=_mm_set1_epi32(3),acc_3=_mm_set1_epi32(4);                  \
    __m128i acc_4=_mm_set1_epi32(5),acc_5=_mm_set1_epi32(6);                  \
    __m128i acc_6=_mm_set1_epi32(7),acc_7=_mm_set1_epi32(8);                  \
    type_val insert_value = 0x42;                                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, insert_value, 0);                             \
        acc_1 = intrinsic(acc_1, insert_value, 1);                             \
        acc_2 = intrinsic(acc_2, insert_value, 0);                             \
        acc_3 = intrinsic(acc_3, insert_value, 1);                             \
        acc_4 = intrinsic(acc_4, insert_value, 0);                             \
        acc_5 = intrinsic(acc_5, insert_value, 1);                             \
        acc_6 = intrinsic(acc_6, insert_value, 0);                             \
        acc_7 = intrinsic(acc_7, insert_value, 1);                             \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(acc_0, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

SSE41_XMM_INSERT_LAT_TP(sse41_pinsrb_lat, sse41_pinsrb_tp, _mm_insert_epi8, int, 0)
SSE41_XMM_INSERT_LAT_TP(sse41_pinsrd_lat, sse41_pinsrd_tp, _mm_insert_epi32, int, 0)
SSE41_XMM_INSERT_LAT_TP(sse41_pinsrq_lat, sse41_pinsrq_tp, _mm_insert_epi64, long long, 0)
SSE41_XMM_INSERT_LAT_TP(sse41_pinsrw_lat, sse41_pinsrw_tp, _mm_insert_epi16, int, 0)

/* PBLENDW */
static double sse41_pblendw_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi32(1);
    __m128i source_operand = _mm_set1_epi32(2);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_blend_epi16(accumulator, source_operand, 0x55);
        accumulator = _mm_blend_epi16(accumulator, source_operand, 0x55);
        accumulator = _mm_blend_epi16(accumulator, source_operand, 0x55);
        accumulator = _mm_blend_epi16(accumulator, source_operand, 0x55);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse41_pblendw_tp(uint64_t iteration_count)
{
    __m128i acc_0=_mm_set1_epi32(1),acc_1=_mm_set1_epi32(2),acc_2=_mm_set1_epi32(3),acc_3=_mm_set1_epi32(4);
    __m128i acc_4=_mm_set1_epi32(5),acc_5=_mm_set1_epi32(6),acc_6=_mm_set1_epi32(7),acc_7=_mm_set1_epi32(8);
    __m128i source_operand = _mm_set1_epi32(0xFF);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_blend_epi16(acc_0, source_operand, 0x55);
        acc_1 = _mm_blend_epi16(acc_1, source_operand, 0x55);
        acc_2 = _mm_blend_epi16(acc_2, source_operand, 0x55);
        acc_3 = _mm_blend_epi16(acc_3, source_operand, 0x55);
        acc_4 = _mm_blend_epi16(acc_4, source_operand, 0x55);
        acc_5 = _mm_blend_epi16(acc_5, source_operand, 0x55);
        acc_6 = _mm_blend_epi16(acc_6, source_operand, 0x55);
        acc_7 = _mm_blend_epi16(acc_7, source_operand, 0x55);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(acc_0, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PTEST -- throughput only */
static double sse41_ptest_tp(uint64_t iteration_count)
{
    __m128i operand_a = _mm_set1_epi32(0xAAAAAAAA);
    __m128i operand_b = _mm_set1_epi32(0x55555555);
    int r0,r1,r2,r3,r4,r5,r6,r7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        r0 = _mm_testz_si128(operand_a, operand_b);
        r1 = _mm_testc_si128(operand_a, operand_b);
        r2 = _mm_testz_si128(operand_a, operand_b);
        r3 = _mm_testc_si128(operand_a, operand_b);
        r4 = _mm_testz_si128(operand_a, operand_b);
        r5 = _mm_testc_si128(operand_a, operand_b);
        r6 = _mm_testz_si128(operand_a, operand_b);
        r7 = _mm_testc_si128(operand_a, operand_b);
        __asm__ volatile("" : "+r"(r0),"+r"(r1),"+r"(r2),"+r"(r3));
        __asm__ volatile("" : "+r"(r4),"+r"(r5),"+r"(r6),"+r"(r7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = r0 ^ r4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * ROUNDPD / ROUNDPS / ROUNDSD / ROUNDSS
 * ======================================================================== */

#define SSE41_ROUND_PACKED(name_lat, name_tp, type, intrinsic, init_val)       \
static double name_lat(uint64_t iteration_count) {                             \
    type accumulator = init_val;                                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, _MM_FROUND_TO_NEAREST_INT);       \
        accumulator = intrinsic(accumulator, _MM_FROUND_TO_NEAREST_INT);       \
        accumulator = intrinsic(accumulator, _MM_FROUND_TO_NEAREST_INT);       \
        accumulator = intrinsic(accumulator, _MM_FROUND_TO_NEAREST_INT);       \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile type sink = accumulator; (void)sink;                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}                                                                              \
static double name_tp(uint64_t iteration_count) {                              \
    type a0=init_val,a1=init_val,a2=init_val,a3=init_val;                     \
    type a4=init_val,a5=init_val,a6=init_val,a7=init_val;                     \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        a0 = intrinsic(a0, _MM_FROUND_TO_NEAREST_INT);                        \
        a1 = intrinsic(a1, _MM_FROUND_TO_NEAREST_INT);                        \
        a2 = intrinsic(a2, _MM_FROUND_TO_NEAREST_INT);                        \
        a3 = intrinsic(a3, _MM_FROUND_TO_NEAREST_INT);                        \
        a4 = intrinsic(a4, _MM_FROUND_TO_NEAREST_INT);                        \
        a5 = intrinsic(a5, _MM_FROUND_TO_NEAREST_INT);                        \
        a6 = intrinsic(a6, _MM_FROUND_TO_NEAREST_INT);                        \
        a7 = intrinsic(a7, _MM_FROUND_TO_NEAREST_INT);                        \
        __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),           \
                              "+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7));           \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile type sink = a0; (void)sink;                                       \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

SSE41_ROUND_PACKED(sse41_roundpd_lat, sse41_roundpd_tp, __m128d, _mm_round_pd, _mm_set1_pd(1.7))
SSE41_ROUND_PACKED(sse41_roundps_lat, sse41_roundps_tp, __m128, _mm_round_ps, _mm_set1_ps(1.7f))

/* ROUNDSD/ROUNDSS -- scalar, takes (dst, src, imm) */
#define SSE41_ROUND_SCALAR(name_lat, name_tp, type, intrinsic, init_val)       \
static double name_lat(uint64_t iteration_count) {                             \
    type accumulator = init_val; type source_operand = init_val;               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand, _MM_FROUND_TO_NEAREST_INT); \
        accumulator = intrinsic(accumulator, source_operand, _MM_FROUND_TO_NEAREST_INT); \
        accumulator = intrinsic(accumulator, source_operand, _MM_FROUND_TO_NEAREST_INT); \
        accumulator = intrinsic(accumulator, source_operand, _MM_FROUND_TO_NEAREST_INT); \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile type sink = accumulator; (void)sink;                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}                                                                              \
static double name_tp(uint64_t iteration_count) {                              \
    type a0=init_val,a1=init_val,a2=init_val,a3=init_val;                     \
    type a4=init_val,a5=init_val,a6=init_val,a7=init_val;                     \
    type source_operand = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        a0 = intrinsic(a0, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        a1 = intrinsic(a1, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        a2 = intrinsic(a2, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        a3 = intrinsic(a3, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        a4 = intrinsic(a4, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        a5 = intrinsic(a5, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        a6 = intrinsic(a6, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        a7 = intrinsic(a7, source_operand, _MM_FROUND_TO_NEAREST_INT);        \
        __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),           \
                              "+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7));           \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile type sink = a0; (void)sink;                                       \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

SSE41_ROUND_SCALAR(sse41_roundsd_lat, sse41_roundsd_tp, __m128d, _mm_round_sd, _mm_set1_pd(3.14159))
SSE41_ROUND_SCALAR(sse41_roundss_lat, sse41_roundss_tp, __m128, _mm_round_ss, _mm_set1_ps(3.14159f))

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
} Sse41GapProbeEntry;

static const Sse41GapProbeEntry sse41_gap_probe_table[] = {
    {"BLENDPD",   "xmm,xmm,$0x1",   "SSE4.1","sse41_blend",  sse41_blendpd_lat,    sse41_blendpd_tp},
    {"BLENDPS",   "xmm,xmm,$0x5",   "SSE4.1","sse41_blend",  sse41_blendps_lat,    sse41_blendps_tp},
    {"BLENDVPD",  "xmm,xmm,xmm0",   "SSE4.1","sse41_blendv", sse41_blendvpd_lat,   sse41_blendvpd_tp},
    {"BLENDVPS",  "xmm,xmm,xmm0",   "SSE4.1","sse41_blendv", sse41_blendvps_lat,   sse41_blendvps_tp},
    {"PBLENDVB",  "xmm,xmm,xmm0",   "SSE4.1","sse41_blendv", sse41_pblendvb_lat,   sse41_pblendvb_tp},
    {"EXTRACTPS", "r32,xmm,$n",      "SSE4.1","sse41_insert", NULL,                 sse41_extractps_tp},
    {"INSERTPS",  "xmm,xmm,$0x10",   "SSE4.1","sse41_insert", sse41_insertps_lat,   sse41_insertps_tp},
    {"PBLENDW",   "xmm,xmm,$0x55",   "SSE4.1","sse41_blend",  sse41_pblendw_lat,    sse41_pblendw_tp},
    {"PINSRB",    "xmm,r32,$n",      "SSE4.1","sse41_insert", sse41_pinsrb_lat,     sse41_pinsrb_tp},
    {"PINSRD",    "xmm,r32,$n",      "SSE4.1","sse41_insert", sse41_pinsrd_lat,     sse41_pinsrd_tp},
    {"PINSRQ",    "xmm,r64,$n",      "SSE4.1","sse41_insert", sse41_pinsrq_lat,     sse41_pinsrq_tp},
    {"PINSRW",    "xmm,r32,$n",      "SSE4.1","sse41_insert", sse41_pinsrw_lat,     sse41_pinsrw_tp},
    {"PTEST",     "xmm,xmm",         "SSE4.1","sse41_test",   NULL,                 sse41_ptest_tp},
    {"ROUNDPD",   "xmm,xmm,$0",      "SSE4.1","sse41_round",  sse41_roundpd_lat,    sse41_roundpd_tp},
    {"ROUNDPS",   "xmm,xmm,$0",      "SSE4.1","sse41_round",  sse41_roundps_lat,    sse41_roundps_tp},
    {"ROUNDSD",   "xmm,xmm,$0",      "SSE4.1","sse41_round",  sse41_roundsd_lat,    sse41_roundsd_tp},
    {"ROUNDSS",   "xmm,xmm,$0",      "SSE4.1","sse41_round",  sse41_roundss_lat,    sse41_roundss_tp},
    {NULL, NULL, NULL, NULL, NULL, NULL}
};

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 100000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    int csv_mode = probe_config.csv;
    if (!csv_mode) {
        sm_zen_print_header("sse41_gaps", &probe_config, &detected_features);
        printf("\nMeasuring missing SSE4.1 gap-fill instruction forms...\n\n");
    }
    printf("mnemonic,operands,extension,latency_cycles,throughput_cycles,category\n");

    for (int warmup_idx = 0; sse41_gap_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const Sse41GapProbeEntry *current_entry = &sse41_gap_probe_table[warmup_idx];
        if (current_entry->measure_latency)   current_entry->measure_latency(1000);
        if (current_entry->measure_throughput) current_entry->measure_throughput(1000);
    }

    for (int entry_idx = 0; sse41_gap_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const Sse41GapProbeEntry *current_entry = &sse41_gap_probe_table[entry_idx];
        double measured_latency = -1.0, measured_throughput = -1.0;
        if (current_entry->measure_latency)
            measured_latency = current_entry->measure_latency(probe_config.iterations);
        if (current_entry->measure_throughput)
            measured_throughput = current_entry->measure_throughput(probe_config.iterations);
        printf("%s,%s,%s,%.4f,%.4f,%s\n",
               current_entry->mnemonic, current_entry->operand_description,
               current_entry->isa_extension, measured_latency, measured_throughput,
               current_entry->instruction_category);
    }
    return 0;
}
