#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_sha.c -- SHA-NI (SHA-1 and SHA-256 hardware acceleration) on Zen 4.
 *
 * All SHA-NI instructions are 128-bit only (no AVX-512 widening).
 * Measures latency/throughput of sha256rnds2, sha256msg1, sha256msg2,
 * sha1rnds4, sha1msg1, sha1msg2, sha1nexte.
 */

/* ── SHA-256 round function: sha256rnds2 xmm, xmm, xmm0 ── */

static double sha256rnds2_latency(uint64_t iterations)
{
    __m128i state_lo    = _mm_set_epi32(0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a);
    __m128i state_hi    = _mm_set_epi32(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19);
    __m128i msg_key     = _mm_set_epi32(0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5);
    __asm__ volatile("" : "+x"(state_hi), "+x"(msg_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        state_lo = _mm_sha256rnds2_epu32(state_lo, state_hi, msg_key);
        state_lo = _mm_sha256rnds2_epu32(state_lo, state_hi, msg_key);
        state_lo = _mm_sha256rnds2_epu32(state_lo, state_hi, msg_key);
        state_lo = _mm_sha256rnds2_epu32(state_lo, state_hi, msg_key);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(state_lo, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double sha256rnds2_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set_epi32(0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a);
    __m128i acc1 = _mm_set_epi32(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19);
    __m128i acc2 = _mm_set_epi32(0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5);
    __m128i acc3 = _mm_set_epi32(0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5);
    __m128i acc4 = _mm_set_epi32(0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3);
    __m128i acc5 = _mm_set_epi32(0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174);
    __m128i acc6 = _mm_set_epi32(0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc);
    __m128i acc7 = _mm_set_epi32(0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da);
    __m128i state_hi = _mm_set_epi32(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19);
    __m128i msg_key  = _mm_set_epi32(0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5);
    __asm__ volatile("" : "+x"(state_hi), "+x"(msg_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_sha256rnds2_epu32(acc0, state_hi, msg_key);
        acc1 = _mm_sha256rnds2_epu32(acc1, state_hi, msg_key);
        acc2 = _mm_sha256rnds2_epu32(acc2, state_hi, msg_key);
        acc3 = _mm_sha256rnds2_epu32(acc3, state_hi, msg_key);
        acc4 = _mm_sha256rnds2_epu32(acc4, state_hi, msg_key);
        acc5 = _mm_sha256rnds2_epu32(acc5, state_hi, msg_key);
        acc6 = _mm_sha256rnds2_epu32(acc6, state_hi, msg_key);
        acc7 = _mm_sha256rnds2_epu32(acc7, state_hi, msg_key);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── sha256msg1 xmm, xmm ── */

static double sha256msg1_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set_epi32(0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a);
    __m128i operand_b   = _mm_set_epi32(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_sha256msg1_epu32(accumulator, operand_b);
        accumulator = _mm_sha256msg1_epu32(accumulator, operand_b);
        accumulator = _mm_sha256msg1_epu32(accumulator, operand_b);
        accumulator = _mm_sha256msg1_epu32(accumulator, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double sha256msg1_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi32(0x01);
    __m128i acc1 = _mm_set1_epi32(0x02);
    __m128i acc2 = _mm_set1_epi32(0x03);
    __m128i acc3 = _mm_set1_epi32(0x04);
    __m128i acc4 = _mm_set1_epi32(0x05);
    __m128i acc5 = _mm_set1_epi32(0x06);
    __m128i acc6 = _mm_set1_epi32(0x07);
    __m128i acc7 = _mm_set1_epi32(0x08);
    __m128i operand_b = _mm_set_epi32(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_sha256msg1_epu32(acc0, operand_b);
        acc1 = _mm_sha256msg1_epu32(acc1, operand_b);
        acc2 = _mm_sha256msg1_epu32(acc2, operand_b);
        acc3 = _mm_sha256msg1_epu32(acc3, operand_b);
        acc4 = _mm_sha256msg1_epu32(acc4, operand_b);
        acc5 = _mm_sha256msg1_epu32(acc5, operand_b);
        acc6 = _mm_sha256msg1_epu32(acc6, operand_b);
        acc7 = _mm_sha256msg1_epu32(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── sha256msg2 xmm, xmm ── */

static double sha256msg2_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set_epi32(0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a);
    __m128i operand_b   = _mm_set_epi32(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_sha256msg2_epu32(accumulator, operand_b);
        accumulator = _mm_sha256msg2_epu32(accumulator, operand_b);
        accumulator = _mm_sha256msg2_epu32(accumulator, operand_b);
        accumulator = _mm_sha256msg2_epu32(accumulator, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double sha256msg2_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi32(0x01);
    __m128i acc1 = _mm_set1_epi32(0x02);
    __m128i acc2 = _mm_set1_epi32(0x03);
    __m128i acc3 = _mm_set1_epi32(0x04);
    __m128i acc4 = _mm_set1_epi32(0x05);
    __m128i acc5 = _mm_set1_epi32(0x06);
    __m128i acc6 = _mm_set1_epi32(0x07);
    __m128i acc7 = _mm_set1_epi32(0x08);
    __m128i operand_b = _mm_set_epi32(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_sha256msg2_epu32(acc0, operand_b);
        acc1 = _mm_sha256msg2_epu32(acc1, operand_b);
        acc2 = _mm_sha256msg2_epu32(acc2, operand_b);
        acc3 = _mm_sha256msg2_epu32(acc3, operand_b);
        acc4 = _mm_sha256msg2_epu32(acc4, operand_b);
        acc5 = _mm_sha256msg2_epu32(acc5, operand_b);
        acc6 = _mm_sha256msg2_epu32(acc6, operand_b);
        acc7 = _mm_sha256msg2_epu32(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── SHA-1: sha1rnds4 xmm, xmm, imm8 ── */

static double sha1rnds4_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set_epi32(0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476);
    __m128i operand_b   = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_sha1rnds4_epu32(accumulator, operand_b, 0);
        accumulator = _mm_sha1rnds4_epu32(accumulator, operand_b, 0);
        accumulator = _mm_sha1rnds4_epu32(accumulator, operand_b, 0);
        accumulator = _mm_sha1rnds4_epu32(accumulator, operand_b, 0);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double sha1rnds4_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi32(0x01);
    __m128i acc1 = _mm_set1_epi32(0x02);
    __m128i acc2 = _mm_set1_epi32(0x03);
    __m128i acc3 = _mm_set1_epi32(0x04);
    __m128i acc4 = _mm_set1_epi32(0x05);
    __m128i acc5 = _mm_set1_epi32(0x06);
    __m128i acc6 = _mm_set1_epi32(0x07);
    __m128i acc7 = _mm_set1_epi32(0x08);
    __m128i operand_b = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_sha1rnds4_epu32(acc0, operand_b, 0);
        acc1 = _mm_sha1rnds4_epu32(acc1, operand_b, 0);
        acc2 = _mm_sha1rnds4_epu32(acc2, operand_b, 0);
        acc3 = _mm_sha1rnds4_epu32(acc3, operand_b, 0);
        acc4 = _mm_sha1rnds4_epu32(acc4, operand_b, 0);
        acc5 = _mm_sha1rnds4_epu32(acc5, operand_b, 0);
        acc6 = _mm_sha1rnds4_epu32(acc6, operand_b, 0);
        acc7 = _mm_sha1rnds4_epu32(acc7, operand_b, 0);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── sha1msg1 xmm, xmm ── */

static double sha1msg1_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set_epi32(0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476);
    __m128i operand_b   = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_sha1msg1_epu32(accumulator, operand_b);
        accumulator = _mm_sha1msg1_epu32(accumulator, operand_b);
        accumulator = _mm_sha1msg1_epu32(accumulator, operand_b);
        accumulator = _mm_sha1msg1_epu32(accumulator, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double sha1msg1_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi32(0x01);
    __m128i acc1 = _mm_set1_epi32(0x02);
    __m128i acc2 = _mm_set1_epi32(0x03);
    __m128i acc3 = _mm_set1_epi32(0x04);
    __m128i acc4 = _mm_set1_epi32(0x05);
    __m128i acc5 = _mm_set1_epi32(0x06);
    __m128i acc6 = _mm_set1_epi32(0x07);
    __m128i acc7 = _mm_set1_epi32(0x08);
    __m128i operand_b = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_sha1msg1_epu32(acc0, operand_b);
        acc1 = _mm_sha1msg1_epu32(acc1, operand_b);
        acc2 = _mm_sha1msg1_epu32(acc2, operand_b);
        acc3 = _mm_sha1msg1_epu32(acc3, operand_b);
        acc4 = _mm_sha1msg1_epu32(acc4, operand_b);
        acc5 = _mm_sha1msg1_epu32(acc5, operand_b);
        acc6 = _mm_sha1msg1_epu32(acc6, operand_b);
        acc7 = _mm_sha1msg1_epu32(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── sha1msg2 xmm, xmm ── */

static double sha1msg2_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set_epi32(0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476);
    __m128i operand_b   = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_sha1msg2_epu32(accumulator, operand_b);
        accumulator = _mm_sha1msg2_epu32(accumulator, operand_b);
        accumulator = _mm_sha1msg2_epu32(accumulator, operand_b);
        accumulator = _mm_sha1msg2_epu32(accumulator, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double sha1msg2_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi32(0x01);
    __m128i acc1 = _mm_set1_epi32(0x02);
    __m128i acc2 = _mm_set1_epi32(0x03);
    __m128i acc3 = _mm_set1_epi32(0x04);
    __m128i acc4 = _mm_set1_epi32(0x05);
    __m128i acc5 = _mm_set1_epi32(0x06);
    __m128i acc6 = _mm_set1_epi32(0x07);
    __m128i acc7 = _mm_set1_epi32(0x08);
    __m128i operand_b = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_sha1msg2_epu32(acc0, operand_b);
        acc1 = _mm_sha1msg2_epu32(acc1, operand_b);
        acc2 = _mm_sha1msg2_epu32(acc2, operand_b);
        acc3 = _mm_sha1msg2_epu32(acc3, operand_b);
        acc4 = _mm_sha1msg2_epu32(acc4, operand_b);
        acc5 = _mm_sha1msg2_epu32(acc5, operand_b);
        acc6 = _mm_sha1msg2_epu32(acc6, operand_b);
        acc7 = _mm_sha1msg2_epu32(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── sha1nexte xmm, xmm ── */

static double sha1nexte_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set_epi32(0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476);
    __m128i operand_b   = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_sha1nexte_epu32(accumulator, operand_b);
        accumulator = _mm_sha1nexte_epu32(accumulator, operand_b);
        accumulator = _mm_sha1nexte_epu32(accumulator, operand_b);
        accumulator = _mm_sha1nexte_epu32(accumulator, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double sha1nexte_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi32(0x01);
    __m128i acc1 = _mm_set1_epi32(0x02);
    __m128i acc2 = _mm_set1_epi32(0x03);
    __m128i acc3 = _mm_set1_epi32(0x04);
    __m128i acc4 = _mm_set1_epi32(0x05);
    __m128i acc5 = _mm_set1_epi32(0x06);
    __m128i acc6 = _mm_set1_epi32(0x07);
    __m128i acc7 = _mm_set1_epi32(0x08);
    __m128i operand_b = _mm_set_epi32(0xC3D2E1F0, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_sha1nexte_epu32(acc0, operand_b);
        acc1 = _mm_sha1nexte_epu32(acc1, operand_b);
        acc2 = _mm_sha1nexte_epu32(acc2, operand_b);
        acc3 = _mm_sha1nexte_epu32(acc3, operand_b);
        acc4 = _mm_sha1nexte_epu32(acc4, operand_b);
        acc5 = _mm_sha1nexte_epu32(acc5, operand_b);
        acc6 = _mm_sha1nexte_epu32(acc6, operand_b);
        acc7 = _mm_sha1nexte_epu32(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    sm_zen_print_header("sha", &cfg, &feat);
    printf("\n");

    printf("=== SHA-256 (SHA-NI, 128-bit only) ===\n\n");

    printf("--- sha256rnds2 (2 rounds of SHA-256) ---\n");
    double sha256rnds2_lat = sha256rnds2_latency(cfg.iterations);
    double sha256rnds2_tp  = sha256rnds2_throughput(cfg.iterations);
    printf("sha256rnds2_latency_cycles=%.4f\n", sha256rnds2_lat);
    printf("sha256rnds2_throughput_cycles=%.4f\n\n", sha256rnds2_tp);

    printf("--- sha256msg1 (message schedule part 1) ---\n");
    double sha256msg1_lat = sha256msg1_latency(cfg.iterations);
    double sha256msg1_tp  = sha256msg1_throughput(cfg.iterations);
    printf("sha256msg1_latency_cycles=%.4f\n", sha256msg1_lat);
    printf("sha256msg1_throughput_cycles=%.4f\n\n", sha256msg1_tp);

    printf("--- sha256msg2 (message schedule part 2) ---\n");
    double sha256msg2_lat = sha256msg2_latency(cfg.iterations);
    double sha256msg2_tp  = sha256msg2_throughput(cfg.iterations);
    printf("sha256msg2_latency_cycles=%.4f\n", sha256msg2_lat);
    printf("sha256msg2_throughput_cycles=%.4f\n\n", sha256msg2_tp);

    printf("=== SHA-1 (SHA-NI, 128-bit only) ===\n\n");

    printf("--- sha1rnds4 (4 rounds of SHA-1) ---\n");
    double sha1rnds4_lat = sha1rnds4_latency(cfg.iterations);
    double sha1rnds4_tp  = sha1rnds4_throughput(cfg.iterations);
    printf("sha1rnds4_latency_cycles=%.4f\n", sha1rnds4_lat);
    printf("sha1rnds4_throughput_cycles=%.4f\n\n", sha1rnds4_tp);

    printf("--- sha1msg1 (SHA-1 message schedule part 1) ---\n");
    double sha1msg1_lat = sha1msg1_latency(cfg.iterations);
    double sha1msg1_tp  = sha1msg1_throughput(cfg.iterations);
    printf("sha1msg1_latency_cycles=%.4f\n", sha1msg1_lat);
    printf("sha1msg1_throughput_cycles=%.4f\n\n", sha1msg1_tp);

    printf("--- sha1msg2 (SHA-1 message schedule part 2) ---\n");
    double sha1msg2_lat = sha1msg2_latency(cfg.iterations);
    double sha1msg2_tp  = sha1msg2_throughput(cfg.iterations);
    printf("sha1msg2_latency_cycles=%.4f\n", sha1msg2_lat);
    printf("sha1msg2_throughput_cycles=%.4f\n\n", sha1msg2_tp);

    printf("--- sha1nexte (SHA-1 next E value) ---\n");
    double sha1nexte_lat = sha1nexte_latency(cfg.iterations);
    double sha1nexte_tp  = sha1nexte_throughput(cfg.iterations);
    printf("sha1nexte_latency_cycles=%.4f\n", sha1nexte_lat);
    printf("sha1nexte_throughput_cycles=%.4f\n\n", sha1nexte_tp);

    if (cfg.csv)
        printf("csv,probe=sha,"
               "sha256rnds2_lat=%.4f,sha256rnds2_tp=%.4f,"
               "sha256msg1_lat=%.4f,sha256msg1_tp=%.4f,"
               "sha256msg2_lat=%.4f,sha256msg2_tp=%.4f,"
               "sha1rnds4_lat=%.4f,sha1rnds4_tp=%.4f,"
               "sha1msg1_lat=%.4f,sha1msg1_tp=%.4f,"
               "sha1msg2_lat=%.4f,sha1msg2_tp=%.4f,"
               "sha1nexte_lat=%.4f,sha1nexte_tp=%.4f\n",
               sha256rnds2_lat, sha256rnds2_tp,
               sha256msg1_lat, sha256msg1_tp,
               sha256msg2_lat, sha256msg2_tp,
               sha1rnds4_lat, sha1rnds4_tp,
               sha1msg1_lat, sha1msg1_tp,
               sha1msg2_lat, sha1msg2_tp,
               sha1nexte_lat, sha1nexte_tp);
    return 0;
}
