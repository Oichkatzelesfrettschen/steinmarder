#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_x86_gaps.c -- Fill the 17 missing x86 scalar mnemonics from the
 * Zen 4 coverage gap report.
 *
 * Covers: CLC, CLD, CMC, CMOVB, CMOVBE, CMOVE, CMOVL, CMOVLE,
 * POP, PUSH, SETB, SETBE, SETE, SETL, SETLE, STC, STD.
 *
 * Output: one CSV line per instruction form:
 *   mnemonic,operands,extension,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx2 -mavx512f -mavx512bw \
 *     -mavx512vl -mfma -msse4.1 -msse4.2 -mssse3 -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_x86_gaps.c -lm \
 *     -o ../../../../build/bin/zen_probe_x86_gaps
 */

/* ========================================================================
 * MACRO TEMPLATES -- same as probe_scalar_exhaustive.c
 * ======================================================================== */

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

/* ========================================================================
 * FLAG INSTRUCTIONS: CLC, STC, CMC, CLD, STD
 * ======================================================================== */

/* CLC -- clear carry flag (throughput only, no data dependency) */
static double x86_clc_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("clc" ::: "cc");
        __asm__ volatile("clc" ::: "cc");
        __asm__ volatile("clc" ::: "cc");
        __asm__ volatile("clc" ::: "cc");
        __asm__ volatile("clc" ::: "cc");
        __asm__ volatile("clc" ::: "cc");
        __asm__ volatile("clc" ::: "cc");
        __asm__ volatile("clc" ::: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* STC -- set carry flag */
static double x86_stc_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("stc" ::: "cc");
        __asm__ volatile("stc" ::: "cc");
        __asm__ volatile("stc" ::: "cc");
        __asm__ volatile("stc" ::: "cc");
        __asm__ volatile("stc" ::: "cc");
        __asm__ volatile("stc" ::: "cc");
        __asm__ volatile("stc" ::: "cc");
        __asm__ volatile("stc" ::: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CMC -- complement carry flag (latency: serial CMC chain) */
static double x86_cmc_lat(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmc\n\tcmc\n\tcmc\n\tcmc" ::: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double x86_cmc_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmc" ::: "cc");
        __asm__ volatile("cmc" ::: "cc");
        __asm__ volatile("cmc" ::: "cc");
        __asm__ volatile("cmc" ::: "cc");
        __asm__ volatile("cmc" ::: "cc");
        __asm__ volatile("cmc" ::: "cc");
        __asm__ volatile("cmc" ::: "cc");
        __asm__ volatile("cmc" ::: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CLD -- clear direction flag */
static double x86_cld_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cld" ::: "cc");
        __asm__ volatile("cld" ::: "cc");
        __asm__ volatile("cld" ::: "cc");
        __asm__ volatile("cld" ::: "cc");
        __asm__ volatile("cld" ::: "cc");
        __asm__ volatile("cld" ::: "cc");
        __asm__ volatile("cld" ::: "cc");
        __asm__ volatile("cld" ::: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* STD -- set direction flag (restore CLD after) */
static double x86_std_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("std\n\tcld" ::: "cc");
        __asm__ volatile("std\n\tcld" ::: "cc");
        __asm__ volatile("std\n\tcld" ::: "cc");
        __asm__ volatile("std\n\tcld" ::: "cc");
        __asm__ volatile("std\n\tcld" ::: "cc");
        __asm__ volatile("std\n\tcld" ::: "cc");
        __asm__ volatile("std\n\tcld" ::: "cc");
        __asm__ volatile("std\n\tcld" ::: "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    /* 2 instructions per slot but measuring STD throughput */
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * CMOVcc VARIANTS: CMOVB, CMOVBE, CMOVE, CMOVL, CMOVLE
 * ======================================================================== */

/* CMOVB -- carry flag set (below) */
static double x86_cmovb_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t alternate_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(alternate_source));
    /* Set CF=1 so CMOVB always fires */
    __asm__ volatile("stc" ::: "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmovb %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovb %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovb %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovb %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(x86_cmovb_tp, "cmovb %1, %0")

/* CMOVBE -- CF=1 or ZF=1 (below or equal) */
static double x86_cmovbe_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t alternate_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(alternate_source));
    __asm__ volatile("stc" ::: "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmovbe %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovbe %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovbe %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovbe %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(x86_cmovbe_tp, "cmovbe %1, %0")

/* CMOVE -- ZF=1 (equal) */
static double x86_cmove_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t alternate_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(alternate_source));
    /* Set ZF=1 by xoring a register with itself */
    __asm__ volatile("xor %%eax, %%eax" ::: "eax", "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmove %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmove %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmove %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmove %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(x86_cmove_tp, "cmove %1, %0")

/* CMOVL -- SF!=OF (less, signed) */
static double x86_cmovl_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t alternate_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(alternate_source));
    /* Set SF=1, OF=0 so SF!=OF => CMOVL fires */
    __asm__ volatile("cmp $0, %0" : : "r"((uint64_t)0x8000000000000000ULL) : "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmovl %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovl %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovl %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovl %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(x86_cmovl_tp, "cmovl %1, %0")

/* CMOVLE -- ZF=1 or SF!=OF (less or equal, signed) */
static double x86_cmovle_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t alternate_source = 0x1111111111111111ULL;
    __asm__ volatile("" : "+r"(alternate_source));
    __asm__ volatile("xor %%eax, %%eax" ::: "eax", "cc"); /* ZF=1 */
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmovle %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovle %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovle %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
        __asm__ volatile("cmovle %1, %0" : "+r"(accumulator) : "r"(alternate_source) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}
THROUGHPUT_BINARY_REG(x86_cmovle_tp, "cmovle %1, %0")

/* ========================================================================
 * PUSH / POP -- separate measurements
 * ======================================================================== */

/* PUSH r64 throughput */
static double x86_push_tp(uint64_t iteration_count)
{
    uint64_t push_value = 0xDEADBEEF;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "push %0\n\t"
            "push %0\n\t"
            "push %0\n\t"
            "push %0\n\t"
            "push %0\n\t"
            "push %0\n\t"
            "push %0\n\t"
            "push %0\n\t"
            "add $64, %%rsp"
            : : "r"(push_value) : "rsp", "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* POP r64 throughput */
static double x86_pop_tp(uint64_t iteration_count)
{
    uint64_t result_0, result_1, result_2, result_3;
    uint64_t result_4, result_5, result_6, result_7;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "sub $64, %%rsp\n\t"
            "pop %0\n\t"
            "pop %1\n\t"
            "pop %2\n\t"
            "pop %3\n\t"
            "pop %4\n\t"
            "pop %5\n\t"
            "pop %6\n\t"
            "pop %7"
            : "=r"(result_0), "=r"(result_1), "=r"(result_2), "=r"(result_3),
              "=r"(result_4), "=r"(result_5), "=r"(result_6), "=r"(result_7)
            : : "rsp", "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = result_0 ^ result_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * SETcc VARIANTS: SETB, SETBE, SETE, SETL, SETLE
 * ======================================================================== */

/* SETB -- set byte if CF=1 */
static double x86_setb_tp(uint64_t iteration_count)
{
    uint8_t result_0, result_1, result_2, result_3;
    uint8_t result_4, result_5, result_6, result_7;
    /* Set CF=1 */
    __asm__ volatile("stc" ::: "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("setb %0" : "=r"(result_0) : : "cc");
        __asm__ volatile("setb %0" : "=r"(result_1) : : "cc");
        __asm__ volatile("setb %0" : "=r"(result_2) : : "cc");
        __asm__ volatile("setb %0" : "=r"(result_3) : : "cc");
        __asm__ volatile("setb %0" : "=r"(result_4) : : "cc");
        __asm__ volatile("setb %0" : "=r"(result_5) : : "cc");
        __asm__ volatile("setb %0" : "=r"(result_6) : : "cc");
        __asm__ volatile("setb %0" : "=r"(result_7) : : "cc");
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint8_t sink = result_0 ^ result_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* SETBE -- set byte if CF=1 or ZF=1 */
static double x86_setbe_tp(uint64_t iteration_count)
{
    uint8_t result_0, result_1, result_2, result_3;
    uint8_t result_4, result_5, result_6, result_7;
    __asm__ volatile("stc" ::: "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("setbe %0" : "=r"(result_0) : : "cc");
        __asm__ volatile("setbe %0" : "=r"(result_1) : : "cc");
        __asm__ volatile("setbe %0" : "=r"(result_2) : : "cc");
        __asm__ volatile("setbe %0" : "=r"(result_3) : : "cc");
        __asm__ volatile("setbe %0" : "=r"(result_4) : : "cc");
        __asm__ volatile("setbe %0" : "=r"(result_5) : : "cc");
        __asm__ volatile("setbe %0" : "=r"(result_6) : : "cc");
        __asm__ volatile("setbe %0" : "=r"(result_7) : : "cc");
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint8_t sink = result_0 ^ result_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* SETE -- set byte if ZF=1 */
static double x86_sete_tp(uint64_t iteration_count)
{
    uint8_t result_0, result_1, result_2, result_3;
    uint8_t result_4, result_5, result_6, result_7;
    __asm__ volatile("xor %%eax, %%eax" ::: "eax", "cc"); /* ZF=1 */
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("sete %0" : "=r"(result_0) : : "cc");
        __asm__ volatile("sete %0" : "=r"(result_1) : : "cc");
        __asm__ volatile("sete %0" : "=r"(result_2) : : "cc");
        __asm__ volatile("sete %0" : "=r"(result_3) : : "cc");
        __asm__ volatile("sete %0" : "=r"(result_4) : : "cc");
        __asm__ volatile("sete %0" : "=r"(result_5) : : "cc");
        __asm__ volatile("sete %0" : "=r"(result_6) : : "cc");
        __asm__ volatile("sete %0" : "=r"(result_7) : : "cc");
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint8_t sink = result_0 ^ result_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* SETL -- set byte if SF!=OF */
static double x86_setl_tp(uint64_t iteration_count)
{
    uint8_t result_0, result_1, result_2, result_3;
    uint8_t result_4, result_5, result_6, result_7;
    __asm__ volatile("cmp $0, %0" : : "r"((uint64_t)0x8000000000000000ULL) : "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("setl %0" : "=r"(result_0) : : "cc");
        __asm__ volatile("setl %0" : "=r"(result_1) : : "cc");
        __asm__ volatile("setl %0" : "=r"(result_2) : : "cc");
        __asm__ volatile("setl %0" : "=r"(result_3) : : "cc");
        __asm__ volatile("setl %0" : "=r"(result_4) : : "cc");
        __asm__ volatile("setl %0" : "=r"(result_5) : : "cc");
        __asm__ volatile("setl %0" : "=r"(result_6) : : "cc");
        __asm__ volatile("setl %0" : "=r"(result_7) : : "cc");
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint8_t sink = result_0 ^ result_4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* SETLE -- set byte if ZF=1 or SF!=OF */
static double x86_setle_tp(uint64_t iteration_count)
{
    uint8_t result_0, result_1, result_2, result_3;
    uint8_t result_4, result_5, result_6, result_7;
    __asm__ volatile("xor %%eax, %%eax" ::: "eax", "cc"); /* ZF=1 */
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("setle %0" : "=r"(result_0) : : "cc");
        __asm__ volatile("setle %0" : "=r"(result_1) : : "cc");
        __asm__ volatile("setle %0" : "=r"(result_2) : : "cc");
        __asm__ volatile("setle %0" : "=r"(result_3) : : "cc");
        __asm__ volatile("setle %0" : "=r"(result_4) : : "cc");
        __asm__ volatile("setle %0" : "=r"(result_5) : : "cc");
        __asm__ volatile("setle %0" : "=r"(result_6) : : "cc");
        __asm__ volatile("setle %0" : "=r"(result_7) : : "cc");
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),
                              "+r"(result_2), "+r"(result_3));
        __asm__ volatile("" : "+r"(result_4), "+r"(result_5),
                              "+r"(result_6), "+r"(result_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint8_t sink = result_0 ^ result_4; (void)sink;
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
} X86GapProbeEntry;

static const X86GapProbeEntry x86_gap_probe_table[] = {
    /* Flags */
    {"CLC",     "",              "x86","flag",   NULL,            x86_clc_tp},
    {"STC",     "",              "x86","flag",   NULL,            x86_stc_tp},
    {"CMC",     "",              "x86","flag",   x86_cmc_lat,     x86_cmc_tp},
    {"CLD",     "",              "x86","flag",   NULL,            x86_cld_tp},
    {"STD",     "(paired w/CLD)","x86","flag",   NULL,            x86_std_tp},

    /* CMOVcc */
    {"CMOVB",   "r64,r64",      "x86","cmov",   x86_cmovb_lat,   x86_cmovb_tp},
    {"CMOVBE",  "r64,r64",      "x86","cmov",   x86_cmovbe_lat,  x86_cmovbe_tp},
    {"CMOVE",   "r64,r64",      "x86","cmov",   x86_cmove_lat,   x86_cmove_tp},
    {"CMOVL",   "r64,r64",      "x86","cmov",   x86_cmovl_lat,   x86_cmovl_tp},
    {"CMOVLE",  "r64,r64",      "x86","cmov",   x86_cmovle_lat,  x86_cmovle_tp},

    /* Push / Pop */
    {"PUSH",    "r64",           "x86","stack",  NULL,            x86_push_tp},
    {"POP",     "r64",           "x86","stack",  NULL,            x86_pop_tp},

    /* SETcc */
    {"SETB",    "r8",            "x86","setcc",  NULL,            x86_setb_tp},
    {"SETBE",   "r8",            "x86","setcc",  NULL,            x86_setbe_tp},
    {"SETE",    "r8",            "x86","setcc",  NULL,            x86_sete_tp},
    {"SETL",    "r8",            "x86","setcc",  NULL,            x86_setl_tp},
    {"SETLE",   "r8",            "x86","setcc",  NULL,            x86_setle_tp},

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
        sm_zen_print_header("x86_gaps", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n",
               "missing x86 scalar gap-fill");
    }

    /* Print CSV header */
    printf("mnemonic,operands,extension,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; x86_gap_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const X86GapProbeEntry *current_entry = &x86_gap_probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; x86_gap_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const X86GapProbeEntry *current_entry = &x86_gap_probe_table[entry_idx];

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
