#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_avx512_mask_exhaustive.c -- Exhaustive AVX-512 mask register (k0-k7)
 * instruction measurement for Zen 4.
 *
 * Measures latency (4-deep dependency chain) and throughput (8 independent
 * streams) for every mask register instruction form.
 *
 * Mask registers are GPR-like; we use raw inline asm with named k-registers.
 * Since GCC/clang have no dedicated "k" constraint for mask regs in all
 * contexts, we pin k1-k7 explicitly.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx512f -mavx512bw \
 *     -mavx512dq -mavx512vl -mavx512cd -mavx512bf16 -mfma -mf16c \
 *     -fno-omit-frame-pointer -include x86intrin.h \
 *     -I. common.c probe_avx512_mask_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_avx512_mask_exhaustive
 */

/* ========================================================================
 * MASK REGISTER MACROS
 *
 * Mask instructions operate on k-registers. We use explicit register names
 * in inline asm since there's no portable k-register constraint.
 *
 * Latency: 4-deep chain on k1 (each result feeds next instruction).
 * Throughput: 4 independent k-register streams (k1,k2,k3,k4).
 *   (We use 4 instead of 8 because only k1-k7 are available and some
 *    instructions need a second source operand.)
 * ======================================================================== */

/*
 * LAT_MASK_BINARY: latency for "op k_src, k_dst, k_dst" style instructions.
 * k1 = accumulator, k2 = constant source.
 */
#define LAT_MASK_BINARY(func_name, asm_instr)                                 \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __asm__ volatile("movw $0x5555, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1"); \
    __asm__ volatile("movw $0x3333, %%ax\n\tkmovw %%eax, %%k2" ::: "eax", "k2"); \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_instr " %%k2, %%k1, %%k1" ::: "k1");            \
        __asm__ volatile(asm_instr " %%k2, %%k1, %%k1" ::: "k1");            \
        __asm__ volatile(asm_instr " %%k2, %%k1, %%k1" ::: "k1");            \
        __asm__ volatile(asm_instr " %%k2, %%k1, %%k1" ::: "k1");            \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_MASK_BINARY: throughput for binary mask op, 4 independent streams.
 * k1-k4 = accumulators, k5 = constant source.
 */
#define TP_MASK_BINARY(func_name, asm_instr)                                  \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __asm__ volatile(                                                         \
        "movw $0x5555, %%ax\n\t"                                              \
        "kmovw %%eax, %%k1\n\t"                                               \
        "kmovw %%eax, %%k2\n\t"                                               \
        "kmovw %%eax, %%k3\n\t"                                               \
        "kmovw %%eax, %%k4\n\t"                                               \
        "movw $0x3333, %%ax\n\t"                                              \
        "kmovw %%eax, %%k5"                                                   \
        ::: "eax", "k1", "k2", "k3", "k4", "k5");                            \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(                                                     \
            asm_instr " %%k5, %%k1, %%k1\n\t"                                \
            asm_instr " %%k5, %%k2, %%k2\n\t"                                \
            asm_instr " %%k5, %%k3, %%k3\n\t"                                \
            asm_instr " %%k5, %%k4, %%k4"                                    \
            ::: "k1", "k2", "k3", "k4");                                     \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * LAT_MASK_UNARY: latency for unary mask op (e.g. KNOT).
 * k1 = accumulator.
 */
#define LAT_MASK_UNARY(func_name, asm_instr)                                  \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __asm__ volatile("movw $0x5555, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1"); \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_instr " %%k1, %%k1" ::: "k1");                  \
        __asm__ volatile(asm_instr " %%k1, %%k1" ::: "k1");                  \
        __asm__ volatile(asm_instr " %%k1, %%k1" ::: "k1");                  \
        __asm__ volatile(asm_instr " %%k1, %%k1" ::: "k1");                  \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_MASK_UNARY: throughput for unary mask op, 4 independent streams.
 */
#define TP_MASK_UNARY(func_name, asm_instr)                                   \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __asm__ volatile(                                                         \
        "movw $0x5555, %%ax\n\t"                                              \
        "kmovw %%eax, %%k1\n\t"                                               \
        "kmovw %%eax, %%k2\n\t"                                               \
        "kmovw %%eax, %%k3\n\t"                                               \
        "kmovw %%eax, %%k4"                                                   \
        ::: "eax", "k1", "k2", "k3", "k4");                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(                                                     \
            asm_instr " %%k1, %%k1\n\t"                                       \
            asm_instr " %%k2, %%k2\n\t"                                       \
            asm_instr " %%k3, %%k3\n\t"                                       \
            asm_instr " %%k4, %%k4"                                           \
            ::: "k1", "k2", "k3", "k4");                                     \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * LAT_MASK_SHIFT: latency for shift with immediate: "op imm, k_src, k_dst"
 */
#define LAT_MASK_SHIFT(func_name, asm_instr)                                  \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __asm__ volatile("movw $0x5555, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1"); \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(asm_instr " $1, %%k1, %%k1" ::: "k1");              \
        __asm__ volatile(asm_instr " $1, %%k1, %%k1" ::: "k1");              \
        __asm__ volatile(asm_instr " $1, %%k1, %%k1" ::: "k1");              \
        __asm__ volatile(asm_instr " $1, %%k1, %%k1" ::: "k1");              \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_MASK_SHIFT: throughput for shift, 4 independent streams.
 */
#define TP_MASK_SHIFT(func_name, asm_instr)                                   \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __asm__ volatile(                                                         \
        "movw $0x5555, %%ax\n\t"                                              \
        "kmovw %%eax, %%k1\n\t"                                               \
        "kmovw %%eax, %%k2\n\t"                                               \
        "kmovw %%eax, %%k3\n\t"                                               \
        "kmovw %%eax, %%k4"                                                   \
        ::: "eax", "k1", "k2", "k3", "k4");                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(                                                     \
            asm_instr " $1, %%k1, %%k1\n\t"                                   \
            asm_instr " $1, %%k2, %%k2\n\t"                                   \
            asm_instr " $1, %%k3, %%k3\n\t"                                   \
            asm_instr " $1, %%k4, %%k4"                                       \
            ::: "k1", "k2", "k3", "k4");                                     \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/*
 * TP_MASK_TEST: throughput for KTEST/KORTEST that sets flags (no k output).
 * Uses 4 independent source pairs to avoid false deps on flags.
 */
#define TP_MASK_TEST(func_name, asm_instr)                                    \
static double func_name(uint64_t iteration_count)                             \
{                                                                             \
    __asm__ volatile(                                                         \
        "movw $0x5555, %%ax\n\t"                                              \
        "kmovw %%eax, %%k1\n\t"                                               \
        "kmovw %%eax, %%k2\n\t"                                               \
        "kmovw %%eax, %%k3\n\t"                                               \
        "kmovw %%eax, %%k4"                                                   \
        ::: "eax", "k1", "k2", "k3", "k4");                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                  \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {     \
        __asm__ volatile(                                                     \
            asm_instr " %%k1, %%k1\n\t"                                       \
            asm_instr " %%k2, %%k2\n\t"                                       \
            asm_instr " %%k3, %%k3\n\t"                                       \
            asm_instr " %%k4, %%k4"                                           \
            ::: "cc");                                                        \
    }                                                                         \
    uint64_t tsc_stop = sm_zen_tsc_end();                                     \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);    \
}

/* ========================================================================
 * MASK ARITHMETIC -- B (8-bit)
 * ======================================================================== */

LAT_MASK_BINARY(kaddb_lat, "kaddb")
TP_MASK_BINARY(kaddb_tp, "kaddb")

LAT_MASK_BINARY(kandb_lat, "kandb")
TP_MASK_BINARY(kandb_tp, "kandb")

LAT_MASK_BINARY(kandnb_lat, "kandnb")
TP_MASK_BINARY(kandnb_tp, "kandnb")

LAT_MASK_BINARY(korb_lat, "korb")
TP_MASK_BINARY(korb_tp, "korb")

LAT_MASK_BINARY(kxorb_lat, "kxorb")
TP_MASK_BINARY(kxorb_tp, "kxorb")

LAT_MASK_BINARY(kxnorb_lat, "kxnorb")
TP_MASK_BINARY(kxnorb_tp, "kxnorb")

LAT_MASK_UNARY(knotb_lat, "knotb")
TP_MASK_UNARY(knotb_tp, "knotb")

LAT_MASK_SHIFT(kshiftlb_lat, "kshiftlb")
TP_MASK_SHIFT(kshiftlb_tp, "kshiftlb")

LAT_MASK_SHIFT(kshiftrb_lat, "kshiftrb")
TP_MASK_SHIFT(kshiftrb_tp, "kshiftrb")

/* ========================================================================
 * MASK ARITHMETIC -- W (16-bit)
 * ======================================================================== */

LAT_MASK_BINARY(kaddw_lat, "kaddw")
TP_MASK_BINARY(kaddw_tp, "kaddw")

LAT_MASK_BINARY(kandw_lat, "kandw")
TP_MASK_BINARY(kandw_tp, "kandw")

LAT_MASK_BINARY(kandnw_lat, "kandnw")
TP_MASK_BINARY(kandnw_tp, "kandnw")

LAT_MASK_BINARY(korw_lat, "korw")
TP_MASK_BINARY(korw_tp, "korw")

LAT_MASK_BINARY(kxorw_lat, "kxorw")
TP_MASK_BINARY(kxorw_tp, "kxorw")

LAT_MASK_BINARY(kxnorw_lat, "kxnorw")
TP_MASK_BINARY(kxnorw_tp, "kxnorw")

LAT_MASK_UNARY(knotw_lat, "knotw")
TP_MASK_UNARY(knotw_tp, "knotw")

LAT_MASK_SHIFT(kshiftlw_lat, "kshiftlw")
TP_MASK_SHIFT(kshiftlw_tp, "kshiftlw")

LAT_MASK_SHIFT(kshiftrw_lat, "kshiftrw")
TP_MASK_SHIFT(kshiftrw_tp, "kshiftrw")

/* ========================================================================
 * MASK ARITHMETIC -- D (32-bit)
 * ======================================================================== */

LAT_MASK_BINARY(kaddd_lat, "kaddd")
TP_MASK_BINARY(kaddd_tp, "kaddd")

LAT_MASK_BINARY(kandd_lat, "kandd")
TP_MASK_BINARY(kandd_tp, "kandd")

LAT_MASK_BINARY(kandnd_lat, "kandnd")
TP_MASK_BINARY(kandnd_tp, "kandnd")

LAT_MASK_BINARY(kord_lat, "kord")
TP_MASK_BINARY(kord_tp, "kord")

LAT_MASK_BINARY(kxord_lat, "kxord")
TP_MASK_BINARY(kxord_tp, "kxord")

LAT_MASK_BINARY(kxnord_lat, "kxnord")
TP_MASK_BINARY(kxnord_tp, "kxnord")

LAT_MASK_UNARY(knotd_lat, "knotd")
TP_MASK_UNARY(knotd_tp, "knotd")

LAT_MASK_SHIFT(kshiftld_lat, "kshiftld")
TP_MASK_SHIFT(kshiftld_tp, "kshiftld")

LAT_MASK_SHIFT(kshiftrd_lat, "kshiftrd")
TP_MASK_SHIFT(kshiftrd_tp, "kshiftrd")

/* ========================================================================
 * MASK ARITHMETIC -- Q (64-bit)
 * ======================================================================== */

LAT_MASK_BINARY(kaddq_lat, "kaddq")
TP_MASK_BINARY(kaddq_tp, "kaddq")

LAT_MASK_BINARY(kandq_lat, "kandq")
TP_MASK_BINARY(kandq_tp, "kandq")

LAT_MASK_BINARY(kandnq_lat, "kandnq")
TP_MASK_BINARY(kandnq_tp, "kandnq")

LAT_MASK_BINARY(korq_lat, "korq")
TP_MASK_BINARY(korq_tp, "korq")

LAT_MASK_BINARY(kxorq_lat, "kxorq")
TP_MASK_BINARY(kxorq_tp, "kxorq")

LAT_MASK_BINARY(kxnorq_lat, "kxnorq")
TP_MASK_BINARY(kxnorq_tp, "kxnorq")

LAT_MASK_UNARY(knotq_lat, "knotq")
TP_MASK_UNARY(knotq_tp, "knotq")

LAT_MASK_SHIFT(kshiftlq_lat, "kshiftlq")
TP_MASK_SHIFT(kshiftlq_tp, "kshiftlq")

LAT_MASK_SHIFT(kshiftrq_lat, "kshiftrq")
TP_MASK_SHIFT(kshiftrq_tp, "kshiftrq")

/* ========================================================================
 * MASK MOVE -- k,k (mask-to-mask)
 * ======================================================================== */

LAT_MASK_UNARY(kmovb_kk_lat, "kmovb")
TP_MASK_UNARY(kmovb_kk_tp, "kmovb")

LAT_MASK_UNARY(kmovw_kk_lat, "kmovw")
TP_MASK_UNARY(kmovw_kk_tp, "kmovw")

LAT_MASK_UNARY(kmovd_kk_lat, "kmovd")
TP_MASK_UNARY(kmovd_kk_tp, "kmovd")

LAT_MASK_UNARY(kmovq_kk_lat, "kmovq")
TP_MASK_UNARY(kmovq_kk_tp, "kmovq")

/* ========================================================================
 * MASK MOVE -- GPR <-> k
 * ======================================================================== */

/* KMOV k, r32 (GPR -> mask): throughput only */
static double kmovw_gpr_to_k_tp(uint64_t iteration_count)
{
    uint32_t gpr_source = 0x5555;
    __asm__ volatile("" : "+r"(gpr_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovw %0, %%k1\n\t"
            "kmovw %0, %%k2\n\t"
            "kmovw %0, %%k3\n\t"
            "kmovw %0, %%k4"
            :: "r"(gpr_source) : "k1", "k2", "k3", "k4");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double kmovb_gpr_to_k_tp(uint64_t iteration_count)
{
    uint32_t gpr_source = 0x55;
    __asm__ volatile("" : "+r"(gpr_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovb %0, %%k1\n\t"
            "kmovb %0, %%k2\n\t"
            "kmovb %0, %%k3\n\t"
            "kmovb %0, %%k4"
            :: "r"(gpr_source) : "k1", "k2", "k3", "k4");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double kmovd_gpr_to_k_tp(uint64_t iteration_count)
{
    uint32_t gpr_source = 0x55555555;
    __asm__ volatile("" : "+r"(gpr_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovd %0, %%k1\n\t"
            "kmovd %0, %%k2\n\t"
            "kmovd %0, %%k3\n\t"
            "kmovd %0, %%k4"
            :: "r"(gpr_source) : "k1", "k2", "k3", "k4");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double kmovq_gpr_to_k_tp(uint64_t iteration_count)
{
    uint64_t gpr_source = 0x5555555555555555ULL;
    __asm__ volatile("" : "+r"(gpr_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovq %0, %%k1\n\t"
            "kmovq %0, %%k2\n\t"
            "kmovq %0, %%k3\n\t"
            "kmovq %0, %%k4"
            :: "r"(gpr_source) : "k1", "k2", "k3", "k4");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* KMOV r32, k (mask -> GPR): throughput only */
static double kmovw_k_to_gpr_tp(uint64_t iteration_count)
{
    uint32_t gpr_dst_0, gpr_dst_1, gpr_dst_2, gpr_dst_3;
    __asm__ volatile("movw $0x5555, %%ax\n\tkmovw %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovw %%k1, %0\n\t"
            "kmovw %%k1, %1\n\t"
            "kmovw %%k1, %2\n\t"
            "kmovw %%k1, %3"
            : "=r"(gpr_dst_0), "=r"(gpr_dst_1), "=r"(gpr_dst_2), "=r"(gpr_dst_3));
        __asm__ volatile("" : "+r"(gpr_dst_0), "+r"(gpr_dst_1),
                              "+r"(gpr_dst_2), "+r"(gpr_dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+r"(gpr_dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double kmovb_k_to_gpr_tp(uint64_t iteration_count)
{
    uint32_t gpr_dst_0, gpr_dst_1, gpr_dst_2, gpr_dst_3;
    __asm__ volatile("movw $0x55, %%ax\n\tkmovb %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovb %%k1, %0\n\t"
            "kmovb %%k1, %1\n\t"
            "kmovb %%k1, %2\n\t"
            "kmovb %%k1, %3"
            : "=r"(gpr_dst_0), "=r"(gpr_dst_1), "=r"(gpr_dst_2), "=r"(gpr_dst_3));
        __asm__ volatile("" : "+r"(gpr_dst_0), "+r"(gpr_dst_1),
                              "+r"(gpr_dst_2), "+r"(gpr_dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+r"(gpr_dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double kmovd_k_to_gpr_tp(uint64_t iteration_count)
{
    uint32_t gpr_dst_0, gpr_dst_1, gpr_dst_2, gpr_dst_3;
    __asm__ volatile("movl $0x55555555, %%eax\n\tkmovd %%eax, %%k1" ::: "eax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovd %%k1, %0\n\t"
            "kmovd %%k1, %1\n\t"
            "kmovd %%k1, %2\n\t"
            "kmovd %%k1, %3"
            : "=r"(gpr_dst_0), "=r"(gpr_dst_1), "=r"(gpr_dst_2), "=r"(gpr_dst_3));
        __asm__ volatile("" : "+r"(gpr_dst_0), "+r"(gpr_dst_1),
                              "+r"(gpr_dst_2), "+r"(gpr_dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+r"(gpr_dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double kmovq_k_to_gpr_tp(uint64_t iteration_count)
{
    uint64_t gpr_dst_0, gpr_dst_1, gpr_dst_2, gpr_dst_3;
    __asm__ volatile("movq $0x5555555555555555, %%rax\n\tkmovq %%rax, %%k1" ::: "rax", "k1");
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovq %%k1, %0\n\t"
            "kmovq %%k1, %1\n\t"
            "kmovq %%k1, %2\n\t"
            "kmovq %%k1, %3"
            : "=r"(gpr_dst_0), "=r"(gpr_dst_1), "=r"(gpr_dst_2), "=r"(gpr_dst_3));
        __asm__ volatile("" : "+r"(gpr_dst_0), "+r"(gpr_dst_1),
                              "+r"(gpr_dst_2), "+r"(gpr_dst_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+r"(gpr_dst_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * MASK MOVE -- k, mem (mask load from memory)
 * ======================================================================== */

static double kmovb_load_tp(uint64_t iteration_count)
{
    uint8_t mask_mem __attribute__((aligned(64))) = 0x55;
    __asm__ volatile("" : "+m"(mask_mem));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovb %0, %%k1\n\t"
            "kmovb %0, %%k2\n\t"
            "kmovb %0, %%k3\n\t"
            "kmovb %0, %%k4"
            :: "m"(mask_mem) : "k1", "k2", "k3", "k4");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double kmovw_load_tp(uint64_t iteration_count)
{
    uint16_t mask_mem __attribute__((aligned(64))) = 0x5555;
    __asm__ volatile("" : "+m"(mask_mem));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "kmovw %0, %%k1\n\t"
            "kmovw %0, %%k2\n\t"
            "kmovw %0, %%k3\n\t"
            "kmovw %0, %%k4"
            :: "m"(mask_mem) : "k1", "k2", "k3", "k4");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * MASK TEST (sets flags)
 * ======================================================================== */

TP_MASK_TEST(ktestb_tp, "ktestb")
TP_MASK_TEST(ktestw_tp, "ktestw")
TP_MASK_TEST(ktestd_tp, "ktestd")
TP_MASK_TEST(ktestq_tp, "ktestq")

TP_MASK_TEST(kortestb_tp, "kortestb")
TP_MASK_TEST(kortestw_tp, "kortestw")
TP_MASK_TEST(kortestd_tp, "kortestd")
TP_MASK_TEST(kortestq_tp, "kortestq")

/* ========================================================================
 * VECTOR COMPARE -> MASK
 * ======================================================================== */

/* VPCMPEQD zmm,zmm -> k (measure k-register write throughput) */
static double vpcmpeqd_zmm_to_k_tp(uint64_t iteration_count)
{
    __m512i src_a = _mm512_set1_epi32(1);
    __m512i src_b = _mm512_set1_epi32(1);
    uint32_t mask_result_0, mask_result_1, mask_result_2, mask_result_3;
    __asm__ volatile("" : "+x"(src_a), "+x"(src_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "vpcmpeqd %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_0) : "x"(src_a), "x"(src_b));
        __asm__ volatile(
            "vpcmpeqd %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_1) : "x"(src_a), "x"(src_b));
        __asm__ volatile(
            "vpcmpeqd %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_2) : "x"(src_a), "x"(src_b));
        __asm__ volatile(
            "vpcmpeqd %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_3) : "x"(src_a), "x"(src_b));
        __asm__ volatile("" : "+r"(mask_result_0), "+r"(mask_result_1),
                              "+r"(mask_result_2), "+r"(mask_result_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+r"(mask_result_0));
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VCMPPS zmm,zmm,imm -> k */
static double vcmpps_zmm_to_k_tp(uint64_t iteration_count)
{
    __m512 src_a = _mm512_set1_ps(1.0001f);
    __m512 src_b = _mm512_set1_ps(1.0001f);
    uint32_t mask_result_0, mask_result_1, mask_result_2, mask_result_3;
    __asm__ volatile("" : "+x"(src_a), "+x"(src_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "vcmpps $0, %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_0) : "x"(src_a), "x"(src_b));
        __asm__ volatile(
            "vcmpps $0, %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_1) : "x"(src_a), "x"(src_b));
        __asm__ volatile(
            "vcmpps $0, %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_2) : "x"(src_a), "x"(src_b));
        __asm__ volatile(
            "vcmpps $0, %2, %1, %%k1\n\tkmovw %%k1, %0"
            : "=r"(mask_result_3) : "x"(src_a), "x"(src_b));
        __asm__ volatile("" : "+r"(mask_result_0), "+r"(mask_result_1),
                              "+r"(mask_result_2), "+r"(mask_result_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("" : "+r"(mask_result_0));
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
} MaskProbeEntry;

static const MaskProbeEntry mask_probe_table[] = {
    /* ---- Mask Arithmetic B (8-bit) ---- */
    {"KADDB",      "k,k,k",     "mask_arith",  kaddb_lat,      kaddb_tp},
    {"KANDB",      "k,k,k",     "mask_arith",  kandb_lat,      kandb_tp},
    {"KANDNB",     "k,k,k",     "mask_arith",  kandnb_lat,     kandnb_tp},
    {"KORB",       "k,k,k",     "mask_arith",  korb_lat,       korb_tp},
    {"KXORB",      "k,k,k",     "mask_arith",  kxorb_lat,      kxorb_tp},
    {"KXNORB",     "k,k,k",     "mask_arith",  kxnorb_lat,     kxnorb_tp},
    {"KNOTB",      "k,k",       "mask_arith",  knotb_lat,      knotb_tp},
    {"KSHIFTLB",   "k,k,imm",   "mask_shift",  kshiftlb_lat,   kshiftlb_tp},
    {"KSHIFTRB",   "k,k,imm",   "mask_shift",  kshiftrb_lat,   kshiftrb_tp},

    /* ---- Mask Arithmetic W (16-bit) ---- */
    {"KADDW",      "k,k,k",     "mask_arith",  kaddw_lat,      kaddw_tp},
    {"KANDW",      "k,k,k",     "mask_arith",  kandw_lat,      kandw_tp},
    {"KANDNW",     "k,k,k",     "mask_arith",  kandnw_lat,     kandnw_tp},
    {"KORW",       "k,k,k",     "mask_arith",  korw_lat,       korw_tp},
    {"KXORW",      "k,k,k",     "mask_arith",  kxorw_lat,      kxorw_tp},
    {"KXNORW",     "k,k,k",     "mask_arith",  kxnorw_lat,     kxnorw_tp},
    {"KNOTW",      "k,k",       "mask_arith",  knotw_lat,      knotw_tp},
    {"KSHIFTLW",   "k,k,imm",   "mask_shift",  kshiftlw_lat,   kshiftlw_tp},
    {"KSHIFTRW",   "k,k,imm",   "mask_shift",  kshiftrw_lat,   kshiftrw_tp},

    /* ---- Mask Arithmetic D (32-bit) ---- */
    {"KADDD",      "k,k,k",     "mask_arith",  kaddd_lat,      kaddd_tp},
    {"KANDD",      "k,k,k",     "mask_arith",  kandd_lat,      kandd_tp},
    {"KANDND",     "k,k,k",     "mask_arith",  kandnd_lat,     kandnd_tp},
    {"KORD",       "k,k,k",     "mask_arith",  kord_lat,       kord_tp},
    {"KXORD",      "k,k,k",     "mask_arith",  kxord_lat,      kxord_tp},
    {"KXNORD",     "k,k,k",     "mask_arith",  kxnord_lat,     kxnord_tp},
    {"KNOTD",      "k,k",       "mask_arith",  knotd_lat,      knotd_tp},
    {"KSHIFTLD",   "k,k,imm",   "mask_shift",  kshiftld_lat,   kshiftld_tp},
    {"KSHIFTRD",   "k,k,imm",   "mask_shift",  kshiftrd_lat,   kshiftrd_tp},

    /* ---- Mask Arithmetic Q (64-bit) ---- */
    {"KADDQ",      "k,k,k",     "mask_arith",  kaddq_lat,      kaddq_tp},
    {"KANDQ",      "k,k,k",     "mask_arith",  kandq_lat,      kandq_tp},
    {"KANDNQ",     "k,k,k",     "mask_arith",  kandnq_lat,     kandnq_tp},
    {"KORQ",       "k,k,k",     "mask_arith",  korq_lat,       korq_tp},
    {"KXORQ",      "k,k,k",     "mask_arith",  kxorq_lat,      kxorq_tp},
    {"KXNORQ",     "k,k,k",     "mask_arith",  kxnorq_lat,     kxnorq_tp},
    {"KNOTQ",      "k,k",       "mask_arith",  knotq_lat,      knotq_tp},
    {"KSHIFTLQ",   "k,k,imm",   "mask_shift",  kshiftlq_lat,   kshiftlq_tp},
    {"KSHIFTRQ",   "k,k,imm",   "mask_shift",  kshiftrq_lat,   kshiftrq_tp},

    /* ---- Mask Move k,k ---- */
    {"KMOVB",      "k,k",       "mask_move",   kmovb_kk_lat,   kmovb_kk_tp},
    {"KMOVW",      "k,k",       "mask_move",   kmovw_kk_lat,   kmovw_kk_tp},
    {"KMOVD",      "k,k",       "mask_move",   kmovd_kk_lat,   kmovd_kk_tp},
    {"KMOVQ",      "k,k",       "mask_move",   kmovq_kk_lat,   kmovq_kk_tp},

    /* ---- Mask Move GPR->k ---- */
    {"KMOVB",      "k,r32",     "mask_move",   NULL,            kmovb_gpr_to_k_tp},
    {"KMOVW",      "k,r32",     "mask_move",   NULL,            kmovw_gpr_to_k_tp},
    {"KMOVD",      "k,r32",     "mask_move",   NULL,            kmovd_gpr_to_k_tp},
    {"KMOVQ",      "k,r64",     "mask_move",   NULL,            kmovq_gpr_to_k_tp},

    /* ---- Mask Move k->GPR ---- */
    {"KMOVB",      "r32,k",     "mask_move",   NULL,            kmovb_k_to_gpr_tp},
    {"KMOVW",      "r32,k",     "mask_move",   NULL,            kmovw_k_to_gpr_tp},
    {"KMOVD",      "r32,k",     "mask_move",   NULL,            kmovd_k_to_gpr_tp},
    {"KMOVQ",      "r64,k",     "mask_move",   NULL,            kmovq_k_to_gpr_tp},

    /* ---- Mask Load from memory ---- */
    {"KMOVB",      "k,m8",      "mask_load",   NULL,            kmovb_load_tp},
    {"KMOVW",      "k,m16",     "mask_load",   NULL,            kmovw_load_tp},

    /* ---- Mask Test (sets flags) ---- */
    {"KTESTB",     "k,k",       "mask_test",   NULL,            ktestb_tp},
    {"KTESTW",     "k,k",       "mask_test",   NULL,            ktestw_tp},
    {"KTESTD",     "k,k",       "mask_test",   NULL,            ktestd_tp},
    {"KTESTQ",     "k,k",       "mask_test",   NULL,            ktestq_tp},
    {"KORTESTB",   "k,k",       "mask_test",   NULL,            kortestb_tp},
    {"KORTESTW",   "k,k",       "mask_test",   NULL,            kortestw_tp},
    {"KORTESTD",   "k,k",       "mask_test",   NULL,            kortestd_tp},
    {"KORTESTQ",   "k,k",       "mask_test",   NULL,            kortestq_tp},

    /* ---- Vector Compare -> Mask ---- */
    {"VPCMPEQD",   "zmm,zmm->k","vec_to_mask", NULL,            vpcmpeqd_zmm_to_k_tp},
    {"VCMPPS",     "zmm,zmm->k","vec_to_mask", NULL,            vcmpps_zmm_to_k_tp},

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
        sm_zen_print_header("avx512_mask_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring AVX-512 mask register (k0-k7) instruction forms...\n\n");
    }

    /* Count entries */
    int total_entries = 0;
    for (int count_idx = 0; mask_probe_table[count_idx].mnemonic != NULL; count_idx++)
        total_entries++;

    if (!csv_mode)
        printf("Total instruction forms: %d\n\n", total_entries);

    /* CSV header */
    printf("mnemonic,operands,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; mask_probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const MaskProbeEntry *current_entry = &mask_probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; mask_probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const MaskProbeEntry *current_entry = &mask_probe_table[entry_idx];

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
