#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_avx512_int_exhaustive.c -- Exhaustive AVX-512 ZMM integer instruction
 * measurement for Zen 4.
 *
 * Measures latency (4-deep dependency chain) and throughput (8 independent
 * accumulators) for every AVX-512 integer instruction form at ZMM (512-bit)
 * width.
 *
 * Output: one CSV line per instruction form:
 *   mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx512f -mavx512bw -mavx512dq \
 *     -mavx512vl -mavx512cd -mavx512vbmi -mavx512vbmi2 -mavx512vnni         \
 *     -mavx512ifma -mavx512bf16 -mavx512bitalg -mavx512vpopcntdq -mfma      \
 *     -mgfni -mvaes -mvpclmulqdq -fno-omit-frame-pointer -include x86intrin.h \
 *     -I. common.c probe_avx512_int_exhaustive.c -lm                         \
 *     -o ../../../../build/bin/zen_probe_avx512_int_exhaustive
 */

/* ========================================================================
 * MACRO TEMPLATES -- ZMM (512-bit)
 * ======================================================================== */

/*
 * ZMM_LAT_BINARY: latency for binary zmm op: acc = op(acc, source).
 *   4-deep dependency chain per iteration.
 */
#define ZMM_LAT_BINARY(func_name, intrinsic, init_val)                         \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_operand = init_val;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
        accumulator = intrinsic(accumulator, source_operand);                  \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

/*
 * ZMM_TP_BINARY: throughput for binary zmm op with 8 independent streams.
 */
#define ZMM_TP_BINARY(func_name, intrinsic, init_fn, init_arg)                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m512i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m512i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m512i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m512i source_operand = init_fn(init_arg);                                \
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
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) + \
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM_LAT_UNARY: latency for unary zmm op (e.g. vpabs, vplzcnt).
 */
#define ZMM_LAT_UNARY(func_name, intrinsic, init_val)                          \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
        accumulator = intrinsic(accumulator);                                  \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_TP_UNARY(func_name, intrinsic, init_fn, init_arg)                  \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m512i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m512i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m512i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
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
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) + \
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM_LAT_SHIFT_IMM / ZMM_TP_SHIFT_IMM: shift-by-immediate.
 */
#define ZMM_LAT_SHIFT_IMM(func_name, intrinsic, init_val, shift_amount)        \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
        accumulator = intrinsic(accumulator, shift_amount);                    \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_TP_SHIFT_IMM(func_name, intrinsic, init_fn, init_arg, shift_amount) \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m512i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m512i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m512i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
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
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) + \
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM_LAT_SHUF_IMM / ZMM_TP_SHUF_IMM: shuffle/permute with single immediate.
 */
#define ZMM_LAT_SHUF_IMM(func_name, intrinsic, init_val, imm)                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
        accumulator = intrinsic(accumulator, imm);                             \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_TP_SHUF_IMM(func_name, intrinsic, init_fn, init_arg, imm)         \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m512i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m512i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m512i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
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
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) + \
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM_LAT_BINARY_IMM / ZMM_TP_BINARY_IMM: binary with immediate (e.g. valignd, vpternlogd).
 */
#define ZMM_LAT_BINARY_IMM(func_name, intrinsic, init_val, imm)               \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_operand = init_val;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
        accumulator = intrinsic(accumulator, source_operand, imm);             \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_TP_BINARY_IMM(func_name, intrinsic, init_fn, init_arg, imm)       \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m512i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m512i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m512i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m512i source_operand = init_fn(init_arg);                                \
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
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) + \
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM_LAT_TERNARY / ZMM_TP_TERNARY: ternary op where accumulator is both
 * first source and destination: acc = op(acc, src1, src2).
 * Used for VPTERNLOGD/Q, VPDPBUSD, VPMADD52LUQ, VPSHLDV, etc.
 */
#define ZMM_LAT_TERNARY(func_name, intrinsic, init_val)                        \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_a = init_val;                                               \
    __m512i source_b = init_val;                                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_a, source_b);              \
        accumulator = intrinsic(accumulator, source_a, source_b);              \
        accumulator = intrinsic(accumulator, source_a, source_b);              \
        accumulator = intrinsic(accumulator, source_a, source_b);              \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_TP_TERNARY(func_name, intrinsic, init_fn, init_arg)                \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m512i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m512i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m512i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m512i source_a = init_fn(init_arg+10);                                   \
    __m512i source_b = init_fn(init_arg+20);                                   \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, source_a, source_b);                          \
        acc_1 = intrinsic(acc_1, source_a, source_b);                          \
        acc_2 = intrinsic(acc_2, source_a, source_b);                          \
        acc_3 = intrinsic(acc_3, source_a, source_b);                          \
        acc_4 = intrinsic(acc_4, source_a, source_b);                          \
        acc_5 = intrinsic(acc_5, source_a, source_b);                          \
        acc_6 = intrinsic(acc_6, source_a, source_b);                          \
        acc_7 = intrinsic(acc_7, source_a, source_b);                          \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) + \
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM_LAT_TERNARY_IMM / ZMM_TP_TERNARY_IMM: ternary op with immediate.
 * acc = op(acc, src, imm).  Used for VPTERNLOGD/Q.
 */
#define ZMM_LAT_TERNARY_IMM(func_name, intrinsic, init_val, imm)              \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_a = init_val;                                               \
    __m512i source_b = init_val;                                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        accumulator = intrinsic(accumulator, source_a, source_b, imm);         \
        accumulator = intrinsic(accumulator, source_a, source_b, imm);         \
        accumulator = intrinsic(accumulator, source_a, source_b, imm);         \
        accumulator = intrinsic(accumulator, source_a, source_b, imm);         \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_TP_TERNARY_IMM(func_name, intrinsic, init_fn, init_arg, imm)      \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i acc_0 = init_fn(init_arg+1), acc_1 = init_fn(init_arg+2);         \
    __m512i acc_2 = init_fn(init_arg+3), acc_3 = init_fn(init_arg+4);         \
    __m512i acc_4 = init_fn(init_arg+5), acc_5 = init_fn(init_arg+6);         \
    __m512i acc_6 = init_fn(init_arg+7), acc_7 = init_fn(init_arg+8);         \
    __m512i source_a = init_fn(init_arg+10);                                   \
    __m512i source_b = init_fn(init_arg+20);                                   \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        acc_0 = intrinsic(acc_0, source_a, source_b, imm);                     \
        acc_1 = intrinsic(acc_1, source_a, source_b, imm);                     \
        acc_2 = intrinsic(acc_2, source_a, source_b, imm);                     \
        acc_3 = intrinsic(acc_3, source_a, source_b, imm);                     \
        acc_4 = intrinsic(acc_4, source_a, source_b, imm);                     \
        acc_5 = intrinsic(acc_5, source_a, source_b, imm);                     \
        acc_6 = intrinsic(acc_6, source_a, source_b, imm);                     \
        acc_7 = intrinsic(acc_7, source_a, source_b, imm);                     \
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),\
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) + \
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM widening conversion: input is __m256i, output is __m512i.
 * Chain: widen -> truncate-back-to-ymm -> widen -> ...
 */
#define ZMM_WIDEN_LAT(func_name, widen_intrinsic, init_val)                    \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i narrow_accumulator = init_val;                                     \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __m512i wide_result = widen_intrinsic(narrow_accumulator);             \
        narrow_accumulator = _mm512_castsi512_si256(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm512_castsi512_si256(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm512_castsi512_si256(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm512_castsi512_si256(wide_result);              \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(narrow_accumulator, 0);     \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_WIDEN_TP(func_name, widen_intrinsic, init_fn, init_arg)            \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m256i src_0 = init_fn(init_arg+1), src_1 = init_fn(init_arg+2);         \
    __m256i src_2 = init_fn(init_arg+3), src_3 = init_fn(init_arg+4);         \
    __m256i src_4 = init_fn(init_arg+5), src_5 = init_fn(init_arg+6);         \
    __m256i src_6 = init_fn(init_arg+7), src_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __m512i d0 = widen_intrinsic(src_0);                                   \
        __m512i d1 = widen_intrinsic(src_1);                                   \
        __m512i d2 = widen_intrinsic(src_2);                                   \
        __m512i d3 = widen_intrinsic(src_3);                                   \
        __m512i d4 = widen_intrinsic(src_4);                                   \
        __m512i d5 = widen_intrinsic(src_5);                                   \
        __m512i d6 = widen_intrinsic(src_6);                                   \
        __m512i d7 = widen_intrinsic(src_7);                                   \
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),           \
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));           \
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),\
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm256_extract_epi32(src_0, 0); (void)sink_value;\
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * ZMM_WIDEN_XMM_LAT / ZMM_WIDEN_XMM_TP: widening from __m128i to __m512i.
 * For conversions like bq, wq, bd where the input is xmm-width.
 */
#define ZMM_WIDEN_XMM_LAT(func_name, widen_intrinsic, init_val)                \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i narrow_accumulator = init_val;                                     \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __m512i wide_result = widen_intrinsic(narrow_accumulator);             \
        narrow_accumulator = _mm512_castsi512_si128(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm512_castsi512_si128(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm512_castsi512_si128(wide_result);              \
        wide_result = widen_intrinsic(narrow_accumulator);                     \
        narrow_accumulator = _mm512_castsi512_si128(wide_result);              \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(narrow_accumulator, 0);        \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_WIDEN_XMM_TP(func_name, widen_intrinsic, init_fn, init_arg)        \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m128i src_0 = init_fn(init_arg+1), src_1 = init_fn(init_arg+2);         \
    __m128i src_2 = init_fn(init_arg+3), src_3 = init_fn(init_arg+4);         \
    __m128i src_4 = init_fn(init_arg+5), src_5 = init_fn(init_arg+6);         \
    __m128i src_6 = init_fn(init_arg+7), src_7 = init_fn(init_arg+8);         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __m512i d0 = widen_intrinsic(src_0);                                   \
        __m512i d1 = widen_intrinsic(src_1);                                   \
        __m512i d2 = widen_intrinsic(src_2);                                   \
        __m512i d3 = widen_intrinsic(src_3);                                   \
        __m512i d4 = widen_intrinsic(src_4);                                   \
        __m512i d5 = widen_intrinsic(src_5);                                   \
        __m512i d6 = widen_intrinsic(src_6);                                   \
        __m512i d7 = widen_intrinsic(src_7);                                   \
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),           \
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));           \
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),\
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));\
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(src_0, 0); (void)sink_value;   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* ========================================================================
 * ARITHMETIC -- ADD (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpaddb_zmm_lat, _mm512_add_epi8, _mm512_set1_epi8(1))
ZMM_TP_BINARY(vpaddb_zmm_tp, _mm512_add_epi8, _mm512_set1_epi8, 1)

ZMM_LAT_BINARY(vpaddw_zmm_lat, _mm512_add_epi16, _mm512_set1_epi16(1))
ZMM_TP_BINARY(vpaddw_zmm_tp, _mm512_add_epi16, _mm512_set1_epi16, 1)

ZMM_LAT_BINARY(vpaddd_zmm_lat, _mm512_add_epi32, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpaddd_zmm_tp, _mm512_add_epi32, _mm512_set1_epi32, 1)

ZMM_LAT_BINARY(vpaddq_zmm_lat, _mm512_add_epi64, _mm512_set1_epi64(1))
ZMM_TP_BINARY(vpaddq_zmm_tp, _mm512_add_epi64, _mm512_set1_epi64, 1)

/* ========================================================================
 * ARITHMETIC -- SUB (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpsubb_zmm_lat, _mm512_sub_epi8, _mm512_set1_epi8(100))
ZMM_TP_BINARY(vpsubb_zmm_tp, _mm512_sub_epi8, _mm512_set1_epi8, 1)

ZMM_LAT_BINARY(vpsubw_zmm_lat, _mm512_sub_epi16, _mm512_set1_epi16(100))
ZMM_TP_BINARY(vpsubw_zmm_tp, _mm512_sub_epi16, _mm512_set1_epi16, 1)

ZMM_LAT_BINARY(vpsubd_zmm_lat, _mm512_sub_epi32, _mm512_set1_epi32(100))
ZMM_TP_BINARY(vpsubd_zmm_tp, _mm512_sub_epi32, _mm512_set1_epi32, 1)

ZMM_LAT_BINARY(vpsubq_zmm_lat, _mm512_sub_epi64, _mm512_set1_epi64(100))
ZMM_TP_BINARY(vpsubq_zmm_tp, _mm512_sub_epi64, _mm512_set1_epi64, 1)

/* ========================================================================
 * ARITHMETIC -- SATURATING ADD/SUB (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpaddusb_zmm_lat, _mm512_adds_epu8, _mm512_set1_epi8(1))
ZMM_TP_BINARY(vpaddusb_zmm_tp, _mm512_adds_epu8, _mm512_set1_epi8, 1)

ZMM_LAT_BINARY(vpaddusw_zmm_lat, _mm512_adds_epu16, _mm512_set1_epi16(1))
ZMM_TP_BINARY(vpaddusw_zmm_tp, _mm512_adds_epu16, _mm512_set1_epi16, 1)

ZMM_LAT_BINARY(vpaddsb_zmm_lat, _mm512_adds_epi8, _mm512_set1_epi8(1))
ZMM_TP_BINARY(vpaddsb_zmm_tp, _mm512_adds_epi8, _mm512_set1_epi8, 1)

ZMM_LAT_BINARY(vpaddsw_zmm_lat, _mm512_adds_epi16, _mm512_set1_epi16(1))
ZMM_TP_BINARY(vpaddsw_zmm_tp, _mm512_adds_epi16, _mm512_set1_epi16, 1)

ZMM_LAT_BINARY(vpsubusb_zmm_lat, _mm512_subs_epu8, _mm512_set1_epi8(100))
ZMM_TP_BINARY(vpsubusb_zmm_tp, _mm512_subs_epu8, _mm512_set1_epi8, 50)

ZMM_LAT_BINARY(vpsubusw_zmm_lat, _mm512_subs_epu16, _mm512_set1_epi16(100))
ZMM_TP_BINARY(vpsubusw_zmm_tp, _mm512_subs_epu16, _mm512_set1_epi16, 50)

ZMM_LAT_BINARY(vpsubsb_zmm_lat, _mm512_subs_epi8, _mm512_set1_epi8(100))
ZMM_TP_BINARY(vpsubsb_zmm_tp, _mm512_subs_epi8, _mm512_set1_epi8, 50)

ZMM_LAT_BINARY(vpsubsw_zmm_lat, _mm512_subs_epi16, _mm512_set1_epi16(100))
ZMM_TP_BINARY(vpsubsw_zmm_tp, _mm512_subs_epi16, _mm512_set1_epi16, 50)

/* ========================================================================
 * ARITHMETIC -- MULTIPLY (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpmullw_zmm_lat, _mm512_mullo_epi16, _mm512_set1_epi16(3))
ZMM_TP_BINARY(vpmullw_zmm_tp, _mm512_mullo_epi16, _mm512_set1_epi16, 3)

ZMM_LAT_BINARY(vpmulld_zmm_lat, _mm512_mullo_epi32, _mm512_set1_epi32(3))
ZMM_TP_BINARY(vpmulld_zmm_tp, _mm512_mullo_epi32, _mm512_set1_epi32, 3)

/* VPMULLQ zmm -- AVX512DQ extension */
ZMM_LAT_BINARY(vpmullq_zmm_lat, _mm512_mullo_epi64, _mm512_set1_epi64(3))
ZMM_TP_BINARY(vpmullq_zmm_tp, _mm512_mullo_epi64, _mm512_set1_epi64, 3)

ZMM_LAT_BINARY(vpmuldq_zmm_lat, _mm512_mul_epi32, _mm512_set1_epi32(3))
ZMM_TP_BINARY(vpmuldq_zmm_tp, _mm512_mul_epi32, _mm512_set1_epi32, 3)

ZMM_LAT_BINARY(vpmuludq_zmm_lat, _mm512_mul_epu32, _mm512_set1_epi32(3))
ZMM_TP_BINARY(vpmuludq_zmm_tp, _mm512_mul_epu32, _mm512_set1_epi32, 3)

ZMM_LAT_BINARY(vpmulhw_zmm_lat, _mm512_mulhi_epi16, _mm512_set1_epi16(3))
ZMM_TP_BINARY(vpmulhw_zmm_tp, _mm512_mulhi_epi16, _mm512_set1_epi16, 3)

ZMM_LAT_BINARY(vpmulhuw_zmm_lat, _mm512_mulhi_epu16, _mm512_set1_epi16(3))
ZMM_TP_BINARY(vpmulhuw_zmm_tp, _mm512_mulhi_epu16, _mm512_set1_epi16, 3)

/* VPMADDWD zmm */
ZMM_LAT_BINARY(vpmaddwd_zmm_lat, _mm512_madd_epi16, _mm512_set1_epi16(1))
ZMM_TP_BINARY(vpmaddwd_zmm_tp, _mm512_madd_epi16, _mm512_set1_epi16, 1)

/* VPMADDUBSW zmm */
ZMM_LAT_BINARY(vpmaddubsw_zmm_lat, _mm512_maddubs_epi16, _mm512_set1_epi8(1))
ZMM_TP_BINARY(vpmaddubsw_zmm_tp, _mm512_maddubs_epi16, _mm512_set1_epi8, 1)

/* VPSADBW zmm */
ZMM_LAT_BINARY(vpsadbw_zmm_lat, _mm512_sad_epu8, _mm512_set1_epi8(1))
ZMM_TP_BINARY(vpsadbw_zmm_tp, _mm512_sad_epu8, _mm512_set1_epi8, 1)

/* VPAVGB/W zmm */
ZMM_LAT_BINARY(vpavgb_zmm_lat, _mm512_avg_epu8, _mm512_set1_epi8(50))
ZMM_TP_BINARY(vpavgb_zmm_tp, _mm512_avg_epu8, _mm512_set1_epi8, 50)

ZMM_LAT_BINARY(vpavgw_zmm_lat, _mm512_avg_epu16, _mm512_set1_epi16(50))
ZMM_TP_BINARY(vpavgw_zmm_tp, _mm512_avg_epu16, _mm512_set1_epi16, 50)

/* VPABSB/W/D/Q zmm */
ZMM_LAT_UNARY(vpabsb_zmm_lat, _mm512_abs_epi8, _mm512_set1_epi8(-5))
ZMM_TP_UNARY(vpabsb_zmm_tp, _mm512_abs_epi8, _mm512_set1_epi8, -5)

ZMM_LAT_UNARY(vpabsw_zmm_lat, _mm512_abs_epi16, _mm512_set1_epi16(-5))
ZMM_TP_UNARY(vpabsw_zmm_tp, _mm512_abs_epi16, _mm512_set1_epi16, -5)

ZMM_LAT_UNARY(vpabsd_zmm_lat, _mm512_abs_epi32, _mm512_set1_epi32(-5))
ZMM_TP_UNARY(vpabsd_zmm_tp, _mm512_abs_epi32, _mm512_set1_epi32, -5)

ZMM_LAT_UNARY(vpabsq_zmm_lat, _mm512_abs_epi64, _mm512_set1_epi64(-5))
ZMM_TP_UNARY(vpabsq_zmm_tp, _mm512_abs_epi64, _mm512_set1_epi64, -5)

/* ========================================================================
 * MIN/MAX SIGNED (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpminsb_zmm_lat, _mm512_min_epi8, _mm512_set1_epi8(50))
ZMM_TP_BINARY(vpminsb_zmm_tp, _mm512_min_epi8, _mm512_set1_epi8, 50)

ZMM_LAT_BINARY(vpminsw_zmm_lat, _mm512_min_epi16, _mm512_set1_epi16(50))
ZMM_TP_BINARY(vpminsw_zmm_tp, _mm512_min_epi16, _mm512_set1_epi16, 50)

ZMM_LAT_BINARY(vpminsd_zmm_lat, _mm512_min_epi32, _mm512_set1_epi32(50))
ZMM_TP_BINARY(vpminsd_zmm_tp, _mm512_min_epi32, _mm512_set1_epi32, 50)

ZMM_LAT_BINARY(vpminsq_zmm_lat, _mm512_min_epi64, _mm512_set1_epi64(50))
ZMM_TP_BINARY(vpminsq_zmm_tp, _mm512_min_epi64, _mm512_set1_epi64, 50)

ZMM_LAT_BINARY(vpmaxsb_zmm_lat, _mm512_max_epi8, _mm512_set1_epi8(50))
ZMM_TP_BINARY(vpmaxsb_zmm_tp, _mm512_max_epi8, _mm512_set1_epi8, 50)

ZMM_LAT_BINARY(vpmaxsw_zmm_lat, _mm512_max_epi16, _mm512_set1_epi16(50))
ZMM_TP_BINARY(vpmaxsw_zmm_tp, _mm512_max_epi16, _mm512_set1_epi16, 50)

ZMM_LAT_BINARY(vpmaxsd_zmm_lat, _mm512_max_epi32, _mm512_set1_epi32(50))
ZMM_TP_BINARY(vpmaxsd_zmm_tp, _mm512_max_epi32, _mm512_set1_epi32, 50)

ZMM_LAT_BINARY(vpmaxsq_zmm_lat, _mm512_max_epi64, _mm512_set1_epi64(50))
ZMM_TP_BINARY(vpmaxsq_zmm_tp, _mm512_max_epi64, _mm512_set1_epi64, 50)

/* ========================================================================
 * MIN/MAX UNSIGNED (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpminub_zmm_lat, _mm512_min_epu8, _mm512_set1_epi8(50))
ZMM_TP_BINARY(vpminub_zmm_tp, _mm512_min_epu8, _mm512_set1_epi8, 50)

ZMM_LAT_BINARY(vpminuw_zmm_lat, _mm512_min_epu16, _mm512_set1_epi16(50))
ZMM_TP_BINARY(vpminuw_zmm_tp, _mm512_min_epu16, _mm512_set1_epi16, 50)

ZMM_LAT_BINARY(vpminud_zmm_lat, _mm512_min_epu32, _mm512_set1_epi32(50))
ZMM_TP_BINARY(vpminud_zmm_tp, _mm512_min_epu32, _mm512_set1_epi32, 50)

ZMM_LAT_BINARY(vpminuq_zmm_lat, _mm512_min_epu64, _mm512_set1_epi64(50))
ZMM_TP_BINARY(vpminuq_zmm_tp, _mm512_min_epu64, _mm512_set1_epi64, 50)

ZMM_LAT_BINARY(vpmaxub_zmm_lat, _mm512_max_epu8, _mm512_set1_epi8(50))
ZMM_TP_BINARY(vpmaxub_zmm_tp, _mm512_max_epu8, _mm512_set1_epi8, 50)

ZMM_LAT_BINARY(vpmaxuw_zmm_lat, _mm512_max_epu16, _mm512_set1_epi16(50))
ZMM_TP_BINARY(vpmaxuw_zmm_tp, _mm512_max_epu16, _mm512_set1_epi16, 50)

ZMM_LAT_BINARY(vpmaxud_zmm_lat, _mm512_max_epu32, _mm512_set1_epi32(50))
ZMM_TP_BINARY(vpmaxud_zmm_tp, _mm512_max_epu32, _mm512_set1_epi32, 50)

ZMM_LAT_BINARY(vpmaxuq_zmm_lat, _mm512_max_epu64, _mm512_set1_epi64(50))
ZMM_TP_BINARY(vpmaxuq_zmm_tp, _mm512_max_epu64, _mm512_set1_epi64, 50)

/* ========================================================================
 * LOGIC (ZMM) -- AVX-512 uses VPANDD/Q, VPORD/Q, VPXORD/Q forms
 * ======================================================================== */

ZMM_LAT_BINARY(vpandd_zmm_lat, _mm512_and_si512, _mm512_set1_epi32(0x55AA55AA))
ZMM_TP_BINARY(vpandd_zmm_tp, _mm512_and_si512, _mm512_set1_epi32, 0x55AA55AA)

ZMM_LAT_BINARY(vpandnd_zmm_lat, _mm512_andnot_si512, _mm512_set1_epi32(0x55AA55AA))
ZMM_TP_BINARY(vpandnd_zmm_tp, _mm512_andnot_si512, _mm512_set1_epi32, 0x55AA55AA)

ZMM_LAT_BINARY(vpord_zmm_lat, _mm512_or_si512, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpord_zmm_tp, _mm512_or_si512, _mm512_set1_epi32, 1)

ZMM_LAT_BINARY(vpxord_zmm_lat, _mm512_xor_si512, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpxord_zmm_tp, _mm512_xor_si512, _mm512_set1_epi32, 1)

/* VPTERNLOGD/Q zmm -- ternary logic (LOP3 equivalent) */
ZMM_LAT_TERNARY_IMM(vpternlogd_zmm_lat, _mm512_ternarylogic_epi32, _mm512_set1_epi32(0x55AA55AA), 0xCA)
ZMM_TP_TERNARY_IMM(vpternlogd_zmm_tp, _mm512_ternarylogic_epi32, _mm512_set1_epi32, 0x55AA55AA, 0xCA)

ZMM_LAT_TERNARY_IMM(vpternlogq_zmm_lat, _mm512_ternarylogic_epi64, _mm512_set1_epi64(0x55AA55AA55AA55AALL), 0xCA)
ZMM_TP_TERNARY_IMM(vpternlogq_zmm_tp, _mm512_ternarylogic_epi64, _mm512_set1_epi64, 0x55AA55AA55AA55AALL, 0xCA)

/* ========================================================================
 * SHIFT -- IMMEDIATE (ZMM)
 * ======================================================================== */

ZMM_LAT_SHIFT_IMM(vpsllw_imm_zmm_lat, _mm512_slli_epi16, _mm512_set1_epi16(0x5555), 1)
ZMM_TP_SHIFT_IMM(vpsllw_imm_zmm_tp, _mm512_slli_epi16, _mm512_set1_epi16, 0x5555, 1)

ZMM_LAT_SHIFT_IMM(vpslld_imm_zmm_lat, _mm512_slli_epi32, _mm512_set1_epi32(0x55555555), 1)
ZMM_TP_SHIFT_IMM(vpslld_imm_zmm_tp, _mm512_slli_epi32, _mm512_set1_epi32, 0x55555555, 1)

ZMM_LAT_SHIFT_IMM(vpsllq_imm_zmm_lat, _mm512_slli_epi64, _mm512_set1_epi64(0x5555555555555555LL), 1)
ZMM_TP_SHIFT_IMM(vpsllq_imm_zmm_tp, _mm512_slli_epi64, _mm512_set1_epi64, 0x5555555555555555LL, 1)

ZMM_LAT_SHIFT_IMM(vpsrlw_imm_zmm_lat, _mm512_srli_epi16, _mm512_set1_epi16(0x5555), 1)
ZMM_TP_SHIFT_IMM(vpsrlw_imm_zmm_tp, _mm512_srli_epi16, _mm512_set1_epi16, 0x5555, 1)

ZMM_LAT_SHIFT_IMM(vpsrld_imm_zmm_lat, _mm512_srli_epi32, _mm512_set1_epi32(0x55555555), 1)
ZMM_TP_SHIFT_IMM(vpsrld_imm_zmm_tp, _mm512_srli_epi32, _mm512_set1_epi32, 0x55555555, 1)

ZMM_LAT_SHIFT_IMM(vpsrlq_imm_zmm_lat, _mm512_srli_epi64, _mm512_set1_epi64(0x5555555555555555LL), 1)
ZMM_TP_SHIFT_IMM(vpsrlq_imm_zmm_tp, _mm512_srli_epi64, _mm512_set1_epi64, 0x5555555555555555LL, 1)

ZMM_LAT_SHIFT_IMM(vpsraw_imm_zmm_lat, _mm512_srai_epi16, _mm512_set1_epi16(0x5555), 1)
ZMM_TP_SHIFT_IMM(vpsraw_imm_zmm_tp, _mm512_srai_epi16, _mm512_set1_epi16, 0x5555, 1)

ZMM_LAT_SHIFT_IMM(vpsrad_imm_zmm_lat, _mm512_srai_epi32, _mm512_set1_epi32(0x55555555), 1)
ZMM_TP_SHIFT_IMM(vpsrad_imm_zmm_tp, _mm512_srai_epi32, _mm512_set1_epi32, 0x55555555, 1)

ZMM_LAT_SHIFT_IMM(vpsraq_imm_zmm_lat, _mm512_srai_epi64, _mm512_set1_epi64(0x5555555555555555LL), 1)
ZMM_TP_SHIFT_IMM(vpsraq_imm_zmm_tp, _mm512_srai_epi64, _mm512_set1_epi64, 0x5555555555555555LL, 1)

/* ========================================================================
 * SHIFT -- VARIABLE PER-ELEMENT (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpsllvd_zmm_lat, _mm512_sllv_epi32, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpsllvd_zmm_tp, _mm512_sllv_epi32, _mm512_set1_epi32, 1)

ZMM_LAT_BINARY(vpsllvq_zmm_lat, _mm512_sllv_epi64, _mm512_set1_epi64(1))
ZMM_TP_BINARY(vpsllvq_zmm_tp, _mm512_sllv_epi64, _mm512_set1_epi64, 1)

ZMM_LAT_BINARY(vpsrlvd_zmm_lat, _mm512_srlv_epi32, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpsrlvd_zmm_tp, _mm512_srlv_epi32, _mm512_set1_epi32, 1)

ZMM_LAT_BINARY(vpsrlvq_zmm_lat, _mm512_srlv_epi64, _mm512_set1_epi64(1))
ZMM_TP_BINARY(vpsrlvq_zmm_tp, _mm512_srlv_epi64, _mm512_set1_epi64, 1)

ZMM_LAT_BINARY(vpsravd_zmm_lat, _mm512_srav_epi32, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpsravd_zmm_tp, _mm512_srav_epi32, _mm512_set1_epi32, 1)

ZMM_LAT_BINARY(vpsravq_zmm_lat, _mm512_srav_epi64, _mm512_set1_epi64(1))
ZMM_TP_BINARY(vpsravq_zmm_tp, _mm512_srav_epi64, _mm512_set1_epi64, 1)

/* ========================================================================
 * ROTATE (ZMM) -- AVX-512F
 * ======================================================================== */

ZMM_LAT_BINARY(vprolvd_zmm_lat, _mm512_rolv_epi32, _mm512_set1_epi32(3))
ZMM_TP_BINARY(vprolvd_zmm_tp, _mm512_rolv_epi32, _mm512_set1_epi32, 3)

ZMM_LAT_BINARY(vprolvq_zmm_lat, _mm512_rolv_epi64, _mm512_set1_epi64(3))
ZMM_TP_BINARY(vprolvq_zmm_tp, _mm512_rolv_epi64, _mm512_set1_epi64, 3)

ZMM_LAT_BINARY(vprorvd_zmm_lat, _mm512_rorv_epi32, _mm512_set1_epi32(3))
ZMM_TP_BINARY(vprorvd_zmm_tp, _mm512_rorv_epi32, _mm512_set1_epi32, 3)

ZMM_LAT_BINARY(vprorvq_zmm_lat, _mm512_rorv_epi64, _mm512_set1_epi64(3))
ZMM_TP_BINARY(vprorvq_zmm_tp, _mm512_rorv_epi64, _mm512_set1_epi64, 3)

/* ========================================================================
 * COMPARE (ZMM) -- result goes to mask register (k)
 * AVX-512 compares produce __mmask, not __m512i. We measure throughput
 * of the compare itself with 8 independent streams.
 * ======================================================================== */

/* Compare latency: chain through mask -> blend -> compare.
 * This measures the full data path, not just compare in isolation. */
#define ZMM_CMP_LAT(func_name, cmp_intrinsic, init_val)                       \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_operand = _mm512_set1_epi32(0);                             \
    __m512i ones_vector = _mm512_set1_epi32(1);                                \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask16 mask_result = cmp_intrinsic(accumulator, source_operand);    \
        accumulator = _mm512_mask_add_epi32(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi32(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi32(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi32(accumulator, mask_result, accumulator, ones_vector); \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

/* Compare throughput: 8 independent compares per iteration */
#define ZMM_CMP_TP(func_name, cmp_intrinsic, init_fn, init_arg)               \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i src_0 = init_fn(init_arg+1), src_1 = init_fn(init_arg+2);         \
    __m512i src_2 = init_fn(init_arg+3), src_3 = init_fn(init_arg+4);         \
    __m512i src_4 = init_fn(init_arg+5), src_5 = init_fn(init_arg+6);         \
    __m512i src_6 = init_fn(init_arg+7), src_7 = init_fn(init_arg+8);         \
    __m512i cmp_target = init_fn(init_arg);                                    \
    volatile __mmask16 mask_sink;                                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask16 k0 = cmp_intrinsic(src_0, cmp_target);                       \
        __mmask16 k1 = cmp_intrinsic(src_1, cmp_target);                       \
        __mmask16 k2 = cmp_intrinsic(src_2, cmp_target);                       \
        __mmask16 k3 = cmp_intrinsic(src_3, cmp_target);                       \
        __mmask16 k4 = cmp_intrinsic(src_4, cmp_target);                       \
        __mmask16 k5 = cmp_intrinsic(src_5, cmp_target);                       \
        __mmask16 k6 = cmp_intrinsic(src_6, cmp_target);                       \
        __mmask16 k7 = cmp_intrinsic(src_7, cmp_target);                       \
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),\
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));\
        mask_sink = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;                 \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    (void)mask_sink;                                                           \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* Byte-granularity compare latency/throughput (mask64) */
#define ZMM_CMP_B_LAT(func_name, cmp_intrinsic, init_val)                     \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_operand = _mm512_set1_epi8(0);                              \
    __m512i ones_vector = _mm512_set1_epi8(1);                                 \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask64 mask_result = cmp_intrinsic(accumulator, source_operand);    \
        accumulator = _mm512_mask_add_epi8(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi8(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi8(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi8(accumulator, mask_result, accumulator, ones_vector); \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_CMP_B_TP(func_name, cmp_intrinsic, init_fn, init_arg)             \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i src_0 = init_fn(init_arg+1), src_1 = init_fn(init_arg+2);         \
    __m512i src_2 = init_fn(init_arg+3), src_3 = init_fn(init_arg+4);         \
    __m512i src_4 = init_fn(init_arg+5), src_5 = init_fn(init_arg+6);         \
    __m512i src_6 = init_fn(init_arg+7), src_7 = init_fn(init_arg+8);         \
    __m512i cmp_target = init_fn(init_arg);                                    \
    volatile __mmask64 mask_sink;                                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask64 k0 = cmp_intrinsic(src_0, cmp_target);                       \
        __mmask64 k1 = cmp_intrinsic(src_1, cmp_target);                       \
        __mmask64 k2 = cmp_intrinsic(src_2, cmp_target);                       \
        __mmask64 k3 = cmp_intrinsic(src_3, cmp_target);                       \
        __mmask64 k4 = cmp_intrinsic(src_4, cmp_target);                       \
        __mmask64 k5 = cmp_intrinsic(src_5, cmp_target);                       \
        __mmask64 k6 = cmp_intrinsic(src_6, cmp_target);                       \
        __mmask64 k7 = cmp_intrinsic(src_7, cmp_target);                       \
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),\
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));\
        mask_sink = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;                 \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    (void)mask_sink;                                                           \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* Word-granularity compare (mask32) */
#define ZMM_CMP_W_LAT(func_name, cmp_intrinsic, init_val)                     \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_operand = _mm512_set1_epi16(0);                             \
    __m512i ones_vector = _mm512_set1_epi16(1);                                \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask32 mask_result = cmp_intrinsic(accumulator, source_operand);    \
        accumulator = _mm512_mask_add_epi16(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi16(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi16(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi16(accumulator, mask_result, accumulator, ones_vector); \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_CMP_W_TP(func_name, cmp_intrinsic, init_fn, init_arg)             \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i src_0 = init_fn(init_arg+1), src_1 = init_fn(init_arg+2);         \
    __m512i src_2 = init_fn(init_arg+3), src_3 = init_fn(init_arg+4);         \
    __m512i src_4 = init_fn(init_arg+5), src_5 = init_fn(init_arg+6);         \
    __m512i src_6 = init_fn(init_arg+7), src_7 = init_fn(init_arg+8);         \
    __m512i cmp_target = init_fn(init_arg);                                    \
    volatile __mmask32 mask_sink;                                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask32 k0 = cmp_intrinsic(src_0, cmp_target);                       \
        __mmask32 k1 = cmp_intrinsic(src_1, cmp_target);                       \
        __mmask32 k2 = cmp_intrinsic(src_2, cmp_target);                       \
        __mmask32 k3 = cmp_intrinsic(src_3, cmp_target);                       \
        __mmask32 k4 = cmp_intrinsic(src_4, cmp_target);                       \
        __mmask32 k5 = cmp_intrinsic(src_5, cmp_target);                       \
        __mmask32 k6 = cmp_intrinsic(src_6, cmp_target);                       \
        __mmask32 k7 = cmp_intrinsic(src_7, cmp_target);                       \
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),\
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));\
        mask_sink = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;                 \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    (void)mask_sink;                                                           \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* Qword-granularity compare (mask8) */
#define ZMM_CMP_Q_LAT(func_name, cmp_intrinsic, init_val)                     \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i accumulator = init_val;                                            \
    __m512i source_operand = _mm512_set1_epi64(0);                             \
    __m512i ones_vector = _mm512_set1_epi64(1);                                \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask8 mask_result = cmp_intrinsic(accumulator, source_operand);     \
        accumulator = _mm512_mask_add_epi64(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi64(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi64(accumulator, mask_result, accumulator, ones_vector); \
        mask_result = cmp_intrinsic(accumulator, source_operand);              \
        accumulator = _mm512_mask_add_epi64(accumulator, mask_result, accumulator, ones_vector); \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(accumulator), 0); \
    (void)sink_value;                                                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

#define ZMM_CMP_Q_TP(func_name, cmp_intrinsic, init_fn, init_arg)             \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    __m512i src_0 = init_fn(init_arg+1), src_1 = init_fn(init_arg+2);         \
    __m512i src_2 = init_fn(init_arg+3), src_3 = init_fn(init_arg+4);         \
    __m512i src_4 = init_fn(init_arg+5), src_5 = init_fn(init_arg+6);         \
    __m512i src_6 = init_fn(init_arg+7), src_7 = init_fn(init_arg+8);         \
    __m512i cmp_target = init_fn(init_arg);                                    \
    volatile __mmask8 mask_sink;                                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __mmask8 k0 = cmp_intrinsic(src_0, cmp_target);                        \
        __mmask8 k1 = cmp_intrinsic(src_1, cmp_target);                        \
        __mmask8 k2 = cmp_intrinsic(src_2, cmp_target);                        \
        __mmask8 k3 = cmp_intrinsic(src_3, cmp_target);                        \
        __mmask8 k4 = cmp_intrinsic(src_4, cmp_target);                        \
        __mmask8 k5 = cmp_intrinsic(src_5, cmp_target);                        \
        __mmask8 k6 = cmp_intrinsic(src_6, cmp_target);                        \
        __mmask8 k7 = cmp_intrinsic(src_7, cmp_target);                        \
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),\
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));\
        mask_sink = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;                 \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    (void)mask_sink;                                                           \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* VPCMPEQB/W/D/Q zmm -> k */
ZMM_CMP_B_LAT(vpcmpeqb_zmm_lat, _mm512_cmpeq_epi8_mask, _mm512_set1_epi8(5))
ZMM_CMP_B_TP(vpcmpeqb_zmm_tp, _mm512_cmpeq_epi8_mask, _mm512_set1_epi8, 5)

ZMM_CMP_W_LAT(vpcmpeqw_zmm_lat, _mm512_cmpeq_epi16_mask, _mm512_set1_epi16(5))
ZMM_CMP_W_TP(vpcmpeqw_zmm_tp, _mm512_cmpeq_epi16_mask, _mm512_set1_epi16, 5)

ZMM_CMP_LAT(vpcmpeqd_zmm_lat, _mm512_cmpeq_epi32_mask, _mm512_set1_epi32(5))
ZMM_CMP_TP(vpcmpeqd_zmm_tp, _mm512_cmpeq_epi32_mask, _mm512_set1_epi32, 5)

ZMM_CMP_Q_LAT(vpcmpeqq_zmm_lat, _mm512_cmpeq_epi64_mask, _mm512_set1_epi64(5))
ZMM_CMP_Q_TP(vpcmpeqq_zmm_tp, _mm512_cmpeq_epi64_mask, _mm512_set1_epi64, 5)

/* VPCMPGTB/W/D/Q zmm -> k */
ZMM_CMP_B_LAT(vpcmpgtb_zmm_lat, _mm512_cmpgt_epi8_mask, _mm512_set1_epi8(5))
ZMM_CMP_B_TP(vpcmpgtb_zmm_tp, _mm512_cmpgt_epi8_mask, _mm512_set1_epi8, 5)

ZMM_CMP_W_LAT(vpcmpgtw_zmm_lat, _mm512_cmpgt_epi16_mask, _mm512_set1_epi16(5))
ZMM_CMP_W_TP(vpcmpgtw_zmm_tp, _mm512_cmpgt_epi16_mask, _mm512_set1_epi16, 5)

ZMM_CMP_LAT(vpcmpgtd_zmm_lat, _mm512_cmpgt_epi32_mask, _mm512_set1_epi32(5))
ZMM_CMP_TP(vpcmpgtd_zmm_tp, _mm512_cmpgt_epi32_mask, _mm512_set1_epi32, 5)

ZMM_CMP_Q_LAT(vpcmpgtq_zmm_lat, _mm512_cmpgt_epi64_mask, _mm512_set1_epi64(5))
ZMM_CMP_Q_TP(vpcmpgtq_zmm_tp, _mm512_cmpgt_epi64_mask, _mm512_set1_epi64, 5)

/* ========================================================================
 * SHUFFLE / PERMUTE (ZMM)
 * ======================================================================== */

/* VPSHUFB zmm */
ZMM_LAT_BINARY(vpshufb_zmm_lat, _mm512_shuffle_epi8, _mm512_set1_epi8(0x03))
ZMM_TP_BINARY(vpshufb_zmm_tp, _mm512_shuffle_epi8, _mm512_set1_epi8, 0x03)

/* VPSHUFD zmm,zmm,imm */
ZMM_LAT_SHUF_IMM(vpshufd_zmm_lat, _mm512_shuffle_epi32, _mm512_set1_epi32(0x12345678), _MM_PERM_ABCD)
ZMM_TP_SHUF_IMM(vpshufd_zmm_tp, _mm512_shuffle_epi32, _mm512_set1_epi32, 0x12345678, _MM_PERM_ABCD)

/* VPERMD zmm */
ZMM_LAT_BINARY(vpermd_zmm_lat, _mm512_permutexvar_epi32, _mm512_set1_epi32(3))
ZMM_TP_BINARY(vpermd_zmm_tp, _mm512_permutexvar_epi32, _mm512_set1_epi32, 3)

/* VPERMQ zmm */
ZMM_LAT_BINARY(vpermq_zmm_lat, _mm512_permutexvar_epi64, _mm512_set1_epi64(3))
ZMM_TP_BINARY(vpermq_zmm_tp, _mm512_permutexvar_epi64, _mm512_set1_epi64, 3)

/* VPERMI2D zmm (2-source permute) */
ZMM_LAT_TERNARY(vpermi2d_zmm_lat, _mm512_permutex2var_epi32, _mm512_set1_epi32(3))
ZMM_TP_TERNARY(vpermi2d_zmm_tp, _mm512_permutex2var_epi32, _mm512_set1_epi32, 3)

/* VPERMI2Q zmm (2-source permute) */
ZMM_LAT_TERNARY(vpermi2q_zmm_lat, _mm512_permutex2var_epi64, _mm512_set1_epi64(3))
ZMM_TP_TERNARY(vpermi2q_zmm_tp, _mm512_permutex2var_epi64, _mm512_set1_epi64, 3)

/* VPERMB zmm (VBMI) */
ZMM_LAT_BINARY(vpermb_zmm_lat, _mm512_permutexvar_epi8, _mm512_set1_epi8(5))
ZMM_TP_BINARY(vpermb_zmm_tp, _mm512_permutexvar_epi8, _mm512_set1_epi8, 5)

/* VPERMI2B zmm (VBMI 2-source byte) */
ZMM_LAT_TERNARY(vpermi2b_zmm_lat, _mm512_permutex2var_epi8, _mm512_set1_epi8(5))
ZMM_TP_TERNARY(vpermi2b_zmm_tp, _mm512_permutex2var_epi8, _mm512_set1_epi8, 5)

/* VALIGND zmm (cross-lane align) */
ZMM_LAT_BINARY_IMM(valignd_zmm_lat, _mm512_alignr_epi32, _mm512_set1_epi32(0x12345678), 4)
ZMM_TP_BINARY_IMM(valignd_zmm_tp, _mm512_alignr_epi32, _mm512_set1_epi32, 0x12345678, 4)

/* VALIGNQ zmm */
ZMM_LAT_BINARY_IMM(valignq_zmm_lat, _mm512_alignr_epi64, _mm512_set1_epi64(0x123456789ABCDEF0LL), 2)
ZMM_TP_BINARY_IMM(valignq_zmm_tp, _mm512_alignr_epi64, _mm512_set1_epi64, 0x123456789ABCDEF0LL, 2)

/* ========================================================================
 * UNPACK (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpunpcklbw_zmm_lat, _mm512_unpacklo_epi8, _mm512_set1_epi8(1))
ZMM_TP_BINARY(vpunpcklbw_zmm_tp, _mm512_unpacklo_epi8, _mm512_set1_epi8, 1)
ZMM_LAT_BINARY(vpunpckhbw_zmm_lat, _mm512_unpackhi_epi8, _mm512_set1_epi8(1))
ZMM_TP_BINARY(vpunpckhbw_zmm_tp, _mm512_unpackhi_epi8, _mm512_set1_epi8, 1)

ZMM_LAT_BINARY(vpunpcklwd_zmm_lat, _mm512_unpacklo_epi16, _mm512_set1_epi16(1))
ZMM_TP_BINARY(vpunpcklwd_zmm_tp, _mm512_unpacklo_epi16, _mm512_set1_epi16, 1)
ZMM_LAT_BINARY(vpunpckhwd_zmm_lat, _mm512_unpackhi_epi16, _mm512_set1_epi16(1))
ZMM_TP_BINARY(vpunpckhwd_zmm_tp, _mm512_unpackhi_epi16, _mm512_set1_epi16, 1)

ZMM_LAT_BINARY(vpunpckldq_zmm_lat, _mm512_unpacklo_epi32, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpunpckldq_zmm_tp, _mm512_unpacklo_epi32, _mm512_set1_epi32, 1)
ZMM_LAT_BINARY(vpunpckhdq_zmm_lat, _mm512_unpackhi_epi32, _mm512_set1_epi32(1))
ZMM_TP_BINARY(vpunpckhdq_zmm_tp, _mm512_unpackhi_epi32, _mm512_set1_epi32, 1)

ZMM_LAT_BINARY(vpunpcklqdq_zmm_lat, _mm512_unpacklo_epi64, _mm512_set1_epi64(1))
ZMM_TP_BINARY(vpunpcklqdq_zmm_tp, _mm512_unpacklo_epi64, _mm512_set1_epi64, 1)
ZMM_LAT_BINARY(vpunpckhqdq_zmm_lat, _mm512_unpackhi_epi64, _mm512_set1_epi64(1))
ZMM_TP_BINARY(vpunpckhqdq_zmm_tp, _mm512_unpackhi_epi64, _mm512_set1_epi64, 1)

/* ========================================================================
 * PACK (ZMM)
 * ======================================================================== */

ZMM_LAT_BINARY(vpackuswb_zmm_lat, _mm512_packus_epi16, _mm512_set1_epi16(50))
ZMM_TP_BINARY(vpackuswb_zmm_tp, _mm512_packus_epi16, _mm512_set1_epi16, 50)

ZMM_LAT_BINARY(vpacksswb_zmm_lat, _mm512_packs_epi16, _mm512_set1_epi16(50))
ZMM_TP_BINARY(vpacksswb_zmm_tp, _mm512_packs_epi16, _mm512_set1_epi16, 50)

ZMM_LAT_BINARY(vpackusdw_zmm_lat, _mm512_packus_epi32, _mm512_set1_epi32(50))
ZMM_TP_BINARY(vpackusdw_zmm_tp, _mm512_packus_epi32, _mm512_set1_epi32, 50)

ZMM_LAT_BINARY(vpackssdw_zmm_lat, _mm512_packs_epi32, _mm512_set1_epi32(50))
ZMM_TP_BINARY(vpackssdw_zmm_tp, _mm512_packs_epi32, _mm512_set1_epi32, 50)

/* ========================================================================
 * BROADCAST (ZMM)
 * ======================================================================== */

static double vpbroadcastb_zmm_tp(uint64_t iteration_count)
{
    __m512i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm512_set1_epi8(1); acc_1 = _mm512_set1_epi8(2);
        acc_2 = _mm512_set1_epi8(3); acc_3 = _mm512_set1_epi8(4);
        acc_4 = _mm512_set1_epi8(5); acc_5 = _mm512_set1_epi8(6);
        acc_6 = _mm512_set1_epi8(7); acc_7 = _mm512_set1_epi8(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) +
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpbroadcastw_zmm_tp(uint64_t iteration_count)
{
    __m512i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm512_set1_epi16(1); acc_1 = _mm512_set1_epi16(2);
        acc_2 = _mm512_set1_epi16(3); acc_3 = _mm512_set1_epi16(4);
        acc_4 = _mm512_set1_epi16(5); acc_5 = _mm512_set1_epi16(6);
        acc_6 = _mm512_set1_epi16(7); acc_7 = _mm512_set1_epi16(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) +
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpbroadcastd_zmm_tp(uint64_t iteration_count)
{
    __m512i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm512_set1_epi32(1); acc_1 = _mm512_set1_epi32(2);
        acc_2 = _mm512_set1_epi32(3); acc_3 = _mm512_set1_epi32(4);
        acc_4 = _mm512_set1_epi32(5); acc_5 = _mm512_set1_epi32(6);
        acc_6 = _mm512_set1_epi32(7); acc_7 = _mm512_set1_epi32(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) +
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpbroadcastq_zmm_tp(uint64_t iteration_count)
{
    __m512i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = _mm512_set1_epi64(1); acc_1 = _mm512_set1_epi64(2);
        acc_2 = _mm512_set1_epi64(3); acc_3 = _mm512_set1_epi64(4);
        acc_4 = _mm512_set1_epi64(5); acc_5 = _mm512_set1_epi64(6);
        acc_6 = _mm512_set1_epi64(7); acc_7 = _mm512_set1_epi64(8);
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) +
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * COMPRESS / EXPAND (ZMM) -- VBMI2
 * ======================================================================== */

/* VPCOMPRESSD zmm -- throughput only (non-deterministic output length) */
static double vpcompressd_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi32(1), src_1 = _mm512_set1_epi32(2);
    __m512i src_2 = _mm512_set1_epi32(3), src_3 = _mm512_set1_epi32(4);
    __m512i src_4 = _mm512_set1_epi32(5), src_5 = _mm512_set1_epi32(6);
    __m512i src_6 = _mm512_set1_epi32(7), src_7 = _mm512_set1_epi32(8);
    __mmask16 compress_mask = 0xAAAA; /* every other element */
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_compress_epi32(compress_mask, src_0);
        __m512i d1 = _mm512_maskz_compress_epi32(compress_mask, src_1);
        __m512i d2 = _mm512_maskz_compress_epi32(compress_mask, src_2);
        __m512i d3 = _mm512_maskz_compress_epi32(compress_mask, src_3);
        __m512i d4 = _mm512_maskz_compress_epi32(compress_mask, src_4);
        __m512i d5 = _mm512_maskz_compress_epi32(compress_mask, src_5);
        __m512i d6 = _mm512_maskz_compress_epi32(compress_mask, src_6);
        __m512i d7 = _mm512_maskz_compress_epi32(compress_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPCOMPRESSQ zmm */
static double vpcompressq_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi64(1), src_1 = _mm512_set1_epi64(2);
    __m512i src_2 = _mm512_set1_epi64(3), src_3 = _mm512_set1_epi64(4);
    __m512i src_4 = _mm512_set1_epi64(5), src_5 = _mm512_set1_epi64(6);
    __m512i src_6 = _mm512_set1_epi64(7), src_7 = _mm512_set1_epi64(8);
    __mmask8 compress_mask = 0xAA;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_compress_epi64(compress_mask, src_0);
        __m512i d1 = _mm512_maskz_compress_epi64(compress_mask, src_1);
        __m512i d2 = _mm512_maskz_compress_epi64(compress_mask, src_2);
        __m512i d3 = _mm512_maskz_compress_epi64(compress_mask, src_3);
        __m512i d4 = _mm512_maskz_compress_epi64(compress_mask, src_4);
        __m512i d5 = _mm512_maskz_compress_epi64(compress_mask, src_5);
        __m512i d6 = _mm512_maskz_compress_epi64(compress_mask, src_6);
        __m512i d7 = _mm512_maskz_compress_epi64(compress_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPEXPANDD zmm */
static double vpexpandd_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi32(1), src_1 = _mm512_set1_epi32(2);
    __m512i src_2 = _mm512_set1_epi32(3), src_3 = _mm512_set1_epi32(4);
    __m512i src_4 = _mm512_set1_epi32(5), src_5 = _mm512_set1_epi32(6);
    __m512i src_6 = _mm512_set1_epi32(7), src_7 = _mm512_set1_epi32(8);
    __mmask16 expand_mask = 0xAAAA;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_expand_epi32(expand_mask, src_0);
        __m512i d1 = _mm512_maskz_expand_epi32(expand_mask, src_1);
        __m512i d2 = _mm512_maskz_expand_epi32(expand_mask, src_2);
        __m512i d3 = _mm512_maskz_expand_epi32(expand_mask, src_3);
        __m512i d4 = _mm512_maskz_expand_epi32(expand_mask, src_4);
        __m512i d5 = _mm512_maskz_expand_epi32(expand_mask, src_5);
        __m512i d6 = _mm512_maskz_expand_epi32(expand_mask, src_6);
        __m512i d7 = _mm512_maskz_expand_epi32(expand_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPEXPANDQ zmm */
static double vpexpandq_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi64(1), src_1 = _mm512_set1_epi64(2);
    __m512i src_2 = _mm512_set1_epi64(3), src_3 = _mm512_set1_epi64(4);
    __m512i src_4 = _mm512_set1_epi64(5), src_5 = _mm512_set1_epi64(6);
    __m512i src_6 = _mm512_set1_epi64(7), src_7 = _mm512_set1_epi64(8);
    __mmask8 expand_mask = 0xAA;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_expand_epi64(expand_mask, src_0);
        __m512i d1 = _mm512_maskz_expand_epi64(expand_mask, src_1);
        __m512i d2 = _mm512_maskz_expand_epi64(expand_mask, src_2);
        __m512i d3 = _mm512_maskz_expand_epi64(expand_mask, src_3);
        __m512i d4 = _mm512_maskz_expand_epi64(expand_mask, src_4);
        __m512i d5 = _mm512_maskz_expand_epi64(expand_mask, src_5);
        __m512i d6 = _mm512_maskz_expand_epi64(expand_mask, src_6);
        __m512i d7 = _mm512_maskz_expand_epi64(expand_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPCOMPRESSB/W zmm (VBMI2) -- byte/word compress */
static double vpcompressb_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi8(1), src_1 = _mm512_set1_epi8(2);
    __m512i src_2 = _mm512_set1_epi8(3), src_3 = _mm512_set1_epi8(4);
    __m512i src_4 = _mm512_set1_epi8(5), src_5 = _mm512_set1_epi8(6);
    __m512i src_6 = _mm512_set1_epi8(7), src_7 = _mm512_set1_epi8(8);
    __mmask64 compress_mask = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_compress_epi8(compress_mask, src_0);
        __m512i d1 = _mm512_maskz_compress_epi8(compress_mask, src_1);
        __m512i d2 = _mm512_maskz_compress_epi8(compress_mask, src_2);
        __m512i d3 = _mm512_maskz_compress_epi8(compress_mask, src_3);
        __m512i d4 = _mm512_maskz_compress_epi8(compress_mask, src_4);
        __m512i d5 = _mm512_maskz_compress_epi8(compress_mask, src_5);
        __m512i d6 = _mm512_maskz_compress_epi8(compress_mask, src_6);
        __m512i d7 = _mm512_maskz_compress_epi8(compress_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpcompressw_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi16(1), src_1 = _mm512_set1_epi16(2);
    __m512i src_2 = _mm512_set1_epi16(3), src_3 = _mm512_set1_epi16(4);
    __m512i src_4 = _mm512_set1_epi16(5), src_5 = _mm512_set1_epi16(6);
    __m512i src_6 = _mm512_set1_epi16(7), src_7 = _mm512_set1_epi16(8);
    __mmask32 compress_mask = 0xAAAAAAAA;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_compress_epi16(compress_mask, src_0);
        __m512i d1 = _mm512_maskz_compress_epi16(compress_mask, src_1);
        __m512i d2 = _mm512_maskz_compress_epi16(compress_mask, src_2);
        __m512i d3 = _mm512_maskz_compress_epi16(compress_mask, src_3);
        __m512i d4 = _mm512_maskz_compress_epi16(compress_mask, src_4);
        __m512i d5 = _mm512_maskz_compress_epi16(compress_mask, src_5);
        __m512i d6 = _mm512_maskz_compress_epi16(compress_mask, src_6);
        __m512i d7 = _mm512_maskz_compress_epi16(compress_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPEXPANDB/W zmm (VBMI2) */
static double vpexpandb_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi8(1), src_1 = _mm512_set1_epi8(2);
    __m512i src_2 = _mm512_set1_epi8(3), src_3 = _mm512_set1_epi8(4);
    __m512i src_4 = _mm512_set1_epi8(5), src_5 = _mm512_set1_epi8(6);
    __m512i src_6 = _mm512_set1_epi8(7), src_7 = _mm512_set1_epi8(8);
    __mmask64 expand_mask = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_expand_epi8(expand_mask, src_0);
        __m512i d1 = _mm512_maskz_expand_epi8(expand_mask, src_1);
        __m512i d2 = _mm512_maskz_expand_epi8(expand_mask, src_2);
        __m512i d3 = _mm512_maskz_expand_epi8(expand_mask, src_3);
        __m512i d4 = _mm512_maskz_expand_epi8(expand_mask, src_4);
        __m512i d5 = _mm512_maskz_expand_epi8(expand_mask, src_5);
        __m512i d6 = _mm512_maskz_expand_epi8(expand_mask, src_6);
        __m512i d7 = _mm512_maskz_expand_epi8(expand_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vpexpandw_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi16(1), src_1 = _mm512_set1_epi16(2);
    __m512i src_2 = _mm512_set1_epi16(3), src_3 = _mm512_set1_epi16(4);
    __m512i src_4 = _mm512_set1_epi16(5), src_5 = _mm512_set1_epi16(6);
    __m512i src_6 = _mm512_set1_epi16(7), src_7 = _mm512_set1_epi16(8);
    __mmask32 expand_mask = 0xAAAAAAAA;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i d0 = _mm512_maskz_expand_epi16(expand_mask, src_0);
        __m512i d1 = _mm512_maskz_expand_epi16(expand_mask, src_1);
        __m512i d2 = _mm512_maskz_expand_epi16(expand_mask, src_2);
        __m512i d3 = _mm512_maskz_expand_epi16(expand_mask, src_3);
        __m512i d4 = _mm512_maskz_expand_epi16(expand_mask, src_4);
        __m512i d5 = _mm512_maskz_expand_epi16(expand_mask, src_5);
        __m512i d6 = _mm512_maskz_expand_epi16(expand_mask, src_6);
        __m512i d7 = _mm512_maskz_expand_epi16(expand_mask, src_7);
        __asm__ volatile("" : "+x"(d0),"+x"(d1),"+x"(d2),"+x"(d3),
                              "+x"(d4),"+x"(d5),"+x"(d6),"+x"(d7));
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(src_0), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * CONFLICT DETECTION (CD) -- ZMM
 * ======================================================================== */

ZMM_LAT_UNARY(vpconflictd_zmm_lat, _mm512_conflict_epi32, _mm512_set1_epi32(5))
ZMM_TP_UNARY(vpconflictd_zmm_tp, _mm512_conflict_epi32, _mm512_set1_epi32, 5)

ZMM_LAT_UNARY(vpconflictq_zmm_lat, _mm512_conflict_epi64, _mm512_set1_epi64(5))
ZMM_TP_UNARY(vpconflictq_zmm_tp, _mm512_conflict_epi64, _mm512_set1_epi64, 5)

ZMM_LAT_UNARY(vplzcntd_zmm_lat, _mm512_lzcnt_epi32, _mm512_set1_epi32(12345))
ZMM_TP_UNARY(vplzcntd_zmm_tp, _mm512_lzcnt_epi32, _mm512_set1_epi32, 12345)

ZMM_LAT_UNARY(vplzcntq_zmm_lat, _mm512_lzcnt_epi64, _mm512_set1_epi64(12345))
ZMM_TP_UNARY(vplzcntq_zmm_tp, _mm512_lzcnt_epi64, _mm512_set1_epi64, 12345)

/* ========================================================================
 * BIT MANIPULATION (BITALG / VPOPCNTDQ) -- ZMM
 * ======================================================================== */

ZMM_LAT_UNARY(vpopcntd_zmm_lat, _mm512_popcnt_epi32, _mm512_set1_epi32(0x55AA55AA))
ZMM_TP_UNARY(vpopcntd_zmm_tp, _mm512_popcnt_epi32, _mm512_set1_epi32, 0x55AA55AA)

ZMM_LAT_UNARY(vpopcntq_zmm_lat, _mm512_popcnt_epi64, _mm512_set1_epi64(0x55AA55AA55AA55AALL))
ZMM_TP_UNARY(vpopcntq_zmm_tp, _mm512_popcnt_epi64, _mm512_set1_epi64, 0x55AA55AA55AA55AALL)

ZMM_LAT_UNARY(vpopcntb_zmm_lat, _mm512_popcnt_epi8, _mm512_set1_epi8(0x55))
ZMM_TP_UNARY(vpopcntb_zmm_tp, _mm512_popcnt_epi8, _mm512_set1_epi8, 0x55)

ZMM_LAT_UNARY(vpopcntw_zmm_lat, _mm512_popcnt_epi16, _mm512_set1_epi16(0x55AA))
ZMM_TP_UNARY(vpopcntw_zmm_tp, _mm512_popcnt_epi16, _mm512_set1_epi16, 0x55AA)

/* VPSHUFBITQMB zmm -> k (BITALG) -- throughput only */
static double vpshufbitqmb_zmm_tp(uint64_t iteration_count)
{
    __m512i src_0 = _mm512_set1_epi8(1), src_1 = _mm512_set1_epi8(2);
    __m512i src_2 = _mm512_set1_epi8(3), src_3 = _mm512_set1_epi8(4);
    __m512i src_4 = _mm512_set1_epi8(5), src_5 = _mm512_set1_epi8(6);
    __m512i src_6 = _mm512_set1_epi8(7), src_7 = _mm512_set1_epi8(8);
    __m512i selector_operand = _mm512_set1_epi8(0x03);
    volatile __mmask64 mask_sink;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __mmask64 k0 = _mm512_bitshuffle_epi64_mask(src_0, selector_operand);
        __mmask64 k1 = _mm512_bitshuffle_epi64_mask(src_1, selector_operand);
        __mmask64 k2 = _mm512_bitshuffle_epi64_mask(src_2, selector_operand);
        __mmask64 k3 = _mm512_bitshuffle_epi64_mask(src_3, selector_operand);
        __mmask64 k4 = _mm512_bitshuffle_epi64_mask(src_4, selector_operand);
        __mmask64 k5 = _mm512_bitshuffle_epi64_mask(src_5, selector_operand);
        __mmask64 k6 = _mm512_bitshuffle_epi64_mask(src_6, selector_operand);
        __mmask64 k7 = _mm512_bitshuffle_epi64_mask(src_7, selector_operand);
        __asm__ volatile("" : "+x"(src_0),"+x"(src_1),"+x"(src_2),"+x"(src_3),
                              "+x"(src_4),"+x"(src_5),"+x"(src_6),"+x"(src_7));
        mask_sink = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    (void)mask_sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * CONCAT-SHIFT (VBMI2) -- ZMM
 * ======================================================================== */

ZMM_LAT_TERNARY(vpshldvd_zmm_lat, _mm512_shldv_epi32, _mm512_set1_epi32(0x55AA55AA))
ZMM_TP_TERNARY(vpshldvd_zmm_tp, _mm512_shldv_epi32, _mm512_set1_epi32, 0x55AA55AA)

ZMM_LAT_TERNARY(vpshldvq_zmm_lat, _mm512_shldv_epi64, _mm512_set1_epi64(0x55AA55AA55AA55AALL))
ZMM_TP_TERNARY(vpshldvq_zmm_tp, _mm512_shldv_epi64, _mm512_set1_epi64, 0x55AA55AA55AA55AALL)

ZMM_LAT_TERNARY(vpshldvw_zmm_lat, _mm512_shldv_epi16, _mm512_set1_epi16(0x55AA))
ZMM_TP_TERNARY(vpshldvw_zmm_tp, _mm512_shldv_epi16, _mm512_set1_epi16, 0x55AA)

ZMM_LAT_TERNARY(vpshrdvd_zmm_lat, _mm512_shrdv_epi32, _mm512_set1_epi32(0x55AA55AA))
ZMM_TP_TERNARY(vpshrdvd_zmm_tp, _mm512_shrdv_epi32, _mm512_set1_epi32, 0x55AA55AA)

ZMM_LAT_TERNARY(vpshrdvq_zmm_lat, _mm512_shrdv_epi64, _mm512_set1_epi64(0x55AA55AA55AA55AALL))
ZMM_TP_TERNARY(vpshrdvq_zmm_tp, _mm512_shrdv_epi64, _mm512_set1_epi64, 0x55AA55AA55AA55AALL)

ZMM_LAT_TERNARY(vpshrdvw_zmm_lat, _mm512_shrdv_epi16, _mm512_set1_epi16(0x55AA))
ZMM_TP_TERNARY(vpshrdvw_zmm_tp, _mm512_shrdv_epi16, _mm512_set1_epi16, 0x55AA)

/* ========================================================================
 * VNNI (ZMM)
 * ======================================================================== */

ZMM_LAT_TERNARY(vpdpbusd_zmm_lat, _mm512_dpbusd_epi32, _mm512_set1_epi32(0x01010101))
ZMM_TP_TERNARY(vpdpbusd_zmm_tp, _mm512_dpbusd_epi32, _mm512_set1_epi32, 0x01010101)

ZMM_LAT_TERNARY(vpdpbusds_zmm_lat, _mm512_dpbusds_epi32, _mm512_set1_epi32(0x01010101))
ZMM_TP_TERNARY(vpdpbusds_zmm_tp, _mm512_dpbusds_epi32, _mm512_set1_epi32, 0x01010101)

ZMM_LAT_TERNARY(vpdpwssd_zmm_lat, _mm512_dpwssd_epi32, _mm512_set1_epi32(0x00010001))
ZMM_TP_TERNARY(vpdpwssd_zmm_tp, _mm512_dpwssd_epi32, _mm512_set1_epi32, 0x00010001)

ZMM_LAT_TERNARY(vpdpwssds_zmm_lat, _mm512_dpwssds_epi32, _mm512_set1_epi32(0x00010001))
ZMM_TP_TERNARY(vpdpwssds_zmm_tp, _mm512_dpwssds_epi32, _mm512_set1_epi32, 0x00010001)

/* ========================================================================
 * IFMA (ZMM)
 * ======================================================================== */

ZMM_LAT_TERNARY(vpmadd52luq_zmm_lat, _mm512_madd52lo_epu64, _mm512_set1_epi64(3))
ZMM_TP_TERNARY(vpmadd52luq_zmm_tp, _mm512_madd52lo_epu64, _mm512_set1_epi64, 3)

ZMM_LAT_TERNARY(vpmadd52huq_zmm_lat, _mm512_madd52hi_epu64, _mm512_set1_epi64(3))
ZMM_TP_TERNARY(vpmadd52huq_zmm_tp, _mm512_madd52hi_epu64, _mm512_set1_epi64, 3)

/* ========================================================================
 * CONVERSION -- PMOVZX / PMOVSX (ymm -> zmm widening)
 * ======================================================================== */

/* ymm->zmm: bw, wd, dq take __m256i input */
ZMM_WIDEN_LAT(vpmovzxbw_zmm_lat, _mm512_cvtepu8_epi16, _mm256_set1_epi8(5))
ZMM_WIDEN_TP(vpmovzxbw_zmm_tp, _mm512_cvtepu8_epi16, _mm256_set1_epi8, 5)

/* xmm->zmm: bd takes __m128i (16 bytes -> 16 dwords) */
ZMM_WIDEN_XMM_LAT(vpmovzxbd_zmm_lat, _mm512_cvtepu8_epi32, _mm_set1_epi8(5))
ZMM_WIDEN_XMM_TP(vpmovzxbd_zmm_tp, _mm512_cvtepu8_epi32, _mm_set1_epi8, 5)

/* xmm->zmm: bq takes __m128i (8 bytes -> 8 qwords) */
ZMM_WIDEN_XMM_LAT(vpmovzxbq_zmm_lat, _mm512_cvtepu8_epi64, _mm_set1_epi8(5))
ZMM_WIDEN_XMM_TP(vpmovzxbq_zmm_tp, _mm512_cvtepu8_epi64, _mm_set1_epi8, 5)

/* ymm->zmm */
ZMM_WIDEN_LAT(vpmovzxwd_zmm_lat, _mm512_cvtepu16_epi32, _mm256_set1_epi16(5))
ZMM_WIDEN_TP(vpmovzxwd_zmm_tp, _mm512_cvtepu16_epi32, _mm256_set1_epi16, 5)

/* xmm->zmm: wq takes __m128i (8 words -> 8 qwords) */
ZMM_WIDEN_XMM_LAT(vpmovzxwq_zmm_lat, _mm512_cvtepu16_epi64, _mm_set1_epi16(5))
ZMM_WIDEN_XMM_TP(vpmovzxwq_zmm_tp, _mm512_cvtepu16_epi64, _mm_set1_epi16, 5)

/* ymm->zmm */
ZMM_WIDEN_LAT(vpmovzxdq_zmm_lat, _mm512_cvtepu32_epi64, _mm256_set1_epi32(5))
ZMM_WIDEN_TP(vpmovzxdq_zmm_tp, _mm512_cvtepu32_epi64, _mm256_set1_epi32, 5)

/* ymm->zmm */
ZMM_WIDEN_LAT(vpmovsxbw_zmm_lat, _mm512_cvtepi8_epi16, _mm256_set1_epi8(-5))
ZMM_WIDEN_TP(vpmovsxbw_zmm_tp, _mm512_cvtepi8_epi16, _mm256_set1_epi8, -5)

/* xmm->zmm */
ZMM_WIDEN_XMM_LAT(vpmovsxbd_zmm_lat, _mm512_cvtepi8_epi32, _mm_set1_epi8(-5))
ZMM_WIDEN_XMM_TP(vpmovsxbd_zmm_tp, _mm512_cvtepi8_epi32, _mm_set1_epi8, -5)

/* xmm->zmm */
ZMM_WIDEN_XMM_LAT(vpmovsxbq_zmm_lat, _mm512_cvtepi8_epi64, _mm_set1_epi8(-5))
ZMM_WIDEN_XMM_TP(vpmovsxbq_zmm_tp, _mm512_cvtepi8_epi64, _mm_set1_epi8, -5)

/* ymm->zmm */
ZMM_WIDEN_LAT(vpmovsxwd_zmm_lat, _mm512_cvtepi16_epi32, _mm256_set1_epi16(-5))
ZMM_WIDEN_TP(vpmovsxwd_zmm_tp, _mm512_cvtepi16_epi32, _mm256_set1_epi16, -5)

/* xmm->zmm */
ZMM_WIDEN_XMM_LAT(vpmovsxwq_zmm_lat, _mm512_cvtepi16_epi64, _mm_set1_epi16(-5))
ZMM_WIDEN_XMM_TP(vpmovsxwq_zmm_tp, _mm512_cvtepi16_epi64, _mm_set1_epi16, -5)

/* ymm->zmm */
ZMM_WIDEN_LAT(vpmovsxdq_zmm_lat, _mm512_cvtepi32_epi64, _mm256_set1_epi32(-5))
ZMM_WIDEN_TP(vpmovsxdq_zmm_tp, _mm512_cvtepi32_epi64, _mm256_set1_epi32, -5)

/* ========================================================================
 * GATHER / SCATTER (ZMM) -- AVX-512F
 * ======================================================================== */

/* VPGATHERDD zmm */
static double vpgatherdd_zmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int32_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m512i index_vector = _mm512_set_epi32(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i gather_result = _mm512_i32gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i32gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i32gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i32gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m512i final_gather = _mm512_i32gather_epi32(index_vector, gather_source_table, 4);
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(final_gather), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPGATHERDQ zmm (32-bit indices, 64-bit elements) */
static double vpgatherdq_zmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int64_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m256i index_vector = _mm256_set_epi32(7,6,5,4,3,2,1,0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i gather_result = _mm512_i32gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i32gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i32gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i32gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m512i final_gather = _mm512_i32gather_epi64(index_vector, gather_source_table, 8);
    volatile long long sink_value = _mm_extract_epi64(_mm512_castsi512_si128(final_gather), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPGATHERQD zmm (64-bit indices, 32-bit elements) */
static double vpgatherqd_zmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int32_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m512i index_vector = _mm512_set_epi64(7,6,5,4,3,2,1,0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m256i gather_result = _mm512_i64gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i64gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i64gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i64gather_epi32(index_vector, gather_source_table, 4);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m256i final_gather = _mm512_i64gather_epi32(index_vector, gather_source_table, 4);
    volatile int sink_value = _mm256_extract_epi32(final_gather, 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPGATHERQQ zmm */
static double vpgatherqq_zmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int64_t gather_source_table[256];
    for (int table_idx = 0; table_idx < 256; table_idx++) gather_source_table[table_idx] = table_idx;
    __m512i index_vector = _mm512_set_epi64(7,6,5,4,3,2,1,0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m512i gather_result = _mm512_i64gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i64gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i64gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
        gather_result = _mm512_i64gather_epi64(index_vector, gather_source_table, 8);
        __asm__ volatile("" : "+x"(gather_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __m512i final_gather = _mm512_i64gather_epi64(index_vector, gather_source_table, 8);
    volatile long long sink_value = _mm_extract_epi64(_mm512_castsi512_si128(final_gather), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPSCATTERDD zmm */
static double vpscatterdd_zmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int32_t scatter_target_table[256];
    memset(scatter_target_table, 0, sizeof(scatter_target_table));
    __m512i index_vector = _mm512_set_epi32(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
    __m512i data_vector = _mm512_set1_epi32(42);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        _mm512_i32scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        _mm512_i32scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        _mm512_i32scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        _mm512_i32scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        __asm__ volatile("" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = scatter_target_table[0]; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPSCATTERQD zmm */
static double vpscatterqd_zmm_tp(uint64_t iteration_count)
{
    __attribute__((aligned(64))) int32_t scatter_target_table[256];
    memset(scatter_target_table, 0, sizeof(scatter_target_table));
    __m512i index_vector = _mm512_set_epi64(7,6,5,4,3,2,1,0);
    __m256i data_vector = _mm256_set1_epi32(42);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        _mm512_i64scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        _mm512_i64scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        _mm512_i64scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        _mm512_i64scatter_epi32(scatter_target_table, index_vector, data_vector, 4);
        __asm__ volatile("" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = scatter_target_table[0]; (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * MOVE (ZMM)
 * ======================================================================== */

static double vmovdqa32_zmm_tp(uint64_t iteration_count)
{
    __m512i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    __m512i source_operand = _mm512_set1_epi32(42);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = source_operand; acc_1 = source_operand;
        acc_2 = source_operand; acc_3 = source_operand;
        acc_4 = source_operand; acc_5 = source_operand;
        acc_6 = source_operand; acc_7 = source_operand;
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7),
                              "+x"(source_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) +
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double vmovdqu64_zmm_tp(uint64_t iteration_count)
{
    __m512i acc_0, acc_1, acc_2, acc_3, acc_4, acc_5, acc_6, acc_7;
    __m512i source_operand = _mm512_set1_epi64(42);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        acc_0 = source_operand; acc_1 = source_operand;
        acc_2 = source_operand; acc_3 = source_operand;
        acc_4 = source_operand; acc_5 = source_operand;
        acc_6 = source_operand; acc_7 = source_operand;
        __asm__ volatile("" : "+x"(acc_0),"+x"(acc_1),"+x"(acc_2),"+x"(acc_3),
                              "+x"(acc_4),"+x"(acc_5),"+x"(acc_6),"+x"(acc_7),
                              "+x"(source_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink_value = _mm_extract_epi32(_mm512_castsi512_si128(acc_0), 0) +
                              _mm_extract_epi32(_mm512_castsi512_si128(acc_4), 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * PROBE TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    int width_bits;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} Avx512IntProbeEntry;

static const Avx512IntProbeEntry probe_table[] = {
    /* --- Arithmetic: Add --- */
    {"VPADDB",     "zmm,zmm",       512, "arith_add",   vpaddb_zmm_lat,     vpaddb_zmm_tp},
    {"VPADDW",     "zmm,zmm",       512, "arith_add",   vpaddw_zmm_lat,     vpaddw_zmm_tp},
    {"VPADDD",     "zmm,zmm",       512, "arith_add",   vpaddd_zmm_lat,     vpaddd_zmm_tp},
    {"VPADDQ",     "zmm,zmm",       512, "arith_add",   vpaddq_zmm_lat,     vpaddq_zmm_tp},

    /* --- Arithmetic: Sub --- */
    {"VPSUBB",     "zmm,zmm",       512, "arith_sub",   vpsubb_zmm_lat,     vpsubb_zmm_tp},
    {"VPSUBW",     "zmm,zmm",       512, "arith_sub",   vpsubw_zmm_lat,     vpsubw_zmm_tp},
    {"VPSUBD",     "zmm,zmm",       512, "arith_sub",   vpsubd_zmm_lat,     vpsubd_zmm_tp},
    {"VPSUBQ",     "zmm,zmm",       512, "arith_sub",   vpsubq_zmm_lat,     vpsubq_zmm_tp},

    /* --- Arithmetic: Saturating --- */
    {"VPADDUSB",   "zmm,zmm",       512, "arith_sat",   vpaddusb_zmm_lat,   vpaddusb_zmm_tp},
    {"VPADDUSW",   "zmm,zmm",       512, "arith_sat",   vpaddusw_zmm_lat,   vpaddusw_zmm_tp},
    {"VPADDSB",    "zmm,zmm",       512, "arith_sat",   vpaddsb_zmm_lat,    vpaddsb_zmm_tp},
    {"VPADDSW",    "zmm,zmm",       512, "arith_sat",   vpaddsw_zmm_lat,    vpaddsw_zmm_tp},
    {"VPSUBUSB",   "zmm,zmm",       512, "arith_sat",   vpsubusb_zmm_lat,   vpsubusb_zmm_tp},
    {"VPSUBUSW",   "zmm,zmm",       512, "arith_sat",   vpsubusw_zmm_lat,   vpsubusw_zmm_tp},
    {"VPSUBSB",    "zmm,zmm",       512, "arith_sat",   vpsubsb_zmm_lat,    vpsubsb_zmm_tp},
    {"VPSUBSW",    "zmm,zmm",       512, "arith_sat",   vpsubsw_zmm_lat,    vpsubsw_zmm_tp},

    /* --- Arithmetic: Multiply --- */
    {"VPMULLW",    "zmm,zmm",       512, "arith_mul",   vpmullw_zmm_lat,    vpmullw_zmm_tp},
    {"VPMULLD",    "zmm,zmm",       512, "arith_mul",   vpmulld_zmm_lat,    vpmulld_zmm_tp},
    {"VPMULLQ",    "zmm,zmm",       512, "arith_mul",   vpmullq_zmm_lat,    vpmullq_zmm_tp},
    {"VPMULDQ",    "zmm,zmm",       512, "arith_mul",   vpmuldq_zmm_lat,    vpmuldq_zmm_tp},
    {"VPMULUDQ",   "zmm,zmm",       512, "arith_mul",   vpmuludq_zmm_lat,   vpmuludq_zmm_tp},
    {"VPMULHW",    "zmm,zmm",       512, "arith_mul",   vpmulhw_zmm_lat,    vpmulhw_zmm_tp},
    {"VPMULHUW",   "zmm,zmm",       512, "arith_mul",   vpmulhuw_zmm_lat,   vpmulhuw_zmm_tp},
    {"VPMADDWD",   "zmm,zmm",       512, "arith_mul",   vpmaddwd_zmm_lat,   vpmaddwd_zmm_tp},
    {"VPMADDUBSW", "zmm,zmm",       512, "arith_mul",   vpmaddubsw_zmm_lat, vpmaddubsw_zmm_tp},

    /* --- Arithmetic: SAD / Average / Abs --- */
    {"VPSADBW",    "zmm,zmm",       512, "arith_misc",  vpsadbw_zmm_lat,    vpsadbw_zmm_tp},
    {"VPAVGB",     "zmm,zmm",       512, "arith_misc",  vpavgb_zmm_lat,     vpavgb_zmm_tp},
    {"VPAVGW",     "zmm,zmm",       512, "arith_misc",  vpavgw_zmm_lat,     vpavgw_zmm_tp},
    {"VPABSB",     "zmm",           512, "arith_abs",   vpabsb_zmm_lat,     vpabsb_zmm_tp},
    {"VPABSW",     "zmm",           512, "arith_abs",   vpabsw_zmm_lat,     vpabsw_zmm_tp},
    {"VPABSD",     "zmm",           512, "arith_abs",   vpabsd_zmm_lat,     vpabsd_zmm_tp},
    {"VPABSQ",     "zmm",           512, "arith_abs",   vpabsq_zmm_lat,     vpabsq_zmm_tp},

    /* --- Min/Max Signed --- */
    {"VPMINSB",    "zmm,zmm",       512, "arith_minmax", vpminsb_zmm_lat,   vpminsb_zmm_tp},
    {"VPMINSW",    "zmm,zmm",       512, "arith_minmax", vpminsw_zmm_lat,   vpminsw_zmm_tp},
    {"VPMINSD",    "zmm,zmm",       512, "arith_minmax", vpminsd_zmm_lat,   vpminsd_zmm_tp},
    {"VPMINSQ",    "zmm,zmm",       512, "arith_minmax", vpminsq_zmm_lat,   vpminsq_zmm_tp},
    {"VPMAXSB",    "zmm,zmm",       512, "arith_minmax", vpmaxsb_zmm_lat,   vpmaxsb_zmm_tp},
    {"VPMAXSW",    "zmm,zmm",       512, "arith_minmax", vpmaxsw_zmm_lat,   vpmaxsw_zmm_tp},
    {"VPMAXSD",    "zmm,zmm",       512, "arith_minmax", vpmaxsd_zmm_lat,   vpmaxsd_zmm_tp},
    {"VPMAXSQ",    "zmm,zmm",       512, "arith_minmax", vpmaxsq_zmm_lat,   vpmaxsq_zmm_tp},

    /* --- Min/Max Unsigned --- */
    {"VPMINUB",    "zmm,zmm",       512, "arith_minmax", vpminub_zmm_lat,   vpminub_zmm_tp},
    {"VPMINUW",    "zmm,zmm",       512, "arith_minmax", vpminuw_zmm_lat,   vpminuw_zmm_tp},
    {"VPMINUD",    "zmm,zmm",       512, "arith_minmax", vpminud_zmm_lat,   vpminud_zmm_tp},
    {"VPMINUQ",    "zmm,zmm",       512, "arith_minmax", vpminuq_zmm_lat,   vpminuq_zmm_tp},
    {"VPMAXUB",    "zmm,zmm",       512, "arith_minmax", vpmaxub_zmm_lat,   vpmaxub_zmm_tp},
    {"VPMAXUW",    "zmm,zmm",       512, "arith_minmax", vpmaxuw_zmm_lat,   vpmaxuw_zmm_tp},
    {"VPMAXUD",    "zmm,zmm",       512, "arith_minmax", vpmaxud_zmm_lat,   vpmaxud_zmm_tp},
    {"VPMAXUQ",    "zmm,zmm",       512, "arith_minmax", vpmaxuq_zmm_lat,   vpmaxuq_zmm_tp},

    /* --- Logic --- */
    {"VPANDD",     "zmm,zmm",       512, "logic",       vpandd_zmm_lat,     vpandd_zmm_tp},
    {"VPANDND",    "zmm,zmm",       512, "logic",       vpandnd_zmm_lat,    vpandnd_zmm_tp},
    {"VPORD",      "zmm,zmm",       512, "logic",       vpord_zmm_lat,      vpord_zmm_tp},
    {"VPXORD",     "zmm,zmm",       512, "logic",       vpxord_zmm_lat,     vpxord_zmm_tp},
    {"VPTERNLOGD", "zmm,zmm,zmm,imm", 512, "logic",    vpternlogd_zmm_lat, vpternlogd_zmm_tp},
    {"VPTERNLOGQ", "zmm,zmm,zmm,imm", 512, "logic",    vpternlogq_zmm_lat, vpternlogq_zmm_tp},

    /* --- Shift: Immediate --- */
    {"VPSLLW",     "zmm,imm",       512, "shift_imm",   vpsllw_imm_zmm_lat, vpsllw_imm_zmm_tp},
    {"VPSLLD",     "zmm,imm",       512, "shift_imm",   vpslld_imm_zmm_lat, vpslld_imm_zmm_tp},
    {"VPSLLQ",     "zmm,imm",       512, "shift_imm",   vpsllq_imm_zmm_lat, vpsllq_imm_zmm_tp},
    {"VPSRLW",     "zmm,imm",       512, "shift_imm",   vpsrlw_imm_zmm_lat, vpsrlw_imm_zmm_tp},
    {"VPSRLD",     "zmm,imm",       512, "shift_imm",   vpsrld_imm_zmm_lat, vpsrld_imm_zmm_tp},
    {"VPSRLQ",     "zmm,imm",       512, "shift_imm",   vpsrlq_imm_zmm_lat, vpsrlq_imm_zmm_tp},
    {"VPSRAW",     "zmm,imm",       512, "shift_imm",   vpsraw_imm_zmm_lat, vpsraw_imm_zmm_tp},
    {"VPSRAD",     "zmm,imm",       512, "shift_imm",   vpsrad_imm_zmm_lat, vpsrad_imm_zmm_tp},
    {"VPSRAQ",     "zmm,imm",       512, "shift_imm",   vpsraq_imm_zmm_lat, vpsraq_imm_zmm_tp},

    /* --- Shift: Variable --- */
    {"VPSLLVD",    "zmm,zmm,zmm",   512, "shift_var",   vpsllvd_zmm_lat,    vpsllvd_zmm_tp},
    {"VPSLLVQ",    "zmm,zmm,zmm",   512, "shift_var",   vpsllvq_zmm_lat,    vpsllvq_zmm_tp},
    {"VPSRLVD",    "zmm,zmm,zmm",   512, "shift_var",   vpsrlvd_zmm_lat,    vpsrlvd_zmm_tp},
    {"VPSRLVQ",    "zmm,zmm,zmm",   512, "shift_var",   vpsrlvq_zmm_lat,    vpsrlvq_zmm_tp},
    {"VPSRAVD",    "zmm,zmm,zmm",   512, "shift_var",   vpsravd_zmm_lat,    vpsravd_zmm_tp},
    {"VPSRAVQ",    "zmm,zmm,zmm",   512, "shift_var",   vpsravq_zmm_lat,    vpsravq_zmm_tp},

    /* --- Rotate --- */
    {"VPROLVD",    "zmm,zmm",       512, "rotate",      vprolvd_zmm_lat,    vprolvd_zmm_tp},
    {"VPROLVQ",    "zmm,zmm",       512, "rotate",      vprolvq_zmm_lat,    vprolvq_zmm_tp},
    {"VPRORVD",    "zmm,zmm",       512, "rotate",      vprorvd_zmm_lat,    vprorvd_zmm_tp},
    {"VPRORVQ",    "zmm,zmm",       512, "rotate",      vprorvq_zmm_lat,    vprorvq_zmm_tp},

    /* --- Compare (mask output) --- */
    {"VPCMPEQB",   "zmm,zmm->k",   512, "compare",     vpcmpeqb_zmm_lat,   vpcmpeqb_zmm_tp},
    {"VPCMPEQW",   "zmm,zmm->k",   512, "compare",     vpcmpeqw_zmm_lat,   vpcmpeqw_zmm_tp},
    {"VPCMPEQD",   "zmm,zmm->k",   512, "compare",     vpcmpeqd_zmm_lat,   vpcmpeqd_zmm_tp},
    {"VPCMPEQQ",   "zmm,zmm->k",   512, "compare",     vpcmpeqq_zmm_lat,   vpcmpeqq_zmm_tp},
    {"VPCMPGTB",   "zmm,zmm->k",   512, "compare",     vpcmpgtb_zmm_lat,   vpcmpgtb_zmm_tp},
    {"VPCMPGTW",   "zmm,zmm->k",   512, "compare",     vpcmpgtw_zmm_lat,   vpcmpgtw_zmm_tp},
    {"VPCMPGTD",   "zmm,zmm->k",   512, "compare",     vpcmpgtd_zmm_lat,   vpcmpgtd_zmm_tp},
    {"VPCMPGTQ",   "zmm,zmm->k",   512, "compare",     vpcmpgtq_zmm_lat,   vpcmpgtq_zmm_tp},

    /* --- Shuffle/Permute --- */
    {"VPSHUFB",    "zmm,zmm",       512, "shuffle",     vpshufb_zmm_lat,    vpshufb_zmm_tp},
    {"VPSHUFD",    "zmm,zmm,imm",   512, "shuffle",     vpshufd_zmm_lat,    vpshufd_zmm_tp},
    {"VPERMD",     "zmm,zmm,zmm",   512, "shuffle",     vpermd_zmm_lat,     vpermd_zmm_tp},
    {"VPERMQ",     "zmm,zmm,zmm",   512, "shuffle",     vpermq_zmm_lat,     vpermq_zmm_tp},
    {"VPERMI2D",   "zmm,zmm,zmm",   512, "shuffle",     vpermi2d_zmm_lat,   vpermi2d_zmm_tp},
    {"VPERMI2Q",   "zmm,zmm,zmm",   512, "shuffle",     vpermi2q_zmm_lat,   vpermi2q_zmm_tp},
    {"VPERMB",     "zmm,zmm",       512, "shuffle",     vpermb_zmm_lat,     vpermb_zmm_tp},
    {"VPERMI2B",   "zmm,zmm,zmm",   512, "shuffle",     vpermi2b_zmm_lat,   vpermi2b_zmm_tp},
    {"VALIGND",    "zmm,zmm,imm",   512, "shuffle",     valignd_zmm_lat,    valignd_zmm_tp},
    {"VALIGNQ",    "zmm,zmm,imm",   512, "shuffle",     valignq_zmm_lat,    valignq_zmm_tp},

    /* --- Unpack --- */
    {"VPUNPCKLBW", "zmm,zmm",       512, "unpack",      vpunpcklbw_zmm_lat, vpunpcklbw_zmm_tp},
    {"VPUNPCKHBW", "zmm,zmm",       512, "unpack",      vpunpckhbw_zmm_lat, vpunpckhbw_zmm_tp},
    {"VPUNPCKLWD", "zmm,zmm",       512, "unpack",      vpunpcklwd_zmm_lat, vpunpcklwd_zmm_tp},
    {"VPUNPCKHWD", "zmm,zmm",       512, "unpack",      vpunpckhwd_zmm_lat, vpunpckhwd_zmm_tp},
    {"VPUNPCKLDQ", "zmm,zmm",       512, "unpack",      vpunpckldq_zmm_lat, vpunpckldq_zmm_tp},
    {"VPUNPCKHDQ", "zmm,zmm",       512, "unpack",      vpunpckhdq_zmm_lat, vpunpckhdq_zmm_tp},
    {"VPUNPCKLQDQ","zmm,zmm",       512, "unpack",      vpunpcklqdq_zmm_lat,vpunpcklqdq_zmm_tp},
    {"VPUNPCKHQDQ","zmm,zmm",       512, "unpack",      vpunpckhqdq_zmm_lat,vpunpckhqdq_zmm_tp},

    /* --- Pack --- */
    {"VPACKUSWB",  "zmm,zmm",       512, "pack",        vpackuswb_zmm_lat,  vpackuswb_zmm_tp},
    {"VPACKSSWB",  "zmm,zmm",       512, "pack",        vpacksswb_zmm_lat,  vpacksswb_zmm_tp},
    {"VPACKUSDW",  "zmm,zmm",       512, "pack",        vpackusdw_zmm_lat,  vpackusdw_zmm_tp},
    {"VPACKSSDW",  "zmm,zmm",       512, "pack",        vpackssdw_zmm_lat,  vpackssdw_zmm_tp},

    /* --- Broadcast --- */
    {"VPBROADCASTB","zmm",          512, "broadcast",   NULL,               vpbroadcastb_zmm_tp},
    {"VPBROADCASTW","zmm",          512, "broadcast",   NULL,               vpbroadcastw_zmm_tp},
    {"VPBROADCASTD","zmm",          512, "broadcast",   NULL,               vpbroadcastd_zmm_tp},
    {"VPBROADCASTQ","zmm",          512, "broadcast",   NULL,               vpbroadcastq_zmm_tp},

    /* --- Compress/Expand --- */
    {"VPCOMPRESSD","zmm{k}",        512, "compress",    NULL,               vpcompressd_zmm_tp},
    {"VPCOMPRESSQ","zmm{k}",        512, "compress",    NULL,               vpcompressq_zmm_tp},
    {"VPEXPANDD",  "zmm{k}",        512, "expand",      NULL,               vpexpandd_zmm_tp},
    {"VPEXPANDQ",  "zmm{k}",        512, "expand",      NULL,               vpexpandq_zmm_tp},
    {"VPCOMPRESSB","zmm{k}",        512, "compress",    NULL,               vpcompressb_zmm_tp},
    {"VPCOMPRESSW","zmm{k}",        512, "compress",    NULL,               vpcompressw_zmm_tp},
    {"VPEXPANDB",  "zmm{k}",        512, "expand",      NULL,               vpexpandb_zmm_tp},
    {"VPEXPANDW",  "zmm{k}",        512, "expand",      NULL,               vpexpandw_zmm_tp},

    /* --- Conflict Detection (CD) --- */
    {"VPCONFLICTD","zmm",           512, "conflict",    vpconflictd_zmm_lat, vpconflictd_zmm_tp},
    {"VPCONFLICTQ","zmm",           512, "conflict",    vpconflictq_zmm_lat, vpconflictq_zmm_tp},
    {"VPLZCNTD",   "zmm",           512, "conflict",    vplzcntd_zmm_lat,   vplzcntd_zmm_tp},
    {"VPLZCNTQ",   "zmm",           512, "conflict",    vplzcntq_zmm_lat,   vplzcntq_zmm_tp},

    /* --- Bit Manipulation (BITALG / VPOPCNTDQ) --- */
    {"VPOPCNTD",   "zmm",           512, "bitmanip",    vpopcntd_zmm_lat,   vpopcntd_zmm_tp},
    {"VPOPCNTQ",   "zmm",           512, "bitmanip",    vpopcntq_zmm_lat,   vpopcntq_zmm_tp},
    {"VPOPCNTB",   "zmm",           512, "bitmanip",    vpopcntb_zmm_lat,   vpopcntb_zmm_tp},
    {"VPOPCNTW",   "zmm",           512, "bitmanip",    vpopcntw_zmm_lat,   vpopcntw_zmm_tp},
    {"VPSHUFBITQMB","zmm,zmm->k",   512, "bitmanip",    NULL,               vpshufbitqmb_zmm_tp},

    /* --- Concat-Shift (VBMI2) --- */
    {"VPSHLDVD",   "zmm,zmm,zmm",   512, "concat_shift", vpshldvd_zmm_lat,  vpshldvd_zmm_tp},
    {"VPSHLDVQ",   "zmm,zmm,zmm",   512, "concat_shift", vpshldvq_zmm_lat,  vpshldvq_zmm_tp},
    {"VPSHLDVW",   "zmm,zmm,zmm",   512, "concat_shift", vpshldvw_zmm_lat,  vpshldvw_zmm_tp},
    {"VPSHRDVD",   "zmm,zmm,zmm",   512, "concat_shift", vpshrdvd_zmm_lat,  vpshrdvd_zmm_tp},
    {"VPSHRDVQ",   "zmm,zmm,zmm",   512, "concat_shift", vpshrdvq_zmm_lat,  vpshrdvq_zmm_tp},
    {"VPSHRDVW",   "zmm,zmm,zmm",   512, "concat_shift", vpshrdvw_zmm_lat,  vpshrdvw_zmm_tp},

    /* --- VNNI --- */
    {"VPDPBUSD",   "zmm,zmm,zmm",   512, "vnni",       vpdpbusd_zmm_lat,   vpdpbusd_zmm_tp},
    {"VPDPBUSDS",  "zmm,zmm,zmm",   512, "vnni",       vpdpbusds_zmm_lat,  vpdpbusds_zmm_tp},
    {"VPDPWSSD",   "zmm,zmm,zmm",   512, "vnni",       vpdpwssd_zmm_lat,   vpdpwssd_zmm_tp},
    {"VPDPWSSDS",  "zmm,zmm,zmm",   512, "vnni",       vpdpwssds_zmm_lat,  vpdpwssds_zmm_tp},

    /* --- IFMA --- */
    {"VPMADD52LUQ","zmm,zmm,zmm",   512, "ifma",       vpmadd52luq_zmm_lat, vpmadd52luq_zmm_tp},
    {"VPMADD52HUQ","zmm,zmm,zmm",   512, "ifma",       vpmadd52huq_zmm_lat, vpmadd52huq_zmm_tp},

    /* --- Conversion: PMOVZX (ymm->zmm) --- */
    {"VPMOVZXBW",  "ymm->zmm",      512, "convert",    vpmovzxbw_zmm_lat,  vpmovzxbw_zmm_tp},
    {"VPMOVZXBD",  "xmm->zmm",      512, "convert",    vpmovzxbd_zmm_lat,  vpmovzxbd_zmm_tp},
    {"VPMOVZXBQ",  "xmm->zmm",      512, "convert",    vpmovzxbq_zmm_lat,  vpmovzxbq_zmm_tp},
    {"VPMOVZXWD",  "ymm->zmm",      512, "convert",    vpmovzxwd_zmm_lat,  vpmovzxwd_zmm_tp},
    {"VPMOVZXWQ",  "xmm->zmm",      512, "convert",    vpmovzxwq_zmm_lat,  vpmovzxwq_zmm_tp},
    {"VPMOVZXDQ",  "ymm->zmm",      512, "convert",    vpmovzxdq_zmm_lat,  vpmovzxdq_zmm_tp},

    /* --- Conversion: PMOVSX (ymm->zmm) --- */
    {"VPMOVSXBW",  "ymm->zmm",      512, "convert",    vpmovsxbw_zmm_lat,  vpmovsxbw_zmm_tp},
    {"VPMOVSXBD",  "xmm->zmm",      512, "convert",    vpmovsxbd_zmm_lat,  vpmovsxbd_zmm_tp},
    {"VPMOVSXBQ",  "xmm->zmm",      512, "convert",    vpmovsxbq_zmm_lat,  vpmovsxbq_zmm_tp},
    {"VPMOVSXWD",  "ymm->zmm",      512, "convert",    vpmovsxwd_zmm_lat,  vpmovsxwd_zmm_tp},
    {"VPMOVSXWQ",  "xmm->zmm",      512, "convert",    vpmovsxwq_zmm_lat,  vpmovsxwq_zmm_tp},
    {"VPMOVSXDQ",  "ymm->zmm",      512, "convert",    vpmovsxdq_zmm_lat,  vpmovsxdq_zmm_tp},

    /* --- Gather --- */
    {"VPGATHERDD", "zmm,[base+zmm*4]", 512, "gather",  NULL,               vpgatherdd_zmm_tp},
    {"VPGATHERDQ", "zmm,[base+ymm*8]", 512, "gather",  NULL,               vpgatherdq_zmm_tp},
    {"VPGATHERQD", "ymm,[base+zmm*4]", 512, "gather",  NULL,               vpgatherqd_zmm_tp},
    {"VPGATHERQQ", "zmm,[base+zmm*8]", 512, "gather",  NULL,               vpgatherqq_zmm_tp},

    /* --- Scatter --- */
    {"VPSCATTERDD","[base+zmm*4],zmm", 512, "scatter",  NULL,              vpscatterdd_zmm_tp},
    {"VPSCATTERQD","[base+zmm*4],ymm", 512, "scatter",  NULL,              vpscatterqd_zmm_tp},

    /* --- Move --- */
    {"VMOVDQA32",  "zmm,zmm",       512, "move",       NULL,               vmovdqa32_zmm_tp},
    {"VMOVDQU64",  "zmm,zmm",       512, "move",       NULL,               vmovdqu64_zmm_tp},

    {NULL, NULL, 0, NULL, NULL, NULL}
};

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 20000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    int csv_mode = probe_config.csv;

    if (!csv_mode) {
        sm_zen_print_header("avx512_int_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring AVX-512 ZMM integer instruction forms (512-bit)...\n\n");
    }

    /* CSV header */
    printf("mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const Avx512IntProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const Avx512IntProbeEntry *current_entry = &probe_table[entry_idx];

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
