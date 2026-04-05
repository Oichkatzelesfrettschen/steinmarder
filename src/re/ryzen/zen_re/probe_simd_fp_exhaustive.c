#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_simd_fp_exhaustive.c -- Exhaustive SIMD floating-point instruction
 * measurement for XMM (128-bit) and YMM (256-bit) widths on Zen 4.
 *
 * Measures latency (4-deep dependency chain) and throughput (8 independent
 * accumulators) for every FP SIMD instruction form.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,precision,width_bits,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -mf16c -fno-omit-frame-pointer -include x86intrin.h \
 *     -I. common.c probe_simd_fp_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_simd_fp_exhaustive
 */

/* ========================================================================
 * MACRO TEMPLATES for XMM (128-bit) and YMM (256-bit) SIMD FP
 *
 * "+x" constraint = xmm/ymm register (SSE/AVX)
 * Barriers: __asm__ volatile("" : "+x"(acc));
 * ======================================================================== */

/*
 * LAT_XMM_BINARY: latency for "op xmm_src, xmm_dst" where dst is
 * both input and output. 4-deep chain.
 */
#define LAT_XMM_BINARY(func_name, asm_str, init_type, init_val)               \
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
 * TP_XMM_BINARY: throughput for "op xmm_src, xmm_dst" with 8 independent
 * accumulator streams.
 */
#define TP_XMM_BINARY(func_name, asm_str, init_type, init_val)                \
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
 * LAT_XMM_UNARY: latency for "op xmm_dst, xmm_dst" (reads and writes same
 * register). Used for SQRT, RSQRT, RCP, etc.
 */
#define LAT_XMM_UNARY(func_name, asm_str, init_type, init_val)               \
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
 * TP_XMM_UNARY: throughput for unary xmm/ymm instruction, 8 streams.
 */
#define TP_XMM_UNARY(func_name, asm_str, init_type, init_val)                \
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
 * LAT_FMA: latency for VEX 3-operand FMA: "vfmadd231ss/ps/sd/pd src2, src1, dst"
 * Dependency chain: dst feeds back as dst (accumulating).
 */
#define LAT_FMA(func_name, asm_str, init_type, init_val)                      \
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
 * TP_FMA: throughput for FMA, 8 independent streams.
 */
#define TP_FMA(func_name, asm_str, init_type, init_val)                       \
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
 * LAT_XMM_TO_GPR: instruction moves xmm data to a GPR (e.g. MOVMSKPS).
 * We chain: gpr result feeds back as a dependency by adding to xmm.
 */
#define LAT_XMM_TO_GPR(func_name, asm_msk, asm_feed, init_type, init_val)    \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type accumulator = init_val;                                         \
    uint64_t gpr_temp = 0;                                                    \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_msk "\n\t" asm_feed                              \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
        __asm__ volatile(asm_msk "\n\t" asm_feed                              \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
        __asm__ volatile(asm_msk "\n\t" asm_feed                              \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
        __asm__ volatile(asm_msk "\n\t" asm_feed                              \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(accumulator));                                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_STANDALONE: throughput for an instruction that produces an output
 * from an input but doesn't naturally chain. 8 independent streams.
 * Uses "=x" output, "x" input constraint.
 */
#define TP_STANDALONE(func_name, asm_str, init_type, init_val)                \
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
    init_type dst_0, dst_1, dst_2, dst_3;                                     \
    init_type dst_4, dst_5, dst_6, dst_7;                                     \
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

/* Convenience init values */
#define XMM_F32_INIT  _mm_set1_ps(1.0001f)
#define XMM_F64_INIT  _mm_set1_pd(1.0001)
#define YMM_F32_INIT  _mm256_set1_ps(1.0001f)
#define YMM_F64_INIT  _mm256_set1_pd(1.0001)
#define XMM_I32_INIT  _mm_set1_epi32(1)
#define YMM_I32_INIT  _mm256_set1_epi32(1)

/* ========================================================================
 * FP32 ARITHMETIC -- scalar SS
 * ======================================================================== */

LAT_XMM_BINARY(addss_lat, "vaddss %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(addss_tp, "vaddss %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(subss_lat, "vsubss %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(subss_tp, "vsubss %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(mulss_lat, "vmulss %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(mulss_tp, "vmulss %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(divss_lat, "vdivss %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(divss_tp, "vdivss %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(maxss_lat, "vmaxss %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(maxss_tp, "vmaxss %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(minss_lat, "vminss %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(minss_tp, "vminss %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_UNARY(sqrtss_lat, "vsqrtss %0, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(sqrtss_tp, "vsqrtss %0, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_UNARY(rsqrtss_lat, "vrsqrtss %0, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(rsqrtss_tp, "vrsqrtss %0, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_UNARY(rcpss_lat, "vrcpss %0, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(rcpss_tp, "vrcpss %0, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(roundss_lat, "vroundss $0, %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(roundss_tp, "vroundss $0, %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(cmpss_lat, "vcmpss $0, %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(cmpss_tp, "vcmpss $0, %1, %0, %0", __m128, XMM_F32_INIT)

/* FMA scalar float */
LAT_FMA(vfmadd231ss_lat, "vfmadd231ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfmadd231ss_tp, "vfmadd231ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA(vfmsub231ss_lat, "vfmsub231ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfmsub231ss_tp, "vfmsub231ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA(vfnmadd231ss_lat, "vfnmadd231ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfnmadd231ss_tp, "vfnmadd231ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA(vfnmsub231ss_lat, "vfnmsub231ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfnmsub231ss_tp, "vfnmsub231ss %2, %1, %0", __m128, XMM_F32_INIT)

/* ========================================================================
 * FP32 ARITHMETIC -- packed xmm (128-bit, 4xfloat)
 * ======================================================================== */

LAT_XMM_BINARY(addps_xmm_lat, "vaddps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(addps_xmm_tp, "vaddps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(subps_xmm_lat, "vsubps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(subps_xmm_tp, "vsubps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(mulps_xmm_lat, "vmulps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(mulps_xmm_tp, "vmulps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(divps_xmm_lat, "vdivps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(divps_xmm_tp, "vdivps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(maxps_xmm_lat, "vmaxps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(maxps_xmm_tp, "vmaxps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(minps_xmm_lat, "vminps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(minps_xmm_tp, "vminps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_UNARY(sqrtps_xmm_lat, "vsqrtps %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(sqrtps_xmm_tp, "vsqrtps %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_UNARY(rsqrtps_xmm_lat, "vrsqrtps %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(rsqrtps_xmm_tp, "vrsqrtps %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_UNARY(rcpps_xmm_lat, "vrcpps %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(rcpps_xmm_tp, "vrcpps %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(roundps_xmm_lat, "vroundps $0, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(roundps_xmm_tp, "vroundps $0, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(haddps_xmm_lat, "vhaddps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(haddps_xmm_tp, "vhaddps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(hsubps_xmm_lat, "vhsubps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(hsubps_xmm_tp, "vhsubps %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(dpps_xmm_lat, "vdpps $0xFF, %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(dpps_xmm_tp, "vdpps $0xFF, %1, %0, %0", __m128, XMM_F32_INIT)

LAT_XMM_BINARY(cmpps_xmm_lat, "vcmpps $0, %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(cmpps_xmm_tp, "vcmpps $0, %1, %0, %0", __m128, XMM_F32_INIT)

/* FMA packed xmm float */
LAT_FMA(vfmadd231ps_xmm_lat, "vfmadd231ps %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfmadd231ps_xmm_tp, "vfmadd231ps %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA(vfmsub231ps_xmm_lat, "vfmsub231ps %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfmsub231ps_xmm_tp, "vfmsub231ps %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA(vfnmadd231ps_xmm_lat, "vfnmadd231ps %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfnmadd231ps_xmm_tp, "vfnmadd231ps %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA(vfnmsub231ps_xmm_lat, "vfnmsub231ps %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA(vfnmsub231ps_xmm_tp, "vfnmsub231ps %2, %1, %0", __m128, XMM_F32_INIT)

/* ========================================================================
 * FP32 ARITHMETIC -- packed ymm (256-bit, 8xfloat)
 * ======================================================================== */

LAT_XMM_BINARY(addps_ymm_lat, "vaddps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(addps_ymm_tp, "vaddps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(subps_ymm_lat, "vsubps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(subps_ymm_tp, "vsubps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(mulps_ymm_lat, "vmulps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(mulps_ymm_tp, "vmulps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(divps_ymm_lat, "vdivps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(divps_ymm_tp, "vdivps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(maxps_ymm_lat, "vmaxps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(maxps_ymm_tp, "vmaxps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(minps_ymm_lat, "vminps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(minps_ymm_tp, "vminps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_UNARY(sqrtps_ymm_lat, "vsqrtps %0, %0", __m256, YMM_F32_INIT)
TP_XMM_UNARY(sqrtps_ymm_tp, "vsqrtps %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_UNARY(rsqrtps_ymm_lat, "vrsqrtps %0, %0", __m256, YMM_F32_INIT)
TP_XMM_UNARY(rsqrtps_ymm_tp, "vrsqrtps %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_UNARY(rcpps_ymm_lat, "vrcpps %0, %0", __m256, YMM_F32_INIT)
TP_XMM_UNARY(rcpps_ymm_tp, "vrcpps %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(roundps_ymm_lat, "vroundps $0, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_UNARY(roundps_ymm_tp, "vroundps $0, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(haddps_ymm_lat, "vhaddps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(haddps_ymm_tp, "vhaddps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(hsubps_ymm_lat, "vhsubps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(hsubps_ymm_tp, "vhsubps %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(dpps_ymm_lat, "vdpps $0xFF, %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(dpps_ymm_tp, "vdpps $0xFF, %1, %0, %0", __m256, YMM_F32_INIT)

LAT_XMM_BINARY(cmpps_ymm_lat, "vcmpps $0, %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(cmpps_ymm_tp, "vcmpps $0, %1, %0, %0", __m256, YMM_F32_INIT)

/* FMA packed ymm float */
LAT_FMA(vfmadd231ps_ymm_lat, "vfmadd231ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA(vfmadd231ps_ymm_tp, "vfmadd231ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA(vfmsub231ps_ymm_lat, "vfmsub231ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA(vfmsub231ps_ymm_tp, "vfmsub231ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA(vfnmadd231ps_ymm_lat, "vfnmadd231ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA(vfnmadd231ps_ymm_tp, "vfnmadd231ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA(vfnmsub231ps_ymm_lat, "vfnmsub231ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA(vfnmsub231ps_ymm_tp, "vfnmsub231ps %2, %1, %0", __m256, YMM_F32_INIT)

/* ========================================================================
 * FP64 ARITHMETIC -- scalar SD
 * ======================================================================== */

LAT_XMM_BINARY(addsd_lat, "vaddsd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(addsd_tp, "vaddsd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(subsd_lat, "vsubsd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(subsd_tp, "vsubsd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(mulsd_lat, "vmulsd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(mulsd_tp, "vmulsd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(divsd_lat, "vdivsd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(divsd_tp, "vdivsd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(maxsd_lat, "vmaxsd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(maxsd_tp, "vmaxsd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(minsd_lat, "vminsd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(minsd_tp, "vminsd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_UNARY(sqrtsd_lat, "vsqrtsd %0, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_UNARY(sqrtsd_tp, "vsqrtsd %0, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(roundsd_lat, "vroundsd $0, %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(roundsd_tp, "vroundsd $0, %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(cmpsd_lat, "vcmpsd $0, %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(cmpsd_tp, "vcmpsd $0, %1, %0, %0", __m128d, XMM_F64_INIT)

/* FMA scalar double */
LAT_FMA(vfmadd231sd_lat, "vfmadd231sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA(vfmadd231sd_tp, "vfmadd231sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* ========================================================================
 * FP64 ARITHMETIC -- packed xmm (128-bit, 2xdouble)
 * ======================================================================== */

LAT_XMM_BINARY(addpd_xmm_lat, "vaddpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(addpd_xmm_tp, "vaddpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(subpd_xmm_lat, "vsubpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(subpd_xmm_tp, "vsubpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(mulpd_xmm_lat, "vmulpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(mulpd_xmm_tp, "vmulpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(divpd_xmm_lat, "vdivpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(divpd_xmm_tp, "vdivpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(maxpd_xmm_lat, "vmaxpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(maxpd_xmm_tp, "vmaxpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(minpd_xmm_lat, "vminpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(minpd_xmm_tp, "vminpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_UNARY(sqrtpd_xmm_lat, "vsqrtpd %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_UNARY(sqrtpd_xmm_tp, "vsqrtpd %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(roundpd_xmm_lat, "vroundpd $0, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_UNARY(roundpd_xmm_tp, "vroundpd $0, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(haddpd_xmm_lat, "vhaddpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(haddpd_xmm_tp, "vhaddpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(hsubpd_xmm_lat, "vhsubpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(hsubpd_xmm_tp, "vhsubpd %1, %0, %0", __m128d, XMM_F64_INIT)

LAT_XMM_BINARY(cmppd_xmm_lat, "vcmppd $0, %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(cmppd_xmm_tp, "vcmppd $0, %1, %0, %0", __m128d, XMM_F64_INIT)

/* FMA packed xmm double */
LAT_FMA(vfmadd231pd_xmm_lat, "vfmadd231pd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA(vfmadd231pd_xmm_tp, "vfmadd231pd %2, %1, %0", __m128d, XMM_F64_INIT)

/* ========================================================================
 * FP64 ARITHMETIC -- packed ymm (256-bit, 4xdouble)
 * ======================================================================== */

LAT_XMM_BINARY(addpd_ymm_lat, "vaddpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(addpd_ymm_tp, "vaddpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(subpd_ymm_lat, "vsubpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(subpd_ymm_tp, "vsubpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(mulpd_ymm_lat, "vmulpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(mulpd_ymm_tp, "vmulpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(divpd_ymm_lat, "vdivpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(divpd_ymm_tp, "vdivpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(maxpd_ymm_lat, "vmaxpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(maxpd_ymm_tp, "vmaxpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(minpd_ymm_lat, "vminpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(minpd_ymm_tp, "vminpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_UNARY(sqrtpd_ymm_lat, "vsqrtpd %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_UNARY(sqrtpd_ymm_tp, "vsqrtpd %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(roundpd_ymm_lat, "vroundpd $0, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_UNARY(roundpd_ymm_tp, "vroundpd $0, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(haddpd_ymm_lat, "vhaddpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(haddpd_ymm_tp, "vhaddpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(hsubpd_ymm_lat, "vhsubpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(hsubpd_ymm_tp, "vhsubpd %1, %0, %0", __m256d, YMM_F64_INIT)

LAT_XMM_BINARY(cmppd_ymm_lat, "vcmppd $0, %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(cmppd_ymm_tp, "vcmppd $0, %1, %0, %0", __m256d, YMM_F64_INIT)

/* FMA packed ymm double */
LAT_FMA(vfmadd231pd_ymm_lat, "vfmadd231pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA(vfmadd231pd_ymm_tp, "vfmadd231pd %2, %1, %0", __m256d, YMM_F64_INIT)

/* ========================================================================
 * MOVE / SHUFFLE -- xmm and ymm
 * ======================================================================== */

/* MOVAPS xmm,xmm (reg-to-reg) */
LAT_XMM_BINARY(movaps_xmm_lat, "vmovaps %1, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(movaps_xmm_tp, "vmovaps %1, %0", __m128, XMM_F32_INIT)

/* MOVAPD xmm,xmm */
LAT_XMM_BINARY(movapd_xmm_lat, "vmovapd %1, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(movapd_xmm_tp, "vmovapd %1, %0", __m128d, XMM_F64_INIT)

/* MOVAPS ymm,ymm */
LAT_XMM_BINARY(movaps_ymm_lat, "vmovaps %1, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(movaps_ymm_tp, "vmovaps %1, %0", __m256, YMM_F32_INIT)

/* MOVAPD ymm,ymm */
LAT_XMM_BINARY(movapd_ymm_lat, "vmovapd %1, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(movapd_ymm_tp, "vmovapd %1, %0", __m256d, YMM_F64_INIT)

/* SHUFPS xmm */
LAT_XMM_BINARY(shufps_xmm_lat, "vshufps $0x1B, %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(shufps_xmm_tp, "vshufps $0x1B, %1, %0, %0", __m128, XMM_F32_INIT)

/* SHUFPS ymm */
LAT_XMM_BINARY(shufps_ymm_lat, "vshufps $0x1B, %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(shufps_ymm_tp, "vshufps $0x1B, %1, %0, %0", __m256, YMM_F32_INIT)

/* SHUFPD xmm */
LAT_XMM_BINARY(shufpd_xmm_lat, "vshufpd $1, %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(shufpd_xmm_tp, "vshufpd $1, %1, %0, %0", __m128d, XMM_F64_INIT)

/* SHUFPD ymm */
LAT_XMM_BINARY(shufpd_ymm_lat, "vshufpd $5, %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(shufpd_ymm_tp, "vshufpd $5, %1, %0, %0", __m256d, YMM_F64_INIT)

/* UNPCKLPS xmm */
LAT_XMM_BINARY(unpcklps_xmm_lat, "vunpcklps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(unpcklps_xmm_tp, "vunpcklps %1, %0, %0", __m128, XMM_F32_INIT)

/* UNPCKHPS xmm */
LAT_XMM_BINARY(unpckhps_xmm_lat, "vunpckhps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(unpckhps_xmm_tp, "vunpckhps %1, %0, %0", __m128, XMM_F32_INIT)

/* UNPCKLPD xmm */
LAT_XMM_BINARY(unpcklpd_xmm_lat, "vunpcklpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(unpcklpd_xmm_tp, "vunpcklpd %1, %0, %0", __m128d, XMM_F64_INIT)

/* UNPCKHPD xmm */
LAT_XMM_BINARY(unpckhpd_xmm_lat, "vunpckhpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(unpckhpd_xmm_tp, "vunpckhpd %1, %0, %0", __m128d, XMM_F64_INIT)

/* UNPCKLPS ymm */
LAT_XMM_BINARY(unpcklps_ymm_lat, "vunpcklps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(unpcklps_ymm_tp, "vunpcklps %1, %0, %0", __m256, YMM_F32_INIT)

/* UNPCKHPS ymm */
LAT_XMM_BINARY(unpckhps_ymm_lat, "vunpckhps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(unpckhps_ymm_tp, "vunpckhps %1, %0, %0", __m256, YMM_F32_INIT)

/* UNPCKLPD ymm */
LAT_XMM_BINARY(unpcklpd_ymm_lat, "vunpcklpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(unpcklpd_ymm_tp, "vunpcklpd %1, %0, %0", __m256d, YMM_F64_INIT)

/* UNPCKHPD ymm */
LAT_XMM_BINARY(unpckhpd_ymm_lat, "vunpckhpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(unpckhpd_ymm_tp, "vunpckhpd %1, %0, %0", __m256d, YMM_F64_INIT)

/* VPERMILPS xmm (in-lane permute, imm8) */
LAT_XMM_UNARY(vpermilps_xmm_lat, "vpermilps $0x1B, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(vpermilps_xmm_tp, "vpermilps $0x1B, %0, %0", __m128, XMM_F32_INIT)

/* VPERMILPS ymm */
LAT_XMM_UNARY(vpermilps_ymm_lat, "vpermilps $0x1B, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_UNARY(vpermilps_ymm_tp, "vpermilps $0x1B, %0, %0", __m256, YMM_F32_INIT)

/* VPERMILPD xmm */
LAT_XMM_UNARY(vpermilpd_xmm_lat, "vpermilpd $1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_UNARY(vpermilpd_xmm_tp, "vpermilpd $1, %0, %0", __m128d, XMM_F64_INIT)

/* VPERMILPD ymm */
LAT_XMM_UNARY(vpermilpd_ymm_lat, "vpermilpd $5, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_UNARY(vpermilpd_ymm_tp, "vpermilpd $5, %0, %0", __m256d, YMM_F64_INIT)

/* VPERM2F128 ymm (cross-lane 128-bit swap) */
LAT_XMM_BINARY(vperm2f128_lat, "vperm2f128 $0x01, %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(vperm2f128_tp, "vperm2f128 $0x01, %1, %0, %0", __m256, YMM_F32_INIT)

/* VINSERTF128 */
static double vinsertf128_lat(uint64_t iteration_count)
{
    __m256 accumulator = YMM_F32_INIT;
    __m128 insert_source = XMM_F32_INIT;
    __asm__ volatile("" : "+x"(insert_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(accumulator) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(accumulator) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(accumulator) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(accumulator) : "x"(insert_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(accumulator));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vinsertf128_tp(uint64_t iteration_count)
{
    __m256 stream_0 = YMM_F32_INIT, stream_1 = YMM_F32_INIT;
    __m256 stream_2 = YMM_F32_INIT, stream_3 = YMM_F32_INIT;
    __m256 stream_4 = YMM_F32_INIT, stream_5 = YMM_F32_INIT;
    __m256 stream_6 = YMM_F32_INIT, stream_7 = YMM_F32_INIT;
    __m128 insert_source = XMM_F32_INIT;
    __asm__ volatile("" : "+x"(insert_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_0) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_1) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_2) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_3) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_4) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_5) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_6) : "x"(insert_source));
        __asm__ volatile("vinsertf128 $1, %1, %0, %0" : "+x"(stream_7) : "x"(insert_source));
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),
                              "+x"(stream_6), "+x"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(stream_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VEXTRACTF128 -- extracts 128-bit lane from ymm to xmm */
static double vextractf128_lat(uint64_t iteration_count)
{
    __m256 ymm_accumulator = YMM_F32_INIT;
    __m128 xmm_extracted;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(xmm_extracted) : "x"(ymm_accumulator));
        __asm__ volatile("vinsertf128 $0, %1, %0, %0" : "+x"(ymm_accumulator) : "x"(xmm_extracted));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(xmm_extracted) : "x"(ymm_accumulator));
        __asm__ volatile("vinsertf128 $0, %1, %0, %0" : "+x"(ymm_accumulator) : "x"(xmm_extracted));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(ymm_accumulator));
    /* 2 extract+insert pairs, report per-extract */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* VEXTRACTF128 throughput: ymm->xmm, 8 independent streams */
static double vextractf128_tp(uint64_t iteration_count)
{
    __m256 src_0 = YMM_F32_INIT, src_1 = YMM_F32_INIT;
    __m256 src_2 = YMM_F32_INIT, src_3 = YMM_F32_INIT;
    __m256 src_4 = YMM_F32_INIT, src_5 = YMM_F32_INIT;
    __m256 src_6 = YMM_F32_INIT, src_7 = YMM_F32_INIT;
    __m128 dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_0) : "x"(src_0));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_1) : "x"(src_1));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_2) : "x"(src_2));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_3) : "x"(src_3));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_4) : "x"(src_4));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_5) : "x"(src_5));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_6) : "x"(src_6));
        __asm__ volatile("vextractf128 $1, %1, %0" : "=x"(dst_7) : "x"(src_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VBROADCASTSS xmm (broadcast scalar to all lanes) */
LAT_XMM_UNARY(vbroadcastss_xmm_lat, "vbroadcastss %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(vbroadcastss_xmm_tp, "vbroadcastss %0, %0", __m128, XMM_F32_INIT)

/* VBROADCASTSS ymm -- src must be xmm, dst is ymm.
 * For latency: chain via vextractf128 to get xmm back from ymm result. */
static double vbroadcastss_ymm_lat(uint64_t iteration_count)
{
    __m128 xmm_src = XMM_F32_INIT;
    __m256 ymm_dst;
    __asm__ volatile("" : "+x"(xmm_src));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vbroadcastss %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
        __asm__ volatile("vbroadcastss %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
        __asm__ volatile("vbroadcastss %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
        __asm__ volatile("vbroadcastss %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(ymm_dst));
    /* Reports combined latency of broadcast+extract pair */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vbroadcastss_ymm_tp(uint64_t iteration_count)
{
    __m128 xmm_0 = XMM_F32_INIT, xmm_1 = XMM_F32_INIT;
    __m128 xmm_2 = XMM_F32_INIT, xmm_3 = XMM_F32_INIT;
    __m128 xmm_4 = XMM_F32_INIT, xmm_5 = XMM_F32_INIT;
    __m128 xmm_6 = XMM_F32_INIT, xmm_7 = XMM_F32_INIT;
    __m256 dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+x"(xmm_0), "+x"(xmm_1), "+x"(xmm_2), "+x"(xmm_3));
    __asm__ volatile("" : "+x"(xmm_4), "+x"(xmm_5), "+x"(xmm_6), "+x"(xmm_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_0) : "x"(xmm_0));
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_1) : "x"(xmm_1));
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_2) : "x"(xmm_2));
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_3) : "x"(xmm_3));
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_4) : "x"(xmm_4));
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_5) : "x"(xmm_5));
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_6) : "x"(xmm_6));
        __asm__ volatile("vbroadcastss %1, %0" : "=x"(dst_7) : "x"(xmm_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VBROADCASTSD ymm -- src must be xmm, dst is ymm.
 * For latency: chain via vextractf128 to get xmm back. */
static double vbroadcastsd_ymm_lat(uint64_t iteration_count)
{
    __m128d xmm_src = XMM_F64_INIT;
    __m256d ymm_dst;
    __asm__ volatile("" : "+x"(xmm_src));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vbroadcastsd %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
        __asm__ volatile("vbroadcastsd %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
        __asm__ volatile("vbroadcastsd %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
        __asm__ volatile("vbroadcastsd %1, %0\n\t"
                         "vextractf128 $0, %0, %1"
                         : "=x"(ymm_dst), "+x"(xmm_src));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(ymm_dst));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vbroadcastsd_ymm_tp(uint64_t iteration_count)
{
    __m128d xmm_0 = XMM_F64_INIT, xmm_1 = XMM_F64_INIT;
    __m128d xmm_2 = XMM_F64_INIT, xmm_3 = XMM_F64_INIT;
    __m128d xmm_4 = XMM_F64_INIT, xmm_5 = XMM_F64_INIT;
    __m128d xmm_6 = XMM_F64_INIT, xmm_7 = XMM_F64_INIT;
    __m256d dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+x"(xmm_0), "+x"(xmm_1), "+x"(xmm_2), "+x"(xmm_3));
    __asm__ volatile("" : "+x"(xmm_4), "+x"(xmm_5), "+x"(xmm_6), "+x"(xmm_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_0) : "x"(xmm_0));
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_1) : "x"(xmm_1));
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_2) : "x"(xmm_2));
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_3) : "x"(xmm_3));
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_4) : "x"(xmm_4));
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_5) : "x"(xmm_5));
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_6) : "x"(xmm_6));
        __asm__ volatile("vbroadcastsd %1, %0" : "=x"(dst_7) : "x"(xmm_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* BLENDPS xmm */
LAT_XMM_BINARY(blendps_xmm_lat, "vblendps $5, %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(blendps_xmm_tp, "vblendps $5, %1, %0, %0", __m128, XMM_F32_INIT)

/* BLENDPS ymm */
LAT_XMM_BINARY(blendps_ymm_lat, "vblendps $0x55, %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(blendps_ymm_tp, "vblendps $0x55, %1, %0, %0", __m256, YMM_F32_INIT)

/* BLENDPD xmm */
LAT_XMM_BINARY(blendpd_xmm_lat, "vblendpd $1, %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(blendpd_xmm_tp, "vblendpd $1, %1, %0, %0", __m128d, XMM_F64_INIT)

/* BLENDPD ymm */
LAT_XMM_BINARY(blendpd_ymm_lat, "vblendpd $5, %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(blendpd_ymm_tp, "vblendpd $5, %1, %0, %0", __m256d, YMM_F64_INIT)

/* BLENDVPS xmm (variable blend, mask in third operand) */
LAT_FMA(blendvps_xmm_lat, "vblendvps %2, %1, %0, %0", __m128, XMM_F32_INIT)
TP_FMA(blendvps_xmm_tp, "vblendvps %2, %1, %0, %0", __m128, XMM_F32_INIT)

/* BLENDVPS ymm */
LAT_FMA(blendvps_ymm_lat, "vblendvps %2, %1, %0, %0", __m256, YMM_F32_INIT)
TP_FMA(blendvps_ymm_tp, "vblendvps %2, %1, %0, %0", __m256, YMM_F32_INIT)

/* BLENDVPD xmm */
LAT_FMA(blendvpd_xmm_lat, "vblendvpd %2, %1, %0, %0", __m128d, XMM_F64_INIT)
TP_FMA(blendvpd_xmm_tp, "vblendvpd %2, %1, %0, %0", __m128d, XMM_F64_INIT)

/* BLENDVPD ymm */
LAT_FMA(blendvpd_ymm_lat, "vblendvpd %2, %1, %0, %0", __m256d, YMM_F64_INIT)
TP_FMA(blendvpd_ymm_tp, "vblendvpd %2, %1, %0, %0", __m256d, YMM_F64_INIT)

/* MOVMSKPS xmm */
static double movmskps_xmm_tp(uint64_t iteration_count)
{
    __m128 src_0 = XMM_F32_INIT, src_1 = XMM_F32_INIT;
    __m128 src_2 = XMM_F32_INIT, src_3 = XMM_F32_INIT;
    __m128 src_4 = XMM_F32_INIT, src_5 = XMM_F32_INIT;
    __m128 src_6 = XMM_F32_INIT, src_7 = XMM_F32_INIT;
    int gpr_0, gpr_1, gpr_2, gpr_3, gpr_4, gpr_5, gpr_6, gpr_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_0) : "x"(src_0));
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_1) : "x"(src_1));
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_2) : "x"(src_2));
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_3) : "x"(src_3));
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_4) : "x"(src_4));
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_5) : "x"(src_5));
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_6) : "x"(src_6));
        __asm__ volatile("vmovmskps %1, %0" : "=r"(gpr_7) : "x"(src_7));
        __asm__ volatile("" : "+r"(gpr_0), "+r"(gpr_1), "+r"(gpr_2), "+r"(gpr_3));
        __asm__ volatile("" : "+r"(gpr_4), "+r"(gpr_5), "+r"(gpr_6), "+r"(gpr_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = gpr_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOVMSKPD xmm */
static double movmskpd_xmm_tp(uint64_t iteration_count)
{
    __m128d src_0 = XMM_F64_INIT, src_1 = XMM_F64_INIT;
    __m128d src_2 = XMM_F64_INIT, src_3 = XMM_F64_INIT;
    __m128d src_4 = XMM_F64_INIT, src_5 = XMM_F64_INIT;
    __m128d src_6 = XMM_F64_INIT, src_7 = XMM_F64_INIT;
    int gpr_0, gpr_1, gpr_2, gpr_3, gpr_4, gpr_5, gpr_6, gpr_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_0) : "x"(src_0));
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_1) : "x"(src_1));
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_2) : "x"(src_2));
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_3) : "x"(src_3));
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_4) : "x"(src_4));
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_5) : "x"(src_5));
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_6) : "x"(src_6));
        __asm__ volatile("vmovmskpd %1, %0" : "=r"(gpr_7) : "x"(src_7));
        __asm__ volatile("" : "+r"(gpr_0), "+r"(gpr_1), "+r"(gpr_2), "+r"(gpr_3));
        __asm__ volatile("" : "+r"(gpr_4), "+r"(gpr_5), "+r"(gpr_6), "+r"(gpr_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = gpr_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * BITWISE on FP registers
 * ======================================================================== */

/* ANDPS xmm */
LAT_XMM_BINARY(andps_xmm_lat, "vandps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(andps_xmm_tp, "vandps %1, %0, %0", __m128, XMM_F32_INIT)

/* ANDPS ymm */
LAT_XMM_BINARY(andps_ymm_lat, "vandps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(andps_ymm_tp, "vandps %1, %0, %0", __m256, YMM_F32_INIT)

/* ANDPD xmm */
LAT_XMM_BINARY(andpd_xmm_lat, "vandpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(andpd_xmm_tp, "vandpd %1, %0, %0", __m128d, XMM_F64_INIT)

/* ANDPD ymm */
LAT_XMM_BINARY(andpd_ymm_lat, "vandpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(andpd_ymm_tp, "vandpd %1, %0, %0", __m256d, YMM_F64_INIT)

/* ANDNPS xmm */
LAT_XMM_BINARY(andnps_xmm_lat, "vandnps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(andnps_xmm_tp, "vandnps %1, %0, %0", __m128, XMM_F32_INIT)

/* ANDNPS ymm */
LAT_XMM_BINARY(andnps_ymm_lat, "vandnps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(andnps_ymm_tp, "vandnps %1, %0, %0", __m256, YMM_F32_INIT)

/* ORPS xmm */
LAT_XMM_BINARY(orps_xmm_lat, "vorps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(orps_xmm_tp, "vorps %1, %0, %0", __m128, XMM_F32_INIT)

/* ORPS ymm */
LAT_XMM_BINARY(orps_ymm_lat, "vorps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(orps_ymm_tp, "vorps %1, %0, %0", __m256, YMM_F32_INIT)

/* ORPD xmm */
LAT_XMM_BINARY(orpd_xmm_lat, "vorpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(orpd_xmm_tp, "vorpd %1, %0, %0", __m128d, XMM_F64_INIT)

/* ORPD ymm */
LAT_XMM_BINARY(orpd_ymm_lat, "vorpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(orpd_ymm_tp, "vorpd %1, %0, %0", __m256d, YMM_F64_INIT)

/* XORPS xmm */
LAT_XMM_BINARY(xorps_xmm_lat, "vxorps %1, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_BINARY(xorps_xmm_tp, "vxorps %1, %0, %0", __m128, XMM_F32_INIT)

/* XORPS ymm */
LAT_XMM_BINARY(xorps_ymm_lat, "vxorps %1, %0, %0", __m256, YMM_F32_INIT)
TP_XMM_BINARY(xorps_ymm_tp, "vxorps %1, %0, %0", __m256, YMM_F32_INIT)

/* XORPD xmm */
LAT_XMM_BINARY(xorpd_xmm_lat, "vxorpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(xorpd_xmm_tp, "vxorpd %1, %0, %0", __m128d, XMM_F64_INIT)

/* XORPD ymm */
LAT_XMM_BINARY(xorpd_ymm_lat, "vxorpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(xorpd_ymm_tp, "vxorpd %1, %0, %0", __m256d, YMM_F64_INIT)

/* ANDNPD xmm */
LAT_XMM_BINARY(andnpd_xmm_lat, "vandnpd %1, %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_BINARY(andnpd_xmm_tp, "vandnpd %1, %0, %0", __m128d, XMM_F64_INIT)

/* ANDNPD ymm */
LAT_XMM_BINARY(andnpd_ymm_lat, "vandnpd %1, %0, %0", __m256d, YMM_F64_INIT)
TP_XMM_BINARY(andnpd_ymm_tp, "vandnpd %1, %0, %0", __m256d, YMM_F64_INIT)

/* ========================================================================
 * CONVERSION
 * ======================================================================== */

/* CVTPS2PD xmm (4xf32->2xf64, takes low 2 floats) */
LAT_XMM_UNARY(cvtps2pd_xmm_lat, "vcvtps2pd %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(cvtps2pd_xmm_tp, "vcvtps2pd %0, %0", __m128, XMM_F32_INIT)

/* CVTPS2PD ymm (xmm->ymm, 4xf32->4xf64) -- output is wider */
static double cvtps2pd_ymm_tp(uint64_t iteration_count)
{
    __m128 src_0 = XMM_F32_INIT, src_1 = XMM_F32_INIT;
    __m128 src_2 = XMM_F32_INIT, src_3 = XMM_F32_INIT;
    __m128 src_4 = XMM_F32_INIT, src_5 = XMM_F32_INIT;
    __m128 src_6 = XMM_F32_INIT, src_7 = XMM_F32_INIT;
    __m256d dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_0) : "x"(src_0));
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_1) : "x"(src_1));
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_2) : "x"(src_2));
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_3) : "x"(src_3));
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_4) : "x"(src_4));
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_5) : "x"(src_5));
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_6) : "x"(src_6));
        __asm__ volatile("vcvtps2pd %1, %0" : "=x"(dst_7) : "x"(src_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CVTPD2PS xmm (2xf64->2xf32, output in low 64 bits) */
LAT_XMM_UNARY(cvtpd2ps_xmm_lat, "vcvtpd2ps %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_UNARY(cvtpd2ps_xmm_tp, "vcvtpd2ps %0, %0", __m128d, XMM_F64_INIT)

/* CVTPD2PS ymm->xmm (4xf64->4xf32) */
static double cvtpd2ps_ymm_tp(uint64_t iteration_count)
{
    __m256d src_0 = YMM_F64_INIT, src_1 = YMM_F64_INIT;
    __m256d src_2 = YMM_F64_INIT, src_3 = YMM_F64_INIT;
    __m256d src_4 = YMM_F64_INIT, src_5 = YMM_F64_INIT;
    __m256d src_6 = YMM_F64_INIT, src_7 = YMM_F64_INIT;
    __m128 dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_0) : "x"(src_0));
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_1) : "x"(src_1));
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_2) : "x"(src_2));
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_3) : "x"(src_3));
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_4) : "x"(src_4));
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_5) : "x"(src_5));
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_6) : "x"(src_6));
        __asm__ volatile("vcvtpd2ps %1, %0" : "=x"(dst_7) : "x"(src_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CVTDQ2PS xmm (4xi32->4xf32) */
LAT_XMM_UNARY(cvtdq2ps_xmm_lat, "vcvtdq2ps %0, %0", __m128i, XMM_I32_INIT)
TP_XMM_UNARY(cvtdq2ps_xmm_tp, "vcvtdq2ps %0, %0", __m128i, XMM_I32_INIT)

/* CVTDQ2PS ymm */
LAT_XMM_UNARY(cvtdq2ps_ymm_lat, "vcvtdq2ps %0, %0", __m256i, YMM_I32_INIT)
TP_XMM_UNARY(cvtdq2ps_ymm_tp, "vcvtdq2ps %0, %0", __m256i, YMM_I32_INIT)

/* CVTPS2DQ xmm (4xf32->4xi32) */
LAT_XMM_UNARY(cvtps2dq_xmm_lat, "vcvtps2dq %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(cvtps2dq_xmm_tp, "vcvtps2dq %0, %0", __m128, XMM_F32_INIT)

/* CVTPS2DQ ymm */
LAT_XMM_UNARY(cvtps2dq_ymm_lat, "vcvtps2dq %0, %0", __m256, YMM_F32_INIT)
TP_XMM_UNARY(cvtps2dq_ymm_tp, "vcvtps2dq %0, %0", __m256, YMM_F32_INIT)

/* CVTTPS2DQ xmm (truncating) */
LAT_XMM_UNARY(cvttps2dq_xmm_lat, "vcvttps2dq %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(cvttps2dq_xmm_tp, "vcvttps2dq %0, %0", __m128, XMM_F32_INIT)

/* CVTTPS2DQ ymm */
LAT_XMM_UNARY(cvttps2dq_ymm_lat, "vcvttps2dq %0, %0", __m256, YMM_F32_INIT)
TP_XMM_UNARY(cvttps2dq_ymm_tp, "vcvttps2dq %0, %0", __m256, YMM_F32_INIT)

/* CVTPD2DQ xmm (2xf64->2xi32 in low 64 bits) */
LAT_XMM_UNARY(cvtpd2dq_xmm_lat, "vcvtpd2dq %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_UNARY(cvtpd2dq_xmm_tp, "vcvtpd2dq %0, %0", __m128d, XMM_F64_INIT)

/* CVTTPD2DQ xmm (truncating) */
LAT_XMM_UNARY(cvttpd2dq_xmm_lat, "vcvttpd2dq %0, %0", __m128d, XMM_F64_INIT)
TP_XMM_UNARY(cvttpd2dq_xmm_tp, "vcvttpd2dq %0, %0", __m128d, XMM_F64_INIT)

/* CVTDQ2PD xmm (2xi32->2xf64) */
LAT_XMM_UNARY(cvtdq2pd_xmm_lat, "vcvtdq2pd %0, %0", __m128i, XMM_I32_INIT)
TP_XMM_UNARY(cvtdq2pd_xmm_tp, "vcvtdq2pd %0, %0", __m128i, XMM_I32_INIT)

/* CVTSI2SS (GPR->scalar float in xmm) -- throughput only, no natural chain */
static double cvtsi2ss_tp(uint64_t iteration_count)
{
    uint64_t gpr_0 = 42, gpr_1 = 43, gpr_2 = 44, gpr_3 = 45;
    uint64_t gpr_4 = 46, gpr_5 = 47, gpr_6 = 48, gpr_7 = 49;
    __m128 dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+r"(gpr_0), "+r"(gpr_1), "+r"(gpr_2), "+r"(gpr_3));
    __asm__ volatile("" : "+r"(gpr_4), "+r"(gpr_5), "+r"(gpr_6), "+r"(gpr_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_0) : "r"(gpr_0));
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_1) : "r"(gpr_1));
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_2) : "r"(gpr_2));
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_3) : "r"(gpr_3));
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_4) : "r"(gpr_4));
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_5) : "r"(gpr_5));
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_6) : "r"(gpr_6));
        __asm__ volatile("vcvtsi2ss %1, %0, %0" : "=x"(dst_7) : "r"(gpr_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CVTSI2SD */
static double cvtsi2sd_tp(uint64_t iteration_count)
{
    uint64_t gpr_0 = 42, gpr_1 = 43, gpr_2 = 44, gpr_3 = 45;
    uint64_t gpr_4 = 46, gpr_5 = 47, gpr_6 = 48, gpr_7 = 49;
    __m128d dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+r"(gpr_0), "+r"(gpr_1), "+r"(gpr_2), "+r"(gpr_3));
    __asm__ volatile("" : "+r"(gpr_4), "+r"(gpr_5), "+r"(gpr_6), "+r"(gpr_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_0) : "r"(gpr_0));
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_1) : "r"(gpr_1));
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_2) : "r"(gpr_2));
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_3) : "r"(gpr_3));
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_4) : "r"(gpr_4));
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_5) : "r"(gpr_5));
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_6) : "r"(gpr_6));
        __asm__ volatile("vcvtsi2sd %1, %0, %0" : "=x"(dst_7) : "r"(gpr_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CVTSS2SI (scalar float -> GPR) */
static double cvtss2si_tp(uint64_t iteration_count)
{
    __m128 src_0 = XMM_F32_INIT, src_1 = XMM_F32_INIT;
    __m128 src_2 = XMM_F32_INIT, src_3 = XMM_F32_INIT;
    __m128 src_4 = XMM_F32_INIT, src_5 = XMM_F32_INIT;
    __m128 src_6 = XMM_F32_INIT, src_7 = XMM_F32_INIT;
    uint64_t gpr_0, gpr_1, gpr_2, gpr_3, gpr_4, gpr_5, gpr_6, gpr_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_0) : "x"(src_0));
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_1) : "x"(src_1));
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_2) : "x"(src_2));
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_3) : "x"(src_3));
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_4) : "x"(src_4));
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_5) : "x"(src_5));
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_6) : "x"(src_6));
        __asm__ volatile("vcvtss2si %1, %0" : "=r"(gpr_7) : "x"(src_7));
        __asm__ volatile("" : "+r"(gpr_0), "+r"(gpr_1), "+r"(gpr_2), "+r"(gpr_3));
        __asm__ volatile("" : "+r"(gpr_4), "+r"(gpr_5), "+r"(gpr_6), "+r"(gpr_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = gpr_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CVTSD2SI */
static double cvtsd2si_tp(uint64_t iteration_count)
{
    __m128d src_0 = XMM_F64_INIT, src_1 = XMM_F64_INIT;
    __m128d src_2 = XMM_F64_INIT, src_3 = XMM_F64_INIT;
    __m128d src_4 = XMM_F64_INIT, src_5 = XMM_F64_INIT;
    __m128d src_6 = XMM_F64_INIT, src_7 = XMM_F64_INIT;
    uint64_t gpr_0, gpr_1, gpr_2, gpr_3, gpr_4, gpr_5, gpr_6, gpr_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_0) : "x"(src_0));
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_1) : "x"(src_1));
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_2) : "x"(src_2));
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_3) : "x"(src_3));
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_4) : "x"(src_4));
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_5) : "x"(src_5));
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_6) : "x"(src_6));
        __asm__ volatile("vcvtsd2si %1, %0" : "=r"(gpr_7) : "x"(src_7));
        __asm__ volatile("" : "+r"(gpr_0), "+r"(gpr_1), "+r"(gpr_2), "+r"(gpr_3));
        __asm__ volatile("" : "+r"(gpr_4), "+r"(gpr_5), "+r"(gpr_6), "+r"(gpr_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = gpr_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* F16C: VCVTPH2PS xmm->xmm (4xf16->4xf32) */
LAT_XMM_UNARY(cvtph2ps_xmm_lat, "vcvtph2ps %0, %0", __m128i, XMM_I32_INIT)
TP_XMM_UNARY(cvtph2ps_xmm_tp, "vcvtph2ps %0, %0", __m128i, XMM_I32_INIT)

/* F16C: VCVTPH2PS xmm->ymm (8xf16->8xf32) */
static double cvtph2ps_ymm_tp(uint64_t iteration_count)
{
    __m128i src_0 = XMM_I32_INIT, src_1 = XMM_I32_INIT;
    __m128i src_2 = XMM_I32_INIT, src_3 = XMM_I32_INIT;
    __m128i src_4 = XMM_I32_INIT, src_5 = XMM_I32_INIT;
    __m128i src_6 = XMM_I32_INIT, src_7 = XMM_I32_INIT;
    __m256 dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_0) : "x"(src_0));
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_1) : "x"(src_1));
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_2) : "x"(src_2));
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_3) : "x"(src_3));
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_4) : "x"(src_4));
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_5) : "x"(src_5));
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_6) : "x"(src_6));
        __asm__ volatile("vcvtph2ps %1, %0" : "=x"(dst_7) : "x"(src_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* F16C: VCVTPS2PH xmm (4xf32->4xf16, result in low 64 bits) */
LAT_XMM_UNARY(cvtps2ph_xmm_lat, "vcvtps2ph $0, %0, %0", __m128, XMM_F32_INIT)
TP_XMM_UNARY(cvtps2ph_xmm_tp, "vcvtps2ph $0, %0, %0", __m128, XMM_F32_INIT)

/* F16C: VCVTPS2PH ymm->xmm (8xf32->8xf16) */
static double cvtps2ph_ymm_tp(uint64_t iteration_count)
{
    __m256 src_0 = YMM_F32_INIT, src_1 = YMM_F32_INIT;
    __m256 src_2 = YMM_F32_INIT, src_3 = YMM_F32_INIT;
    __m256 src_4 = YMM_F32_INIT, src_5 = YMM_F32_INIT;
    __m256 src_6 = YMM_F32_INIT, src_7 = YMM_F32_INIT;
    __m128i dst_0, dst_1, dst_2, dst_3, dst_4, dst_5, dst_6, dst_7;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_0) : "x"(src_0));
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_1) : "x"(src_1));
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_2) : "x"(src_2));
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_3) : "x"(src_3));
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_4) : "x"(src_4));
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_5) : "x"(src_5));
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_6) : "x"(src_6));
        __asm__ volatile("vcvtps2ph $0, %1, %0" : "=x"(dst_7) : "x"(src_7));
        __asm__ volatile("" : "+x"(dst_0), "+x"(dst_1), "+x"(dst_2), "+x"(dst_3));
        __asm__ volatile("" : "+x"(dst_4), "+x"(dst_5), "+x"(dst_6), "+x"(dst_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * MISC: PTEST, VZEROUPPER
 * ======================================================================== */

/* VPTEST xmm (integer-domain test on FP data, sets EFLAGS) */
static double vptest_xmm_tp(uint64_t iteration_count)
{
    __m128i src_0 = XMM_I32_INIT, src_1 = XMM_I32_INIT;
    __m128i src_2 = XMM_I32_INIT, src_3 = XMM_I32_INIT;
    __m128i src_4 = XMM_I32_INIT, src_5 = XMM_I32_INIT;
    __m128i src_6 = XMM_I32_INIT, src_7 = XMM_I32_INIT;
    __m128i mask_operand = XMM_I32_INIT;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    __asm__ volatile("" : "+x"(mask_operand));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vptest %0, %1" : : "x"(src_0), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_1), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_2), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_3), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_4), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_5), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_6), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_7), "x"(mask_operand) : "cc");
        __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
        __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(src_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VPTEST ymm */
static double vptest_ymm_tp(uint64_t iteration_count)
{
    __m256i src_0 = YMM_I32_INIT, src_1 = YMM_I32_INIT;
    __m256i src_2 = YMM_I32_INIT, src_3 = YMM_I32_INIT;
    __m256i src_4 = YMM_I32_INIT, src_5 = YMM_I32_INIT;
    __m256i src_6 = YMM_I32_INIT, src_7 = YMM_I32_INIT;
    __m256i mask_operand = YMM_I32_INIT;
    __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
    __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    __asm__ volatile("" : "+x"(mask_operand));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vptest %0, %1" : : "x"(src_0), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_1), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_2), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_3), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_4), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_5), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_6), "x"(mask_operand) : "cc");
        __asm__ volatile("vptest %0, %1" : : "x"(src_7), "x"(mask_operand) : "cc");
        __asm__ volatile("" : "+x"(src_0), "+x"(src_1), "+x"(src_2), "+x"(src_3));
        __asm__ volatile("" : "+x"(src_4), "+x"(src_5), "+x"(src_6), "+x"(src_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+x"(src_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VZEROUPPER throughput */
static double vzeroupper_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
        __asm__ volatile("vzeroupper" ::: "ymm0","ymm1","ymm2","ymm3",
            "ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11",
            "ymm12","ymm13","ymm14","ymm15");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * MEASUREMENT ENTRY TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    const char *precision_label;
    int width_bits;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} SimdFpProbeEntry;

static const SimdFpProbeEntry probe_table[] = {
    /* ---- FP32 Scalar (SS) ---- */
    {"VADDSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  addss_lat,         addss_tp},
    {"VSUBSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  subss_lat,         subss_tp},
    {"VMULSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  mulss_lat,         mulss_tp},
    {"VDIVSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  divss_lat,         divss_tp},
    {"VMAXSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  maxss_lat,         maxss_tp},
    {"VMINSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  minss_lat,         minss_tp},
    {"VSQRTSS",      "xmm,xmm",      "fp32", 128, "fp_arith",  sqrtss_lat,        sqrtss_tp},
    {"VRSQRTSS",     "xmm,xmm",      "fp32", 128, "fp_arith",  rsqrtss_lat,       rsqrtss_tp},
    {"VRCPSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  rcpss_lat,         rcpss_tp},
    {"VROUNDSS",     "xmm,xmm",      "fp32", 128, "fp_arith",  roundss_lat,       roundss_tp},
    {"VCMPSS",       "xmm,xmm",      "fp32", 128, "fp_arith",  cmpss_lat,         cmpss_tp},
    {"VFMADD231SS",  "xmm,xmm,xmm",  "fp32", 128, "fma",       vfmadd231ss_lat,   vfmadd231ss_tp},
    {"VFMSUB231SS",  "xmm,xmm,xmm",  "fp32", 128, "fma",       vfmsub231ss_lat,   vfmsub231ss_tp},
    {"VFNMADD231SS", "xmm,xmm,xmm",  "fp32", 128, "fma",       vfnmadd231ss_lat,  vfnmadd231ss_tp},
    {"VFNMSUB231SS", "xmm,xmm,xmm",  "fp32", 128, "fma",       vfnmsub231ss_lat,  vfnmsub231ss_tp},

    /* ---- FP32 Packed XMM (128-bit) ---- */
    {"VADDPS",       "xmm,xmm",      "fp32", 128, "fp_arith",  addps_xmm_lat,     addps_xmm_tp},
    {"VSUBPS",       "xmm,xmm",      "fp32", 128, "fp_arith",  subps_xmm_lat,     subps_xmm_tp},
    {"VMULPS",       "xmm,xmm",      "fp32", 128, "fp_arith",  mulps_xmm_lat,     mulps_xmm_tp},
    {"VDIVPS",       "xmm,xmm",      "fp32", 128, "fp_arith",  divps_xmm_lat,     divps_xmm_tp},
    {"VMAXPS",       "xmm,xmm",      "fp32", 128, "fp_arith",  maxps_xmm_lat,     maxps_xmm_tp},
    {"VMINPS",       "xmm,xmm",      "fp32", 128, "fp_arith",  minps_xmm_lat,     minps_xmm_tp},
    {"VSQRTPS",      "xmm",          "fp32", 128, "fp_arith",  sqrtps_xmm_lat,    sqrtps_xmm_tp},
    {"VRSQRTPS",     "xmm",          "fp32", 128, "fp_arith",  rsqrtps_xmm_lat,   rsqrtps_xmm_tp},
    {"VRCPPS",       "xmm",          "fp32", 128, "fp_arith",  rcpps_xmm_lat,     rcpps_xmm_tp},
    {"VROUNDPS",     "xmm",          "fp32", 128, "fp_arith",  roundps_xmm_lat,   roundps_xmm_tp},
    {"VHADDPS",      "xmm,xmm",      "fp32", 128, "fp_arith",  haddps_xmm_lat,    haddps_xmm_tp},
    {"VHSUBPS",      "xmm,xmm",      "fp32", 128, "fp_arith",  hsubps_xmm_lat,    hsubps_xmm_tp},
    {"VDPPS",        "xmm,xmm",      "fp32", 128, "fp_arith",  dpps_xmm_lat,      dpps_xmm_tp},
    {"VCMPPS",       "xmm,xmm",      "fp32", 128, "fp_arith",  cmpps_xmm_lat,     cmpps_xmm_tp},
    {"VFMADD231PS",  "xmm,xmm,xmm",  "fp32", 128, "fma",       vfmadd231ps_xmm_lat, vfmadd231ps_xmm_tp},
    {"VFMSUB231PS",  "xmm,xmm,xmm",  "fp32", 128, "fma",       vfmsub231ps_xmm_lat, vfmsub231ps_xmm_tp},
    {"VFNMADD231PS", "xmm,xmm,xmm",  "fp32", 128, "fma",       vfnmadd231ps_xmm_lat, vfnmadd231ps_xmm_tp},
    {"VFNMSUB231PS", "xmm,xmm,xmm",  "fp32", 128, "fma",       vfnmsub231ps_xmm_lat, vfnmsub231ps_xmm_tp},

    /* ---- FP32 Packed YMM (256-bit) ---- */
    {"VADDPS",       "ymm,ymm",      "fp32", 256, "fp_arith",  addps_ymm_lat,     addps_ymm_tp},
    {"VSUBPS",       "ymm,ymm",      "fp32", 256, "fp_arith",  subps_ymm_lat,     subps_ymm_tp},
    {"VMULPS",       "ymm,ymm",      "fp32", 256, "fp_arith",  mulps_ymm_lat,     mulps_ymm_tp},
    {"VDIVPS",       "ymm,ymm",      "fp32", 256, "fp_arith",  divps_ymm_lat,     divps_ymm_tp},
    {"VMAXPS",       "ymm,ymm",      "fp32", 256, "fp_arith",  maxps_ymm_lat,     maxps_ymm_tp},
    {"VMINPS",       "ymm,ymm",      "fp32", 256, "fp_arith",  minps_ymm_lat,     minps_ymm_tp},
    {"VSQRTPS",      "ymm",          "fp32", 256, "fp_arith",  sqrtps_ymm_lat,    sqrtps_ymm_tp},
    {"VRSQRTPS",     "ymm",          "fp32", 256, "fp_arith",  rsqrtps_ymm_lat,   rsqrtps_ymm_tp},
    {"VRCPPS",       "ymm",          "fp32", 256, "fp_arith",  rcpps_ymm_lat,     rcpps_ymm_tp},
    {"VROUNDPS",     "ymm",          "fp32", 256, "fp_arith",  roundps_ymm_lat,   roundps_ymm_tp},
    {"VHADDPS",      "ymm,ymm",      "fp32", 256, "fp_arith",  haddps_ymm_lat,    haddps_ymm_tp},
    {"VHSUBPS",      "ymm,ymm",      "fp32", 256, "fp_arith",  hsubps_ymm_lat,    hsubps_ymm_tp},
    {"VDPPS",        "ymm,ymm",      "fp32", 256, "fp_arith",  dpps_ymm_lat,      dpps_ymm_tp},
    {"VCMPPS",       "ymm,ymm",      "fp32", 256, "fp_arith",  cmpps_ymm_lat,     cmpps_ymm_tp},
    {"VFMADD231PS",  "ymm,ymm,ymm",  "fp32", 256, "fma",       vfmadd231ps_ymm_lat, vfmadd231ps_ymm_tp},
    {"VFMSUB231PS",  "ymm,ymm,ymm",  "fp32", 256, "fma",       vfmsub231ps_ymm_lat, vfmsub231ps_ymm_tp},
    {"VFNMADD231PS", "ymm,ymm,ymm",  "fp32", 256, "fma",       vfnmadd231ps_ymm_lat, vfnmadd231ps_ymm_tp},
    {"VFNMSUB231PS", "ymm,ymm,ymm",  "fp32", 256, "fma",       vfnmsub231ps_ymm_lat, vfnmsub231ps_ymm_tp},

    /* ---- FP64 Scalar (SD) ---- */
    {"VADDSD",       "xmm,xmm",      "fp64", 128, "fp_arith",  addsd_lat,         addsd_tp},
    {"VSUBSD",       "xmm,xmm",      "fp64", 128, "fp_arith",  subsd_lat,         subsd_tp},
    {"VMULSD",       "xmm,xmm",      "fp64", 128, "fp_arith",  mulsd_lat,         mulsd_tp},
    {"VDIVSD",       "xmm,xmm",      "fp64", 128, "fp_arith",  divsd_lat,         divsd_tp},
    {"VMAXSD",       "xmm,xmm",      "fp64", 128, "fp_arith",  maxsd_lat,         maxsd_tp},
    {"VMINSD",       "xmm,xmm",      "fp64", 128, "fp_arith",  minsd_lat,         minsd_tp},
    {"VSQRTSD",      "xmm,xmm",      "fp64", 128, "fp_arith",  sqrtsd_lat,        sqrtsd_tp},
    {"VROUNDSD",     "xmm,xmm",      "fp64", 128, "fp_arith",  roundsd_lat,       roundsd_tp},
    {"VCMPSD",       "xmm,xmm",      "fp64", 128, "fp_arith",  cmpsd_lat,         cmpsd_tp},
    {"VFMADD231SD",  "xmm,xmm,xmm",  "fp64", 128, "fma",       vfmadd231sd_lat,   vfmadd231sd_tp},

    /* ---- FP64 Packed XMM (128-bit) ---- */
    {"VADDPD",       "xmm,xmm",      "fp64", 128, "fp_arith",  addpd_xmm_lat,     addpd_xmm_tp},
    {"VSUBPD",       "xmm,xmm",      "fp64", 128, "fp_arith",  subpd_xmm_lat,     subpd_xmm_tp},
    {"VMULPD",       "xmm,xmm",      "fp64", 128, "fp_arith",  mulpd_xmm_lat,     mulpd_xmm_tp},
    {"VDIVPD",       "xmm,xmm",      "fp64", 128, "fp_arith",  divpd_xmm_lat,     divpd_xmm_tp},
    {"VMAXPD",       "xmm,xmm",      "fp64", 128, "fp_arith",  maxpd_xmm_lat,     maxpd_xmm_tp},
    {"VMINPD",       "xmm,xmm",      "fp64", 128, "fp_arith",  minpd_xmm_lat,     minpd_xmm_tp},
    {"VSQRTPD",      "xmm",          "fp64", 128, "fp_arith",  sqrtpd_xmm_lat,    sqrtpd_xmm_tp},
    {"VROUNDPD",     "xmm",          "fp64", 128, "fp_arith",  roundpd_xmm_lat,   roundpd_xmm_tp},
    {"VHADDPD",      "xmm,xmm",      "fp64", 128, "fp_arith",  haddpd_xmm_lat,    haddpd_xmm_tp},
    {"VHSUBPD",      "xmm,xmm",      "fp64", 128, "fp_arith",  hsubpd_xmm_lat,    hsubpd_xmm_tp},
    {"VCMPPD",       "xmm,xmm",      "fp64", 128, "fp_arith",  cmppd_xmm_lat,     cmppd_xmm_tp},
    {"VFMADD231PD",  "xmm,xmm,xmm",  "fp64", 128, "fma",       vfmadd231pd_xmm_lat, vfmadd231pd_xmm_tp},

    /* ---- FP64 Packed YMM (256-bit) ---- */
    {"VADDPD",       "ymm,ymm",      "fp64", 256, "fp_arith",  addpd_ymm_lat,     addpd_ymm_tp},
    {"VSUBPD",       "ymm,ymm",      "fp64", 256, "fp_arith",  subpd_ymm_lat,     subpd_ymm_tp},
    {"VMULPD",       "ymm,ymm",      "fp64", 256, "fp_arith",  mulpd_ymm_lat,     mulpd_ymm_tp},
    {"VDIVPD",       "ymm,ymm",      "fp64", 256, "fp_arith",  divpd_ymm_lat,     divpd_ymm_tp},
    {"VMAXPD",       "ymm,ymm",      "fp64", 256, "fp_arith",  maxpd_ymm_lat,     maxpd_ymm_tp},
    {"VMINPD",       "ymm,ymm",      "fp64", 256, "fp_arith",  minpd_ymm_lat,     minpd_ymm_tp},
    {"VSQRTPD",      "ymm",          "fp64", 256, "fp_arith",  sqrtpd_ymm_lat,    sqrtpd_ymm_tp},
    {"VROUNDPD",     "ymm",          "fp64", 256, "fp_arith",  roundpd_ymm_lat,   roundpd_ymm_tp},
    {"VHADDPD",      "ymm,ymm",      "fp64", 256, "fp_arith",  haddpd_ymm_lat,    haddpd_ymm_tp},
    {"VHSUBPD",      "ymm,ymm",      "fp64", 256, "fp_arith",  hsubpd_ymm_lat,    hsubpd_ymm_tp},
    {"VCMPPD",       "ymm,ymm",      "fp64", 256, "fp_arith",  cmppd_ymm_lat,     cmppd_ymm_tp},
    {"VFMADD231PD",  "ymm,ymm,ymm",  "fp64", 256, "fma",       vfmadd231pd_ymm_lat, vfmadd231pd_ymm_tp},

    /* ---- Move / Shuffle ---- */
    {"VMOVAPS",      "xmm,xmm",      "fp32", 128, "move",      movaps_xmm_lat,    movaps_xmm_tp},
    {"VMOVAPD",      "xmm,xmm",      "fp64", 128, "move",      movapd_xmm_lat,    movapd_xmm_tp},
    {"VMOVAPS",      "ymm,ymm",      "fp32", 256, "move",      movaps_ymm_lat,    movaps_ymm_tp},
    {"VMOVAPD",      "ymm,ymm",      "fp64", 256, "move",      movapd_ymm_lat,    movapd_ymm_tp},
    {"VSHUFPS",      "xmm,xmm",      "fp32", 128, "shuffle",   shufps_xmm_lat,    shufps_xmm_tp},
    {"VSHUFPS",      "ymm,ymm",      "fp32", 256, "shuffle",   shufps_ymm_lat,    shufps_ymm_tp},
    {"VSHUFPD",      "xmm,xmm",      "fp64", 128, "shuffle",   shufpd_xmm_lat,    shufpd_xmm_tp},
    {"VSHUFPD",      "ymm,ymm",      "fp64", 256, "shuffle",   shufpd_ymm_lat,    shufpd_ymm_tp},
    {"VUNPCKLPS",    "xmm,xmm",      "fp32", 128, "shuffle",   unpcklps_xmm_lat,  unpcklps_xmm_tp},
    {"VUNPCKHPS",    "xmm,xmm",      "fp32", 128, "shuffle",   unpckhps_xmm_lat,  unpckhps_xmm_tp},
    {"VUNPCKLPD",    "xmm,xmm",      "fp64", 128, "shuffle",   unpcklpd_xmm_lat,  unpcklpd_xmm_tp},
    {"VUNPCKHPD",    "xmm,xmm",      "fp64", 128, "shuffle",   unpckhpd_xmm_lat,  unpckhpd_xmm_tp},
    {"VUNPCKLPS",    "ymm,ymm",      "fp32", 256, "shuffle",   unpcklps_ymm_lat,  unpcklps_ymm_tp},
    {"VUNPCKHPS",    "ymm,ymm",      "fp32", 256, "shuffle",   unpckhps_ymm_lat,  unpckhps_ymm_tp},
    {"VUNPCKLPD",    "ymm,ymm",      "fp64", 256, "shuffle",   unpcklpd_ymm_lat,  unpcklpd_ymm_tp},
    {"VUNPCKHPD",    "ymm,ymm",      "fp64", 256, "shuffle",   unpckhpd_ymm_lat,  unpckhpd_ymm_tp},
    {"VPERMILPS",    "xmm,imm",      "fp32", 128, "shuffle",   vpermilps_xmm_lat, vpermilps_xmm_tp},
    {"VPERMILPS",    "ymm,imm",      "fp32", 256, "shuffle",   vpermilps_ymm_lat, vpermilps_ymm_tp},
    {"VPERMILPD",    "xmm,imm",      "fp64", 128, "shuffle",   vpermilpd_xmm_lat, vpermilpd_xmm_tp},
    {"VPERMILPD",    "ymm,imm",      "fp64", 256, "shuffle",   vpermilpd_ymm_lat, vpermilpd_ymm_tp},
    {"VPERM2F128",   "ymm,ymm,imm",  "fp32", 256, "shuffle",   vperm2f128_lat,    vperm2f128_tp},
    {"VINSERTF128",  "ymm,xmm,imm",  "fp32", 256, "shuffle",   vinsertf128_lat,   vinsertf128_tp},
    {"VEXTRACTF128", "ymm,xmm,imm",  "fp32", 256, "shuffle",   vextractf128_lat,  vextractf128_tp},
    {"VBROADCASTSS", "xmm",          "fp32", 128, "shuffle",   vbroadcastss_xmm_lat, vbroadcastss_xmm_tp},
    {"VBROADCASTSS", "ymm",          "fp32", 256, "shuffle",   vbroadcastss_ymm_lat, vbroadcastss_ymm_tp},
    {"VBROADCASTSD", "ymm",          "fp64", 256, "shuffle",   vbroadcastsd_ymm_lat, vbroadcastsd_ymm_tp},
    {"VBLENDPS",     "xmm,xmm,imm",  "fp32", 128, "shuffle",   blendps_xmm_lat,   blendps_xmm_tp},
    {"VBLENDPS",     "ymm,ymm,imm",  "fp32", 256, "shuffle",   blendps_ymm_lat,   blendps_ymm_tp},
    {"VBLENDPD",     "xmm,xmm,imm",  "fp64", 128, "shuffle",   blendpd_xmm_lat,   blendpd_xmm_tp},
    {"VBLENDPD",     "ymm,ymm,imm",  "fp64", 256, "shuffle",   blendpd_ymm_lat,   blendpd_ymm_tp},
    {"VBLENDVPS",    "xmm,xmm,xmm",  "fp32", 128, "shuffle",   blendvps_xmm_lat,  blendvps_xmm_tp},
    {"VBLENDVPS",    "ymm,ymm,ymm",  "fp32", 256, "shuffle",   blendvps_ymm_lat,  blendvps_ymm_tp},
    {"VBLENDVPD",    "xmm,xmm,xmm",  "fp64", 128, "shuffle",   blendvpd_xmm_lat,  blendvpd_xmm_tp},
    {"VBLENDVPD",    "ymm,ymm,ymm",  "fp64", 256, "shuffle",   blendvpd_ymm_lat,  blendvpd_ymm_tp},
    {"VMOVMSKPS",    "r32,xmm",      "fp32", 128, "move",      NULL,              movmskps_xmm_tp},
    {"VMOVMSKPD",    "r32,xmm",      "fp64", 128, "move",      NULL,              movmskpd_xmm_tp},

    /* ---- Bitwise FP ---- */
    {"VANDPS",       "xmm,xmm",      "fp32", 128, "bitwise",   andps_xmm_lat,     andps_xmm_tp},
    {"VANDPS",       "ymm,ymm",      "fp32", 256, "bitwise",   andps_ymm_lat,     andps_ymm_tp},
    {"VANDPD",       "xmm,xmm",      "fp64", 128, "bitwise",   andpd_xmm_lat,     andpd_xmm_tp},
    {"VANDPD",       "ymm,ymm",      "fp64", 256, "bitwise",   andpd_ymm_lat,     andpd_ymm_tp},
    {"VANDNPS",      "xmm,xmm",      "fp32", 128, "bitwise",   andnps_xmm_lat,    andnps_xmm_tp},
    {"VANDNPS",      "ymm,ymm",      "fp32", 256, "bitwise",   andnps_ymm_lat,    andnps_ymm_tp},
    {"VANDNPD",      "xmm,xmm",      "fp64", 128, "bitwise",   andnpd_xmm_lat,    andnpd_xmm_tp},
    {"VANDNPD",      "ymm,ymm",      "fp64", 256, "bitwise",   andnpd_ymm_lat,    andnpd_ymm_tp},
    {"VORPS",        "xmm,xmm",      "fp32", 128, "bitwise",   orps_xmm_lat,      orps_xmm_tp},
    {"VORPS",        "ymm,ymm",      "fp32", 256, "bitwise",   orps_ymm_lat,      orps_ymm_tp},
    {"VORPD",        "xmm,xmm",      "fp64", 128, "bitwise",   orpd_xmm_lat,      orpd_xmm_tp},
    {"VORPD",        "ymm,ymm",      "fp64", 256, "bitwise",   orpd_ymm_lat,      orpd_ymm_tp},
    {"VXORPS",       "xmm,xmm",      "fp32", 128, "bitwise",   xorps_xmm_lat,     xorps_xmm_tp},
    {"VXORPS",       "ymm,ymm",      "fp32", 256, "bitwise",   xorps_ymm_lat,     xorps_ymm_tp},
    {"VXORPD",       "xmm,xmm",      "fp64", 128, "bitwise",   xorpd_xmm_lat,     xorpd_xmm_tp},
    {"VXORPD",       "ymm,ymm",      "fp64", 256, "bitwise",   xorpd_ymm_lat,     xorpd_ymm_tp},

    /* ---- Conversion ---- */
    {"VCVTPS2PD",    "xmm",          "fp32", 128, "convert",   cvtps2pd_xmm_lat,  cvtps2pd_xmm_tp},
    {"VCVTPS2PD",    "xmm->ymm",     "fp32", 256, "convert",   NULL,              cvtps2pd_ymm_tp},
    {"VCVTPD2PS",    "xmm",          "fp64", 128, "convert",   cvtpd2ps_xmm_lat,  cvtpd2ps_xmm_tp},
    {"VCVTPD2PS",    "ymm->xmm",     "fp64", 256, "convert",   NULL,              cvtpd2ps_ymm_tp},
    {"VCVTDQ2PS",    "xmm",          "i32",  128, "convert",   cvtdq2ps_xmm_lat,  cvtdq2ps_xmm_tp},
    {"VCVTDQ2PS",    "ymm",          "i32",  256, "convert",   cvtdq2ps_ymm_lat,  cvtdq2ps_ymm_tp},
    {"VCVTPS2DQ",    "xmm",          "fp32", 128, "convert",   cvtps2dq_xmm_lat,  cvtps2dq_xmm_tp},
    {"VCVTPS2DQ",    "ymm",          "fp32", 256, "convert",   cvtps2dq_ymm_lat,  cvtps2dq_ymm_tp},
    {"VCVTTPS2DQ",   "xmm",          "fp32", 128, "convert",   cvttps2dq_xmm_lat, cvttps2dq_xmm_tp},
    {"VCVTTPS2DQ",   "ymm",          "fp32", 256, "convert",   cvttps2dq_ymm_lat, cvttps2dq_ymm_tp},
    {"VCVTPD2DQ",    "xmm",          "fp64", 128, "convert",   cvtpd2dq_xmm_lat,  cvtpd2dq_xmm_tp},
    {"VCVTTPD2DQ",   "xmm",          "fp64", 128, "convert",   cvttpd2dq_xmm_lat, cvttpd2dq_xmm_tp},
    {"VCVTDQ2PD",    "xmm",          "i32",  128, "convert",   cvtdq2pd_xmm_lat,  cvtdq2pd_xmm_tp},
    {"VCVTSI2SS",    "r64->xmm",     "fp32", 128, "convert",   NULL,              cvtsi2ss_tp},
    {"VCVTSI2SD",    "r64->xmm",     "fp64", 128, "convert",   NULL,              cvtsi2sd_tp},
    {"VCVTSS2SI",    "xmm->r64",     "fp32", 128, "convert",   NULL,              cvtss2si_tp},
    {"VCVTSD2SI",    "xmm->r64",     "fp64", 128, "convert",   NULL,              cvtsd2si_tp},
    {"VCVTPH2PS",    "xmm",          "fp16", 128, "convert",   cvtph2ps_xmm_lat,  cvtph2ps_xmm_tp},
    {"VCVTPH2PS",    "xmm->ymm",     "fp16", 256, "convert",   NULL,              cvtph2ps_ymm_tp},
    {"VCVTPS2PH",    "xmm",          "fp32", 128, "convert",   cvtps2ph_xmm_lat,  cvtps2ph_xmm_tp},
    {"VCVTPS2PH",    "ymm->xmm",     "fp32", 256, "convert",   NULL,              cvtps2ph_ymm_tp},

    /* ---- Misc ---- */
    {"VPTEST",       "xmm,xmm",      "i128", 128, "misc",      NULL,              vptest_xmm_tp},
    {"VPTEST",       "ymm,ymm",      "i256", 256, "misc",      NULL,              vptest_ymm_tp},
    {"VZEROUPPER",   "",              "n/a",  256, "misc",      NULL,              vzeroupper_tp},

    {NULL, NULL, NULL, 0, NULL, NULL, NULL}
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
        sm_zen_print_header("simd_fp_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring SIMD floating-point instruction forms...\n\n");
    }

    /* Count entries */
    int total_entries = 0;
    for (int count_idx = 0; probe_table[count_idx].mnemonic != NULL; count_idx++)
        total_entries++;

    if (!csv_mode)
        printf("Total instruction forms: %d\n\n", total_entries);

    /* CSV header */
    printf("mnemonic,operands,precision,width_bits,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const SimdFpProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const SimdFpProbeEntry *current_entry = &probe_table[entry_idx];

        double measured_latency = -1.0;
        double measured_throughput = -1.0;

        if (current_entry->measure_latency)
            measured_latency = current_entry->measure_latency(probe_config.iterations);
        if (current_entry->measure_throughput)
            measured_throughput = current_entry->measure_throughput(probe_config.iterations);

        printf("%s,%s,%s,%d,%.4f,%.4f,%s\n",
               current_entry->mnemonic,
               current_entry->operand_description,
               current_entry->precision_label,
               current_entry->width_bits,
               measured_latency,
               measured_throughput,
               current_entry->instruction_category);
    }

    return 0;
}
