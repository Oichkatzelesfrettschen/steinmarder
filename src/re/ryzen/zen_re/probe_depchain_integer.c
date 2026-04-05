#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>
#include <math.h>

/*
 * probe_depchain_integer.c - Exhaustive integer data-type dependency chain
 * latency probes for Zen 4 (Ryzen 9 7945HX).
 *
 * Measures ADD and MUL latency with 4-deep true dependency chains per
 * iteration. The output of each operation feeds directly as input to
 * the next. We use __asm__ volatile("" : "+x"(acc)) after each intrinsic
 * to prevent the compiler from constant-folding the chain while preserving
 * the true data dependency through the register. This is NOT a memory
 * barrier or execution barrier -- it only tells the compiler "this value
 * is opaque, do not optimize through it."
 *
 * Memory barriers (__asm__ volatile("" ::: "memory")) are placed between
 * measurement phases only.
 *
 * Types covered:
 *   UINT8/INT8 add (vpaddb zmm)     - 64 lanes
 *   UINT16/INT16 add (vpaddw zmm)   - 32 lanes
 *   UINT32/INT32 add (vpaddd zmm)   - 16 lanes
 *   UINT64/INT64 add (vpaddq zmm)   - 8 lanes
 *   INT16 mul (vpmullw zmm)         - 32 lanes, low 16 of 16x16
 *   UINT32/INT32 mul (vpmulld zmm)  - 16 lanes, low 32 of 32x32
 *   UINT64 mul (emulated)           - no native 64x64 SIMD mul
 *   Scalar INT32 add (add eax,eax)  - GPR baseline
 *   Scalar INT64 add (add rax,rax)  - GPR baseline
 *
 * Signed vs unsigned use identical instructions for add/mullo.
 */

#define WARMUP_ITERS 5000
#define CHAIN_DEPTH  4

/* Macro for SIMD integer add probes with compiler opacity fence */
#define DEFINE_SIMD_ADD_PROBE(name, set_func, add_func, reduce_func, reduce_type) \
static double name(uint64_t iterations)                                     \
{                                                                           \
    __m512i accumulator = set_func(1);                                      \
    __m512i addend = set_func(1);                                           \
    /* Warmup */                                                            \
    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) { \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
    }                                                                       \
    __asm__ volatile("" ::: "memory");                                      \
    uint64_t tsc_start = sm_zen_tsc_begin();                                \
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) { \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
        accumulator = add_func(accumulator, addend);                        \
        __asm__ volatile("" : "+x"(accumulator));                           \
    }                                                                       \
    uint64_t tsc_stop = sm_zen_tsc_end();                                   \
    volatile reduce_type sink_value = reduce_func(accumulator);             \
    (void)sink_value;                                                       \
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH); \
}

DEFINE_SIMD_ADD_PROBE(depchain_uint8_add,
    _mm512_set1_epi8, _mm512_add_epi8, _mm512_reduce_add_epi32, int)
DEFINE_SIMD_ADD_PROBE(depchain_int8_add,
    _mm512_set1_epi8, _mm512_add_epi8, _mm512_reduce_add_epi32, int)
DEFINE_SIMD_ADD_PROBE(depchain_uint16_add,
    _mm512_set1_epi16, _mm512_add_epi16, _mm512_reduce_add_epi32, int)
DEFINE_SIMD_ADD_PROBE(depchain_int16_add,
    _mm512_set1_epi16, _mm512_add_epi16, _mm512_reduce_add_epi32, int)
DEFINE_SIMD_ADD_PROBE(depchain_uint32_add,
    _mm512_set1_epi32, _mm512_add_epi32, _mm512_reduce_add_epi32, int)
DEFINE_SIMD_ADD_PROBE(depchain_int32_add,
    _mm512_set1_epi32, _mm512_add_epi32, _mm512_reduce_add_epi32, int)
DEFINE_SIMD_ADD_PROBE(depchain_uint64_add,
    _mm512_set1_epi64, _mm512_add_epi64, _mm512_reduce_add_epi64, long long)
DEFINE_SIMD_ADD_PROBE(depchain_int64_add,
    _mm512_set1_epi64, _mm512_add_epi64, _mm512_reduce_add_epi64, long long)

/* INT16 multiply: vpmullw zmm */
static double depchain_int16_mul(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi16(3);
    __m512i multiplier = _mm512_set1_epi16(1);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mullo_epi16(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm512_reduce_add_epi32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* Macro for SIMD integer mul probes */
#define DEFINE_SIMD_MUL32_PROBE(name) \
static double name(uint64_t iterations) \
{ \
    __m512i accumulator = _mm512_set1_epi32(3); \
    __m512i multiplier = _mm512_set1_epi32(1); \
    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) { \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
    } \
    __asm__ volatile("" ::: "memory"); \
    uint64_t tsc_start = sm_zen_tsc_begin(); \
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) { \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = _mm512_mullo_epi32(accumulator, multiplier); \
        __asm__ volatile("" : "+x"(accumulator)); \
    } \
    uint64_t tsc_stop = sm_zen_tsc_end(); \
    volatile int sink_value = _mm512_reduce_add_epi32(accumulator); \
    (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH); \
}

DEFINE_SIMD_MUL32_PROBE(depchain_uint32_mul)
DEFINE_SIMD_MUL32_PROBE(depchain_int32_mul)

/*
 * UINT64 mul (emulated): No native 64x64->64 SIMD instruction.
 * Zen 4 only has vpmuludq (32x32->64 unsigned).
 * Emulate full 64x64->low64:
 *   lo = vpmuludq(a, b)
 *   hi_a = vpsrlq(a, 32)
 *   cross1 = vpmuludq(hi_a, b)
 *   cross2 = vpmuludq(a, hi_b)  -- hi_b is constant, precomputed
 *   result = vpaddq(lo, vpsllq(vpaddq(cross1, cross2), 32))
 */
static double depchain_uint64_mul_emulated(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi64(0x0000000300000007LL);
    __m512i multiplier = _mm512_set1_epi64(1);
    /* Precompute high32 of multiplier (0 since multiplier=1, but keep the dep chain honest) */
    __m512i multiplier_high = _mm512_srli_epi64(multiplier, 32);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int chain_link = 0; chain_link < CHAIN_DEPTH; chain_link++) {
            __m512i low_product = _mm512_mul_epu32(accumulator, multiplier);
            __m512i accumulator_high = _mm512_srli_epi64(accumulator, 32);
            __m512i cross_product_1 = _mm512_mul_epu32(accumulator_high, multiplier);
            __m512i cross_product_2 = _mm512_mul_epu32(accumulator, multiplier_high);
            __m512i cross_sum = _mm512_add_epi64(cross_product_1, cross_product_2);
            __m512i cross_shifted = _mm512_slli_epi64(cross_sum, 32);
            accumulator = _mm512_add_epi64(low_product, cross_shifted);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        for (int chain_link = 0; chain_link < CHAIN_DEPTH; chain_link++) {
            __m512i low_product = _mm512_mul_epu32(accumulator, multiplier);
            __m512i accumulator_high = _mm512_srli_epi64(accumulator, 32);
            __m512i cross_product_1 = _mm512_mul_epu32(accumulator_high, multiplier);
            __m512i cross_product_2 = _mm512_mul_epu32(accumulator, multiplier_high);
            __m512i cross_sum = _mm512_add_epi64(cross_product_1, cross_product_2);
            __m512i cross_shifted = _mm512_slli_epi64(cross_sum, 32);
            accumulator = _mm512_add_epi64(low_product, cross_shifted);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink_value = _mm512_reduce_add_epi64(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* Scalar INT32 add: add eax, eax */
static double depchain_scalar_int32_add(uint64_t iterations)
{
    uint32_t accumulator = 1;

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile uint32_t sink_value = accumulator;
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* Scalar INT64 add: add rax, rax */
static double depchain_scalar_int64_add(uint64_t iterations)
{
    uint64_t accumulator = 1;

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
        __asm__ volatile("add %0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile uint64_t sink_value = accumulator;
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

typedef struct {
    const char *type_name;
    int width_bits;
    int simd_width;
    const char *operation;
    double latency_cycles;
    const char *instruction;
    const char *native_status;
} IntegerProbeResult;

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 100000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    sm_zen_print_header("depchain_integer", &probe_config, &detected_features);
    printf("\n");

    if (!detected_features.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    uint64_t total_iterations = probe_config.iterations;

    IntegerProbeResult results_table[] = {
        {"UINT8",       8,  512, "add", 0.0, "vpaddb zmm", "native"},
        {"INT8",        8,  512, "add", 0.0, "vpaddb zmm", "native"},
        {"UINT16",      16, 512, "add", 0.0, "vpaddw zmm", "native"},
        {"INT16",       16, 512, "add", 0.0, "vpaddw zmm", "native"},
        {"UINT32",      32, 512, "add", 0.0, "vpaddd zmm", "native"},
        {"INT32",       32, 512, "add", 0.0, "vpaddd zmm", "native"},
        {"UINT64",      64, 512, "add", 0.0, "vpaddq zmm", "native"},
        {"INT64",       64, 512, "add", 0.0, "vpaddq zmm", "native"},
        {"INT16",       16, 512, "mul", 0.0, "vpmullw zmm", "native"},
        {"UINT32",      32, 512, "mul", 0.0, "vpmulld zmm", "native"},
        {"INT32",       32, 512, "mul", 0.0, "vpmulld zmm", "native"},
        {"UINT64",      64, 512, "mul", 0.0, "vpmuludq+shifts", "emulated"},
        {"INT64",       64, 512, "mul", 0.0, "vpmuludq+shifts", "emulated"},
        {"Scalar INT32", 32, 1,  "add", 0.0, "add eax,eax", "native"},
        {"Scalar INT64", 64, 1,  "add", 0.0, "add rax,rax", "native"},
    };
    int result_count = sizeof(results_table) / sizeof(results_table[0]);

    results_table[0].latency_cycles  = depchain_uint8_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[1].latency_cycles  = depchain_int8_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[2].latency_cycles  = depchain_uint16_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[3].latency_cycles  = depchain_int16_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[4].latency_cycles  = depchain_uint32_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[5].latency_cycles  = depchain_int32_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[6].latency_cycles  = depchain_uint64_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[7].latency_cycles  = depchain_int64_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[8].latency_cycles  = depchain_int16_mul(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[9].latency_cycles  = depchain_uint32_mul(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[10].latency_cycles = depchain_int32_mul(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[11].latency_cycles = depchain_uint64_mul_emulated(total_iterations);
    __asm__ volatile("" ::: "memory");
    /* INT64 mul emulated is same codepath as UINT64 */
    results_table[12].latency_cycles = depchain_uint64_mul_emulated(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[13].latency_cycles = depchain_scalar_int32_add(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[14].latency_cycles = depchain_scalar_int64_add(total_iterations);

    printf("%-16s %6s %10s %5s %14s  %-20s %s\n",
           "type", "width", "simd_width", "op", "latency_cycles", "instruction", "native");
    printf("%-16s %6s %10s %5s %14s  %-20s %s\n",
           "----", "-----", "----------", "--", "--------------", "-----------", "------");
    for (int row_index = 0; row_index < result_count; row_index++) {
        printf("%-16s %6d %10d %5s %14.4f  %-20s %s\n",
               results_table[row_index].type_name,
               results_table[row_index].width_bits,
               results_table[row_index].simd_width,
               results_table[row_index].operation,
               results_table[row_index].latency_cycles,
               results_table[row_index].instruction,
               results_table[row_index].native_status);
    }

    if (probe_config.csv) {
        printf("\ncsv,probe=depchain_integer");
        for (int row_index = 0; row_index < result_count; row_index++) {
            printf(",%s_%s=%.4f",
                   results_table[row_index].type_name,
                   results_table[row_index].operation,
                   results_table[row_index].latency_cycles);
        }
        printf("\n");
    }

    return 0;
}
