#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_avx512_fp_exhaustive.c -- Exhaustive AVX-512 ZMM floating-point
 * instruction measurement for Zen 4 (512-bit width).
 *
 * Measures latency (4-deep dependency chain) and throughput (8 independent
 * accumulators) for every AVX-512 FP instruction form at 512-bit width.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx512f -mavx512bw \
 *     -mavx512dq -mavx512vl -mavx512cd -mavx512bf16 -mfma -mf16c \
 *     -fno-omit-frame-pointer -include x86intrin.h \
 *     -I. common.c probe_avx512_fp_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_avx512_fp_exhaustive
 */

/* ========================================================================
 * MACRO TEMPLATES for ZMM (512-bit) AVX-512 FP
 *
 * "+x" constraint covers xmm/ymm/zmm on clang.
 * Barriers: __asm__ volatile("" : "+x"(acc));
 * ======================================================================== */

/* ZMM init values */
#define ZMM_F32_INIT  _mm512_set1_ps(1.0001f)
#define ZMM_F64_INIT  _mm512_set1_pd(1.0001)
#define ZMM_I32_INIT  _mm512_set1_epi32(1)
#define ZMM_I64_INIT  _mm512_set1_epi64(1)
#define YMM_F32_INIT  _mm256_set1_ps(1.0001f)
#define YMM_F64_INIT  _mm256_set1_pd(1.0001)
#define YMM_I32_INIT  _mm256_set1_epi32(1)
#define XMM_F32_INIT  _mm_set1_ps(1.0001f)

/*
 * LAT_ZMM_BINARY: latency for "op zmm_src, zmm_dst" where dst is both
 * input and output. 4-deep dependency chain.
 */
#define LAT_ZMM_BINARY(func_name, asm_str, init_type, init_val)               \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type accumulator = init_val;                                         \
    init_type operand_source = init_val;                                      \
    __asm__ volatile("" : "+x"(operand_source));                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(operand_source));  \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(operand_source));  \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(operand_source));  \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(operand_source));  \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(accumulator));                                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_ZMM_BINARY: throughput for "op zmm_src, zmm_dst" with 8 independent
 * accumulator streams.
 */
#define TP_ZMM_BINARY(func_name, asm_str, init_type, init_val)                \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type stream_0 = init_val;                                            \
    init_type stream_1 = init_val;                                            \
    init_type stream_2 = init_val;                                            \
    init_type stream_3 = init_val;                                            \
    init_type stream_4 = init_val;                                            \
    init_type stream_5 = init_val;                                            \
    init_type stream_6 = init_val;                                            \
    init_type stream_7 = init_val;                                            \
    init_type operand_source = init_val;                                      \
    __asm__ volatile("" : "+x"(operand_source));                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+x"(stream_0) : "x"(operand_source));     \
        __asm__ volatile(asm_str : "+x"(stream_1) : "x"(operand_source));     \
        __asm__ volatile(asm_str : "+x"(stream_2) : "x"(operand_source));     \
        __asm__ volatile(asm_str : "+x"(stream_3) : "x"(operand_source));     \
        __asm__ volatile(asm_str : "+x"(stream_4) : "x"(operand_source));     \
        __asm__ volatile(asm_str : "+x"(stream_5) : "x"(operand_source));     \
        __asm__ volatile(asm_str : "+x"(stream_6) : "x"(operand_source));     \
        __asm__ volatile(asm_str : "+x"(stream_7) : "x"(operand_source));     \
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),                \
                              "+x"(stream_2), "+x"(stream_3));                \
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),                \
                              "+x"(stream_6), "+x"(stream_7));                \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(stream_0));                                    \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/*
 * LAT_ZMM_UNARY: latency for "op zmm_dst, zmm_dst" (reads and writes same
 * register). Used for SQRT, RCP14, RSQRT14, etc.
 */
#define LAT_ZMM_UNARY(func_name, asm_str, init_type, init_val)               \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type accumulator = init_val;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+x"(accumulator));                        \
        __asm__ volatile(asm_str : "+x"(accumulator));                        \
        __asm__ volatile(asm_str : "+x"(accumulator));                        \
        __asm__ volatile(asm_str : "+x"(accumulator));                        \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(accumulator));                                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_ZMM_UNARY: throughput for unary zmm instruction, 8 streams.
 */
#define TP_ZMM_UNARY(func_name, asm_str, init_type, init_val)                \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type stream_0 = init_val;                                            \
    init_type stream_1 = init_val;                                            \
    init_type stream_2 = init_val;                                            \
    init_type stream_3 = init_val;                                            \
    init_type stream_4 = init_val;                                            \
    init_type stream_5 = init_val;                                            \
    init_type stream_6 = init_val;                                            \
    init_type stream_7 = init_val;                                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+x"(stream_0));                           \
        __asm__ volatile(asm_str : "+x"(stream_1));                           \
        __asm__ volatile(asm_str : "+x"(stream_2));                           \
        __asm__ volatile(asm_str : "+x"(stream_3));                           \
        __asm__ volatile(asm_str : "+x"(stream_4));                           \
        __asm__ volatile(asm_str : "+x"(stream_5));                           \
        __asm__ volatile(asm_str : "+x"(stream_6));                           \
        __asm__ volatile(asm_str : "+x"(stream_7));                           \
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),                \
                              "+x"(stream_2), "+x"(stream_3));                \
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),                \
                              "+x"(stream_6), "+x"(stream_7));                \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(stream_0));                                    \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/*
 * LAT_ZMM_FMA: latency for EVEX 3-operand FMA:
 * "vfmadd231ps src2, src1, dst" — dst feeds back as accumulator.
 */
#define LAT_ZMM_FMA(func_name, asm_str, init_type, init_val)                  \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type accumulator = init_val;                                         \
    init_type mul_operand = init_val;                                         \
    init_type add_operand = init_val;                                         \
    __asm__ volatile("" : "+x"(mul_operand));                                 \
    __asm__ volatile("" : "+x"(add_operand));                                 \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(accumulator) : "x"(mul_operand), "x"(add_operand)); \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(accumulator));                                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_ZMM_FMA: throughput for FMA, 8 independent streams.
 */
#define TP_ZMM_FMA(func_name, asm_str, init_type, init_val)                   \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type stream_0 = init_val;                                            \
    init_type stream_1 = init_val;                                            \
    init_type stream_2 = init_val;                                            \
    init_type stream_3 = init_val;                                            \
    init_type stream_4 = init_val;                                            \
    init_type stream_5 = init_val;                                            \
    init_type stream_6 = init_val;                                            \
    init_type stream_7 = init_val;                                            \
    init_type mul_operand = init_val;                                         \
    init_type add_operand = init_val;                                         \
    __asm__ volatile("" : "+x"(mul_operand));                                 \
    __asm__ volatile("" : "+x"(add_operand));                                 \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+x"(stream_0) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(stream_1) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(stream_2) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(stream_3) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(stream_4) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(stream_5) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(stream_6) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile(asm_str : "+x"(stream_7) : "x"(mul_operand), "x"(add_operand)); \
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),                \
                              "+x"(stream_2), "+x"(stream_3));                \
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),                \
                              "+x"(stream_6), "+x"(stream_7));                \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(stream_0));                                    \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/*
 * TP_ZMM_STANDALONE: throughput where output register != input register.
 * "=x" output, "x" input.  Used for conversions, etc.
 */
#define TP_ZMM_STANDALONE(func_name, asm_str, in_type, in_val, out_type)      \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    in_type src_0 = in_val;                                                   \
    in_type src_1 = in_val;                                                   \
    in_type src_2 = in_val;                                                   \
    in_type src_3 = in_val;                                                   \
    in_type src_4 = in_val;                                                   \
    in_type src_5 = in_val;                                                   \
    in_type src_6 = in_val;                                                   \
    in_type src_7 = in_val;                                                   \
    out_type dst_0, dst_1, dst_2, dst_3;                                      \
    out_type dst_4, dst_5, dst_6, dst_7;                                      \
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1),                          \
                          "+x"(src_2), "+x"(src_3));                          \
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5),                          \
                          "+x"(src_6), "+x"(src_7));                          \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "=x"(dst_0) : "x"(src_0));                \
        __asm__ volatile(asm_str : "=x"(dst_1) : "x"(src_1));                \
        __asm__ volatile(asm_str : "=x"(dst_2) : "x"(src_2));                \
        __asm__ volatile(asm_str : "=x"(dst_3) : "x"(src_3));                \
        __asm__ volatile(asm_str : "=x"(dst_4) : "x"(src_4));                \
        __asm__ volatile(asm_str : "=x"(dst_5) : "x"(src_5));                \
        __asm__ volatile(asm_str : "=x"(dst_6) : "x"(src_6));                \
        __asm__ volatile(asm_str : "=x"(dst_7) : "x"(src_7));                \
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1),                      \
                              "+x"(dst_2), "+x"(dst_3));                      \
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5),                      \
                              "+x"(dst_6), "+x"(dst_7));                      \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(dst_0));                                       \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/*
 * TP_ZMM_CMP_TO_MASK: throughput for vector compare → mask register.
 * Output is a __mmask (integer), input is zmm.
 */
#define TP_ZMM_CMP_TO_MASK(func_name, asm_str, init_type, init_val)           \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type src_0 = init_val;                                               \
    init_type src_1 = init_val;                                               \
    init_type src_2 = init_val;                                               \
    init_type src_3 = init_val;                                               \
    init_type src_4 = init_val;                                               \
    init_type src_5 = init_val;                                               \
    init_type src_6 = init_val;                                               \
    init_type src_7 = init_val;                                               \
    init_type cmp_operand = init_val;                                         \
    uint32_t mask_result_0, mask_result_1, mask_result_2, mask_result_3;      \
    uint32_t mask_result_4, mask_result_5, mask_result_6, mask_result_7;      \
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1),                          \
                          "+x"(src_2), "+x"(src_3));                          \
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5),                          \
                          "+x"(src_6), "+x"(src_7));                          \
    __asm__ volatile("" : "+x"(cmp_operand));                                 \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "=r"(mask_result_0) : "x"(src_0), "x"(cmp_operand)); \
        __asm__ volatile(asm_str : "=r"(mask_result_1) : "x"(src_1), "x"(cmp_operand)); \
        __asm__ volatile(asm_str : "=r"(mask_result_2) : "x"(src_2), "x"(cmp_operand)); \
        __asm__ volatile(asm_str : "=r"(mask_result_3) : "x"(src_3), "x"(cmp_operand)); \
        __asm__ volatile(asm_str : "=r"(mask_result_4) : "x"(src_4), "x"(cmp_operand)); \
        __asm__ volatile(asm_str : "=r"(mask_result_5) : "x"(src_5), "x"(cmp_operand)); \
        __asm__ volatile(asm_str : "=r"(mask_result_6) : "x"(src_6), "x"(cmp_operand)); \
        __asm__ volatile(asm_str : "=r"(mask_result_7) : "x"(src_7), "x"(cmp_operand)); \
        __asm__ volatile("" : "+r"(mask_result_0), "+r"(mask_result_1),       \
                              "+r"(mask_result_2), "+r"(mask_result_3));       \
        __asm__ volatile("" : "+r"(mask_result_4), "+r"(mask_result_5),       \
                              "+r"(mask_result_6), "+r"(mask_result_7));       \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+r"(mask_result_0));                               \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* ========================================================================
 * FP32 ARITHMETIC -- packed zmm (512-bit, 16xfloat)
 * ======================================================================== */

LAT_ZMM_BINARY(vaddps_zmm_lat, "vaddps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vaddps_zmm_tp, "vaddps %1, %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_BINARY(vsubps_zmm_lat, "vsubps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vsubps_zmm_tp, "vsubps %1, %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_BINARY(vmulps_zmm_lat, "vmulps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vmulps_zmm_tp, "vmulps %1, %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_BINARY(vdivps_zmm_lat, "vdivps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vdivps_zmm_tp, "vdivps %1, %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_BINARY(vmaxps_zmm_lat, "vmaxps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vmaxps_zmm_tp, "vmaxps %1, %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_BINARY(vminps_zmm_lat, "vminps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vminps_zmm_tp, "vminps %1, %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_UNARY(vsqrtps_zmm_lat, "vsqrtps %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vsqrtps_zmm_tp, "vsqrtps %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_UNARY(vrcp14ps_zmm_lat, "vrcp14ps %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vrcp14ps_zmm_tp, "vrcp14ps %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_UNARY(vrsqrt14ps_zmm_lat, "vrsqrt14ps %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vrsqrt14ps_zmm_tp, "vrsqrt14ps %0, %0", __m512, ZMM_F32_INIT)

/* VREDUCEPS zmm, zmm, imm8 (AVX512DQ) -- reduce elements by imm */
LAT_ZMM_UNARY(vreduceps_zmm_lat, "vreduceps $4, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vreduceps_zmm_tp, "vreduceps $4, %0, %0", __m512, ZMM_F32_INIT)

/* VRANGEPS zmm, zmm, zmm, imm8 (AVX512DQ) -- range clamp/selection */
LAT_ZMM_BINARY(vrangeps_zmm_lat, "vrangeps $0, %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vrangeps_zmm_tp, "vrangeps $0, %1, %0, %0", __m512, ZMM_F32_INIT)

/* VRNDSCALEPS zmm, zmm, imm8 -- round with scale */
LAT_ZMM_UNARY(vrndscaleps_zmm_lat, "vrndscaleps $0, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vrndscaleps_zmm_tp, "vrndscaleps $0, %0, %0", __m512, ZMM_F32_INIT)

/* VSCALEFPS zmm, zmm, zmm -- scale by power of 2 */
LAT_ZMM_BINARY(vscalefps_zmm_lat, "vscalefps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vscalefps_zmm_tp, "vscalefps %1, %0, %0", __m512, ZMM_F32_INIT)

/* VGETEXPPS zmm, zmm -- extract exponent */
LAT_ZMM_UNARY(vgetexpps_zmm_lat, "vgetexpps %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vgetexpps_zmm_tp, "vgetexpps %0, %0", __m512, ZMM_F32_INIT)

/* VGETMANTPS zmm, zmm, imm8 -- extract mantissa */
LAT_ZMM_UNARY(vgetmantps_zmm_lat, "vgetmantps $0, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vgetmantps_zmm_tp, "vgetmantps $0, %0, %0", __m512, ZMM_F32_INIT)

/* VFIXUPIMMPS zmm, zmm, zmm, imm8 -- fixup special FP values */
LAT_ZMM_FMA(vfixupimmps_zmm_lat, "vfixupimmps $0, %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vfixupimmps_zmm_tp, "vfixupimmps $0, %2, %1, %0", __m512, ZMM_F32_INIT)

/* ========================================================================
 * FP32 FMA variants -- all 4 forms (zmm)
 * ======================================================================== */

LAT_ZMM_FMA(vfmadd231ps_zmm_lat, "vfmadd231ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vfmadd231ps_zmm_tp, "vfmadd231ps %2, %1, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_FMA(vfmsub231ps_zmm_lat, "vfmsub231ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vfmsub231ps_zmm_tp, "vfmsub231ps %2, %1, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_FMA(vfnmadd231ps_zmm_lat, "vfnmadd231ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vfnmadd231ps_zmm_tp, "vfnmadd231ps %2, %1, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_FMA(vfnmsub231ps_zmm_lat, "vfnmsub231ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vfnmsub231ps_zmm_tp, "vfnmsub231ps %2, %1, %0", __m512, ZMM_F32_INIT)

/* Interleaved add/sub FMA */
LAT_ZMM_FMA(vfmaddsub231ps_zmm_lat, "vfmaddsub231ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vfmaddsub231ps_zmm_tp, "vfmaddsub231ps %2, %1, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_FMA(vfmsubadd231ps_zmm_lat, "vfmsubadd231ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vfmsubadd231ps_zmm_tp, "vfmsubadd231ps %2, %1, %0", __m512, ZMM_F32_INIT)

/* ========================================================================
 * FP64 ARITHMETIC -- packed zmm (512-bit, 8xdouble)
 * ======================================================================== */

LAT_ZMM_BINARY(vaddpd_zmm_lat, "vaddpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vaddpd_zmm_tp, "vaddpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vsubpd_zmm_lat, "vsubpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vsubpd_zmm_tp, "vsubpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vmulpd_zmm_lat, "vmulpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vmulpd_zmm_tp, "vmulpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vdivpd_zmm_lat, "vdivpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vdivpd_zmm_tp, "vdivpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vmaxpd_zmm_lat, "vmaxpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vmaxpd_zmm_tp, "vmaxpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vminpd_zmm_lat, "vminpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vminpd_zmm_tp, "vminpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_UNARY(vsqrtpd_zmm_lat, "vsqrtpd %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vsqrtpd_zmm_tp, "vsqrtpd %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_UNARY(vrcp14pd_zmm_lat, "vrcp14pd %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vrcp14pd_zmm_tp, "vrcp14pd %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_UNARY(vrsqrt14pd_zmm_lat, "vrsqrt14pd %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vrsqrt14pd_zmm_tp, "vrsqrt14pd %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_UNARY(vreducepd_zmm_lat, "vreducepd $4, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vreducepd_zmm_tp, "vreducepd $4, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vrangepd_zmm_lat, "vrangepd $0, %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vrangepd_zmm_tp, "vrangepd $0, %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_UNARY(vrndscalepd_zmm_lat, "vrndscalepd $0, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vrndscalepd_zmm_tp, "vrndscalepd $0, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vscalefpd_zmm_lat, "vscalefpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vscalefpd_zmm_tp, "vscalefpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_UNARY(vgetexppd_zmm_lat, "vgetexppd %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vgetexppd_zmm_tp, "vgetexppd %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_UNARY(vgetmantpd_zmm_lat, "vgetmantpd $0, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vgetmantpd_zmm_tp, "vgetmantpd $0, %0, %0", __m512d, ZMM_F64_INIT)

/* FP64 FMA */
LAT_ZMM_FMA(vfmadd231pd_zmm_lat, "vfmadd231pd %2, %1, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_FMA(vfmadd231pd_zmm_tp, "vfmadd231pd %2, %1, %0", __m512d, ZMM_F64_INIT)

/* ========================================================================
 * FP32 Scalar (AVX-512 EVEX encoding)
 * ======================================================================== */

/*
 * EVEX scalar forms: {rn-sae} (static rounding/SAE) cannot be assembled by
 * clang for scalar xmm instructions. Instead we measure the EVEX-only scalar
 * instructions (VRCP14SS, VRSQRT14SS, VGETEXPSS, VGETMANTSS) which are the
 * forms that only exist in EVEX encoding. The basic VADDSS/VMULSS/VDIVSS/
 * VSQRTSS were already measured in the xmm/ymm exhaustive probe and decode
 * identically whether VEX or EVEX encoded on Zen 4.
 */

/* EVEX scalar approximations */
LAT_ZMM_UNARY(vrcp14ss_lat, "vrcp14ss %0, %0, %0", __m128, XMM_F32_INIT)
TP_ZMM_UNARY(vrcp14ss_tp, "vrcp14ss %0, %0, %0", __m128, XMM_F32_INIT)

LAT_ZMM_UNARY(vrsqrt14ss_lat, "vrsqrt14ss %0, %0, %0", __m128, XMM_F32_INIT)
TP_ZMM_UNARY(vrsqrt14ss_tp, "vrsqrt14ss %0, %0, %0", __m128, XMM_F32_INIT)

LAT_ZMM_UNARY(vgetexpss_lat, "vgetexpss %0, %0, %0", __m128, XMM_F32_INIT)
TP_ZMM_UNARY(vgetexpss_tp, "vgetexpss %0, %0, %0", __m128, XMM_F32_INIT)

LAT_ZMM_UNARY(vgetmantss_lat, "vgetmantss $0, %0, %0, %0", __m128, XMM_F32_INIT)
TP_ZMM_UNARY(vgetmantss_tp, "vgetmantss $0, %0, %0, %0", __m128, XMM_F32_INIT)

/* ========================================================================
 * FP Compare -> mask register
 * ======================================================================== */

/* VCMPPS zmm,zmm,imm -> k (compare EQ) */
TP_ZMM_CMP_TO_MASK(vcmpps_zmm_tp,
    "vcmpps $0, %2, %1, %%k1\n\tkmovw %%k1, %0",
    __m512, ZMM_F32_INIT)

/* VCMPPD zmm,zmm,imm -> k */
TP_ZMM_CMP_TO_MASK(vcmppd_zmm_tp,
    "vcmppd $0, %2, %1, %%k1\n\tkmovw %%k1, %0",
    __m512d, ZMM_F64_INIT)

/* VFPCLASSPS zmm, imm -> k (AVX512DQ) */
static double vfpclassps_zmm_tp(uint64_t iteration_count)
{
    __m512 src_0 = ZMM_F32_INIT;
    __m512 src_1 = ZMM_F32_INIT;
    __m512 src_2 = ZMM_F32_INIT;
    __m512 src_3 = ZMM_F32_INIT;
    __m512 src_4 = ZMM_F32_INIT;
    __m512 src_5 = ZMM_F32_INIT;
    __m512 src_6 = ZMM_F32_INIT;
    __m512 src_7 = ZMM_F32_INIT;
    uint32_t mask_result_0, mask_result_1, mask_result_2, mask_result_3;
    uint32_t mask_result_4, mask_result_5, mask_result_6, mask_result_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_0) : "x"(src_0));
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_1) : "x"(src_1));
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_2) : "x"(src_2));
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_3) : "x"(src_3));
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_4) : "x"(src_4));
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_5) : "x"(src_5));
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_6) : "x"(src_6));
        __asm__ volatile("vfpclassps $0x1C, %1, %%k1\n\tkmovw %%k1, %0" : "=r"(mask_result_7) : "x"(src_7));
        __asm__ volatile("" : "+r"(mask_result_0), "+r"(mask_result_1),
                              "+r"(mask_result_2), "+r"(mask_result_3));
        __asm__ volatile("" : "+r"(mask_result_4), "+r"(mask_result_5),
                              "+r"(mask_result_6), "+r"(mask_result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+r"(mask_result_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * FP Conversion (zmm widths)
 * ======================================================================== */

/* VCVTPS2PD ymm -> zmm */
TP_ZMM_STANDALONE(vcvtps2pd_zmm_tp, "vcvtps2pd %1, %0", __m256, YMM_F32_INIT, __m512d)

/* VCVTPD2PS zmm -> ymm */
TP_ZMM_STANDALONE(vcvtpd2ps_zmm_tp, "vcvtpd2ps %1, %0", __m512d, ZMM_F64_INIT, __m256)

/* VCVTDQ2PS zmm */
LAT_ZMM_UNARY(vcvtdq2ps_zmm_lat, "vcvtdq2ps %0, %0", __m512i, ZMM_I32_INIT)
TP_ZMM_UNARY(vcvtdq2ps_zmm_tp, "vcvtdq2ps %0, %0", __m512i, ZMM_I32_INIT)

/* VCVTPS2DQ zmm */
LAT_ZMM_UNARY(vcvtps2dq_zmm_lat, "vcvtps2dq %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vcvtps2dq_zmm_tp, "vcvtps2dq %0, %0", __m512, ZMM_F32_INIT)

/* VCVTTPS2DQ zmm */
LAT_ZMM_UNARY(vcvttps2dq_zmm_lat, "vcvttps2dq %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vcvttps2dq_zmm_tp, "vcvttps2dq %0, %0", __m512, ZMM_F32_INIT)

/* VCVTPD2DQ zmm -> ymm */
TP_ZMM_STANDALONE(vcvtpd2dq_zmm_tp, "vcvtpd2dq %1, %0", __m512d, ZMM_F64_INIT, __m256i)

/* VCVTTPD2DQ zmm -> ymm */
TP_ZMM_STANDALONE(vcvttpd2dq_zmm_tp, "vcvttpd2dq %1, %0", __m512d, ZMM_F64_INIT, __m256i)

/* VCVTQQ2PD zmm (AVX512DQ: int64 -> fp64) */
LAT_ZMM_UNARY(vcvtqq2pd_zmm_lat, "vcvtqq2pd %0, %0", __m512i, ZMM_I64_INIT)
TP_ZMM_UNARY(vcvtqq2pd_zmm_tp, "vcvtqq2pd %0, %0", __m512i, ZMM_I64_INIT)

/* VCVTTPD2QQ zmm (AVX512DQ: fp64 -> int64 truncate) */
LAT_ZMM_UNARY(vcvttpd2qq_zmm_lat, "vcvttpd2qq %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vcvttpd2qq_zmm_tp, "vcvttpd2qq %0, %0", __m512d, ZMM_F64_INIT)

/* VCVTPS2UDQ zmm (unsigned int) */
LAT_ZMM_UNARY(vcvtps2udq_zmm_lat, "vcvtps2udq %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vcvtps2udq_zmm_tp, "vcvtps2udq %0, %0", __m512, ZMM_F32_INIT)

/* VCVTUDQ2PS zmm */
LAT_ZMM_UNARY(vcvtudq2ps_zmm_lat, "vcvtudq2ps %0, %0", __m512i, ZMM_I32_INIT)
TP_ZMM_UNARY(vcvtudq2ps_zmm_tp, "vcvtudq2ps %0, %0", __m512i, ZMM_I32_INIT)

/* VCVTPD2UDQ zmm -> ymm */
TP_ZMM_STANDALONE(vcvtpd2udq_zmm_tp, "vcvtpd2udq %1, %0", __m512d, ZMM_F64_INIT, __m256i)

/* VCVTUDQ2PD ymm -> zmm */
TP_ZMM_STANDALONE(vcvtudq2pd_zmm_tp, "vcvtudq2pd %1, %0", __m256i, YMM_I32_INIT, __m512d)

/* VCVTPH2PS ymm -> zmm (F16C at 512-bit) */
TP_ZMM_STANDALONE(vcvtph2ps_zmm_tp, "vcvtph2ps %1, %0", __m256i, YMM_I32_INIT, __m512)

/* VCVTPS2PH zmm -> ymm */
TP_ZMM_STANDALONE(vcvtps2ph_zmm_tp, "vcvtps2ph $0, %1, %0", __m512, ZMM_F32_INIT, __m256i)

/* VCVTNE2PS2BF16 zmm, zmm, zmm (AVX512BF16) */
LAT_ZMM_FMA(vcvtne2ps2bf16_zmm_lat, "vcvtne2ps2bf16 %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vcvtne2ps2bf16_zmm_tp, "vcvtne2ps2bf16 %2, %1, %0", __m512, ZMM_F32_INIT)

/* VCVTNEPS2BF16 zmm -> ymm (AVX512BF16) */
TP_ZMM_STANDALONE(vcvtneps2bf16_zmm_tp, "vcvtneps2bf16 %1, %0", __m512, ZMM_F32_INIT, __m256i)

/* ========================================================================
 * FP Shuffle / Permute (zmm)
 * ======================================================================== */

/* VSHUFPS zmm */
LAT_ZMM_BINARY(vshufps_zmm_lat, "vshufps $0, %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vshufps_zmm_tp, "vshufps $0, %1, %0, %0", __m512, ZMM_F32_INIT)

/* VSHUFPD zmm */
LAT_ZMM_BINARY(vshufpd_zmm_lat, "vshufpd $0, %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vshufpd_zmm_tp, "vshufpd $0, %1, %0, %0", __m512d, ZMM_F64_INIT)

/* VUNPCKLPS/VUNPCKHPS zmm */
LAT_ZMM_BINARY(vunpcklps_zmm_lat, "vunpcklps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vunpcklps_zmm_tp, "vunpcklps %1, %0, %0", __m512, ZMM_F32_INIT)

LAT_ZMM_BINARY(vunpckhps_zmm_lat, "vunpckhps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vunpckhps_zmm_tp, "vunpckhps %1, %0, %0", __m512, ZMM_F32_INIT)

/* VUNPCKLPD/VUNPCKHPD zmm */
LAT_ZMM_BINARY(vunpcklpd_zmm_lat, "vunpcklpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vunpcklpd_zmm_tp, "vunpcklpd %1, %0, %0", __m512d, ZMM_F64_INIT)

LAT_ZMM_BINARY(vunpckhpd_zmm_lat, "vunpckhpd %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vunpckhpd_zmm_tp, "vunpckhpd %1, %0, %0", __m512d, ZMM_F64_INIT)

/* VPERMILPS zmm, imm8 (in-lane permute) */
LAT_ZMM_UNARY(vpermilps_zmm_lat, "vpermilps $0, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vpermilps_zmm_tp, "vpermilps $0, %0, %0", __m512, ZMM_F32_INIT)

/* VPERMILPD zmm, imm8 */
LAT_ZMM_UNARY(vpermilpd_zmm_lat, "vpermilpd $0, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vpermilpd_zmm_tp, "vpermilpd $0, %0, %0", __m512d, ZMM_F64_INIT)

/* VPERMPS zmm, zmm, zmm (cross-lane) */
LAT_ZMM_BINARY(vpermps_zmm_lat, "vpermps %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vpermps_zmm_tp, "vpermps %1, %0, %0", __m512, ZMM_F32_INIT)

/* VPERMPD zmm, zmm, imm8 (AVX512DQ cross-lane) */
LAT_ZMM_UNARY(vpermpd_zmm_lat, "vpermpd $0, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vpermpd_zmm_tp, "vpermpd $0, %0, %0", __m512d, ZMM_F64_INIT)

/* VPERMI2PS zmm, zmm, zmm (2-source index permute) */
LAT_ZMM_FMA(vpermi2ps_zmm_lat, "vpermi2ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vpermi2ps_zmm_tp, "vpermi2ps %2, %1, %0", __m512, ZMM_F32_INIT)

/* VPERMI2PD zmm, zmm, zmm */
LAT_ZMM_FMA(vpermi2pd_zmm_lat, "vpermi2pd %2, %1, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_FMA(vpermi2pd_zmm_tp, "vpermi2pd %2, %1, %0", __m512d, ZMM_F64_INIT)

/* VSHUFF32X4 zmm, zmm, zmm, imm8 (128-bit lane shuffle) */
LAT_ZMM_BINARY(vshuff32x4_zmm_lat, "vshuff32x4 $0, %1, %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_BINARY(vshuff32x4_zmm_tp, "vshuff32x4 $0, %1, %0, %0", __m512, ZMM_F32_INIT)

/* VSHUFF64X2 zmm, zmm, zmm, imm8 */
LAT_ZMM_BINARY(vshuff64x2_zmm_lat, "vshuff64x2 $0, %1, %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_BINARY(vshuff64x2_zmm_tp, "vshuff64x2 $0, %1, %0, %0", __m512d, ZMM_F64_INIT)

/* VINSERTF32X4 zmm, xmm, imm8 */
static double vinsertf32x4_zmm_tp(uint64_t iteration_count)
{
    __m512 zmm_stream_0 = ZMM_F32_INIT;
    __m512 zmm_stream_1 = ZMM_F32_INIT;
    __m512 zmm_stream_2 = ZMM_F32_INIT;
    __m512 zmm_stream_3 = ZMM_F32_INIT;
    __m128 xmm_operand = XMM_F32_INIT;
    __asm__ volatile("" : "+x"(xmm_operand));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vinsertf32x4 $0, %1, %0, %0" : "+x"(zmm_stream_0) : "x"(xmm_operand));
        __asm__ volatile("vinsertf32x4 $0, %1, %0, %0" : "+x"(zmm_stream_1) : "x"(xmm_operand));
        __asm__ volatile("vinsertf32x4 $0, %1, %0, %0" : "+x"(zmm_stream_2) : "x"(xmm_operand));
        __asm__ volatile("vinsertf32x4 $0, %1, %0, %0" : "+x"(zmm_stream_3) : "x"(xmm_operand));
        __asm__ volatile("" : "+x"(zmm_stream_0), "+x"(zmm_stream_1),
                              "+x"(zmm_stream_2), "+x"(zmm_stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(zmm_stream_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VINSERTF64X2 zmm, xmm, imm8 */
static double vinsertf64x2_zmm_tp(uint64_t iteration_count)
{
    __m512d zmm_stream_0 = ZMM_F64_INIT;
    __m512d zmm_stream_1 = ZMM_F64_INIT;
    __m512d zmm_stream_2 = ZMM_F64_INIT;
    __m512d zmm_stream_3 = ZMM_F64_INIT;
    __m128d xmm_operand = _mm_set1_pd(1.0001);
    __asm__ volatile("" : "+x"(xmm_operand));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vinsertf64x2 $0, %1, %0, %0" : "+x"(zmm_stream_0) : "x"(xmm_operand));
        __asm__ volatile("vinsertf64x2 $0, %1, %0, %0" : "+x"(zmm_stream_1) : "x"(xmm_operand));
        __asm__ volatile("vinsertf64x2 $0, %1, %0, %0" : "+x"(zmm_stream_2) : "x"(xmm_operand));
        __asm__ volatile("vinsertf64x2 $0, %1, %0, %0" : "+x"(zmm_stream_3) : "x"(xmm_operand));
        __asm__ volatile("" : "+x"(zmm_stream_0), "+x"(zmm_stream_1),
                              "+x"(zmm_stream_2), "+x"(zmm_stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(zmm_stream_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VEXTRACTF32X4 zmm -> xmm, imm8 */
TP_ZMM_STANDALONE(vextractf32x4_zmm_tp, "vextractf32x4 $0, %1, %0", __m512, ZMM_F32_INIT, __m128)

/* VEXTRACTF64X2 zmm -> xmm, imm8 */
TP_ZMM_STANDALONE(vextractf64x2_zmm_tp, "vextractf64x2 $0, %1, %0", __m512d, ZMM_F64_INIT, __m128d)

/* ========================================================================
 * FP Blend / Move (zmm)
 * ======================================================================== */

/* VBLENDMPS zmm {k} -- masked blend. We measure throughput with a constant mask. */
static double vblendmps_zmm_tp(uint64_t iteration_count)
{
    __m512 stream_0 = ZMM_F32_INIT;
    __m512 stream_1 = ZMM_F32_INIT;
    __m512 stream_2 = ZMM_F32_INIT;
    __m512 stream_3 = ZMM_F32_INIT;
    __m512 stream_4 = ZMM_F32_INIT;
    __m512 stream_5 = ZMM_F32_INIT;
    __m512 stream_6 = ZMM_F32_INIT;
    __m512 stream_7 = ZMM_F32_INIT;
    __m512 blend_source = ZMM_F32_INIT;
    __asm__ volatile("" : "+x"(blend_source));
    /* Set mask k1 = 0x5555 */
    __asm__ volatile("movw $0x5555, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_0) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_1) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_2) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_3) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_4) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_5) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_6) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmps %1, %0, %0 %{%%k1%}" : "+x"(stream_7) : "x"(blend_source) : "k1");
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),
                              "+x"(stream_6), "+x"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(stream_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VBLENDMPD zmm {k} */
static double vblendmpd_zmm_tp(uint64_t iteration_count)
{
    __m512d stream_0 = ZMM_F64_INIT;
    __m512d stream_1 = ZMM_F64_INIT;
    __m512d stream_2 = ZMM_F64_INIT;
    __m512d stream_3 = ZMM_F64_INIT;
    __m512d stream_4 = ZMM_F64_INIT;
    __m512d stream_5 = ZMM_F64_INIT;
    __m512d stream_6 = ZMM_F64_INIT;
    __m512d stream_7 = ZMM_F64_INIT;
    __m512d blend_source = ZMM_F64_INIT;
    __asm__ volatile("" : "+x"(blend_source));
    __asm__ volatile("movw $0x55, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_0) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_1) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_2) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_3) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_4) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_5) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_6) : "x"(blend_source) : "k1");
        __asm__ volatile("vblendmpd %1, %0, %0 %{%%k1%}" : "+x"(stream_7) : "x"(blend_source) : "k1");
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),
                              "+x"(stream_6), "+x"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(stream_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VMOVAPS zmm, zmm (reg-reg) */
LAT_ZMM_UNARY(vmovaps_zmm_lat, "vmovaps %0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vmovaps_zmm_tp, "vmovaps %0, %0", __m512, ZMM_F32_INIT)

/* VMOVAPD zmm, zmm */
LAT_ZMM_UNARY(vmovapd_zmm_lat, "vmovapd %0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vmovapd_zmm_tp, "vmovapd %0, %0", __m512d, ZMM_F64_INIT)

/* VBROADCASTSS zmm */
LAT_ZMM_UNARY(vbroadcastss_zmm_lat, "vbroadcastss %x0, %0", __m512, ZMM_F32_INIT)
TP_ZMM_UNARY(vbroadcastss_zmm_tp, "vbroadcastss %x0, %0", __m512, ZMM_F32_INIT)

/* VBROADCASTSD zmm */
LAT_ZMM_UNARY(vbroadcastsd_zmm_lat, "vbroadcastsd %x0, %0", __m512d, ZMM_F64_INIT)
TP_ZMM_UNARY(vbroadcastsd_zmm_tp, "vbroadcastsd %x0, %0", __m512d, ZMM_F64_INIT)

/* VCOMPRESSPS zmm (conditional pack -- throughput only, needs mask) */
static double vcompressps_zmm_tp(uint64_t iteration_count)
{
    __m512 src_0 = ZMM_F32_INIT;
    __m512 src_1 = ZMM_F32_INIT;
    __m512 src_2 = ZMM_F32_INIT;
    __m512 src_3 = ZMM_F32_INIT;
    __m512 dst_0, dst_1, dst_2, dst_3;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("movw $0xFFFF, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcompressps %1, %0 %{%%k1%}" : "=x"(dst_0) : "x"(src_0) : "k1");
        __asm__ volatile("vcompressps %1, %0 %{%%k1%}" : "=x"(dst_1) : "x"(src_1) : "k1");
        __asm__ volatile("vcompressps %1, %0 %{%%k1%}" : "=x"(dst_2) : "x"(src_2) : "k1");
        __asm__ volatile("vcompressps %1, %0 %{%%k1%}" : "=x"(dst_3) : "x"(src_3) : "k1");
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VCOMPRESSPD zmm */
static double vcompresspd_zmm_tp(uint64_t iteration_count)
{
    __m512d src_0 = ZMM_F64_INIT;
    __m512d src_1 = ZMM_F64_INIT;
    __m512d src_2 = ZMM_F64_INIT;
    __m512d src_3 = ZMM_F64_INIT;
    __m512d dst_0, dst_1, dst_2, dst_3;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("movw $0xFF, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcompresspd %1, %0 %{%%k1%}" : "=x"(dst_0) : "x"(src_0) : "k1");
        __asm__ volatile("vcompresspd %1, %0 %{%%k1%}" : "=x"(dst_1) : "x"(src_1) : "k1");
        __asm__ volatile("vcompresspd %1, %0 %{%%k1%}" : "=x"(dst_2) : "x"(src_2) : "k1");
        __asm__ volatile("vcompresspd %1, %0 %{%%k1%}" : "=x"(dst_3) : "x"(src_3) : "k1");
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VEXPANDPS zmm (conditional unpack) */
static double vexpandps_zmm_tp(uint64_t iteration_count)
{
    __m512 src_0 = ZMM_F32_INIT;
    __m512 src_1 = ZMM_F32_INIT;
    __m512 src_2 = ZMM_F32_INIT;
    __m512 src_3 = ZMM_F32_INIT;
    __m512 dst_0, dst_1, dst_2, dst_3;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("movw $0xFFFF, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vexpandps %1, %0 %{%%k1%}" : "=x"(dst_0) : "x"(src_0) : "k1");
        __asm__ volatile("vexpandps %1, %0 %{%%k1%}" : "=x"(dst_1) : "x"(src_1) : "k1");
        __asm__ volatile("vexpandps %1, %0 %{%%k1%}" : "=x"(dst_2) : "x"(src_2) : "k1");
        __asm__ volatile("vexpandps %1, %0 %{%%k1%}" : "=x"(dst_3) : "x"(src_3) : "k1");
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VEXPANDPD zmm */
static double vexpandpd_zmm_tp(uint64_t iteration_count)
{
    __m512d src_0 = ZMM_F64_INIT;
    __m512d src_1 = ZMM_F64_INIT;
    __m512d src_2 = ZMM_F64_INIT;
    __m512d src_3 = ZMM_F64_INIT;
    __m512d dst_0, dst_1, dst_2, dst_3;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("movw $0xFF, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vexpandpd %1, %0 %{%%k1%}" : "=x"(dst_0) : "x"(src_0) : "k1");
        __asm__ volatile("vexpandpd %1, %0 %{%%k1%}" : "=x"(dst_1) : "x"(src_1) : "k1");
        __asm__ volatile("vexpandpd %1, %0 %{%%k1%}" : "=x"(dst_2) : "x"(src_2) : "k1");
        __asm__ volatile("vexpandpd %1, %0 %{%%k1%}" : "=x"(dst_3) : "x"(src_3) : "k1");
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VDPBF16PS zmm (AVX512BF16 dot product) */
LAT_ZMM_FMA(vdpbf16ps_zmm_lat, "vdpbf16ps %2, %1, %0", __m512, ZMM_F32_INIT)
TP_ZMM_FMA(vdpbf16ps_zmm_tp, "vdpbf16ps %2, %1, %0", __m512, ZMM_F32_INIT)

/* ========================================================================
 * MEASUREMENT ENTRY TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} Avx512FpProbeEntry;

static const Avx512FpProbeEntry avx512_fp_probe_table[] = {
    /* ---- FP32 Arithmetic (zmm) ---- */
    {"VADDPS",          "zmm,zmm,zmm",        "fp32_arith",  vaddps_zmm_lat,        vaddps_zmm_tp},
    {"VSUBPS",          "zmm,zmm,zmm",        "fp32_arith",  vsubps_zmm_lat,        vsubps_zmm_tp},
    {"VMULPS",          "zmm,zmm,zmm",        "fp32_arith",  vmulps_zmm_lat,        vmulps_zmm_tp},
    {"VDIVPS",          "zmm,zmm,zmm",        "fp32_arith",  vdivps_zmm_lat,        vdivps_zmm_tp},
    {"VMAXPS",          "zmm,zmm,zmm",        "fp32_arith",  vmaxps_zmm_lat,        vmaxps_zmm_tp},
    {"VMINPS",          "zmm,zmm,zmm",        "fp32_arith",  vminps_zmm_lat,        vminps_zmm_tp},
    {"VSQRTPS",         "zmm,zmm",            "fp32_arith",  vsqrtps_zmm_lat,       vsqrtps_zmm_tp},
    {"VRCP14PS",        "zmm,zmm",            "fp32_arith",  vrcp14ps_zmm_lat,      vrcp14ps_zmm_tp},
    {"VRSQRT14PS",      "zmm,zmm",            "fp32_arith",  vrsqrt14ps_zmm_lat,    vrsqrt14ps_zmm_tp},
    {"VREDUCEPS",       "zmm,zmm,imm",        "fp32_arith",  vreduceps_zmm_lat,     vreduceps_zmm_tp},
    {"VRANGEPS",        "zmm,zmm,zmm,imm",    "fp32_arith",  vrangeps_zmm_lat,      vrangeps_zmm_tp},
    {"VRNDSCALEPS",     "zmm,zmm,imm",        "fp32_arith",  vrndscaleps_zmm_lat,   vrndscaleps_zmm_tp},
    {"VSCALEFPS",       "zmm,zmm,zmm",        "fp32_arith",  vscalefps_zmm_lat,     vscalefps_zmm_tp},
    {"VGETEXPPS",       "zmm,zmm",            "fp32_arith",  vgetexpps_zmm_lat,     vgetexpps_zmm_tp},
    {"VGETMANTPS",      "zmm,zmm,imm",        "fp32_arith",  vgetmantps_zmm_lat,    vgetmantps_zmm_tp},
    {"VFIXUPIMMPS",     "zmm,zmm,zmm,imm",    "fp32_arith",  vfixupimmps_zmm_lat,   vfixupimmps_zmm_tp},

    /* ---- FP32 FMA (zmm) ---- */
    {"VFMADD231PS",     "zmm,zmm,zmm",        "fp32_fma",    vfmadd231ps_zmm_lat,   vfmadd231ps_zmm_tp},
    {"VFMSUB231PS",     "zmm,zmm,zmm",        "fp32_fma",    vfmsub231ps_zmm_lat,   vfmsub231ps_zmm_tp},
    {"VFNMADD231PS",    "zmm,zmm,zmm",        "fp32_fma",    vfnmadd231ps_zmm_lat,  vfnmadd231ps_zmm_tp},
    {"VFNMSUB231PS",    "zmm,zmm,zmm",        "fp32_fma",    vfnmsub231ps_zmm_lat,  vfnmsub231ps_zmm_tp},
    {"VFMADDSUB231PS",  "zmm,zmm,zmm",        "fp32_fma",    vfmaddsub231ps_zmm_lat,vfmaddsub231ps_zmm_tp},
    {"VFMSUBADD231PS",  "zmm,zmm,zmm",        "fp32_fma",    vfmsubadd231ps_zmm_lat,vfmsubadd231ps_zmm_tp},

    /* ---- FP64 Arithmetic (zmm) ---- */
    {"VADDPD",          "zmm,zmm,zmm",        "fp64_arith",  vaddpd_zmm_lat,        vaddpd_zmm_tp},
    {"VSUBPD",          "zmm,zmm,zmm",        "fp64_arith",  vsubpd_zmm_lat,        vsubpd_zmm_tp},
    {"VMULPD",          "zmm,zmm,zmm",        "fp64_arith",  vmulpd_zmm_lat,        vmulpd_zmm_tp},
    {"VDIVPD",          "zmm,zmm,zmm",        "fp64_arith",  vdivpd_zmm_lat,        vdivpd_zmm_tp},
    {"VMAXPD",          "zmm,zmm,zmm",        "fp64_arith",  vmaxpd_zmm_lat,        vmaxpd_zmm_tp},
    {"VMINPD",          "zmm,zmm,zmm",        "fp64_arith",  vminpd_zmm_lat,        vminpd_zmm_tp},
    {"VSQRTPD",         "zmm,zmm",            "fp64_arith",  vsqrtpd_zmm_lat,       vsqrtpd_zmm_tp},
    {"VRCP14PD",        "zmm,zmm",            "fp64_arith",  vrcp14pd_zmm_lat,      vrcp14pd_zmm_tp},
    {"VRSQRT14PD",      "zmm,zmm",            "fp64_arith",  vrsqrt14pd_zmm_lat,    vrsqrt14pd_zmm_tp},
    {"VREDUCEPD",       "zmm,zmm,imm",        "fp64_arith",  vreducepd_zmm_lat,     vreducepd_zmm_tp},
    {"VRANGEPD",        "zmm,zmm,zmm,imm",    "fp64_arith",  vrangepd_zmm_lat,      vrangepd_zmm_tp},
    {"VRNDSCALEPD",     "zmm,zmm,imm",        "fp64_arith",  vrndscalepd_zmm_lat,   vrndscalepd_zmm_tp},
    {"VSCALEFPD",       "zmm,zmm,zmm",        "fp64_arith",  vscalefpd_zmm_lat,     vscalefpd_zmm_tp},
    {"VGETEXPPD",       "zmm,zmm",            "fp64_arith",  vgetexppd_zmm_lat,     vgetexppd_zmm_tp},
    {"VGETMANTPD",      "zmm,zmm,imm",        "fp64_arith",  vgetmantpd_zmm_lat,    vgetmantpd_zmm_tp},

    /* ---- FP64 FMA (zmm) ---- */
    {"VFMADD231PD",     "zmm,zmm,zmm",        "fp64_fma",    vfmadd231pd_zmm_lat,   vfmadd231pd_zmm_tp},

    /* ---- FP32 Scalar EVEX-only ---- */
    {"VRCP14SS",        "xmm,xmm",            "fp32_scalar", vrcp14ss_lat,          vrcp14ss_tp},
    {"VRSQRT14SS",      "xmm,xmm",            "fp32_scalar", vrsqrt14ss_lat,        vrsqrt14ss_tp},
    {"VGETEXPSS",       "xmm,xmm",            "fp32_scalar", vgetexpss_lat,         vgetexpss_tp},
    {"VGETMANTSS",      "xmm,xmm,imm",        "fp32_scalar", vgetmantss_lat,        vgetmantss_tp},

    /* ---- FP Compare -> mask ---- */
    {"VCMPPS",          "zmm,zmm,imm->k",     "fp_compare",  NULL,                  vcmpps_zmm_tp},
    {"VCMPPD",          "zmm,zmm,imm->k",     "fp_compare",  NULL,                  vcmppd_zmm_tp},
    {"VFPCLASSPS",      "zmm,imm->k",         "fp_compare",  NULL,                  vfpclassps_zmm_tp},

    /* ---- FP Conversion (zmm) ---- */
    {"VCVTPS2PD",       "ymm->zmm",           "fp_convert",  NULL,                  vcvtps2pd_zmm_tp},
    {"VCVTPD2PS",       "zmm->ymm",           "fp_convert",  NULL,                  vcvtpd2ps_zmm_tp},
    {"VCVTDQ2PS",       "zmm,zmm",            "fp_convert",  vcvtdq2ps_zmm_lat,     vcvtdq2ps_zmm_tp},
    {"VCVTPS2DQ",       "zmm,zmm",            "fp_convert",  vcvtps2dq_zmm_lat,     vcvtps2dq_zmm_tp},
    {"VCVTTPS2DQ",      "zmm,zmm",            "fp_convert",  vcvttps2dq_zmm_lat,    vcvttps2dq_zmm_tp},
    {"VCVTPD2DQ",       "zmm->ymm",           "fp_convert",  NULL,                  vcvtpd2dq_zmm_tp},
    {"VCVTTPD2DQ",      "zmm->ymm",           "fp_convert",  NULL,                  vcvttpd2dq_zmm_tp},
    {"VCVTQQ2PD",       "zmm,zmm",            "fp_convert",  vcvtqq2pd_zmm_lat,     vcvtqq2pd_zmm_tp},
    {"VCVTTPD2QQ",      "zmm,zmm",            "fp_convert",  vcvttpd2qq_zmm_lat,    vcvttpd2qq_zmm_tp},
    {"VCVTPS2UDQ",      "zmm,zmm",            "fp_convert",  vcvtps2udq_zmm_lat,    vcvtps2udq_zmm_tp},
    {"VCVTUDQ2PS",      "zmm,zmm",            "fp_convert",  vcvtudq2ps_zmm_lat,    vcvtudq2ps_zmm_tp},
    {"VCVTPD2UDQ",      "zmm->ymm",           "fp_convert",  NULL,                  vcvtpd2udq_zmm_tp},
    {"VCVTUDQ2PD",      "ymm->zmm",           "fp_convert",  NULL,                  vcvtudq2pd_zmm_tp},
    {"VCVTPH2PS",       "ymm->zmm",           "fp_convert",  NULL,                  vcvtph2ps_zmm_tp},
    {"VCVTPS2PH",       "zmm->ymm",           "fp_convert",  NULL,                  vcvtps2ph_zmm_tp},
    {"VCVTNE2PS2BF16",  "zmm,zmm,zmm",        "fp_convert",  vcvtne2ps2bf16_zmm_lat,vcvtne2ps2bf16_zmm_tp},
    {"VCVTNEPS2BF16",   "zmm->ymm",           "fp_convert",  NULL,                  vcvtneps2bf16_zmm_tp},

    /* ---- FP Shuffle / Permute (zmm) ---- */
    {"VSHUFPS",         "zmm,zmm,zmm,imm",    "fp_shuffle",  vshufps_zmm_lat,       vshufps_zmm_tp},
    {"VSHUFPD",         "zmm,zmm,zmm,imm",    "fp_shuffle",  vshufpd_zmm_lat,       vshufpd_zmm_tp},
    {"VUNPCKLPS",       "zmm,zmm,zmm",        "fp_shuffle",  vunpcklps_zmm_lat,     vunpcklps_zmm_tp},
    {"VUNPCKHPS",       "zmm,zmm,zmm",        "fp_shuffle",  vunpckhps_zmm_lat,     vunpckhps_zmm_tp},
    {"VUNPCKLPD",       "zmm,zmm,zmm",        "fp_shuffle",  vunpcklpd_zmm_lat,     vunpcklpd_zmm_tp},
    {"VUNPCKHPD",       "zmm,zmm,zmm",        "fp_shuffle",  vunpckhpd_zmm_lat,     vunpckhpd_zmm_tp},
    {"VPERMILPS",       "zmm,zmm,imm",        "fp_shuffle",  vpermilps_zmm_lat,     vpermilps_zmm_tp},
    {"VPERMILPD",       "zmm,zmm,imm",        "fp_shuffle",  vpermilpd_zmm_lat,     vpermilpd_zmm_tp},
    {"VPERMPS",         "zmm,zmm,zmm",        "fp_shuffle",  vpermps_zmm_lat,       vpermps_zmm_tp},
    {"VPERMPD",         "zmm,zmm,imm",        "fp_shuffle",  vpermpd_zmm_lat,       vpermpd_zmm_tp},
    {"VPERMI2PS",       "zmm,zmm,zmm",        "fp_shuffle",  vpermi2ps_zmm_lat,     vpermi2ps_zmm_tp},
    {"VPERMI2PD",       "zmm,zmm,zmm",        "fp_shuffle",  vpermi2pd_zmm_lat,     vpermi2pd_zmm_tp},
    {"VSHUFF32X4",      "zmm,zmm,zmm,imm",    "fp_shuffle",  vshuff32x4_zmm_lat,    vshuff32x4_zmm_tp},
    {"VSHUFF64X2",      "zmm,zmm,zmm,imm",    "fp_shuffle",  vshuff64x2_zmm_lat,    vshuff64x2_zmm_tp},
    {"VINSERTF32X4",    "zmm,xmm,imm",        "fp_shuffle",  NULL,                  vinsertf32x4_zmm_tp},
    {"VINSERTF64X2",    "zmm,xmm,imm",        "fp_shuffle",  NULL,                  vinsertf64x2_zmm_tp},
    {"VEXTRACTF32X4",   "zmm->xmm,imm",       "fp_shuffle",  NULL,                  vextractf32x4_zmm_tp},
    {"VEXTRACTF64X2",   "zmm->xmm,imm",       "fp_shuffle",  NULL,                  vextractf64x2_zmm_tp},

    /* ---- FP Blend / Move (zmm) ---- */
    {"VBLENDMPS",       "zmm,zmm {k}",        "fp_blend",    NULL,                  vblendmps_zmm_tp},
    {"VBLENDMPD",       "zmm,zmm {k}",        "fp_blend",    NULL,                  vblendmpd_zmm_tp},
    {"VMOVAPS",         "zmm,zmm",            "fp_move",     vmovaps_zmm_lat,       vmovaps_zmm_tp},
    {"VMOVAPD",         "zmm,zmm",            "fp_move",     vmovapd_zmm_lat,       vmovapd_zmm_tp},
    {"VBROADCASTSS",    "zmm",                "fp_move",     vbroadcastss_zmm_lat,  vbroadcastss_zmm_tp},
    {"VBROADCASTSD",    "zmm",                "fp_move",     vbroadcastsd_zmm_lat,  vbroadcastsd_zmm_tp},
    {"VCOMPRESSPS",     "zmm {k}",            "fp_move",     NULL,                  vcompressps_zmm_tp},
    {"VCOMPRESSPD",     "zmm {k}",            "fp_move",     NULL,                  vcompresspd_zmm_tp},
    {"VEXPANDPS",       "zmm {k}",            "fp_move",     NULL,                  vexpandps_zmm_tp},
    {"VEXPANDPD",       "zmm {k}",            "fp_move",     NULL,                  vexpandpd_zmm_tp},

    /* ---- BF16 (AVX512BF16) ---- */
    {"VDPBF16PS",       "zmm,zmm,zmm",        "bf16",        vdpbf16ps_zmm_lat,     vdpbf16ps_zmm_tp},

    {NULL, NULL, NULL, NULL, NULL}
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
        sm_zen_print_header("avx512_fp_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring AVX-512 ZMM floating-point instruction forms...\n\n");
    }

    /* Count entries */
    int total_entries = 0;
    for (int count_idx = 0; avx512_fp_probe_table[count_idx].mnemonic != NULL; count_idx++)
        total_entries++;

    if (!csv_mode)
        printf("Total instruction forms: %d\n\n", total_entries);

    /* CSV header */
    printf("mnemonic,operands,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; avx512_fp_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const Avx512FpProbeEntry *current_entry = &avx512_fp_probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; avx512_fp_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const Avx512FpProbeEntry *current_entry = &avx512_fp_probe_table[entry_idx];

        double measured_latency = -1.0;
        double measured_throughput = -1.0;

        if (current_entry->measure_latency)
            measured_latency = current_entry->measure_latency(probe_config.iterations);
        if (current_entry->measure_throughput)
            measured_throughput = current_entry->measure_throughput(probe_config.iterations);

        printf("%s,%s,%.4f,%.4f,%s\n",
               current_entry->mnemonic,
               current_entry->operand_description,
               measured_latency,
               measured_throughput,
               current_entry->instruction_category);
    }

    return 0;
}
