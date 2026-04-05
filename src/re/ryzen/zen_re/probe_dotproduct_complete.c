#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_dotproduct_complete.c -- Definitive dot-product characterization.
 *
 * Every dot-product and dot-product-adjacent instruction at every supported
 * width on Zen 4. Covers strict dot products (DPPD, DPPS, VPDPBUSD, VPDPWSSD,
 * VDPBF16PS) and building blocks (PMADDWD, PMADDUBSW, MPSADBW).
 *
 * Output: one CSV line per instruction:
 *   mnemonic,width_bits,latency_cycles,throughput_cycles,extension
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -mavx512f -mavx512bw -mavx512vl \
 *     -mavx512dq -mavx512vnni -mavx512bf16 -mfma -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_dotproduct_complete.c -lm \
 *     -o ../../../../build/bin/zen_probe_dotproduct_complete
 */

/* ========================================================================
 * STRICT DOT PRODUCTS -- SSE4.1 FP
 * ======================================================================== */

/* DPPD xmm -- SSE4.1 FP64 dot product, 128-bit */
static double dppd_xmm_lat(uint64_t iteration_count)
{
    __m128d accumulator = _mm_set_pd(1.0001, 2.0001);
    __m128d operand_source = _mm_set_pd(0.9999, 1.0001);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
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

static double dppd_xmm_tp(uint64_t iteration_count)
{
    __m128d stream_0 = _mm_set_pd(1.0001, 2.0001);
    __m128d stream_1 = _mm_set_pd(1.0002, 2.0002);
    __m128d stream_2 = _mm_set_pd(1.0003, 2.0003);
    __m128d stream_3 = _mm_set_pd(1.0004, 2.0004);
    __m128d operand_source = _mm_set_pd(0.9999, 1.0001);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_dp_pd(stream_0, operand_source, 0x31);
        stream_1 = _mm_dp_pd(stream_1, operand_source, 0x31);
        stream_2 = _mm_dp_pd(stream_2, operand_source, 0x31);
        stream_3 = _mm_dp_pd(stream_3, operand_source, 0x31);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128d sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* DPPS xmm -- SSE4.1 FP32 dot product, 128-bit */
static double dpps_xmm_lat(uint64_t iteration_count)
{
    __m128 accumulator = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 operand_source = _mm_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_dp_ps(accumulator, operand_source, 0xFF);
        accumulator = _mm_dp_ps(accumulator, operand_source, 0xFF);
        accumulator = _mm_dp_ps(accumulator, operand_source, 0xFF);
        accumulator = _mm_dp_ps(accumulator, operand_source, 0xFF);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double dpps_xmm_tp(uint64_t iteration_count)
{
    __m128 stream_0 = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 stream_1 = _mm_setr_ps(1.1f, 2.1f, 3.1f, 4.1f);
    __m128 stream_2 = _mm_setr_ps(1.2f, 2.2f, 3.2f, 4.2f);
    __m128 stream_3 = _mm_setr_ps(1.3f, 2.3f, 3.3f, 4.3f);
    __m128 operand_source = _mm_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_dp_ps(stream_0, operand_source, 0xFF);
        stream_1 = _mm_dp_ps(stream_1, operand_source, 0xFF);
        stream_2 = _mm_dp_ps(stream_2, operand_source, 0xFF);
        stream_3 = _mm_dp_ps(stream_3, operand_source, 0xFF);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128 sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VDPPS ymm -- AVX FP32 dot product, 256-bit */
static double vdpps_ymm_lat(uint64_t iteration_count)
{
    __m256 accumulator = _mm256_setr_ps(1.0f, 2.0f, 3.0f, 4.0f,
                                         5.0f, 6.0f, 7.0f, 8.0f);
    __m256 operand_source = _mm256_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_dp_ps(accumulator, operand_source, 0xFF);
        accumulator = _mm256_dp_ps(accumulator, operand_source, 0xFF);
        accumulator = _mm256_dp_ps(accumulator, operand_source, 0xFF);
        accumulator = _mm256_dp_ps(accumulator, operand_source, 0xFF);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vdpps_ymm_tp(uint64_t iteration_count)
{
    __m256 stream_0 = _mm256_set1_ps(1.0f);
    __m256 stream_1 = _mm256_set1_ps(2.0f);
    __m256 stream_2 = _mm256_set1_ps(3.0f);
    __m256 stream_3 = _mm256_set1_ps(4.0f);
    __m256 operand_source = _mm256_set1_ps(0.1f);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_dp_ps(stream_0, operand_source, 0xFF);
        stream_1 = _mm256_dp_ps(stream_1, operand_source, 0xFF);
        stream_2 = _mm256_dp_ps(stream_2, operand_source, 0xFF);
        stream_3 = _mm256_dp_ps(stream_3, operand_source, 0xFF);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256 sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * VNNI INTEGER DOT PRODUCTS -- VPDPBUSD (uint8 * int8 -> int32)
 * ======================================================================== */

/* VPDPBUSD xmm -- 128-bit */
static double vpdpbusd_xmm_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi8(1);
    __m128i operand_b = _mm_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpbusd_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpbusd_xmm_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_setzero_si128();
    __m128i stream_1 = _mm_setzero_si128();
    __m128i stream_2 = _mm_setzero_si128();
    __m128i stream_3 = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi8(1);
    __m128i operand_b = _mm_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_dpbusd_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm_dpbusd_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm_dpbusd_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm_dpbusd_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPBUSD ymm -- 256-bit */
static double vpdpbusd_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi8(1);
    __m256i operand_b = _mm256_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbusd_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpbusd_ymm_tp(uint64_t iteration_count)
{
    __m256i stream_0 = _mm256_setzero_si256();
    __m256i stream_1 = _mm256_setzero_si256();
    __m256i stream_2 = _mm256_setzero_si256();
    __m256i stream_3 = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi8(1);
    __m256i operand_b = _mm256_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_dpbusd_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm256_dpbusd_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm256_dpbusd_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm256_dpbusd_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPBUSD zmm -- 512-bit */
static double vpdpbusd_zmm_lat(uint64_t iteration_count)
{
    __m512i accumulator = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi8(1);
    __m512i operand_b = _mm512_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbusd_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpbusd_zmm_tp(uint64_t iteration_count)
{
    __m512i stream_0 = _mm512_setzero_si512();
    __m512i stream_1 = _mm512_setzero_si512();
    __m512i stream_2 = _mm512_setzero_si512();
    __m512i stream_3 = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi8(1);
    __m512i operand_b = _mm512_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm512_dpbusd_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm512_dpbusd_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm512_dpbusd_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm512_dpbusd_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * VNNI INTEGER DOT PRODUCTS -- VPDPBUSDS (saturating uint8*int8 -> int32)
 * ======================================================================== */

/* VPDPBUSDS xmm -- 128-bit saturating */
static double vpdpbusds_xmm_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi8(1);
    __m128i operand_b = _mm_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpbusds_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpbusds_xmm_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_setzero_si128();
    __m128i stream_1 = _mm_setzero_si128();
    __m128i stream_2 = _mm_setzero_si128();
    __m128i stream_3 = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi8(1);
    __m128i operand_b = _mm_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_dpbusds_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm_dpbusds_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm_dpbusds_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm_dpbusds_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPBUSDS ymm -- 256-bit saturating */
static double vpdpbusds_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi8(1);
    __m256i operand_b = _mm256_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbusds_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpbusds_ymm_tp(uint64_t iteration_count)
{
    __m256i stream_0 = _mm256_setzero_si256();
    __m256i stream_1 = _mm256_setzero_si256();
    __m256i stream_2 = _mm256_setzero_si256();
    __m256i stream_3 = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi8(1);
    __m256i operand_b = _mm256_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_dpbusds_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm256_dpbusds_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm256_dpbusds_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm256_dpbusds_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPBUSDS zmm -- 512-bit saturating */
static double vpdpbusds_zmm_lat(uint64_t iteration_count)
{
    __m512i accumulator = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi8(1);
    __m512i operand_b = _mm512_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm512_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbusds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbusds_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpbusds_zmm_tp(uint64_t iteration_count)
{
    __m512i stream_0 = _mm512_setzero_si512();
    __m512i stream_1 = _mm512_setzero_si512();
    __m512i stream_2 = _mm512_setzero_si512();
    __m512i stream_3 = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi8(1);
    __m512i operand_b = _mm512_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm512_dpbusds_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm512_dpbusds_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm512_dpbusds_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm512_dpbusds_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * VNNI INTEGER DOT PRODUCTS -- VPDPWSSD (int16*int16 -> int32)
 * ======================================================================== */

/* VPDPWSSD xmm -- 128-bit */
static double vpdpwssd_xmm_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi16(3);
    __m128i operand_b = _mm_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpwssd_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpwssd_xmm_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_setzero_si128();
    __m128i stream_1 = _mm_setzero_si128();
    __m128i stream_2 = _mm_setzero_si128();
    __m128i stream_3 = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi16(3);
    __m128i operand_b = _mm_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_dpwssd_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm_dpwssd_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm_dpwssd_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm_dpwssd_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPWSSD ymm -- 256-bit */
static double vpdpwssd_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi16(3);
    __m256i operand_b = _mm256_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpwssd_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpwssd_ymm_tp(uint64_t iteration_count)
{
    __m256i stream_0 = _mm256_setzero_si256();
    __m256i stream_1 = _mm256_setzero_si256();
    __m256i stream_2 = _mm256_setzero_si256();
    __m256i stream_3 = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi16(3);
    __m256i operand_b = _mm256_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_dpwssd_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm256_dpwssd_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm256_dpwssd_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm256_dpwssd_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPWSSD zmm -- 512-bit */
static double vpdpwssd_zmm_lat(uint64_t iteration_count)
{
    __m512i accumulator = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi16(3);
    __m512i operand_b = _mm512_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm512_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpwssd_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpwssd_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpwssd_zmm_tp(uint64_t iteration_count)
{
    __m512i stream_0 = _mm512_setzero_si512();
    __m512i stream_1 = _mm512_setzero_si512();
    __m512i stream_2 = _mm512_setzero_si512();
    __m512i stream_3 = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi16(3);
    __m512i operand_b = _mm512_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm512_dpwssd_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm512_dpwssd_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm512_dpwssd_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm512_dpwssd_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * VNNI INTEGER DOT PRODUCTS -- VPDPWSSDS (saturating int16*int16 -> int32)
 * ======================================================================== */

/* VPDPWSSDS xmm -- 128-bit saturating */
static double vpdpwssds_xmm_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi16(3);
    __m128i operand_b = _mm_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm_dpwssds_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpwssds_xmm_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_setzero_si128();
    __m128i stream_1 = _mm_setzero_si128();
    __m128i stream_2 = _mm_setzero_si128();
    __m128i stream_3 = _mm_setzero_si128();
    __m128i operand_a = _mm_set1_epi16(3);
    __m128i operand_b = _mm_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_dpwssds_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm_dpwssds_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm_dpwssds_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm_dpwssds_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPWSSDS ymm -- 256-bit saturating */
static double vpdpwssds_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi16(3);
    __m256i operand_b = _mm256_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpwssds_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpwssds_ymm_tp(uint64_t iteration_count)
{
    __m256i stream_0 = _mm256_setzero_si256();
    __m256i stream_1 = _mm256_setzero_si256();
    __m256i stream_2 = _mm256_setzero_si256();
    __m256i stream_3 = _mm256_setzero_si256();
    __m256i operand_a = _mm256_set1_epi16(3);
    __m256i operand_b = _mm256_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_dpwssds_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm256_dpwssds_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm256_dpwssds_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm256_dpwssds_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPDPWSSDS zmm -- 512-bit saturating */
static double vpdpwssds_zmm_lat(uint64_t iteration_count)
{
    __m512i accumulator = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi16(3);
    __m512i operand_b = _mm512_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm512_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpwssds_epi32(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpwssds_epi32(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpdpwssds_zmm_tp(uint64_t iteration_count)
{
    __m512i stream_0 = _mm512_setzero_si512();
    __m512i stream_1 = _mm512_setzero_si512();
    __m512i stream_2 = _mm512_setzero_si512();
    __m512i stream_3 = _mm512_setzero_si512();
    __m512i operand_a = _mm512_set1_epi16(3);
    __m512i operand_b = _mm512_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm512_dpwssds_epi32(stream_0, operand_a, operand_b);
        stream_1 = _mm512_dpwssds_epi32(stream_1, operand_a, operand_b);
        stream_2 = _mm512_dpwssds_epi32(stream_2, operand_a, operand_b);
        stream_3 = _mm512_dpwssds_epi32(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * BF16 DOT PRODUCT -- VDPBF16PS
 * ======================================================================== */

/* VDPBF16PS ymm -- BF16 dot product, 256-bit */
static double vdpbf16ps_ymm_lat(uint64_t iteration_count)
{
    __m256 accumulator = _mm256_set1_ps(1.0f);
    __m256i bf16_raw_bits = _mm256_set1_epi16(0x3F80); /* 1.0 in bf16 */
    __m256bh operand_a = (__m256bh)bf16_raw_bits;
    __m256bh operand_b = (__m256bh)bf16_raw_bits;
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_dpbf16_ps(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbf16_ps(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbf16_ps(accumulator, operand_a, operand_b);
        accumulator = _mm256_dpbf16_ps(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vdpbf16ps_ymm_tp(uint64_t iteration_count)
{
    __m256 stream_0 = _mm256_set1_ps(1.0f);
    __m256 stream_1 = _mm256_set1_ps(2.0f);
    __m256 stream_2 = _mm256_set1_ps(3.0f);
    __m256 stream_3 = _mm256_set1_ps(4.0f);
    __m256i bf16_ymm_bits = _mm256_set1_epi16(0x3F80);
    __m256bh operand_a = (__m256bh)bf16_ymm_bits;
    __m256bh operand_b = (__m256bh)bf16_ymm_bits;
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_dpbf16_ps(stream_0, operand_a, operand_b);
        stream_1 = _mm256_dpbf16_ps(stream_1, operand_a, operand_b);
        stream_2 = _mm256_dpbf16_ps(stream_2, operand_a, operand_b);
        stream_3 = _mm256_dpbf16_ps(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256 sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VDPBF16PS zmm -- BF16 dot product, 512-bit */
static double vdpbf16ps_zmm_lat(uint64_t iteration_count)
{
    __m512 accumulator = _mm512_set1_ps(1.0f);
    __m512i bf16_zmm_init = _mm512_set1_epi16(0x3F80);
    __m512bh operand_a = (__m512bh)bf16_zmm_init;
    __m512bh operand_b = (__m512bh)bf16_zmm_init;
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm512_dpbf16_ps(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbf16_ps(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbf16_ps(accumulator, operand_a, operand_b);
        accumulator = _mm512_dpbf16_ps(accumulator, operand_a, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512 sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vdpbf16ps_zmm_tp(uint64_t iteration_count)
{
    __m512 stream_0 = _mm512_set1_ps(1.0f);
    __m512 stream_1 = _mm512_set1_ps(2.0f);
    __m512 stream_2 = _mm512_set1_ps(3.0f);
    __m512 stream_3 = _mm512_set1_ps(4.0f);
    __m512i bf16_zmm_tp_bits = _mm512_set1_epi16(0x3F80);
    __m512bh operand_a = (__m512bh)bf16_zmm_tp_bits;
    __m512bh operand_b = (__m512bh)bf16_zmm_tp_bits;
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_a), "+x"(operand_b));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm512_dpbf16_ps(stream_0, operand_a, operand_b);
        stream_1 = _mm512_dpbf16_ps(stream_1, operand_a, operand_b);
        stream_2 = _mm512_dpbf16_ps(stream_2, operand_a, operand_b);
        stream_3 = _mm512_dpbf16_ps(stream_3, operand_a, operand_b);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512 sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * DOT-PRODUCT BUILDING BLOCKS -- PMADDWD
 * ======================================================================== */

/* PMADDWD xmm -- SSE2 int16 multiply-add pairs, 128-bit */
static double pmaddwd_xmm_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi16(3);
    __m128i operand_source = _mm_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_madd_epi16(accumulator, operand_source);
        accumulator = _mm_madd_epi16(accumulator, operand_source);
        accumulator = _mm_madd_epi16(accumulator, operand_source);
        accumulator = _mm_madd_epi16(accumulator, operand_source);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double pmaddwd_xmm_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_set1_epi16(1);
    __m128i stream_1 = _mm_set1_epi16(2);
    __m128i stream_2 = _mm_set1_epi16(3);
    __m128i stream_3 = _mm_set1_epi16(4);
    __m128i operand_source = _mm_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_madd_epi16(stream_0, operand_source);
        stream_1 = _mm_madd_epi16(stream_1, operand_source);
        stream_2 = _mm_madd_epi16(stream_2, operand_source);
        stream_3 = _mm_madd_epi16(stream_3, operand_source);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPMADDWD ymm -- AVX2, 256-bit */
static double vpmaddwd_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi16(3);
    __m256i operand_source = _mm256_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_madd_epi16(accumulator, operand_source);
        accumulator = _mm256_madd_epi16(accumulator, operand_source);
        accumulator = _mm256_madd_epi16(accumulator, operand_source);
        accumulator = _mm256_madd_epi16(accumulator, operand_source);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpmaddwd_ymm_tp(uint64_t iteration_count)
{
    __m256i stream_0 = _mm256_set1_epi16(1);
    __m256i stream_1 = _mm256_set1_epi16(2);
    __m256i stream_2 = _mm256_set1_epi16(3);
    __m256i stream_3 = _mm256_set1_epi16(4);
    __m256i operand_source = _mm256_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_madd_epi16(stream_0, operand_source);
        stream_1 = _mm256_madd_epi16(stream_1, operand_source);
        stream_2 = _mm256_madd_epi16(stream_2, operand_source);
        stream_3 = _mm256_madd_epi16(stream_3, operand_source);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPMADDWD zmm -- AVX-512, 512-bit */
static double vpmaddwd_zmm_lat(uint64_t iteration_count)
{
    __m512i accumulator = _mm512_set1_epi16(3);
    __m512i operand_source = _mm512_set1_epi16(5);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm512_madd_epi16(accumulator, operand_source);
        accumulator = _mm512_madd_epi16(accumulator, operand_source);
        accumulator = _mm512_madd_epi16(accumulator, operand_source);
        accumulator = _mm512_madd_epi16(accumulator, operand_source);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpmaddwd_zmm_tp(uint64_t iteration_count)
{
    __m512i stream_0 = _mm512_set1_epi16(1);
    __m512i stream_1 = _mm512_set1_epi16(2);
    __m512i stream_2 = _mm512_set1_epi16(3);
    __m512i stream_3 = _mm512_set1_epi16(4);
    __m512i operand_source = _mm512_set1_epi16(5);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm512_madd_epi16(stream_0, operand_source);
        stream_1 = _mm512_madd_epi16(stream_1, operand_source);
        stream_2 = _mm512_madd_epi16(stream_2, operand_source);
        stream_3 = _mm512_madd_epi16(stream_3, operand_source);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * DOT-PRODUCT BUILDING BLOCKS -- PMADDUBSW
 * ======================================================================== */

/* PMADDUBSW xmm -- SSSE3 uint8*int8 multiply-add, 128-bit */
static double pmaddubsw_xmm_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi8(1);
    __m128i operand_source = _mm_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm_maddubs_epi16(accumulator, operand_source);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double pmaddubsw_xmm_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_set1_epi8(1);
    __m128i stream_1 = _mm_set1_epi8(2);
    __m128i stream_2 = _mm_set1_epi8(3);
    __m128i stream_3 = _mm_set1_epi8(4);
    __m128i operand_source = _mm_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_maddubs_epi16(stream_0, operand_source);
        stream_1 = _mm_maddubs_epi16(stream_1, operand_source);
        stream_2 = _mm_maddubs_epi16(stream_2, operand_source);
        stream_3 = _mm_maddubs_epi16(stream_3, operand_source);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPMADDUBSW ymm -- AVX2, 256-bit */
static double vpmaddubsw_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi8(1);
    __m256i operand_source = _mm256_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm256_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm256_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm256_maddubs_epi16(accumulator, operand_source);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpmaddubsw_ymm_tp(uint64_t iteration_count)
{
    __m256i stream_0 = _mm256_set1_epi8(1);
    __m256i stream_1 = _mm256_set1_epi8(2);
    __m256i stream_2 = _mm256_set1_epi8(3);
    __m256i stream_3 = _mm256_set1_epi8(4);
    __m256i operand_source = _mm256_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_maddubs_epi16(stream_0, operand_source);
        stream_1 = _mm256_maddubs_epi16(stream_1, operand_source);
        stream_2 = _mm256_maddubs_epi16(stream_2, operand_source);
        stream_3 = _mm256_maddubs_epi16(stream_3, operand_source);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VPMADDUBSW zmm -- AVX-512, 512-bit */
static double vpmaddubsw_zmm_lat(uint64_t iteration_count)
{
    __m512i accumulator = _mm512_set1_epi8(1);
    __m512i operand_source = _mm512_set1_epi8(2);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm512_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm512_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm512_maddubs_epi16(accumulator, operand_source);
        accumulator = _mm512_maddubs_epi16(accumulator, operand_source);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vpmaddubsw_zmm_tp(uint64_t iteration_count)
{
    __m512i stream_0 = _mm512_set1_epi8(1);
    __m512i stream_1 = _mm512_set1_epi8(2);
    __m512i stream_2 = _mm512_set1_epi8(3);
    __m512i stream_3 = _mm512_set1_epi8(4);
    __m512i operand_source = _mm512_set1_epi8(2);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm512_maddubs_epi16(stream_0, operand_source);
        stream_1 = _mm512_maddubs_epi16(stream_1, operand_source);
        stream_2 = _mm512_maddubs_epi16(stream_2, operand_source);
        stream_3 = _mm512_maddubs_epi16(stream_3, operand_source);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m512i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * DOT-PRODUCT BUILDING BLOCKS -- MPSADBW
 * ======================================================================== */

/* MPSADBW xmm -- SSE4.1 multiple-precision SAD, 128-bit */
static double mpsadbw_xmm_lat(uint64_t iteration_count)
{
    __m128i accumulator = _mm_set1_epi8(10);
    __m128i operand_source = _mm_set1_epi8(11);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm_mpsadbw_epu8(accumulator, operand_source, 0);
        accumulator = _mm_mpsadbw_epu8(accumulator, operand_source, 0);
        accumulator = _mm_mpsadbw_epu8(accumulator, operand_source, 0);
        accumulator = _mm_mpsadbw_epu8(accumulator, operand_source, 0);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double mpsadbw_xmm_tp(uint64_t iteration_count)
{
    __m128i stream_0 = _mm_set1_epi8(10);
    __m128i stream_1 = _mm_set1_epi8(11);
    __m128i stream_2 = _mm_set1_epi8(12);
    __m128i stream_3 = _mm_set1_epi8(13);
    __m128i operand_source = _mm_set1_epi8(14);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm_mpsadbw_epu8(stream_0, operand_source, 0);
        stream_1 = _mm_mpsadbw_epu8(stream_1, operand_source, 0);
        stream_2 = _mm_mpsadbw_epu8(stream_2, operand_source, 0);
        stream_3 = _mm_mpsadbw_epu8(stream_3, operand_source, 0);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m128i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* VMPSADBW ymm -- AVX2 multiple-precision SAD, 256-bit */
static double vmpsadbw_ymm_lat(uint64_t iteration_count)
{
    __m256i accumulator = _mm256_set1_epi8(10);
    __m256i operand_source = _mm256_set1_epi8(11);
    __asm__ volatile("" : "+x"(accumulator), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        accumulator = _mm256_mpsadbw_epu8(accumulator, operand_source, 0);
        accumulator = _mm256_mpsadbw_epu8(accumulator, operand_source, 0);
        accumulator = _mm256_mpsadbw_epu8(accumulator, operand_source, 0);
        accumulator = _mm256_mpsadbw_epu8(accumulator, operand_source, 0);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

static double vmpsadbw_ymm_tp(uint64_t iteration_count)
{
    __m256i stream_0 = _mm256_set1_epi8(10);
    __m256i stream_1 = _mm256_set1_epi8(11);
    __m256i stream_2 = _mm256_set1_epi8(12);
    __m256i stream_3 = _mm256_set1_epi8(13);
    __m256i operand_source = _mm256_set1_epi8(14);
    __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1), "+x"(stream_2),
                          "+x"(stream_3), "+x"(operand_source));
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        stream_0 = _mm256_mpsadbw_epu8(stream_0, operand_source, 0);
        stream_1 = _mm256_mpsadbw_epu8(stream_1, operand_source, 0);
        stream_2 = _mm256_mpsadbw_epu8(stream_2, operand_source, 0);
        stream_3 = _mm256_mpsadbw_epu8(stream_3, operand_source, 0);
        __asm__ volatile("" : "+x"(stream_0), "+x"(stream_1),
                              "+x"(stream_2), "+x"(stream_3));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile __m256i sink = stream_0; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 4);
}

/* ========================================================================
 * MEASUREMENT ENTRY TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    int width_bits;
    const char *isa_extension;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} DotProductProbeEntry;

static const DotProductProbeEntry probe_table[] = {
    /* --- Strict FP dot products --- */
    {"DPPD",        128, "SSE4.1",       dppd_xmm_lat,       dppd_xmm_tp},
    {"DPPS",        128, "SSE4.1",       dpps_xmm_lat,       dpps_xmm_tp},
    {"VDPPS",       256, "AVX",          vdpps_ymm_lat,      vdpps_ymm_tp},

    /* --- VNNI int8 dot products (VPDPBUSD) --- */
    {"VPDPBUSD",    128, "AVX512_VNNI+VL", vpdpbusd_xmm_lat,  vpdpbusd_xmm_tp},
    {"VPDPBUSD",    256, "AVX512_VNNI+VL", vpdpbusd_ymm_lat,  vpdpbusd_ymm_tp},
    {"VPDPBUSD",    512, "AVX512_VNNI",    vpdpbusd_zmm_lat,  vpdpbusd_zmm_tp},

    /* --- VNNI int8 saturating dot products (VPDPBUSDS) --- */
    {"VPDPBUSDS",   128, "AVX512_VNNI+VL", vpdpbusds_xmm_lat, vpdpbusds_xmm_tp},
    {"VPDPBUSDS",   256, "AVX512_VNNI+VL", vpdpbusds_ymm_lat, vpdpbusds_ymm_tp},
    {"VPDPBUSDS",   512, "AVX512_VNNI",    vpdpbusds_zmm_lat, vpdpbusds_zmm_tp},

    /* --- VNNI int16 dot products (VPDPWSSD) --- */
    {"VPDPWSSD",    128, "AVX512_VNNI+VL", vpdpwssd_xmm_lat,  vpdpwssd_xmm_tp},
    {"VPDPWSSD",    256, "AVX512_VNNI+VL", vpdpwssd_ymm_lat,  vpdpwssd_ymm_tp},
    {"VPDPWSSD",    512, "AVX512_VNNI",    vpdpwssd_zmm_lat,  vpdpwssd_zmm_tp},

    /* --- VNNI int16 saturating dot products (VPDPWSSDS) --- */
    {"VPDPWSSDS",   128, "AVX512_VNNI+VL", vpdpwssds_xmm_lat, vpdpwssds_xmm_tp},
    {"VPDPWSSDS",   256, "AVX512_VNNI+VL", vpdpwssds_ymm_lat, vpdpwssds_ymm_tp},
    {"VPDPWSSDS",   512, "AVX512_VNNI",    vpdpwssds_zmm_lat, vpdpwssds_zmm_tp},

    /* --- BF16 dot products --- */
    {"VDPBF16PS",   256, "AVX512_BF16+VL", vdpbf16ps_ymm_lat, vdpbf16ps_ymm_tp},
    {"VDPBF16PS",   512, "AVX512_BF16",    vdpbf16ps_zmm_lat, vdpbf16ps_zmm_tp},

    /* --- Building blocks: PMADDWD --- */
    {"PMADDWD",     128, "SSE2",           pmaddwd_xmm_lat,    pmaddwd_xmm_tp},
    {"VPMADDWD",    256, "AVX2",           vpmaddwd_ymm_lat,   vpmaddwd_ymm_tp},
    {"VPMADDWD",    512, "AVX-512BW",      vpmaddwd_zmm_lat,   vpmaddwd_zmm_tp},

    /* --- Building blocks: PMADDUBSW --- */
    {"PMADDUBSW",   128, "SSSE3",          pmaddubsw_xmm_lat,  pmaddubsw_xmm_tp},
    {"VPMADDUBSW",  256, "AVX2",           vpmaddubsw_ymm_lat, vpmaddubsw_ymm_tp},
    {"VPMADDUBSW",  512, "AVX-512BW",      vpmaddubsw_zmm_lat, vpmaddubsw_zmm_tp},

    /* --- Building blocks: MPSADBW --- */
    {"MPSADBW",     128, "SSE4.1",         mpsadbw_xmm_lat,    mpsadbw_xmm_tp},
    {"VMPSADBW",    256, "AVX2",           vmpsadbw_ymm_lat,   vmpsadbw_ymm_tp},

    {NULL, 0, NULL, NULL, NULL}
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
        sm_zen_print_header("dotproduct_complete", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n", "dot product");
    }

    /* Print CSV header */
    printf("mnemonic,width_bits,latency_cycles,throughput_cycles,extension\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const DotProductProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const DotProductProbeEntry *current_entry = &probe_table[entry_idx];

        double measured_latency = -1.0;
        double measured_throughput = -1.0;

        if (current_entry->measure_latency)
            measured_latency = current_entry->measure_latency(probe_config.iterations);
        if (current_entry->measure_throughput)
            measured_throughput = current_entry->measure_throughput(probe_config.iterations);

        printf("%s,%d,%.4f,%.4f,%s\n",
               current_entry->mnemonic,
               current_entry->width_bits,
               measured_latency,
               measured_throughput,
               current_entry->isa_extension);
    }

    return 0;
}
