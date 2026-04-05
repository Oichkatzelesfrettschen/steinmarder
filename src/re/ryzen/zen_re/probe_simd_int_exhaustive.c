#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_simd_int_exhaustive.c -- Exhaustive SIMD integer instruction measurement.
 *
 * Measures latency (4-deep dependency chain) and throughput (8 independent
 * accumulators) for every integer SIMD instruction form on Zen 4, at both
 * XMM (128-bit) and YMM (256-bit) widths.
 *
 * Output: one CSV line per instruction form:
 *   mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -mbmi2 -fno-omit-frame-pointer -include x86intrin.h \
 *     -I. common.c probe_simd_int_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_simd_int_exhaustive
 */

/* ========================================================================
 * MACRO TEMPLATES -- XMM (128-bit)
 * ======================================================================== */

/*
 * XMM_LAT_BINARY: latency for binary xmm op: acc = op(acc, source).
 *   4-deep dependency chain per iteration.
 */
#define XMM_LAT_BINARY(func_name, intrinsic, init_val)                         \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i accumulator = init_val;                                            \
    __m128i source_operand = init_val;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

/*
 * XMM_TP_BINARY: throughput for binary xmm op with 8 independent streams.
 */
#define XMM_TP_BINARY(func_name, intrinsic, init_fn, init_arg)                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m128i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m128i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m128i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m128i source_operand = init_fn(init_arg);                                \
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
    volatile int sink_value = _mm_extract_epi32(acc_0, 0) +                    \
                              _mm_extract_epi32(acc_4, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* ========================================================================
 * MACRO TEMPLATES -- YMM (256-bit)
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

/*
 * Shift-by-immediate macros: intrinsic takes (vec, int constant).
 * Latency: 4-deep chain  acc = shift(acc, count)
 * Throughput: 8 independent streams
 */
#define XMM_LAT_SHIFT_IMM(func_name, intrinsic, init_val, shift_amount)        \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define XMM_TP_SHIFT_IMM(func_name, intrinsic, init_fn, init_arg, shift_amount) \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m128i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m128i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m128i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, shift_amount);                                \
        acc_1 = intrinsic(acc_1, shift_amount);                                \
        acc_2 = intrinsic(acc_2, shift_amount);                                \
        acc_3 = intrinsic(acc_3, shift_amount);                                \
        acc_4 = intrinsic(acc_4, shift_amount);                                \
        acc_5 = intrinsic(acc_5, shift_amount);                                \
        acc_6 = intrinsic(acc_6, shift_amount);                                \
        acc_7 = intrinsic(acc_7, shift_amount);                                \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(acc_0, 0) +                    \
                              _mm_extract_epi32(acc_4, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

#define YMM_LAT_SHIFT_IMM(func_name, intrinsic, init_val, shift_amount)        \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define YMM_TP_SHIFT_IMM(func_name, intrinsic, init_fn, init_arg, shift_amount) \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m256i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m256i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m256i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, shift_amount);                                \
        acc_1 = intrinsic(acc_1, shift_amount);                                \
        acc_2 = intrinsic(acc_2, shift_amount);                                \
        acc_3 = intrinsic(acc_3, shift_amount);                                \
        acc_4 = intrinsic(acc_4, shift_amount);                                \
        acc_5 = intrinsic(acc_5, shift_amount);                                \
        acc_6 = intrinsic(acc_6, shift_amount);                                \
        acc_7 = intrinsic(acc_7, shift_amount);                                \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) +                 \
                              _mm256_extract_epi32(acc_4, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * Unary/widening macros: intrinsic takes one argument (e.g. pmovzx, pabs).
 * Latency: chain through the single operand.
 * Throughput: 8 independent streams, each fed back to itself.
 */
#define XMM_LAT_UNARY(func_name, intrinsic, init_val)                          \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define XMM_TP_UNARY(func_name, intrinsic, init_fn, init_arg)                  \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m128i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m128i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m128i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0); acc_1 = intrinsic(acc_1);                    \
        acc_2 = intrinsic(acc_2); acc_3 = intrinsic(acc_3);                    \
        acc_4 = intrinsic(acc_4); acc_5 = intrinsic(acc_5);                    \
        acc_6 = intrinsic(acc_6); acc_7 = intrinsic(acc_7);                    \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(acc_0, 0) +                    \
                              _mm_extract_epi32(acc_4, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

#define YMM_LAT_UNARY(func_name, intrinsic, init_val)                          \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define YMM_TP_UNARY(func_name, intrinsic, init_fn, init_arg)                  \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m256i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m256i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m256i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0); acc_1 = intrinsic(acc_1);                    \
        acc_2 = intrinsic(acc_2); acc_3 = intrinsic(acc_3);                    \
        acc_4 = intrinsic(acc_4); acc_5 = intrinsic(acc_5);                    \
        acc_6 = intrinsic(acc_6); acc_7 = intrinsic(acc_7);                    \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) +                 \
                              _mm256_extract_epi32(acc_4, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * Shuffle-with-immediate macros (e.g. PSHUFD, PSHUFHW, PSHUFLW, PALIGNR).
 */
#define XMM_LAT_SHUF_IMM(func_name, intrinsic, init_val, imm)                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define XMM_TP_SHUF_IMM(func_name, intrinsic, init_fn, init_arg, imm)         \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m128i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m128i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m128i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, imm); acc_1 = intrinsic(acc_1, imm);         \
        acc_2 = intrinsic(acc_2, imm); acc_3 = intrinsic(acc_3, imm);         \
        acc_4 = intrinsic(acc_4, imm); acc_5 = intrinsic(acc_5, imm);         \
        acc_6 = intrinsic(acc_6, imm); acc_7 = intrinsic(acc_7, imm);         \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(acc_0, 0) +                    \
                              _mm_extract_epi32(acc_4, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

#define YMM_LAT_SHUF_IMM(func_name, intrinsic, init_val, imm)                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define YMM_TP_SHUF_IMM(func_name, intrinsic, init_fn, init_arg, imm)         \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m256i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m256i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m256i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, imm); acc_1 = intrinsic(acc_1, imm);         \
        acc_2 = intrinsic(acc_2, imm); acc_3 = intrinsic(acc_3, imm);         \
        acc_4 = intrinsic(acc_4, imm); acc_5 = intrinsic(acc_5, imm);         \
        acc_6 = intrinsic(acc_6, imm); acc_7 = intrinsic(acc_7, imm);         \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) +                 \
                              _mm256_extract_epi32(acc_4, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * PALIGNR-style: binary with immediate (acc, source, imm).
 */
#define XMM_LAT_BINARY_IMM(func_name, intrinsic, init_val, imm)               \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i accumulator = init_val;                                            \
    __m128i source_operand = init_val;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define XMM_TP_BINARY_IMM(func_name, intrinsic, init_fn, init_arg, imm)       \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m128i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m128i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m128i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m128i source_operand = init_fn(init_arg);                                \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, source_operand, imm);                         \
        acc_1 = intrinsic(acc_1, source_operand, imm);                         \
        acc_2 = intrinsic(acc_2, source_operand, imm);                         \
        acc_3 = intrinsic(acc_3, source_operand, imm);                         \
        acc_4 = intrinsic(acc_4, source_operand, imm);                         \
        acc_5 = intrinsic(acc_5, source_operand, imm);                         \
        acc_6 = intrinsic(acc_6, source_operand, imm);                         \
        acc_7 = intrinsic(acc_7, source_operand, imm);                         \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(acc_0, 0) +                    \
                              _mm_extract_epi32(acc_4, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

#define YMM_LAT_BINARY_IMM(func_name, intrinsic, init_val, imm)               \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i accumulator = init_val;                                            \
    __m256i source_operand = init_val;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define YMM_TP_BINARY_IMM(func_name, intrinsic, init_fn, init_arg, imm)       \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m256i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m256i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m256i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m256i source_operand = init_fn(init_arg);                                \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, source_operand, imm);                         \
        acc_1 = intrinsic(acc_1, source_operand, imm);                         \
        acc_2 = intrinsic(acc_2, source_operand, imm);                         \
        acc_3 = intrinsic(acc_3, source_operand, imm);                         \
        acc_4 = intrinsic(acc_4, source_operand, imm);                         \
        acc_5 = intrinsic(acc_5, source_operand, imm);                         \
        acc_6 = intrinsic(acc_6, source_operand, imm);                         \
        acc_7 = intrinsic(acc_7, source_operand, imm);                         \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) +                 \
                              _mm256_extract_epi32(acc_4, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* ========================================================================
 * ARITHMETIC -- ADD
 * ======================================================================== */

/* PADDB xmm */
XMM_LAT_BINARY(paddb_xmm_lat, _mm_add_epi8, _mm_set1_epi8(1))
XMM_TP_BINARY(paddb_xmm_tp, _mm_add_epi8, _mm_set1_epi8, 1)
YMM_LAT_BINARY(paddb_ymm_lat, _mm256_add_epi8, _mm256_set1_epi8(1))
YMM_TP_BINARY(paddb_ymm_tp, _mm256_add_epi8, _mm256_set1_epi8, 1)

/* PADDW xmm/ymm */
XMM_LAT_BINARY(paddw_xmm_lat, _mm_add_epi16, _mm_set1_epi16(1))
XMM_TP_BINARY(paddw_xmm_tp, _mm_add_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(paddw_ymm_lat, _mm256_add_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(paddw_ymm_tp, _mm256_add_epi16, _mm256_set1_epi16, 1)

/* PADDD xmm/ymm */
XMM_LAT_BINARY(paddd_xmm_lat, _mm_add_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(paddd_xmm_tp, _mm_add_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(paddd_ymm_lat, _mm256_add_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(paddd_ymm_tp, _mm256_add_epi32, _mm256_set1_epi32, 1)

/* PADDQ xmm/ymm */
XMM_LAT_BINARY(paddq_xmm_lat, _mm_add_epi64, _mm_set1_epi64x(1))
XMM_TP_BINARY(paddq_xmm_tp, _mm_add_epi64, _mm_set1_epi64x, 1)
YMM_LAT_BINARY(paddq_ymm_lat, _mm256_add_epi64, _mm256_set1_epi64x(1))
YMM_TP_BINARY(paddq_ymm_tp, _mm256_add_epi64, _mm256_set1_epi64x, 1)

/* ========================================================================
 * ARITHMETIC -- SUB
 * ======================================================================== */

XMM_LAT_BINARY(psubb_xmm_lat, _mm_sub_epi8, _mm_set1_epi8(100))
XMM_TP_BINARY(psubb_xmm_tp, _mm_sub_epi8, _mm_set1_epi8, 1)
YMM_LAT_BINARY(psubb_ymm_lat, _mm256_sub_epi8, _mm256_set1_epi8(100))
YMM_TP_BINARY(psubb_ymm_tp, _mm256_sub_epi8, _mm256_set1_epi8, 1)

XMM_LAT_BINARY(psubw_xmm_lat, _mm_sub_epi16, _mm_set1_epi16(100))
XMM_TP_BINARY(psubw_xmm_tp, _mm_sub_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(psubw_ymm_lat, _mm256_sub_epi16, _mm256_set1_epi16(100))
YMM_TP_BINARY(psubw_ymm_tp, _mm256_sub_epi16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(psubd_xmm_lat, _mm_sub_epi32, _mm_set1_epi32(100))
XMM_TP_BINARY(psubd_xmm_tp, _mm_sub_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(psubd_ymm_lat, _mm256_sub_epi32, _mm256_set1_epi32(100))
YMM_TP_BINARY(psubd_ymm_tp, _mm256_sub_epi32, _mm256_set1_epi32, 1)

XMM_LAT_BINARY(psubq_xmm_lat, _mm_sub_epi64, _mm_set1_epi64x(100))
XMM_TP_BINARY(psubq_xmm_tp, _mm_sub_epi64, _mm_set1_epi64x, 1)
YMM_LAT_BINARY(psubq_ymm_lat, _mm256_sub_epi64, _mm256_set1_epi64x(100))
YMM_TP_BINARY(psubq_ymm_tp, _mm256_sub_epi64, _mm256_set1_epi64x, 1)

/* ========================================================================
 * ARITHMETIC -- SATURATING ADD/SUB
 * ======================================================================== */

XMM_LAT_BINARY(paddusb_xmm_lat, _mm_adds_epu8, _mm_set1_epi8(1))
XMM_TP_BINARY(paddusb_xmm_tp, _mm_adds_epu8, _mm_set1_epi8, 1)
YMM_LAT_BINARY(paddusb_ymm_lat, _mm256_adds_epu8, _mm256_set1_epi8(1))
YMM_TP_BINARY(paddusb_ymm_tp, _mm256_adds_epu8, _mm256_set1_epi8, 1)

XMM_LAT_BINARY(paddusw_xmm_lat, _mm_adds_epu16, _mm_set1_epi16(1))
XMM_TP_BINARY(paddusw_xmm_tp, _mm_adds_epu16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(paddusw_ymm_lat, _mm256_adds_epu16, _mm256_set1_epi16(1))
YMM_TP_BINARY(paddusw_ymm_tp, _mm256_adds_epu16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(paddsb_xmm_lat, _mm_adds_epi8, _mm_set1_epi8(1))
XMM_TP_BINARY(paddsb_xmm_tp, _mm_adds_epi8, _mm_set1_epi8, 1)
YMM_LAT_BINARY(paddsb_ymm_lat, _mm256_adds_epi8, _mm256_set1_epi8(1))
YMM_TP_BINARY(paddsb_ymm_tp, _mm256_adds_epi8, _mm256_set1_epi8, 1)

XMM_LAT_BINARY(paddsw_xmm_lat, _mm_adds_epi16, _mm_set1_epi16(1))
XMM_TP_BINARY(paddsw_xmm_tp, _mm_adds_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(paddsw_ymm_lat, _mm256_adds_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(paddsw_ymm_tp, _mm256_adds_epi16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(psubusb_xmm_lat, _mm_subs_epu8, _mm_set1_epi8(100))
XMM_TP_BINARY(psubusb_xmm_tp, _mm_subs_epu8, _mm_set1_epi8, 50)
YMM_LAT_BINARY(psubusb_ymm_lat, _mm256_subs_epu8, _mm256_set1_epi8(100))
YMM_TP_BINARY(psubusb_ymm_tp, _mm256_subs_epu8, _mm256_set1_epi8, 50)

XMM_LAT_BINARY(psubusw_xmm_lat, _mm_subs_epu16, _mm_set1_epi16(1000))
XMM_TP_BINARY(psubusw_xmm_tp, _mm_subs_epu16, _mm_set1_epi16, 50)
YMM_LAT_BINARY(psubusw_ymm_lat, _mm256_subs_epu16, _mm256_set1_epi16(1000))
YMM_TP_BINARY(psubusw_ymm_tp, _mm256_subs_epu16, _mm256_set1_epi16, 50)

XMM_LAT_BINARY(psubsb_xmm_lat, _mm_subs_epi8, _mm_set1_epi8(50))
XMM_TP_BINARY(psubsb_xmm_tp, _mm_subs_epi8, _mm_set1_epi8, 50)
YMM_LAT_BINARY(psubsb_ymm_lat, _mm256_subs_epi8, _mm256_set1_epi8(50))
YMM_TP_BINARY(psubsb_ymm_tp, _mm256_subs_epi8, _mm256_set1_epi8, 50)

XMM_LAT_BINARY(psubsw_xmm_lat, _mm_subs_epi16, _mm_set1_epi16(50))
XMM_TP_BINARY(psubsw_xmm_tp, _mm_subs_epi16, _mm_set1_epi16, 50)
YMM_LAT_BINARY(psubsw_ymm_lat, _mm256_subs_epi16, _mm256_set1_epi16(50))
YMM_TP_BINARY(psubsw_ymm_tp, _mm256_subs_epi16, _mm256_set1_epi16, 50)

/* ========================================================================
 * ARITHMETIC -- MULTIPLY
 * ======================================================================== */

XMM_LAT_BINARY(pmullw_xmm_lat, _mm_mullo_epi16, _mm_set1_epi16(3))
XMM_TP_BINARY(pmullw_xmm_tp, _mm_mullo_epi16, _mm_set1_epi16, 3)
YMM_LAT_BINARY(pmullw_ymm_lat, _mm256_mullo_epi16, _mm256_set1_epi16(3))
YMM_TP_BINARY(pmullw_ymm_tp, _mm256_mullo_epi16, _mm256_set1_epi16, 3)

XMM_LAT_BINARY(pmulld_xmm_lat, _mm_mullo_epi32, _mm_set1_epi32(3))
XMM_TP_BINARY(pmulld_xmm_tp, _mm_mullo_epi32, _mm_set1_epi32, 3)
YMM_LAT_BINARY(pmulld_ymm_lat, _mm256_mullo_epi32, _mm256_set1_epi32(3))
YMM_TP_BINARY(pmulld_ymm_tp, _mm256_mullo_epi32, _mm256_set1_epi32, 3)

XMM_LAT_BINARY(pmuldq_xmm_lat, _mm_mul_epi32, _mm_set1_epi32(3))
XMM_TP_BINARY(pmuldq_xmm_tp, _mm_mul_epi32, _mm_set1_epi32, 3)
YMM_LAT_BINARY(pmuldq_ymm_lat, _mm256_mul_epi32, _mm256_set1_epi32(3))
YMM_TP_BINARY(pmuldq_ymm_tp, _mm256_mul_epi32, _mm256_set1_epi32, 3)

XMM_LAT_BINARY(pmuludq_xmm_lat, _mm_mul_epu32, _mm_set1_epi32(3))
XMM_TP_BINARY(pmuludq_xmm_tp, _mm_mul_epu32, _mm_set1_epi32, 3)
YMM_LAT_BINARY(pmuludq_ymm_lat, _mm256_mul_epu32, _mm256_set1_epi32(3))
YMM_TP_BINARY(pmuludq_ymm_tp, _mm256_mul_epu32, _mm256_set1_epi32, 3)

XMM_LAT_BINARY(pmulhw_xmm_lat, _mm_mulhi_epi16, _mm_set1_epi16(3))
XMM_TP_BINARY(pmulhw_xmm_tp, _mm_mulhi_epi16, _mm_set1_epi16, 3)
YMM_LAT_BINARY(pmulhw_ymm_lat, _mm256_mulhi_epi16, _mm256_set1_epi16(3))
YMM_TP_BINARY(pmulhw_ymm_tp, _mm256_mulhi_epi16, _mm256_set1_epi16, 3)

XMM_LAT_BINARY(pmulhuw_xmm_lat, _mm_mulhi_epu16, _mm_set1_epi16(3))
XMM_TP_BINARY(pmulhuw_xmm_tp, _mm_mulhi_epu16, _mm_set1_epi16, 3)
YMM_LAT_BINARY(pmulhuw_ymm_lat, _mm256_mulhi_epu16, _mm256_set1_epi16(3))
YMM_TP_BINARY(pmulhuw_ymm_tp, _mm256_mulhi_epu16, _mm256_set1_epi16, 3)

XMM_LAT_BINARY(pmaddwd_xmm_lat, _mm_madd_epi16, _mm_set1_epi16(1))
XMM_TP_BINARY(pmaddwd_xmm_tp, _mm_madd_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(pmaddwd_ymm_lat, _mm256_madd_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(pmaddwd_ymm_tp, _mm256_madd_epi16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(pmaddubsw_xmm_lat, _mm_maddubs_epi16, _mm_set1_epi8(1))
XMM_TP_BINARY(pmaddubsw_xmm_tp, _mm_maddubs_epi16, _mm_set1_epi8, 1)
YMM_LAT_BINARY(pmaddubsw_ymm_lat, _mm256_maddubs_epi16, _mm256_set1_epi8(1))
YMM_TP_BINARY(pmaddubsw_ymm_tp, _mm256_maddubs_epi16, _mm256_set1_epi8, 1)

/* ========================================================================
 * ARITHMETIC -- HORIZONTAL ADD/SUB
 * ======================================================================== */

XMM_LAT_BINARY(phaddw_xmm_lat, _mm_hadd_epi16, _mm_set1_epi16(1))
XMM_TP_BINARY(phaddw_xmm_tp, _mm_hadd_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(phaddw_ymm_lat, _mm256_hadd_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(phaddw_ymm_tp, _mm256_hadd_epi16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(phaddd_xmm_lat, _mm_hadd_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(phaddd_xmm_tp, _mm_hadd_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(phaddd_ymm_lat, _mm256_hadd_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(phaddd_ymm_tp, _mm256_hadd_epi32, _mm256_set1_epi32, 1)

XMM_LAT_BINARY(phsubw_xmm_lat, _mm_hsub_epi16, _mm_set1_epi16(1))
XMM_TP_BINARY(phsubw_xmm_tp, _mm_hsub_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(phsubw_ymm_lat, _mm256_hsub_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(phsubw_ymm_tp, _mm256_hsub_epi16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(phsubd_xmm_lat, _mm_hsub_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(phsubd_xmm_tp, _mm_hsub_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(phsubd_ymm_lat, _mm256_hsub_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(phsubd_ymm_tp, _mm256_hsub_epi32, _mm256_set1_epi32, 1)

/* ========================================================================
 * ARITHMETIC -- SAD, AVG, ABS
 * ======================================================================== */

XMM_LAT_BINARY(psadbw_xmm_lat, _mm_sad_epu8, _mm_set1_epi8(1))
XMM_TP_BINARY(psadbw_xmm_tp, _mm_sad_epu8, _mm_set1_epi8, 1)
YMM_LAT_BINARY(psadbw_ymm_lat, _mm256_sad_epu8, _mm256_set1_epi8(1))
YMM_TP_BINARY(psadbw_ymm_tp, _mm256_sad_epu8, _mm256_set1_epi8, 1)

XMM_LAT_BINARY(pavgb_xmm_lat, _mm_avg_epu8, _mm_set1_epi8(10))
XMM_TP_BINARY(pavgb_xmm_tp, _mm_avg_epu8, _mm_set1_epi8, 10)
YMM_LAT_BINARY(pavgb_ymm_lat, _mm256_avg_epu8, _mm256_set1_epi8(10))
YMM_TP_BINARY(pavgb_ymm_tp, _mm256_avg_epu8, _mm256_set1_epi8, 10)

XMM_LAT_BINARY(pavgw_xmm_lat, _mm_avg_epu16, _mm_set1_epi16(10))
XMM_TP_BINARY(pavgw_xmm_tp, _mm_avg_epu16, _mm_set1_epi16, 10)
YMM_LAT_BINARY(pavgw_ymm_lat, _mm256_avg_epu16, _mm256_set1_epi16(10))
YMM_TP_BINARY(pavgw_ymm_tp, _mm256_avg_epu16, _mm256_set1_epi16, 10)

XMM_LAT_UNARY(pabsb_xmm_lat, _mm_abs_epi8, _mm_set1_epi8(-5))
XMM_TP_UNARY(pabsb_xmm_tp, _mm_abs_epi8, _mm_set1_epi8, -5)
YMM_LAT_UNARY(pabsb_ymm_lat, _mm256_abs_epi8, _mm256_set1_epi8(-5))
YMM_TP_UNARY(pabsb_ymm_tp, _mm256_abs_epi8, _mm256_set1_epi8, -5)

XMM_LAT_UNARY(pabsw_xmm_lat, _mm_abs_epi16, _mm_set1_epi16(-5))
XMM_TP_UNARY(pabsw_xmm_tp, _mm_abs_epi16, _mm_set1_epi16, -5)
YMM_LAT_UNARY(pabsw_ymm_lat, _mm256_abs_epi16, _mm256_set1_epi16(-5))
YMM_TP_UNARY(pabsw_ymm_tp, _mm256_abs_epi16, _mm256_set1_epi16, -5)

XMM_LAT_UNARY(pabsd_xmm_lat, _mm_abs_epi32, _mm_set1_epi32(-5))
XMM_TP_UNARY(pabsd_xmm_tp, _mm_abs_epi32, _mm_set1_epi32, -5)
YMM_LAT_UNARY(pabsd_ymm_lat, _mm256_abs_epi32, _mm256_set1_epi32(-5))
YMM_TP_UNARY(pabsd_ymm_tp, _mm256_abs_epi32, _mm256_set1_epi32, -5)

/* ========================================================================
 * ARITHMETIC -- MIN / MAX (signed and unsigned)
 * ======================================================================== */

XMM_LAT_BINARY(pminsb_xmm_lat, _mm_min_epi8, _mm_set1_epi8(50))
XMM_TP_BINARY(pminsb_xmm_tp, _mm_min_epi8, _mm_set1_epi8, 10)
YMM_LAT_BINARY(pminsb_ymm_lat, _mm256_min_epi8, _mm256_set1_epi8(50))
YMM_TP_BINARY(pminsb_ymm_tp, _mm256_min_epi8, _mm256_set1_epi8, 10)

XMM_LAT_BINARY(pminsw_xmm_lat, _mm_min_epi16, _mm_set1_epi16(500))
XMM_TP_BINARY(pminsw_xmm_tp, _mm_min_epi16, _mm_set1_epi16, 10)
YMM_LAT_BINARY(pminsw_ymm_lat, _mm256_min_epi16, _mm256_set1_epi16(500))
YMM_TP_BINARY(pminsw_ymm_tp, _mm256_min_epi16, _mm256_set1_epi16, 10)

XMM_LAT_BINARY(pminsd_xmm_lat, _mm_min_epi32, _mm_set1_epi32(500))
XMM_TP_BINARY(pminsd_xmm_tp, _mm_min_epi32, _mm_set1_epi32, 10)
YMM_LAT_BINARY(pminsd_ymm_lat, _mm256_min_epi32, _mm256_set1_epi32(500))
YMM_TP_BINARY(pminsd_ymm_tp, _mm256_min_epi32, _mm256_set1_epi32, 10)

XMM_LAT_BINARY(pmaxsb_xmm_lat, _mm_max_epi8, _mm_set1_epi8(50))
XMM_TP_BINARY(pmaxsb_xmm_tp, _mm_max_epi8, _mm_set1_epi8, 10)
YMM_LAT_BINARY(pmaxsb_ymm_lat, _mm256_max_epi8, _mm256_set1_epi8(50))
YMM_TP_BINARY(pmaxsb_ymm_tp, _mm256_max_epi8, _mm256_set1_epi8, 10)

XMM_LAT_BINARY(pmaxsw_xmm_lat, _mm_max_epi16, _mm_set1_epi16(500))
XMM_TP_BINARY(pmaxsw_xmm_tp, _mm_max_epi16, _mm_set1_epi16, 10)
YMM_LAT_BINARY(pmaxsw_ymm_lat, _mm256_max_epi16, _mm256_set1_epi16(500))
YMM_TP_BINARY(pmaxsw_ymm_tp, _mm256_max_epi16, _mm256_set1_epi16, 10)

XMM_LAT_BINARY(pmaxsd_xmm_lat, _mm_max_epi32, _mm_set1_epi32(500))
XMM_TP_BINARY(pmaxsd_xmm_tp, _mm_max_epi32, _mm_set1_epi32, 10)
YMM_LAT_BINARY(pmaxsd_ymm_lat, _mm256_max_epi32, _mm256_set1_epi32(500))
YMM_TP_BINARY(pmaxsd_ymm_tp, _mm256_max_epi32, _mm256_set1_epi32, 10)

XMM_LAT_BINARY(pminub_xmm_lat, _mm_min_epu8, _mm_set1_epi8((char)200))
XMM_TP_BINARY(pminub_xmm_tp, _mm_min_epu8, _mm_set1_epi8, 10)
YMM_LAT_BINARY(pminub_ymm_lat, _mm256_min_epu8, _mm256_set1_epi8((char)200))
YMM_TP_BINARY(pminub_ymm_tp, _mm256_min_epu8, _mm256_set1_epi8, 10)

XMM_LAT_BINARY(pminuw_xmm_lat, _mm_min_epu16, _mm_set1_epi16(500))
XMM_TP_BINARY(pminuw_xmm_tp, _mm_min_epu16, _mm_set1_epi16, 10)
YMM_LAT_BINARY(pminuw_ymm_lat, _mm256_min_epu16, _mm256_set1_epi16(500))
YMM_TP_BINARY(pminuw_ymm_tp, _mm256_min_epu16, _mm256_set1_epi16, 10)

XMM_LAT_BINARY(pminud_xmm_lat, _mm_min_epu32, _mm_set1_epi32(500))
XMM_TP_BINARY(pminud_xmm_tp, _mm_min_epu32, _mm_set1_epi32, 10)
YMM_LAT_BINARY(pminud_ymm_lat, _mm256_min_epu32, _mm256_set1_epi32(500))
YMM_TP_BINARY(pminud_ymm_tp, _mm256_min_epu32, _mm256_set1_epi32, 10)

XMM_LAT_BINARY(pmaxub_xmm_lat, _mm_max_epu8, _mm_set1_epi8(10))
XMM_TP_BINARY(pmaxub_xmm_tp, _mm_max_epu8, _mm_set1_epi8, 10)
YMM_LAT_BINARY(pmaxub_ymm_lat, _mm256_max_epu8, _mm256_set1_epi8(10))
YMM_TP_BINARY(pmaxub_ymm_tp, _mm256_max_epu8, _mm256_set1_epi8, 10)

XMM_LAT_BINARY(pmaxuw_xmm_lat, _mm_max_epu16, _mm_set1_epi16(500))
XMM_TP_BINARY(pmaxuw_xmm_tp, _mm_max_epu16, _mm_set1_epi16, 10)
YMM_LAT_BINARY(pmaxuw_ymm_lat, _mm256_max_epu16, _mm256_set1_epi16(500))
YMM_TP_BINARY(pmaxuw_ymm_tp, _mm256_max_epu16, _mm256_set1_epi16, 10)

XMM_LAT_BINARY(pmaxud_xmm_lat, _mm_max_epu32, _mm_set1_epi32(500))
XMM_TP_BINARY(pmaxud_xmm_tp, _mm_max_epu32, _mm_set1_epi32, 10)
YMM_LAT_BINARY(pmaxud_ymm_lat, _mm256_max_epu32, _mm256_set1_epi32(500))
YMM_TP_BINARY(pmaxud_ymm_tp, _mm256_max_epu32, _mm256_set1_epi32, 10)

/* ========================================================================
 * LOGIC -- PAND, PANDN, POR, PXOR
 * ======================================================================== */

XMM_LAT_BINARY(pand_xmm_lat, _mm_and_si128, _mm_set1_epi32(0x7FFFFFFF))
XMM_TP_BINARY(pand_xmm_tp, _mm_and_si128, _mm_set1_epi32, 0x7FFFFFFF)
YMM_LAT_BINARY(pand_ymm_lat, _mm256_and_si256, _mm256_set1_epi32(0x7FFFFFFF))
YMM_TP_BINARY(pand_ymm_tp, _mm256_and_si256, _mm256_set1_epi32, 0x7FFFFFFF)

XMM_LAT_BINARY(pandn_xmm_lat, _mm_andnot_si128, _mm_set1_epi32(0x0F0F0F0F))
XMM_TP_BINARY(pandn_xmm_tp, _mm_andnot_si128, _mm_set1_epi32, 0x0F0F0F0F)
YMM_LAT_BINARY(pandn_ymm_lat, _mm256_andnot_si256, _mm256_set1_epi32(0x0F0F0F0F))
YMM_TP_BINARY(pandn_ymm_tp, _mm256_andnot_si256, _mm256_set1_epi32, 0x0F0F0F0F)

XMM_LAT_BINARY(por_xmm_lat, _mm_or_si128, _mm_set1_epi32(1))
XMM_TP_BINARY(por_xmm_tp, _mm_or_si128, _mm_set1_epi32, 1)
YMM_LAT_BINARY(por_ymm_lat, _mm256_or_si256, _mm256_set1_epi32(1))
YMM_TP_BINARY(por_ymm_tp, _mm256_or_si256, _mm256_set1_epi32, 1)

XMM_LAT_BINARY(pxor_xmm_lat, _mm_xor_si128, _mm_set1_epi32(1))
XMM_TP_BINARY(pxor_xmm_tp, _mm_xor_si128, _mm_set1_epi32, 1)
YMM_LAT_BINARY(pxor_ymm_lat, _mm256_xor_si256, _mm256_set1_epi32(1))
YMM_TP_BINARY(pxor_ymm_tp, _mm256_xor_si256, _mm256_set1_epi32, 1)

/* ========================================================================
 * SHIFT -- IMMEDIATE
 * ======================================================================== */

XMM_LAT_SHIFT_IMM(psllw_imm_xmm_lat, _mm_slli_epi16, _mm_set1_epi16(0x1234), 1)
XMM_TP_SHIFT_IMM(psllw_imm_xmm_tp, _mm_slli_epi16, _mm_set1_epi16, 0x1234, 1)
YMM_LAT_SHIFT_IMM(psllw_imm_ymm_lat, _mm256_slli_epi16, _mm256_set1_epi16(0x1234), 1)
YMM_TP_SHIFT_IMM(psllw_imm_ymm_tp, _mm256_slli_epi16, _mm256_set1_epi16, 0x1234, 1)

XMM_LAT_SHIFT_IMM(pslld_imm_xmm_lat, _mm_slli_epi32, _mm_set1_epi32(0x12345678), 1)
XMM_TP_SHIFT_IMM(pslld_imm_xmm_tp, _mm_slli_epi32, _mm_set1_epi32, 0x12345678, 1)
YMM_LAT_SHIFT_IMM(pslld_imm_ymm_lat, _mm256_slli_epi32, _mm256_set1_epi32(0x12345678), 1)
YMM_TP_SHIFT_IMM(pslld_imm_ymm_tp, _mm256_slli_epi32, _mm256_set1_epi32, 0x12345678, 1)

XMM_LAT_SHIFT_IMM(psllq_imm_xmm_lat, _mm_slli_epi64, _mm_set1_epi64x(0x123456789LL), 1)
XMM_TP_SHIFT_IMM(psllq_imm_xmm_tp, _mm_slli_epi64, _mm_set1_epi64x, 0x123456789LL, 1)
YMM_LAT_SHIFT_IMM(psllq_imm_ymm_lat, _mm256_slli_epi64, _mm256_set1_epi64x(0x123456789LL), 1)
YMM_TP_SHIFT_IMM(psllq_imm_ymm_tp, _mm256_slli_epi64, _mm256_set1_epi64x, 0x123456789LL, 1)

XMM_LAT_SHIFT_IMM(psrlw_imm_xmm_lat, _mm_srli_epi16, _mm_set1_epi16(0x1234), 1)
XMM_TP_SHIFT_IMM(psrlw_imm_xmm_tp, _mm_srli_epi16, _mm_set1_epi16, 0x1234, 1)
YMM_LAT_SHIFT_IMM(psrlw_imm_ymm_lat, _mm256_srli_epi16, _mm256_set1_epi16(0x1234), 1)
YMM_TP_SHIFT_IMM(psrlw_imm_ymm_tp, _mm256_srli_epi16, _mm256_set1_epi16, 0x1234, 1)

XMM_LAT_SHIFT_IMM(psrld_imm_xmm_lat, _mm_srli_epi32, _mm_set1_epi32(0x12345678), 1)
XMM_TP_SHIFT_IMM(psrld_imm_xmm_tp, _mm_srli_epi32, _mm_set1_epi32, 0x12345678, 1)
YMM_LAT_SHIFT_IMM(psrld_imm_ymm_lat, _mm256_srli_epi32, _mm256_set1_epi32(0x12345678), 1)
YMM_TP_SHIFT_IMM(psrld_imm_ymm_tp, _mm256_srli_epi32, _mm256_set1_epi32, 0x12345678, 1)

XMM_LAT_SHIFT_IMM(psrlq_imm_xmm_lat, _mm_srli_epi64, _mm_set1_epi64x(0x123456789LL), 1)
XMM_TP_SHIFT_IMM(psrlq_imm_xmm_tp, _mm_srli_epi64, _mm_set1_epi64x, 0x123456789LL, 1)
YMM_LAT_SHIFT_IMM(psrlq_imm_ymm_lat, _mm256_srli_epi64, _mm256_set1_epi64x(0x123456789LL), 1)
YMM_TP_SHIFT_IMM(psrlq_imm_ymm_tp, _mm256_srli_epi64, _mm256_set1_epi64x, 0x123456789LL, 1)

XMM_LAT_SHIFT_IMM(psraw_imm_xmm_lat, _mm_srai_epi16, _mm_set1_epi16(0x1234), 1)
XMM_TP_SHIFT_IMM(psraw_imm_xmm_tp, _mm_srai_epi16, _mm_set1_epi16, 0x1234, 1)
YMM_LAT_SHIFT_IMM(psraw_imm_ymm_lat, _mm256_srai_epi16, _mm256_set1_epi16(0x1234), 1)
YMM_TP_SHIFT_IMM(psraw_imm_ymm_tp, _mm256_srai_epi16, _mm256_set1_epi16, 0x1234, 1)

XMM_LAT_SHIFT_IMM(psrad_imm_xmm_lat, _mm_srai_epi32, _mm_set1_epi32(0x12345678), 1)
XMM_TP_SHIFT_IMM(psrad_imm_xmm_tp, _mm_srai_epi32, _mm_set1_epi32, 0x12345678, 1)
YMM_LAT_SHIFT_IMM(psrad_imm_ymm_lat, _mm256_srai_epi32, _mm256_set1_epi32(0x12345678), 1)
YMM_TP_SHIFT_IMM(psrad_imm_ymm_tp, _mm256_srai_epi32, _mm256_set1_epi32, 0x12345678, 1)

/* ========================================================================
 * SHIFT -- REGISTER (shift count in xmm)
 * ======================================================================== */

/*
 * Register-shift XMM: both data and count are __m128i, macro works fine.
 * Register-shift YMM: data is __m256i, count is __m128i -- need custom macros.
 */

XMM_LAT_BINARY(psllw_reg_xmm_lat, _mm_sll_epi16, _mm_set1_epi64x(1))
XMM_TP_BINARY(psllw_reg_xmm_tp, _mm_sll_epi16, _mm_set1_epi64x, 1)

XMM_LAT_BINARY(pslld_reg_xmm_lat, _mm_sll_epi32, _mm_set1_epi64x(1))
XMM_TP_BINARY(pslld_reg_xmm_tp, _mm_sll_epi32, _mm_set1_epi64x, 1)

XMM_LAT_BINARY(psllq_reg_xmm_lat, _mm_sll_epi64, _mm_set1_epi64x(1))
XMM_TP_BINARY(psllq_reg_xmm_tp, _mm_sll_epi64, _mm_set1_epi64x, 1)

XMM_LAT_BINARY(psrlw_reg_xmm_lat, _mm_srl_epi16, _mm_set1_epi64x(1))
XMM_TP_BINARY(psrlw_reg_xmm_tp, _mm_srl_epi16, _mm_set1_epi64x, 1)

XMM_LAT_BINARY(psrld_reg_xmm_lat, _mm_srl_epi32, _mm_set1_epi64x(1))
XMM_TP_BINARY(psrld_reg_xmm_tp, _mm_srl_epi32, _mm_set1_epi64x, 1)

XMM_LAT_BINARY(psrlq_reg_xmm_lat, _mm_srl_epi64, _mm_set1_epi64x(1))
XMM_TP_BINARY(psrlq_reg_xmm_tp, _mm_srl_epi64, _mm_set1_epi64x, 1)

XMM_LAT_BINARY(psraw_reg_xmm_lat, _mm_sra_epi16, _mm_set1_epi64x(1))
XMM_TP_BINARY(psraw_reg_xmm_tp, _mm_sra_epi16, _mm_set1_epi64x, 1)

XMM_LAT_BINARY(psrad_reg_xmm_lat, _mm_sra_epi32, _mm_set1_epi64x(1))
XMM_TP_BINARY(psrad_reg_xmm_tp, _mm_sra_epi32, _mm_set1_epi64x, 1)

/*
 * YMM register shift: __m256i data, __m128i shift-count.
 * Custom macro since data and count have different types.
 */
#define YMM_REG_SHIFT_LAT(func_name, intrinsic, data_init, count_init)         \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i accumulator = data_init;                                           \
    __m128i shift_count = count_init;                                          \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, shift_count);                     \
        accumulator = intrinsic(accumulator, shift_count);                     \
        accumulator = intrinsic(accumulator, shift_count);                     \
        accumulator = intrinsic(accumulator, shift_count);                     \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define YMM_REG_SHIFT_TP(func_name, intrinsic, data_init_fn, arg, count_init)  \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i acc_0 = data_init_fn(arg+1), acc_1 = data_init_fn(arg+2);         \
    __m256i acc_2 = data_init_fn(arg+3), acc_3 = data_init_fn(arg+4);         \
    __m256i acc_4 = data_init_fn(arg+5), acc_5 = data_init_fn(arg+6);         \
    __m256i acc_6 = data_init_fn(arg+7), acc_7 = data_init_fn(arg+8);         \
    __m128i shift_count = count_init;                                          \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, shift_count);                                 \
        acc_1 = intrinsic(acc_1, shift_count);                                 \
        acc_2 = intrinsic(acc_2, shift_count);                                 \
        acc_3 = intrinsic(acc_3, shift_count);                                 \
        acc_4 = intrinsic(acc_4, shift_count);                                 \
        acc_5 = intrinsic(acc_5, shift_count);                                 \
        acc_6 = intrinsic(acc_6, shift_count);                                 \
        acc_7 = intrinsic(acc_7, shift_count);                                 \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) +                 \
                              _mm256_extract_epi32(acc_4, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

YMM_REG_SHIFT_LAT(psllw_reg_ymm_lat, _mm256_sll_epi16, _mm256_set1_epi16(0x1234), _mm_set1_epi64x(1))
YMM_REG_SHIFT_TP(psllw_reg_ymm_tp, _mm256_sll_epi16, _mm256_set1_epi16, 0x1234, _mm_set1_epi64x(1))

/* ========================================================================
 * SHIFT -- VARIABLE (AVX2 per-element shift)
 * ======================================================================== */

XMM_LAT_BINARY(vpsllvd_xmm_lat, _mm_sllv_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(vpsllvd_xmm_tp, _mm_sllv_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(vpsllvd_ymm_lat, _mm256_sllv_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(vpsllvd_ymm_tp, _mm256_sllv_epi32, _mm256_set1_epi32, 1)

XMM_LAT_BINARY(vpsllvq_xmm_lat, _mm_sllv_epi64, _mm_set1_epi64x(1))
XMM_TP_BINARY(vpsllvq_xmm_tp, _mm_sllv_epi64, _mm_set1_epi64x, 1)
YMM_LAT_BINARY(vpsllvq_ymm_lat, _mm256_sllv_epi64, _mm256_set1_epi64x(1))
YMM_TP_BINARY(vpsllvq_ymm_tp, _mm256_sllv_epi64, _mm256_set1_epi64x, 1)

XMM_LAT_BINARY(vpsrlvd_xmm_lat, _mm_srlv_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(vpsrlvd_xmm_tp, _mm_srlv_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(vpsrlvd_ymm_lat, _mm256_srlv_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(vpsrlvd_ymm_tp, _mm256_srlv_epi32, _mm256_set1_epi32, 1)

XMM_LAT_BINARY(vpsrlvq_xmm_lat, _mm_srlv_epi64, _mm_set1_epi64x(1))
XMM_TP_BINARY(vpsrlvq_xmm_tp, _mm_srlv_epi64, _mm_set1_epi64x, 1)
YMM_LAT_BINARY(vpsrlvq_ymm_lat, _mm256_srlv_epi64, _mm256_set1_epi64x(1))
YMM_TP_BINARY(vpsrlvq_ymm_tp, _mm256_srlv_epi64, _mm256_set1_epi64x, 1)

XMM_LAT_BINARY(vpsravd_xmm_lat, _mm_srav_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(vpsravd_xmm_tp, _mm_srav_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(vpsravd_ymm_lat, _mm256_srav_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(vpsravd_ymm_tp, _mm256_srav_epi32, _mm256_set1_epi32, 1)

/* ========================================================================
 * COMPARE
 * ======================================================================== */

XMM_LAT_BINARY(pcmpeqb_xmm_lat, _mm_cmpeq_epi8, _mm_set1_epi8(42))
XMM_TP_BINARY(pcmpeqb_xmm_tp, _mm_cmpeq_epi8, _mm_set1_epi8, 42)
YMM_LAT_BINARY(pcmpeqb_ymm_lat, _mm256_cmpeq_epi8, _mm256_set1_epi8(42))
YMM_TP_BINARY(pcmpeqb_ymm_tp, _mm256_cmpeq_epi8, _mm256_set1_epi8, 42)

XMM_LAT_BINARY(pcmpeqw_xmm_lat, _mm_cmpeq_epi16, _mm_set1_epi16(42))
XMM_TP_BINARY(pcmpeqw_xmm_tp, _mm_cmpeq_epi16, _mm_set1_epi16, 42)
YMM_LAT_BINARY(pcmpeqw_ymm_lat, _mm256_cmpeq_epi16, _mm256_set1_epi16(42))
YMM_TP_BINARY(pcmpeqw_ymm_tp, _mm256_cmpeq_epi16, _mm256_set1_epi16, 42)

XMM_LAT_BINARY(pcmpeqd_xmm_lat, _mm_cmpeq_epi32, _mm_set1_epi32(42))
XMM_TP_BINARY(pcmpeqd_xmm_tp, _mm_cmpeq_epi32, _mm_set1_epi32, 42)
YMM_LAT_BINARY(pcmpeqd_ymm_lat, _mm256_cmpeq_epi32, _mm256_set1_epi32(42))
YMM_TP_BINARY(pcmpeqd_ymm_tp, _mm256_cmpeq_epi32, _mm256_set1_epi32, 42)

XMM_LAT_BINARY(pcmpeqq_xmm_lat, _mm_cmpeq_epi64, _mm_set1_epi64x(42))
XMM_TP_BINARY(pcmpeqq_xmm_tp, _mm_cmpeq_epi64, _mm_set1_epi64x, 42)
YMM_LAT_BINARY(pcmpeqq_ymm_lat, _mm256_cmpeq_epi64, _mm256_set1_epi64x(42))
YMM_TP_BINARY(pcmpeqq_ymm_tp, _mm256_cmpeq_epi64, _mm256_set1_epi64x, 42)

XMM_LAT_BINARY(pcmpgtb_xmm_lat, _mm_cmpgt_epi8, _mm_set1_epi8(42))
XMM_TP_BINARY(pcmpgtb_xmm_tp, _mm_cmpgt_epi8, _mm_set1_epi8, 42)
YMM_LAT_BINARY(pcmpgtb_ymm_lat, _mm256_cmpgt_epi8, _mm256_set1_epi8(42))
YMM_TP_BINARY(pcmpgtb_ymm_tp, _mm256_cmpgt_epi8, _mm256_set1_epi8, 42)

XMM_LAT_BINARY(pcmpgtw_xmm_lat, _mm_cmpgt_epi16, _mm_set1_epi16(42))
XMM_TP_BINARY(pcmpgtw_xmm_tp, _mm_cmpgt_epi16, _mm_set1_epi16, 42)
YMM_LAT_BINARY(pcmpgtw_ymm_lat, _mm256_cmpgt_epi16, _mm256_set1_epi16(42))
YMM_TP_BINARY(pcmpgtw_ymm_tp, _mm256_cmpgt_epi16, _mm256_set1_epi16, 42)

XMM_LAT_BINARY(pcmpgtd_xmm_lat, _mm_cmpgt_epi32, _mm_set1_epi32(42))
XMM_TP_BINARY(pcmpgtd_xmm_tp, _mm_cmpgt_epi32, _mm_set1_epi32, 42)
YMM_LAT_BINARY(pcmpgtd_ymm_lat, _mm256_cmpgt_epi32, _mm256_set1_epi32(42))
YMM_TP_BINARY(pcmpgtd_ymm_tp, _mm256_cmpgt_epi32, _mm256_set1_epi32, 42)

XMM_LAT_BINARY(pcmpgtq_xmm_lat, _mm_cmpgt_epi64, _mm_set1_epi64x(42))
XMM_TP_BINARY(pcmpgtq_xmm_tp, _mm_cmpgt_epi64, _mm_set1_epi64x, 42)
YMM_LAT_BINARY(pcmpgtq_ymm_lat, _mm256_cmpgt_epi64, _mm256_set1_epi64x(42))
YMM_TP_BINARY(pcmpgtq_ymm_tp, _mm256_cmpgt_epi64, _mm256_set1_epi64x, 42)

/* ========================================================================
 * SHUFFLE / PERMUTE
 * ======================================================================== */

/* PSHUFB xmm/ymm -- byte shuffle */
XMM_LAT_BINARY(pshufb_xmm_lat, _mm_shuffle_epi8, _mm_set1_epi8(0x03))
XMM_TP_BINARY(pshufb_xmm_tp, _mm_shuffle_epi8, _mm_set1_epi8, 0x03)
YMM_LAT_BINARY(pshufb_ymm_lat, _mm256_shuffle_epi8, _mm256_set1_epi8(0x03))
YMM_TP_BINARY(pshufb_ymm_tp, _mm256_shuffle_epi8, _mm256_set1_epi8, 0x03)

/* PSHUFD xmm/ymm -- dword shuffle with imm8 */
XMM_LAT_SHUF_IMM(pshufd_xmm_lat, _mm_shuffle_epi32, _mm_set1_epi32(0x12345678), 0x1B)
XMM_TP_SHUF_IMM(pshufd_xmm_tp, _mm_shuffle_epi32, _mm_set1_epi32, 0x12345678, 0x1B)
YMM_LAT_SHUF_IMM(pshufd_ymm_lat, _mm256_shuffle_epi32, _mm256_set1_epi32(0x12345678), 0x1B)
YMM_TP_SHUF_IMM(pshufd_ymm_tp, _mm256_shuffle_epi32, _mm256_set1_epi32, 0x12345678, 0x1B)

/* PSHUFHW / PSHUFLW xmm/ymm */
XMM_LAT_SHUF_IMM(pshufhw_xmm_lat, _mm_shufflehi_epi16, _mm_set1_epi16(0x1234), 0x1B)
XMM_TP_SHUF_IMM(pshufhw_xmm_tp, _mm_shufflehi_epi16, _mm_set1_epi16, 0x1234, 0x1B)
YMM_LAT_SHUF_IMM(pshufhw_ymm_lat, _mm256_shufflehi_epi16, _mm256_set1_epi16(0x1234), 0x1B)
YMM_TP_SHUF_IMM(pshufhw_ymm_tp, _mm256_shufflehi_epi16, _mm256_set1_epi16, 0x1234, 0x1B)

XMM_LAT_SHUF_IMM(pshuflw_xmm_lat, _mm_shufflelo_epi16, _mm_set1_epi16(0x1234), 0x1B)
XMM_TP_SHUF_IMM(pshuflw_xmm_tp, _mm_shufflelo_epi16, _mm_set1_epi16, 0x1234, 0x1B)
YMM_LAT_SHUF_IMM(pshuflw_ymm_lat, _mm256_shufflelo_epi16, _mm256_set1_epi16(0x1234), 0x1B)
YMM_TP_SHUF_IMM(pshuflw_ymm_tp, _mm256_shufflelo_epi16, _mm256_set1_epi16, 0x1234, 0x1B)

/* VPERMD ymm -- cross-lane dword permute (AVX2 only) */
YMM_LAT_BINARY(vpermd_ymm_lat, _mm256_permutevar8x32_epi32, _mm256_set_epi32(7,6,5,4,3,2,1,0))
YMM_TP_BINARY(vpermd_ymm_tp, _mm256_permutevar8x32_epi32, _mm256_set1_epi32, 0)

/* VPERM2I128 ymm -- 128-bit lane permute */
YMM_LAT_BINARY_IMM(vperm2i128_ymm_lat, _mm256_permute2x128_si256, _mm256_set1_epi32(0x12345678), 0x01)
YMM_TP_BINARY_IMM(vperm2i128_ymm_tp, _mm256_permute2x128_si256, _mm256_set1_epi32, 0x12345678, 0x01)

/* PALIGNR xmm/ymm */
XMM_LAT_BINARY_IMM(palignr_xmm_lat, _mm_alignr_epi8, _mm_set1_epi32(0x12345678), 4)
XMM_TP_BINARY_IMM(palignr_xmm_tp, _mm_alignr_epi8, _mm_set1_epi32, 0x12345678, 4)
YMM_LAT_BINARY_IMM(palignr_ymm_lat, _mm256_alignr_epi8, _mm256_set1_epi32(0x12345678), 4)
YMM_TP_BINARY_IMM(palignr_ymm_tp, _mm256_alignr_epi8, _mm256_set1_epi32, 0x12345678, 4)

/* ========================================================================
 * UNPACK
 * ======================================================================== */

XMM_LAT_BINARY(punpcklbw_xmm_lat, _mm_unpacklo_epi8, _mm_set1_epi8(1))
XMM_TP_BINARY(punpcklbw_xmm_tp, _mm_unpacklo_epi8, _mm_set1_epi8, 1)
YMM_LAT_BINARY(punpcklbw_ymm_lat, _mm256_unpacklo_epi8, _mm256_set1_epi8(1))
YMM_TP_BINARY(punpcklbw_ymm_tp, _mm256_unpacklo_epi8, _mm256_set1_epi8, 1)

XMM_LAT_BINARY(punpckhbw_xmm_lat, _mm_unpackhi_epi8, _mm_set1_epi8(1))
XMM_TP_BINARY(punpckhbw_xmm_tp, _mm_unpackhi_epi8, _mm_set1_epi8, 1)
YMM_LAT_BINARY(punpckhbw_ymm_lat, _mm256_unpackhi_epi8, _mm256_set1_epi8(1))
YMM_TP_BINARY(punpckhbw_ymm_tp, _mm256_unpackhi_epi8, _mm256_set1_epi8, 1)

XMM_LAT_BINARY(punpcklwd_xmm_lat, _mm_unpacklo_epi16, _mm_set1_epi16(1))
XMM_TP_BINARY(punpcklwd_xmm_tp, _mm_unpacklo_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(punpcklwd_ymm_lat, _mm256_unpacklo_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(punpcklwd_ymm_tp, _mm256_unpacklo_epi16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(punpckhwd_xmm_lat, _mm_unpackhi_epi16, _mm_set1_epi16(1))
XMM_TP_BINARY(punpckhwd_xmm_tp, _mm_unpackhi_epi16, _mm_set1_epi16, 1)
YMM_LAT_BINARY(punpckhwd_ymm_lat, _mm256_unpackhi_epi16, _mm256_set1_epi16(1))
YMM_TP_BINARY(punpckhwd_ymm_tp, _mm256_unpackhi_epi16, _mm256_set1_epi16, 1)

XMM_LAT_BINARY(punpckldq_xmm_lat, _mm_unpacklo_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(punpckldq_xmm_tp, _mm_unpacklo_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(punpckldq_ymm_lat, _mm256_unpacklo_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(punpckldq_ymm_tp, _mm256_unpacklo_epi32, _mm256_set1_epi32, 1)

XMM_LAT_BINARY(punpckhdq_xmm_lat, _mm_unpackhi_epi32, _mm_set1_epi32(1))
XMM_TP_BINARY(punpckhdq_xmm_tp, _mm_unpackhi_epi32, _mm_set1_epi32, 1)
YMM_LAT_BINARY(punpckhdq_ymm_lat, _mm256_unpackhi_epi32, _mm256_set1_epi32(1))
YMM_TP_BINARY(punpckhdq_ymm_tp, _mm256_unpackhi_epi32, _mm256_set1_epi32, 1)

XMM_LAT_BINARY(punpcklqdq_xmm_lat, _mm_unpacklo_epi64, _mm_set1_epi64x(1))
XMM_TP_BINARY(punpcklqdq_xmm_tp, _mm_unpacklo_epi64, _mm_set1_epi64x, 1)
YMM_LAT_BINARY(punpcklqdq_ymm_lat, _mm256_unpacklo_epi64, _mm256_set1_epi64x(1))
YMM_TP_BINARY(punpcklqdq_ymm_tp, _mm256_unpacklo_epi64, _mm256_set1_epi64x, 1)

XMM_LAT_BINARY(punpckhqdq_xmm_lat, _mm_unpackhi_epi64, _mm_set1_epi64x(1))
XMM_TP_BINARY(punpckhqdq_xmm_tp, _mm_unpackhi_epi64, _mm_set1_epi64x, 1)
YMM_LAT_BINARY(punpckhqdq_ymm_lat, _mm256_unpackhi_epi64, _mm256_set1_epi64x(1))
YMM_TP_BINARY(punpckhqdq_ymm_tp, _mm256_unpackhi_epi64, _mm256_set1_epi64x, 1)

/* ========================================================================
 * PACK
 * ======================================================================== */

XMM_LAT_BINARY(packuswb_xmm_lat, _mm_packus_epi16, _mm_set1_epi16(42))
XMM_TP_BINARY(packuswb_xmm_tp, _mm_packus_epi16, _mm_set1_epi16, 42)
YMM_LAT_BINARY(packuswb_ymm_lat, _mm256_packus_epi16, _mm256_set1_epi16(42))
YMM_TP_BINARY(packuswb_ymm_tp, _mm256_packus_epi16, _mm256_set1_epi16, 42)

XMM_LAT_BINARY(packsswb_xmm_lat, _mm_packs_epi16, _mm_set1_epi16(42))
XMM_TP_BINARY(packsswb_xmm_tp, _mm_packs_epi16, _mm_set1_epi16, 42)
YMM_LAT_BINARY(packsswb_ymm_lat, _mm256_packs_epi16, _mm256_set1_epi16(42))
YMM_TP_BINARY(packsswb_ymm_tp, _mm256_packs_epi16, _mm256_set1_epi16, 42)

XMM_LAT_BINARY(packusdw_xmm_lat, _mm_packus_epi32, _mm_set1_epi32(42))
XMM_TP_BINARY(packusdw_xmm_tp, _mm_packus_epi32, _mm_set1_epi32, 42)
YMM_LAT_BINARY(packusdw_ymm_lat, _mm256_packus_epi32, _mm256_set1_epi32(42))
YMM_TP_BINARY(packusdw_ymm_tp, _mm256_packus_epi32, _mm256_set1_epi32, 42)

XMM_LAT_BINARY(packssdw_xmm_lat, _mm_packs_epi32, _mm_set1_epi32(42))
XMM_TP_BINARY(packssdw_xmm_tp, _mm_packs_epi32, _mm_set1_epi32, 42)
YMM_LAT_BINARY(packssdw_ymm_lat, _mm256_packs_epi32, _mm256_set1_epi32(42))
YMM_TP_BINARY(packssdw_ymm_tp, _mm256_packs_epi32, _mm256_set1_epi32, 42)

/* ========================================================================
 * MOVE / EXTRACT / INSERT / BROADCAST
 * ======================================================================== */

/* MOVD: GPR -> XMM -> GPR latency round-trip */
static double movd_xmm_lat(uint64_t iteration_count)
{
    uint32_t gpr_value = 0x12345678u;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m128i xmm_temp = _mm_cvtsi32_si128((int)gpr_value);
        gpr_value = (uint32_t)_mm_cvtsi128_si32(xmm_temp);
        xmm_temp = _mm_cvtsi32_si128((int)gpr_value);
        gpr_value = (uint32_t)_mm_cvtsi128_si32(xmm_temp);
        xmm_temp = _mm_cvtsi32_si128((int)gpr_value);
        gpr_value = (uint32_t)_mm_cvtsi128_si32(xmm_temp);
        xmm_temp = _mm_cvtsi32_si128((int)gpr_value);
        gpr_value = (uint32_t)_mm_cvtsi128_si32(xmm_temp);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink_value = gpr_value; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double movd_xmm_tp(uint64_t iteration_count)
{
    __m128i acc_0 = _mm_set1_epi32(1), acc_1 = _mm_set1_epi32(2);
    __m128i acc_2 = _mm_set1_epi32(3), acc_3 = _mm_set1_epi32(4);
    __m128i acc_4 = _mm_set1_epi32(5), acc_5 = _mm_set1_epi32(6);
    __m128i acc_6 = _mm_set1_epi32(7), acc_7 = _mm_set1_epi32(8);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_cvtsi32_si128(1); acc_1 = _mm_cvtsi32_si128(2);
        acc_2 = _mm_cvtsi32_si128(3); acc_3 = _mm_cvtsi32_si128(4);
        acc_4 = _mm_cvtsi32_si128(5); acc_5 = _mm_cvtsi32_si128(6);
        acc_6 = _mm_cvtsi32_si128(7); acc_7 = _mm_cvtsi32_si128(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_cvtsi128_si32(acc_0) + _mm_cvtsi128_si32(acc_4); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOVQ: GPR64 -> XMM -> GPR64 latency */
static double movq_xmm_lat(uint64_t iteration_count)
{
    int64_t gpr_value = 0x123456789ABCDEFLL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m128i xmm_temp = _mm_cvtsi64_si128(gpr_value);
        gpr_value = _mm_cvtsi128_si64(xmm_temp);
        xmm_temp = _mm_cvtsi64_si128(gpr_value);
        gpr_value = _mm_cvtsi128_si64(xmm_temp);
        xmm_temp = _mm_cvtsi64_si128(gpr_value);
        gpr_value = _mm_cvtsi128_si64(xmm_temp);
        xmm_temp = _mm_cvtsi64_si128(gpr_value);
        gpr_value = _mm_cvtsi128_si64(xmm_temp);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int64_t sink_value = gpr_value; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* PEXTRB throughput -- extract byte to GPR */
static double pextrb_xmm_tp(uint64_t iteration_count)
{
    __m128i src_0 = _mm_set1_epi8(1), src_1 = _mm_set1_epi8(2);
    __m128i src_2 = _mm_set1_epi8(3), src_3 = _mm_set1_epi8(4);
    __m128i src_4 = _mm_set1_epi8(5), src_5 = _mm_set1_epi8(6);
    __m128i src_6 = _mm_set1_epi8(7), src_7 = _mm_set1_epi8(8);
    int result_sum = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_sum += _mm_extract_epi8(src_0, 0);
        result_sum += _mm_extract_epi8(src_1, 1);
        result_sum += _mm_extract_epi8(src_2, 2);
        result_sum += _mm_extract_epi8(src_3, 3);
        result_sum += _mm_extract_epi8(src_4, 4);
        result_sum += _mm_extract_epi8(src_5, 5);
        result_sum += _mm_extract_epi8(src_6, 6);
        result_sum += _mm_extract_epi8(src_7, 7);
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = result_sum; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PEXTRW throughput */
static double pextrw_xmm_tp(uint64_t iteration_count)
{
    __m128i src_0 = _mm_set1_epi16(1), src_1 = _mm_set1_epi16(2);
    __m128i src_2 = _mm_set1_epi16(3), src_3 = _mm_set1_epi16(4);
    __m128i src_4 = _mm_set1_epi16(5), src_5 = _mm_set1_epi16(6);
    __m128i src_6 = _mm_set1_epi16(7), src_7 = _mm_set1_epi16(8);
    int result_sum = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_sum += _mm_extract_epi16(src_0, 0);
        result_sum += _mm_extract_epi16(src_1, 1);
        result_sum += _mm_extract_epi16(src_2, 2);
        result_sum += _mm_extract_epi16(src_3, 3);
        result_sum += _mm_extract_epi16(src_4, 4);
        result_sum += _mm_extract_epi16(src_5, 5);
        result_sum += _mm_extract_epi16(src_6, 6);
        result_sum += _mm_extract_epi16(src_7, 7);
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = result_sum; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PEXTRD throughput */
static double pextrd_xmm_tp(uint64_t iteration_count)
{
    __m128i src_0 = _mm_set1_epi32(1), src_1 = _mm_set1_epi32(2);
    __m128i src_2 = _mm_set1_epi32(3), src_3 = _mm_set1_epi32(4);
    __m128i src_4 = _mm_set1_epi32(5), src_5 = _mm_set1_epi32(6);
    __m128i src_6 = _mm_set1_epi32(7), src_7 = _mm_set1_epi32(8);
    int result_sum = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_sum += _mm_extract_epi32(src_0, 0);
        result_sum += _mm_extract_epi32(src_1, 1);
        result_sum += _mm_extract_epi32(src_2, 2);
        result_sum += _mm_extract_epi32(src_3, 3);
        result_sum += _mm_extract_epi32(src_4, 0);
        result_sum += _mm_extract_epi32(src_5, 1);
        result_sum += _mm_extract_epi32(src_6, 2);
        result_sum += _mm_extract_epi32(src_7, 3);
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = result_sum; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PEXTRQ throughput */
static double pextrq_xmm_tp(uint64_t iteration_count)
{
    __m128i src_0 = _mm_set1_epi64x(1), src_1 = _mm_set1_epi64x(2);
    __m128i src_2 = _mm_set1_epi64x(3), src_3 = _mm_set1_epi64x(4);
    __m128i src_4 = _mm_set1_epi64x(5), src_5 = _mm_set1_epi64x(6);
    __m128i src_6 = _mm_set1_epi64x(7), src_7 = _mm_set1_epi64x(8);
    int64_t result_sum = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_sum += _mm_extract_epi64(src_0, 0);
        result_sum += _mm_extract_epi64(src_1, 1);
        result_sum += _mm_extract_epi64(src_2, 0);
        result_sum += _mm_extract_epi64(src_3, 1);
        result_sum += _mm_extract_epi64(src_4, 0);
        result_sum += _mm_extract_epi64(src_5, 1);
        result_sum += _mm_extract_epi64(src_6, 0);
        result_sum += _mm_extract_epi64(src_7, 1);
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int64_t sink_value = result_sum; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPBROADCASTD xmm/ymm throughput (from GPR) */
static double vpbroadcastd_xmm_tp(uint64_t iteration_count)
{
    __m128i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_set1_epi32(1); acc_1 = _mm_set1_epi32(2);
        acc_2 = _mm_set1_epi32(3); acc_3 = _mm_set1_epi32(4);
        acc_4 = _mm_set1_epi32(5); acc_5 = _mm_set1_epi32(6);
        acc_6 = _mm_set1_epi32(7); acc_7 = _mm_set1_epi32(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(acc_0, 0) + _mm_extract_epi32(acc_4, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpbroadcastd_ymm_tp(uint64_t iteration_count)
{
    __m256i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm256_set1_epi32(1); acc_1 = _mm256_set1_epi32(2);
        acc_2 = _mm256_set1_epi32(3); acc_3 = _mm256_set1_epi32(4);
        acc_4 = _mm256_set1_epi32(5); acc_5 = _mm256_set1_epi32(6);
        acc_6 = _mm256_set1_epi32(7); acc_7 = _mm256_set1_epi32(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) + _mm256_extract_epi32(acc_4, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpbroadcastq_xmm_tp(uint64_t iteration_count)
{
    __m128i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm_set1_epi64x(1); acc_1 = _mm_set1_epi64x(2);
        acc_2 = _mm_set1_epi64x(3); acc_3 = _mm_set1_epi64x(4);
        acc_4 = _mm_set1_epi64x(5); acc_5 = _mm_set1_epi64x(6);
        acc_6 = _mm_set1_epi64x(7); acc_7 = _mm_set1_epi64x(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(acc_0, 0) + _mm_extract_epi32(acc_4, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpbroadcastq_ymm_tp(uint64_t iteration_count)
{
    __m256i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm256_set1_epi64x(1); acc_1 = _mm256_set1_epi64x(2);
        acc_2 = _mm256_set1_epi64x(3); acc_3 = _mm256_set1_epi64x(4);
        acc_4 = _mm256_set1_epi64x(5); acc_5 = _mm256_set1_epi64x(6);
        acc_6 = _mm256_set1_epi64x(7); acc_7 = _mm256_set1_epi64x(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm256_extract_epi32(acc_0, 0) + _mm256_extract_epi32(acc_4, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPGATHERDD xmm/ymm -- gather dwords with dword indices */
static double vpgatherdd_xmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int32_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m128i index_vector = _mm_set_epi32(3, 2, 1, 0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m128i gather_result = _mm_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m128i final_gather = _mm_i32gather_epi32(gather_source_table, index_vector, 4);
    volatile int sink_value = _mm_extract_epi32(final_gather, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpgatherdd_ymm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int32_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m256i index_vector = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m256i gather_result = _mm256_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm256_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm256_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm256_i32gather_epi32(gather_source_table, index_vector, 4);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m256i final_gather = _mm256_i32gather_epi32(gather_source_table, index_vector, 4);
    volatile int sink_value = _mm256_extract_epi32(final_gather, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpgatherdq_xmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int64_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m128i index_vector = _mm_set_epi32(0, 0, 1, 0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m128i gather_result = _mm_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m128i final_gather = _mm_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
    volatile long long sink_value = _mm_extract_epi64(final_gather, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpgatherdq_ymm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int64_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m128i index_vector = _mm_set_epi32(3, 2, 1, 0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m256i gather_result = _mm256_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm256_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm256_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm256_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m256i final_gather = _mm256_i32gather_epi64((const long long *)gather_source_table, index_vector, 8);
    volatile long long sink_value = _mm256_extract_epi64(final_gather, 0); (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * CONVERSION -- PMOVZX / PMOVSX
 * ======================================================================== */

XMM_LAT_UNARY(pmovzxbw_xmm_lat, _mm_cvtepu8_epi16, _mm_set1_epi8(5))
XMM_TP_UNARY(pmovzxbw_xmm_tp, _mm_cvtepu8_epi16, _mm_set1_epi8, 5)

/*
 * YMM widening conversion latency: input is __m128i, output is __m256i.
 * Chain: widen -> truncate-back-to-xmm -> widen -> ...
 * This measures the widening instruction latency itself.
 */
#define YMM_WIDEN_LAT(func_name, widen_intrinsic, init_val)                    \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i narrow_accumulator = init_val;                                     \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __m256i wide_result = widen_intrinsic(narrow_accumulator);             \
        narrow_accumulator = _mm256_castsi256_si128(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm256_castsi256_si128(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm256_castsi256_si128(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm256_castsi256_si128(wide_result);              \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(narrow_accumulator, 0); (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define YMM_WIDEN_TP(func_name, widen_intrinsic, init_fn, init_arg)            \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i src_0 = init_fn(init_arg+1), src_1 = init_fn(init_arg+2);         \
    __m128i src_2 = init_fn(init_arg+3), src_3 = init_fn(init_arg+4);         \
    __m128i src_4 = init_fn(init_arg+5), src_5 = init_fn(init_arg+6);         \
    __m128i src_6 = init_fn(init_arg+7), src_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __m256i d0 = widen_intrinsic(src_0);                                   \
        __m256i d1 = widen_intrinsic(src_1);                                   \
        __m256i d2 = widen_intrinsic(src_2);                                   \
        __m256i d3 = widen_intrinsic(src_3);                                   \
        __m256i d4 = widen_intrinsic(src_4);                                   \
        __m256i d5 = widen_intrinsic(src_5);                                   \
        __m256i d6 = widen_intrinsic(src_6);                                   \
        __m256i d7 = widen_intrinsic(src_7);                                   \
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),           \
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));           \
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),\
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(src_0, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

YMM_WIDEN_LAT(pmovzxbw_ymm_lat, _mm256_cvtepu8_epi16, _mm_set1_epi8(5))
YMM_WIDEN_TP(pmovzxbw_ymm_tp, _mm256_cvtepu8_epi16, _mm_set1_epi8, 5)

XMM_LAT_UNARY(pmovzxbd_xmm_lat, _mm_cvtepu8_epi32, _mm_set1_epi8(5))
XMM_TP_UNARY(pmovzxbd_xmm_tp, _mm_cvtepu8_epi32, _mm_set1_epi8, 5)

XMM_LAT_UNARY(pmovzxbq_xmm_lat, _mm_cvtepu8_epi64, _mm_set1_epi8(5))
XMM_TP_UNARY(pmovzxbq_xmm_tp, _mm_cvtepu8_epi64, _mm_set1_epi8, 5)

XMM_LAT_UNARY(pmovzxwd_xmm_lat, _mm_cvtepu16_epi32, _mm_set1_epi16(5))
XMM_TP_UNARY(pmovzxwd_xmm_tp, _mm_cvtepu16_epi32, _mm_set1_epi16, 5)
YMM_WIDEN_LAT(pmovzxwd_ymm_lat, _mm256_cvtepu16_epi32, _mm_set1_epi16(5))
YMM_WIDEN_TP(pmovzxwd_ymm_tp, _mm256_cvtepu16_epi32, _mm_set1_epi16, 5)

XMM_LAT_UNARY(pmovzxwq_xmm_lat, _mm_cvtepu16_epi64, _mm_set1_epi16(5))
XMM_TP_UNARY(pmovzxwq_xmm_tp, _mm_cvtepu16_epi64, _mm_set1_epi16, 5)

XMM_LAT_UNARY(pmovzxdq_xmm_lat, _mm_cvtepu32_epi64, _mm_set1_epi32(5))
XMM_TP_UNARY(pmovzxdq_xmm_tp, _mm_cvtepu32_epi64, _mm_set1_epi32, 5)
YMM_WIDEN_LAT(pmovzxdq_ymm_lat, _mm256_cvtepu32_epi64, _mm_set1_epi32(5))
YMM_WIDEN_TP(pmovzxdq_ymm_tp, _mm256_cvtepu32_epi64, _mm_set1_epi32, 5)

XMM_LAT_UNARY(pmovsxbw_xmm_lat, _mm_cvtepi8_epi16, _mm_set1_epi8(-5))
XMM_TP_UNARY(pmovsxbw_xmm_tp, _mm_cvtepi8_epi16, _mm_set1_epi8, -5)
YMM_WIDEN_LAT(pmovsxbw_ymm_lat, _mm256_cvtepi8_epi16, _mm_set1_epi8(-5))
YMM_WIDEN_TP(pmovsxbw_ymm_tp, _mm256_cvtepi8_epi16, _mm_set1_epi8, -5)

XMM_LAT_UNARY(pmovsxbd_xmm_lat, _mm_cvtepi8_epi32, _mm_set1_epi8(-5))
XMM_TP_UNARY(pmovsxbd_xmm_tp, _mm_cvtepi8_epi32, _mm_set1_epi8, -5)

XMM_LAT_UNARY(pmovsxbq_xmm_lat, _mm_cvtepi8_epi64, _mm_set1_epi8(-5))
XMM_TP_UNARY(pmovsxbq_xmm_tp, _mm_cvtepi8_epi64, _mm_set1_epi8, -5)

XMM_LAT_UNARY(pmovsxwd_xmm_lat, _mm_cvtepi16_epi32, _mm_set1_epi16(-5))
XMM_TP_UNARY(pmovsxwd_xmm_tp, _mm_cvtepi16_epi32, _mm_set1_epi16, -5)
YMM_WIDEN_LAT(pmovsxwd_ymm_lat, _mm256_cvtepi16_epi32, _mm_set1_epi16(-5))
YMM_WIDEN_TP(pmovsxwd_ymm_tp, _mm256_cvtepi16_epi32, _mm_set1_epi16, -5)

XMM_LAT_UNARY(pmovsxwq_xmm_lat, _mm_cvtepi16_epi64, _mm_set1_epi16(-5))
XMM_TP_UNARY(pmovsxwq_xmm_tp, _mm_cvtepi16_epi64, _mm_set1_epi16, -5)

XMM_LAT_UNARY(pmovsxdq_xmm_lat, _mm_cvtepi32_epi64, _mm_set1_epi32(-5))
XMM_TP_UNARY(pmovsxdq_xmm_tp, _mm_cvtepi32_epi64, _mm_set1_epi32, -5)
YMM_WIDEN_LAT(pmovsxdq_ymm_lat, _mm256_cvtepi32_epi64, _mm_set1_epi32(-5))
YMM_WIDEN_TP(pmovsxdq_ymm_tp, _mm256_cvtepi32_epi64, _mm_set1_epi32, -5)

/* ========================================================================
 * MEASUREMENT TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    int width_bits;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} SimdIntProbeEntry;

static const SimdIntProbeEntry probe_table[] = {
    /* --- Arithmetic: Add --- */
    {"PADDB",      "xmm,xmm",       128, "arith_add",  paddb_xmm_lat,     paddb_xmm_tp},
    {"PADDB",      "ymm,ymm",       256, "arith_add",  paddb_ymm_lat,     paddb_ymm_tp},
    {"PADDW",      "xmm,xmm",       128, "arith_add",  paddw_xmm_lat,     paddw_xmm_tp},
    {"PADDW",      "ymm,ymm",       256, "arith_add",  paddw_ymm_lat,     paddw_ymm_tp},
    {"PADDD",      "xmm,xmm",       128, "arith_add",  paddd_xmm_lat,     paddd_xmm_tp},
    {"PADDD",      "ymm,ymm",       256, "arith_add",  paddd_ymm_lat,     paddd_ymm_tp},
    {"PADDQ",      "xmm,xmm",       128, "arith_add",  paddq_xmm_lat,     paddq_xmm_tp},
    {"PADDQ",      "ymm,ymm",       256, "arith_add",  paddq_ymm_lat,     paddq_ymm_tp},

    /* --- Arithmetic: Sub --- */
    {"PSUBB",      "xmm,xmm",       128, "arith_sub",  psubb_xmm_lat,     psubb_xmm_tp},
    {"PSUBB",      "ymm,ymm",       256, "arith_sub",  psubb_ymm_lat,     psubb_ymm_tp},
    {"PSUBW",      "xmm,xmm",       128, "arith_sub",  psubw_xmm_lat,     psubw_xmm_tp},
    {"PSUBW",      "ymm,ymm",       256, "arith_sub",  psubw_ymm_lat,     psubw_ymm_tp},
    {"PSUBD",      "xmm,xmm",       128, "arith_sub",  psubd_xmm_lat,     psubd_xmm_tp},
    {"PSUBD",      "ymm,ymm",       256, "arith_sub",  psubd_ymm_lat,     psubd_ymm_tp},
    {"PSUBQ",      "xmm,xmm",       128, "arith_sub",  psubq_xmm_lat,     psubq_xmm_tp},
    {"PSUBQ",      "ymm,ymm",       256, "arith_sub",  psubq_ymm_lat,     psubq_ymm_tp},

    /* --- Arithmetic: Saturating --- */
    {"PADDUSB",    "xmm,xmm",       128, "arith_sat",  paddusb_xmm_lat,   paddusb_xmm_tp},
    {"PADDUSB",    "ymm,ymm",       256, "arith_sat",  paddusb_ymm_lat,   paddusb_ymm_tp},
    {"PADDUSW",    "xmm,xmm",       128, "arith_sat",  paddusw_xmm_lat,   paddusw_xmm_tp},
    {"PADDUSW",    "ymm,ymm",       256, "arith_sat",  paddusw_ymm_lat,   paddusw_ymm_tp},
    {"PADDSB",     "xmm,xmm",       128, "arith_sat",  paddsb_xmm_lat,    paddsb_xmm_tp},
    {"PADDSB",     "ymm,ymm",       256, "arith_sat",  paddsb_ymm_lat,    paddsb_ymm_tp},
    {"PADDSW",     "xmm,xmm",       128, "arith_sat",  paddsw_xmm_lat,    paddsw_xmm_tp},
    {"PADDSW",     "ymm,ymm",       256, "arith_sat",  paddsw_ymm_lat,    paddsw_ymm_tp},
    {"PSUBUSB",    "xmm,xmm",       128, "arith_sat",  psubusb_xmm_lat,   psubusb_xmm_tp},
    {"PSUBUSB",    "ymm,ymm",       256, "arith_sat",  psubusb_ymm_lat,   psubusb_ymm_tp},
    {"PSUBUSW",    "xmm,xmm",       128, "arith_sat",  psubusw_xmm_lat,   psubusw_xmm_tp},
    {"PSUBUSW",    "ymm,ymm",       256, "arith_sat",  psubusw_ymm_lat,   psubusw_ymm_tp},
    {"PSUBSB",     "xmm,xmm",       128, "arith_sat",  psubsb_xmm_lat,    psubsb_xmm_tp},
    {"PSUBSB",     "ymm,ymm",       256, "arith_sat",  psubsb_ymm_lat,    psubsb_ymm_tp},
    {"PSUBSW",     "xmm,xmm",       128, "arith_sat",  psubsw_xmm_lat,    psubsw_xmm_tp},
    {"PSUBSW",     "ymm,ymm",       256, "arith_sat",  psubsw_ymm_lat,    psubsw_ymm_tp},

    /* --- Arithmetic: Multiply --- */
    {"PMULLW",     "xmm,xmm",       128, "arith_mul",  pmullw_xmm_lat,    pmullw_xmm_tp},
    {"PMULLW",     "ymm,ymm",       256, "arith_mul",  pmullw_ymm_lat,    pmullw_ymm_tp},
    {"PMULLD",     "xmm,xmm",       128, "arith_mul",  pmulld_xmm_lat,    pmulld_xmm_tp},
    {"PMULLD",     "ymm,ymm",       256, "arith_mul",  pmulld_ymm_lat,    pmulld_ymm_tp},
    {"PMULDQ",     "xmm,xmm",       128, "arith_mul",  pmuldq_xmm_lat,    pmuldq_xmm_tp},
    {"PMULDQ",     "ymm,ymm",       256, "arith_mul",  pmuldq_ymm_lat,    pmuldq_ymm_tp},
    {"PMULUDQ",    "xmm,xmm",       128, "arith_mul",  pmuludq_xmm_lat,   pmuludq_xmm_tp},
    {"PMULUDQ",    "ymm,ymm",       256, "arith_mul",  pmuludq_ymm_lat,   pmuludq_ymm_tp},
    {"PMULHW",     "xmm,xmm",       128, "arith_mul",  pmulhw_xmm_lat,    pmulhw_xmm_tp},
    {"PMULHW",     "ymm,ymm",       256, "arith_mul",  pmulhw_ymm_lat,    pmulhw_ymm_tp},
    {"PMULHUW",    "xmm,xmm",       128, "arith_mul",  pmulhuw_xmm_lat,   pmulhuw_xmm_tp},
    {"PMULHUW",    "ymm,ymm",       256, "arith_mul",  pmulhuw_ymm_lat,   pmulhuw_ymm_tp},
    {"PMADDWD",    "xmm,xmm",       128, "arith_mul",  pmaddwd_xmm_lat,   pmaddwd_xmm_tp},
    {"PMADDWD",    "ymm,ymm",       256, "arith_mul",  pmaddwd_ymm_lat,   pmaddwd_ymm_tp},
    {"PMADDUBSW",  "xmm,xmm",       128, "arith_mul",  pmaddubsw_xmm_lat, pmaddubsw_xmm_tp},
    {"PMADDUBSW",  "ymm,ymm",       256, "arith_mul",  pmaddubsw_ymm_lat, pmaddubsw_ymm_tp},

    /* --- Arithmetic: Horizontal --- */
    {"PHADDW",     "xmm,xmm",       128, "arith_horiz", phaddw_xmm_lat,   phaddw_xmm_tp},
    {"PHADDW",     "ymm,ymm",       256, "arith_horiz", phaddw_ymm_lat,   phaddw_ymm_tp},
    {"PHADDD",     "xmm,xmm",       128, "arith_horiz", phaddd_xmm_lat,   phaddd_xmm_tp},
    {"PHADDD",     "ymm,ymm",       256, "arith_horiz", phaddd_ymm_lat,   phaddd_ymm_tp},
    {"PHSUBW",     "xmm,xmm",       128, "arith_horiz", phsubw_xmm_lat,   phsubw_xmm_tp},
    {"PHSUBW",     "ymm,ymm",       256, "arith_horiz", phsubw_ymm_lat,   phsubw_ymm_tp},
    {"PHSUBD",     "xmm,xmm",       128, "arith_horiz", phsubd_xmm_lat,   phsubd_xmm_tp},
    {"PHSUBD",     "ymm,ymm",       256, "arith_horiz", phsubd_ymm_lat,   phsubd_ymm_tp},

    /* --- Arithmetic: SAD, AVG, ABS --- */
    {"PSADBW",     "xmm,xmm",       128, "arith_misc", psadbw_xmm_lat,    psadbw_xmm_tp},
    {"PSADBW",     "ymm,ymm",       256, "arith_misc", psadbw_ymm_lat,    psadbw_ymm_tp},
    {"PAVGB",      "xmm,xmm",       128, "arith_misc", pavgb_xmm_lat,     pavgb_xmm_tp},
    {"PAVGB",      "ymm,ymm",       256, "arith_misc", pavgb_ymm_lat,     pavgb_ymm_tp},
    {"PAVGW",      "xmm,xmm",       128, "arith_misc", pavgw_xmm_lat,     pavgw_xmm_tp},
    {"PAVGW",      "ymm,ymm",       256, "arith_misc", pavgw_ymm_lat,     pavgw_ymm_tp},
    {"PABSB",      "xmm,xmm",       128, "arith_misc", pabsb_xmm_lat,     pabsb_xmm_tp},
    {"PABSB",      "ymm,ymm",       256, "arith_misc", pabsb_ymm_lat,     pabsb_ymm_tp},
    {"PABSW",      "xmm,xmm",       128, "arith_misc", pabsw_xmm_lat,     pabsw_xmm_tp},
    {"PABSW",      "ymm,ymm",       256, "arith_misc", pabsw_ymm_lat,     pabsw_ymm_tp},
    {"PABSD",      "xmm,xmm",       128, "arith_misc", pabsd_xmm_lat,     pabsd_xmm_tp},
    {"PABSD",      "ymm,ymm",       256, "arith_misc", pabsd_ymm_lat,     pabsd_ymm_tp},

    /* --- Arithmetic: Min/Max signed --- */
    {"PMINSB",     "xmm,xmm",       128, "arith_minmax", pminsb_xmm_lat,  pminsb_xmm_tp},
    {"PMINSB",     "ymm,ymm",       256, "arith_minmax", pminsb_ymm_lat,  pminsb_ymm_tp},
    {"PMINSW",     "xmm,xmm",       128, "arith_minmax", pminsw_xmm_lat,  pminsw_xmm_tp},
    {"PMINSW",     "ymm,ymm",       256, "arith_minmax", pminsw_ymm_lat,  pminsw_ymm_tp},
    {"PMINSD",     "xmm,xmm",       128, "arith_minmax", pminsd_xmm_lat,  pminsd_xmm_tp},
    {"PMINSD",     "ymm,ymm",       256, "arith_minmax", pminsd_ymm_lat,  pminsd_ymm_tp},
    {"PMAXSB",     "xmm,xmm",       128, "arith_minmax", pmaxsb_xmm_lat,  pmaxsb_xmm_tp},
    {"PMAXSB",     "ymm,ymm",       256, "arith_minmax", pmaxsb_ymm_lat,  pmaxsb_ymm_tp},
    {"PMAXSW",     "xmm,xmm",       128, "arith_minmax", pmaxsw_xmm_lat,  pmaxsw_xmm_tp},
    {"PMAXSW",     "ymm,ymm",       256, "arith_minmax", pmaxsw_ymm_lat,  pmaxsw_ymm_tp},
    {"PMAXSD",     "xmm,xmm",       128, "arith_minmax", pmaxsd_xmm_lat,  pmaxsd_xmm_tp},
    {"PMAXSD",     "ymm,ymm",       256, "arith_minmax", pmaxsd_ymm_lat,  pmaxsd_ymm_tp},

    /* --- Arithmetic: Min/Max unsigned --- */
    {"PMINUB",     "xmm,xmm",       128, "arith_minmax", pminub_xmm_lat,  pminub_xmm_tp},
    {"PMINUB",     "ymm,ymm",       256, "arith_minmax", pminub_ymm_lat,  pminub_ymm_tp},
    {"PMINUW",     "xmm,xmm",       128, "arith_minmax", pminuw_xmm_lat,  pminuw_xmm_tp},
    {"PMINUW",     "ymm,ymm",       256, "arith_minmax", pminuw_ymm_lat,  pminuw_ymm_tp},
    {"PMINUD",     "xmm,xmm",       128, "arith_minmax", pminud_xmm_lat,  pminud_xmm_tp},
    {"PMINUD",     "ymm,ymm",       256, "arith_minmax", pminud_ymm_lat,  pminud_ymm_tp},
    {"PMAXUB",     "xmm,xmm",       128, "arith_minmax", pmaxub_xmm_lat,  pmaxub_xmm_tp},
    {"PMAXUB",     "ymm,ymm",       256, "arith_minmax", pmaxub_ymm_lat,  pmaxub_ymm_tp},
    {"PMAXUW",     "xmm,xmm",       128, "arith_minmax", pmaxuw_xmm_lat,  pmaxuw_xmm_tp},
    {"PMAXUW",     "ymm,ymm",       256, "arith_minmax", pmaxuw_ymm_lat,  pmaxuw_ymm_tp},
    {"PMAXUD",     "xmm,xmm",       128, "arith_minmax", pmaxud_xmm_lat,  pmaxud_xmm_tp},
    {"PMAXUD",     "ymm,ymm",       256, "arith_minmax", pmaxud_ymm_lat,  pmaxud_ymm_tp},

    /* --- Logic --- */
    {"PAND",       "xmm,xmm",       128, "logic",       pand_xmm_lat,     pand_xmm_tp},
    {"PAND",       "ymm,ymm",       256, "logic",       pand_ymm_lat,     pand_ymm_tp},
    {"PANDN",      "xmm,xmm",       128, "logic",       pandn_xmm_lat,    pandn_xmm_tp},
    {"PANDN",      "ymm,ymm",       256, "logic",       pandn_ymm_lat,    pandn_ymm_tp},
    {"POR",        "xmm,xmm",       128, "logic",       por_xmm_lat,      por_xmm_tp},
    {"POR",        "ymm,ymm",       256, "logic",       por_ymm_lat,      por_ymm_tp},
    {"PXOR",       "xmm,xmm",       128, "logic",       pxor_xmm_lat,     pxor_xmm_tp},
    {"PXOR",       "ymm,ymm",       256, "logic",       pxor_ymm_lat,     pxor_ymm_tp},

    /* --- Shift: Immediate --- */
    {"PSLLW",      "xmm,imm",       128, "shift_imm",   psllw_imm_xmm_lat, psllw_imm_xmm_tp},
    {"PSLLW",      "ymm,imm",       256, "shift_imm",   psllw_imm_ymm_lat, psllw_imm_ymm_tp},
    {"PSLLD",      "xmm,imm",       128, "shift_imm",   pslld_imm_xmm_lat, pslld_imm_xmm_tp},
    {"PSLLD",      "ymm,imm",       256, "shift_imm",   pslld_imm_ymm_lat, pslld_imm_ymm_tp},
    {"PSLLQ",      "xmm,imm",       128, "shift_imm",   psllq_imm_xmm_lat, psllq_imm_xmm_tp},
    {"PSLLQ",      "ymm,imm",       256, "shift_imm",   psllq_imm_ymm_lat, psllq_imm_ymm_tp},
    {"PSRLW",      "xmm,imm",       128, "shift_imm",   psrlw_imm_xmm_lat, psrlw_imm_xmm_tp},
    {"PSRLW",      "ymm,imm",       256, "shift_imm",   psrlw_imm_ymm_lat, psrlw_imm_ymm_tp},
    {"PSRLD",      "xmm,imm",       128, "shift_imm",   psrld_imm_xmm_lat, psrld_imm_xmm_tp},
    {"PSRLD",      "ymm,imm",       256, "shift_imm",   psrld_imm_ymm_lat, psrld_imm_ymm_tp},
    {"PSRLQ",      "xmm,imm",       128, "shift_imm",   psrlq_imm_xmm_lat, psrlq_imm_xmm_tp},
    {"PSRLQ",      "ymm,imm",       256, "shift_imm",   psrlq_imm_ymm_lat, psrlq_imm_ymm_tp},
    {"PSRAW",      "xmm,imm",       128, "shift_imm",   psraw_imm_xmm_lat, psraw_imm_xmm_tp},
    {"PSRAW",      "ymm,imm",       256, "shift_imm",   psraw_imm_ymm_lat, psraw_imm_ymm_tp},
    {"PSRAD",      "xmm,imm",       128, "shift_imm",   psrad_imm_xmm_lat, psrad_imm_xmm_tp},
    {"PSRAD",      "ymm,imm",       256, "shift_imm",   psrad_imm_ymm_lat, psrad_imm_ymm_tp},

    /* --- Shift: Register --- */
    {"PSLLW",      "xmm,xmm(cnt)",  128, "shift_reg",   psllw_reg_xmm_lat, psllw_reg_xmm_tp},
    {"PSLLW",      "ymm,xmm(cnt)",  256, "shift_reg",   psllw_reg_ymm_lat, psllw_reg_ymm_tp},
    {"PSLLD",      "xmm,xmm(cnt)",  128, "shift_reg",   pslld_reg_xmm_lat, pslld_reg_xmm_tp},
    {"PSLLQ",      "xmm,xmm(cnt)",  128, "shift_reg",   psllq_reg_xmm_lat, psllq_reg_xmm_tp},
    {"PSRLW",      "xmm,xmm(cnt)",  128, "shift_reg",   psrlw_reg_xmm_lat, psrlw_reg_xmm_tp},
    {"PSRLD",      "xmm,xmm(cnt)",  128, "shift_reg",   psrld_reg_xmm_lat, psrld_reg_xmm_tp},
    {"PSRLQ",      "xmm,xmm(cnt)",  128, "shift_reg",   psrlq_reg_xmm_lat, psrlq_reg_xmm_tp},
    {"PSRAW",      "xmm,xmm(cnt)",  128, "shift_reg",   psraw_reg_xmm_lat, psraw_reg_xmm_tp},
    {"PSRAD",      "xmm,xmm(cnt)",  128, "shift_reg",   psrad_reg_xmm_lat, psrad_reg_xmm_tp},

    /* --- Shift: Variable (AVX2) --- */
    {"VPSLLVD",    "xmm,xmm,xmm",  128, "shift_var",   vpsllvd_xmm_lat,  vpsllvd_xmm_tp},
    {"VPSLLVD",    "ymm,ymm,ymm",  256, "shift_var",   vpsllvd_ymm_lat,  vpsllvd_ymm_tp},
    {"VPSLLVQ",    "xmm,xmm,xmm",  128, "shift_var",   vpsllvq_xmm_lat,  vpsllvq_xmm_tp},
    {"VPSLLVQ",    "ymm,ymm,ymm",  256, "shift_var",   vpsllvq_ymm_lat,  vpsllvq_ymm_tp},
    {"VPSRLVD",    "xmm,xmm,xmm",  128, "shift_var",   vpsrlvd_xmm_lat,  vpsrlvd_xmm_tp},
    {"VPSRLVD",    "ymm,ymm,ymm",  256, "shift_var",   vpsrlvd_ymm_lat,  vpsrlvd_ymm_tp},
    {"VPSRLVQ",    "xmm,xmm,xmm",  128, "shift_var",   vpsrlvq_xmm_lat,  vpsrlvq_xmm_tp},
    {"VPSRLVQ",    "ymm,ymm,ymm",  256, "shift_var",   vpsrlvq_ymm_lat,  vpsrlvq_ymm_tp},
    {"VPSRAVD",    "xmm,xmm,xmm",  128, "shift_var",   vpsravd_xmm_lat,  vpsravd_xmm_tp},
    {"VPSRAVD",    "ymm,ymm,ymm",  256, "shift_var",   vpsravd_ymm_lat,  vpsravd_ymm_tp},

    /* --- Compare --- */
    {"PCMPEQB",    "xmm,xmm",       128, "compare",     pcmpeqb_xmm_lat,  pcmpeqb_xmm_tp},
    {"PCMPEQB",    "ymm,ymm",       256, "compare",     pcmpeqb_ymm_lat,  pcmpeqb_ymm_tp},
    {"PCMPEQW",    "xmm,xmm",       128, "compare",     pcmpeqw_xmm_lat,  pcmpeqw_xmm_tp},
    {"PCMPEQW",    "ymm,ymm",       256, "compare",     pcmpeqw_ymm_lat,  pcmpeqw_ymm_tp},
    {"PCMPEQD",    "xmm,xmm",       128, "compare",     pcmpeqd_xmm_lat,  pcmpeqd_xmm_tp},
    {"PCMPEQD",    "ymm,ymm",       256, "compare",     pcmpeqd_ymm_lat,  pcmpeqd_ymm_tp},
    {"PCMPEQQ",    "xmm,xmm",       128, "compare",     pcmpeqq_xmm_lat,  pcmpeqq_xmm_tp},
    {"PCMPEQQ",    "ymm,ymm",       256, "compare",     pcmpeqq_ymm_lat,  pcmpeqq_ymm_tp},
    {"PCMPGTB",    "xmm,xmm",       128, "compare",     pcmpgtb_xmm_lat,  pcmpgtb_xmm_tp},
    {"PCMPGTB",    "ymm,ymm",       256, "compare",     pcmpgtb_ymm_lat,  pcmpgtb_ymm_tp},
    {"PCMPGTW",    "xmm,xmm",       128, "compare",     pcmpgtw_xmm_lat,  pcmpgtw_xmm_tp},
    {"PCMPGTW",    "ymm,ymm",       256, "compare",     pcmpgtw_ymm_lat,  pcmpgtw_ymm_tp},
    {"PCMPGTD",    "xmm,xmm",       128, "compare",     pcmpgtd_xmm_lat,  pcmpgtd_xmm_tp},
    {"PCMPGTD",    "ymm,ymm",       256, "compare",     pcmpgtd_ymm_lat,  pcmpgtd_ymm_tp},
    {"PCMPGTQ",    "xmm,xmm",       128, "compare",     pcmpgtq_xmm_lat,  pcmpgtq_xmm_tp},
    {"PCMPGTQ",    "ymm,ymm",       256, "compare",     pcmpgtq_ymm_lat,  pcmpgtq_ymm_tp},

    /* --- Shuffle/Permute --- */
    {"PSHUFB",     "xmm,xmm",       128, "shuffle",     pshufb_xmm_lat,    pshufb_xmm_tp},
    {"PSHUFB",     "ymm,ymm",       256, "shuffle",     pshufb_ymm_lat,    pshufb_ymm_tp},
    {"PSHUFD",     "xmm,xmm,imm",   128, "shuffle",     pshufd_xmm_lat,    pshufd_xmm_tp},
    {"PSHUFD",     "ymm,ymm,imm",   256, "shuffle",     pshufd_ymm_lat,    pshufd_ymm_tp},
    {"PSHUFHW",    "xmm,xmm,imm",   128, "shuffle",     pshufhw_xmm_lat,   pshufhw_xmm_tp},
    {"PSHUFHW",    "ymm,ymm,imm",   256, "shuffle",     pshufhw_ymm_lat,   pshufhw_ymm_tp},
    {"PSHUFLW",    "xmm,xmm,imm",   128, "shuffle",     pshuflw_xmm_lat,   pshuflw_xmm_tp},
    {"PSHUFLW",    "ymm,ymm,imm",   256, "shuffle",     pshuflw_ymm_lat,   pshuflw_ymm_tp},
    {"VPERMD",     "ymm,ymm,ymm",   256, "shuffle",     vpermd_ymm_lat,    vpermd_ymm_tp},
    {"VPERM2I128", "ymm,ymm,ymm,imm", 256, "shuffle",   vperm2i128_ymm_lat, vperm2i128_ymm_tp},
    {"PALIGNR",    "xmm,xmm,imm",   128, "shuffle",     palignr_xmm_lat,   palignr_xmm_tp},
    {"PALIGNR",    "ymm,ymm,imm",   256, "shuffle",     palignr_ymm_lat,   palignr_ymm_tp},

    /* --- Unpack --- */
    {"PUNPCKLBW",  "xmm,xmm",       128, "unpack",      punpcklbw_xmm_lat, punpcklbw_xmm_tp},
    {"PUNPCKLBW",  "ymm,ymm",       256, "unpack",      punpcklbw_ymm_lat, punpcklbw_ymm_tp},
    {"PUNPCKHBW",  "xmm,xmm",       128, "unpack",      punpckhbw_xmm_lat, punpckhbw_xmm_tp},
    {"PUNPCKHBW",  "ymm,ymm",       256, "unpack",      punpckhbw_ymm_lat, punpckhbw_ymm_tp},
    {"PUNPCKLWD",  "xmm,xmm",       128, "unpack",      punpcklwd_xmm_lat, punpcklwd_xmm_tp},
    {"PUNPCKLWD",  "ymm,ymm",       256, "unpack",      punpcklwd_ymm_lat, punpcklwd_ymm_tp},
    {"PUNPCKHWD",  "xmm,xmm",       128, "unpack",      punpckhwd_xmm_lat, punpckhwd_xmm_tp},
    {"PUNPCKHWD",  "ymm,ymm",       256, "unpack",      punpckhwd_ymm_lat, punpckhwd_ymm_tp},
    {"PUNPCKLDQ",  "xmm,xmm",       128, "unpack",      punpckldq_xmm_lat, punpckldq_xmm_tp},
    {"PUNPCKLDQ",  "ymm,ymm",       256, "unpack",      punpckldq_ymm_lat, punpckldq_ymm_tp},
    {"PUNPCKHDQ",  "xmm,xmm",       128, "unpack",      punpckhdq_xmm_lat, punpckhdq_xmm_tp},
    {"PUNPCKHDQ",  "ymm,ymm",       256, "unpack",      punpckhdq_ymm_lat, punpckhdq_ymm_tp},
    {"PUNPCKLQDQ", "xmm,xmm",       128, "unpack",      punpcklqdq_xmm_lat, punpcklqdq_xmm_tp},
    {"PUNPCKLQDQ", "ymm,ymm",       256, "unpack",      punpcklqdq_ymm_lat, punpcklqdq_ymm_tp},
    {"PUNPCKHQDQ", "xmm,xmm",       128, "unpack",      punpckhqdq_xmm_lat, punpckhqdq_xmm_tp},
    {"PUNPCKHQDQ", "ymm,ymm",       256, "unpack",      punpckhqdq_ymm_lat, punpckhqdq_ymm_tp},

    /* --- Pack --- */
    {"PACKUSWB",   "xmm,xmm",       128, "pack",        packuswb_xmm_lat,  packuswb_xmm_tp},
    {"PACKUSWB",   "ymm,ymm",       256, "pack",        packuswb_ymm_lat,  packuswb_ymm_tp},
    {"PACKSSWB",   "xmm,xmm",       128, "pack",        packsswb_xmm_lat,  packsswb_xmm_tp},
    {"PACKSSWB",   "ymm,ymm",       256, "pack",        packsswb_ymm_lat,  packsswb_ymm_tp},
    {"PACKUSDW",   "xmm,xmm",       128, "pack",        packusdw_xmm_lat,  packusdw_xmm_tp},
    {"PACKUSDW",   "ymm,ymm",       256, "pack",        packusdw_ymm_lat,  packusdw_ymm_tp},
    {"PACKSSDW",   "xmm,xmm",       128, "pack",        packssdw_xmm_lat,  packssdw_xmm_tp},
    {"PACKSSDW",   "ymm,ymm",       256, "pack",        packssdw_ymm_lat,  packssdw_ymm_tp},

    /* --- Move / Extract / Insert / Broadcast --- */
    {"MOVD",       "xmm<->r32",     128, "move",        movd_xmm_lat,     movd_xmm_tp},
    {"MOVQ",       "xmm<->r64",     128, "move",        movq_xmm_lat,     NULL},
    {"PEXTRB",     "r32,xmm,imm",   128, "move",        NULL,              pextrb_xmm_tp},
    {"PEXTRW",     "r32,xmm,imm",   128, "move",        NULL,              pextrw_xmm_tp},
    {"PEXTRD",     "r32,xmm,imm",   128, "move",        NULL,              pextrd_xmm_tp},
    {"PEXTRQ",     "r64,xmm,imm",   128, "move",        NULL,              pextrq_xmm_tp},
    {"VPBROADCASTD","xmm",          128, "move",        NULL,              vpbroadcastd_xmm_tp},
    {"VPBROADCASTD","ymm",          256, "move",        NULL,              vpbroadcastd_ymm_tp},
    {"VPBROADCASTQ","xmm",          128, "move",        NULL,              vpbroadcastq_xmm_tp},
    {"VPBROADCASTQ","ymm",          256, "move",        NULL,              vpbroadcastq_ymm_tp},

    /* --- Gather --- */
    {"VPGATHERDD", "xmm,[base+xmm*4]", 128, "gather",  NULL,              vpgatherdd_xmm_tp},
    {"VPGATHERDD", "ymm,[base+ymm*4]", 256, "gather",  NULL,              vpgatherdd_ymm_tp},
    {"VPGATHERDQ", "xmm,[base+xmm*8]", 128, "gather",  NULL,              vpgatherdq_xmm_tp},
    {"VPGATHERDQ", "ymm,[base+xmm*8]", 256, "gather",  NULL,              vpgatherdq_ymm_tp},

    /* --- Conversion: PMOVZX --- */
    {"PMOVZXBW",   "xmm,xmm",       128, "convert",     pmovzxbw_xmm_lat, pmovzxbw_xmm_tp},
    {"PMOVZXBW",   "ymm,xmm",       256, "convert",     pmovzxbw_ymm_lat, pmovzxbw_ymm_tp},
    {"PMOVZXBD",   "xmm,xmm",       128, "convert",     pmovzxbd_xmm_lat, pmovzxbd_xmm_tp},
    {"PMOVZXBQ",   "xmm,xmm",       128, "convert",     pmovzxbq_xmm_lat, pmovzxbq_xmm_tp},
    {"PMOVZXWD",   "xmm,xmm",       128, "convert",     pmovzxwd_xmm_lat, pmovzxwd_xmm_tp},
    {"PMOVZXWD",   "ymm,xmm",       256, "convert",     pmovzxwd_ymm_lat, pmovzxwd_ymm_tp},
    {"PMOVZXWQ",   "xmm,xmm",       128, "convert",     pmovzxwq_xmm_lat, pmovzxwq_xmm_tp},
    {"PMOVZXDQ",   "xmm,xmm",       128, "convert",     pmovzxdq_xmm_lat, pmovzxdq_xmm_tp},
    {"PMOVZXDQ",   "ymm,xmm",       256, "convert",     pmovzxdq_ymm_lat, pmovzxdq_ymm_tp},

    /* --- Conversion: PMOVSX --- */
    {"PMOVSXBW",   "xmm,xmm",       128, "convert",     pmovsxbw_xmm_lat, pmovsxbw_xmm_tp},
    {"PMOVSXBW",   "ymm,xmm",       256, "convert",     pmovsxbw_ymm_lat, pmovsxbw_ymm_tp},
    {"PMOVSXBD",   "xmm,xmm",       128, "convert",     pmovsxbd_xmm_lat, pmovsxbd_xmm_tp},
    {"PMOVSXBQ",   "xmm,xmm",       128, "convert",     pmovsxbq_xmm_lat, pmovsxbq_xmm_tp},
    {"PMOVSXWD",   "xmm,xmm",       128, "convert",     pmovsxwd_xmm_lat, pmovsxwd_xmm_tp},
    {"PMOVSXWD",   "ymm,xmm",       256, "convert",     pmovsxwd_ymm_lat, pmovsxwd_ymm_tp},
    {"PMOVSXWQ",   "xmm,xmm",       128, "convert",     pmovsxwq_xmm_lat, pmovsxwq_xmm_tp},
    {"PMOVSXDQ",   "xmm,xmm",       128, "convert",     pmovsxdq_xmm_lat, pmovsxdq_xmm_tp},
    {"PMOVSXDQ",   "ymm,xmm",       256, "convert",     pmovsxdq_ymm_lat, pmovsxdq_ymm_tp},

    {NULL, NULL, 0, NULL, NULL, NULL}
};

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 30000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    int csv_mode = probe_config.csv;

    if (!csv_mode) {
        sm_zen_print_header("simd_int_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring SIMD integer instruction forms (XMM + YMM)...\n\n");
    }

    /* CSV header */
    printf("mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const SimdIntProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const SimdIntProbeEntry *current_entry = &probe_table[entry_idx];

        double measured_latency = -1.0;
        double measured_throughput = -1.0;

        if (current_entry->measure_latency)
            measured_latency = current_entry->measure_latency(probe_config.iterations);
        if (current_entry->measure_throughput)
            measured_throughput = current_entry->measure_throughput(probe_config.iterations);

        printf("%s,%s,%d,%.4f,%.4f,%s\n",
               current_entry->mnemonic,
               current_entry->operand_description,
               current_entry->width_bits,
               measured_latency,
               measured_throughput,
               current_entry->instruction_category);
    }

    return 0;
}
