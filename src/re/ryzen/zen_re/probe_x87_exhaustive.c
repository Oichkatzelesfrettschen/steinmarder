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
 * probe_x87_exhaustive.c -- Exhaustive x87 FPU instruction measurement.
 *
 * Auto-measures latency (4-deep dependency chain) and throughput (4 independent
 * streams) for every x87 instruction form on Zen 4.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_x87_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_x87_exhaustive
 */

/* ========================================================================
 * MACRO TEMPLATES FOR X87
 *
 * x87 uses the "+t" constraint for st(0) (top of FPU stack).
 * For throughput we use 4 independent streams since x87 stack depth
 * is limited to 8 registers.
 * ======================================================================== */

/*
 * X87_LATENCY_ST0: instruction reads and writes st(0).
 *   e.g., "fadd %%st(0), %%st(0)"
 */
#define X87_LATENCY_ST0(func_name, asm_str)                                    \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    long double accumulator = 1.0000001L;                                      \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __asm__ volatile(asm_str : "+t"(accumulator));                          \
        __asm__ volatile(asm_str : "+t"(accumulator));                          \
        __asm__ volatile(asm_str : "+t"(accumulator));                          \
        __asm__ volatile(asm_str : "+t"(accumulator));                          \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile long double sink = accumulator; (void)sink;                        \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

/*
 * X87_THROUGHPUT_ST0: 4 independent x87 streams.
 * We push/pop around each measurement since x87 stack is 8-deep.
 */
#define X87_THROUGHPUT_ST0(func_name, asm_str)                                 \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    long double stream_0 = 1.0000001L;                                         \
    long double stream_1 = 1.0000002L;                                         \
    long double stream_2 = 1.0000003L;                                         \
    long double stream_3 = 1.0000004L;                                         \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __asm__ volatile(asm_str : "+t"(stream_0));                             \
        __asm__ volatile(asm_str : "+t"(stream_1));                             \
        __asm__ volatile(asm_str : "+t"(stream_2));                             \
        __asm__ volatile(asm_str : "+t"(stream_3));                             \
        __asm__ volatile("" : "+t"(stream_0)); /* prevent merging */            \
        __asm__ volatile("" : "+t"(stream_1));                                  \
        __asm__ volatile("" : "+t"(stream_2));                                  \
        __asm__ volatile("" : "+t"(stream_3));                                  \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile long double sink = stream_0 + stream_2; (void)sink;               \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

/*
 * X87_THROUGHPUT_NOIO: instruction with no register I/O (e.g., FNOP, FWAIT).
 * Measure pure dispatch throughput.
 */
#define X87_THROUGHPUT_NOIO(func_name, asm_str)                                \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __asm__ volatile(asm_str);                                              \
        __asm__ volatile(asm_str);                                              \
        __asm__ volatile(asm_str);                                              \
        __asm__ volatile(asm_str);                                              \
        __asm__ volatile(asm_str);                                              \
        __asm__ volatile(asm_str);                                              \
        __asm__ volatile(asm_str);                                              \
        __asm__ volatile(asm_str);                                              \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* ========================================================================
 * MOVE / LOAD / STORE
 * ======================================================================== */

/* FLD st(0) -- load from stack register (fld %st(0) duplicates TOS) */
static double fld_reg_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* fld %st(0) pushes a copy; fstp %st(1) pops it, netting zero stack change */
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(accumulator));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(accumulator));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(accumulator));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double fld_reg_tp(uint64_t iteration_count)
{
    long double stream_0 = 1.0L;
    long double stream_1 = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_0));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_1));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_0));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_1));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_0));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_1));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_0));
        __asm__ volatile("fld %%st(0)\n\tfstp %%st(1)" : "+t"(stream_1));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = stream_0 + stream_1; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FLD m32 -- load 32-bit float from memory */
static double fld_m32_lat(uint64_t iteration_count)
{
    volatile float memory_operand __attribute__((aligned(64))) = 1.0f;
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("flds %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstps %0" : "=m"(memory_operand) : "t"(accumulator));
        __asm__ volatile("flds %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstps %0" : "=m"(memory_operand) : "t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FLD m64 -- load 64-bit double from memory */
static double fld_m64_lat(uint64_t iteration_count)
{
    volatile double memory_operand __attribute__((aligned(64))) = 1.0;
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldl %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstpl %0" : "=m"(memory_operand) : "t"(accumulator));
        __asm__ volatile("fldl %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstpl %0" : "=m"(memory_operand) : "t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FLD m80 -- load 80-bit extended precision from memory */
static double fld_m80_lat(uint64_t iteration_count)
{
    volatile long double memory_operand __attribute__((aligned(64))) = 1.0L;
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldt %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstpt %0" : "=m"(memory_operand) : "t"(accumulator));
        __asm__ volatile("fldt %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fstpt %0" : "=m"(memory_operand) : "t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FST m32 -- store 32-bit float to memory (throughput) */
static double fst_m32_tp(uint64_t iteration_count)
{
    float memory_buf[16] __attribute__((aligned(64)));
    long double source_value = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fsts %0" : "=m"(memory_buf[0]) : "t"(source_value));
        __asm__ volatile("fsts %0" : "=m"(memory_buf[1]) : "t"(source_value));
        __asm__ volatile("fsts %0" : "=m"(memory_buf[2]) : "t"(source_value));
        __asm__ volatile("fsts %0" : "=m"(memory_buf[3]) : "t"(source_value));
        __asm__ volatile("fsts %0" : "=m"(memory_buf[4]) : "t"(source_value));
        __asm__ volatile("fsts %0" : "=m"(memory_buf[5]) : "t"(source_value));
        __asm__ volatile("fsts %0" : "=m"(memory_buf[6]) : "t"(source_value));
        __asm__ volatile("fsts %0" : "=m"(memory_buf[7]) : "t"(source_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile float sink = memory_buf[0]; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FST m64 -- store 64-bit double to memory (throughput) */
static double fst_m64_tp(uint64_t iteration_count)
{
    double memory_buf[8] __attribute__((aligned(64)));
    long double source_value = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fstl %0" : "=m"(memory_buf[0]) : "t"(source_value));
        __asm__ volatile("fstl %0" : "=m"(memory_buf[1]) : "t"(source_value));
        __asm__ volatile("fstl %0" : "=m"(memory_buf[2]) : "t"(source_value));
        __asm__ volatile("fstl %0" : "=m"(memory_buf[3]) : "t"(source_value));
        __asm__ volatile("fstl %0" : "=m"(memory_buf[4]) : "t"(source_value));
        __asm__ volatile("fstl %0" : "=m"(memory_buf[5]) : "t"(source_value));
        __asm__ volatile("fstl %0" : "=m"(memory_buf[6]) : "t"(source_value));
        __asm__ volatile("fstl %0" : "=m"(memory_buf[7]) : "t"(source_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile double sink = memory_buf[0]; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FSTP m80 -- store 80-bit extended to memory (throughput) */
static double fstp_m80_tp(uint64_t iteration_count)
{
    long double memory_buf[4] __attribute__((aligned(64)));
    long double source_value = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* fstp pops, so reload after each one */
        __asm__ volatile("fstpt %0" : "=m"(memory_buf[0]) : "t"(source_value));
        __asm__ volatile("fldt %0" : : "m"(memory_buf[0]));
        __asm__ volatile("fstpt %0" : "=m"(memory_buf[1]) : "t"(source_value));
        __asm__ volatile("fldt %0" : : "m"(memory_buf[1]));
        __asm__ volatile("fstpt %0" : "=m"(memory_buf[2]) : "t"(source_value));
        __asm__ volatile("fldt %0" : : "m"(memory_buf[2]));
        __asm__ volatile("fstpt %0" : "=m"(memory_buf[3]) : "t"(source_value));
        __asm__ volatile("fldt %0" : : "m"(memory_buf[3]));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = memory_buf[0]; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FXCH -- exchange st(0) and st(1); may be zero-latency on Zen 4 */
static double fxch_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    /* Load two values onto x87 stack */
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a + value_b; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double fxch_tp(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
        __asm__ volatile("fxch %%st(1)" : "+t"(value_a), "+u"(value_b));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a + value_b; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FILD m32 -- load integer from memory to x87 */
static double fild_m32_lat(uint64_t iteration_count)
{
    volatile int32_t memory_operand __attribute__((aligned(64))) = 42;
    long double accumulator;
    int32_t store_buf __attribute__((aligned(64)));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fildl %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fistpl %0" : "=m"(store_buf) : "t"(accumulator));
        __asm__ volatile("fildl %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fistpl %0" : "=m"(store_buf) : "t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FILD m64 -- load 64-bit integer from memory */
static double fild_m64_lat(uint64_t iteration_count)
{
    volatile int64_t memory_operand __attribute__((aligned(64))) = 42;
    long double accumulator;
    int64_t store_buf __attribute__((aligned(64)));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fildq %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fistpq %0" : "=m"(store_buf) : "t"(accumulator));
        __asm__ volatile("fildq %1" : "=t"(accumulator) : "m"(memory_operand));
        __asm__ volatile("fistpq %0" : "=m"(store_buf) : "t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* FISTP m32 -- store integer to memory (throughput) */
static double fistp_m32_tp(uint64_t iteration_count)
{
    int32_t store_buf[16] __attribute__((aligned(64)));
    long double source_value = 42.0L;
    volatile int32_t reload_buf __attribute__((aligned(64))) = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fistpl %0" : "=m"(store_buf[0]) : "t"(source_value));
        __asm__ volatile("fildl %0" : : "m"(reload_buf)); /* reload st(0) */
        __asm__ volatile("fistpl %0" : "=m"(store_buf[1]) : "t"(source_value));
        __asm__ volatile("fildl %0" : : "m"(reload_buf));
        __asm__ volatile("fistpl %0" : "=m"(store_buf[2]) : "t"(source_value));
        __asm__ volatile("fildl %0" : : "m"(reload_buf));
        __asm__ volatile("fistpl %0" : "=m"(store_buf[3]) : "t"(source_value));
        __asm__ volatile("fildl %0" : : "m"(reload_buf));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile int32_t sink = store_buf[0]; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * ARITHMETIC
 * ======================================================================== */

/* FADD st(0),st(0) */
X87_LATENCY_ST0(fadd_lat, "fadd %%st(0), %%st(0)")
X87_THROUGHPUT_ST0(fadd_tp, "fadd %%st(0), %%st(0)")

/* FSUB st(0),st(0) -- always produces zero, but the dep chain is real */
X87_LATENCY_ST0(fsub_lat, "fsub %%st(0), %%st(0)")
X87_THROUGHPUT_ST0(fsub_tp, "fsub %%st(0), %%st(0)")

/* FMUL st(0),st(0) */
X87_LATENCY_ST0(fmul_lat, "fmul %%st(0), %%st(0)")
X87_THROUGHPUT_ST0(fmul_tp, "fmul %%st(0), %%st(0)")

/* FDIV st(0),st(0) -- result is 1.0 always, dep chain is real */
static double fdiv_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0000001L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(accumulator));
        /* Result is 1.0; chain still carries the dependency */
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double fdiv_tp(uint64_t iteration_count)
{
    long double stream_0 = 1.000001L;
    long double stream_1 = 1.000002L;
    long double stream_2 = 1.000003L;
    long double stream_3 = 1.000004L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(stream_0));
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(stream_1));
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(stream_2));
        __asm__ volatile("fdiv %%st(0), %%st(0)" : "+t"(stream_3));
        __asm__ volatile("" : "+t"(stream_0));
        __asm__ volatile("" : "+t"(stream_1));
        __asm__ volatile("" : "+t"(stream_2));
        __asm__ volatile("" : "+t"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = stream_0 + stream_2; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FABS */
X87_LATENCY_ST0(fabs_lat, "fabs")
X87_THROUGHPUT_ST0(fabs_tp, "fabs")

/* FCHS -- negate */
X87_LATENCY_ST0(fchs_lat, "fchs")
X87_THROUGHPUT_ST0(fchs_tp, "fchs")

/* FRNDINT -- round to integer */
X87_LATENCY_ST0(frndint_lat, "frndint")
X87_THROUGHPUT_ST0(frndint_tp, "frndint")

/* ========================================================================
 * COMPARE
 * ======================================================================== */

/* FCOM st(1) -- compare st(0) with st(1), no pop */
static double fcom_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fcom %%st(1)" : "+t"(value_a) : "u"(value_b));
        __asm__ volatile("fcom %%st(1)" : "+t"(value_a) : "u"(value_b));
        __asm__ volatile("fcom %%st(1)" : "+t"(value_a) : "u"(value_b));
        __asm__ volatile("fcom %%st(1)" : "+t"(value_a) : "u"(value_b));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCOMI st(0),st(1) -- compare and set EFLAGS directly */
static double fcomi_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fcomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("fcomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("fcomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("fcomi %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FUCOM st(1) -- unordered compare */
static double fucom_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fucom %%st(1)" : "+t"(value_a) : "u"(value_b));
        __asm__ volatile("fucom %%st(1)" : "+t"(value_a) : "u"(value_b));
        __asm__ volatile("fucom %%st(1)" : "+t"(value_a) : "u"(value_b));
        __asm__ volatile("fucom %%st(1)" : "+t"(value_a) : "u"(value_b));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FTST -- compare st(0) with 0.0 */
static double ftst_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("ftst" : "+t"(accumulator));
        __asm__ volatile("ftst" : "+t"(accumulator));
        __asm__ volatile("ftst" : "+t"(accumulator));
        __asm__ volatile("ftst" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FXAM -- examine st(0) class */
static double fxam_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fxam" : "+t"(accumulator));
        __asm__ volatile("fxam" : "+t"(accumulator));
        __asm__ volatile("fxam" : "+t"(accumulator));
        __asm__ volatile("fxam" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * TRANSCENDENTAL / SPECIAL MATH
 * ======================================================================== */

/* FSQRT -- square root of st(0) */
static double fsqrt_lat(uint64_t iteration_count)
{
    /* Use a value that survives repeated sqrt without going to 1.0 too fast */
    long double accumulator = 1.0e10L;
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fsqrt" : "+t"(accumulator));
        __asm__ volatile("fsqrt" : "+t"(accumulator));
        __asm__ volatile("fsqrt" : "+t"(accumulator));
        __asm__ volatile("fsqrt" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

static double fsqrt_tp(uint64_t iteration_count)
{
    long double stream_0 = 1.0e10L;
    long double stream_1 = 2.0e10L;
    long double stream_2 = 3.0e10L;
    long double stream_3 = 4.0e10L;
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fsqrt" : "+t"(stream_0));
        __asm__ volatile("fsqrt" : "+t"(stream_1));
        __asm__ volatile("fsqrt" : "+t"(stream_2));
        __asm__ volatile("fsqrt" : "+t"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = stream_0 + stream_2; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 4);
}

/* FSIN -- transcendental, extremely slow and microcoded */
static double fsin_lat(uint64_t iteration_count)
{
    long double accumulator = 0.5L;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fsin" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FCOS */
static double fcos_lat(uint64_t iteration_count)
{
    long double accumulator = 0.5L;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fcos" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FSINCOS -- compute sin and cos simultaneously */
static double fsincos_lat(uint64_t iteration_count)
{
    long double accumulator = 0.5L;
    long double extra_result;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        /* fsincos replaces st(0) with sin, pushes cos */
        __asm__ volatile("fsincos\n\tfstp %%st(1)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    (void)extra_result;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FPTAN -- partial tangent: replaces st(0) with tan, pushes 1.0 */
static double fptan_lat(uint64_t iteration_count)
{
    long double accumulator = 0.5L;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("fptan\n\tfstp %%st(0)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FPATAN -- arctangent: st(1)/st(0), result in st(0) after pop */
static double fpatan_lat(uint64_t iteration_count)
{
    long double value_a = 0.5L;
    long double value_b = 1.0L;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        /* Load two values, compute atan2 */
        __asm__ volatile(
            "fld %%st(0)\n\t"     /* push copy of value_a */
            "fpatan"              /* st(1) = atan2(st(1), st(0)), pop */
            : "+t"(value_a));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    (void)value_b;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* F2XM1 -- 2^st(0) - 1, input must be in [-1,1] */
static double f2xm1_lat(uint64_t iteration_count)
{
    long double accumulator = 0.5L;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("f2xm1" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FYL2X -- st(1) * log2(st(0)), pops one */
static double fyl2x_lat(uint64_t iteration_count)
{
    long double accumulator = 2.0L;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile(
            "fld1\n\t"
            "fxch %%st(1)\n\t"
            "fyl2x"
            : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FYL2XP1 -- st(1) * log2(st(0) + 1.0), pops one */
static double fyl2xp1_lat(uint64_t iteration_count)
{
    long double accumulator = 0.5L;
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile(
            "fld1\n\t"
            "fxch %%st(1)\n\t"
            "fyl2xp1"
            : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* FSCALE -- st(0) = st(0) * 2^trunc(st(1)) */
static double fscale_lat(uint64_t iteration_count)
{
    long double accumulator = 1.0L;
    long double exponent = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(accumulator), "+u"(exponent));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fscale" : "+t"(accumulator) : "u"(exponent));
        __asm__ volatile("fscale" : "+t"(accumulator) : "u"(exponent));
        __asm__ volatile("fscale" : "+t"(accumulator) : "u"(exponent));
        __asm__ volatile("fscale" : "+t"(accumulator) : "u"(exponent));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FXTRACT -- split st(0) into exponent and significand */
static double fxtract_lat(uint64_t iteration_count)
{
    long double accumulator = 123.456L;
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        /* fxtract pushes exponent, replaces st(0) with significand */
        __asm__ volatile("fxtract\n\tfstp %%st(1)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* ========================================================================
 * CONSTANTS
 * ======================================================================== */

/* FLD1 -- push 1.0 onto stack */
static double fld1_tp(uint64_t iteration_count)
{
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fld1\n\tfstp %%st(0)" : "=t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FLDZ -- push 0.0 onto stack */
static double fldz_tp(uint64_t iteration_count)
{
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldz\n\tfstp %%st(0)" : "=t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FLDPI -- push pi */
static double fldpi_tp(uint64_t iteration_count)
{
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldpi\n\tfstp %%st(0)" : "=t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FLDL2E -- push log2(e) */
static double fldl2e_tp(uint64_t iteration_count)
{
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2e\n\tfstp %%st(0)" : "=t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FLDL2T -- push log2(10) */
static double fldl2t_tp(uint64_t iteration_count)
{
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldl2t\n\tfstp %%st(0)" : "=t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FLDLN2 -- push ln(2) */
static double fldln2_tp(uint64_t iteration_count)
{
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldln2\n\tfstp %%st(0)" : "=t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FLDLG2 -- push log10(2) */
static double fldlg2_tp(uint64_t iteration_count)
{
    long double accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
        __asm__ volatile("fldlg2\n\tfstp %%st(0)" : "=t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * CONTROL / MISC
 * ======================================================================== */

/* FINCSTP -- increment FPU stack pointer */
X87_THROUGHPUT_NOIO(fincstp_tp, "fincstp")

/* FDECSTP -- decrement FPU stack pointer */
X87_THROUGHPUT_NOIO(fdecstp_tp, "fdecstp")

/* FFREE st(7) -- mark register as empty */
static double ffree_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "ffree %st(7)\n\tffree %st(7)\n\t"
            "ffree %st(7)\n\tffree %st(7)\n\t"
            "ffree %st(7)\n\tffree %st(7)\n\t"
            "ffree %st(7)\n\tffree %st(7)"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* FNOP */
X87_THROUGHPUT_NOIO(fnop_tp, "fnop")

/* FWAIT / WAIT */
X87_THROUGHPUT_NOIO(fwait_tp, "fwait")

/* FNCLEX -- clear exceptions (no wait variant) */
static double fnclex_tp(uint64_t iteration_count)
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

/* FCMOVB -- conditional move (below/CF=1) */
static double fcmovb_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    /* Set CF=1 so FCMOVB always fires */
    __asm__ volatile("stc" ::: "cc");
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("stc\n\tfcmovb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("stc\n\tfcmovb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("stc\n\tfcmovb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("stc\n\tfcmovb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCMOVE -- conditional move (equal/ZF=1) */
static double fcmove_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        /* xor eax,eax sets ZF=1 */
        __asm__ volatile("xor %%eax, %%eax\n\tfcmove %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("xor %%eax, %%eax\n\tfcmove %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("xor %%eax, %%eax\n\tfcmove %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
        __asm__ volatile("xor %%eax, %%eax\n\tfcmove %%st(1), %%st(0)"
                         : "+t"(value_a) : "u"(value_b) : "eax", "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile long double sink = value_a; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* FCMOVNB -- conditional move (not below/CF=0) */
static double fcmovnb_lat(uint64_t iteration_count)
{
    long double value_a = 1.0L;
    long double value_b = 2.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("" : "+t"(value_a), "+u"(value_b));
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("clc\n\tfcmovnb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("clc\n\tfcmovnb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("clc\n\tfcmovnb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
        __asm__ volatile("clc\n\tfcmovnb %%st(1), %%st(0)" : "+t"(value_a) : "u"(value_b) : "cc");
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
} X87ProbeEntry;

static const X87ProbeEntry probe_table[] = {
    /* --- Move / Load / Store --- */
    {"FLD",       "st(i)",         "x87_move",   fld_reg_lat,      fld_reg_tp},
    {"FLD",       "m32",           "x87_move",   fld_m32_lat,      NULL},
    {"FLD",       "m64",           "x87_move",   fld_m64_lat,      NULL},
    {"FLD",       "m80",           "x87_move",   fld_m80_lat,      NULL},
    {"FST",       "m32",           "x87_move",   NULL,              fst_m32_tp},
    {"FST",       "m64",           "x87_move",   NULL,              fst_m64_tp},
    {"FSTP",      "m80",           "x87_move",   NULL,              fstp_m80_tp},
    {"FXCH",      "st(1)",         "x87_move",   fxch_lat,          fxch_tp},
    {"FCMOVB",    "st(0),st(1)",   "x87_move",   fcmovb_lat,        NULL},
    {"FCMOVE",    "st(0),st(1)",   "x87_move",   fcmove_lat,        NULL},
    {"FCMOVNB",   "st(0),st(1)",   "x87_move",   fcmovnb_lat,       NULL},
    {"FILD",      "m32",           "x87_move",   fild_m32_lat,      NULL},
    {"FILD",      "m64",           "x87_move",   fild_m64_lat,      NULL},
    {"FISTP",     "m32",           "x87_move",   NULL,              fistp_m32_tp},

    /* --- Arithmetic --- */
    {"FADD",      "st(0),st(0)",   "x87_arith",  fadd_lat,          fadd_tp},
    {"FSUB",      "st(0),st(0)",   "x87_arith",  fsub_lat,          fsub_tp},
    {"FMUL",      "st(0),st(0)",   "x87_arith",  fmul_lat,          fmul_tp},
    {"FDIV",      "st(0),st(0)",   "x87_arith",  fdiv_lat,          fdiv_tp},
    {"FABS",      "",              "x87_arith",  fabs_lat,          fabs_tp},
    {"FCHS",      "",              "x87_arith",  fchs_lat,          fchs_tp},
    {"FRNDINT",   "",              "x87_arith",  frndint_lat,       frndint_tp},

    /* --- Compare --- */
    {"FCOM",      "st(1)",         "x87_cmp",    fcom_lat,          NULL},
    {"FCOMI",     "st(0),st(1)",   "x87_cmp",    fcomi_lat,         NULL},
    {"FUCOM",     "st(1)",         "x87_cmp",    fucom_lat,         NULL},
    {"FTST",      "",              "x87_cmp",    ftst_lat,          NULL},
    {"FXAM",      "",              "x87_cmp",    fxam_lat,          NULL},

    /* --- Transcendental / Special --- */
    {"FSQRT",     "",              "x87_trans",  fsqrt_lat,         fsqrt_tp},
    {"FSIN",      "",              "x87_trans",  fsin_lat,          NULL},
    {"FCOS",      "",              "x87_trans",  fcos_lat,          NULL},
    {"FSINCOS",   "",              "x87_trans",  fsincos_lat,       NULL},
    {"FPTAN",     "",              "x87_trans",  fptan_lat,         NULL},
    {"FPATAN",    "",              "x87_trans",  fpatan_lat,        NULL},
    {"F2XM1",     "",              "x87_trans",  f2xm1_lat,         NULL},
    {"FYL2X",     "",              "x87_trans",  fyl2x_lat,         NULL},
    {"FYL2XP1",   "",              "x87_trans",  fyl2xp1_lat,       NULL},
    {"FSCALE",    "",              "x87_trans",  fscale_lat,         NULL},
    {"FXTRACT",   "",              "x87_trans",  fxtract_lat,        NULL},

    /* --- Constants --- */
    {"FLD1",      "",              "x87_const",  NULL,              fld1_tp},
    {"FLDZ",      "",              "x87_const",  NULL,              fldz_tp},
    {"FLDPI",     "",              "x87_const",  NULL,              fldpi_tp},
    {"FLDL2E",    "",              "x87_const",  NULL,              fldl2e_tp},
    {"FLDL2T",    "",              "x87_const",  NULL,              fldl2t_tp},
    {"FLDLN2",    "",              "x87_const",  NULL,              fldln2_tp},
    {"FLDLG2",    "",              "x87_const",  NULL,              fldlg2_tp},

    /* --- Control / Misc --- */
    {"FINCSTP",   "",              "x87_ctrl",   NULL,              fincstp_tp},
    {"FDECSTP",   "",              "x87_ctrl",   NULL,              fdecstp_tp},
    {"FFREE",     "st(7)",         "x87_ctrl",   NULL,              ffree_tp},
    {"FNOP",      "",              "x87_ctrl",   NULL,              fnop_tp},
    {"FWAIT",     "",              "x87_ctrl",   NULL,              fwait_tp},
    {"FNCLEX",    "",              "x87_ctrl",   NULL,              fnclex_tp},

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
        sm_zen_print_header("x87_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n", "x87 FPU");
    }

    /* Print CSV header */
    printf("mnemonic,operands,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const X87ProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const X87ProbeEntry *current_entry = &probe_table[entry_idx];

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
