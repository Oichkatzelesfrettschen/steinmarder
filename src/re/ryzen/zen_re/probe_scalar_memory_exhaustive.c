#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_scalar_memory_exhaustive.c -- Exhaustive scalar memory instruction measurement.
 *
 * Measures latency and throughput for all scalar instructions with memory
 * operands on Zen 4: loads, stores, RMW, atomic/LOCK, prefetch, string ops.
 *
 * Uses a 4 KB L1-resident buffer for latency measurements and independent
 * addresses for throughput measurements.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_scalar_memory_exhaustive.c -lm \
 *     -o ../../../../build/bin/zen_probe_scalar_memory_exhaustive
 */

/* ========================================================================
 * BUFFER SETUP
 *
 * 4 KB buffer aligned to cache line for L1-resident measurements.
 * For pointer-chase latency tests, we build a sequential linked list
 * within the buffer.
 * ======================================================================== */

#define L1_BUFFER_SIZE    4096
#define L1_BUFFER_U64     (L1_BUFFER_SIZE / sizeof(uint64_t))
#define THROUGHPUT_SLOTS  64  /* independent addresses for TP tests */

/* Global buffers -- allocated in main */
static volatile uint64_t *latency_buffer;
static volatile uint64_t *throughput_buffer;
/* Secondary buffers for string ops */
static volatile uint8_t *string_source_buffer;
static volatile uint8_t *string_destination_buffer;

static void initialize_latency_buffer(void)
{
    /* Simple sequential walk for L1 latency -- each element points to next */
    for (uint64_t slot_idx = 0; slot_idx < L1_BUFFER_U64; slot_idx++) {
        latency_buffer[slot_idx] = (slot_idx + 1) % L1_BUFFER_U64;
    }
    /* Also store non-zero values for arithmetic tests */
    latency_buffer[0] = 1;
}

/* ========================================================================
 * MACRO TEMPLATES FOR MEMORY OPERANDS
 * ======================================================================== */

/*
 * LOAD_LATENCY: measure load latency via pointer chase.
 *   Reads from buffer[acc], acc = result, forming a chain.
 */
#define LOAD_LATENCY(func_name, load_asm, type_cast)                           \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t accumulator = 0;                                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __asm__ volatile(load_asm : "=r"(accumulator) : "r"(latency_buffer + (accumulator & 0x1F))); \
        __asm__ volatile(load_asm : "=r"(accumulator) : "r"(latency_buffer + (accumulator & 0x1F))); \
        __asm__ volatile(load_asm : "=r"(accumulator) : "r"(latency_buffer + (accumulator & 0x1F))); \
        __asm__ volatile(load_asm : "=r"(accumulator) : "r"(latency_buffer + (accumulator & 0x1F))); \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile uint64_t sink = accumulator; (void)sink;                           \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);     \
}

/*
 * LOAD_THROUGHPUT: measure load throughput with independent addresses.
 */
#define LOAD_THROUGHPUT(func_name, load_asm)                                    \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t result_0, result_1, result_2, result_3;                           \
    uint64_t result_4, result_5, result_6, result_7;                           \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __asm__ volatile(load_asm : "=r"(result_0) : "r"(throughput_buffer + 0));  \
        __asm__ volatile(load_asm : "=r"(result_1) : "r"(throughput_buffer + 8));  \
        __asm__ volatile(load_asm : "=r"(result_2) : "r"(throughput_buffer + 16)); \
        __asm__ volatile(load_asm : "=r"(result_3) : "r"(throughput_buffer + 24)); \
        __asm__ volatile(load_asm : "=r"(result_4) : "r"(throughput_buffer + 32)); \
        __asm__ volatile(load_asm : "=r"(result_5) : "r"(throughput_buffer + 40)); \
        __asm__ volatile(load_asm : "=r"(result_6) : "r"(throughput_buffer + 48)); \
        __asm__ volatile(load_asm : "=r"(result_7) : "r"(throughput_buffer + 56)); \
        __asm__ volatile("" : "+r"(result_0), "+r"(result_1),                  \
                              "+r"(result_2), "+r"(result_3),                  \
                              "+r"(result_4), "+r"(result_5),                  \
                              "+r"(result_6), "+r"(result_7));                 \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile uint64_t sink = result_0 ^ result_4; (void)sink;                  \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/*
 * STORE_THROUGHPUT: measure store throughput to independent addresses.
 */
#define STORE_THROUGHPUT(func_name, store_asm)                                  \
static double func_name(uint64_t iteration_count)                              \
{                                                                              \
    uint64_t store_value = 0xDEADBEEFCAFEBABEULL;                             \
    __asm__ volatile("" : "+r"(store_value));                                  \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {      \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 0),  "r"(store_value) : "memory"); \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 8),  "r"(store_value) : "memory"); \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 16), "r"(store_value) : "memory"); \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 24), "r"(store_value) : "memory"); \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 32), "r"(store_value) : "memory"); \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 40), "r"(store_value) : "memory"); \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 48), "r"(store_value) : "memory"); \
        __asm__ volatile(store_asm : : "r"(throughput_buffer + 56), "r"(store_value) : "memory"); \
    }                                                                          \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);     \
}

/* ========================================================================
 * MOV r,m -- LOADS (various sizes)
 * ======================================================================== */

/* MOV r64,[m64] -- 64-bit load */
LOAD_LATENCY(mov_r64_m64_lat, "mov (%1), %0", uint64_t)
LOAD_THROUGHPUT(mov_r64_m64_tp, "mov (%1), %0")

/* MOV r32,[m32] -- 32-bit load (zero-extends) */
LOAD_LATENCY(mov_r32_m32_lat, "movl (%1), %k0", uint32_t)
LOAD_THROUGHPUT(mov_r32_m32_tp, "movl (%1), %k0")

/* MOV r16,[m16] -- 16-bit load */
LOAD_LATENCY(mov_r16_m16_lat, "movw (%1), %w0", uint16_t)
LOAD_THROUGHPUT(mov_r16_m16_tp, "movw (%1), %w0")

/* MOV r8,[m8] -- 8-bit load */
LOAD_LATENCY(mov_r8_m8_lat, "movb (%1), %b0", uint8_t)
LOAD_THROUGHPUT(mov_r8_m8_tp, "movb (%1), %b0")

/* MOVZX r64,[m8] -- zero-extending byte load */
LOAD_LATENCY(movzx_r64_m8_lat, "movzbq (%1), %0", uint8_t)
LOAD_THROUGHPUT(movzx_r64_m8_tp, "movzbq (%1), %0")

/* MOVZX r64,[m16] -- zero-extending word load */
LOAD_LATENCY(movzx_r64_m16_lat, "movzwq (%1), %0", uint16_t)
LOAD_THROUGHPUT(movzx_r64_m16_tp, "movzwq (%1), %0")

/* MOVSX r64,[m8] -- sign-extending byte load */
LOAD_LATENCY(movsx_r64_m8_lat, "movsbq (%1), %0", int8_t)
LOAD_THROUGHPUT(movsx_r64_m8_tp, "movsbq (%1), %0")

/* MOVSX r64,[m16] -- sign-extending word load */
LOAD_LATENCY(movsx_r64_m16_lat, "movswq (%1), %0", int16_t)
LOAD_THROUGHPUT(movsx_r64_m16_tp, "movswq (%1), %0")

/* ========================================================================
 * MOV m,r / MOV m,imm -- STORES (various sizes)
 * ======================================================================== */

/* MOV [m64],r64 -- 64-bit store */
STORE_THROUGHPUT(mov_m64_r64_tp, "mov %1, (%0)")

/* MOV [m32],r32 -- 32-bit store */
static double mov_m32_r32_tp(uint64_t iteration_count)
{
    uint32_t store_value = 0xDEADBEEF;
    __asm__ volatile("" : "+r"(store_value));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 0),  "r"(store_value) : "memory");
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 8),  "r"(store_value) : "memory");
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 16), "r"(store_value) : "memory");
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 24), "r"(store_value) : "memory");
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 32), "r"(store_value) : "memory");
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 40), "r"(store_value) : "memory");
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 48), "r"(store_value) : "memory");
        __asm__ volatile("movl %1, (%0)" : : "r"(throughput_buffer + 56), "r"(store_value) : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOV [m64],imm32 -- store immediate */
static double mov_m64_imm_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 0)  : "memory");
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 8)  : "memory");
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 16) : "memory");
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 24) : "memory");
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 32) : "memory");
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 40) : "memory");
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 48) : "memory");
        __asm__ volatile("movq $0x42, (%0)" : : "r"(throughput_buffer + 56) : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOV [m16],r16 -- 16-bit store */
static double mov_m16_r16_tp(uint64_t iteration_count)
{
    uint16_t store_value = 0xBEEF;
    __asm__ volatile("" : "+r"(store_value));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 0),  "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 8),  "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 16), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 24), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 32), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 40), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 48), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movw %w1, (%0)" : : "r"(throughput_buffer + 56), "r"((uint64_t)store_value) : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* MOV [m8],r8 -- 8-bit store */
static double mov_m8_r8_tp(uint64_t iteration_count)
{
    uint8_t store_value = 0xAB;
    __asm__ volatile("" : "+r"(store_value));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 0),  "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 8),  "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 16), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 24), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 32), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 40), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 48), "r"((uint64_t)store_value) : "memory");
        __asm__ volatile("movb %b1, (%0)" : : "r"(throughput_buffer + 56), "r"((uint64_t)store_value) : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* Store-load round-trip latency (store forwarding) -- MOV m64,r64 -> MOV r64,m64 */
static double store_load_roundtrip_lat(uint64_t iteration_count)
{
    volatile uint64_t scratch __attribute__((aligned(64))) = 0;
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("mov %1, %0\n\tmov %0, %1"
                         : "+m"(scratch), "+r"(accumulator));
        __asm__ volatile("mov %1, %0\n\tmov %0, %1"
                         : "+m"(scratch), "+r"(accumulator));
        __asm__ volatile("mov %1, %0\n\tmov %0, %1"
                         : "+m"(scratch), "+r"(accumulator));
        __asm__ volatile("mov %1, %0\n\tmov %0, %1"
                         : "+m"(scratch), "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * LEA WITH MEMORY-LIKE ADDRESSING
 * ======================================================================== */

/* LEA r64, [rip+disp] -- RIP-relative */
static double lea_rip_lat(uint64_t iteration_count)
{
    uint64_t accumulator;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("lea latency_buffer(%%rip), %0" : "=r"(accumulator));
        __asm__ volatile("" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* LEA r64, [base+index*8+disp] -- full SIB addressing */
static double lea_sib_full_lat(uint64_t iteration_count)
{
    uint64_t accumulator = 0;
    uint64_t index_value = 2;
    __asm__ volatile("" : "+r"(index_value));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lea 0x100(%0,%1,8), %0" : "+r"(accumulator) : "r"(index_value));
        __asm__ volatile("lea 0x100(%0,%1,8), %0" : "+r"(accumulator) : "r"(index_value));
        __asm__ volatile("lea 0x100(%0,%1,8), %0" : "+r"(accumulator) : "r"(index_value));
        __asm__ volatile("lea 0x100(%0,%1,8), %0" : "+r"(accumulator) : "r"(index_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * ALU r,m and m,r and m,imm FORMS
 * ======================================================================== */

/* ADD r64,[m64] -- load + add latency */
static double add_r64_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t accumulator = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("add (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("add (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("add (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("add (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

LOAD_THROUGHPUT(add_r64_m64_tp, "add (%1), %0")

/* ADD [m64],r64 -- memory RMW */
static double add_m64_r64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t add_source = 1;
    __asm__ volatile("" : "+r"(add_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
        __asm__ volatile("addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
        __asm__ volatile("addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
        __asm__ volatile("addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ADD [m64],imm -- memory RMW with immediate */
static double add_m64_imm_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("addq $1, %0" : "+m"(memory_operand));
        __asm__ volatile("addq $1, %0" : "+m"(memory_operand));
        __asm__ volatile("addq $1, %0" : "+m"(memory_operand));
        __asm__ volatile("addq $1, %0" : "+m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* SUB r64,[m64] */
static double sub_r64_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t accumulator = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("sub (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("sub (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("sub (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("sub (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* SUB [m64],r64 -- memory RMW */
static double sub_m64_r64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t sub_source = 1;
    __asm__ volatile("" : "+r"(sub_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
        __asm__ volatile("subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
        __asm__ volatile("subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
        __asm__ volatile("subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* AND r64,[m64] */
static double and_r64_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("and (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("and (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("and (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("and (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* AND [m64],r64 */
static double and_m64_r64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t and_source = 0xFFFFFFFFFFFFFFFFULL;
    __asm__ volatile("" : "+r"(and_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
        __asm__ volatile("andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
        __asm__ volatile("andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
        __asm__ volatile("andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* OR r64,[m64] */
static double or_r64_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("or (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("or (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("or (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("or (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* OR [m64],r64 */
static double or_m64_r64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t or_source = 0;
    __asm__ volatile("" : "+r"(or_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
        __asm__ volatile("orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
        __asm__ volatile("orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
        __asm__ volatile("orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* XOR r64,[m64] */
static double xor_r64_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("xor (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("xor (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("xor (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
        __asm__ volatile("xor (%1), %0" : "+r"(accumulator) : "r"(&memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* XOR [m64],r64 */
static double xor_m64_r64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t xor_source = 0;
    __asm__ volatile("" : "+r"(xor_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
        __asm__ volatile("xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
        __asm__ volatile("xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
        __asm__ volatile("xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* CMP r64,[m64] */
static double cmp_r64_m64_tp(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t compare_operand = 42;
    __asm__ volatile("" : "+r"(compare_operand));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
        __asm__ volatile("cmp (%1), %0" : : "r"(compare_operand), "r"(&memory_operand) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CMP [m64],r64 */
static double cmp_m64_r64_tp(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t compare_operand = 42;
    __asm__ volatile("" : "+r"(compare_operand));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
        __asm__ volatile("cmpq %1, %0" : : "m"(memory_operand), "r"(compare_operand) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * INC/DEC/NEG/NOT [m64]
 * ======================================================================== */

/* INC [m64] */
static double inc_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("incq %0" : "+m"(memory_operand));
        __asm__ volatile("incq %0" : "+m"(memory_operand));
        __asm__ volatile("incq %0" : "+m"(memory_operand));
        __asm__ volatile("incq %0" : "+m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* DEC [m64] */
static double dec_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("decq %0" : "+m"(memory_operand));
        __asm__ volatile("decq %0" : "+m"(memory_operand));
        __asm__ volatile("decq %0" : "+m"(memory_operand));
        __asm__ volatile("decq %0" : "+m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* NEG [m64] */
static double neg_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("negq %0" : "+m"(memory_operand));
        __asm__ volatile("negq %0" : "+m"(memory_operand));
        __asm__ volatile("negq %0" : "+m"(memory_operand));
        __asm__ volatile("negq %0" : "+m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* NOT [m64] */
static double not_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("notq %0" : "+m"(memory_operand));
        __asm__ volatile("notq %0" : "+m"(memory_operand));
        __asm__ volatile("notq %0" : "+m"(memory_operand));
        __asm__ volatile("notq %0" : "+m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * XCHG, CMPXCHG, XADD (implicit LOCK on memory)
 * ======================================================================== */

/* XCHG r64,[m64] -- implicit LOCK prefix */
static double xchg_r64_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 42;
    uint64_t register_value = 99;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("xchg %0, %1" : "+r"(register_value), "+m"(memory_operand));
        __asm__ volatile("xchg %0, %1" : "+r"(register_value), "+m"(memory_operand));
        __asm__ volatile("xchg %0, %1" : "+r"(register_value), "+m"(memory_operand));
        __asm__ volatile("xchg %0, %1" : "+r"(register_value), "+m"(memory_operand));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = register_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* CMPXCHG [m64],r64 -- compare and exchange */
static double cmpxchg_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 42;
    uint64_t rax_value = 42;  /* Match so exchange happens */
    uint64_t new_value = 42;
    __asm__ volatile("" : "+r"(new_value));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock cmpxchgq %2, %0"
                         : "+m"(memory_operand), "+a"(rax_value)
                         : "r"(new_value) : "cc");
        __asm__ volatile("lock cmpxchgq %2, %0"
                         : "+m"(memory_operand), "+a"(rax_value)
                         : "r"(new_value) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = rax_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* XADD [m64],r64 -- exchange and add */
static double xadd_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t add_value = 1;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = add_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * LOCK PREFIX ATOMICS
 * ======================================================================== */

/* LOCK ADD [m64],r64 -- atomic add (uncontended) */
static double lock_add_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t add_source = 1;
    __asm__ volatile("" : "+r"(add_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
        __asm__ volatile("lock addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
        __asm__ volatile("lock addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
        __asm__ volatile("lock addq %1, %0" : "+m"(memory_operand) : "r"(add_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* LOCK SUB [m64],r64 */
static double lock_sub_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t sub_source = 1;
    __asm__ volatile("" : "+r"(sub_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
        __asm__ volatile("lock subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
        __asm__ volatile("lock subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
        __asm__ volatile("lock subq %1, %0" : "+m"(memory_operand) : "r"(sub_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* LOCK AND [m64],r64 */
static double lock_and_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t and_source = 0xFFFFFFFFFFFFFFFFULL;
    __asm__ volatile("" : "+r"(and_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
        __asm__ volatile("lock andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
        __asm__ volatile("lock andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
        __asm__ volatile("lock andq %1, %0" : "+m"(memory_operand) : "r"(and_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* LOCK OR [m64],r64 */
static double lock_or_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t or_source = 0;
    __asm__ volatile("" : "+r"(or_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
        __asm__ volatile("lock orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
        __asm__ volatile("lock orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
        __asm__ volatile("lock orq %1, %0" : "+m"(memory_operand) : "r"(or_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* LOCK XOR [m64],r64 */
static double lock_xor_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t xor_source = 0;
    __asm__ volatile("" : "+r"(xor_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
        __asm__ volatile("lock xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
        __asm__ volatile("lock xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
        __asm__ volatile("lock xorq %1, %0" : "+m"(memory_operand) : "r"(xor_source));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = memory_operand; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* LOCK CMPXCHG [m64],r64 */
static double lock_cmpxchg_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 42;
    uint64_t rax_value = 42;
    uint64_t new_value = 42;
    __asm__ volatile("" : "+r"(new_value));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock cmpxchgq %2, %0"
                         : "+m"(memory_operand), "+a"(rax_value)
                         : "r"(new_value) : "cc");
        __asm__ volatile("lock cmpxchgq %2, %0"
                         : "+m"(memory_operand), "+a"(rax_value)
                         : "r"(new_value) : "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = rax_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 2);
}

/* LOCK XADD [m64],r64 */
static double lock_xadd_m64_lat(uint64_t iteration_count)
{
    volatile uint64_t memory_operand __attribute__((aligned(64))) = 0;
    uint64_t add_value = 1;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
        __asm__ volatile("lock xaddq %1, %0" : "+m"(memory_operand), "+r"(add_value));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = add_value; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * NON-TEMPORAL / CACHE CONTROL
 * ======================================================================== */

/* MOVNTI [m64],r64 -- non-temporal store */
static double movnti_m64_tp(uint64_t iteration_count)
{
    uint64_t store_value = 0xDEADBEEFCAFEBABEULL;
    __asm__ volatile("" : "+r"(store_value));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 0),  "r"(store_value) : "memory");
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 8),  "r"(store_value) : "memory");
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 16), "r"(store_value) : "memory");
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 24), "r"(store_value) : "memory");
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 32), "r"(store_value) : "memory");
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 40), "r"(store_value) : "memory");
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 48), "r"(store_value) : "memory");
        __asm__ volatile("movnti %1, (%0)" : : "r"(throughput_buffer + 56), "r"(store_value) : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    __asm__ volatile("sfence" ::: "memory");
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHT0 */
static double prefetcht0_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 0));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 8));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 16));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 24));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 32));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 40));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 48));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(throughput_buffer + 56));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHT1 */
static double prefetcht1_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 0));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 8));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 16));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 24));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 32));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 40));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 48));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(throughput_buffer + 56));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHT2 */
static double prefetcht2_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 0));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 8));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 16));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 24));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 32));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 40));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 48));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(throughput_buffer + 56));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHNTA */
static double prefetchnta_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 0));
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 8));
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 16));
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 24));
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 32));
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 40));
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 48));
        __asm__ volatile("prefetchnta (%0)" : : "r"(throughput_buffer + 56));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* CLFLUSH */
static double clflush_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("clflush (%0)" : : "r"(throughput_buffer) : "memory");
        __asm__ volatile("mfence" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* CLFLUSHOPT */
static double clflushopt_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("clflushopt (%0)" : : "r"(throughput_buffer) : "memory");
        __asm__ volatile("sfence" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* CLWB */
static double clwb_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 100;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        __asm__ volatile("clwb (%0)" : : "r"(throughput_buffer) : "memory");
        __asm__ volatile("sfence" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* ========================================================================
 * STRING OPERATIONS (REP prefix)
 * ======================================================================== */

/* REP MOVSB -- 1 byte */
static double rep_movsb_1_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile(
            "rep movsb"
            : "+S"(string_source_buffer), "+D"(string_destination_buffer), "+c"((uint64_t){1})
            : : "memory");
        /* Reset pointers */
        string_source_buffer -= 1;
        string_destination_buffer -= 1;
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* REP MOVSB -- 8 bytes */
static double rep_movsb_8_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        const uint8_t *src_ptr = (const uint8_t *)string_source_buffer;
        uint8_t *dst_ptr = (uint8_t *)string_destination_buffer;
        uint64_t count_value = 8;
        __asm__ volatile(
            "rep movsb"
            : "+S"(src_ptr), "+D"(dst_ptr), "+c"(count_value)
            : : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* REP MOVSB -- 64 bytes */
static double rep_movsb_64_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        const uint8_t *src_ptr = (const uint8_t *)string_source_buffer;
        uint8_t *dst_ptr = (uint8_t *)string_destination_buffer;
        uint64_t count_value = 64;
        __asm__ volatile(
            "rep movsb"
            : "+S"(src_ptr), "+D"(dst_ptr), "+c"(count_value)
            : : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* REP STOSB -- 1 byte */
static double rep_stosb_1_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        uint8_t *dst_ptr = (uint8_t *)string_destination_buffer;
        uint64_t count_value = 1;
        __asm__ volatile(
            "rep stosb"
            : "+D"(dst_ptr), "+c"(count_value)
            : "a"((uint8_t)0x42)
            : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* REP STOSB -- 8 bytes */
static double rep_stosb_8_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        uint8_t *dst_ptr = (uint8_t *)string_destination_buffer;
        uint64_t count_value = 8;
        __asm__ volatile(
            "rep stosb"
            : "+D"(dst_ptr), "+c"(count_value)
            : "a"((uint8_t)0x42)
            : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* REP STOSB -- 64 bytes */
static double rep_stosb_64_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        uint8_t *dst_ptr = (uint8_t *)string_destination_buffer;
        uint64_t count_value = 64;
        __asm__ volatile(
            "rep stosb"
            : "+D"(dst_ptr), "+c"(count_value)
            : "a"((uint8_t)0x42)
            : "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
}

/* REP CMPSB -- 1 byte */
static double rep_cmpsb_1_tp(uint64_t iteration_count)
{
    /* Make buffers equal for the comparison */
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        const uint8_t *src_ptr = (const uint8_t *)string_source_buffer;
        const uint8_t *dst_ptr = (const uint8_t *)string_destination_buffer;
        uint64_t count_value = 1;
        __asm__ volatile(
            "repe cmpsb"
            : "+S"(src_ptr), "+D"(dst_ptr), "+c"(count_value)
            : : "cc", "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* REP CMPSB -- 8 bytes */
static double rep_cmpsb_8_tp(uint64_t iteration_count)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        const uint8_t *src_ptr = (const uint8_t *)string_source_buffer;
        const uint8_t *dst_ptr = (const uint8_t *)string_destination_buffer;
        uint64_t count_value = 8;
        __asm__ volatile(
            "repe cmpsb"
            : "+S"(src_ptr), "+D"(dst_ptr), "+c"(count_value)
            : : "cc", "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 1);
}

/* REP CMPSB -- 64 bytes */
static double rep_cmpsb_64_tp(uint64_t iteration_count)
{
    uint64_t reduced_iterations = iteration_count / 10;
    if (reduced_iterations < 100) reduced_iterations = 100;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < reduced_iterations; loop_idx++) {
        const uint8_t *src_ptr = (const uint8_t *)string_source_buffer;
        const uint8_t *dst_ptr = (const uint8_t *)string_destination_buffer;
        uint64_t count_value = 64;
        __asm__ volatile(
            "repe cmpsb"
            : "+S"(src_ptr), "+D"(dst_ptr), "+c"(count_value)
            : : "cc", "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)(tsc_stop - tsc_start) / (double)(reduced_iterations * 1);
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
} MemoryProbeEntry;

static const MemoryProbeEntry probe_table[] = {
    /* --- Loads --- */
    {"MOV",       "r64,[m64]",     "load",        mov_r64_m64_lat,     mov_r64_m64_tp},
    {"MOV",       "r32,[m32]",     "load",        mov_r32_m32_lat,     mov_r32_m32_tp},
    {"MOV",       "r16,[m16]",     "load",        mov_r16_m16_lat,     mov_r16_m16_tp},
    {"MOV",       "r8,[m8]",       "load",        mov_r8_m8_lat,       mov_r8_m8_tp},
    {"MOVZX",     "r64,[m8]",      "load",        movzx_r64_m8_lat,    movzx_r64_m8_tp},
    {"MOVZX",     "r64,[m16]",     "load",        movzx_r64_m16_lat,   movzx_r64_m16_tp},
    {"MOVSX",     "r64,[m8]",      "load",        movsx_r64_m8_lat,    movsx_r64_m8_tp},
    {"MOVSX",     "r64,[m16]",     "load",        movsx_r64_m16_lat,   movsx_r64_m16_tp},

    /* --- Stores --- */
    {"MOV",       "[m64],r64",     "store",       NULL,                mov_m64_r64_tp},
    {"MOV",       "[m32],r32",     "store",       NULL,                mov_m32_r32_tp},
    {"MOV",       "[m16],r16",     "store",       NULL,                mov_m16_r16_tp},
    {"MOV",       "[m8],r8",       "store",       NULL,                mov_m8_r8_tp},
    {"MOV",       "[m64],imm",     "store",       NULL,                mov_m64_imm_tp},
    {"STR-LD",    "roundtrip",     "store",       store_load_roundtrip_lat, NULL},

    /* --- LEA memory forms --- */
    {"LEA",       "[rip+disp]",    "lea",         NULL,                lea_rip_lat},
    {"LEA",       "[b+i*8+d]",     "lea",         lea_sib_full_lat,    NULL},

    /* --- ALU r,m --- */
    {"ADD",       "r64,[m64]",     "alu_load",    add_r64_m64_lat,     NULL},
    {"SUB",       "r64,[m64]",     "alu_load",    sub_r64_m64_lat,     NULL},
    {"AND",       "r64,[m64]",     "alu_load",    and_r64_m64_lat,     NULL},
    {"OR",        "r64,[m64]",     "alu_load",    or_r64_m64_lat,      NULL},
    {"XOR",       "r64,[m64]",     "alu_load",    xor_r64_m64_lat,     NULL},

    /* --- ALU m,r and m,imm --- */
    {"ADD",       "[m64],r64",     "alu_rmw",     add_m64_r64_lat,     NULL},
    {"ADD",       "[m64],imm",     "alu_rmw",     add_m64_imm_lat,     NULL},
    {"SUB",       "[m64],r64",     "alu_rmw",     sub_m64_r64_lat,     NULL},
    {"AND",       "[m64],r64",     "alu_rmw",     and_m64_r64_lat,     NULL},
    {"OR",        "[m64],r64",     "alu_rmw",     or_m64_r64_lat,      NULL},
    {"XOR",       "[m64],r64",     "alu_rmw",     xor_m64_r64_lat,     NULL},

    /* --- CMP m --- */
    {"CMP",       "r64,[m64]",     "cmp_mem",     NULL,                cmp_r64_m64_tp},
    {"CMP",       "[m64],r64",     "cmp_mem",     NULL,                cmp_m64_r64_tp},

    /* --- INC/DEC/NEG/NOT m --- */
    {"INC",       "[m64]",         "rmw_unary",   inc_m64_lat,         NULL},
    {"DEC",       "[m64]",         "rmw_unary",   dec_m64_lat,         NULL},
    {"NEG",       "[m64]",         "rmw_unary",   neg_m64_lat,         NULL},
    {"NOT",       "[m64]",         "rmw_unary",   not_m64_lat,         NULL},

    /* --- XCHG / CMPXCHG / XADD --- */
    {"XCHG",      "r64,[m64]",     "atomic",      xchg_r64_m64_lat,   NULL},
    {"CMPXCHG",   "[m64],r64",     "atomic",      cmpxchg_m64_lat,    NULL},
    {"XADD",      "[m64],r64",     "atomic",      xadd_m64_lat,       NULL},

    /* --- LOCK prefix atomics --- */
    {"LOCK ADD",  "[m64],r64",     "lock",        lock_add_m64_lat,    NULL},
    {"LOCK SUB",  "[m64],r64",     "lock",        lock_sub_m64_lat,    NULL},
    {"LOCK AND",  "[m64],r64",     "lock",        lock_and_m64_lat,    NULL},
    {"LOCK OR",   "[m64],r64",     "lock",        lock_or_m64_lat,     NULL},
    {"LOCK XOR",  "[m64],r64",     "lock",        lock_xor_m64_lat,    NULL},
    {"LOCK CMPXCHG","[m64],r64",   "lock",        lock_cmpxchg_m64_lat, NULL},
    {"LOCK XADD", "[m64],r64",     "lock",        lock_xadd_m64_lat,  NULL},

    /* --- Non-temporal / Cache control --- */
    {"MOVNTI",    "[m64],r64",     "nt_store",    NULL,                movnti_m64_tp},
    {"PREFETCHT0","[m]",           "prefetch",    NULL,                prefetcht0_tp},
    {"PREFETCHT1","[m]",           "prefetch",    NULL,                prefetcht1_tp},
    {"PREFETCHT2","[m]",           "prefetch",    NULL,                prefetcht2_tp},
    {"PREFETCHNTA","[m]",          "prefetch",    NULL,                prefetchnta_tp},
    {"CLFLUSH",   "[m]",           "cache_ctrl",  NULL,                clflush_tp},
    {"CLFLUSHOPT","[m]",           "cache_ctrl",  NULL,                clflushopt_tp},
    {"CLWB",      "[m]",           "cache_ctrl",  NULL,                clwb_tp},

    /* --- String operations --- */
    {"REP MOVSB", "1 byte",        "string",      NULL,                rep_movsb_1_tp},
    {"REP MOVSB", "8 bytes",       "string",      NULL,                rep_movsb_8_tp},
    {"REP MOVSB", "64 bytes",      "string",      NULL,                rep_movsb_64_tp},
    {"REP STOSB", "1 byte",        "string",      NULL,                rep_stosb_1_tp},
    {"REP STOSB", "8 bytes",       "string",      NULL,                rep_stosb_8_tp},
    {"REP STOSB", "64 bytes",      "string",      NULL,                rep_stosb_64_tp},
    {"REP CMPSB", "1 byte",        "string",      NULL,                rep_cmpsb_1_tp},
    {"REP CMPSB", "8 bytes",       "string",      NULL,                rep_cmpsb_8_tp},
    {"REP CMPSB", "64 bytes",      "string",      NULL,                rep_cmpsb_64_tp},

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

    /* Allocate buffers */
    latency_buffer = (volatile uint64_t *)sm_zen_aligned_alloc64(L1_BUFFER_SIZE);
    throughput_buffer = (volatile uint64_t *)sm_zen_aligned_alloc64(L1_BUFFER_SIZE);
    string_source_buffer = (volatile uint8_t *)sm_zen_aligned_alloc64(4096);
    string_destination_buffer = (volatile uint8_t *)sm_zen_aligned_alloc64(4096);

    initialize_latency_buffer();
    /* Fill throughput buffer with known values */
    for (uint64_t fill_idx = 0; fill_idx < L1_BUFFER_U64; fill_idx++) {
        throughput_buffer[fill_idx] = fill_idx;
    }
    /* Fill string buffers with same data for CMPSB */
    for (int fill_idx = 0; fill_idx < 4096; fill_idx++) {
        string_source_buffer[fill_idx] = (uint8_t)(fill_idx & 0xFF);
        string_destination_buffer[fill_idx] = (uint8_t)(fill_idx & 0xFF);
    }

    if (!csv_mode) {
        sm_zen_print_header("scalar_memory_exhaustive", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n", "scalar memory");
        printf("Buffer sizes: latency=%d bytes (L1), throughput=%d bytes (L1)\n\n",
               L1_BUFFER_SIZE, L1_BUFFER_SIZE);
    }

    /* Print CSV header */
    printf("mnemonic,operands,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const MemoryProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const MemoryProbeEntry *current_entry = &probe_table[entry_idx];

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

    free((void *)latency_buffer);
    free((void *)throughput_buffer);
    free((void *)string_source_buffer);
    free((void *)string_destination_buffer);

    return 0;
}
