#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_fma_gaps.c -- Fill 48 missing FMA3 instruction form measurements
 * on Zen 4 (Ryzen 9 7945HX).
 *
 * The prior exhaustive probes only measured the 231 operand-ordering forms.
 * This probe covers the 132 and 213 forms for all FMA3 operation types,
 * plus the missing 231 forms for FMADDSUB/FMSUBADD and scalar variants.
 *
 * Key question: do 132/213/231 forms have different latency or throughput
 * on Zen 4?  Expected: identical (they decode to the same uop), but worth
 * confirming.
 *
 * FMA3 operand ordering:
 *   vfmadd132ps a, b, c  =>  a = a*c + b   (overwrites first multiplicand)
 *   vfmadd213ps a, b, c  =>  a = b*a + c   (overwrites middle operand)
 *   vfmadd231ps a, b, c  =>  a = b*c + a   (overwrites addend/accumulator)
 *
 * Uses inline asm to force specific encodings since intrinsics let the
 * compiler choose any form.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -mf16c -msse4.1 -msse4.2 -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_fma_gaps.c -lm \
 *     -o ../../../../build/bin/zen_probe_fma_gaps
 */

/* ========================================================================
 * MACRO TEMPLATES for FMA latency and throughput
 *
 * FMA instructions are 3-operand: "+x"(dst), "x"(src1), "x"(src2)
 * The dst operand is both read and written (the accumulated result).
 *
 * For latency: 4-deep chain through the destination register.
 * For throughput: 8 independent accumulator streams.
 *
 * CRITICAL: __asm__ volatile("" : "+x"(stream_N)); barriers prevent
 * the compiler from optimizing away independent streams.
 * ======================================================================== */

/*
 * LAT_FMA_3OP: latency for VEX 3-operand FMA form.
 * asm_str must use %0 = dst (read/write), %1 = src1 (read), %2 = src2 (read).
 */
#define LAT_FMA_3OP(func_name, asm_str, init_type, init_val)                  \
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
 * TP_FMA_3OP: throughput for FMA, 8 independent streams.
 */
#define TP_FMA_3OP(func_name, asm_str, init_type, init_val)                   \
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

/* Convenience init values */
#define YMM_F32_INIT  _mm256_set1_ps(1.0001f)
#define YMM_F64_INIT  _mm256_set1_pd(1.0001)
#define XMM_F32_INIT  _mm_set1_ps(1.0001f)
#define XMM_F64_INIT  _mm_set1_pd(1.0001)

/* ========================================================================
 * VFMADD -- packed PS YMM (256-bit, 8xfloat)
 *
 * 132: dst = dst * src2 + src1   =>  vfmadd132ps src2, src1, dst
 * 213: dst = src1 * dst + src2   =>  vfmadd213ps src2, src1, dst
 * ======================================================================== */

LAT_FMA_3OP(vfmadd132ps_ymm_lat, "vfmadd132ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmadd132ps_ymm_tp,   "vfmadd132ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmadd213ps_ymm_lat, "vfmadd213ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmadd213ps_ymm_tp,   "vfmadd213ps %2, %1, %0", __m256, YMM_F32_INIT)

/* ========================================================================
 * VFMADD -- packed PD YMM (256-bit, 4xdouble)
 * ======================================================================== */

LAT_FMA_3OP(vfmadd132pd_ymm_lat, "vfmadd132pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmadd132pd_ymm_tp,   "vfmadd132pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfmadd213pd_ymm_lat, "vfmadd213pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmadd213pd_ymm_tp,   "vfmadd213pd %2, %1, %0", __m256d, YMM_F64_INIT)

/* ========================================================================
 * VFMADD -- scalar SS (xmm, float)
 * ======================================================================== */

LAT_FMA_3OP(vfmadd132ss_lat, "vfmadd132ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfmadd132ss_tp,   "vfmadd132ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA_3OP(vfmadd213ss_lat, "vfmadd213ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfmadd213ss_tp,   "vfmadd213ss %2, %1, %0", __m128, XMM_F32_INIT)

/* ========================================================================
 * VFMADD -- scalar SD (xmm, double)
 * ======================================================================== */

LAT_FMA_3OP(vfmadd132sd_lat, "vfmadd132sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfmadd132sd_tp,   "vfmadd132sd %2, %1, %0", __m128d, XMM_F64_INIT)

LAT_FMA_3OP(vfmadd213sd_lat, "vfmadd213sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfmadd213sd_tp,   "vfmadd213sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* ========================================================================
 * VFMSUB -- packed PS/PD YMM, scalar SS/SD
 * ======================================================================== */

LAT_FMA_3OP(vfmsub132ps_ymm_lat, "vfmsub132ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmsub132ps_ymm_tp,   "vfmsub132ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmsub213ps_ymm_lat, "vfmsub213ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmsub213ps_ymm_tp,   "vfmsub213ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmsub132pd_ymm_lat, "vfmsub132pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmsub132pd_ymm_tp,   "vfmsub132pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfmsub213pd_ymm_lat, "vfmsub213pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmsub213pd_ymm_tp,   "vfmsub213pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfmsub132ss_lat, "vfmsub132ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfmsub132ss_tp,   "vfmsub132ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA_3OP(vfmsub213ss_lat, "vfmsub213ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfmsub213ss_tp,   "vfmsub213ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA_3OP(vfmsub132sd_lat, "vfmsub132sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfmsub132sd_tp,   "vfmsub132sd %2, %1, %0", __m128d, XMM_F64_INIT)

LAT_FMA_3OP(vfmsub213sd_lat, "vfmsub213sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfmsub213sd_tp,   "vfmsub213sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* Missing VFMSUB231 forms (PD and SD) from gap report */
LAT_FMA_3OP(vfmsub231pd_ymm_lat, "vfmsub231pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmsub231pd_ymm_tp,   "vfmsub231pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfmsub231sd_lat, "vfmsub231sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfmsub231sd_tp,   "vfmsub231sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* ========================================================================
 * VFNMADD -- packed PS/PD YMM, scalar SS/SD
 * ======================================================================== */

LAT_FMA_3OP(vfnmadd132ps_ymm_lat, "vfnmadd132ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfnmadd132ps_ymm_tp,   "vfnmadd132ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfnmadd213ps_ymm_lat, "vfnmadd213ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfnmadd213ps_ymm_tp,   "vfnmadd213ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfnmadd132pd_ymm_lat, "vfnmadd132pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfnmadd132pd_ymm_tp,   "vfnmadd132pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfnmadd213pd_ymm_lat, "vfnmadd213pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfnmadd213pd_ymm_tp,   "vfnmadd213pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfnmadd132ss_lat, "vfnmadd132ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfnmadd132ss_tp,   "vfnmadd132ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA_3OP(vfnmadd213ss_lat, "vfnmadd213ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfnmadd213ss_tp,   "vfnmadd213ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA_3OP(vfnmadd132sd_lat, "vfnmadd132sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfnmadd132sd_tp,   "vfnmadd132sd %2, %1, %0", __m128d, XMM_F64_INIT)

LAT_FMA_3OP(vfnmadd213sd_lat, "vfnmadd213sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfnmadd213sd_tp,   "vfnmadd213sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* Missing VFNMADD231 forms (PD and SD) */
LAT_FMA_3OP(vfnmadd231pd_ymm_lat, "vfnmadd231pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfnmadd231pd_ymm_tp,   "vfnmadd231pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfnmadd231sd_lat, "vfnmadd231sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfnmadd231sd_tp,   "vfnmadd231sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* ========================================================================
 * VFNMSUB -- packed PS/PD YMM, scalar SS/SD
 * ======================================================================== */

LAT_FMA_3OP(vfnmsub132ps_ymm_lat, "vfnmsub132ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfnmsub132ps_ymm_tp,   "vfnmsub132ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfnmsub213ps_ymm_lat, "vfnmsub213ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfnmsub213ps_ymm_tp,   "vfnmsub213ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfnmsub132pd_ymm_lat, "vfnmsub132pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfnmsub132pd_ymm_tp,   "vfnmsub132pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfnmsub213pd_ymm_lat, "vfnmsub213pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfnmsub213pd_ymm_tp,   "vfnmsub213pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfnmsub132ss_lat, "vfnmsub132ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfnmsub132ss_tp,   "vfnmsub132ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA_3OP(vfnmsub213ss_lat, "vfnmsub213ss %2, %1, %0", __m128, XMM_F32_INIT)
TP_FMA_3OP(vfnmsub213ss_tp,   "vfnmsub213ss %2, %1, %0", __m128, XMM_F32_INIT)

LAT_FMA_3OP(vfnmsub132sd_lat, "vfnmsub132sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfnmsub132sd_tp,   "vfnmsub132sd %2, %1, %0", __m128d, XMM_F64_INIT)

LAT_FMA_3OP(vfnmsub213sd_lat, "vfnmsub213sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfnmsub213sd_tp,   "vfnmsub213sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* Missing VFNMSUB231 forms (PD and SD) */
LAT_FMA_3OP(vfnmsub231pd_ymm_lat, "vfnmsub231pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfnmsub231pd_ymm_tp,   "vfnmsub231pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfnmsub231sd_lat, "vfnmsub231sd %2, %1, %0", __m128d, XMM_F64_INIT)
TP_FMA_3OP(vfnmsub231sd_tp,   "vfnmsub231sd %2, %1, %0", __m128d, XMM_F64_INIT)

/* ========================================================================
 * VFMADDSUB -- packed PS/PD YMM (all three orderings)
 * ======================================================================== */

LAT_FMA_3OP(vfmaddsub132ps_ymm_lat, "vfmaddsub132ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmaddsub132ps_ymm_tp,   "vfmaddsub132ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmaddsub213ps_ymm_lat, "vfmaddsub213ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmaddsub213ps_ymm_tp,   "vfmaddsub213ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmaddsub231ps_ymm_lat, "vfmaddsub231ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmaddsub231ps_ymm_tp,   "vfmaddsub231ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmaddsub132pd_ymm_lat, "vfmaddsub132pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmaddsub132pd_ymm_tp,   "vfmaddsub132pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfmaddsub213pd_ymm_lat, "vfmaddsub213pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmaddsub213pd_ymm_tp,   "vfmaddsub213pd %2, %1, %0", __m256d, YMM_F64_INIT)

/* Note: VFMADDSUB231PD was also missing from the gap report */
LAT_FMA_3OP(vfmaddsub231pd_ymm_lat, "vfmaddsub231pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmaddsub231pd_ymm_tp,   "vfmaddsub231pd %2, %1, %0", __m256d, YMM_F64_INIT)

/* ========================================================================
 * VFMSUBADD -- packed PS/PD YMM (all three orderings)
 * ======================================================================== */

LAT_FMA_3OP(vfmsubadd132ps_ymm_lat, "vfmsubadd132ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmsubadd132ps_ymm_tp,   "vfmsubadd132ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmsubadd213ps_ymm_lat, "vfmsubadd213ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmsubadd213ps_ymm_tp,   "vfmsubadd213ps %2, %1, %0", __m256, YMM_F32_INIT)

/* Note: VFMSUBADD231PD was missing from gap report */
LAT_FMA_3OP(vfmsubadd231ps_ymm_lat, "vfmsubadd231ps %2, %1, %0", __m256, YMM_F32_INIT)
TP_FMA_3OP(vfmsubadd231ps_ymm_tp,   "vfmsubadd231ps %2, %1, %0", __m256, YMM_F32_INIT)

LAT_FMA_3OP(vfmsubadd132pd_ymm_lat, "vfmsubadd132pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmsubadd132pd_ymm_tp,   "vfmsubadd132pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfmsubadd213pd_ymm_lat, "vfmsubadd213pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmsubadd213pd_ymm_tp,   "vfmsubadd213pd %2, %1, %0", __m256d, YMM_F64_INIT)

LAT_FMA_3OP(vfmsubadd231pd_ymm_lat, "vfmsubadd231pd %2, %1, %0", __m256d, YMM_F64_INIT)
TP_FMA_3OP(vfmsubadd231pd_ymm_tp,   "vfmsubadd231pd %2, %1, %0", __m256d, YMM_F64_INIT)

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
} FmaGapProbeEntry;

static const FmaGapProbeEntry fma_gap_probe_table[] = {
    /* VFMADD 132/213 packed PS YMM */
    {"VFMADD132PS",  "ymm,ymm,ymm",  256, "fma",  vfmadd132ps_ymm_lat,  vfmadd132ps_ymm_tp},
    {"VFMADD213PS",  "ymm,ymm,ymm",  256, "fma",  vfmadd213ps_ymm_lat,  vfmadd213ps_ymm_tp},

    /* VFMADD 132/213 packed PD YMM */
    {"VFMADD132PD",  "ymm,ymm,ymm",  256, "fma",  vfmadd132pd_ymm_lat,  vfmadd132pd_ymm_tp},
    {"VFMADD213PD",  "ymm,ymm,ymm",  256, "fma",  vfmadd213pd_ymm_lat,  vfmadd213pd_ymm_tp},

    /* VFMADD 132/213 scalar SS */
    {"VFMADD132SS",  "xmm,xmm,xmm",  128, "fma",  vfmadd132ss_lat,      vfmadd132ss_tp},
    {"VFMADD213SS",  "xmm,xmm,xmm",  128, "fma",  vfmadd213ss_lat,      vfmadd213ss_tp},

    /* VFMADD 132/213 scalar SD */
    {"VFMADD132SD",  "xmm,xmm,xmm",  128, "fma",  vfmadd132sd_lat,      vfmadd132sd_tp},
    {"VFMADD213SD",  "xmm,xmm,xmm",  128, "fma",  vfmadd213sd_lat,      vfmadd213sd_tp},

    /* VFMSUB 132/213 packed PS YMM */
    {"VFMSUB132PS",  "ymm,ymm,ymm",  256, "fma",  vfmsub132ps_ymm_lat,  vfmsub132ps_ymm_tp},
    {"VFMSUB213PS",  "ymm,ymm,ymm",  256, "fma",  vfmsub213ps_ymm_lat,  vfmsub213ps_ymm_tp},

    /* VFMSUB 132/213 packed PD YMM */
    {"VFMSUB132PD",  "ymm,ymm,ymm",  256, "fma",  vfmsub132pd_ymm_lat,  vfmsub132pd_ymm_tp},
    {"VFMSUB213PD",  "ymm,ymm,ymm",  256, "fma",  vfmsub213pd_ymm_lat,  vfmsub213pd_ymm_tp},

    /* VFMSUB 132/213 scalar SS */
    {"VFMSUB132SS",  "xmm,xmm,xmm",  128, "fma",  vfmsub132ss_lat,      vfmsub132ss_tp},
    {"VFMSUB213SS",  "xmm,xmm,xmm",  128, "fma",  vfmsub213ss_lat,      vfmsub213ss_tp},

    /* VFMSUB 132/213 scalar SD */
    {"VFMSUB132SD",  "xmm,xmm,xmm",  128, "fma",  vfmsub132sd_lat,      vfmsub132sd_tp},
    {"VFMSUB213SD",  "xmm,xmm,xmm",  128, "fma",  vfmsub213sd_lat,      vfmsub213sd_tp},

    /* VFMSUB231 missing PD and SD */
    {"VFMSUB231PD",  "ymm,ymm,ymm",  256, "fma",  vfmsub231pd_ymm_lat,  vfmsub231pd_ymm_tp},
    {"VFMSUB231SD",  "xmm,xmm,xmm",  128, "fma",  vfmsub231sd_lat,      vfmsub231sd_tp},

    /* VFNMADD 132/213 packed PS YMM */
    {"VFNMADD132PS", "ymm,ymm,ymm",  256, "fma",  vfnmadd132ps_ymm_lat, vfnmadd132ps_ymm_tp},
    {"VFNMADD213PS", "ymm,ymm,ymm",  256, "fma",  vfnmadd213ps_ymm_lat, vfnmadd213ps_ymm_tp},

    /* VFNMADD 132/213 packed PD YMM */
    {"VFNMADD132PD", "ymm,ymm,ymm",  256, "fma",  vfnmadd132pd_ymm_lat, vfnmadd132pd_ymm_tp},
    {"VFNMADD213PD", "ymm,ymm,ymm",  256, "fma",  vfnmadd213pd_ymm_lat, vfnmadd213pd_ymm_tp},

    /* VFNMADD 132/213 scalar SS */
    {"VFNMADD132SS", "xmm,xmm,xmm",  128, "fma",  vfnmadd132ss_lat,     vfnmadd132ss_tp},
    {"VFNMADD213SS", "xmm,xmm,xmm",  128, "fma",  vfnmadd213ss_lat,     vfnmadd213ss_tp},

    /* VFNMADD 132/213 scalar SD */
    {"VFNMADD132SD", "xmm,xmm,xmm",  128, "fma",  vfnmadd132sd_lat,     vfnmadd132sd_tp},
    {"VFNMADD213SD", "xmm,xmm,xmm",  128, "fma",  vfnmadd213sd_lat,     vfnmadd213sd_tp},

    /* VFNMADD231 missing PD and SD */
    {"VFNMADD231PD", "ymm,ymm,ymm",  256, "fma",  vfnmadd231pd_ymm_lat, vfnmadd231pd_ymm_tp},
    {"VFNMADD231SD", "xmm,xmm,xmm",  128, "fma",  vfnmadd231sd_lat,     vfnmadd231sd_tp},

    /* VFNMSUB 132/213 packed PS YMM */
    {"VFNMSUB132PS", "ymm,ymm,ymm",  256, "fma",  vfnmsub132ps_ymm_lat, vfnmsub132ps_ymm_tp},
    {"VFNMSUB213PS", "ymm,ymm,ymm",  256, "fma",  vfnmsub213ps_ymm_lat, vfnmsub213ps_ymm_tp},

    /* VFNMSUB 132/213 packed PD YMM */
    {"VFNMSUB132PD", "ymm,ymm,ymm",  256, "fma",  vfnmsub132pd_ymm_lat, vfnmsub132pd_ymm_tp},
    {"VFNMSUB213PD", "ymm,ymm,ymm",  256, "fma",  vfnmsub213pd_ymm_lat, vfnmsub213pd_ymm_tp},

    /* VFNMSUB 132/213 scalar SS */
    {"VFNMSUB132SS", "xmm,xmm,xmm",  128, "fma",  vfnmsub132ss_lat,     vfnmsub132ss_tp},
    {"VFNMSUB213SS", "xmm,xmm,xmm",  128, "fma",  vfnmsub213ss_lat,     vfnmsub213ss_tp},

    /* VFNMSUB 132/213 scalar SD */
    {"VFNMSUB132SD", "xmm,xmm,xmm",  128, "fma",  vfnmsub132sd_lat,     vfnmsub132sd_tp},
    {"VFNMSUB213SD", "xmm,xmm,xmm",  128, "fma",  vfnmsub213sd_lat,     vfnmsub213sd_tp},

    /* VFNMSUB231 missing PD and SD */
    {"VFNMSUB231PD", "ymm,ymm,ymm",  256, "fma",  vfnmsub231pd_ymm_lat, vfnmsub231pd_ymm_tp},
    {"VFNMSUB231SD", "xmm,xmm,xmm",  128, "fma",  vfnmsub231sd_lat,     vfnmsub231sd_tp},

    /* VFMADDSUB all three orderings, PS YMM */
    {"VFMADDSUB132PS", "ymm,ymm,ymm", 256, "fma",  vfmaddsub132ps_ymm_lat, vfmaddsub132ps_ymm_tp},
    {"VFMADDSUB213PS", "ymm,ymm,ymm", 256, "fma",  vfmaddsub213ps_ymm_lat, vfmaddsub213ps_ymm_tp},
    {"VFMADDSUB231PS", "ymm,ymm,ymm", 256, "fma",  vfmaddsub231ps_ymm_lat, vfmaddsub231ps_ymm_tp},

    /* VFMADDSUB all three orderings, PD YMM */
    {"VFMADDSUB132PD", "ymm,ymm,ymm", 256, "fma",  vfmaddsub132pd_ymm_lat, vfmaddsub132pd_ymm_tp},
    {"VFMADDSUB213PD", "ymm,ymm,ymm", 256, "fma",  vfmaddsub213pd_ymm_lat, vfmaddsub213pd_ymm_tp},
    {"VFMADDSUB231PD", "ymm,ymm,ymm", 256, "fma",  vfmaddsub231pd_ymm_lat, vfmaddsub231pd_ymm_tp},

    /* VFMSUBADD all three orderings, PS YMM */
    {"VFMSUBADD132PS", "ymm,ymm,ymm", 256, "fma",  vfmsubadd132ps_ymm_lat, vfmsubadd132ps_ymm_tp},
    {"VFMSUBADD213PS", "ymm,ymm,ymm", 256, "fma",  vfmsubadd213ps_ymm_lat, vfmsubadd213ps_ymm_tp},
    {"VFMSUBADD231PS", "ymm,ymm,ymm", 256, "fma",  vfmsubadd231ps_ymm_lat, vfmsubadd231ps_ymm_tp},

    /* VFMSUBADD all three orderings, PD YMM */
    {"VFMSUBADD132PD", "ymm,ymm,ymm", 256, "fma",  vfmsubadd132pd_ymm_lat, vfmsubadd132pd_ymm_tp},
    {"VFMSUBADD213PD", "ymm,ymm,ymm", 256, "fma",  vfmsubadd213pd_ymm_lat, vfmsubadd213pd_ymm_tp},
    {"VFMSUBADD231PD", "ymm,ymm,ymm", 256, "fma",  vfmsubadd231pd_ymm_lat, vfmsubadd231pd_ymm_tp},

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
        sm_zen_print_header("fma_gaps", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring 48 missing FMA3 operand-ordering forms...\n");
        printf("Question: do 132/213/231 forms differ in latency/throughput on Zen 4?\n\n");
    }

    /* Count entries */
    int total_entries = 0;
    for (int count_idx = 0; fma_gap_probe_table[count_idx].mnemonic != NULL; count_idx++)
        total_entries++;

    if (!csv_mode)
        printf("Total instruction forms: %d\n\n", total_entries);

    /* CSV header */
    printf("mnemonic,operands,width_bits,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; fma_gap_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const FmaGapProbeEntry *current_entry = &fma_gap_probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; fma_gap_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const FmaGapProbeEntry *current_entry = &fma_gap_probe_table[entry_idx];

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
