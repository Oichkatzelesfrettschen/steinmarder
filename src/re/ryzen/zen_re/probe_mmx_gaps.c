#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <mmintrin.h>
#include <tmmintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_mmx_gaps.c -- Fill missing native MMX (mm register) instruction forms
 * identified in the Zen 4 coverage gap report.
 *
 * Covers: PADDQ, PSUBQ, PSUBUSB, PSUBUSW, PSUBSB, PSUBSW, PMULHUW,
 * PMULUDQ, PUNPCKHQDQ, PUNPCKLQDQ, PSHUFW, PEXTRW, PINSRW, PAVGB, PAVGW,
 * PSADBW, PMAXUB, PMINUB, PMAXSW, PMINSW, shift-by-register forms
 * (PSLLW/D/Q mm,mm; PSRLW/D/Q mm,mm; PSRAW/D mm,mm), PSHUFB mm (SSSE3),
 * PALIGNR mm (SSSE3).
 *
 * Output: one CSV line per instruction form:
 *   mnemonic,operands,extension,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -msse4.1 -msse4.2 -mssse3 -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_mmx_gaps.c -lm \
 *     -o ../../../../build/bin/zen_probe_mmx_gaps
 */

/* ========================================================================
 * MACRO TEMPLATES -- MMX latency (4-deep dependent chain)
 * ======================================================================== */

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

/* ========================================================================
 * MACRO TEMPLATES -- MMX throughput (8 independent streams)
 * ======================================================================== */

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

/* MMX latency/throughput using inline asm with "+y" constraint */
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

/* ========================================================================
 * SECTION 1: Missing MMX arithmetic
 * ======================================================================== */

MMX_LATENCY_ASM(mmx_paddq_lat, "paddq %1, %0")
MMX_THROUGHPUT_ASM(mmx_paddq_tp, "paddq %1, %0")

MMX_LATENCY_ASM(mmx_psubq_lat, "psubq %1, %0")
MMX_THROUGHPUT_ASM(mmx_psubq_tp, "psubq %1, %0")

MMX_LATENCY_BINARY(mmx_psubusb_lat, _mm_subs_pu8)
MMX_THROUGHPUT_BINARY(mmx_psubusb_tp, _mm_subs_pu8)

MMX_LATENCY_BINARY(mmx_psubusw_lat, _mm_subs_pu16)
MMX_THROUGHPUT_BINARY(mmx_psubusw_tp, _mm_subs_pu16)

MMX_LATENCY_BINARY(mmx_psubsb_lat, _mm_subs_pi8)
MMX_THROUGHPUT_BINARY(mmx_psubsb_tp, _mm_subs_pi8)

MMX_LATENCY_BINARY(mmx_psubsw_lat, _mm_subs_pi16)
MMX_THROUGHPUT_BINARY(mmx_psubsw_tp, _mm_subs_pi16)

/* ========================================================================
 * SECTION 2: Missing MMX multiply
 * ======================================================================== */

MMX_LATENCY_ASM(mmx_pmulhuw_lat, "pmulhuw %1, %0")
MMX_THROUGHPUT_ASM(mmx_pmulhuw_tp, "pmulhuw %1, %0")

MMX_LATENCY_ASM(mmx_pmuludq_lat, "pmuludq %1, %0")
MMX_THROUGHPUT_ASM(mmx_pmuludq_tp, "pmuludq %1, %0")

/* ========================================================================
 * SECTION 3: Missing MMX shuffle/unpack
 * ======================================================================== */

/* Note: PUNPCKHQDQ/PUNPCKLQDQ are SSE2 XMM-only; no MMX mm form exists. */

/* PSHUFW mm,mm,imm8 */
static double mmx_pshufw_lat(uint64_t iteration_count)
{
    __m64 accumulator = _mm_set1_pi32(0x12345678);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_shuffle_pi16(accumulator, 0x1B);
        accumulator = _mm_shuffle_pi16(accumulator, 0x1B);
        accumulator = _mm_shuffle_pi16(accumulator, 0x1B);
        accumulator = _mm_shuffle_pi16(accumulator, 0x1B);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m64 sink = accumulator; (void)sink;
    _mm_empty();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mmx_pshufw_tp(uint64_t iteration_count)
{
    __m64 stream_0 = _mm_set1_pi32(0x01), stream_1 = _mm_set1_pi32(0x02);
    __m64 stream_2 = _mm_set1_pi32(0x03), stream_3 = _mm_set1_pi32(0x04);
    __m64 stream_4 = _mm_set1_pi32(0x05), stream_5 = _mm_set1_pi32(0x06);
    __m64 stream_6 = _mm_set1_pi32(0x07), stream_7 = _mm_set1_pi32(0x08);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_shuffle_pi16(stream_0, 0x1B);
        stream_1 = _mm_shuffle_pi16(stream_1, 0x1B);
        stream_2 = _mm_shuffle_pi16(stream_2, 0x1B);
        stream_3 = _mm_shuffle_pi16(stream_3, 0x1B);
        stream_4 = _mm_shuffle_pi16(stream_4, 0x1B);
        stream_5 = _mm_shuffle_pi16(stream_5, 0x1B);
        stream_6 = _mm_shuffle_pi16(stream_6, 0x1B);
        stream_7 = _mm_shuffle_pi16(stream_7, 0x1B);
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

/* ========================================================================
 * SECTION 4: PEXTRW / PINSRW (SSE extension for MMX)
 * ======================================================================== */

static double mmx_pextrw_lat(uint64_t iteration_count)
{
    __m64 mmx_source = _mm_set1_pi32(0xCAFEBABE);
    int extracted_word = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        extracted_word = _mm_extract_pi16(mmx_source, 0);
        mmx_source = _mm_set1_pi16((short)extracted_word);
        extracted_word = _mm_extract_pi16(mmx_source, 1);
        mmx_source = _mm_set1_pi16((short)extracted_word);
        extracted_word = _mm_extract_pi16(mmx_source, 2);
        mmx_source = _mm_set1_pi16((short)extracted_word);
        extracted_word = _mm_extract_pi16(mmx_source, 3);
        mmx_source = _mm_set1_pi16((short)extracted_word);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = extracted_word; (void)sink;
    _mm_empty();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mmx_pextrw_tp(uint64_t iteration_count)
{
    __m64 mmx_source = _mm_set1_pi32(0xCAFEBABE);
    int result_0, result_1, result_2, result_3;
    int result_4, result_5, result_6, result_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        result_0 = _mm_extract_pi16(mmx_source, 0);
        result_1 = _mm_extract_pi16(mmx_source, 1);
        result_2 = _mm_extract_pi16(mmx_source, 2);
        result_3 = _mm_extract_pi16(mmx_source, 3);
        result_4 = _mm_extract_pi16(mmx_source, 0);
        result_5 = _mm_extract_pi16(mmx_source, 1);
        result_6 = _mm_extract_pi16(mmx_source, 2);
        result_7 = _mm_extract_pi16(mmx_source, 3);
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int sink = result_0 ^ result_4; (void)sink;
    _mm_empty();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

static double mmx_pinsrw_lat(uint64_t iteration_count)
{
    __m64 accumulator = _mm_set1_pi32(1);
    int insert_value = 0x1234;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_insert_pi16(accumulator, insert_value, 0);
        accumulator = _mm_insert_pi16(accumulator, insert_value, 1);
        accumulator = _mm_insert_pi16(accumulator, insert_value, 2);
        accumulator = _mm_insert_pi16(accumulator, insert_value, 3);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m64 sink = accumulator; (void)sink;
    _mm_empty();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mmx_pinsrw_tp(uint64_t iteration_count)
{
    __m64 stream_0 = _mm_set1_pi32(0x01), stream_1 = _mm_set1_pi32(0x02);
    __m64 stream_2 = _mm_set1_pi32(0x03), stream_3 = _mm_set1_pi32(0x04);
    __m64 stream_4 = _mm_set1_pi32(0x05), stream_5 = _mm_set1_pi32(0x06);
    __m64 stream_6 = _mm_set1_pi32(0x07), stream_7 = _mm_set1_pi32(0x08);
    int insert_value = 0xABCD;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_insert_pi16(stream_0, insert_value, 0);
        stream_1 = _mm_insert_pi16(stream_1, insert_value, 1);
        stream_2 = _mm_insert_pi16(stream_2, insert_value, 2);
        stream_3 = _mm_insert_pi16(stream_3, insert_value, 3);
        stream_4 = _mm_insert_pi16(stream_4, insert_value, 0);
        stream_5 = _mm_insert_pi16(stream_5, insert_value, 1);
        stream_6 = _mm_insert_pi16(stream_6, insert_value, 2);
        stream_7 = _mm_insert_pi16(stream_7, insert_value, 3);
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

/* ========================================================================
 * SECTION 5: PAVGB, PAVGW, PSADBW, PMAXUB, PMINUB, PMAXSW, PMINSW
 * ======================================================================== */

MMX_LATENCY_BINARY(mmx_pavgb_lat, _mm_avg_pu8)
MMX_THROUGHPUT_BINARY(mmx_pavgb_tp, _mm_avg_pu8)
MMX_LATENCY_BINARY(mmx_pavgw_lat, _mm_avg_pu16)
MMX_THROUGHPUT_BINARY(mmx_pavgw_tp, _mm_avg_pu16)
MMX_LATENCY_BINARY(mmx_psadbw_lat, _mm_sad_pu8)
MMX_THROUGHPUT_BINARY(mmx_psadbw_tp, _mm_sad_pu8)
MMX_LATENCY_BINARY(mmx_pmaxub_lat, _mm_max_pu8)
MMX_THROUGHPUT_BINARY(mmx_pmaxub_tp, _mm_max_pu8)
MMX_LATENCY_BINARY(mmx_pminub_lat, _mm_min_pu8)
MMX_THROUGHPUT_BINARY(mmx_pminub_tp, _mm_min_pu8)
MMX_LATENCY_BINARY(mmx_pmaxsw_lat, _mm_max_pi16)
MMX_THROUGHPUT_BINARY(mmx_pmaxsw_tp, _mm_max_pi16)
MMX_LATENCY_BINARY(mmx_pminsw_lat, _mm_min_pi16)
MMX_THROUGHPUT_BINARY(mmx_pminsw_tp, _mm_min_pi16)

/* ========================================================================
 * SECTION 6: Shift-by-register forms
 * ======================================================================== */

MMX_LATENCY_ASM(mmx_psllw_reg_lat, "psllw %1, %0")
MMX_THROUGHPUT_ASM(mmx_psllw_reg_tp, "psllw %1, %0")
MMX_LATENCY_ASM(mmx_pslld_reg_lat, "pslld %1, %0")
MMX_THROUGHPUT_ASM(mmx_pslld_reg_tp, "pslld %1, %0")
MMX_LATENCY_ASM(mmx_psllq_reg_lat, "psllq %1, %0")
MMX_THROUGHPUT_ASM(mmx_psllq_reg_tp, "psllq %1, %0")
MMX_LATENCY_ASM(mmx_psrlw_reg_lat, "psrlw %1, %0")
MMX_THROUGHPUT_ASM(mmx_psrlw_reg_tp, "psrlw %1, %0")
MMX_LATENCY_ASM(mmx_psrld_reg_lat, "psrld %1, %0")
MMX_THROUGHPUT_ASM(mmx_psrld_reg_tp, "psrld %1, %0")
MMX_LATENCY_ASM(mmx_psrlq_reg_lat, "psrlq %1, %0")
MMX_THROUGHPUT_ASM(mmx_psrlq_reg_tp, "psrlq %1, %0")
MMX_LATENCY_ASM(mmx_psraw_reg_lat, "psraw %1, %0")
MMX_THROUGHPUT_ASM(mmx_psraw_reg_tp, "psraw %1, %0")
MMX_LATENCY_ASM(mmx_psrad_reg_lat, "psrad %1, %0")
MMX_THROUGHPUT_ASM(mmx_psrad_reg_tp, "psrad %1, %0")

/* ========================================================================
 * SECTION 7: SSSE3 in MMX registers -- PSHUFB mm, PALIGNR mm
 * ======================================================================== */

MMX_LATENCY_ASM(mmx_pshufb_lat, "pshufb %1, %0")
MMX_THROUGHPUT_ASM(mmx_pshufb_tp, "pshufb %1, %0")

static double mmx_palignr_lat(uint64_t iteration_count)
{
    __m64 accumulator = _mm_set1_pi32(1);
    __m64 operand_source = _mm_set1_pi32(2);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("palignr $4, %1, %0" : "+y"(accumulator) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(accumulator) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(accumulator) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(accumulator) : "y"(operand_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m64 sink = accumulator; (void)sink;
    _mm_empty();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mmx_palignr_tp(uint64_t iteration_count)
{
    __m64 stream_0 = _mm_set1_pi32(0x01), stream_1 = _mm_set1_pi32(0x02);
    __m64 stream_2 = _mm_set1_pi32(0x03), stream_3 = _mm_set1_pi32(0x04);
    __m64 stream_4 = _mm_set1_pi32(0x05), stream_5 = _mm_set1_pi32(0x06);
    __m64 stream_6 = _mm_set1_pi32(0x07), stream_7 = _mm_set1_pi32(0x08);
    __m64 operand_source = _mm_set1_pi32(1);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_0) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_1) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_2) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_3) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_4) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_5) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_6) : "y"(operand_source));
        __asm__ volatile("palignr $4, %1, %0" : "+y"(stream_7) : "y"(operand_source));
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
} MmxGapProbeEntry;

static const MmxGapProbeEntry mmx_gap_probe_table[] = {
    {"PADDQ",        "mm,mm",       "MMX/SSE2","mmx_arith",  mmx_paddq_lat,         mmx_paddq_tp},
    {"PSUBQ",        "mm,mm",       "MMX/SSE2","mmx_arith",  mmx_psubq_lat,         mmx_psubq_tp},
    {"PSUBUSB",      "mm,mm",       "MMX",     "mmx_arith",  mmx_psubusb_lat,       mmx_psubusb_tp},
    {"PSUBUSW",      "mm,mm",       "MMX",     "mmx_arith",  mmx_psubusw_lat,       mmx_psubusw_tp},
    {"PSUBSB",       "mm,mm",       "MMX",     "mmx_arith",  mmx_psubsb_lat,        mmx_psubsb_tp},
    {"PSUBSW",       "mm,mm",       "MMX",     "mmx_arith",  mmx_psubsw_lat,        mmx_psubsw_tp},
    {"PMULHUW",      "mm,mm",       "MMX/SSE", "mmx_mul",    mmx_pmulhuw_lat,       mmx_pmulhuw_tp},
    {"PMULUDQ",      "mm,mm",       "MMX/SSE2","mmx_mul",    mmx_pmuludq_lat,       mmx_pmuludq_tp},
    {"PSHUFW",       "mm,mm,$1B",   "MMX/SSE", "mmx_shuf",   mmx_pshufw_lat,        mmx_pshufw_tp},
    {"PEXTRW",       "r32,mm,$n",   "MMX/SSE", "mmx_insert", mmx_pextrw_lat,        mmx_pextrw_tp},
    {"PINSRW",       "mm,r32,$n",   "MMX/SSE", "mmx_insert", mmx_pinsrw_lat,        mmx_pinsrw_tp},
    {"PAVGB",        "mm,mm",       "MMX/SSE", "mmx_stat",   mmx_pavgb_lat,         mmx_pavgb_tp},
    {"PAVGW",        "mm,mm",       "MMX/SSE", "mmx_stat",   mmx_pavgw_lat,         mmx_pavgw_tp},
    {"PSADBW",       "mm,mm",       "MMX/SSE", "mmx_stat",   mmx_psadbw_lat,        mmx_psadbw_tp},
    {"PMAXUB",       "mm,mm",       "MMX/SSE", "mmx_minmax", mmx_pmaxub_lat,        mmx_pmaxub_tp},
    {"PMINUB",       "mm,mm",       "MMX/SSE", "mmx_minmax", mmx_pminub_lat,        mmx_pminub_tp},
    {"PMAXSW",       "mm,mm",       "MMX/SSE", "mmx_minmax", mmx_pmaxsw_lat,        mmx_pmaxsw_tp},
    {"PMINSW",       "mm,mm",       "MMX/SSE", "mmx_minmax", mmx_pminsw_lat,        mmx_pminsw_tp},
    {"PSLLW",        "mm,mm",       "MMX",     "mmx_shift",  mmx_psllw_reg_lat,     mmx_psllw_reg_tp},
    {"PSLLD",        "mm,mm",       "MMX",     "mmx_shift",  mmx_pslld_reg_lat,     mmx_pslld_reg_tp},
    {"PSLLQ",        "mm,mm",       "MMX",     "mmx_shift",  mmx_psllq_reg_lat,     mmx_psllq_reg_tp},
    {"PSRLW",        "mm,mm",       "MMX",     "mmx_shift",  mmx_psrlw_reg_lat,     mmx_psrlw_reg_tp},
    {"PSRLD",        "mm,mm",       "MMX",     "mmx_shift",  mmx_psrld_reg_lat,     mmx_psrld_reg_tp},
    {"PSRLQ",        "mm,mm",       "MMX",     "mmx_shift",  mmx_psrlq_reg_lat,     mmx_psrlq_reg_tp},
    {"PSRAW",        "mm,mm",       "MMX",     "mmx_shift",  mmx_psraw_reg_lat,     mmx_psraw_reg_tp},
    {"PSRAD",        "mm,mm",       "MMX",     "mmx_shift",  mmx_psrad_reg_lat,     mmx_psrad_reg_tp},
    {"PSHUFB",       "mm,mm",       "SSSE3",   "mmx_ssse3",  mmx_pshufb_lat,        mmx_pshufb_tp},
    {"PALIGNR",      "mm,mm,$4",    "SSSE3",   "mmx_ssse3",  mmx_palignr_lat,       mmx_palignr_tp},
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
        sm_zen_print_header("mmx_gaps", &probe_config, &detected_features);
        printf("\nMeasuring missing MMX register (mm) gap-fill instruction forms...\n\n");
    }
    printf("mnemonic,operands,extension,latency_cycles,throughput_cycles,category\n");

    for (int warmup_idx = 0; mmx_gap_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const MmxGapProbeEntry *current_entry = &mmx_gap_probe_table[warmup_idx];
        if (current_entry->measure_latency)   current_entry->measure_latency(1000);
        if (current_entry->measure_throughput) current_entry->measure_throughput(1000);
    }

    for (int entry_idx = 0; mmx_gap_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const MmxGapProbeEntry *current_entry = &mmx_gap_probe_table[entry_idx];
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
