#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_scalar_exhaustive.c -- Exhaustive scalar integer instruction measurement.
 *
 * Auto-measures latency (4-deep dependency chain) and throughput (8 independent
 * streams) for every scalar integer instruction form on Zen 4.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_scalar_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_scalar_exhaustive
 */

/* ========================================================================
 * MACRO TEMPLATES
 * ======================================================================== */

/*
 * LATENCY_UNARY: instruction reads and writes the same register.
 *   e.g., "inc %0", "neg %0", "not %0", "bswap %0"
 */
#define LATENCY_UNARY(func_name, asm_str)                                      \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;                             \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * THROUGHPUT_UNARY: 8 independent streams of a unary instruction.
 */
#define THROUGHPUT_UNARY(func_name, asm_str)                                    \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t stream_0 = 0x0123456789ABCDEFULL;                                \
    uint64_t stream_1 = 0x1122334455667788ULL;                                \
    uint64_t stream_2 = 0x2233445566778899ULL;                                \
    uint64_t stream_3 = 0x33445566778899AAULL;                                \
    uint64_t stream_4 = 0x445566778899AABBULL;                                \
    uint64_t stream_5 = 0x5566778899AABBCCULL;                                \
    uint64_t stream_6 = 0x66778899AABBCCDDULL;                                \
    uint64_t stream_7 = 0x778899AABBCCDDEULL;                                 \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(stream_0));                            \
        __asm__ volatile(asm_str : "+r"(stream_1));                            \
        __asm__ volatile(asm_str : "+r"(stream_2));                            \
        __asm__ volatile(asm_str : "+r"(stream_3));                            \
        __asm__ volatile(asm_str : "+r"(stream_4));                            \
        __asm__ volatile(asm_str : "+r"(stream_5));                            \
        __asm__ volatile(asm_str : "+r"(stream_6));                            \
        __asm__ volatile(asm_str : "+r"(stream_7));                            \
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),                 \
                              "+r"(stream_2), "+r"(stream_3),                 \
                              "+r"(stream_4), "+r"(stream_5),                 \
                              "+r"(stream_6), "+r"(stream_7));                \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/*
 * LATENCY_BINARY_REG: two-operand instruction: "op src, dst" where dst
 * is both input and output. Second operand is a non-dependent register.
 *   e.g., "add %1, %0", "sub %1, %0", "adc %1, %0"
 */
#define LATENCY_BINARY_REG(func_name, asm_str)                                  \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;                             \
    uint64_t operand_source = 0x0000000000000001ULL;                           \
    __asm__ volatile("" : "+r"(operand_source));                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * THROUGHPUT_BINARY_REG: 8 independent binary-reg streams.
 */
#define THROUGHPUT_BINARY_REG(func_name, asm_str)                               \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t stream_0 = 0x0123456789ABCDEFULL;                                \
    uint64_t stream_1 = 0x1122334455667788ULL;                                \
    uint64_t stream_2 = 0x2233445566778899ULL;                                \
    uint64_t stream_3 = 0x33445566778899AAULL;                                \
    uint64_t stream_4 = 0x445566778899AABBULL;                                \
    uint64_t stream_5 = 0x5566778899AABBCCULL;                                \
    uint64_t stream_6 = 0x66778899AABBCCDDULL;                                \
    uint64_t stream_7 = 0x778899AABBCCDDEULL;                                 \
    uint64_t operand_source = 0x0000000000000001ULL;                           \
    __asm__ volatile("" : "+r"(operand_source));                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(stream_0) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_1) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_2) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_3) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_4) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_5) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_6) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_7) : "r"(operand_source));      \
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),                 \
                              "+r"(stream_2), "+r"(stream_3),                 \
                              "+r"(stream_4), "+r"(stream_5),                 \
                              "+r"(stream_6), "+r"(stream_7));                \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/*
 * LATENCY_BMI_TERNARY: three-operand BMI-style "op src2, src1, dst"
 * where dst feeds back as src1 next iteration.
 *   e.g., "andn %1, %0, %0", "bextr %1, %0, %0"
 */
#define LATENCY_BMI_TERNARY(func_name, asm_str)                                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;                             \
    uint64_t operand_source = 0x0F0F0F0F0F0F0F0FULL;                          \
    __asm__ volatile("" : "+r"(operand_source));                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
        __asm__ volatile(asm_str : "+r"(accumulator) : "r"(operand_source));   \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * THROUGHPUT_BMI_TERNARY: 8 independent BMI ternary streams.
 */
#define THROUGHPUT_BMI_TERNARY(func_name, asm_str)                              \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t stream_0 = 0x0123456789ABCDEFULL;                                \
    uint64_t stream_1 = 0x1122334455667788ULL;                                \
    uint64_t stream_2 = 0x2233445566778899ULL;                                \
    uint64_t stream_3 = 0x33445566778899AAULL;                                \
    uint64_t stream_4 = 0x445566778899AABBULL;                                \
    uint64_t stream_5 = 0x5566778899AABBCCULL;                                \
    uint64_t stream_6 = 0x66778899AABBCCDDULL;                                \
    uint64_t stream_7 = 0x778899AABBCCDDEULL;                                 \
    uint64_t operand_source = 0x0F0F0F0F0F0F0F0FULL;                          \
    __asm__ volatile("" : "+r"(operand_source));                               \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(stream_0) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_1) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_2) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_3) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_4) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_5) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_6) : "r"(operand_source));      \
        __asm__ volatile(asm_str : "+r"(stream_7) : "r"(operand_source));      \
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),                 \
                              "+r"(stream_2), "+r"(stream_3),                 \
                              "+r"(stream_4), "+r"(stream_5),                 \
                              "+r"(stream_6), "+r"(stream_7));                \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/*
 * LATENCY_IMM_UNARY: instruction with an immediate operand that reads/writes
 * the same register, e.g., "add $1, %0", "shl $7, %0"
 */
#define LATENCY_IMM_UNARY(func_name, asm_str)                                   \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;                             \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
        __asm__ volatile(asm_str : "+r"(accumulator));                         \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = accumulator; (void)sink;                          \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * THROUGHPUT_IMM_UNARY: 8 independent streams of an immediate-operand instruction.
 */
#define THROUGHPUT_IMM_UNARY(func_name, asm_str)                                \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t stream_0 = 0x0123456789ABCDEFULL;                                \
    uint64_t stream_1 = 0x1122334455667788ULL;                                \
    uint64_t stream_2 = 0x2233445566778899ULL;                                \
    uint64_t stream_3 = 0x33445566778899AAULL;                                \
    uint64_t stream_4 = 0x445566778899AABBULL;                                \
    uint64_t stream_5 = 0x5566778899AABBCCULL;                                \
    uint64_t stream_6 = 0x66778899AABBCCDDULL;                                \
    uint64_t stream_7 = 0x778899AABBCCDDEULL;                                 \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_str : "+r"(stream_0));                            \
        __asm__ volatile(asm_str : "+r"(stream_1));                            \
        __asm__ volatile(asm_str : "+r"(stream_2));                            \
        __asm__ volatile(asm_str : "+r"(stream_3));                            \
        __asm__ volatile(asm_str : "+r"(stream_4));                            \
        __asm__ volatile(asm_str : "+r"(stream_5));                            \
        __asm__ volatile(asm_str : "+r"(stream_6));                            \
        __asm__ volatile(asm_str : "+r"(stream_7));                            \
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),                 \
                              "+r"(stream_2), "+r"(stream_3),                 \
                              "+r"(stream_4), "+r"(stream_5),                 \
                              "+r"(stream_6), "+r"(stream_7));                \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;                 \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);    \
}

/* ========================================================================
 * MOVE INSTRUCTIONS
 * ======================================================================== */

/* MOV r64,r64 -- likely zero-latency register rename on Zen 4 */
LATENCY_UNARY(mov_r64_r64_lat, "mov %0, %0")
THROUGHPUT_UNARY(mov_r64_r64_tp, "mov %0, %0")

/* MOV r64,imm -- load immediate; for latency, chain through add */
static double mov_r64_imm_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mov $0x12345, %0\n\tadd %0, %0" : "+r"(accumulator));
        __asm__ volatile("mov $0x12345, %0\n\tadd %0, %0" : "+r"(accumulator));
        __asm__ volatile("mov $0x12345, %0\n\tadd %0, %0" : "+r"(accumulator));
        __asm__ volatile("mov $0x12345, %0\n\tadd %0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    /* 2 instructions per unroll slot, report per-pair */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mov_r64_imm_tp(uint64_t iteration_count)
{
    uint64_t stream_0, stream_1, stream_2, stream_3;
    uint64_t stream_4, stream_5, stream_6, stream_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mov $0x12345, %0" : "=r"(stream_0));
        __asm__ volatile("mov $0x23456, %0" : "=r"(stream_1));
        __asm__ volatile("mov $0x34567, %0" : "=r"(stream_2));
        __asm__ volatile("mov $0x45678, %0" : "=r"(stream_3));
        __asm__ volatile("mov $0x56789, %0" : "=r"(stream_4));
        __asm__ volatile("mov $0x6789A, %0" : "=r"(stream_5));
        __asm__ volatile("mov $0x789AB, %0" : "=r"(stream_6));
        __asm__ volatile("mov $0x89ABC, %0" : "=r"(stream_7));
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOV r32,r32 -- zero-extends upper 32 bits */
LATENCY_UNARY(mov_r32_r32_lat, "mov %k0, %k0")
THROUGHPUT_UNARY(mov_r32_r32_tp, "mov %k0, %k0")

/* MOVZX r64,r8 */
static double movzx_r64_r8_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movzbq %b0, %0" : "+r"(accumulator));
        __asm__ volatile("movzbq %b0, %0" : "+r"(accumulator));
        __asm__ volatile("movzbq %b0, %0" : "+r"(accumulator));
        __asm__ volatile("movzbq %b0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_UNARY(movzx_r64_r8_tp, "movzbq %b0, %0")

/* MOVZX r64,r16 */
static double movzx_r64_r16_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movzwq %w0, %0" : "+r"(accumulator));
        __asm__ volatile("movzwq %w0, %0" : "+r"(accumulator));
        __asm__ volatile("movzwq %w0, %0" : "+r"(accumulator));
        __asm__ volatile("movzwq %w0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_UNARY(movzx_r64_r16_tp, "movzwq %w0, %0")

/* MOVSX r64,r8 */
static double movsx_r64_r8_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movsbq %b0, %0" : "+r"(accumulator));
        __asm__ volatile("movsbq %b0, %0" : "+r"(accumulator));
        __asm__ volatile("movsbq %b0, %0" : "+r"(accumulator));
        __asm__ volatile("movsbq %b0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_UNARY(movsx_r64_r8_tp, "movsbq %b0, %0")

/* MOVSX r64,r16 */
static double movsx_r64_r16_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movswq %w0, %0" : "+r"(accumulator));
        __asm__ volatile("movswq %w0, %0" : "+r"(accumulator));
        __asm__ volatile("movswq %w0, %0" : "+r"(accumulator));
        __asm__ volatile("movswq %w0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_UNARY(movsx_r64_r16_tp, "movswq %w0, %0")

/* MOVSXD r64,r32 -- AT&T mnemonic is movslq */
LATENCY_UNARY(movsxd_r64_r32_lat, "movslq %k0, %0")
THROUGHPUT_UNARY(movsxd_r64_r32_tp, "movslq %k0, %0")

/* CMOVcc r64,r64 -- conditional move (CMOVNE used, flags pre-set) */
static double cmovne_r64_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t alternate_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(alternate_source));
    /* Set ZF=0 so CMOVNE always fires */
    __asm__ volatile("test %0, %0" : : "r"(accumulator) : "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmovne %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovne %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovne %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovne %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(cmovne_r64_r64_tp, "cmovne %1, %0")

/* LEA r64, [r64+r64] -- 2-component */
static double lea_2comp_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lea (%0,%0), %0" : "+r"(accumulator));
        __asm__ volatile("lea (%0,%0), %0" : "+r"(accumulator));
        __asm__ volatile("lea (%0,%0), %0" : "+r"(accumulator));
        __asm__ volatile("lea (%0,%0), %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_UNARY(lea_2comp_tp, "lea (%0,%0), %0")

/* LEA r64, [r64+r64*2+imm] -- 3-component (scale+index+disp) */
static double lea_3comp_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0x100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lea 0x10(%0,%0,2), %0" : "+r"(accumulator));
        __asm__ volatile("lea 0x10(%0,%0,2), %0" : "+r"(accumulator));
        __asm__ volatile("lea 0x10(%0,%0,2), %0" : "+r"(accumulator));
        __asm__ volatile("lea 0x10(%0,%0,2), %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_UNARY(lea_3comp_tp, "lea 0x10(%0,%0,2), %0")

/* XCHG r64,r64 -- implicit lock prefix on memory; register form may still be slow */
static double xchg_r64_r64_lat(uint64_t iteration_count)
{
    uint64_t register_a = 0xDEADBEEFCAFEBABEULL;
    uint64_t register_b = 0x1111111111111111ULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("xchg %0, %1" : "+r"(register_a), "+r"(register_b));
        __asm__ volatile("xchg %0, %1" : "+r"(register_a), "+r"(register_b));
        __asm__ volatile("xchg %0, %1" : "+r"(register_a), "+r"(register_b));
        __asm__ volatile("xchg %0, %1" : "+r"(register_a), "+r"(register_b));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = register_a ^ register_b; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double xchg_r64_r64_tp(uint64_t iteration_count)
{
    uint64_t pair_a0 = 1, pair_b0 = 2;
    uint64_t pair_a1 = 3, pair_b1 = 4;
    uint64_t pair_a2 = 5, pair_b2 = 6;
    uint64_t pair_a3 = 7, pair_b3 = 8;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a0), "+r"(pair_b0));
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a1), "+r"(pair_b1));
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a2), "+r"(pair_b2));
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a3), "+r"(pair_b3));
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a0), "+r"(pair_b0));
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a1), "+r"(pair_b1));
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a2), "+r"(pair_b2));
        __asm__ volatile("xchg %0, %1" : "+r"(pair_a3), "+r"(pair_b3));
        __asm__ volatile("" : "+r"(pair_a0), "+r"(pair_b0),
                              "+r"(pair_a1), "+r"(pair_b1),
                              "+r"(pair_a2), "+r"(pair_b2),
                              "+r"(pair_a3), "+r"(pair_b3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = pair_a0 ^ pair_a2; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * ARITHMETIC INSTRUCTIONS
 * ======================================================================== */

/* ADD r64,r64 */
LATENCY_BINARY_REG(add_r64_r64_lat, "add %1, %0")
THROUGHPUT_BINARY_REG(add_r64_r64_tp, "add %1, %0")

/* ADD r64,imm */
LATENCY_IMM_UNARY(add_r64_imm_lat, "add $1, %0")
THROUGHPUT_IMM_UNARY(add_r64_imm_tp, "add $1, %0")

/* SUB r64,r64 */
LATENCY_BINARY_REG(sub_r64_r64_lat, "sub %1, %0")
THROUGHPUT_BINARY_REG(sub_r64_r64_tp, "sub %1, %0")

/* SUB r64,imm */
LATENCY_IMM_UNARY(sub_r64_imm_lat, "sub $1, %0")
THROUGHPUT_IMM_UNARY(sub_r64_imm_tp, "sub $1, %0")

/* ADC r64,r64 -- add with carry: needs CF propagation */
LATENCY_BINARY_REG(adc_r64_r64_lat, "adc %1, %0")
THROUGHPUT_BINARY_REG(adc_r64_r64_tp, "adc %1, %0")

/* SBB r64,r64 -- subtract with borrow */
LATENCY_BINARY_REG(sbb_r64_r64_lat, "sbb %1, %0")
THROUGHPUT_BINARY_REG(sbb_r64_r64_tp, "sbb %1, %0")

/* CMP r64,r64 -- output is flags only, chain through SETcc back to reg */
static double cmp_r64_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t compare_target = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(compare_target));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* cmp -> setne -> movzx forms a dependency chain via flags */
        __asm__ volatile("cmp %1, %0\n\tsetne %b0\n\tmovzbq %b0, %0"
                         : "+r"(accumulator) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0\n\tsetne %b0\n\tmovzbq %b0, %0"
                         : "+r"(accumulator) : "r"(compare_target) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    /* 3 ops per slot, 2 slots: report total/2 as the combined latency */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

static double cmp_r64_r64_tp(uint64_t iteration_count)
{
    uint64_t stream_0 = 0x0123456789ABCDEFULL;
    uint64_t stream_1 = 0x1122334455667788ULL;
    uint64_t stream_2 = 0x2233445566778899ULL;
    uint64_t stream_3 = 0x33445566778899AAULL;
    uint64_t stream_4 = 0x445566778899AABBULL;
    uint64_t stream_5 = 0x5566778899AABBCCULL;
    uint64_t stream_6 = 0x66778899AABBCCDDULL;
    uint64_t stream_7 = 0x778899AABBCCDDEULL;
    uint64_t compare_target = 0;
    __asm__ volatile("" : "+r"(compare_target));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmp %1, %0" : "+r"(stream_0) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0" : "+r"(stream_1) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0" : "+r"(stream_2) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0" : "+r"(stream_3) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0" : "+r"(stream_4) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0" : "+r"(stream_5) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0" : "+r"(stream_6) : "r"(compare_target) : "cc");
        __asm__ volatile("cmp %1, %0" : "+r"(stream_7) : "r"(compare_target) : "cc");
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* INC r64 */
LATENCY_UNARY(inc_r64_lat, "inc %0")
THROUGHPUT_UNARY(inc_r64_tp, "inc %0")

/* DEC r64 */
LATENCY_UNARY(dec_r64_lat, "dec %0")
THROUGHPUT_UNARY(dec_r64_tp, "dec %0")

/* NEG r64 */
LATENCY_UNARY(neg_r64_lat, "neg %0")
THROUGHPUT_UNARY(neg_r64_tp, "neg %0")

/* IMUL r64,r64 (2-operand form) */
LATENCY_BINARY_REG(imul_r64_r64_lat, "imul %1, %0")
THROUGHPUT_BINARY_REG(imul_r64_r64_tp, "imul %1, %0")

/* IMUL r64,r64,imm (3-operand form) */
static double imul_r64_imm_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("imul $3, %0, %0" : "+r"(accumulator));
        __asm__ volatile("imul $3, %0, %0" : "+r"(accumulator));
        __asm__ volatile("imul $3, %0, %0" : "+r"(accumulator));
        __asm__ volatile("imul $3, %0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_IMM_UNARY(imul_r64_imm_tp, "imul $3, %0, %0")

/* MUL r64 (1-operand: RDX:RAX = RAX * r64) */
static double mul_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 3;
    uint64_t high_result;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mul %2" : "+a"(accumulator), "=d"(high_result) : "r"(accumulator));
        __asm__ volatile("mul %2" : "+a"(accumulator), "=d"(high_result) : "r"(accumulator));
        __asm__ volatile("mul %2" : "+a"(accumulator), "=d"(high_result) : "r"(accumulator));
        __asm__ volatile("mul %2" : "+a"(accumulator), "=d"(high_result) : "r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator ^ high_result; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mul_r64_tp(uint64_t iteration_count)
{
    uint64_t rax_val = 3;
    uint64_t high_result;
    uint64_t multiplier = 5;
    __asm__ volatile("" : "+r"(multiplier));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mul %2" : "+a"(rax_val), "=d"(high_result) : "r"(multiplier));
        __asm__ volatile("" : "+a"(rax_val));
        __asm__ volatile("mul %2" : "+a"(rax_val), "=d"(high_result) : "r"(multiplier));
        __asm__ volatile("" : "+a"(rax_val));
        __asm__ volatile("mul %2" : "+a"(rax_val), "=d"(high_result) : "r"(multiplier));
        __asm__ volatile("" : "+a"(rax_val));
        __asm__ volatile("mul %2" : "+a"(rax_val), "=d"(high_result) : "r"(multiplier));
        __asm__ volatile("" : "+a"(rax_val));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = rax_val ^ high_result; (void)sink;
    /* Only 4 ops due to RAX constraint, so report /4 */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* DIV r64 -- unsigned divide: very slow, microcoded */
static double div_r64_lat(uint64_t iteration_count)
{
    /* Use small values to avoid extremely long divisions */
    uint64_t rax_val = 100;
    uint64_t rdx_val = 0;
    uint64_t divisor = 3;
    __asm__ volatile("" : "+r"(divisor));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("div %2" : "+a"(rax_val), "+d"(rdx_val) : "r"(divisor));
        /* Reset to avoid divide overflow on next iteration */
        __asm__ volatile("xor %0, %0" : "=r"(rdx_val));
        __asm__ volatile("or $100, %0" : "+a"(rax_val));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = rax_val; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* IDIV r64 -- signed divide */
static double idiv_r64_lat(uint64_t iteration_count)
{
    int64_t rax_val = 100;
    int64_t rdx_val = 0;
    int64_t divisor = 3;
    __asm__ volatile("" : "+r"(divisor));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("idiv %2" : "+a"(rax_val), "+d"(rdx_val) : "r"(divisor));
        __asm__ volatile("xor %0, %0" : "=r"(rdx_val));
        __asm__ volatile("or $100, %0" : "+a"(rax_val));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int64_t sink = rax_val; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* ========================================================================
 * LOGIC INSTRUCTIONS
 * ======================================================================== */

/* AND r64,r64 */
LATENCY_BINARY_REG(and_r64_r64_lat, "and %1, %0")
THROUGHPUT_BINARY_REG(and_r64_r64_tp, "and %1, %0")

/* AND r64,imm */
LATENCY_IMM_UNARY(and_r64_imm_lat, "and $0x7FFFFFFF, %0")
THROUGHPUT_IMM_UNARY(and_r64_imm_tp, "and $0x7FFFFFFF, %0")

/* OR r64,r64 */
LATENCY_BINARY_REG(or_r64_r64_lat, "or %1, %0")
THROUGHPUT_BINARY_REG(or_r64_r64_tp, "or %1, %0")

/* OR r64,imm */
LATENCY_IMM_UNARY(or_r64_imm_lat, "or $1, %0")
THROUGHPUT_IMM_UNARY(or_r64_imm_tp, "or $1, %0")

/* XOR r64,r64 */
LATENCY_BINARY_REG(xor_r64_r64_lat, "xor %1, %0")
THROUGHPUT_BINARY_REG(xor_r64_r64_tp, "xor %1, %0")

/* XOR r64,imm */
LATENCY_IMM_UNARY(xor_r64_imm_lat, "xor $1, %0")
THROUGHPUT_IMM_UNARY(xor_r64_imm_tp, "xor $1, %0")

/* NOT r64 */
LATENCY_UNARY(not_r64_lat, "not %0")
THROUGHPUT_UNARY(not_r64_tp, "not %0")

/* TEST r64,r64 -- flags only, like CMP */
static double test_r64_r64_tp(uint64_t iteration_count)
{
    uint64_t stream_0 = 0x0123456789ABCDEFULL;
    uint64_t stream_1 = 0x1122334455667788ULL;
    uint64_t stream_2 = 0x2233445566778899ULL;
    uint64_t stream_3 = 0x33445566778899AAULL;
    uint64_t stream_4 = 0x445566778899AABBULL;
    uint64_t stream_5 = 0x5566778899AABBCCULL;
    uint64_t stream_6 = 0x66778899AABBCCDDULL;
    uint64_t stream_7 = 0x778899AABBCCDDEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("test %0, %0" : : "r"(stream_0) : "cc");
        __asm__ volatile("test %0, %0" : : "r"(stream_1) : "cc");
        __asm__ volatile("test %0, %0" : : "r"(stream_2) : "cc");
        __asm__ volatile("test %0, %0" : : "r"(stream_3) : "cc");
        __asm__ volatile("test %0, %0" : : "r"(stream_4) : "cc");
        __asm__ volatile("test %0, %0" : : "r"(stream_5) : "cc");
        __asm__ volatile("test %0, %0" : : "r"(stream_6) : "cc");
        __asm__ volatile("test %0, %0" : : "r"(stream_7) : "cc");
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * SHIFT/ROTATE INSTRUCTIONS
 * ======================================================================== */

/* SHL r64,$1 */
LATENCY_IMM_UNARY(shl_r64_1_lat, "shl $1, %0")
THROUGHPUT_IMM_UNARY(shl_r64_1_tp, "shl $1, %0")

/* SHL r64,$7 */
LATENCY_IMM_UNARY(shl_r64_imm_lat, "shl $7, %0")
THROUGHPUT_IMM_UNARY(shl_r64_imm_tp, "shl $7, %0")

/* SHL r64,CL */
static double shl_r64_cl_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("shl %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(accumulator) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double shl_r64_cl_tp(uint64_t iteration_count)
{
    uint64_t stream_0 = 0x0123456789ABCDEFULL;
    uint64_t stream_1 = 0x1122334455667788ULL;
    uint64_t stream_2 = 0x2233445566778899ULL;
    uint64_t stream_3 = 0x33445566778899AAULL;
    uint64_t stream_4 = 0x445566778899AABBULL;
    uint64_t stream_5 = 0x5566778899AABBCCULL;
    uint64_t stream_6 = 0x66778899AABBCCDDULL;
    uint64_t stream_7 = 0x778899AABBCCDDEULL;
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_0) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_1) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_2) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_3) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_4) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_5) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_6) :: "cc");
        __asm__ volatile("shl %%cl, %0" : "+r"(stream_7) :: "cc");
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* SHR r64,$1 */
LATENCY_IMM_UNARY(shr_r64_1_lat, "shr $1, %0")
THROUGHPUT_IMM_UNARY(shr_r64_1_tp, "shr $1, %0")

/* SHR r64,$7 */
LATENCY_IMM_UNARY(shr_r64_imm_lat, "shr $7, %0")
THROUGHPUT_IMM_UNARY(shr_r64_imm_tp, "shr $7, %0")

/* SHR r64,CL */
static double shr_r64_cl_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("shr %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(accumulator) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double shr_r64_cl_tp(uint64_t iteration_count)
{
    uint64_t stream_0 = 0x0123456789ABCDEFULL;
    uint64_t stream_1 = 0x1122334455667788ULL;
    uint64_t stream_2 = 0x2233445566778899ULL;
    uint64_t stream_3 = 0x33445566778899AAULL;
    uint64_t stream_4 = 0x445566778899AABBULL;
    uint64_t stream_5 = 0x5566778899AABBCCULL;
    uint64_t stream_6 = 0x66778899AABBCCDDULL;
    uint64_t stream_7 = 0x778899AABBCCDDEULL;
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_0) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_1) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_2) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_3) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_4) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_5) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_6) :: "cc");
        __asm__ volatile("shr %%cl, %0" : "+r"(stream_7) :: "cc");
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* SAR r64,$1 */
LATENCY_IMM_UNARY(sar_r64_1_lat, "sar $1, %0")
THROUGHPUT_IMM_UNARY(sar_r64_1_tp, "sar $1, %0")

/* SAR r64,$7 */
LATENCY_IMM_UNARY(sar_r64_imm_lat, "sar $7, %0")
THROUGHPUT_IMM_UNARY(sar_r64_imm_tp, "sar $7, %0")

/* SAR r64,CL */
static double sar_r64_cl_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("sar %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(accumulator) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double sar_r64_cl_tp(uint64_t iteration_count)
{
    uint64_t stream_0 = 0x0123456789ABCDEFULL, stream_1 = 0x1122334455667788ULL;
    uint64_t stream_2 = 0x2233445566778899ULL, stream_3 = 0x33445566778899AAULL;
    uint64_t stream_4 = 0x445566778899AABBULL, stream_5 = 0x5566778899AABBCCULL;
    uint64_t stream_6 = 0x66778899AABBCCDDULL, stream_7 = 0x778899AABBCCDDEULL;
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_0) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_1) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_2) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_3) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_4) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_5) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_6) :: "cc");
        __asm__ volatile("sar %%cl, %0" : "+r"(stream_7) :: "cc");
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ROL r64,$1 */
LATENCY_IMM_UNARY(rol_r64_1_lat, "rol $1, %0")
THROUGHPUT_IMM_UNARY(rol_r64_1_tp, "rol $1, %0")

/* ROL r64,$7 */
LATENCY_IMM_UNARY(rol_r64_imm_lat, "rol $7, %0")
THROUGHPUT_IMM_UNARY(rol_r64_imm_tp, "rol $7, %0")

/* ROL r64,CL */
static double rol_r64_cl_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("rol %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("rol %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("rol %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("rol %%cl, %0" : "+r"(accumulator) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_IMM_UNARY(rol_r64_cl_tp, "rol $3, %0") /* approx CL throughput */

/* ROR r64,$1 */
LATENCY_IMM_UNARY(ror_r64_1_lat, "ror $1, %0")
THROUGHPUT_IMM_UNARY(ror_r64_1_tp, "ror $1, %0")

/* ROR r64,$7 */
LATENCY_IMM_UNARY(ror_r64_imm_lat, "ror $7, %0")
THROUGHPUT_IMM_UNARY(ror_r64_imm_tp, "ror $7, %0")

/* ROR r64,CL */
static double ror_r64_cl_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    __asm__ volatile("mov $3, %%cl" ::: "rcx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("ror %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("ror %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("ror %%cl, %0" : "+r"(accumulator) :: "cc");
        __asm__ volatile("ror %%cl, %0" : "+r"(accumulator) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_IMM_UNARY(ror_r64_cl_tp, "ror $3, %0")

/* RCL r64,$1 -- rotate through carry, potentially slow */
LATENCY_IMM_UNARY(rcl_r64_1_lat, "rcl $1, %0")
THROUGHPUT_IMM_UNARY(rcl_r64_1_tp, "rcl $1, %0")

/* RCR r64,$1 */
LATENCY_IMM_UNARY(rcr_r64_1_lat, "rcr $1, %0")
THROUGHPUT_IMM_UNARY(rcr_r64_1_tp, "rcr $1, %0")

/* SHLD r64,r64,imm */
static double shld_r64_imm_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t shift_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(shift_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("shld $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
        __asm__ volatile("shld $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
        __asm__ volatile("shld $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
        __asm__ volatile("shld $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(shld_r64_imm_tp, "shld $7, %1, %0")

/* SHRD r64,r64,imm */
static double shrd_r64_imm_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t shift_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(shift_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("shrd $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
        __asm__ volatile("shrd $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
        __asm__ volatile("shrd $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
        __asm__ volatile("shrd $7, %1, %0" : "+r"(accumulator) : "r"(shift_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(shrd_r64_imm_tp, "shrd $7, %1, %0")

/* ========================================================================
 * BIT MANIPULATION (BMI1/BMI2/ABM)
 * ======================================================================== */

/* BSF r64,r64 */
LATENCY_UNARY(bsf_r64_lat, "bsf %0, %0")
THROUGHPUT_UNARY(bsf_r64_tp, "bsf %0, %0")

/* BSR r64,r64 */
LATENCY_UNARY(bsr_r64_lat, "bsr %0, %0")
THROUGHPUT_UNARY(bsr_r64_tp, "bsr %0, %0")

/* POPCNT r64,r64 */
LATENCY_UNARY(popcnt_r64_lat, "popcnt %0, %0")
THROUGHPUT_UNARY(popcnt_r64_tp, "popcnt %0, %0")

/* LZCNT r64,r64 */
LATENCY_UNARY(lzcnt_r64_lat, "lzcnt %0, %0")
THROUGHPUT_UNARY(lzcnt_r64_tp, "lzcnt %0, %0")

/* TZCNT r64,r64 */
LATENCY_UNARY(tzcnt_r64_lat, "tzcnt %0, %0")
THROUGHPUT_UNARY(tzcnt_r64_tp, "tzcnt %0, %0")

/* BLSI r64,r64 */
LATENCY_UNARY(blsi_r64_lat, "blsi %0, %0")
THROUGHPUT_UNARY(blsi_r64_tp, "blsi %0, %0")

/* BLSR r64,r64 */
LATENCY_UNARY(blsr_r64_lat, "blsr %0, %0")
THROUGHPUT_UNARY(blsr_r64_tp, "blsr %0, %0")

/* BLSMSK r64,r64 */
LATENCY_UNARY(blsmsk_r64_lat, "blsmsk %0, %0")
THROUGHPUT_UNARY(blsmsk_r64_tp, "blsmsk %0, %0")

/* ANDN r64,r64,r64 (BMI1 ternary) */
LATENCY_BMI_TERNARY(andn_r64_lat, "andn %1, %0, %0")
THROUGHPUT_BMI_TERNARY(andn_r64_tp, "andn %1, %0, %0")

/* BEXTR r64,r64,r64 */
LATENCY_BMI_TERNARY(bextr_r64_lat, "bextr %1, %0, %0")
THROUGHPUT_BMI_TERNARY(bextr_r64_tp, "bextr %1, %0, %0")

/* BZHI r64,r64,r64 */
LATENCY_BMI_TERNARY(bzhi_r64_lat, "bzhi %1, %0, %0")
THROUGHPUT_BMI_TERNARY(bzhi_r64_tp, "bzhi %1, %0, %0")

/* PDEP r64,r64,r64 */
LATENCY_BMI_TERNARY(pdep_r64_lat, "pdep %1, %0, %0")
THROUGHPUT_BMI_TERNARY(pdep_r64_tp, "pdep %1, %0, %0")

/* PEXT r64,r64,r64 */
LATENCY_BMI_TERNARY(pext_r64_lat, "pext %1, %0, %0")
THROUGHPUT_BMI_TERNARY(pext_r64_tp, "pext %1, %0, %0")

/* SARX r64,r64,r64 */
LATENCY_BMI_TERNARY(sarx_r64_lat, "sarx %1, %0, %0")
THROUGHPUT_BMI_TERNARY(sarx_r64_tp, "sarx %1, %0, %0")

/* SHLX r64,r64,r64 */
LATENCY_BMI_TERNARY(shlx_r64_lat, "shlx %1, %0, %0")
THROUGHPUT_BMI_TERNARY(shlx_r64_tp, "shlx %1, %0, %0")

/* SHRX r64,r64,r64 */
LATENCY_BMI_TERNARY(shrx_r64_lat, "shrx %1, %0, %0")
THROUGHPUT_BMI_TERNARY(shrx_r64_tp, "shrx %1, %0, %0")

/* RORX r64,r64,imm */
LATENCY_IMM_UNARY(rorx_r64_lat, "rorx $17, %0, %0")
THROUGHPUT_IMM_UNARY(rorx_r64_tp, "rorx $17, %0, %0")

/* MULX r64,r64,r64 -- uses implicit RDX */
static double mulx_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t high_half;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator ^ high_half; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mulx_r64_tp(uint64_t iteration_count)
{
    uint64_t rdx_input = 0xDEADBEEFCAFEBABEULL;
    uint64_t low_result, high_result;
    uint64_t multiplier_source_0 = 0x0123456789ABCDEFULL;
    uint64_t multiplier_source_1 = 0x1122334455667788ULL;
    uint64_t multiplier_source_2 = 0x2233445566778899ULL;
    uint64_t multiplier_source_3 = 0x33445566778899AAULL;
    __asm__ volatile("" : "+r"(multiplier_source_0), "+r"(multiplier_source_1),
                          "+r"(multiplier_source_2), "+r"(multiplier_source_3));
    __asm__ volatile("mov %0, %%rdx" : : "r"(rdx_input) : "rdx");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mulx %2, %0, %1" : "=r"(low_result), "=r"(high_result)
                         : "r"(multiplier_source_0) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(low_result), "=r"(high_result)
                         : "r"(multiplier_source_1) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(low_result), "=r"(high_result)
                         : "r"(multiplier_source_2) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(low_result), "=r"(high_result)
                         : "r"(multiplier_source_3) : "rdx");
        __asm__ volatile("" : "+r"(low_result), "+r"(high_result));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = low_result ^ high_result; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* BT r64,r64 -- bit test, output in CF */
static double bt_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t bit_index = 5;
    __asm__ volatile("" : "+r"(bit_index));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("bt %1, %0\n\tadc $0, %0"
                         : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0\n\tadc $0, %0"
                         : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0\n\tadc $0, %0"
                         : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0\n\tadc $0, %0"
                         : "+r"(accumulator) : "r"(bit_index) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double bt_r64_tp(uint64_t iteration_count)
{
    uint64_t stream_0 = 0x0123456789ABCDEFULL, stream_1 = 0x1122334455667788ULL;
    uint64_t stream_2 = 0x2233445566778899ULL, stream_3 = 0x33445566778899AAULL;
    uint64_t stream_4 = 0x445566778899AABBULL, stream_5 = 0x5566778899AABBCCULL;
    uint64_t stream_6 = 0x66778899AABBCCDDULL, stream_7 = 0x778899AABBCCDDEULL;
    uint64_t bit_index = 5;
    __asm__ volatile("" : "+r"(bit_index));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("bt %1, %0" : : "r"(stream_0), "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0" : : "r"(stream_1), "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0" : : "r"(stream_2), "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0" : : "r"(stream_3), "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0" : : "r"(stream_4), "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0" : : "r"(stream_5), "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0" : : "r"(stream_6), "r"(bit_index) : "cc");
        __asm__ volatile("bt %1, %0" : : "r"(stream_7), "r"(bit_index) : "cc");
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* BTS r64,r64 -- bit test and set */
static double bts_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t bit_index = 5;
    __asm__ volatile("" : "+r"(bit_index));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("bts %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("bts %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("bts %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("bts %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(bts_r64_tp, "bts %1, %0")

/* BTR r64,r64 -- bit test and reset */
static double btr_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t bit_index = 5;
    __asm__ volatile("" : "+r"(bit_index));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("btr %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("btr %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("btr %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("btr %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(btr_r64_tp, "btr %1, %0")

/* BTC r64,r64 -- bit test and complement */
static double btc_r64_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t bit_index = 5;
    __asm__ volatile("" : "+r"(bit_index));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("btc %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("btc %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("btc %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
        __asm__ volatile("btc %1, %0" : "+r"(accumulator) : "r"(bit_index) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(btc_r64_tp, "btc %1, %0")

/* ========================================================================
 * FLAG MANIPULATION
 * ======================================================================== */

/* SETcc (SETNE) -- set byte from flags */
static double setne_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* chain: test -> setne -> test -> setne */
        __asm__ volatile("test %0, %0\n\tsetne %b0\n\tmovzbq %b0, %0"
                         : "+r"(accumulator) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0\n\tmovzbq %b0, %0"
                         : "+r"(accumulator) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0\n\tmovzbq %b0, %0"
                         : "+r"(accumulator) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0\n\tmovzbq %b0, %0"
                         : "+r"(accumulator) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double setne_tp(uint64_t iteration_count)
{
    uint64_t stream_0 = 1, stream_1 = 2, stream_2 = 3, stream_3 = 4;
    uint64_t stream_4 = 5, stream_5 = 6, stream_6 = 7, stream_7 = 8;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_0) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_1) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_2) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_3) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_4) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_5) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_6) :: "cc");
        __asm__ volatile("test %0, %0\n\tsetne %b0" : "+r"(stream_7) :: "cc");
        __asm__ volatile("" : "+r"(stream_0), "+r"(stream_1),
                              "+r"(stream_2), "+r"(stream_3),
                              "+r"(stream_4), "+r"(stream_5),
                              "+r"(stream_6), "+r"(stream_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = stream_0 ^ stream_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* LAHF -- load AH from flags.
 * Cannot use movzbq %%ah with REX prefix; extract AH via shr $8. */
static double lahf_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("test %0, %0\n\tlahf\n\tshr $8, %0\n\tand $0xFF, %0"
                         : "+a"(accumulator) :: "cc");
        __asm__ volatile("test %0, %0\n\tlahf\n\tshr $8, %0\n\tand $0xFF, %0"
                         : "+a"(accumulator) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* SAHF -- store AH to flags */
static double sahf_tp(uint64_t iteration_count)
{
    uint64_t rax_val = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
        __asm__ volatile("sahf" : : "a"(rax_val) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CLC / STC / CMC throughput (flag-only) */
static double clc_stc_cmc_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("clc\n\tstc\n\tcmc\n\tclc\n\tstc\n\tcmc\n\tclc\n\tstc" ::: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * CONVERSION INSTRUCTIONS
 * ======================================================================== */

/* CBW -- sign-extend AL to AX */
static double cbw_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cbw" : "+a"(accumulator));
        __asm__ volatile("cbw" : "+a"(accumulator));
        __asm__ volatile("cbw" : "+a"(accumulator));
        __asm__ volatile("cbw" : "+a"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* CWDE -- sign-extend AX to EAX */
static double cwde_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cwde" : "+a"(accumulator));
        __asm__ volatile("cwde" : "+a"(accumulator));
        __asm__ volatile("cwde" : "+a"(accumulator));
        __asm__ volatile("cwde" : "+a"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* CDQE -- sign-extend EAX to RAX */
static double cdqe_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cdqe" : "+a"(accumulator));
        __asm__ volatile("cdqe" : "+a"(accumulator));
        __asm__ volatile("cdqe" : "+a"(accumulator));
        __asm__ volatile("cdqe" : "+a"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* CQO -- sign-extend RAX into RDX:RAX */
static double cqo_lat(uint64_t iteration_count)
{
    uint64_t rax_val = 42;
    uint64_t rdx_val;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cqo" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cqo" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cqo" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cqo" : "+a"(rax_val), "=d"(rdx_val));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = rax_val ^ rdx_val; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* CDQ -- sign-extend EAX into EDX:EAX */
static double cdq_lat(uint64_t iteration_count)
{
    uint64_t rax_val = 42;
    uint64_t rdx_val;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cdq" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cdq" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cdq" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cdq" : "+a"(rax_val), "=d"(rdx_val));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = rax_val ^ rdx_val; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* CWD -- sign-extend AX into DX:AX */
static double cwd_lat(uint64_t iteration_count)
{
    uint64_t rax_val = 42;
    uint64_t rdx_val;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cwd" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cwd" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cwd" : "+a"(rax_val), "=d"(rdx_val));
        __asm__ volatile("cwd" : "+a"(rax_val), "=d"(rdx_val));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = rax_val ^ rdx_val; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* BSWAP r64 */
LATENCY_UNARY(bswap_r64_lat, "bswap %0")
THROUGHPUT_UNARY(bswap_r64_tp, "bswap %0")

/* BSWAP r32 */
LATENCY_UNARY(bswap_r32_lat, "bswap %k0")
THROUGHPUT_UNARY(bswap_r32_tp, "bswap %k0")

/* ========================================================================
 * MISC / NOP / FENCE
 * ======================================================================== */

/* NOP (1-byte) */
static double nop_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* NOP 2-byte (0x66 0x90) */
static double nop2_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            ".byte 0x66,0x90\n\t.byte 0x66,0x90\n\t"
            ".byte 0x66,0x90\n\t.byte 0x66,0x90\n\t"
            ".byte 0x66,0x90\n\t.byte 0x66,0x90\n\t"
            ".byte 0x66,0x90\n\t.byte 0x66,0x90"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* NOP 3-byte (0x0F 0x1F 0x00) */
static double nop3_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            ".byte 0x0F,0x1F,0x00\n\t.byte 0x0F,0x1F,0x00\n\t"
            ".byte 0x0F,0x1F,0x00\n\t.byte 0x0F,0x1F,0x00\n\t"
            ".byte 0x0F,0x1F,0x00\n\t.byte 0x0F,0x1F,0x00\n\t"
            ".byte 0x0F,0x1F,0x00\n\t.byte 0x0F,0x1F,0x00"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* LFENCE throughput */
static double lfence_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lfence\n\tlfence\n\tlfence\n\tlfence");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* SFENCE throughput */
static double sfence_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("sfence\n\tsfence\n\tsfence\n\tsfence");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* MFENCE throughput */
static double mfence_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mfence\n\tmfence\n\tmfence\n\tmfence");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* RDTSC throughput (serializing) */
static double rdtsc_tp(uint64_t iteration_count)
{
    uint64_t low_bits, high_bits;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("rdtsc" : "=a"(low_bits), "=d"(high_bits));
        __asm__ volatile("rdtsc" : "=a"(low_bits), "=d"(high_bits));
        __asm__ volatile("rdtsc" : "=a"(low_bits), "=d"(high_bits));
        __asm__ volatile("rdtsc" : "=a"(low_bits), "=d"(high_bits));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = low_bits ^ high_bits; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* RDTSCP throughput */
static double rdtscp_tp(uint64_t iteration_count)
{
    uint64_t low_bits, high_bits;
    unsigned int aux_val;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("rdtscp" : "=a"(low_bits), "=d"(high_bits), "=c"(aux_val));
        __asm__ volatile("rdtscp" : "=a"(low_bits), "=d"(high_bits), "=c"(aux_val));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = low_bits ^ high_bits; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* CPUID throughput -- very slow, serializing */
static double cpuid_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 100; /* CPUID is extremely slow */
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint32_t eax_val, ebx_val, ecx_val, edx_val;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("cpuid" : "=a"(eax_val), "=b"(ebx_val),
                         "=c"(ecx_val), "=d"(edx_val) : "a"(0));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = eax_val; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* RDRAND r64 */
static double rdrand_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t random_value;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("rdrand %0" : "=r"(random_value) :: "cc");
        __asm__ volatile("rdrand %0" : "=r"(random_value) :: "cc");
        __asm__ volatile("rdrand %0" : "=r"(random_value) :: "cc");
        __asm__ volatile("rdrand %0" : "=r"(random_value) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = random_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* RDSEED r64 */
static double rdseed_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t seed_value;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("rdseed %0" : "=r"(seed_value) :: "cc");
        __asm__ volatile("rdseed %0" : "=r"(seed_value) :: "cc");
        __asm__ volatile("rdseed %0" : "=r"(seed_value) :: "cc");
        __asm__ volatile("rdseed %0" : "=r"(seed_value) :: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = seed_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* PUSH/POP pair throughput -- uses the stack */
static double push_pop_tp(uint64_t iteration_count)
{
    uint64_t register_val = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "push %0\n\tpop %0\n\tpush %0\n\tpop %0\n\t"
            "push %0\n\tpop %0\n\tpush %0\n\tpop %0"
            : "+r"(register_val) :: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = register_val; (void)sink;
    /* 4 push/pop pairs = 8 individual ops */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * MEASUREMENT ENTRY TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} InstructionProbeEntry;

static const InstructionProbeEntry probe_table[] = {
    /* --- Move --- */
    {"MOV",     "r64,r64",       "move",    mov_r64_r64_lat,      mov_r64_r64_tp},
    {"MOV",     "r64,imm",       "move",    mov_r64_imm_lat,      mov_r64_imm_tp},
    {"MOV",     "r32,r32",       "move",    mov_r32_r32_lat,      mov_r32_r32_tp},
    {"MOVZX",   "r64,r8",        "move",    movzx_r64_r8_lat,     movzx_r64_r8_tp},
    {"MOVZX",   "r64,r16",       "move",    movzx_r64_r16_lat,    movzx_r64_r16_tp},
    {"MOVSX",   "r64,r8",        "move",    movsx_r64_r8_lat,     movsx_r64_r8_tp},
    {"MOVSX",   "r64,r16",       "move",    movsx_r64_r16_lat,    movsx_r64_r16_tp},
    {"MOVSXD",  "r64,r32",       "move",    movsxd_r64_r32_lat,   movsxd_r64_r32_tp},
    {"CMOVNE",  "r64,r64",       "move",    cmovne_r64_r64_lat,   cmovne_r64_r64_tp},
    {"LEA",     "[r+r]",         "move",    lea_2comp_lat,         lea_2comp_tp},
    {"LEA",     "[r+r*s+d]",     "move",    lea_3comp_lat,         lea_3comp_tp},
    {"XCHG",    "r64,r64",       "move",    xchg_r64_r64_lat,     xchg_r64_r64_tp},
    {"PUSH/POP","r64",           "move",    NULL,                  push_pop_tp},

    /* --- Arithmetic --- */
    {"ADD",     "r64,r64",       "arith",   add_r64_r64_lat,      add_r64_r64_tp},
    {"ADD",     "r64,imm",       "arith",   add_r64_imm_lat,      add_r64_imm_tp},
    {"SUB",     "r64,r64",       "arith",   sub_r64_r64_lat,      sub_r64_r64_tp},
    {"SUB",     "r64,imm",       "arith",   sub_r64_imm_lat,      sub_r64_imm_tp},
    {"ADC",     "r64,r64",       "arith",   adc_r64_r64_lat,      adc_r64_r64_tp},
    {"SBB",     "r64,r64",       "arith",   sbb_r64_r64_lat,      sbb_r64_r64_tp},
    {"CMP",     "r64,r64",       "arith",   cmp_r64_r64_lat,      cmp_r64_r64_tp},
    {"INC",     "r64",           "arith",   inc_r64_lat,           inc_r64_tp},
    {"DEC",     "r64",           "arith",   dec_r64_lat,           dec_r64_tp},
    {"NEG",     "r64",           "arith",   neg_r64_lat,           neg_r64_tp},
    {"IMUL",    "r64,r64",       "arith",   imul_r64_r64_lat,     imul_r64_r64_tp},
    {"IMUL",    "r64,r64,imm",   "arith",   imul_r64_imm_lat,     imul_r64_imm_tp},
    {"MUL",     "r64",           "arith",   mul_r64_lat,           mul_r64_tp},
    {"DIV",     "r64",           "arith",   div_r64_lat,           NULL},
    {"IDIV",    "r64",           "arith",   idiv_r64_lat,          NULL},

    /* --- Logic --- */
    {"AND",     "r64,r64",       "logic",   and_r64_r64_lat,      and_r64_r64_tp},
    {"AND",     "r64,imm",       "logic",   and_r64_imm_lat,      and_r64_imm_tp},
    {"OR",      "r64,r64",       "logic",   or_r64_r64_lat,       or_r64_r64_tp},
    {"OR",      "r64,imm",       "logic",   or_r64_imm_lat,       or_r64_imm_tp},
    {"XOR",     "r64,r64",       "logic",   xor_r64_r64_lat,      xor_r64_r64_tp},
    {"XOR",     "r64,imm",       "logic",   xor_r64_imm_lat,      xor_r64_imm_tp},
    {"NOT",     "r64",           "logic",   not_r64_lat,           not_r64_tp},
    {"TEST",    "r64,r64",       "logic",   NULL,                  test_r64_r64_tp},

    /* --- Shift --- */
    {"SHL",     "r64,$1",        "shift",   shl_r64_1_lat,        shl_r64_1_tp},
    {"SHL",     "r64,$7",        "shift",   shl_r64_imm_lat,      shl_r64_imm_tp},
    {"SHL",     "r64,CL",        "shift",   shl_r64_cl_lat,       shl_r64_cl_tp},
    {"SHR",     "r64,$1",        "shift",   shr_r64_1_lat,        shr_r64_1_tp},
    {"SHR",     "r64,$7",        "shift",   shr_r64_imm_lat,      shr_r64_imm_tp},
    {"SHR",     "r64,CL",        "shift",   shr_r64_cl_lat,       shr_r64_cl_tp},
    {"SAR",     "r64,$1",        "shift",   sar_r64_1_lat,        sar_r64_1_tp},
    {"SAR",     "r64,$7",        "shift",   sar_r64_imm_lat,      sar_r64_imm_tp},
    {"SAR",     "r64,CL",        "shift",   sar_r64_cl_lat,       sar_r64_cl_tp},
    {"ROL",     "r64,$1",        "shift",   rol_r64_1_lat,        rol_r64_1_tp},
    {"ROL",     "r64,$7",        "shift",   rol_r64_imm_lat,      rol_r64_imm_tp},
    {"ROL",     "r64,CL",        "shift",   rol_r64_cl_lat,       rol_r64_cl_tp},
    {"ROR",     "r64,$1",        "shift",   ror_r64_1_lat,        ror_r64_1_tp},
    {"ROR",     "r64,$7",        "shift",   ror_r64_imm_lat,      ror_r64_imm_tp},
    {"ROR",     "r64,CL",        "shift",   ror_r64_cl_lat,       ror_r64_cl_tp},
    {"RCL",     "r64,$1",        "shift",   rcl_r64_1_lat,        rcl_r64_1_tp},
    {"RCR",     "r64,$1",        "shift",   rcr_r64_1_lat,        rcr_r64_1_tp},
    {"SHLD",    "r64,r64,$7",    "shift",   shld_r64_imm_lat,     shld_r64_imm_tp},
    {"SHRD",    "r64,r64,$7",    "shift",   shrd_r64_imm_lat,     shrd_r64_imm_tp},

    /* --- Bit manipulation (BMI) --- */
    {"BSF",     "r64,r64",       "bit",     bsf_r64_lat,          bsf_r64_tp},
    {"BSR",     "r64,r64",       "bit",     bsr_r64_lat,          bsr_r64_tp},
    {"POPCNT",  "r64,r64",       "bit",     popcnt_r64_lat,       popcnt_r64_tp},
    {"LZCNT",   "r64,r64",       "bit",     lzcnt_r64_lat,        lzcnt_r64_tp},
    {"TZCNT",   "r64,r64",       "bit",     tzcnt_r64_lat,        tzcnt_r64_tp},
    {"BLSI",    "r64,r64",       "bit",     blsi_r64_lat,         blsi_r64_tp},
    {"BLSR",    "r64,r64",       "bit",     blsr_r64_lat,         blsr_r64_tp},
    {"BLSMSK",  "r64,r64",       "bit",     blsmsk_r64_lat,       blsmsk_r64_tp},
    {"ANDN",    "r64,r64,r64",   "bit",     andn_r64_lat,         andn_r64_tp},
    {"BEXTR",   "r64,r64,r64",   "bit",     bextr_r64_lat,        bextr_r64_tp},
    {"BZHI",    "r64,r64,r64",   "bit",     bzhi_r64_lat,         bzhi_r64_tp},
    {"PDEP",    "r64,r64,r64",   "bit",     pdep_r64_lat,         pdep_r64_tp},
    {"PEXT",    "r64,r64,r64",   "bit",     pext_r64_lat,         pext_r64_tp},
    {"SARX",    "r64,r64,r64",   "bit",     sarx_r64_lat,         sarx_r64_tp},
    {"SHLX",    "r64,r64,r64",   "bit",     shlx_r64_lat,         shlx_r64_tp},
    {"SHRX",    "r64,r64,r64",   "bit",     shrx_r64_lat,         shrx_r64_tp},
    {"RORX",    "r64,r64,$17",   "bit",     rorx_r64_lat,         rorx_r64_tp},
    {"MULX",    "r64,r64,r64",   "bit",     mulx_r64_lat,         mulx_r64_tp},
    {"BT",      "r64,r64",       "bit",     bt_r64_lat,           bt_r64_tp},
    {"BTS",     "r64,r64",       "bit",     bts_r64_lat,          bts_r64_tp},
    {"BTR",     "r64,r64",       "bit",     btr_r64_lat,          btr_r64_tp},
    {"BTC",     "r64,r64",       "bit",     btc_r64_lat,          btc_r64_tp},

    /* --- Flags --- */
    {"SETNE",   "r8",            "flag",    setne_lat,             setne_tp},
    {"LAHF",    "",              "flag",    lahf_lat,              NULL},
    {"SAHF",    "",              "flag",    NULL,                  sahf_tp},
    {"CLC/STC/CMC","",           "flag",    NULL,                  clc_stc_cmc_tp},

    /* --- Conversion --- */
    {"CBW",     "",              "convert", cbw_lat,               NULL},
    {"CWDE",    "",              "convert", cwde_lat,              NULL},
    {"CDQE",    "",              "convert", cdqe_lat,              NULL},
    {"CQO",     "",              "convert", cqo_lat,               NULL},
    {"CDQ",     "",              "convert", cdq_lat,               NULL},
    {"CWD",     "",              "convert", cwd_lat,               NULL},
    {"BSWAP",   "r64",           "convert", bswap_r64_lat,         bswap_r64_tp},
    {"BSWAP",   "r32",           "convert", bswap_r32_lat,         bswap_r32_tp},

    /* --- Misc / NOP / Fence / System --- */
    {"NOP",     "1-byte",        "misc",    NULL,                  nop_tp},
    {"NOP",     "2-byte",        "misc",    NULL,                  nop2_tp},
    {"NOP",     "3-byte",        "misc",    NULL,                  nop3_tp},
    {"LFENCE",  "",              "misc",    NULL,                  lfence_tp},
    {"SFENCE",  "",              "misc",    NULL,                  sfence_tp},
    {"MFENCE",  "",              "misc",    NULL,                  mfence_tp},
    {"RDTSC",   "",              "misc",    NULL,                  rdtsc_tp},
    {"RDTSCP",  "",              "misc",    NULL,                  rdtscp_tp},
    {"CPUID",   "",              "misc",    NULL,                  cpuid_tp},
    {"RDRAND",  "r64",           "misc",    NULL,                  rdrand_tp},
    {"RDSEED",  "r64",           "misc",    NULL,                  rdseed_tp},
    {"PUSH/POP","r64",           "misc",    NULL,                  push_pop_tp},

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
        sm_zen_print_header("scalar_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n", "scalar integer");
    }

    /* Print CSV header */
    printf("mnemonic,operands,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const InstructionProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const InstructionProbeEntry *current_entry = &probe_table[entry_idx];

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
