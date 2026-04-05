#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <mmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>
#include <ammintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_legacy_exhaustive.c -- Exhaustive measurement of legacy ISA extension
 * instruction forms on Zen 4: MMX, SSE4a, SSE4.2 strings, CRC32, DPPD,
 * missing SSSE3/SSE4.1, missing FMA variants, and VEX vs legacy encoding.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,extension,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -msse4a -mcrc32 \
 *     -fno-omit-frame-pointer -include x86intrin.h -I. \
 *     common.c probe_legacy_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_legacy_exhaustive
 */

/* ========================================================================
 * SECTION 1: NATIVE MMX (mm0-mm7, __m64)
 * ======================================================================== */

/* --- MMX LATENCY: 4-deep dependent chain on mm registers --- */

#define MMX_LATENCY_BINARY(func_name, intrinsic_call)                         \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m64 accumulator = _mm_set1_pi32(1);                                     \
    __m64 operand_source = _mm_set1_pi32(1);                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(accumulator, operand_source);            \
        accumulator = intrinsic_call(accumulator, operand_source);            \
        accumulator = intrinsic_call(accumulator, operand_source);            \
        accumulator = intrinsic_call(accumulator, operand_source);            \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m64 sink = accumulator; (void)sink;                            \
    _mm_empty();                                                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/* --- MMX THROUGHPUT: 8 independent streams on mm registers --- */

#define MMX_THROUGHPUT_BINARY(func_name, intrinsic_call)                      \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m64 stream_0 = _mm_set1_pi32(0x01);                                     \
    __m64 stream_1 = _mm_set1_pi32(0x02);                                     \
    __m64 stream_2 = _mm_set1_pi32(0x03);                                     \
    __m64 stream_3 = _mm_set1_pi32(0x04);                                     \
    __m64 stream_4 = _mm_set1_pi32(0x05);                                     \
    __m64 stream_5 = _mm_set1_pi32(0x06);                                     \
    __m64 stream_6 = _mm_set1_pi32(0x07);                                     \
    __m64 stream_7 = _mm_set1_pi32(0x08);                                     \
    __m64 operand_source = _mm_set1_pi32(1);                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        stream_0 = intrinsic_call(stream_0, operand_source);                  \
        stream_1 = intrinsic_call(stream_1, operand_source);                  \
        stream_2 = intrinsic_call(stream_2, operand_source);                  \
        stream_3 = intrinsic_call(stream_3, operand_source);                  \
        stream_4 = intrinsic_call(stream_4, operand_source);                  \
        stream_5 = intrinsic_call(stream_5, operand_source);                  \
        stream_6 = intrinsic_call(stream_6, operand_source);                  \
        stream_7 = intrinsic_call(stream_7, operand_source);                  \
        __asm__ volatile("" : "+y"(stream_0), "+y"(stream_1),                 \
                              "+y"(stream_2), "+y"(stream_3));                \
        __asm__ volatile("" : "+y"(stream_4), "+y"(stream_5),                 \
                              "+y"(stream_6), "+y"(stream_7));                \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m64 sink = stream_0; (void)sink;                               \
    _mm_empty();                                                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* MMX latency using inline asm with "+y" constraint for dep chain */
#define MMX_LATENCY_ASM(func_name, asm_str)                                   \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m64 accumulator = _mm_set1_pi32(1);                                     \
    __m64 operand_source = _mm_set1_pi32(1);                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+y"(accumulator) : "y"(operand_source));  \
        __asm__ volatile(asm_str : "+y"(accumulator) : "y"(operand_source));  \
        __asm__ volatile(asm_str : "+y"(accumulator) : "y"(operand_source));  \
        __asm__ volatile(asm_str : "+y"(accumulator) : "y"(operand_source));  \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m64 sink = accumulator; (void)sink;                            \
    _mm_empty();                                                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define MMX_THROUGHPUT_ASM(func_name, asm_str)                                \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m64 stream_0 = _mm_set1_pi32(0x01);                                     \
    __m64 stream_1 = _mm_set1_pi32(0x02);                                     \
    __m64 stream_2 = _mm_set1_pi32(0x03);                                     \
    __m64 stream_3 = _mm_set1_pi32(0x04);                                     \
    __m64 stream_4 = _mm_set1_pi32(0x05);                                     \
    __m64 stream_5 = _mm_set1_pi32(0x06);                                     \
    __m64 stream_6 = _mm_set1_pi32(0x07);                                     \
    __m64 stream_7 = _mm_set1_pi32(0x08);                                     \
    __m64 operand_source = _mm_set1_pi32(1);                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+y"(stream_0) : "y"(operand_source));     \
        __asm__ volatile(asm_str : "+y"(stream_1) : "y"(operand_source));     \
        __asm__ volatile(asm_str : "+y"(stream_2) : "y"(operand_source));     \
        __asm__ volatile(asm_str : "+y"(stream_3) : "y"(operand_source));     \
        __asm__ volatile(asm_str : "+y"(stream_4) : "y"(operand_source));     \
        __asm__ volatile(asm_str : "+y"(stream_5) : "y"(operand_source));     \
        __asm__ volatile(asm_str : "+y"(stream_6) : "y"(operand_source));     \
        __asm__ volatile(asm_str : "+y"(stream_7) : "y"(operand_source));     \
        __asm__ volatile("" : "+y"(stream_0), "+y"(stream_1),                 \
                              "+y"(stream_2), "+y"(stream_3));                \
        __asm__ volatile("" : "+y"(stream_4), "+y"(stream_5),                 \
                              "+y"(stream_6), "+y"(stream_7));                \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m64 sink = stream_0; (void)sink;                               \
    _mm_empty();                                                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* --- MOVD / MOVQ --- */

static double mmx_movd_mm_r32_lat(uint64_t iteration_count)
{
    uint32_t accumulator = 0xDEADBEEF;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __m64 mmx_temp;
        __asm__ volatile("movd %1, %0" : "=y"(mmx_temp) : "r"(accumulator));
        __asm__ volatile("movd %1, %0" : "=r"(accumulator) : "y"(mmx_temp));
        __asm__ volatile("movd %1, %0" : "=y"(mmx_temp) : "r"(accumulator));
        __asm__ volatile("movd %1, %0" : "=r"(accumulator) : "y"(mmx_temp));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = accumulator; (void)sink;
    _mm_empty();
    /* 4 instructions, 2 round-trips: latency = total / 4 */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mmx_movd_mm_r32_tp(uint64_t iteration_count)
{
    uint32_t source_val = 0xDEADBEEF;
    __m64 stream_0, stream_1, stream_2, stream_3;
    __m64 stream_4, stream_5, stream_6, stream_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movd %1, %0" : "=y"(stream_0) : "r"(source_val));
        __asm__ volatile("movd %1, %0" : "=y"(stream_1) : "r"(source_val));
        __asm__ volatile("movd %1, %0" : "=y"(stream_2) : "r"(source_val));
        __asm__ volatile("movd %1, %0" : "=y"(stream_3) : "r"(source_val));
        __asm__ volatile("movd %1, %0" : "=y"(stream_4) : "r"(source_val));
        __asm__ volatile("movd %1, %0" : "=y"(stream_5) : "r"(source_val));
        __asm__ volatile("movd %1, %0" : "=y"(stream_6) : "r"(source_val));
        __asm__ volatile("movd %1, %0" : "=y"(stream_7) : "r"(source_val));
        __asm__ volatile("" : "+y"(stream_0), "+y"(stream_1),
                              "+y"(stream_2), "+y"(stream_3));
        __asm__ volatile("" : "+y"(stream_4), "+y"(stream_5),
                              "+y"(stream_6), "+y"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m64 sink = stream_0; (void)sink;
    _mm_empty();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double mmx_movd_r32_mm_tp(uint64_t iteration_count)
{
    __m64 mmx_source = _mm_set1_pi32(0xCAFE);
    uint32_t stream_0, stream_1, stream_2, stream_3;
    uint32_t stream_4, stream_5, stream_6, stream_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movd %1, %0" : "=r"(stream_0) : "y"(mmx_source));
        __asm__ volatile("movd %1, %0" : "=r"(stream_1) : "y"(mmx_source));
        __asm__ volatile("movd %1, %0" : "=r"(stream_2) : "y"(mmx_source));
        __asm__ volatile("movd %1, %0" : "=r"(stream_3) : "y"(mmx_source));
        __asm__ volatile("movd %1, %0" : "=r"(stream_4) : "y"(mmx_source));
        __asm__ volatile("movd %1, %0" : "=r"(stream_5) : "y"(mmx_source));
        __asm__ volatile("movd %1, %0" : "=r"(stream_6) : "y"(mmx_source));
        __asm__ volatile("movd %1, %0" : "=r"(stream_7) : "y"(mmx_source));
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3));
        __asm__ volatile("" : "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = stream_0; (void)sink;
    _mm_empty();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

MMX_LATENCY_ASM(mmx_movq_mm_mm_lat, "movq %1, %0")
MMX_THROUGHPUT_ASM(mmx_movq_mm_mm_tp, "movq %1, %0")

/* --- Arithmetic --- */
MMX_LATENCY_BINARY(mmx_paddb_lat, _mm_add_pi8)
MMX_THROUGHPUT_BINARY(mmx_paddb_tp, _mm_add_pi8)
MMX_LATENCY_BINARY(mmx_paddw_lat, _mm_add_pi16)
MMX_THROUGHPUT_BINARY(mmx_paddw_tp, _mm_add_pi16)
MMX_LATENCY_BINARY(mmx_paddd_lat, _mm_add_pi32)
MMX_THROUGHPUT_BINARY(mmx_paddd_tp, _mm_add_pi32)

MMX_LATENCY_BINARY(mmx_psubb_lat, _mm_sub_pi8)
MMX_THROUGHPUT_BINARY(mmx_psubb_tp, _mm_sub_pi8)
MMX_LATENCY_BINARY(mmx_psubw_lat, _mm_sub_pi16)
MMX_THROUGHPUT_BINARY(mmx_psubw_tp, _mm_sub_pi16)
MMX_LATENCY_BINARY(mmx_psubd_lat, _mm_sub_pi32)
MMX_THROUGHPUT_BINARY(mmx_psubd_tp, _mm_sub_pi32)

MMX_LATENCY_BINARY(mmx_paddusb_lat, _mm_adds_pu8)
MMX_THROUGHPUT_BINARY(mmx_paddusb_tp, _mm_adds_pu8)
MMX_LATENCY_BINARY(mmx_paddusw_lat, _mm_adds_pu16)
MMX_THROUGHPUT_BINARY(mmx_paddusw_tp, _mm_adds_pu16)
MMX_LATENCY_BINARY(mmx_paddsb_lat, _mm_adds_pi8)
MMX_THROUGHPUT_BINARY(mmx_paddsb_tp, _mm_adds_pi8)
MMX_LATENCY_BINARY(mmx_paddsw_lat, _mm_adds_pi16)
MMX_THROUGHPUT_BINARY(mmx_paddsw_tp, _mm_adds_pi16)

/* --- Multiply --- */
MMX_LATENCY_BINARY(mmx_pmullw_lat, _mm_mullo_pi16)
MMX_THROUGHPUT_BINARY(mmx_pmullw_tp, _mm_mullo_pi16)
MMX_LATENCY_BINARY(mmx_pmulhw_lat, _mm_mulhi_pi16)
MMX_THROUGHPUT_BINARY(mmx_pmulhw_tp, _mm_mulhi_pi16)
MMX_LATENCY_BINARY(mmx_pmaddwd_lat, _mm_madd_pi16)
MMX_THROUGHPUT_BINARY(mmx_pmaddwd_tp, _mm_madd_pi16)

/* --- Logic --- */
MMX_LATENCY_BINARY(mmx_pand_lat, _mm_and_si64)
MMX_THROUGHPUT_BINARY(mmx_pand_tp, _mm_and_si64)
MMX_LATENCY_BINARY(mmx_pandn_lat, _mm_andnot_si64)
MMX_THROUGHPUT_BINARY(mmx_pandn_tp, _mm_andnot_si64)
MMX_LATENCY_BINARY(mmx_por_lat, _mm_or_si64)
MMX_THROUGHPUT_BINARY(mmx_por_tp, _mm_or_si64)
MMX_LATENCY_BINARY(mmx_pxor_lat, _mm_xor_si64)
MMX_THROUGHPUT_BINARY(mmx_pxor_tp, _mm_xor_si64)

/* --- Shifts (immediate) --- */
#define MMX_LATENCY_SHIFT(func_name, intrinsic_call, shift_amount)            \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m64 accumulator = _mm_set1_pi32(0x55AA55AA);                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(accumulator, shift_amount);              \
        accumulator = intrinsic_call(accumulator, shift_amount);              \
        accumulator = intrinsic_call(accumulator, shift_amount);              \
        accumulator = intrinsic_call(accumulator, shift_amount);              \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m64 sink = accumulator; (void)sink;                            \
    _mm_empty();                                                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define MMX_THROUGHPUT_SHIFT(func_name, intrinsic_call, shift_amount)          \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m64 stream_0 = _mm_set1_pi32(0x01);                                     \
    __m64 stream_1 = _mm_set1_pi32(0x02);                                     \
    __m64 stream_2 = _mm_set1_pi32(0x03);                                     \
    __m64 stream_3 = _mm_set1_pi32(0x04);                                     \
    __m64 stream_4 = _mm_set1_pi32(0x05);                                     \
    __m64 stream_5 = _mm_set1_pi32(0x06);                                     \
    __m64 stream_6 = _mm_set1_pi32(0x07);                                     \
    __m64 stream_7 = _mm_set1_pi32(0x08);                                     \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        stream_0 = intrinsic_call(stream_0, shift_amount);                    \
        stream_1 = intrinsic_call(stream_1, shift_amount);                    \
        stream_2 = intrinsic_call(stream_2, shift_amount);                    \
        stream_3 = intrinsic_call(stream_3, shift_amount);                    \
        stream_4 = intrinsic_call(stream_4, shift_amount);                    \
        stream_5 = intrinsic_call(stream_5, shift_amount);                    \
        stream_6 = intrinsic_call(stream_6, shift_amount);                    \
        stream_7 = intrinsic_call(stream_7, shift_amount);                    \
        __asm__ volatile("" : "+y"(stream_0), "+y"(stream_1),                 \
                              "+y"(stream_2), "+y"(stream_3));                \
        __asm__ volatile("" : "+y"(stream_4), "+y"(stream_5),                 \
                              "+y"(stream_6), "+y"(stream_7));                \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m64 sink = stream_0; (void)sink;                               \
    _mm_empty();                                                              \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

MMX_LATENCY_SHIFT(mmx_psllw_lat, _mm_slli_pi16, 3)
MMX_THROUGHPUT_SHIFT(mmx_psllw_tp, _mm_slli_pi16, 3)
MMX_LATENCY_SHIFT(mmx_pslld_lat, _mm_slli_pi32, 7)
MMX_THROUGHPUT_SHIFT(mmx_pslld_tp, _mm_slli_pi32, 7)
MMX_LATENCY_SHIFT(mmx_psllq_lat, _mm_slli_si64, 11)
MMX_THROUGHPUT_SHIFT(mmx_psllq_tp, _mm_slli_si64, 11)

MMX_LATENCY_SHIFT(mmx_psrlw_lat, _mm_srli_pi16, 3)
MMX_THROUGHPUT_SHIFT(mmx_psrlw_tp, _mm_srli_pi16, 3)
MMX_LATENCY_SHIFT(mmx_psrld_lat, _mm_srli_pi32, 7)
MMX_THROUGHPUT_SHIFT(mmx_psrld_tp, _mm_srli_pi32, 7)
MMX_LATENCY_SHIFT(mmx_psrlq_lat, _mm_srli_si64, 11)
MMX_THROUGHPUT_SHIFT(mmx_psrlq_tp, _mm_srli_si64, 11)

MMX_LATENCY_SHIFT(mmx_psraw_lat, _mm_srai_pi16, 3)
MMX_THROUGHPUT_SHIFT(mmx_psraw_tp, _mm_srai_pi16, 3)
MMX_LATENCY_SHIFT(mmx_psrad_lat, _mm_srai_pi32, 7)
MMX_THROUGHPUT_SHIFT(mmx_psrad_tp, _mm_srai_pi32, 7)

/* --- Compare --- */
MMX_LATENCY_BINARY(mmx_pcmpeqb_lat, _mm_cmpeq_pi8)
MMX_THROUGHPUT_BINARY(mmx_pcmpeqb_tp, _mm_cmpeq_pi8)
MMX_LATENCY_BINARY(mmx_pcmpeqw_lat, _mm_cmpeq_pi16)
MMX_THROUGHPUT_BINARY(mmx_pcmpeqw_tp, _mm_cmpeq_pi16)
MMX_LATENCY_BINARY(mmx_pcmpeqd_lat, _mm_cmpeq_pi32)
MMX_THROUGHPUT_BINARY(mmx_pcmpeqd_tp, _mm_cmpeq_pi32)

MMX_LATENCY_BINARY(mmx_pcmpgtb_lat, _mm_cmpgt_pi8)
MMX_THROUGHPUT_BINARY(mmx_pcmpgtb_tp, _mm_cmpgt_pi8)
MMX_LATENCY_BINARY(mmx_pcmpgtw_lat, _mm_cmpgt_pi16)
MMX_THROUGHPUT_BINARY(mmx_pcmpgtw_tp, _mm_cmpgt_pi16)
MMX_LATENCY_BINARY(mmx_pcmpgtd_lat, _mm_cmpgt_pi32)
MMX_THROUGHPUT_BINARY(mmx_pcmpgtd_tp, _mm_cmpgt_pi32)

/* --- Unpack / Pack --- */
MMX_LATENCY_BINARY(mmx_punpckhbw_lat, _mm_unpackhi_pi8)
MMX_THROUGHPUT_BINARY(mmx_punpckhbw_tp, _mm_unpackhi_pi8)
MMX_LATENCY_BINARY(mmx_punpckhwd_lat, _mm_unpackhi_pi16)
MMX_THROUGHPUT_BINARY(mmx_punpckhwd_tp, _mm_unpackhi_pi16)
MMX_LATENCY_BINARY(mmx_punpckhdq_lat, _mm_unpackhi_pi32)
MMX_THROUGHPUT_BINARY(mmx_punpckhdq_tp, _mm_unpackhi_pi32)

MMX_LATENCY_BINARY(mmx_punpcklbw_lat, _mm_unpacklo_pi8)
MMX_THROUGHPUT_BINARY(mmx_punpcklbw_tp, _mm_unpacklo_pi8)
MMX_LATENCY_BINARY(mmx_punpcklwd_lat, _mm_unpacklo_pi16)
MMX_THROUGHPUT_BINARY(mmx_punpcklwd_tp, _mm_unpacklo_pi16)
MMX_LATENCY_BINARY(mmx_punpckldq_lat, _mm_unpacklo_pi32)
MMX_THROUGHPUT_BINARY(mmx_punpckldq_tp, _mm_unpacklo_pi32)

MMX_LATENCY_BINARY(mmx_packuswb_lat, _mm_packs_pu16)
MMX_THROUGHPUT_BINARY(mmx_packuswb_tp, _mm_packs_pu16)
MMX_LATENCY_BINARY(mmx_packsswb_lat, _mm_packs_pi16)
MMX_THROUGHPUT_BINARY(mmx_packsswb_tp, _mm_packs_pi16)
MMX_LATENCY_BINARY(mmx_packssdw_lat, _mm_packs_pi32)
MMX_THROUGHPUT_BINARY(mmx_packssdw_tp, _mm_packs_pi32)

/* --- EMMS throughput --- */
static double mmx_emms_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
        __asm__ volatile("emms" ::: "mm0","mm1","mm2","mm3",
                         "mm4","mm5","mm6","mm7");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}


/* ========================================================================
 * SECTION 2: SSE4a (AMD-specific)
 * ======================================================================== */

/* EXTRQ xmm, imm8, imm8 -- extract 8-bit bitfield from position 0 */
static double sse4a_extrq_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set_epi64x(0, 0x0123456789ABCDEFLL);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("extrq $8, $0, %0" : "+x"(accumulator));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(accumulator));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(accumulator));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse4a_extrq_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_set_epi64x(0, 0x0123456789ABCDEFLL);
    __m128i stream_1 = _mm_set_epi64x(0, 0x1122334455667788LL);
    __m128i stream_2 = _mm_set_epi64x(0, 0x2233445566778899LL);
    __m128i stream_3 = _mm_set_epi64x(0, 0x33445566778899AALL);
    __m128i stream_4 = _mm_set_epi64x(0, 0x445566778899AABBLL);
    __m128i stream_5 = _mm_set_epi64x(0, 0x5566778899AABBCCLL);
    __m128i stream_6 = _mm_set_epi64x(0, 0x6677889900AABBCCLL);
    __m128i stream_7 = _mm_set_epi64x(0, 0x778899AABBCCDDAALL);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_0));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_1));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_2));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_3));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_4));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_5));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_6));
        __asm__ volatile("extrq $8, $0, %0" : "+x"(stream_7));
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),
                              "+x"(stream_6), "+x"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* INSERTQ xmm, xmm, imm8, imm8 -- insert 8-bit bitfield at position 0 */
static double sse4a_insertq_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set_epi64x(0, 0x0123456789ABCDEFLL);
    __m128i operand_source = _mm_set_epi64x(0, 0xFF);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse4a_insertq_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_set_epi64x(0, 0x0123456789ABCDEFLL);
    __m128i stream_1 = _mm_set_epi64x(0, 0x1122334455667788LL);
    __m128i stream_2 = _mm_set_epi64x(0, 0x2233445566778899LL);
    __m128i stream_3 = _mm_set_epi64x(0, 0x33445566778899AALL);
    __m128i stream_4 = _mm_set_epi64x(0, 0x445566778899AABBLL);
    __m128i stream_5 = _mm_set_epi64x(0, 0x5566778899AABBCCLL);
    __m128i stream_6 = _mm_set_epi64x(0, 0x6677889900AABBCCLL);
    __m128i stream_7 = _mm_set_epi64x(0, 0x778899AABBCCDDAALL);
    __m128i operand_source = _mm_set_epi64x(0, 0xFF);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_0) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_1) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_2) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_3) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_4) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_5) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_6) : "x"(operand_source));
        __asm__ volatile("insertq $8, $0, %1, %0"
                         : "+x"(stream_7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
        __asm__ volatile("" : "+x"(stream_4), "+x"(stream_5),
                              "+x"(stream_6), "+x"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOVNTSS [m32], xmm -- non-temporal scalar float store (throughput only) */
static double sse4a_movntss_tp(uint64_t iteration_count)
{
    float __attribute__((aligned(64))) nt_store_buffer[16];
    __m128 store_value = _mm_set1_ps(1.0f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[0]), "x"(store_value) : "memory");
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[1]), "x"(store_value) : "memory");
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[2]), "x"(store_value) : "memory");
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[3]), "x"(store_value) : "memory");
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[4]), "x"(store_value) : "memory");
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[5]), "x"(store_value) : "memory");
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[6]), "x"(store_value) : "memory");
        __asm__ volatile("movntss %1, (%0)" : : "r"(&nt_store_buffer[7]), "x"(store_value) : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOVNTSD [m64], xmm -- non-temporal scalar double store (throughput only) */
static double sse4a_movntsd_tp(uint64_t iteration_count)
{
    double __attribute__((aligned(64))) nt_store_buffer[8];
    __m128d store_value = _mm_set1_pd(1.0);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[0]), "x"(store_value) : "memory");
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[1]), "x"(store_value) : "memory");
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[2]), "x"(store_value) : "memory");
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[3]), "x"(store_value) : "memory");
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[4]), "x"(store_value) : "memory");
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[5]), "x"(store_value) : "memory");
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[6]), "x"(store_value) : "memory");
        __asm__ volatile("movntsd %1, (%0)" : : "r"(&nt_store_buffer[7]), "x"(store_value) : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}


/* ========================================================================
 * SECTION 3: SSE4.2 String Instructions
 * ======================================================================== */

/*
 * PCMPxSTRx: These instructions return either an index (I) or a mask (M),
 * using explicit (E) or implicit (I) length. Test two imm8 modes each:
 *   - 0x0C = equal-each, unsigned bytes
 *   - 0x0D = equal-each, unsigned words
 */

/* --- PCMPESTRI --- */
static double sse42_pcmpestri_0c_lat(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    int result_index = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x0C);
        /* Feed result back: shift needle by result to create dependency */
        needle = _mm_srli_si128(needle, 0);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = result_index; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse42_pcmpestri_0c_tp(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    int result_0, result_1, result_2, result_3;
    int result_4, result_5, result_6, result_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_0 = _mm_cmpestri(haystack, 12, needle, 3, 0x0C);
        result_1 = _mm_cmpestri(haystack, 11, needle, 3, 0x0C);
        result_2 = _mm_cmpestri(haystack, 10, needle, 3, 0x0C);
        result_3 = _mm_cmpestri(haystack, 9, needle, 3, 0x0C);
        result_4 = _mm_cmpestri(haystack, 8, needle, 3, 0x0C);
        result_5 = _mm_cmpestri(haystack, 7, needle, 3, 0x0C);
        result_6 = _mm_cmpestri(haystack, 6, needle, 3, 0x0C);
        result_7 = _mm_cmpestri(haystack, 5, needle, 3, 0x0C);
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = result_0 + result_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PCMPESTRI with equal-ordered (substring match) mode 0x0C -> 0x0C already done; 0x4C = equal-ordered */
static double sse42_pcmpestri_4c_lat(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('l','l','o',0,0,0,0,0,0,0,0,0,0,0,0,0);
    int result_index = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpestri(haystack, 12, needle, 3, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = result_index; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* --- PCMPESTRM --- */
static double sse42_pcmpestrm_0c_lat(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    __m128i result_mask;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_mask = _mm_cmpestrm(haystack, 12, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpestrm(haystack, 12, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpestrm(haystack, 12, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpestrm(haystack, 12, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = result_mask; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse42_pcmpestrm_0c_tp(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    __m128i r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        r0 = _mm_cmpestrm(haystack, 12, needle, 3, 0x0C);
        r1 = _mm_cmpestrm(haystack, 11, needle, 3, 0x0C);
        r2 = _mm_cmpestrm(haystack, 10, needle, 3, 0x0C);
        r3 = _mm_cmpestrm(haystack, 9, needle, 3, 0x0C);
        r4 = _mm_cmpestrm(haystack, 8, needle, 3, 0x0C);
        r5 = _mm_cmpestrm(haystack, 7, needle, 3, 0x0C);
        r6 = _mm_cmpestrm(haystack, 6, needle, 3, 0x0C);
        r7 = _mm_cmpestrm(haystack, 5, needle, 3, 0x0C);
        __asm__ volatile("" : "+x"(r0), "+x"(r1), "+x"(r2), "+x"(r3));
        __asm__ volatile("" : "+x"(r4), "+x"(r5), "+x"(r6), "+x"(r7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = r0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* --- PCMPISTRI --- */
static double sse42_pcmpistri_0c_lat(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    int result_index = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_index = _mm_cmpistri(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpistri(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpistri(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpistri(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = result_index; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse42_pcmpistri_0c_tp(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    int r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        r0 = _mm_cmpistri(haystack, needle, 0x0C);
        r1 = _mm_cmpistri(haystack, needle, 0x0C);
        r2 = _mm_cmpistri(haystack, needle, 0x0C);
        r3 = _mm_cmpistri(haystack, needle, 0x0C);
        r4 = _mm_cmpistri(haystack, needle, 0x0C);
        r5 = _mm_cmpistri(haystack, needle, 0x0C);
        r6 = _mm_cmpistri(haystack, needle, 0x0C);
        r7 = _mm_cmpistri(haystack, needle, 0x0C);
        __asm__ volatile("" : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3));
        __asm__ volatile("" : "+r"(r4), "+r"(r5), "+r"(r6), "+r"(r7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = r0 + r4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PCMPISTRI equal-ordered (substring) mode */
static double sse42_pcmpistri_4c_lat(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('l','l','o',0,0,0,0,0,0,0,0,0,0,0,0,0);
    int result_index = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_index = _mm_cmpistri(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpistri(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpistri(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
        result_index = _mm_cmpistri(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "r"(result_index));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = result_index; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* --- PCMPISTRM --- */
static double sse42_pcmpistrm_0c_lat(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    __m128i result_mask;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_mask = _mm_cmpistrm(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpistrm(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpistrm(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpistrm(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = result_mask; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse42_pcmpistrm_0c_tp(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('o',' ','W',0,0,0,0,0,0,0,0,0,0,0,0,0);
    __m128i r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        r0 = _mm_cmpistrm(haystack, needle, 0x0C);
        r1 = _mm_cmpistrm(haystack, needle, 0x0C);
        r2 = _mm_cmpistrm(haystack, needle, 0x0C);
        r3 = _mm_cmpistrm(haystack, needle, 0x0C);
        r4 = _mm_cmpistrm(haystack, needle, 0x0C);
        r5 = _mm_cmpistrm(haystack, needle, 0x0C);
        r6 = _mm_cmpistrm(haystack, needle, 0x0C);
        r7 = _mm_cmpistrm(haystack, needle, 0x0C);
        __asm__ volatile("" : "+x"(r0), "+x"(r1), "+x"(r2), "+x"(r3));
        __asm__ volatile("" : "+x"(r4), "+x"(r5), "+x"(r6), "+x"(r7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = r0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PCMPISTRM equal-ordered mode */
static double sse42_pcmpistrm_4c_lat(uint64_t iteration_count)
{
    __m128i haystack = _mm_setr_epi8('H','e','l','l','o',' ','W','o',
                                     'r','l','d','!',0,0,0,0);
    __m128i needle = _mm_setr_epi8('l','l','o',0,0,0,0,0,0,0,0,0,0,0,0,0);
    __m128i result_mask;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_mask = _mm_cmpistrm(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpistrm(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpistrm(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
        result_mask = _mm_cmpistrm(haystack, needle, 0x4C);
        __asm__ volatile("" : "+x"(needle) : "x"(result_mask));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = result_mask; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}


/* ========================================================================
 * SECTION 4: CRC32
 * ======================================================================== */

/* CRC32 r32, r8 */
static double crc32_u8_lat(uint64_t iteration_count)
{
    uint32_t accumulator = 0xDEADBEEF;
    uint8_t data_byte = 0xAB;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_crc32_u8(accumulator, data_byte);
        accumulator = _mm_crc32_u8(accumulator, data_byte);
        accumulator = _mm_crc32_u8(accumulator, data_byte);
        accumulator = _mm_crc32_u8(accumulator, data_byte);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double crc32_u8_tp(uint64_t iteration_count)
{
    uint32_t s0 = 0x01, s1 = 0x02, s2 = 0x03, s3 = 0x04;
    uint32_t s4 = 0x05, s5 = 0x06, s6 = 0x07, s7 = 0x08;
    uint8_t data_byte = 0xAB;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        s0 = _mm_crc32_u8(s0, data_byte);
        s1 = _mm_crc32_u8(s1, data_byte);
        s2 = _mm_crc32_u8(s2, data_byte);
        s3 = _mm_crc32_u8(s3, data_byte);
        s4 = _mm_crc32_u8(s4, data_byte);
        s5 = _mm_crc32_u8(s5, data_byte);
        s6 = _mm_crc32_u8(s6, data_byte);
        s7 = _mm_crc32_u8(s7, data_byte);
        __asm__ volatile("" : "+r"(s0), "+r"(s1), "+r"(s2), "+r"(s3));
        __asm__ volatile("" : "+r"(s4), "+r"(s5), "+r"(s6), "+r"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = s0 ^ s4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CRC32 r32, r16 */
static double crc32_u16_lat(uint64_t iteration_count)
{
    uint32_t accumulator = 0xDEADBEEF;
    uint16_t data_word = 0xABCD;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_crc32_u16(accumulator, data_word);
        accumulator = _mm_crc32_u16(accumulator, data_word);
        accumulator = _mm_crc32_u16(accumulator, data_word);
        accumulator = _mm_crc32_u16(accumulator, data_word);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double crc32_u16_tp(uint64_t iteration_count)
{
    uint32_t s0 = 0x01, s1 = 0x02, s2 = 0x03, s3 = 0x04;
    uint32_t s4 = 0x05, s5 = 0x06, s6 = 0x07, s7 = 0x08;
    uint16_t data_word = 0xABCD;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        s0 = _mm_crc32_u16(s0, data_word);
        s1 = _mm_crc32_u16(s1, data_word);
        s2 = _mm_crc32_u16(s2, data_word);
        s3 = _mm_crc32_u16(s3, data_word);
        s4 = _mm_crc32_u16(s4, data_word);
        s5 = _mm_crc32_u16(s5, data_word);
        s6 = _mm_crc32_u16(s6, data_word);
        s7 = _mm_crc32_u16(s7, data_word);
        __asm__ volatile("" : "+r"(s0), "+r"(s1), "+r"(s2), "+r"(s3));
        __asm__ volatile("" : "+r"(s4), "+r"(s5), "+r"(s6), "+r"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = s0 ^ s4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CRC32 r32, r32 */
static double crc32_u32_lat(uint64_t iteration_count)
{
    uint32_t accumulator = 0xDEADBEEF;
    uint32_t data_dword = 0xCAFEBABE;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_crc32_u32(accumulator, data_dword);
        accumulator = _mm_crc32_u32(accumulator, data_dword);
        accumulator = _mm_crc32_u32(accumulator, data_dword);
        accumulator = _mm_crc32_u32(accumulator, data_dword);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double crc32_u32_tp(uint64_t iteration_count)
{
    uint32_t s0 = 0x01, s1 = 0x02, s2 = 0x03, s3 = 0x04;
    uint32_t s4 = 0x05, s5 = 0x06, s6 = 0x07, s7 = 0x08;
    uint32_t data_dword = 0xCAFEBABE;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        s0 = _mm_crc32_u32(s0, data_dword);
        s1 = _mm_crc32_u32(s1, data_dword);
        s2 = _mm_crc32_u32(s2, data_dword);
        s3 = _mm_crc32_u32(s3, data_dword);
        s4 = _mm_crc32_u32(s4, data_dword);
        s5 = _mm_crc32_u32(s5, data_dword);
        s6 = _mm_crc32_u32(s6, data_dword);
        s7 = _mm_crc32_u32(s7, data_dword);
        __asm__ volatile("" : "+r"(s0), "+r"(s1), "+r"(s2), "+r"(s3));
        __asm__ volatile("" : "+r"(s4), "+r"(s5), "+r"(s6), "+r"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = s0 ^ s4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CRC32 r64, r64 */
static double crc32_u64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t data_qword = 0x0123456789ABCDEFULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_crc32_u64(accumulator, data_qword);
        accumulator = _mm_crc32_u64(accumulator, data_qword);
        accumulator = _mm_crc32_u64(accumulator, data_qword);
        accumulator = _mm_crc32_u64(accumulator, data_qword);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double crc32_u64_tp(uint64_t iteration_count)
{
    uint64_t s0 = 0x01, s1 = 0x02, s2 = 0x03, s3 = 0x04;
    uint64_t s4 = 0x05, s5 = 0x06, s6 = 0x07, s7 = 0x08;
    uint64_t data_qword = 0x0123456789ABCDEFULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        s0 = _mm_crc32_u64(s0, data_qword);
        s1 = _mm_crc32_u64(s1, data_qword);
        s2 = _mm_crc32_u64(s2, data_qword);
        s3 = _mm_crc32_u64(s3, data_qword);
        s4 = _mm_crc32_u64(s4, data_qword);
        s5 = _mm_crc32_u64(s5, data_qword);
        s6 = _mm_crc32_u64(s6, data_qword);
        s7 = _mm_crc32_u64(s7, data_qword);
        __asm__ volatile("" : "+r"(s0), "+r"(s1), "+r"(s2), "+r"(s3));
        __asm__ volatile("" : "+r"(s4), "+r"(s5), "+r"(s6), "+r"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = s0 ^ s4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}


/* ========================================================================
 * SECTION 5: DPPD (SSE4.1 FP64 dot product)
 * ======================================================================== */

/* DPPD xmm, xmm, 0x31 -- dot product of 2 FP64 elements -> lane 0 */
static double dppd_0x31_lat(uint64_t iteration_count)
{
    __m128d accumulator = _mm_set_pd(1.0, 2.0);
    __m128d operand_source = _mm_set_pd(0.5, 0.5);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x31);
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x31);
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x31);
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x31);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128d sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double dppd_0x31_tp(uint64_t iteration_count)
{
    __m128d s0 = _mm_set_pd(1.0, 2.0);
    __m128d s1 = _mm_set_pd(1.1, 2.1);
    __m128d s2 = _mm_set_pd(1.2, 2.2);
    __m128d s3 = _mm_set_pd(1.3, 2.3);
    __m128d s4 = _mm_set_pd(1.4, 2.4);
    __m128d s5 = _mm_set_pd(1.5, 2.5);
    __m128d s6 = _mm_set_pd(1.6, 2.6);
    __m128d s7 = _mm_set_pd(1.7, 2.7);
    __m128d operand_source = _mm_set_pd(0.5, 0.5);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        s0 = _mm_dp_pd(s0, operand_source, 0x31);
        s1 = _mm_dp_pd(s1, operand_source, 0x31);
        s2 = _mm_dp_pd(s2, operand_source, 0x31);
        s3 = _mm_dp_pd(s3, operand_source, 0x31);
        s4 = _mm_dp_pd(s4, operand_source, 0x31);
        s5 = _mm_dp_pd(s5, operand_source, 0x31);
        s6 = _mm_dp_pd(s6, operand_source, 0x31);
        s7 = _mm_dp_pd(s7, operand_source, 0x31);
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128d sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* DPPD with 0x33 -- both elements multiplied, result in both lanes */
static double dppd_0x33_lat(uint64_t iteration_count)
{
    __m128d accumulator = _mm_set_pd(1.0, 2.0);
    __m128d operand_source = _mm_set_pd(0.5, 0.5);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x33);
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x33);
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x33);
        accumulator = _mm_dp_pd(accumulator, operand_source, 0x33);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128d sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double dppd_0x33_tp(uint64_t iteration_count)
{
    __m128d s0 = _mm_set_pd(1.0, 2.0);
    __m128d s1 = _mm_set_pd(1.1, 2.1);
    __m128d s2 = _mm_set_pd(1.2, 2.2);
    __m128d s3 = _mm_set_pd(1.3, 2.3);
    __m128d s4 = _mm_set_pd(1.4, 2.4);
    __m128d s5 = _mm_set_pd(1.5, 2.5);
    __m128d s6 = _mm_set_pd(1.6, 2.6);
    __m128d s7 = _mm_set_pd(1.7, 2.7);
    __m128d operand_source = _mm_set_pd(0.5, 0.5);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        s0 = _mm_dp_pd(s0, operand_source, 0x33);
        s1 = _mm_dp_pd(s1, operand_source, 0x33);
        s2 = _mm_dp_pd(s2, operand_source, 0x33);
        s3 = _mm_dp_pd(s3, operand_source, 0x33);
        s4 = _mm_dp_pd(s4, operand_source, 0x33);
        s5 = _mm_dp_pd(s5, operand_source, 0x33);
        s6 = _mm_dp_pd(s6, operand_source, 0x33);
        s7 = _mm_dp_pd(s7, operand_source, 0x33);
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128d sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}


/* ========================================================================
 * SECTION 6: Missing SSSE3 and SSE4.1
 * ======================================================================== */

/* XMM latency/throughput macros */
#define XMM_LATENCY_BINARY(func_name, intrinsic_call)                         \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128i accumulator = _mm_set1_epi32(0x12345678);                         \
    __m128i operand_source = _mm_set1_epi32(0x01020304);                      \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(accumulator, operand_source);            \
        accumulator = intrinsic_call(accumulator, operand_source);            \
        accumulator = intrinsic_call(accumulator, operand_source);            \
        accumulator = intrinsic_call(accumulator, operand_source);            \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128i sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define XMM_THROUGHPUT_BINARY(func_name, intrinsic_call)                      \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128i s0 = _mm_set1_epi32(0x01);                                        \
    __m128i s1 = _mm_set1_epi32(0x02);                                        \
    __m128i s2 = _mm_set1_epi32(0x03);                                        \
    __m128i s3 = _mm_set1_epi32(0x04);                                        \
    __m128i s4 = _mm_set1_epi32(0x05);                                        \
    __m128i s5 = _mm_set1_epi32(0x06);                                        \
    __m128i s6 = _mm_set1_epi32(0x07);                                        \
    __m128i s7 = _mm_set1_epi32(0x08);                                        \
    __m128i operand_source = _mm_set1_epi32(0x01020304);                      \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        s0 = intrinsic_call(s0, operand_source);                              \
        s1 = intrinsic_call(s1, operand_source);                              \
        s2 = intrinsic_call(s2, operand_source);                              \
        s3 = intrinsic_call(s3, operand_source);                              \
        s4 = intrinsic_call(s4, operand_source);                              \
        s5 = intrinsic_call(s5, operand_source);                              \
        s6 = intrinsic_call(s6, operand_source);                              \
        s7 = intrinsic_call(s7, operand_source);                              \
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));       \
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));       \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128i sink = s0; (void)sink;                                   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* Unary XMM (for abs etc.) */
#define XMM_LATENCY_UNARY(func_name, intrinsic_call)                          \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128i accumulator = _mm_set1_epi32(0x12345678);                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(accumulator);                            \
        accumulator = intrinsic_call(accumulator);                            \
        accumulator = intrinsic_call(accumulator);                            \
        accumulator = intrinsic_call(accumulator);                            \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128i sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define XMM_THROUGHPUT_UNARY(func_name, intrinsic_call)                       \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128i s0 = _mm_set1_epi32(0x01);                                        \
    __m128i s1 = _mm_set1_epi32(0x02);                                        \
    __m128i s2 = _mm_set1_epi32(0x03);                                        \
    __m128i s3 = _mm_set1_epi32(0x04);                                        \
    __m128i s4 = _mm_set1_epi32(0x05);                                        \
    __m128i s5 = _mm_set1_epi32(0x06);                                        \
    __m128i s6 = _mm_set1_epi32(0x07);                                        \
    __m128i s7 = _mm_set1_epi32(0x08);                                        \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        s0 = intrinsic_call(s0);                                              \
        s1 = intrinsic_call(s1);                                              \
        s2 = intrinsic_call(s2);                                              \
        s3 = intrinsic_call(s3);                                              \
        s4 = intrinsic_call(s4);                                              \
        s5 = intrinsic_call(s5);                                              \
        s6 = intrinsic_call(s6);                                              \
        s7 = intrinsic_call(s7);                                              \
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));       \
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));       \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128i sink = s0; (void)sink;                                   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* SSSE3: PSIGNB/W/D */
XMM_LATENCY_BINARY(ssse3_psignb_lat, _mm_sign_epi8)
XMM_THROUGHPUT_BINARY(ssse3_psignb_tp, _mm_sign_epi8)
XMM_LATENCY_BINARY(ssse3_psignw_lat, _mm_sign_epi16)
XMM_THROUGHPUT_BINARY(ssse3_psignw_tp, _mm_sign_epi16)
XMM_LATENCY_BINARY(ssse3_psignd_lat, _mm_sign_epi32)
XMM_THROUGHPUT_BINARY(ssse3_psignd_tp, _mm_sign_epi32)

/* SSSE3: PABSB/W/D */
XMM_LATENCY_UNARY(ssse3_pabsb_lat, _mm_abs_epi8)
XMM_THROUGHPUT_UNARY(ssse3_pabsb_tp, _mm_abs_epi8)
XMM_LATENCY_UNARY(ssse3_pabsw_lat, _mm_abs_epi16)
XMM_THROUGHPUT_UNARY(ssse3_pabsw_tp, _mm_abs_epi16)
XMM_LATENCY_UNARY(ssse3_pabsd_lat, _mm_abs_epi32)
XMM_THROUGHPUT_UNARY(ssse3_pabsd_tp, _mm_abs_epi32)

/* SSE4.1: PHMINPOSUW */
static double sse41_phminposuw_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_setr_epi16(100, 200, 50, 300, 150, 250, 75, 125);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_minpos_epu16(accumulator);
        accumulator = _mm_minpos_epu16(accumulator);
        accumulator = _mm_minpos_epu16(accumulator);
        accumulator = _mm_minpos_epu16(accumulator);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sse41_phminposuw_tp(uint64_t iteration_count)
{
    __m128i s0 = _mm_setr_epi16(100, 200, 50, 300, 150, 250, 75, 125);
    __m128i s1 = _mm_setr_epi16(110, 210, 55, 310, 155, 255, 80, 130);
    __m128i s2 = _mm_setr_epi16(120, 220, 60, 320, 160, 260, 85, 135);
    __m128i s3 = _mm_setr_epi16(130, 230, 65, 330, 165, 265, 90, 140);
    __m128i s4 = _mm_setr_epi16(140, 240, 70, 340, 170, 270, 95, 145);
    __m128i s5 = _mm_setr_epi16(150, 250, 75, 350, 175, 275, 100, 150);
    __m128i s6 = _mm_setr_epi16(160, 260, 80, 360, 180, 280, 105, 155);
    __m128i s7 = _mm_setr_epi16(170, 270, 85, 370, 185, 285, 110, 160);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        s0 = _mm_minpos_epu16(s0);
        s1 = _mm_minpos_epu16(s1);
        s2 = _mm_minpos_epu16(s2);
        s3 = _mm_minpos_epu16(s3);
        s4 = _mm_minpos_epu16(s4);
        s5 = _mm_minpos_epu16(s5);
        s6 = _mm_minpos_epu16(s6);
        s7 = _mm_minpos_epu16(s7);
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}


/* ========================================================================
 * SECTION 7: Missing FMA variants (YMM)
 * ======================================================================== */

/* YMM FP macros */
#define YMM_FMA_LATENCY_PS(func_name, intrinsic_call)                         \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m256 accumulator = _mm256_set1_ps(1.0f);                                \
    __m256 multiplicand = _mm256_set1_ps(1.0000001f);                         \
    __m256 addend = _mm256_set1_ps(0.0000001f);                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m256 sink = accumulator; (void)sink;                           \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define YMM_FMA_THROUGHPUT_PS(func_name, intrinsic_call)                      \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m256 s0 = _mm256_set1_ps(1.0f);                                         \
    __m256 s1 = _mm256_set1_ps(1.1f);                                         \
    __m256 s2 = _mm256_set1_ps(1.2f);                                         \
    __m256 s3 = _mm256_set1_ps(1.3f);                                         \
    __m256 s4 = _mm256_set1_ps(1.4f);                                         \
    __m256 s5 = _mm256_set1_ps(1.5f);                                         \
    __m256 s6 = _mm256_set1_ps(1.6f);                                         \
    __m256 s7 = _mm256_set1_ps(1.7f);                                         \
    __m256 multiplicand = _mm256_set1_ps(1.0000001f);                         \
    __m256 addend = _mm256_set1_ps(0.0000001f);                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        s0 = intrinsic_call(multiplicand, addend, s0);                        \
        s1 = intrinsic_call(multiplicand, addend, s1);                        \
        s2 = intrinsic_call(multiplicand, addend, s2);                        \
        s3 = intrinsic_call(multiplicand, addend, s3);                        \
        s4 = intrinsic_call(multiplicand, addend, s4);                        \
        s5 = intrinsic_call(multiplicand, addend, s5);                        \
        s6 = intrinsic_call(multiplicand, addend, s6);                        \
        s7 = intrinsic_call(multiplicand, addend, s7);                        \
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));       \
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));       \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m256 sink = s0; (void)sink;                                    \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

#define YMM_FMA_LATENCY_PD(func_name, intrinsic_call)                         \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m256d accumulator = _mm256_set1_pd(1.0);                                \
    __m256d multiplicand = _mm256_set1_pd(1.0000001);                         \
    __m256d addend = _mm256_set1_pd(0.0000001);                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m256d sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define YMM_FMA_THROUGHPUT_PD(func_name, intrinsic_call)                      \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m256d s0 = _mm256_set1_pd(1.0);                                         \
    __m256d s1 = _mm256_set1_pd(1.1);                                         \
    __m256d s2 = _mm256_set1_pd(1.2);                                         \
    __m256d s3 = _mm256_set1_pd(1.3);                                         \
    __m256d s4 = _mm256_set1_pd(1.4);                                         \
    __m256d s5 = _mm256_set1_pd(1.5);                                         \
    __m256d s6 = _mm256_set1_pd(1.6);                                         \
    __m256d s7 = _mm256_set1_pd(1.7);                                         \
    __m256d multiplicand = _mm256_set1_pd(1.0000001);                         \
    __m256d addend = _mm256_set1_pd(0.0000001);                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        s0 = intrinsic_call(multiplicand, addend, s0);                        \
        s1 = intrinsic_call(multiplicand, addend, s1);                        \
        s2 = intrinsic_call(multiplicand, addend, s2);                        \
        s3 = intrinsic_call(multiplicand, addend, s3);                        \
        s4 = intrinsic_call(multiplicand, addend, s4);                        \
        s5 = intrinsic_call(multiplicand, addend, s5);                        \
        s6 = intrinsic_call(multiplicand, addend, s6);                        \
        s7 = intrinsic_call(multiplicand, addend, s7);                        \
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));       \
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));       \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m256d sink = s0; (void)sink;                                   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* Scalar FMA variants */
#define SCALAR_FMA_LATENCY_SS(func_name, intrinsic_call)                      \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128 accumulator = _mm_set_ss(1.0f);                                    \
    __m128 multiplicand = _mm_set_ss(1.0000001f);                             \
    __m128 addend = _mm_set_ss(0.0000001f);                                   \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128 sink = accumulator; (void)sink;                           \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define SCALAR_FMA_LATENCY_SD(func_name, intrinsic_call)                      \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128d accumulator = _mm_set_sd(1.0);                                    \
    __m128d multiplicand = _mm_set_sd(1.0000001);                             \
    __m128d addend = _mm_set_sd(0.0000001);                                   \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
        accumulator = intrinsic_call(multiplicand, addend, accumulator);      \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128d sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

#define SCALAR_FMA_THROUGHPUT_SS(func_name, intrinsic_call)                   \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128 s0 = _mm_set_ss(1.0f);                                             \
    __m128 s1 = _mm_set_ss(1.1f);                                             \
    __m128 s2 = _mm_set_ss(1.2f);                                             \
    __m128 s3 = _mm_set_ss(1.3f);                                             \
    __m128 s4 = _mm_set_ss(1.4f);                                             \
    __m128 s5 = _mm_set_ss(1.5f);                                             \
    __m128 s6 = _mm_set_ss(1.6f);                                             \
    __m128 s7 = _mm_set_ss(1.7f);                                             \
    __m128 multiplicand = _mm_set_ss(1.0000001f);                             \
    __m128 addend = _mm_set_ss(0.0000001f);                                   \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        s0 = intrinsic_call(multiplicand, addend, s0);                        \
        s1 = intrinsic_call(multiplicand, addend, s1);                        \
        s2 = intrinsic_call(multiplicand, addend, s2);                        \
        s3 = intrinsic_call(multiplicand, addend, s3);                        \
        s4 = intrinsic_call(multiplicand, addend, s4);                        \
        s5 = intrinsic_call(multiplicand, addend, s5);                        \
        s6 = intrinsic_call(multiplicand, addend, s6);                        \
        s7 = intrinsic_call(multiplicand, addend, s7);                        \
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));       \
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));       \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128 sink = s0; (void)sink;                                    \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

#define SCALAR_FMA_THROUGHPUT_SD(func_name, intrinsic_call)                   \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __m128d s0 = _mm_set_sd(1.0);                                             \
    __m128d s1 = _mm_set_sd(1.1);                                             \
    __m128d s2 = _mm_set_sd(1.2);                                             \
    __m128d s3 = _mm_set_sd(1.3);                                             \
    __m128d s4 = _mm_set_sd(1.4);                                             \
    __m128d s5 = _mm_set_sd(1.5);                                             \
    __m128d s6 = _mm_set_sd(1.6);                                             \
    __m128d s7 = _mm_set_sd(1.7);                                             \
    __m128d multiplicand = _mm_set_sd(1.0000001);                             \
    __m128d addend = _mm_set_sd(0.0000001);                                   \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        s0 = intrinsic_call(multiplicand, addend, s0);                        \
        s1 = intrinsic_call(multiplicand, addend, s1);                        \
        s2 = intrinsic_call(multiplicand, addend, s2);                        \
        s3 = intrinsic_call(multiplicand, addend, s3);                        \
        s4 = intrinsic_call(multiplicand, addend, s4);                        \
        s5 = intrinsic_call(multiplicand, addend, s5);                        \
        s6 = intrinsic_call(multiplicand, addend, s6);                        \
        s7 = intrinsic_call(multiplicand, addend, s7);                        \
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));       \
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));       \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile __m128d sink = s0; (void)sink;                                   \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* VFMADDSUB231PS/PD ymm */
YMM_FMA_LATENCY_PS(fma_vfmaddsubps_lat, _mm256_fmaddsub_ps)
YMM_FMA_THROUGHPUT_PS(fma_vfmaddsubps_tp, _mm256_fmaddsub_ps)
YMM_FMA_LATENCY_PD(fma_vfmaddsubpd_lat, _mm256_fmaddsub_pd)
YMM_FMA_THROUGHPUT_PD(fma_vfmaddsubpd_tp, _mm256_fmaddsub_pd)

/* VFMSUBADD231PS/PD ymm */
YMM_FMA_LATENCY_PS(fma_vfmsubaddps_lat, _mm256_fmsubadd_ps)
YMM_FMA_THROUGHPUT_PS(fma_vfmsubaddps_tp, _mm256_fmsubadd_ps)
YMM_FMA_LATENCY_PD(fma_vfmsubaddpd_lat, _mm256_fmsubadd_pd)
YMM_FMA_THROUGHPUT_PD(fma_vfmsubaddpd_tp, _mm256_fmsubadd_pd)

/* VFMSUB231PS/PD ymm */
YMM_FMA_LATENCY_PS(fma_vfmsubps_lat, _mm256_fmsub_ps)
YMM_FMA_THROUGHPUT_PS(fma_vfmsubps_tp, _mm256_fmsub_ps)
YMM_FMA_LATENCY_PD(fma_vfmsubpd_lat, _mm256_fmsub_pd)
YMM_FMA_THROUGHPUT_PD(fma_vfmsubpd_tp, _mm256_fmsub_pd)

/* VFNMADD231PS/PD ymm */
YMM_FMA_LATENCY_PS(fma_vfnmaddps_lat, _mm256_fnmadd_ps)
YMM_FMA_THROUGHPUT_PS(fma_vfnmaddps_tp, _mm256_fnmadd_ps)
YMM_FMA_LATENCY_PD(fma_vfnmaddpd_lat, _mm256_fnmadd_pd)
YMM_FMA_THROUGHPUT_PD(fma_vfnmaddpd_tp, _mm256_fnmadd_pd)

/* VFNMSUB231PS/PD ymm */
YMM_FMA_LATENCY_PS(fma_vfnmsubps_lat, _mm256_fnmsub_ps)
YMM_FMA_THROUGHPUT_PS(fma_vfnmsubps_tp, _mm256_fnmsub_ps)
YMM_FMA_LATENCY_PD(fma_vfnmsubpd_lat, _mm256_fnmsub_pd)
YMM_FMA_THROUGHPUT_PD(fma_vfnmsubpd_tp, _mm256_fnmsub_pd)

/* Scalar VFMSUB231SS/SD */
SCALAR_FMA_LATENCY_SS(fma_vfmsubss_lat, _mm_fmsub_ss)
SCALAR_FMA_THROUGHPUT_SS(fma_vfmsubss_tp, _mm_fmsub_ss)
SCALAR_FMA_LATENCY_SD(fma_vfmsubsd_lat, _mm_fmsub_sd)
SCALAR_FMA_THROUGHPUT_SD(fma_vfmsubsd_tp, _mm_fmsub_sd)


/* ========================================================================
 * SECTION 8: VEX vs Legacy SSE encoding comparison
 * ======================================================================== */

/* Legacy ADDPS xmm (no VEX prefix, raw .byte encoding) */
static double legacy_addps_lat(uint64_t iteration_count)
{
    __m128 accumulator = _mm_set1_ps(1.0f);
    __m128 operand_source = _mm_set1_ps(0.0000001f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* legacy addps xmm0, xmm1 = 0F 58 C1 */
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double legacy_addps_tp(uint64_t iteration_count)
{
    __m128 s0 = _mm_set1_ps(1.0f);
    __m128 s1 = _mm_set1_ps(1.1f);
    __m128 s2 = _mm_set1_ps(1.2f);
    __m128 s3 = _mm_set1_ps(1.3f);
    __m128 s4 = _mm_set1_ps(1.4f);
    __m128 s5 = _mm_set1_ps(1.5f);
    __m128 s6 = _mm_set1_ps(1.6f);
    __m128 s7 = _mm_set1_ps(1.7f);
    __m128 operand_source = _mm_set1_ps(0.0000001f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x58, 0xc1"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VEX VADDPS xmm */
static double vex_vaddps_lat(uint64_t iteration_count)
{
    __m128 accumulator = _mm_set1_ps(1.0f);
    __m128 operand_source = _mm_set1_ps(0.0000001f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vex_vaddps_tp(uint64_t iteration_count)
{
    __m128 s0 = _mm_set1_ps(1.0f);
    __m128 s1 = _mm_set1_ps(1.1f);
    __m128 s2 = _mm_set1_ps(1.2f);
    __m128 s3 = _mm_set1_ps(1.3f);
    __m128 s4 = _mm_set1_ps(1.4f);
    __m128 s5 = _mm_set1_ps(1.5f);
    __m128 s6 = _mm_set1_ps(1.6f);
    __m128 s7 = _mm_set1_ps(1.7f);
    __m128 operand_source = _mm_set1_ps(0.0000001f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile("vaddps %1, %0, %0"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* Legacy MULPS xmm = 0F 59 C1 */
static double legacy_mulps_lat(uint64_t iteration_count)
{
    __m128 accumulator = _mm_set1_ps(1.0000001f);
    __m128 operand_source = _mm_set1_ps(0.9999999f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double legacy_mulps_tp(uint64_t iteration_count)
{
    __m128 s0 = _mm_set1_ps(1.0f);
    __m128 s1 = _mm_set1_ps(1.1f);
    __m128 s2 = _mm_set1_ps(1.2f);
    __m128 s3 = _mm_set1_ps(1.3f);
    __m128 s4 = _mm_set1_ps(1.4f);
    __m128 s5 = _mm_set1_ps(1.5f);
    __m128 s6 = _mm_set1_ps(1.6f);
    __m128 s7 = _mm_set1_ps(1.7f);
    __m128 operand_source = _mm_set1_ps(0.9999999f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile(".byte 0x0f, 0x59, 0xc1"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VEX VMULPS xmm */
static double vex_vmulps_lat(uint64_t iteration_count)
{
    __m128 accumulator = _mm_set1_ps(1.0000001f);
    __m128 operand_source = _mm_set1_ps(0.9999999f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vex_vmulps_tp(uint64_t iteration_count)
{
    __m128 s0 = _mm_set1_ps(1.0f);
    __m128 s1 = _mm_set1_ps(1.1f);
    __m128 s2 = _mm_set1_ps(1.2f);
    __m128 s3 = _mm_set1_ps(1.3f);
    __m128 s4 = _mm_set1_ps(1.4f);
    __m128 s5 = _mm_set1_ps(1.5f);
    __m128 s6 = _mm_set1_ps(1.6f);
    __m128 s7 = _mm_set1_ps(1.7f);
    __m128 operand_source = _mm_set1_ps(0.9999999f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile("vmulps %1, %0, %0"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* Legacy PADDD xmm = 66 0F FE C1 */
static double legacy_paddd_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi32(1);
    __m128i operand_source = _mm_set1_epi32(1);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double legacy_paddd_tp(uint64_t iteration_count)
{
    __m128i s0 = _mm_set1_epi32(0x01);
    __m128i s1 = _mm_set1_epi32(0x02);
    __m128i s2 = _mm_set1_epi32(0x03);
    __m128i s3 = _mm_set1_epi32(0x04);
    __m128i s4 = _mm_set1_epi32(0x05);
    __m128i s5 = _mm_set1_epi32(0x06);
    __m128i s6 = _mm_set1_epi32(0x07);
    __m128i s7 = _mm_set1_epi32(0x08);
    __m128i operand_source = _mm_set1_epi32(1);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xfe, 0xc1"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VEX VPADDD xmm */
static double vex_vpaddd_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi32(1);
    __m128i operand_source = _mm_set1_epi32(1);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vex_vpaddd_tp(uint64_t iteration_count)
{
    __m128i s0 = _mm_set1_epi32(0x01);
    __m128i s1 = _mm_set1_epi32(0x02);
    __m128i s2 = _mm_set1_epi32(0x03);
    __m128i s3 = _mm_set1_epi32(0x04);
    __m128i s4 = _mm_set1_epi32(0x05);
    __m128i s5 = _mm_set1_epi32(0x06);
    __m128i s6 = _mm_set1_epi32(0x07);
    __m128i s7 = _mm_set1_epi32(0x08);
    __m128i operand_source = _mm_set1_epi32(1);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile("vpaddd %1, %0, %0"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* Legacy PMULLW xmm = 66 0F D5 C1 */
static double legacy_pmullw_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi16(3);
    __m128i operand_source = _mm_set1_epi16(2);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double legacy_pmullw_tp(uint64_t iteration_count)
{
    __m128i s0 = _mm_set1_epi16(0x01);
    __m128i s1 = _mm_set1_epi16(0x02);
    __m128i s2 = _mm_set1_epi16(0x03);
    __m128i s3 = _mm_set1_epi16(0x04);
    __m128i s4 = _mm_set1_epi16(0x05);
    __m128i s5 = _mm_set1_epi16(0x06);
    __m128i s6 = _mm_set1_epi16(0x07);
    __m128i s7 = _mm_set1_epi16(0x08);
    __m128i operand_source = _mm_set1_epi16(2);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile(".byte 0x66, 0x0f, 0xd5, 0xc1"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = s0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* VEX VPMULLW xmm */
static double vex_vpmullw_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi16(3);
    __m128i operand_source = _mm_set1_epi16(2);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(accumulator) : "x"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vex_vpmullw_tp(uint64_t iteration_count)
{
    __m128i s0 = _mm_set1_epi16(0x01);
    __m128i s1 = _mm_set1_epi16(0x02);
    __m128i s2 = _mm_set1_epi16(0x03);
    __m128i s3 = _mm_set1_epi16(0x04);
    __m128i s4 = _mm_set1_epi16(0x05);
    __m128i s5 = _mm_set1_epi16(0x06);
    __m128i s6 = _mm_set1_epi16(0x07);
    __m128i s7 = _mm_set1_epi16(0x08);
    __m128i operand_source = _mm_set1_epi16(2);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s0) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s1) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s2) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s3) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s4) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s5) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s6) : "x"(operand_source));
        __asm__ volatile("vpmullw %1, %0, %0"
                         : "+x"(s7) : "x"(operand_source));
        __asm__ volatile("" : "+x"(s0), "+x"(s1), "+x"(s2), "+x"(s3));
        __asm__ volatile("" : "+x"(s4), "+x"(s5), "+x"(s6), "+x"(s7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = s0; (void)sink;
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
} LegacyProbeEntry;

static const LegacyProbeEntry legacy_probe_table[] = {
    /* --- MMX --- */
    {"MOVD",      "mm,r32/r32,mm",  "MMX",   "mmx_move",  mmx_movd_mm_r32_lat,  mmx_movd_mm_r32_tp},
    {"MOVD",      "r32,mm",         "MMX",   "mmx_move",  NULL,                  mmx_movd_r32_mm_tp},
    {"MOVQ",      "mm,mm",          "MMX",   "mmx_move",  mmx_movq_mm_mm_lat,   mmx_movq_mm_mm_tp},
    {"PADDB",     "mm,mm",          "MMX",   "mmx_arith", mmx_paddb_lat,         mmx_paddb_tp},
    {"PADDW",     "mm,mm",          "MMX",   "mmx_arith", mmx_paddw_lat,         mmx_paddw_tp},
    {"PADDD",     "mm,mm",          "MMX",   "mmx_arith", mmx_paddd_lat,         mmx_paddd_tp},
    {"PSUBB",     "mm,mm",          "MMX",   "mmx_arith", mmx_psubb_lat,         mmx_psubb_tp},
    {"PSUBW",     "mm,mm",          "MMX",   "mmx_arith", mmx_psubw_lat,         mmx_psubw_tp},
    {"PSUBD",     "mm,mm",          "MMX",   "mmx_arith", mmx_psubd_lat,         mmx_psubd_tp},
    {"PADDUSB",   "mm,mm",          "MMX",   "mmx_arith", mmx_paddusb_lat,       mmx_paddusb_tp},
    {"PADDUSW",   "mm,mm",          "MMX",   "mmx_arith", mmx_paddusw_lat,       mmx_paddusw_tp},
    {"PADDSB",    "mm,mm",          "MMX",   "mmx_arith", mmx_paddsb_lat,        mmx_paddsb_tp},
    {"PADDSW",    "mm,mm",          "MMX",   "mmx_arith", mmx_paddsw_lat,        mmx_paddsw_tp},
    {"PMULLW",    "mm,mm",          "MMX",   "mmx_mul",   mmx_pmullw_lat,        mmx_pmullw_tp},
    {"PMULHW",    "mm,mm",          "MMX",   "mmx_mul",   mmx_pmulhw_lat,        mmx_pmulhw_tp},
    {"PMADDWD",   "mm,mm",          "MMX",   "mmx_mul",   mmx_pmaddwd_lat,       mmx_pmaddwd_tp},
    {"PAND",      "mm,mm",          "MMX",   "mmx_logic", mmx_pand_lat,          mmx_pand_tp},
    {"PANDN",     "mm,mm",          "MMX",   "mmx_logic", mmx_pandn_lat,         mmx_pandn_tp},
    {"POR",       "mm,mm",          "MMX",   "mmx_logic", mmx_por_lat,           mmx_por_tp},
    {"PXOR",      "mm,mm",          "MMX",   "mmx_logic", mmx_pxor_lat,          mmx_pxor_tp},
    {"PSLLW",     "mm,$3",          "MMX",   "mmx_shift", mmx_psllw_lat,         mmx_psllw_tp},
    {"PSLLD",     "mm,$7",          "MMX",   "mmx_shift", mmx_pslld_lat,         mmx_pslld_tp},
    {"PSLLQ",     "mm,$11",         "MMX",   "mmx_shift", mmx_psllq_lat,         mmx_psllq_tp},
    {"PSRLW",     "mm,$3",          "MMX",   "mmx_shift", mmx_psrlw_lat,         mmx_psrlw_tp},
    {"PSRLD",     "mm,$7",          "MMX",   "mmx_shift", mmx_psrld_lat,         mmx_psrld_tp},
    {"PSRLQ",     "mm,$11",         "MMX",   "mmx_shift", mmx_psrlq_lat,         mmx_psrlq_tp},
    {"PSRAW",     "mm,$3",          "MMX",   "mmx_shift", mmx_psraw_lat,         mmx_psraw_tp},
    {"PSRAD",     "mm,$7",          "MMX",   "mmx_shift", mmx_psrad_lat,         mmx_psrad_tp},
    {"PCMPEQB",   "mm,mm",          "MMX",   "mmx_cmp",   mmx_pcmpeqb_lat,       mmx_pcmpeqb_tp},
    {"PCMPEQW",   "mm,mm",          "MMX",   "mmx_cmp",   mmx_pcmpeqw_lat,       mmx_pcmpeqw_tp},
    {"PCMPEQD",   "mm,mm",          "MMX",   "mmx_cmp",   mmx_pcmpeqd_lat,       mmx_pcmpeqd_tp},
    {"PCMPGTB",   "mm,mm",          "MMX",   "mmx_cmp",   mmx_pcmpgtb_lat,       mmx_pcmpgtb_tp},
    {"PCMPGTW",   "mm,mm",          "MMX",   "mmx_cmp",   mmx_pcmpgtw_lat,       mmx_pcmpgtw_tp},
    {"PCMPGTD",   "mm,mm",          "MMX",   "mmx_cmp",   mmx_pcmpgtd_lat,       mmx_pcmpgtd_tp},
    {"PUNPCKHBW", "mm,mm",          "MMX",   "mmx_shuf",  mmx_punpckhbw_lat,     mmx_punpckhbw_tp},
    {"PUNPCKHWD", "mm,mm",          "MMX",   "mmx_shuf",  mmx_punpckhwd_lat,     mmx_punpckhwd_tp},
    {"PUNPCKHDQ", "mm,mm",          "MMX",   "mmx_shuf",  mmx_punpckhdq_lat,     mmx_punpckhdq_tp},
    {"PUNPCKLBW", "mm,mm",          "MMX",   "mmx_shuf",  mmx_punpcklbw_lat,     mmx_punpcklbw_tp},
    {"PUNPCKLWD", "mm,mm",          "MMX",   "mmx_shuf",  mmx_punpcklwd_lat,     mmx_punpcklwd_tp},
    {"PUNPCKLDQ", "mm,mm",          "MMX",   "mmx_shuf",  mmx_punpckldq_lat,     mmx_punpckldq_tp},
    {"PACKUSWB",  "mm,mm",          "MMX",   "mmx_pack",  mmx_packuswb_lat,      mmx_packuswb_tp},
    {"PACKSSWB",  "mm,mm",          "MMX",   "mmx_pack",  mmx_packsswb_lat,      mmx_packsswb_tp},
    {"PACKSSDW",  "mm,mm",          "MMX",   "mmx_pack",  mmx_packssdw_lat,      mmx_packssdw_tp},
    {"EMMS",      "",               "MMX",   "mmx_misc",  NULL,                  mmx_emms_tp},

    /* --- SSE4a --- */
    {"EXTRQ",     "xmm,$8,$0",      "SSE4a", "sse4a",     sse4a_extrq_lat,       sse4a_extrq_tp},
    {"INSERTQ",   "xmm,xmm,$8,$0",  "SSE4a", "sse4a",     sse4a_insertq_lat,     sse4a_insertq_tp},
    {"MOVNTSS",   "[m32],xmm",      "SSE4a", "sse4a_nt",  NULL,                  sse4a_movntss_tp},
    {"MOVNTSD",   "[m64],xmm",      "SSE4a", "sse4a_nt",  NULL,                  sse4a_movntsd_tp},

    /* --- SSE4.2 String Instructions --- */
    {"PCMPESTRI", "xmm,xmm,$0C",    "SSE4.2","str_cmp",   sse42_pcmpestri_0c_lat,sse42_pcmpestri_0c_tp},
    {"PCMPESTRI", "xmm,xmm,$4C",    "SSE4.2","str_cmp",   sse42_pcmpestri_4c_lat,NULL},
    {"PCMPESTRM", "xmm,xmm,$0C",    "SSE4.2","str_cmp",   sse42_pcmpestrm_0c_lat,sse42_pcmpestrm_0c_tp},
    {"PCMPISTRI", "xmm,xmm,$0C",    "SSE4.2","str_cmp",   sse42_pcmpistri_0c_lat,sse42_pcmpistri_0c_tp},
    {"PCMPISTRI", "xmm,xmm,$4C",    "SSE4.2","str_cmp",   sse42_pcmpistri_4c_lat,NULL},
    {"PCMPISTRM", "xmm,xmm,$0C",    "SSE4.2","str_cmp",   sse42_pcmpistrm_0c_lat,sse42_pcmpistrm_0c_tp},
    {"PCMPISTRM", "xmm,xmm,$4C",    "SSE4.2","str_cmp",   sse42_pcmpistrm_4c_lat,NULL},

    /* --- CRC32 --- */
    {"CRC32",     "r32,r8",          "SSE4.2","crc",       crc32_u8_lat,          crc32_u8_tp},
    {"CRC32",     "r32,r16",         "SSE4.2","crc",       crc32_u16_lat,         crc32_u16_tp},
    {"CRC32",     "r32,r32",         "SSE4.2","crc",       crc32_u32_lat,         crc32_u32_tp},
    {"CRC32",     "r64,r64",         "SSE4.2","crc",       crc32_u64_lat,         crc32_u64_tp},

    /* --- DPPD --- */
    {"DPPD",      "xmm,xmm,$31",    "SSE4.1","fp_dot",    dppd_0x31_lat,         dppd_0x31_tp},
    {"DPPD",      "xmm,xmm,$33",    "SSE4.1","fp_dot",    dppd_0x33_lat,         dppd_0x33_tp},

    /* --- SSSE3 --- */
    {"PSIGNB",    "xmm,xmm",        "SSSE3", "ssse3_sign",ssse3_psignb_lat,      ssse3_psignb_tp},
    {"PSIGNW",    "xmm,xmm",        "SSSE3", "ssse3_sign",ssse3_psignw_lat,      ssse3_psignw_tp},
    {"PSIGND",    "xmm,xmm",        "SSSE3", "ssse3_sign",ssse3_psignd_lat,      ssse3_psignd_tp},
    {"PABSB",     "xmm,xmm",        "SSSE3", "ssse3_abs", ssse3_pabsb_lat,       ssse3_pabsb_tp},
    {"PABSW",     "xmm,xmm",        "SSSE3", "ssse3_abs", ssse3_pabsw_lat,       ssse3_pabsw_tp},
    {"PABSD",     "xmm,xmm",        "SSSE3", "ssse3_abs", ssse3_pabsd_lat,       ssse3_pabsd_tp},
    {"PHMINPOSUW","xmm,xmm",        "SSE4.1","sse41_hmin",sse41_phminposuw_lat,  sse41_phminposuw_tp},

    /* --- FMA variants --- */
    {"VFMADDSUBPS","ymm,ymm,ymm",   "FMA",   "fma_addsub",fma_vfmaddsubps_lat,  fma_vfmaddsubps_tp},
    {"VFMADDSUBPD","ymm,ymm,ymm",   "FMA",   "fma_addsub",fma_vfmaddsubpd_lat,  fma_vfmaddsubpd_tp},
    {"VFMSUBADDPS","ymm,ymm,ymm",   "FMA",   "fma_addsub",fma_vfmsubaddps_lat,  fma_vfmsubaddps_tp},
    {"VFMSUBADDPD","ymm,ymm,ymm",   "FMA",   "fma_addsub",fma_vfmsubaddpd_lat,  fma_vfmsubaddpd_tp},
    {"VFMSUBPS",  "ymm,ymm,ymm",    "FMA",   "fma_sub",   fma_vfmsubps_lat,     fma_vfmsubps_tp},
    {"VFMSUBPD",  "ymm,ymm,ymm",    "FMA",   "fma_sub",   fma_vfmsubpd_lat,     fma_vfmsubpd_tp},
    {"VFNMADDPS", "ymm,ymm,ymm",    "FMA",   "fma_neg",   fma_vfnmaddps_lat,    fma_vfnmaddps_tp},
    {"VFNMADDPD", "ymm,ymm,ymm",    "FMA",   "fma_neg",   fma_vfnmaddpd_lat,    fma_vfnmaddpd_tp},
    {"VFNMSUBPS", "ymm,ymm,ymm",    "FMA",   "fma_neg",   fma_vfnmsubps_lat,    fma_vfnmsubps_tp},
    {"VFNMSUBPD", "ymm,ymm,ymm",    "FMA",   "fma_neg",   fma_vfnmsubpd_lat,    fma_vfnmsubpd_tp},
    {"VFMSUBSS",  "xmm,xmm,xmm",   "FMA",   "fma_scalar",fma_vfmsubss_lat,     fma_vfmsubss_tp},
    {"VFMSUBSD",  "xmm,xmm,xmm",   "FMA",   "fma_scalar",fma_vfmsubsd_lat,     fma_vfmsubsd_tp},

    /* --- VEX vs Legacy encoding --- */
    {"ADDPS",     "xmm,xmm(legacy)","legacy","vex_cmp",   legacy_addps_lat,      legacy_addps_tp},
    {"VADDPS",    "xmm,xmm(VEX)",   "VEX",   "vex_cmp",   vex_vaddps_lat,        vex_vaddps_tp},
    {"MULPS",     "xmm,xmm(legacy)","legacy","vex_cmp",   legacy_mulps_lat,      legacy_mulps_tp},
    {"VMULPS",    "xmm,xmm(VEX)",   "VEX",   "vex_cmp",   vex_vmulps_lat,        vex_vmulps_tp},
    {"PADDD",     "xmm,xmm(legacy)","legacy","vex_cmp",   legacy_paddd_lat,      legacy_paddd_tp},
    {"VPADDD",    "xmm,xmm(VEX)",   "VEX",   "vex_cmp",   vex_vpaddd_lat,        vex_vpaddd_tp},
    {"PMULLW",    "xmm,xmm(legacy)","legacy","vex_cmp",   legacy_pmullw_lat,     legacy_pmullw_tp},
    {"VPMULLW",   "xmm,xmm(VEX)",   "VEX",   "vex_cmp",   vex_vpmullw_lat,       vex_vpmullw_tp},

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
        sm_zen_print_header("legacy_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n",
               "legacy ISA extension (MMX/SSE4a/SSE4.2str/CRC32/DPPD/SSSE3/FMA/VEX)");
    }

    /* Print CSV header */
    printf("mnemonic,operands,extension,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; legacy_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const LegacyProbeEntry *current_entry = &legacy_probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; legacy_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const LegacyProbeEntry *current_entry = &legacy_probe_table[entry_idx];

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
