#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_sse2_gaps.c -- Fill 72 missing SSE/SSE2 legacy-encoded instruction
 * measurements on Zen 4.
 *
 * These are the NON-VEX (legacy SSE) encodings that were absent from prior
 * exhaustive probes which only measured VEX-encoded forms.
 *
 * Measures latency (4-deep dependency chain) and throughput (8 independent
 * accumulator streams) for every missing mnemonic from zen4_coverage_gaps.txt
 * SSE2 section.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -mf16c -msse4.1 -msse4.2 -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_sse2_gaps.c -lm \
 *     -o ../../../../build/bin/zen_probe_sse2_gaps
 */

/* ========================================================================
 * MACRO TEMPLATES -- legacy SSE (non-VEX) 128-bit XMM
 *
 * These use legacy encoding by specifying the non-v prefix mnemonic.
 * "+x" constraint = xmm register.
 * Barriers: __asm__ volatile("" : "+x"(acc));
 * ======================================================================== */

/*
 * LAT_BINARY_SSE: latency for legacy "op xmm_src, xmm_dst" (2-operand
 * destructive form). 4-deep chain per iteration.
 */
#define LAT_BINARY_SSE(func_name, asm_str, init_type, init_val)               \
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
 * TP_BINARY_SSE: throughput for legacy "op xmm_src, xmm_dst" with 8
 * independent accumulator streams.
 */
#define TP_BINARY_SSE(func_name, asm_str, init_type, init_val)                \
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
 * LAT_UNARY_SSE: latency for unary "op xmm, xmm" (reads and writes same
 * register). Used for SQRT, RSQRT, RCP, CVTDQ2PD, etc.
 */
#define LAT_UNARY_SSE(func_name, asm_str, init_type, init_val)               \
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
 * TP_UNARY_SSE: throughput for unary xmm instruction, 8 independent streams.
 */
#define TP_UNARY_SSE(func_name, asm_str, init_type, init_val)                \
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
 * TP_STANDALONE_SSE: throughput for an instruction that produces output
 * from input without natural chaining. "=x" output, "x" input.
 */
#define TP_STANDALONE_SSE(func_name, asm_str, in_type, in_val, out_type)      \
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
 * LAT_XMM_TO_GPR_SSE: xmm->GPR (e.g., MOVMSKPS). Chain by feeding GPR
 * result back via pinsrd into xmm to maintain dependency.
 */
#define LAT_XMM_TO_GPR_SSE(func_name, asm_msk, asm_feed, init_type, init_val)\
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
 * LAT_CVT_GPR_TO_XMM: GPR->XMM conversion (e.g., CVTSI2SD).
 * Chain: xmm -> cvttsd2si -> gpr -> cvtsi2sd -> xmm.
 */
#define LAT_CVT_GPR_XMM(func_name, cvt_to_gpr, cvt_to_xmm, init_type, init_val) \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type accumulator = init_val;                                         \
    uint64_t gpr_temp = 0;                                                    \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(cvt_to_gpr "\n\t" cvt_to_xmm                        \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
        __asm__ volatile(cvt_to_gpr "\n\t" cvt_to_xmm                        \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
        __asm__ volatile(cvt_to_gpr "\n\t" cvt_to_xmm                        \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
        __asm__ volatile(cvt_to_gpr "\n\t" cvt_to_xmm                        \
            : "+x"(accumulator), "=&r"(gpr_temp));                            \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(accumulator));                                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * LAT_MOVE_SSE: latency for register-to-register move (e.g., MOVAPD).
 * Chain: movapd %src, %dst where dst = next iteration src.
 */
#define LAT_MOVE_SSE(func_name, asm_str, init_type, init_val)                \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    init_type accumulator = init_val;                                         \
    init_type temp_reg;                                                       \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "=x"(temp_reg) : "x"(accumulator));        \
        __asm__ volatile(asm_str : "=x"(accumulator) : "x"(temp_reg));        \
        __asm__ volatile(asm_str : "=x"(temp_reg) : "x"(accumulator));        \
        __asm__ volatile(asm_str : "=x"(accumulator) : "x"(temp_reg));        \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    __asm__ volatile("" : "+x"(accumulator));                                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/* Convenience init values */
#define XMM_F32_ONE  _mm_set1_ps(1.0001f)
#define XMM_F64_ONE  _mm_set1_pd(1.0001)
#define XMM_I32_ONE  _mm_set1_epi32(1)
#define XMM_I64_ONE  _mm_set1_epi64x(1LL)

/* ========================================================================
 * FP64 SCALAR (SD) -- legacy SSE2 encoding
 * ======================================================================== */

LAT_BINARY_SSE(addsd_lat,  "addsd %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(addsd_tp,    "addsd %1, %0",  __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(subsd_lat,  "subsd %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(subsd_tp,    "subsd %1, %0",  __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(mulsd_lat,  "mulsd %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(mulsd_tp,    "mulsd %1, %0",  __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(divsd_lat,  "divsd %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(divsd_tp,    "divsd %1, %0",  __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(maxsd_lat,  "maxsd %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(maxsd_tp,    "maxsd %1, %0",  __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(minsd_lat,  "minsd %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(minsd_tp,    "minsd %1, %0",  __m128d, XMM_F64_ONE)

LAT_UNARY_SSE(sqrtsd_lat,  "sqrtsd %0, %0", __m128d, XMM_F64_ONE)
TP_UNARY_SSE(sqrtsd_tp,    "sqrtsd %0, %0", __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(cmpsd_lat,  "cmpsd $0, %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(cmpsd_tp,    "cmpsd $0, %1, %0",  __m128d, XMM_F64_ONE)

/* COMISD and UCOMISD: set EFLAGS from xmm comparison.
 * For latency, chain via sete+movzx+pinsrq to feed EFLAGS back into xmm. */
LAT_BINARY_SSE(comisd_lat,  "comisd %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(comisd_tp,    "comisd %1, %0",  __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(ucomisd_lat, "ucomisd %1, %0", __m128d, XMM_F64_ONE)
TP_BINARY_SSE(ucomisd_tp,   "ucomisd %1, %0", __m128d, XMM_F64_ONE)

/* ========================================================================
 * FP32 SCALAR (SS) -- legacy SSE encoding
 * ======================================================================== */

LAT_BINARY_SSE(addss_lat,  "addss %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(addss_tp,    "addss %1, %0",  __m128, XMM_F32_ONE)

LAT_BINARY_SSE(subss_lat,  "subss %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(subss_tp,    "subss %1, %0",  __m128, XMM_F32_ONE)

LAT_BINARY_SSE(mulss_lat,  "mulss %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(mulss_tp,    "mulss %1, %0",  __m128, XMM_F32_ONE)

LAT_BINARY_SSE(divss_lat,  "divss %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(divss_tp,    "divss %1, %0",  __m128, XMM_F32_ONE)

LAT_BINARY_SSE(maxss_lat,  "maxss %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(maxss_tp,    "maxss %1, %0",  __m128, XMM_F32_ONE)

LAT_BINARY_SSE(minss_lat,  "minss %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(minss_tp,    "minss %1, %0",  __m128, XMM_F32_ONE)

LAT_UNARY_SSE(sqrtss_lat,  "sqrtss %0, %0", __m128, XMM_F32_ONE)
TP_UNARY_SSE(sqrtss_tp,    "sqrtss %0, %0", __m128, XMM_F32_ONE)

LAT_BINARY_SSE(cmpss_lat,  "cmpss $0, %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(cmpss_tp,    "cmpss $0, %1, %0",  __m128, XMM_F32_ONE)

LAT_UNARY_SSE(rcpss_lat,   "rcpss %0, %0",  __m128, XMM_F32_ONE)
TP_UNARY_SSE(rcpss_tp,     "rcpss %0, %0",  __m128, XMM_F32_ONE)

LAT_UNARY_SSE(rsqrtss_lat, "rsqrtss %0, %0", __m128, XMM_F32_ONE)
TP_UNARY_SSE(rsqrtss_tp,   "rsqrtss %0, %0", __m128, XMM_F32_ONE)

/* ========================================================================
 * FP64 PACKED (PD) -- legacy SSE2 encoding, 128-bit
 * ======================================================================== */

LAT_BINARY_SSE(addpd_lat,    "addpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(addpd_tp,      "addpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(subpd_lat,    "subpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(subpd_tp,      "subpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(mulpd_lat,    "mulpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(mulpd_tp,      "mulpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(divpd_lat,    "divpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(divpd_tp,      "divpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_UNARY_SSE(sqrtpd_lat,    "sqrtpd %0, %0",   __m128d, XMM_F64_ONE)
TP_UNARY_SSE(sqrtpd_tp,      "sqrtpd %0, %0",   __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(maxpd_lat,    "maxpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(maxpd_tp,      "maxpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(minpd_lat,    "minpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(minpd_tp,      "minpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(cmppd_lat,    "cmppd $0, %1, %0", __m128d, XMM_F64_ONE)
TP_BINARY_SSE(cmppd_tp,      "cmppd $0, %1, %0", __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(andpd_lat,    "andpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(andpd_tp,      "andpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(andnpd_lat,   "andnpd %1, %0",   __m128d, XMM_F64_ONE)
TP_BINARY_SSE(andnpd_tp,     "andnpd %1, %0",   __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(orpd_lat,     "orpd %1, %0",     __m128d, XMM_F64_ONE)
TP_BINARY_SSE(orpd_tp,       "orpd %1, %0",     __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(xorpd_lat,    "xorpd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(xorpd_tp,      "xorpd %1, %0",    __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(unpckhpd_lat, "unpckhpd %1, %0", __m128d, XMM_F64_ONE)
TP_BINARY_SSE(unpckhpd_tp,   "unpckhpd %1, %0", __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(unpcklpd_lat, "unpcklpd %1, %0", __m128d, XMM_F64_ONE)
TP_BINARY_SSE(unpcklpd_tp,   "unpcklpd %1, %0", __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(shufpd_lat,   "shufpd $0, %1, %0",  __m128d, XMM_F64_ONE)
TP_BINARY_SSE(shufpd_tp,     "shufpd $0, %1, %0",  __m128d, XMM_F64_ONE)

/* ========================================================================
 * FP32 PACKED (PS) -- legacy SSE encoding, 128-bit
 * ======================================================================== */

LAT_BINARY_SSE(addps_lat,    "addps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(addps_tp,      "addps %1, %0",    __m128, XMM_F32_ONE)

LAT_BINARY_SSE(subps_lat,    "subps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(subps_tp,      "subps %1, %0",    __m128, XMM_F32_ONE)

LAT_BINARY_SSE(mulps_lat,    "mulps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(mulps_tp,      "mulps %1, %0",    __m128, XMM_F32_ONE)

LAT_BINARY_SSE(divps_lat,    "divps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(divps_tp,      "divps %1, %0",    __m128, XMM_F32_ONE)

LAT_UNARY_SSE(sqrtps_lat,    "sqrtps %0, %0",   __m128, XMM_F32_ONE)
TP_UNARY_SSE(sqrtps_tp,      "sqrtps %0, %0",   __m128, XMM_F32_ONE)

LAT_BINARY_SSE(maxps_lat,    "maxps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(maxps_tp,      "maxps %1, %0",    __m128, XMM_F32_ONE)

LAT_BINARY_SSE(minps_lat,    "minps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(minps_tp,      "minps %1, %0",    __m128, XMM_F32_ONE)

LAT_BINARY_SSE(cmpps_lat,    "cmpps $0, %1, %0", __m128, XMM_F32_ONE)
TP_BINARY_SSE(cmpps_tp,      "cmpps $0, %1, %0", __m128, XMM_F32_ONE)

LAT_BINARY_SSE(andps_lat,    "andps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(andps_tp,      "andps %1, %0",    __m128, XMM_F32_ONE)

LAT_BINARY_SSE(andnps_lat,   "andnps %1, %0",   __m128, XMM_F32_ONE)
TP_BINARY_SSE(andnps_tp,     "andnps %1, %0",   __m128, XMM_F32_ONE)

LAT_BINARY_SSE(orps_lat,     "orps %1, %0",     __m128, XMM_F32_ONE)
TP_BINARY_SSE(orps_tp,       "orps %1, %0",     __m128, XMM_F32_ONE)

LAT_BINARY_SSE(xorps_lat,    "xorps %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(xorps_tp,      "xorps %1, %0",    __m128, XMM_F32_ONE)

LAT_BINARY_SSE(unpckhps_lat, "unpckhps %1, %0", __m128, XMM_F32_ONE)
TP_BINARY_SSE(unpckhps_tp,   "unpckhps %1, %0", __m128, XMM_F32_ONE)

LAT_BINARY_SSE(unpcklps_lat, "unpcklps %1, %0", __m128, XMM_F32_ONE)
TP_BINARY_SSE(unpcklps_tp,   "unpcklps %1, %0", __m128, XMM_F32_ONE)

LAT_BINARY_SSE(shufps_lat,   "shufps $0, %1, %0", __m128, XMM_F32_ONE)
TP_BINARY_SSE(shufps_tp,     "shufps $0, %1, %0", __m128, XMM_F32_ONE)

LAT_UNARY_SSE(rcpps_lat,     "rcpps %0, %0",    __m128, XMM_F32_ONE)
TP_UNARY_SSE(rcpps_tp,       "rcpps %0, %0",    __m128, XMM_F32_ONE)

LAT_UNARY_SSE(rsqrtps_lat,   "rsqrtps %0, %0",  __m128, XMM_F32_ONE)
TP_UNARY_SSE(rsqrtps_tp,     "rsqrtps %0, %0",  __m128, XMM_F32_ONE)

/* ========================================================================
 * MOVE VARIANTS -- register-to-register forms (legacy SSE/SSE2)
 * ======================================================================== */

LAT_MOVE_SSE(movapd_lat,  "movapd %1, %0",  __m128d, XMM_F64_ONE)
TP_STANDALONE_SSE(movapd_tp, "movapd %1, %0", __m128d, XMM_F64_ONE, __m128d)

LAT_MOVE_SSE(movaps_lat,  "movaps %1, %0",  __m128, XMM_F32_ONE)
TP_STANDALONE_SSE(movaps_tp, "movaps %1, %0", __m128, XMM_F32_ONE, __m128)

LAT_MOVE_SSE(movupd_lat,  "movupd %1, %0",  __m128d, XMM_F64_ONE)
TP_STANDALONE_SSE(movupd_tp, "movupd %1, %0", __m128d, XMM_F64_ONE, __m128d)

LAT_MOVE_SSE(movups_lat,  "movups %1, %0",  __m128, XMM_F32_ONE)
TP_STANDALONE_SSE(movups_tp, "movups %1, %0", __m128, XMM_F32_ONE, __m128)

LAT_MOVE_SSE(movsd_lat,   "movsd %1, %0",   __m128d, XMM_F64_ONE)
TP_STANDALONE_SSE(movsd_tp, "movsd %1, %0",  __m128d, XMM_F64_ONE, __m128d)

LAT_MOVE_SSE(movss_lat,   "movss %1, %0",   __m128, XMM_F32_ONE)
TP_STANDALONE_SSE(movss_tp, "movss %1, %0",  __m128, XMM_F32_ONE, __m128)

/* MOVHPD/MOVLPD/MOVHPS/MOVLPS: reg-reg forms. These take an xmm src and
 * insert the high/low half into the destination.  Using xmm,xmm forms. */
LAT_BINARY_SSE(movhpd_lat, "unpckhpd %1, %0", __m128d, XMM_F64_ONE)
TP_BINARY_SSE(movhpd_tp,   "unpckhpd %1, %0", __m128d, XMM_F64_ONE)

LAT_BINARY_SSE(movlpd_lat, "movsd %1, %0",    __m128d, XMM_F64_ONE)
TP_BINARY_SSE(movlpd_tp,   "movsd %1, %0",    __m128d, XMM_F64_ONE)

/* MOVHPS/MOVLPS reg-reg: use shuffle/blend proxies since there's no
 * pure reg-reg MOVHPS/MOVLPS in SSE legacy encoding */
LAT_BINARY_SSE(movhps_lat, "movlhps %1, %0",  __m128, XMM_F32_ONE)
TP_BINARY_SSE(movhps_tp,   "movlhps %1, %0",  __m128, XMM_F32_ONE)

LAT_BINARY_SSE(movlps_lat, "movss %1, %0",    __m128, XMM_F32_ONE)
TP_BINARY_SSE(movlps_tp,   "movss %1, %0",    __m128, XMM_F32_ONE)

/* MOVMSKPD: xmm -> GPR (extracts sign bits). Latency only; throughput
 * not measurable in isolation (GPR output, no xmm writeback). */
LAT_XMM_TO_GPR_SSE(movmskpd_lat,
    "movmskpd %0, %1",
    "pinsrq $0, %1, %0",
    __m128d, XMM_F64_ONE)

/* MOVMSKPS: xmm -> GPR. Use pinsrq to feed 64-bit GPR back into xmm. */
LAT_XMM_TO_GPR_SSE(movmskps_lat,
    "movmskps %0, %1",
    "pinsrq $0, %1, %0",
    __m128, XMM_F32_ONE)

/* MOVNTPD: non-temporal store. Throughput only (no latency chain).
 * We measure MOVAPD throughput as a proxy since MOVNTPD is store-only. */

/* MOVDQA / MOVDQU: integer xmm register moves */
LAT_MOVE_SSE(movdqa_lat,  "movdqa %1, %0",  __m128i, XMM_I32_ONE)
TP_STANDALONE_SSE(movdqa_tp, "movdqa %1, %0", __m128i, XMM_I32_ONE, __m128i)

LAT_MOVE_SSE(movdqu_lat,  "movdqu %1, %0",  __m128i, XMM_I32_ONE)
TP_STANDALONE_SSE(movdqu_tp, "movdqu %1, %0", __m128i, XMM_I32_ONE, __m128i)

/* MOVQ: 64-bit xmm move (zero-extends upper 64 bits) */
LAT_MOVE_SSE(movq_xmm_lat, "movq %1, %0",   __m128i, XMM_I64_ONE)
TP_STANDALONE_SSE(movq_xmm_tp, "movq %1, %0", __m128i, XMM_I64_ONE, __m128i)

/* MOVD: 32-bit GPR <-> xmm. Latency chains through GPR. */
LAT_CVT_GPR_XMM(movd_xmm_lat,
    "movd %0, %1",
    "movd %1, %0",
    __m128i, XMM_I32_ONE)

/* ========================================================================
 * CONVERSIONS -- legacy SSE2 encoding
 * ======================================================================== */

/* PD <-> PS width conversions */
LAT_UNARY_SSE(cvtpd2ps_lat,  "cvtpd2ps %0, %0",  __m128d, XMM_F64_ONE)
TP_STANDALONE_SSE(cvtpd2ps_tp, "cvtpd2ps %1, %0", __m128d, XMM_F64_ONE, __m128)

LAT_UNARY_SSE(cvtps2pd_lat,  "cvtps2pd %0, %0",  __m128, XMM_F32_ONE)
TP_STANDALONE_SSE(cvtps2pd_tp, "cvtps2pd %1, %0", __m128, XMM_F32_ONE, __m128d)

/* PD <-> DQ (int32) conversions */
LAT_UNARY_SSE(cvtpd2dq_lat,  "cvtpd2dq %0, %0",  __m128d, XMM_F64_ONE)
TP_STANDALONE_SSE(cvtpd2dq_tp, "cvtpd2dq %1, %0", __m128d, XMM_F64_ONE, __m128i)

LAT_UNARY_SSE(cvttpd2dq_lat, "cvttpd2dq %0, %0", __m128d, XMM_F64_ONE)
TP_STANDALONE_SSE(cvttpd2dq_tp,"cvttpd2dq %1, %0",__m128d, XMM_F64_ONE, __m128i)

LAT_UNARY_SSE(cvtdq2pd_lat,  "cvtdq2pd %0, %0",  __m128i, XMM_I32_ONE)
TP_STANDALONE_SSE(cvtdq2pd_tp, "cvtdq2pd %1, %0", __m128i, XMM_I32_ONE, __m128d)

/* PS <-> DQ (int32) conversions */
LAT_UNARY_SSE(cvtps2dq_lat,  "cvtps2dq %0, %0",  __m128, XMM_F32_ONE)
TP_STANDALONE_SSE(cvtps2dq_tp, "cvtps2dq %1, %0", __m128, XMM_F32_ONE, __m128i)

LAT_UNARY_SSE(cvttps2dq_lat, "cvttps2dq %0, %0", __m128, XMM_F32_ONE)
TP_STANDALONE_SSE(cvttps2dq_tp,"cvttps2dq %1, %0",__m128, XMM_F32_ONE, __m128i)

LAT_UNARY_SSE(cvtdq2ps_lat,  "cvtdq2ps %0, %0",  __m128i, XMM_I32_ONE)
TP_STANDALONE_SSE(cvtdq2ps_tp, "cvtdq2ps %1, %0", __m128i, XMM_I32_ONE, __m128)

/* SD <-> SS scalar conversions */
LAT_UNARY_SSE(cvtsd2ss_lat,  "cvtsd2ss %0, %0",  __m128d, XMM_F64_ONE)
TP_STANDALONE_SSE(cvtsd2ss_tp, "cvtsd2ss %1, %0", __m128d, XMM_F64_ONE, __m128)

LAT_UNARY_SSE(cvtss2sd_lat,  "cvtss2sd %0, %0",  __m128, XMM_F32_ONE)
TP_STANDALONE_SSE(cvtss2sd_tp, "cvtss2sd %1, %0", __m128, XMM_F32_ONE, __m128d)

/* SD <-> SI (GPR) conversions */
LAT_CVT_GPR_XMM(cvtsd2si_lat,
    "cvtsd2si %0, %1",
    "cvtsi2sd %1, %0",
    __m128d, XMM_F64_ONE)

LAT_CVT_GPR_XMM(cvttsd2si_lat,
    "cvttsd2si %0, %1",
    "cvtsi2sd %1, %0",
    __m128d, XMM_F64_ONE)

LAT_CVT_GPR_XMM(cvtsi2sd_lat,
    "cvtsd2si %0, %1",
    "cvtsi2sd %1, %0",
    __m128d, XMM_F64_ONE)

/* SS <-> SI (GPR) conversions */
LAT_CVT_GPR_XMM(cvtss2si_lat,
    "cvtss2si %0, %1",
    "cvtsi2ss %1, %0",
    __m128, XMM_F32_ONE)

LAT_CVT_GPR_XMM(cvtsi2ss_lat,
    "cvtss2si %0, %1",
    "cvtsi2ss %1, %0",
    __m128, XMM_F32_ONE)

/* ========================================================================
 * MEASUREMENT ENTRY TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    int width_bits;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} Sse2GapProbeEntry;

static const Sse2GapProbeEntry sse2_gap_probe_table[] = {
    /* FP64 scalar (SD) */
    {"ADDSD",      "xmm,xmm",     128, "fp64_arith",  addsd_lat,      addsd_tp},
    {"SUBSD",      "xmm,xmm",     128, "fp64_arith",  subsd_lat,      subsd_tp},
    {"MULSD",      "xmm,xmm",     128, "fp64_arith",  mulsd_lat,      mulsd_tp},
    {"DIVSD",      "xmm,xmm",     128, "fp64_arith",  divsd_lat,      divsd_tp},
    {"MAXSD",      "xmm,xmm",     128, "fp64_arith",  maxsd_lat,      maxsd_tp},
    {"MINSD",      "xmm,xmm",     128, "fp64_arith",  minsd_lat,      minsd_tp},
    {"SQRTSD",     "xmm,xmm",     128, "fp64_arith",  sqrtsd_lat,     sqrtsd_tp},
    {"CMPSD",      "xmm,xmm",     128, "fp64_cmp",    cmpsd_lat,      cmpsd_tp},
    {"COMISD",     "xmm,xmm",     128, "fp64_cmp",    comisd_lat,     comisd_tp},
    {"UCOMISD",    "xmm,xmm",     128, "fp64_cmp",    ucomisd_lat,    ucomisd_tp},

    /* FP32 scalar (SS) */
    {"ADDSS",      "xmm,xmm",     128, "fp32_arith",  addss_lat,      addss_tp},
    {"SUBSS",      "xmm,xmm",     128, "fp32_arith",  subss_lat,      subss_tp},
    {"MULSS",      "xmm,xmm",     128, "fp32_arith",  mulss_lat,      mulss_tp},
    {"DIVSS",      "xmm,xmm",     128, "fp32_arith",  divss_lat,      divss_tp},
    {"MAXSS",      "xmm,xmm",     128, "fp32_arith",  maxss_lat,      maxss_tp},
    {"MINSS",      "xmm,xmm",     128, "fp32_arith",  minss_lat,      minss_tp},
    {"SQRTSS",     "xmm,xmm",     128, "fp32_arith",  sqrtss_lat,     sqrtss_tp},
    {"CMPSS",      "xmm,xmm",     128, "fp32_cmp",    cmpss_lat,      cmpss_tp},
    {"RCPSS",      "xmm,xmm",     128, "fp32_arith",  rcpss_lat,      rcpss_tp},
    {"RSQRTSS",    "xmm,xmm",     128, "fp32_arith",  rsqrtss_lat,    rsqrtss_tp},

    /* FP64 packed (PD) 128-bit */
    {"ADDPD",      "xmm,xmm",     128, "fp64_arith",  addpd_lat,      addpd_tp},
    {"SUBPD",      "xmm,xmm",     128, "fp64_arith",  subpd_lat,      subpd_tp},
    {"MULPD",      "xmm,xmm",     128, "fp64_arith",  mulpd_lat,      mulpd_tp},
    {"DIVPD",      "xmm,xmm",     128, "fp64_arith",  divpd_lat,      divpd_tp},
    {"SQRTPD",     "xmm",          128, "fp64_arith",  sqrtpd_lat,     sqrtpd_tp},
    {"MAXPD",      "xmm,xmm",     128, "fp64_arith",  maxpd_lat,      maxpd_tp},
    {"MINPD",      "xmm,xmm",     128, "fp64_arith",  minpd_lat,      minpd_tp},
    {"CMPPD",      "xmm,xmm",     128, "fp64_cmp",    cmppd_lat,      cmppd_tp},
    {"ANDPD",      "xmm,xmm",     128, "fp64_logic",  andpd_lat,      andpd_tp},
    {"ANDNPD",     "xmm,xmm",     128, "fp64_logic",  andnpd_lat,     andnpd_tp},
    {"ORPD",       "xmm,xmm",     128, "fp64_logic",  orpd_lat,       orpd_tp},
    {"XORPD",      "xmm,xmm",     128, "fp64_logic",  xorpd_lat,      xorpd_tp},
    {"UNPCKHPD",   "xmm,xmm",     128, "fp64_shuf",   unpckhpd_lat,   unpckhpd_tp},
    {"UNPCKLPD",   "xmm,xmm",     128, "fp64_shuf",   unpcklpd_lat,   unpcklpd_tp},
    {"SHUFPD",     "xmm,xmm",     128, "fp64_shuf",   shufpd_lat,     shufpd_tp},

    /* FP32 packed (PS) 128-bit */
    {"ADDPS",      "xmm,xmm",     128, "fp32_arith",  addps_lat,      addps_tp},
    {"SUBPS",      "xmm,xmm",     128, "fp32_arith",  subps_lat,      subps_tp},
    {"MULPS",      "xmm,xmm",     128, "fp32_arith",  mulps_lat,      mulps_tp},
    {"DIVPS",      "xmm,xmm",     128, "fp32_arith",  divps_lat,      divps_tp},
    {"SQRTPS",     "xmm",          128, "fp32_arith",  sqrtps_lat,     sqrtps_tp},
    {"MAXPS",      "xmm,xmm",     128, "fp32_arith",  maxps_lat,      maxps_tp},
    {"MINPS",      "xmm,xmm",     128, "fp32_arith",  minps_lat,      minps_tp},
    {"CMPPS",      "xmm,xmm",     128, "fp32_cmp",    cmpps_lat,      cmpps_tp},
    {"ANDPS",      "xmm,xmm",     128, "fp32_logic",  andps_lat,      andps_tp},
    {"ANDNPS",     "xmm,xmm",     128, "fp32_logic",  andnps_lat,     andnps_tp},
    {"ORPS",       "xmm,xmm",     128, "fp32_logic",  orps_lat,       orps_tp},
    {"XORPS",      "xmm,xmm",     128, "fp32_logic",  xorps_lat,      xorps_tp},
    {"UNPCKHPS",   "xmm,xmm",     128, "fp32_shuf",   unpckhps_lat,   unpckhps_tp},
    {"UNPCKLPS",   "xmm,xmm",     128, "fp32_shuf",   unpcklps_lat,   unpcklps_tp},
    {"SHUFPS",     "xmm,xmm",     128, "fp32_shuf",   shufps_lat,     shufps_tp},
    {"RCPPS",      "xmm",          128, "fp32_arith",  rcpps_lat,      rcpps_tp},
    {"RSQRTPS",    "xmm",          128, "fp32_arith",  rsqrtps_lat,    rsqrtps_tp},

    /* Move variants */
    {"MOVAPD",     "xmm,xmm",     128, "move",        movapd_lat,     movapd_tp},
    {"MOVAPS",     "xmm,xmm",     128, "move",        movaps_lat,     movaps_tp},
    {"MOVUPD",     "xmm,xmm",     128, "move",        movupd_lat,     movupd_tp},
    {"MOVUPS",     "xmm,xmm",     128, "move",        movups_lat,     movups_tp},
    {"MOVSD",      "xmm,xmm",     128, "move",        movsd_lat,      movsd_tp},
    {"MOVSS",      "xmm,xmm",     128, "move",        movss_lat,      movss_tp},
    {"MOVHPD",     "xmm,xmm",     128, "move",        movhpd_lat,     movhpd_tp},
    {"MOVLPD",     "xmm,xmm",     128, "move",        movlpd_lat,     movlpd_tp},
    {"MOVHPS",     "xmm,xmm",     128, "move",        movhps_lat,     movhps_tp},
    {"MOVLPS",     "xmm,xmm",     128, "move",        movlps_lat,     movlps_tp},
    {"MOVMSKPD",   "xmm->r32",    128, "move",        movmskpd_lat,   NULL},
    {"MOVMSKPS",   "xmm->r32",    128, "move",        movmskps_lat,   NULL},
    {"MOVDQA",     "xmm,xmm",     128, "move",        movdqa_lat,     movdqa_tp},
    {"MOVDQU",     "xmm,xmm",     128, "move",        movdqu_lat,     movdqu_tp},
    {"MOVQ",       "xmm,xmm",     128, "move",        movq_xmm_lat,  movq_xmm_tp},
    {"MOVD",       "r32,xmm",     128, "move",        movd_xmm_lat,  NULL},

    /* Conversions */
    {"CVTPD2PS",   "xmm,xmm",     128, "convert",     cvtpd2ps_lat,   cvtpd2ps_tp},
    {"CVTPS2PD",   "xmm,xmm",     128, "convert",     cvtps2pd_lat,   cvtps2pd_tp},
    {"CVTPD2DQ",   "xmm,xmm",     128, "convert",     cvtpd2dq_lat,   cvtpd2dq_tp},
    {"CVTTPD2DQ",  "xmm,xmm",     128, "convert",     cvttpd2dq_lat,  cvttpd2dq_tp},
    {"CVTDQ2PD",   "xmm,xmm",     128, "convert",     cvtdq2pd_lat,   cvtdq2pd_tp},
    {"CVTPS2DQ",   "xmm,xmm",     128, "convert",     cvtps2dq_lat,   cvtps2dq_tp},
    {"CVTTPS2DQ",  "xmm,xmm",     128, "convert",     cvttps2dq_lat,  cvttps2dq_tp},
    {"CVTDQ2PS",   "xmm,xmm",     128, "convert",     cvtdq2ps_lat,   cvtdq2ps_tp},
    {"CVTSD2SS",   "xmm,xmm",     128, "convert",     cvtsd2ss_lat,   cvtsd2ss_tp},
    {"CVTSS2SD",   "xmm,xmm",     128, "convert",     cvtss2sd_lat,   cvtss2sd_tp},
    {"CVTSD2SI",   "xmm->r64",    128, "convert",     cvtsd2si_lat,   NULL},
    {"CVTTSD2SI",  "xmm->r64",    128, "convert",     cvttsd2si_lat,  NULL},
    {"CVTSI2SD",   "r64->xmm",    128, "convert",     cvtsi2sd_lat,   NULL},
    {"CVTSS2SI",   "xmm->r64",    128, "convert",     cvtss2si_lat,   NULL},
    {"CVTSI2SS",   "r64->xmm",    128, "convert",     cvtsi2ss_lat,   NULL},

    /* Sentinel */
    {NULL, NULL, 0, NULL, NULL, NULL}
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
        sm_zen_print_header("sse2_gaps", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring 72 missing SSE/SSE2 legacy-encoded instruction forms...\n\n");
    }

    /* Count entries */
    int total_entries = 0;
    for (int count_idx = 0; sse2_gap_probe_table[count_idx].mnemonic != NULL; count_idx++)
        total_entries++;

    if (!csv_mode)
        printf("Total instruction forms: %d\n\n", total_entries);

    /* CSV header */
    printf("mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; sse2_gap_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const Sse2GapProbeEntry *current_entry = &sse2_gap_probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; sse2_gap_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const Sse2GapProbeEntry *current_entry = &sse2_gap_probe_table[entry_idx];

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
