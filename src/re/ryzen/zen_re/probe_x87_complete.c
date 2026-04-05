#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <x86intrin.h>

/*
 * probe_x87_complete.c -- Fill ALL 40 missing x87 forms from AMD Volume 5.
 *
 * These are the forms NOT in the existing probe_x87_exhaustive.c:
 *   - Arithmetic P-forms (pop after operation)
 *   - Integer-memory forms (FIADD, FISUB, etc.)
 *   - BCD (FBLD, FBSTP)
 *   - Compare-and-pop variants
 *   - Control/state (FLDCW, FSTCW, FSTSW, FLDENV, FSTENV, FSAVE, FRSTOR, FINIT)
 *   - FPREM, FPREM1, FSUBR, FDIVR
 *   - All 8 FCMOVcc condition codes
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_x87_complete.c -lm \
 *     -o ../../../../build/bin/zen_probe_x87_complete
 */

/* ========================================================================
 * ARITHMETIC P-FORMS (pop stack after operation)
 *
 * Each P-form pops st(1) and leaves result in st(0). We push a fresh
 * value each iteration to keep the stack balanced.
 * ======================================================================== */

/* FADDP -- add st(0) to st(1), pop */
static double faddp_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0L;
    long double operand_value = 0.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(accumulator) : "m"(operand_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double faddp_tp(uint64_t iteration_count)
{
    long double stream_0 = 1.0L, stream_1 = 2.0L;
    long double operand_value = 0.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(stream_0) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(stream_1) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(stream_0) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfaddp" : "+t"(stream_1) : "m"(operand_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = stream_0 + stream_1; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FSUBP -- subtract st(0) from st(1), pop */
static double fsubp_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0e10L;
    long double operand_value = 0.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldt %1\n\tfsubp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfsubp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfsubp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfsubp" : "+t"(accumulator) : "m"(operand_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FSUBRP -- reverse subtract and pop */
static double fsubrp_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0e10L;
    long double operand_value = 1.0e10L + 0.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldt %1\n\tfsubrp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfsubrp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfsubrp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfsubrp" : "+t"(accumulator) : "m"(operand_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FMULP -- multiply st(0) by st(1), pop */
static double fmulp_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0000001L;
    long double operand_value = 1.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldt %1\n\tfmulp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfmulp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfmulp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfmulp" : "+t"(accumulator) : "m"(operand_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FDIVP -- divide st(1) by st(0), pop */
static double fdivp_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0000001L;
    long double operand_value = 1.0000001L;
    uint64_t reduced_iterations = iteration_count / 4;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fldt %1\n\tfdivp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfdivp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfdivp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfdivp" : "+t"(accumulator) : "m"(operand_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FDIVRP -- reverse divide and pop */
static double fdivrp_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0000001L;
    long double operand_value = 1.0000001L;
    uint64_t reduced_iterations = iteration_count / 4;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fldt %1\n\tfdivrp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfdivrp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfdivrp" : "+t"(accumulator) : "m"(operand_value));
        __asm__ volatile("fldt %1\n\tfdivrp" : "+t"(accumulator) : "m"(operand_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* ========================================================================
 * REVERSE ARITHMETIC (non-pop forms)
 * ======================================================================== */

/* FSUBR st(0),st(0) -- reverse subtract: st(0) = st(0) - st(0) */
static double fsubr_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double fsubr_tp(uint64_t iteration_count)
{
    long double stream_0 = 1.0000001L, stream_1 = 1.0000002L;
    long double stream_2 = 1.0000003L, stream_3 = 1.0000004L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(stream_0));
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(stream_1));
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(stream_2));
        __asm__ volatile("fsubr %%st(0), %%st(0)" : "+t"(stream_3));
        __asm__ volatile("" : "+t"(stream_0));
        __asm__ volatile("" : "+t"(stream_1));
        __asm__ volatile("" : "+t"(stream_2));
        __asm__ volatile("" : "+t"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = stream_0 + stream_2; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FDIVR st(0),st(0) -- reverse divide: st(0) = st(0) / st(0) = 1.0 */
static double fdivr_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0000001L;
    uint64_t reduced_iterations = iteration_count / 4;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

static double fdivr_tp(uint64_t iteration_count)
{
    long double stream_0 = 1.000001L, stream_1 = 1.000002L;
    long double stream_2 = 1.000003L, stream_3 = 1.000004L;
    uint64_t reduced_iterations = iteration_count / 4;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(stream_0));
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(stream_1));
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(stream_2));
        __asm__ volatile("fdivr %%st(0), %%st(0)" : "+t"(stream_3));
        __asm__ volatile("" : "+t"(stream_0));
        __asm__ volatile("" : "+t"(stream_1));
        __asm__ volatile("" : "+t"(stream_2));
        __asm__ volatile("" : "+t"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = stream_0 + stream_2; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* ========================================================================
 * INTEGER-MEMORY FORMS
 * ======================================================================== */

/* FIADD m32 -- add 32-bit integer from memory to st(0) */
static double fiadd_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 1;
    long double accumulator = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fiaddl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fiaddl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fiaddl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fiaddl %1" : "+t"(accumulator) : "m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FISUB m32 */
static double fisub_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 0;
    long double accumulator = 1.0e10L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fisubl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fisubl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fisubl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fisubl %1" : "+t"(accumulator) : "m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FISUBR m32 */
static double fisubr_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 1000000;
    long double accumulator = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fisubrl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fisubrl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fisubrl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fisubrl %1" : "+t"(accumulator) : "m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FIMUL m32 */
static double fimul_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 1;
    long double accumulator = 1.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fimull %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fimull %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fimull %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fimull %1" : "+t"(accumulator) : "m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FIDIV m32 */
static double fidiv_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 1;
    long double accumulator = 1.0000001L;
    uint64_t reduced_iterations = iteration_count / 4;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fidivl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fidivl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fidivl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fidivl %1" : "+t"(accumulator) : "m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FIDIVR m32 */
static double fidivr_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 1000000;
    long double accumulator = 1.0L;
    uint64_t reduced_iterations = iteration_count / 4;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fidivrl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fidivrl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fidivrl %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fidivrl %1" : "+t"(accumulator) : "m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FICOM m32 -- compare st(0) with integer in memory */
static double ficom_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 42;
    long double accumulator = 42.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("ficoml %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("ficoml %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("ficoml %1" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("ficoml %1" : "+t"(accumulator) : "m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FICOMP m32 -- compare and pop */
static double ficomp_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 42;
    long double accumulator = 42.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* ficomp pops, so reload st(0) */
        __asm__ volatile("ficompl %1\n\tfld %%st(0)" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstp %%st(1)" : "+t"(accumulator));
        __asm__ volatile("ficompl %1\n\tfld %%st(0)" : "+t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstp %%st(1)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FISTTP m32 -- SSE3 truncate and pop to m32 */
static double fisttp_m32_lat(uint64_t iteration_count)
{
    long double accumulator = 42.5L;
    int32_t store_buf __attribute__((aligned(64)));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fisttpl %1\n\tfildl %1"
                         : "+t"(accumulator), "=m"(store_buf));
        __asm__ volatile("fisttpl %1\n\tfildl %1"
                         : "+t"(accumulator), "=m"(store_buf));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FISTTP m64 -- SSE3 truncate and pop to m64 */
static double fisttp_m64_lat(uint64_t iteration_count)
{
    long double accumulator = 42.5L;
    int64_t store_buf __attribute__((aligned(64)));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fisttpq %1\n\tfildq %1"
                         : "+t"(accumulator), "=m"(store_buf));
        __asm__ volatile("fisttpq %1\n\tfildq %1"
                         : "+t"(accumulator), "=m"(store_buf));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* ========================================================================
 * BCD
 * ======================================================================== */

/* FBLD m80 -- load packed BCD from memory */
static double fbld_lat(uint64_t iteration_count)
{
    /* 80-bit packed BCD: 10 bytes, value = 0 */
    unsigned char bcd_memory[32] __attribute__((aligned(64)));
    memset(bcd_memory, 0, sizeof(bcd_memory));
    bcd_memory[0] = 0x42; /* encode 42 in low BCD digits */
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fbld %0\n\tfstp %%st(0)" : : "m"(bcd_memory[0]));
        __asm__ volatile("fbld %0\n\tfstp %%st(0)" : : "m"(bcd_memory[0]));
        __asm__ volatile("fbld %0\n\tfstp %%st(0)" : : "m"(bcd_memory[0]));
        __asm__ volatile("fbld %0\n\tfstp %%st(0)" : : "m"(bcd_memory[0]));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FBSTP m80 -- store packed BCD and pop */
static double fbstp_lat(uint64_t iteration_count)
{
    unsigned char bcd_memory[32] __attribute__((aligned(64)));
    memset(bcd_memory, 0, sizeof(bcd_memory));
    long double source_value = 42.0L;
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        /* fbstp pops st(0), fbld pushes it back -- chain through memory */
        __asm__ volatile("fbstp %0\n\tfbld %0" : "+m"(bcd_memory[0]) : "t"(source_value));
        __asm__ volatile("fbstp %0\n\tfbld %0" : "+m"(bcd_memory[0]) : "t"(source_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = source_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 2);
}

/* ========================================================================
 * COMPARE VARIANTS -- pop forms
 * ======================================================================== */

/* FCOMP st(1) -- compare and pop */
static double fcomp_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* fcomp pops st(0), so push it back */
        __asm__ volatile("fcomp %%st(1)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
        __asm__ volatile("fcomp %%st(1)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FCOMPP -- compare st(0) with st(1), pop both */
static double fcompp_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* Push two values, compare and pop both */
        __asm__ volatile(
            "fldt %0\n\t"
            "fldt %1\n\t"
            "fcompp"
            : : "m"(value_a), "m"(value_b) : "memory"
        );
        __asm__ volatile(
            "fldt %0\n\t"
            "fldt %1\n\t"
            "fcompp"
            : : "m"(value_a), "m"(value_b) : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FUCOMP st(1) -- unordered compare and pop */
static double fucomp_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fucomp %%st(1)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
        __asm__ volatile("fucomp %%st(1)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FUCOMPP -- unordered compare, pop both */
static double fucompp_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "fldt %0\n\t"
            "fldt %1\n\t"
            "fucompp"
            : : "m"(value_a), "m"(value_b) : "memory"
        );
        __asm__ volatile(
            "fldt %0\n\t"
            "fldt %1\n\t"
            "fucompp"
            : : "m"(value_a), "m"(value_b) : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FUCOMI st(0),st(1) -- unordered compare, set EFLAGS, no pop */
static double fucomi_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fucomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("fucomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("fucomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("fucomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCOMIP st(0),st(1) -- compare, set EFLAGS, pop */
static double fcomip_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fcomip %%st(1), %%st(0)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b) : : "cc");
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
        __asm__ volatile("fcomip %%st(1), %%st(0)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b) : : "cc");
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FUCOMIP st(0),st(1) -- unordered compare, set EFLAGS, pop */
static double fucomip_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fucomip %%st(1), %%st(0)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b) : : "cc");
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
        __asm__ volatile("fucomip %%st(1), %%st(0)\n\tfld %%st(0)" : "+t"(value_a), "+u"(value_b) : : "cc");
        __asm__ volatile("fstp %%st(1)" : "+t"(value_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* ========================================================================
 * REMAINDER
 * ======================================================================== */

/* FPREM -- IEEE non-compliant partial remainder */
static double fprem_lat(uint64_t iteration_count)
{
    long double accumulator = 7.5L;
    long double divisor = 3.0L;
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(accumulator), "+u"(divisor));
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fprem" : "+t"(accumulator) : "u"(divisor));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FPREM1 -- IEEE compliant partial remainder */
static double fprem1_lat(uint64_t iteration_count)
{
    long double accumulator = 7.5L;
    long double divisor = 3.0L;
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(accumulator), "+u"(divisor));
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fprem1" : "+t"(accumulator) : "u"(divisor));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* ========================================================================
 * CONTROL / STATE
 * ======================================================================== */

/* FLDCW m16 -- load FPU control word */
static double fldcw_tp(uint64_t iteration_count)
{
    uint16_t control_word __attribute__((aligned(64)));
    __asm__ volatile("fnstcw %0" : "=m"(control_word));
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fldcw %0" : : "m"(control_word));
        __asm__ volatile("fldcw %0" : : "m"(control_word));
        __asm__ volatile("fldcw %0" : : "m"(control_word));
        __asm__ volatile("fldcw %0" : : "m"(control_word));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FNSTCW m16 -- store FPU control word (no-wait) */
static double fnstcw_tp(uint64_t iteration_count)
{
    uint16_t control_word_buf[32] __attribute__((aligned(64)));
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fnstcw %0" : "=m"(control_word_buf[0]));
        __asm__ volatile("fnstcw %0" : "=m"(control_word_buf[1]));
        __asm__ volatile("fnstcw %0" : "=m"(control_word_buf[2]));
        __asm__ volatile("fnstcw %0" : "=m"(control_word_buf[3]));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint16_t sink = control_word_buf[0]; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FNSTSW m16 -- store FPU status word */
static double fnstsw_m16_tp(uint64_t iteration_count)
{
    uint16_t status_word_buf[32] __attribute__((aligned(64)));
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fnstsw %0" : "=m"(status_word_buf[0]));
        __asm__ volatile("fnstsw %0" : "=m"(status_word_buf[1]));
        __asm__ volatile("fnstsw %0" : "=m"(status_word_buf[2]));
        __asm__ volatile("fnstsw %0" : "=m"(status_word_buf[3]));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint16_t sink = status_word_buf[0]; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FNSTSW AX -- store FPU status word to AX register */
static double fnstsw_ax_tp(uint64_t iteration_count)
{
    uint16_t result_ax;
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fnstsw %%ax" : "=a"(result_ax));
        __asm__ volatile("fnstsw %%ax" : "=a"(result_ax));
        __asm__ volatile("fnstsw %%ax" : "=a"(result_ax));
        __asm__ volatile("fnstsw %%ax" : "=a"(result_ax));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint16_t sink = result_ax; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FLDENV m -- load FPU environment (28 bytes in protected mode) */
static double fldenv_tp(uint64_t iteration_count)
{
    unsigned char fpu_env_buffer[64] __attribute__((aligned(64)));
    __asm__ volatile("fnstenv %0" : "=m"(fpu_env_buffer));
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fldenv %0" : : "m"(fpu_env_buffer));
        __asm__ volatile("fldenv %0" : : "m"(fpu_env_buffer));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 2);
}

/* FNSTENV m -- store FPU environment */
static double fnstenv_tp(uint64_t iteration_count)
{
    unsigned char fpu_env_buffer[64] __attribute__((aligned(64)));
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fnstenv %0" : "=m"(fpu_env_buffer));
        __asm__ volatile("fnstenv %0" : "=m"(fpu_env_buffer));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 2);
}

/* FNSAVE m -- save full FPU state (108 bytes in 64-bit mode) */
static double fnsave_tp(uint64_t iteration_count)
{
    unsigned char fpu_save_buffer[128] __attribute__((aligned(64)));
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fnsave %0\n\tfrstor %0" : "=m"(fpu_save_buffer) : "m"(fpu_save_buffer));
        __asm__ volatile("fnsave %0\n\tfrstor %0" : "=m"(fpu_save_buffer) : "m"(fpu_save_buffer));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 2);
}

/* FRSTOR m -- restore full FPU state */
static double frstor_tp(uint64_t iteration_count)
{
    unsigned char fpu_save_buffer[128] __attribute__((aligned(64)));
    __asm__ volatile("fnsave %0" : "=m"(fpu_save_buffer));
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("frstor %0" : : "m"(fpu_save_buffer));
        __asm__ volatile("frstor %0" : : "m"(fpu_save_buffer));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 2);
}

/* FNINIT -- reinitialize FPU */
static double fninit_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fninit");
        __asm__ volatile("fninit");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 2);
}

/* FNCLEX -- clear exceptions (throughput) */
static double fnclex_verify_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fnclex");
        __asm__ volatile("fnclex");
        __asm__ volatile("fnclex");
        __asm__ volatile("fnclex");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* ========================================================================
 * FCMOVcc -- all 8 condition codes
 * (FCMOVB and FCMOVE and FCMOVNB already in existing probe; cover remaining)
 * ======================================================================== */

/* FCMOVBE -- below or equal (CF=1 or ZF=1) */
static double fcmovbe_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("stc\n\tfcmovbe %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("stc\n\tfcmovbe %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("stc\n\tfcmovbe %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("stc\n\tfcmovbe %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCMOVNBE -- not below or equal (CF=0 and ZF=0) */
static double fcmovnbe_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* mov $1, eax; cmp $0, eax -> clears ZF and CF */
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovnbe %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovnbe %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovnbe %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovnbe %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCMOVNE -- not equal (ZF=0) */
static double fcmovne_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* cmp $1, $0 -> ZF=0, so FCMOVNE fires */
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovne %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovne %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovne %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\tcmp $0, %%eax\n\tfcmovne %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCMOVU -- unordered (PF=1) */
static double fcmovu_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* fcomi with a NaN would set PF, but simpler: use inline to set PF via pushf */
        /* Alternative: compare value that sets PF. Use stc; cmc to get known flag state */
        /* Actually, we can use "mov $0, %%eax; add $0, %%eax" which sets PF=1 (even parity of 0) */
        __asm__ volatile("xor %%eax, %%eax\n\tfcmovu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("xor %%eax, %%eax\n\tfcmovu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("xor %%eax, %%eax\n\tfcmovu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("xor %%eax, %%eax\n\tfcmovu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCMOVNU -- not unordered (PF=0) */
static double fcmovnu_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* "mov $1, %%eax; test %%eax, %%eax" -> PF=0 (odd parity of 1) */
        __asm__ volatile("mov $1, %%eax\n\ttest %%eax, %%eax\n\tfcmovnu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\ttest %%eax, %%eax\n\tfcmovnu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\ttest %%eax, %%eax\n\tfcmovnu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("mov $1, %%eax\n\ttest %%eax, %%eax\n\tfcmovnu %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
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
} X87CompleteEntry;

static const X87CompleteEntry probe_table[] = {
    /* --- Arithmetic P-forms --- */
    {"FADDP",      "st(1),st(0)",    "x87_arith_p",  faddp_lat,         faddp_tp},
    {"FSUBP",      "st(1),st(0)",    "x87_arith_p",  fsubp_lat,         NULL},
    {"FSUBRP",     "st(1),st(0)",    "x87_arith_p",  fsubrp_lat,        NULL},
    {"FMULP",      "st(1),st(0)",    "x87_arith_p",  fmulp_lat,         NULL},
    {"FDIVP",      "st(1),st(0)",    "x87_arith_p",  fdivp_lat,         NULL},
    {"FDIVRP",     "st(1),st(0)",    "x87_arith_p",  fdivrp_lat,        NULL},

    /* --- Reverse arithmetic (non-pop) --- */
    {"FSUBR",      "st(0),st(0)",    "x87_arith",    fsubr_lat,         fsubr_tp},
    {"FDIVR",      "st(0),st(0)",    "x87_arith",    fdivr_lat,         fdivr_tp},

    /* --- Integer-memory forms --- */
    {"FIADD",      "m32",            "x87_int_mem",  fiadd_m32_lat,     NULL},
    {"FISUB",      "m32",            "x87_int_mem",  fisub_m32_lat,     NULL},
    {"FISUBR",     "m32",            "x87_int_mem",  fisubr_m32_lat,    NULL},
    {"FIMUL",      "m32",            "x87_int_mem",  fimul_m32_lat,     NULL},
    {"FIDIV",      "m32",            "x87_int_mem",  fidiv_m32_lat,     NULL},
    {"FIDIVR",     "m32",            "x87_int_mem",  fidivr_m32_lat,    NULL},
    {"FICOM",      "m32",            "x87_int_mem",  ficom_m32_lat,     NULL},
    {"FICOMP",     "m32",            "x87_int_mem",  ficomp_m32_lat,    NULL},
    {"FISTTP",     "m32",            "x87_sse3",     fisttp_m32_lat,    NULL},
    {"FISTTP",     "m64",            "x87_sse3",     fisttp_m64_lat,    NULL},

    /* --- BCD --- */
    {"FBLD",       "m80",            "x87_bcd",      fbld_lat,          NULL},
    {"FBSTP",      "m80",            "x87_bcd",      fbstp_lat,         NULL},

    /* --- Compare variants --- */
    {"FCOMP",      "st(1)",          "x87_cmp",      fcomp_lat,         NULL},
    {"FCOMPP",     "",               "x87_cmp",      fcompp_lat,        NULL},
    {"FUCOMP",     "st(1)",          "x87_cmp",      fucomp_lat,        NULL},
    {"FUCOMPP",    "",               "x87_cmp",      fucompp_lat,       NULL},
    {"FUCOMI",     "st(0),st(1)",    "x87_cmp",      fucomi_lat,        NULL},
    {"FCOMIP",     "st(0),st(1)",    "x87_cmp",      fcomip_lat,        NULL},
    {"FUCOMIP",    "st(0),st(1)",    "x87_cmp",      fucomip_lat,       NULL},

    /* --- Remainder --- */
    {"FPREM",      "",               "x87_remain",   fprem_lat,         NULL},
    {"FPREM1",     "",               "x87_remain",   fprem1_lat,        NULL},

    /* --- Control/state --- */
    {"FLDCW",      "m16",            "x87_ctrl",     NULL,              fldcw_tp},
    {"FNSTCW",     "m16",            "x87_ctrl",     NULL,              fnstcw_tp},
    {"FNSTSW",     "m16",            "x87_ctrl",     NULL,              fnstsw_m16_tp},
    {"FNSTSW",     "AX",             "x87_ctrl",     NULL,              fnstsw_ax_tp},
    {"FLDENV",     "m",              "x87_ctrl",     NULL,              fldenv_tp},
    {"FNSTENV",    "m",              "x87_ctrl",     NULL,              fnstenv_tp},
    {"FNSAVE",     "m",              "x87_ctrl",     NULL,              fnsave_tp},
    {"FRSTOR",     "m",              "x87_ctrl",     NULL,              frstor_tp},
    {"FNINIT",     "",               "x87_ctrl",     NULL,              fninit_tp},
    {"FNCLEX",     "(verify)",       "x87_ctrl",     NULL,              fnclex_verify_tp},

    /* --- FCMOVcc (remaining 5 condition codes) --- */
    {"FCMOVBE",    "st(0),st(1)",    "x87_cmov",     fcmovbe_lat,       NULL},
    {"FCMOVNBE",   "st(0),st(1)",    "x87_cmov",     fcmovnbe_lat,      NULL},
    {"FCMOVNE",    "st(0),st(1)",    "x87_cmov",     fcmovne_lat,       NULL},
    {"FCMOVU",     "st(0),st(1)",    "x87_cmov",     fcmovu_lat,        NULL},
    {"FCMOVNU",    "st(0),st(1)",    "x87_cmov",     fcmovnu_lat,       NULL},

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
        sm_zen_print_header("x87_complete", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n", "x87 FPU (gap-fill)");
    }

    /* Print CSV header */
    printf("mnemonic,operands,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const X87CompleteEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const X87CompleteEntry *current_entry = &probe_table[entry_idx];

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
