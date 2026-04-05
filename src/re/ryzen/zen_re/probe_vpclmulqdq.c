#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_vpclmulqdq.c -- Vectorized carry-less multiplication on Zen 4.
 *
 * Measures pclmulqdq (SSE/128), vpclmulqdq (AVX/256), vpclmulqdq (AVX-512/512).
 * Carry-less multiply is the core of CRC-32, GCM (AES-GCM), and polynomial
 * arithmetic. Each instruction multiplies two 64-bit inputs -> 128-bit result.
 */

/* ── pclmulqdq xmm (SSE, 1 carry-less multiply) ── */

static double pclmulqdq_xmm_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set_epi64x(0xDEADBEEFCAFEBABE, 0x0123456789ABCDEF);
    __m128i operand_b   = _mm_set_epi64x(0x1B, 0x87);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_clmulepi64_si128(accumulator, operand_b, 0x00);
        accumulator = _mm_clmulepi64_si128(accumulator, operand_b, 0x00);
        accumulator = _mm_clmulepi64_si128(accumulator, operand_b, 0x00);
        accumulator = _mm_clmulepi64_si128(accumulator, operand_b, 0x00);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double pclmulqdq_xmm_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set_epi64x(1, 2);
    __m128i acc1 = _mm_set_epi64x(3, 4);
    __m128i acc2 = _mm_set_epi64x(5, 6);
    __m128i acc3 = _mm_set_epi64x(7, 8);
    __m128i acc4 = _mm_set_epi64x(9, 10);
    __m128i acc5 = _mm_set_epi64x(11, 12);
    __m128i acc6 = _mm_set_epi64x(13, 14);
    __m128i acc7 = _mm_set_epi64x(15, 16);
    __m128i operand_b = _mm_set_epi64x(0x1B, 0x87);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_clmulepi64_si128(acc0, operand_b, 0x00);
        acc1 = _mm_clmulepi64_si128(acc1, operand_b, 0x00);
        acc2 = _mm_clmulepi64_si128(acc2, operand_b, 0x00);
        acc3 = _mm_clmulepi64_si128(acc3, operand_b, 0x00);
        acc4 = _mm_clmulepi64_si128(acc4, operand_b, 0x00);
        acc5 = _mm_clmulepi64_si128(acc5, operand_b, 0x00);
        acc6 = _mm_clmulepi64_si128(acc6, operand_b, 0x00);
        acc7 = _mm_clmulepi64_si128(acc7, operand_b, 0x00);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── vpclmulqdq ymm (AVX, 2 carry-less multiplies) ── */

static double vpclmulqdq_ymm_latency(uint64_t iterations)
{
    __m256i accumulator = _mm256_set_epi64x(0xDEADBEEFCAFEBABE, 0x0123456789ABCDEF,
                                             0xFEDCBA9876543210, 0xAAAABBBBCCCCDDDD);
    __m256i operand_b   = _mm256_set1_epi64x(0x1B);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm256_clmulepi64_epi128(accumulator, operand_b, 0x00);
        accumulator = _mm256_clmulepi64_epi128(accumulator, operand_b, 0x00);
        accumulator = _mm256_clmulepi64_epi128(accumulator, operand_b, 0x00);
        accumulator = _mm256_clmulepi64_epi128(accumulator, operand_b, 0x00);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm256_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vpclmulqdq_ymm_throughput(uint64_t iterations)
{
    __m256i acc0 = _mm256_set_epi64x(1, 2, 3, 4);
    __m256i acc1 = _mm256_set_epi64x(5, 6, 7, 8);
    __m256i acc2 = _mm256_set_epi64x(9, 10, 11, 12);
    __m256i acc3 = _mm256_set_epi64x(13, 14, 15, 16);
    __m256i acc4 = _mm256_set_epi64x(17, 18, 19, 20);
    __m256i acc5 = _mm256_set_epi64x(21, 22, 23, 24);
    __m256i acc6 = _mm256_set_epi64x(25, 26, 27, 28);
    __m256i acc7 = _mm256_set_epi64x(29, 30, 31, 32);
    __m256i operand_b = _mm256_set1_epi64x(0x1B);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm256_clmulepi64_epi128(acc0, operand_b, 0x00);
        acc1 = _mm256_clmulepi64_epi128(acc1, operand_b, 0x00);
        acc2 = _mm256_clmulepi64_epi128(acc2, operand_b, 0x00);
        acc3 = _mm256_clmulepi64_epi128(acc3, operand_b, 0x00);
        acc4 = _mm256_clmulepi64_epi128(acc4, operand_b, 0x00);
        acc5 = _mm256_clmulepi64_epi128(acc5, operand_b, 0x00);
        acc6 = _mm256_clmulepi64_epi128(acc6, operand_b, 0x00);
        acc7 = _mm256_clmulepi64_epi128(acc7, operand_b, 0x00);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm256_extract_epi64(acc0, 0) + _mm256_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── vpclmulqdq zmm (AVX-512, 4 carry-less multiplies) ── */

static double vpclmulqdq_zmm_latency(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi64(0xDEADBEEFCAFEBABE);
    __m512i operand_b   = _mm512_set1_epi64(0x1B);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_clmulepi64_epi128(accumulator, operand_b, 0x00);
        accumulator = _mm512_clmulepi64_epi128(accumulator, operand_b, 0x00);
        accumulator = _mm512_clmulepi64_epi128(accumulator, operand_b, 0x00);
        accumulator = _mm512_clmulepi64_epi128(accumulator, operand_b, 0x00);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vpclmulqdq_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(1);
    __m512i acc1 = _mm512_set1_epi64(2);
    __m512i acc2 = _mm512_set1_epi64(3);
    __m512i acc3 = _mm512_set1_epi64(4);
    __m512i acc4 = _mm512_set1_epi64(5);
    __m512i acc5 = _mm512_set1_epi64(6);
    __m512i acc6 = _mm512_set1_epi64(7);
    __m512i acc7 = _mm512_set1_epi64(8);
    __m512i operand_b = _mm512_set1_epi64(0x1B);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_clmulepi64_epi128(acc0, operand_b, 0x00);
        acc1 = _mm512_clmulepi64_epi128(acc1, operand_b, 0x00);
        acc2 = _mm512_clmulepi64_epi128(acc2, operand_b, 0x00);
        acc3 = _mm512_clmulepi64_epi128(acc3, operand_b, 0x00);
        acc4 = _mm512_clmulepi64_epi128(acc4, operand_b, 0x00);
        acc5 = _mm512_clmulepi64_epi128(acc5, operand_b, 0x00);
        acc6 = _mm512_clmulepi64_epi128(acc6, operand_b, 0x00);
        acc7 = _mm512_clmulepi64_epi128(acc7, operand_b, 0x00);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
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

    sm_zen_print_header("vpclmulqdq", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported on this CPU\n");
        return 1;
    }

    printf("--- pclmulqdq xmm (SSE, 1 clmul per instr) ---\n");
    double xmm_latency    = pclmulqdq_xmm_latency(cfg.iterations);
    double xmm_throughput  = pclmulqdq_xmm_throughput(cfg.iterations);
    printf("pclmulqdq_xmm_latency_cycles=%.4f\n", xmm_latency);
    printf("pclmulqdq_xmm_throughput_cycles=%.4f\n", xmm_throughput);
    printf("pclmulqdq_xmm_clmuls_per_cycle=%.2f\n\n", 1.0 / xmm_throughput);

    printf("--- vpclmulqdq ymm (AVX, 2 clmuls per instr) ---\n");
    double ymm_latency    = vpclmulqdq_ymm_latency(cfg.iterations);
    double ymm_throughput  = vpclmulqdq_ymm_throughput(cfg.iterations);
    printf("vpclmulqdq_ymm_latency_cycles=%.4f\n", ymm_latency);
    printf("vpclmulqdq_ymm_throughput_cycles=%.4f\n", ymm_throughput);
    printf("vpclmulqdq_ymm_clmuls_per_cycle=%.2f\n\n", 2.0 / ymm_throughput);

    printf("--- vpclmulqdq zmm (AVX-512, 4 clmuls per instr) ---\n");
    double zmm_latency    = vpclmulqdq_zmm_latency(cfg.iterations);
    double zmm_throughput  = vpclmulqdq_zmm_throughput(cfg.iterations);
    printf("vpclmulqdq_zmm_latency_cycles=%.4f\n", zmm_latency);
    printf("vpclmulqdq_zmm_throughput_cycles=%.4f\n", zmm_throughput);
    printf("vpclmulqdq_zmm_clmuls_per_cycle=%.2f\n\n", 4.0 / zmm_throughput);

    printf("--- width scaling ---\n");
    printf("zmm_vs_xmm_throughput_ratio=%.2fx\n", zmm_throughput / xmm_throughput);
    printf("zmm_vs_ymm_throughput_ratio=%.2fx\n", zmm_throughput / ymm_throughput);

    if (cfg.csv)
        printf("csv,probe=vpclmulqdq,"
               "xmm_lat=%.4f,xmm_tp=%.4f,"
               "ymm_lat=%.4f,ymm_tp=%.4f,"
               "zmm_lat=%.4f,zmm_tp=%.4f\n",
               xmm_latency, xmm_throughput,
               ymm_latency, ymm_throughput,
               zmm_latency, zmm_throughput);
    return 0;
}
