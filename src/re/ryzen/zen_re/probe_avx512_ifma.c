#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_ifma.c -- AVX-512 IFMA (Integer Fused Multiply-Accumulate) on Zen 4.
 *
 * Measures vpmadd52luq (52-bit low multiply-accumulate) and
 * vpmadd52huq (52-bit high multiply-accumulate) latency/throughput.
 * These are the key instructions for big-integer and cryptographic workloads.
 */

static double ifma52lo_latency(uint64_t iterations)
{
    /*
     * Dep chain: acc = acc + (b * c)[51:0], feeding acc back each time.
     * Use asm barrier on operand_b to prevent constant-folding b*c into
     * a simple VPADDQ. This forces the compiler to emit VPMADD52LUQ.
     */
    __m512i accumulator = _mm512_set1_epi64(3);
    __m512i operand_b   = _mm512_set1_epi64(5);
    __m512i operand_c   = _mm512_set1_epi64(7);
    __asm__ volatile("" : "+x"(operand_b), "+x"(operand_c));

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_madd52lo_epu64(accumulator, operand_b, operand_c);
        accumulator = _mm512_madd52lo_epu64(accumulator, operand_b, operand_c);
        accumulator = _mm512_madd52lo_epu64(accumulator, operand_b, operand_c);
        accumulator = _mm512_madd52lo_epu64(accumulator, operand_b, operand_c);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 4);
}

static double ifma52lo_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(1);
    __m512i acc1 = _mm512_set1_epi64(2);
    __m512i acc2 = _mm512_set1_epi64(3);
    __m512i acc3 = _mm512_set1_epi64(4);
    __m512i acc4 = _mm512_set1_epi64(5);
    __m512i acc5 = _mm512_set1_epi64(6);
    __m512i acc6 = _mm512_set1_epi64(7);
    __m512i acc7 = _mm512_set1_epi64(8);
    __m512i operand_b = _mm512_set1_epi64(5);
    __m512i operand_c = _mm512_set1_epi64(7);
    __asm__ volatile("" : "+x"(operand_b), "+x"(operand_c));

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_madd52lo_epu64(acc0, operand_b, operand_c);
        acc1 = _mm512_madd52lo_epu64(acc1, operand_b, operand_c);
        acc2 = _mm512_madd52lo_epu64(acc2, operand_b, operand_c);
        acc3 = _mm512_madd52lo_epu64(acc3, operand_b, operand_c);
        acc4 = _mm512_madd52lo_epu64(acc4, operand_b, operand_c);
        acc5 = _mm512_madd52lo_epu64(acc5, operand_b, operand_c);
        acc6 = _mm512_madd52lo_epu64(acc6, operand_b, operand_c);
        acc7 = _mm512_madd52lo_epu64(acc7, operand_b, operand_c);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

static double ifma52hi_latency(uint64_t iterations)
{
    /* dep chain: acc = acc + (b * c)[103:52], feeding acc back each time */
    __m512i accumulator = _mm512_set1_epi64(3);
    __m512i operand_b   = _mm512_set1_epi64((1ULL << 30));
    __m512i operand_c   = _mm512_set1_epi64((1ULL << 30));
    __asm__ volatile("" : "+x"(operand_b), "+x"(operand_c));

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_madd52hi_epu64(accumulator, operand_b, operand_c);
        accumulator = _mm512_madd52hi_epu64(accumulator, operand_b, operand_c);
        accumulator = _mm512_madd52hi_epu64(accumulator, operand_b, operand_c);
        accumulator = _mm512_madd52hi_epu64(accumulator, operand_b, operand_c);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 4);
}

static double ifma52hi_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(1);
    __m512i acc1 = _mm512_set1_epi64(2);
    __m512i acc2 = _mm512_set1_epi64(3);
    __m512i acc3 = _mm512_set1_epi64(4);
    __m512i acc4 = _mm512_set1_epi64(5);
    __m512i acc5 = _mm512_set1_epi64(6);
    __m512i acc6 = _mm512_set1_epi64(7);
    __m512i acc7 = _mm512_set1_epi64(8);
    __m512i operand_b = _mm512_set1_epi64((1ULL << 30));
    __m512i operand_c = _mm512_set1_epi64((1ULL << 30));
    __asm__ volatile("" : "+x"(operand_b), "+x"(operand_c));

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_madd52hi_epu64(acc0, operand_b, operand_c);
        acc1 = _mm512_madd52hi_epu64(acc1, operand_b, operand_c);
        acc2 = _mm512_madd52hi_epu64(acc2, operand_b, operand_c);
        acc3 = _mm512_madd52hi_epu64(acc3, operand_b, operand_c);
        acc4 = _mm512_madd52hi_epu64(acc4, operand_b, operand_c);
        acc5 = _mm512_madd52hi_epu64(acc5, operand_b, operand_c);
        acc6 = _mm512_madd52hi_epu64(acc6, operand_b, operand_c);
        acc7 = _mm512_madd52hi_epu64(acc7, operand_b, operand_c);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_ifma\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double lo_latency    = ifma52lo_latency(cfg.iterations);
    double lo_throughput  = ifma52lo_throughput(cfg.iterations);
    double hi_latency    = ifma52hi_latency(cfg.iterations);
    double hi_throughput  = ifma52hi_throughput(cfg.iterations);

    printf("vpmadd52luq_latency_cycles=%.4f\n", lo_latency);
    printf("vpmadd52luq_throughput_cycles=%.4f\n", lo_throughput);
    printf("vpmadd52huq_latency_cycles=%.4f\n", hi_latency);
    printf("vpmadd52huq_throughput_cycles=%.4f\n", hi_throughput);
    printf("lo_52bit_ops_per_cycle=%.1f\n", 8.0 / lo_throughput);
    printf("hi_52bit_ops_per_cycle=%.1f\n", 8.0 / hi_throughput);

    if (cfg.csv)
        printf("csv,probe=avx512_ifma,lo_lat=%.4f,lo_tp=%.4f,hi_lat=%.4f,hi_tp=%.4f\n",
               lo_latency, lo_throughput, hi_latency, hi_throughput);
    return 0;
}
