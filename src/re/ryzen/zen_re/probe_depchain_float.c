#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>
#include <math.h>
#include <float.h>

/*
 * probe_depchain_float.c - Exhaustive floating-point data-type dependency chain
 * latency probes for Zen 4 (Ryzen 9 7945HX).
 *
 * 4-deep true dependency chains per iteration with __asm__ volatile("" : "+x"(acc))
 * compiler opacity fences to prevent constant-folding while preserving the
 * true data dependency through the register.
 *
 * Memory barriers between measurement phases only.
 *
 * Types: FP16 (emulated via F16C convert), BF16 (vdpbf16ps native),
 * FP32 (scalar/128/256/512), FP64 (scalar/256/512), FP80 (x87),
 * FP128 (__float128 software emulation), TF32 (emulated as truncated FP32).
 *
 * Zen 4 has NO native FP16 compute (AVX512-FP16 is Sapphire Rapids only).
 * Zen 4 has native BF16 via AVX512-BF16 (vdpbf16ps).
 */

#define WARMUP_ITERS 5000
#define CHAIN_DEPTH  4

typedef struct {
    const char *type_name;
    int width_bits;
    int simd_width;
    const char *operation;
    double latency_cycles;
    const char *instruction;
    const char *native_status;
} FloatProbeResult;

/* ===== FP16 add (emulated): convert FP16->FP32, addss, convert back ===== */
static double depchain_fp16_add_emulated(uint64_t iterations)
{
    __m128i accumulator_fp16 = _mm_set1_epi16(0x3C00); /* 1.0 in FP16 */
    __m128 addend_fp32 = _mm_set1_ps(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int chain_link = 0; chain_link < CHAIN_DEPTH; chain_link++) {
            __m128 as_fp32 = _mm_cvtph_ps(accumulator_fp16);
            as_fp32 = _mm_add_ss(as_fp32, addend_fp32);
            accumulator_fp16 = _mm_cvtps_ph(as_fp32, _MM_FROUND_TO_NEAREST_INT);
            __asm__ volatile("" : "+x"(accumulator_fp16));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        for (int chain_link = 0; chain_link < CHAIN_DEPTH; chain_link++) {
            __m128 as_fp32 = _mm_cvtph_ps(accumulator_fp16);
            as_fp32 = _mm_add_ss(as_fp32, addend_fp32);
            accumulator_fp16 = _mm_cvtps_ph(as_fp32, _MM_FROUND_TO_NEAREST_INT);
            __asm__ volatile("" : "+x"(accumulator_fp16));
        }
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm_extract_epi16(accumulator_fp16, 0);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ===== BF16: vdpbf16ps zmm ===== */
static double depchain_bf16_dpbf16ps(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1.0f);
    __m512bh bf16_ones = (__m512bh)_mm512_set1_epi16(0x3F80);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_dpbf16_ps(accumulator, bf16_ones, bf16_ones);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ===== FP32 add scalar ===== */
static double depchain_fp32_add_scalar(uint64_t iterations)
{
    __m128 accumulator = _mm_set_ss(1.0f);
    __m128 addend = _mm_set_ss(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_ss(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm_cvtss_f32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ===== Generic FP32 packed add macro ===== */
#define DEFINE_FP32_ADD_PROBE(name, vec_type, set_func, add_func, sink_expr) \
static double name(uint64_t iterations) \
{ \
    vec_type accumulator = set_func(1.0f); \
    vec_type addend = set_func(0.001f); \
    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) { \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
    } \
    __asm__ volatile("" ::: "memory"); \
    uint64_t tsc_start = sm_zen_tsc_begin(); \
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) { \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
        accumulator = add_func(accumulator, addend); \
        __asm__ volatile("" : "+x"(accumulator)); \
    } \
    uint64_t tsc_stop = sm_zen_tsc_end(); \
    volatile float sink_value = sink_expr; \
    (void)sink_value; \
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH); \
}

DEFINE_FP32_ADD_PROBE(depchain_fp32_add_128, __m128, _mm_set1_ps, _mm_add_ps, accumulator[0])
DEFINE_FP32_ADD_PROBE(depchain_fp32_add_256, __m256, _mm256_set1_ps, _mm256_add_ps, accumulator[0])
DEFINE_FP32_ADD_PROBE(depchain_fp32_add_512, __m512, _mm512_set1_ps, _mm512_add_ps, _mm512_reduce_add_ps(accumulator))

/* FP32 mul scalar */
static double depchain_fp32_mul_scalar(uint64_t iterations)
{
    __m128 accumulator = _mm_set_ss(1.0f);
    __m128 multiplier = _mm_set_ss(1.0000001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_ss(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm_cvtss_f32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 mul 512-bit */
static double depchain_fp32_mul_512(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1.0f);
    __m512 multiplier = _mm512_set1_ps(1.0000001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_mul_ps(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 fma scalar */
static double depchain_fp32_fma_scalar(uint64_t iterations)
{
    __m128 accumulator = _mm_set_ss(1.0f);
    __m128 multiplier = _mm_set_ss(1.0f);
    __m128 addend = _mm_set_ss(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_fmadd_ss(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm_cvtss_f32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 fma 256-bit */
static double depchain_fp32_fma_256(uint64_t iterations)
{
    __m256 accumulator = _mm256_set1_ps(1.0f);
    __m256 multiplier = _mm256_set1_ps(1.0f);
    __m256 addend = _mm256_set1_ps(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = accumulator[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 fma 512-bit */
static double depchain_fp32_fma_512(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1.0f);
    __m512 multiplier = _mm512_set1_ps(1.0f);
    __m512 addend = _mm512_set1_ps(0.001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_ps(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 div scalar */
static double depchain_fp32_div_scalar(uint64_t iterations)
{
    __m128 accumulator = _mm_set_ss(1.0e10f);
    __m128 divisor = _mm_set_ss(1.0000001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_ss(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm_cvtss_f32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 div 512-bit */
static double depchain_fp32_div_512(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1.0e10f);
    __m512 divisor = _mm512_set1_ps(1.0000001f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_div_ps(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 sqrt scalar */
static double depchain_fp32_sqrt_scalar(uint64_t iterations)
{
    __m128 accumulator = _mm_set_ss(1.0e10f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    accumulator = _mm_set_ss(1.0e10f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_ss(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm_cvtss_f32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP32 sqrt 512-bit */
static double depchain_fp32_sqrt_512(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1.0e10f);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    accumulator = _mm512_set1_ps(1.0e10f);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_sqrt_ps(accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* ===== FP64 operations ===== */

/* FP64 add scalar */
static double depchain_fp64_add_scalar(uint64_t iterations)
{
    __m128d accumulator = _mm_set_sd(1.0);
    __m128d addend = _mm_set_sd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_add_sd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm_cvtsd_f64(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP64 add 256-bit */
static double depchain_fp64_add_256(uint64_t iterations)
{
    __m256d accumulator = _mm256_set1_pd(1.0);
    __m256d addend = _mm256_set1_pd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm256_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = accumulator[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP64 add 512-bit */
static double depchain_fp64_add_512(uint64_t iterations)
{
    __m512d accumulator = _mm512_set1_pd(1.0);
    __m512d addend = _mm512_set1_pd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_add_pd(accumulator, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm512_reduce_add_pd(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP64 mul scalar */
static double depchain_fp64_mul_scalar(uint64_t iterations)
{
    __m128d accumulator = _mm_set_sd(1.0);
    __m128d multiplier = _mm_set_sd(1.0000000001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_mul_sd(accumulator, multiplier);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm_cvtsd_f64(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP64 fma 512-bit */
static double depchain_fp64_fma_512(uint64_t iterations)
{
    __m512d accumulator = _mm512_set1_pd(1.0);
    __m512d multiplier = _mm512_set1_pd(1.0);
    __m512d addend = _mm512_set1_pd(0.001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm512_fmadd_pd(accumulator, multiplier, addend);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm512_reduce_add_pd(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP64 div scalar */
static double depchain_fp64_div_scalar(uint64_t iterations)
{
    __m128d accumulator = _mm_set_sd(1.0e100);
    __m128d divisor = _mm_set_sd(1.0000000001);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_div_sd(accumulator, divisor);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm_cvtsd_f64(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP64 sqrt scalar */
static double depchain_fp64_sqrt_scalar(uint64_t iterations)
{
    __m128d accumulator = _mm_set_sd(1.0e100);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    accumulator = _mm_set_sd(1.0e100);
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
        accumulator = _mm_sqrt_sd(accumulator, accumulator);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = _mm_cvtsd_f64(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP80 x87: fadd st(0), st(0) */
static double depchain_fp80_add_x87(uint64_t iterations)
{
    long double accumulator = 1.0L;

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
        accumulator = 1.0L;
    }
    __asm__ volatile("" ::: "memory");

    accumulator = 1.0L;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
        __asm__ volatile("fadd %%st(0), %%st(0)" : "+t"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long double sink_value = accumulator;
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* FP128 (__float128) software emulation */
static double depchain_fp128_add_emulated(uint64_t iterations)
{
    __float128 accumulator = 1.0Q;
    __float128 addend = 0.001Q;

    for (uint64_t warmup_iter = 0; warmup_iter < 100; warmup_iter++) {
        accumulator += addend;
        accumulator += addend;
        accumulator += addend;
        accumulator += addend;
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        accumulator += addend;
        __asm__ volatile("" : "+r"(accumulator));
        accumulator += addend;
        __asm__ volatile("" : "+r"(accumulator));
        accumulator += addend;
        __asm__ volatile("" : "+r"(accumulator));
        accumulator += addend;
        __asm__ volatile("" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile double sink_value = (double)accumulator;
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

/* TF32 emulated (FP32 with manual mantissa truncation to 10 bits) */
static double depchain_tf32_add_emulated(uint64_t iterations)
{
    __m512 accumulator = _mm512_set1_ps(1.0f);
    __m512 addend = _mm512_set1_ps(0.001f);
    __m512i truncation_mask = _mm512_set1_epi32(0xFFFFE000u);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        for (int chain_link = 0; chain_link < CHAIN_DEPTH; chain_link++) {
            accumulator = _mm512_add_ps(accumulator, addend);
            accumulator = (__m512)_mm512_and_si512((__m512i)accumulator, truncation_mask);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        for (int chain_link = 0; chain_link < CHAIN_DEPTH; chain_link++) {
            accumulator = _mm512_add_ps(accumulator, addend);
            accumulator = (__m512)_mm512_and_si512((__m512i)accumulator, truncation_mask);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * CHAIN_DEPTH);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 100000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    sm_zen_print_header("depchain_float", &probe_config, &detected_features);
    printf("\n");

    if (!detected_features.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    uint64_t total_iterations = probe_config.iterations;
    uint64_t slow_iterations = total_iterations / 10;
    if (slow_iterations < 1000) slow_iterations = 1000;
    uint64_t fp128_iterations = total_iterations / 100;
    if (fp128_iterations < 100) fp128_iterations = 100;

    FloatProbeResult results_table[] = {
        {"FP16 (emul)",    16,   1,   "add",     0.0, "cvtph2ps+addss+cvtps2ph", "emulated"},
        {"BF16",           16, 512,   "dpbf16ps",0.0, "vdpbf16ps zmm",           "native"},
        {"FP32",           32,   1,   "add",     0.0, "vaddss xmm",              "native"},
        {"FP32",           32, 128,   "add",     0.0, "vaddps xmm",              "native"},
        {"FP32",           32, 256,   "add",     0.0, "vaddps ymm",              "native"},
        {"FP32",           32, 512,   "add",     0.0, "vaddps zmm",              "native"},
        {"FP32",           32,   1,   "mul",     0.0, "vmulss xmm",              "native"},
        {"FP32",           32, 512,   "mul",     0.0, "vmulps zmm",              "native"},
        {"FP32",           32,   1,   "fma",     0.0, "vfmadd231ss xmm",         "native"},
        {"FP32",           32, 256,   "fma",     0.0, "vfmadd231ps ymm",         "native"},
        {"FP32",           32, 512,   "fma",     0.0, "vfmadd231ps zmm",         "native"},
        {"FP32",           32,   1,   "div",     0.0, "vdivss xmm",              "native"},
        {"FP32",           32, 512,   "div",     0.0, "vdivps zmm",              "native"},
        {"FP32",           32,   1,   "sqrt",    0.0, "vsqrtss xmm",             "native"},
        {"FP32",           32, 512,   "sqrt",    0.0, "vsqrtps zmm",             "native"},
        {"FP64",           64,   1,   "add",     0.0, "vaddsd xmm",              "native"},
        {"FP64",           64, 256,   "add",     0.0, "vaddpd ymm",              "native"},
        {"FP64",           64, 512,   "add",     0.0, "vaddpd zmm",              "native"},
        {"FP64",           64,   1,   "mul",     0.0, "vmulsd xmm",              "native"},
        {"FP64",           64, 512,   "fma",     0.0, "vfmadd231pd zmm",         "native"},
        {"FP64",           64,   1,   "div",     0.0, "vdivsd xmm",              "native"},
        {"FP64",           64,   1,   "sqrt",    0.0, "vsqrtsd xmm",             "native"},
        {"FP80 (x87)",     80,   1,   "add",     0.0, "fadd st(0),st(0)",        "native"},
        {"FP128",         128,   1,   "add",     0.0, "__float128 +=",           "emulated"},
        {"TF32 (emul)",    19, 512,   "add",     0.0, "vaddps+vpand zmm",        "emulated"},
    };
    int result_count = sizeof(results_table) / sizeof(results_table[0]);

    printf("Running FP probes (fast=%lu, slow=%lu, fp128=%lu)...\n\n",
           (unsigned long)total_iterations,
           (unsigned long)slow_iterations,
           (unsigned long)fp128_iterations);

    results_table[0].latency_cycles  = depchain_fp16_add_emulated(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[1].latency_cycles  = depchain_bf16_dpbf16ps(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[2].latency_cycles  = depchain_fp32_add_scalar(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[3].latency_cycles  = depchain_fp32_add_128(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[4].latency_cycles  = depchain_fp32_add_256(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[5].latency_cycles  = depchain_fp32_add_512(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[6].latency_cycles  = depchain_fp32_mul_scalar(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[7].latency_cycles  = depchain_fp32_mul_512(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[8].latency_cycles  = depchain_fp32_fma_scalar(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[9].latency_cycles  = depchain_fp32_fma_256(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[10].latency_cycles = depchain_fp32_fma_512(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[11].latency_cycles = depchain_fp32_div_scalar(slow_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[12].latency_cycles = depchain_fp32_div_512(slow_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[13].latency_cycles = depchain_fp32_sqrt_scalar(slow_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[14].latency_cycles = depchain_fp32_sqrt_512(slow_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[15].latency_cycles = depchain_fp64_add_scalar(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[16].latency_cycles = depchain_fp64_add_256(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[17].latency_cycles = depchain_fp64_add_512(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[18].latency_cycles = depchain_fp64_mul_scalar(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[19].latency_cycles = depchain_fp64_fma_512(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[20].latency_cycles = depchain_fp64_div_scalar(slow_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[21].latency_cycles = depchain_fp64_sqrt_scalar(slow_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[22].latency_cycles = depchain_fp80_add_x87(total_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[23].latency_cycles = depchain_fp128_add_emulated(fp128_iterations);
    __asm__ volatile("" ::: "memory");
    results_table[24].latency_cycles = depchain_tf32_add_emulated(total_iterations);

    printf("%-16s %6s %10s %10s %14s  %-28s %s\n",
           "type", "width", "simd_width", "op", "latency_cycles", "instruction", "native");
    printf("%-16s %6s %10s %10s %14s  %-28s %s\n",
           "----", "-----", "----------", "--", "--------------", "-----------", "------");
    for (int row_index = 0; row_index < result_count; row_index++) {
        printf("%-16s %6d %10d %10s %14.4f  %-28s %s\n",
               results_table[row_index].type_name,
               results_table[row_index].width_bits,
               results_table[row_index].simd_width,
               results_table[row_index].operation,
               results_table[row_index].latency_cycles,
               results_table[row_index].instruction,
               results_table[row_index].native_status);
    }

    if (probe_config.csv) {
        printf("\ncsv,probe=depchain_float");
        for (int row_index = 0; row_index < result_count; row_index++) {
            printf(",%s_%dbit_%s=%.4f",
                   results_table[row_index].type_name,
                   results_table[row_index].width_bits,
                   results_table[row_index].operation,
                   results_table[row_index].latency_cycles);
        }
        printf("\n");
    }

    return 0;
}
