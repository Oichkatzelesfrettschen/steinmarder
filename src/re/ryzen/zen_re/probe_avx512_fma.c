#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_fma.c — AVX-512 FMA latency and throughput on Zen 4.
 *
 * Zen 4 executes AVX-512 as 2x256-bit uops. This probe measures:
 * 1. 512-bit FMA latency (dep chain of vfmadd231ps zmm)
 * 2. 512-bit FMA throughput (independent zmm FMAs)
 * 3. Comparison: 256-bit FMA throughput (same work in ymm)
 * 4. 512-bit frequency throttle detection (sustained zmm workload)
 */

static double avx512_fma_latency(uint64_t iterations)
{
    __m512 acc = _mm512_set1_ps(1.0001f);
    __m512 mul = _mm512_set1_ps(1.0f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc = _mm512_fmadd_ps(acc, mul, acc);
        acc = _mm512_fmadd_ps(acc, mul, acc);
        acc = _mm512_fmadd_ps(acc, mul, acc);
        acc = _mm512_fmadd_ps(acc, mul, acc);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 4);
}

static double avx512_fma_throughput(uint64_t iterations)
{
    __m512 a0 = _mm512_set1_ps(1.0f);
    __m512 a1 = _mm512_set1_ps(1.0001f);
    __m512 a2 = _mm512_set1_ps(1.0002f);
    __m512 a3 = _mm512_set1_ps(1.0003f);
    __m512 a4 = _mm512_set1_ps(1.0004f);
    __m512 a5 = _mm512_set1_ps(1.0005f);
    __m512 a6 = _mm512_set1_ps(1.0006f);
    __m512 a7 = _mm512_set1_ps(1.0007f);
    __m512 b  = _mm512_set1_ps(1.0000001f);
    __m512 c  = _mm512_set1_ps(0.0000001f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        a0 = _mm512_fmadd_ps(a0, b, c);
        a1 = _mm512_fmadd_ps(a1, b, c);
        a2 = _mm512_fmadd_ps(a2, b, c);
        a3 = _mm512_fmadd_ps(a3, b, c);
        a4 = _mm512_fmadd_ps(a4, b, c);
        a5 = _mm512_fmadd_ps(a5, b, c);
        a6 = _mm512_fmadd_ps(a6, b, c);
        a7 = _mm512_fmadd_ps(a7, b, c);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(a0) + _mm512_reduce_add_ps(a1) +
                          _mm512_reduce_add_ps(a2) + _mm512_reduce_add_ps(a3) +
                          _mm512_reduce_add_ps(a4) + _mm512_reduce_add_ps(a5) +
                          _mm512_reduce_add_ps(a6) + _mm512_reduce_add_ps(a7);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

static double avx256_fma_throughput(uint64_t iterations)
{
    __m256 a0 = _mm256_set1_ps(1.0f);
    __m256 a1 = _mm256_set1_ps(1.0001f);
    __m256 a2 = _mm256_set1_ps(1.0002f);
    __m256 a3 = _mm256_set1_ps(1.0003f);
    __m256 a4 = _mm256_set1_ps(1.0004f);
    __m256 a5 = _mm256_set1_ps(1.0005f);
    __m256 a6 = _mm256_set1_ps(1.0006f);
    __m256 a7 = _mm256_set1_ps(1.0007f);
    __m256 b  = _mm256_set1_ps(1.0000001f);
    __m256 c  = _mm256_set1_ps(0.0000001f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        a0 = _mm256_fmadd_ps(a0, b, c);
        a1 = _mm256_fmadd_ps(a1, b, c);
        a2 = _mm256_fmadd_ps(a2, b, c);
        a3 = _mm256_fmadd_ps(a3, b, c);
        a4 = _mm256_fmadd_ps(a4, b, c);
        a5 = _mm256_fmadd_ps(a5, b, c);
        a6 = _mm256_fmadd_ps(a6, b, c);
        a7 = _mm256_fmadd_ps(a7, b, c);
        __asm__ volatile("" : "+x"(a0), "+x"(a1), "+x"(a2), "+x"(a3),
                              "+x"(a4), "+x"(a5), "+x"(a6), "+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    __m256 sum = _mm256_add_ps(_mm256_add_ps(a0, a1), _mm256_add_ps(a2, a3));
    sum = _mm256_add_ps(sum, _mm256_add_ps(_mm256_add_ps(a4, a5), _mm256_add_ps(a6, a7)));
    volatile float sink;
    _mm_store_ss((float*)&sink, _mm256_castps256_ps128(sum));
    return (double)(t1 - t0) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_fma\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double lat = avx512_fma_latency(cfg.iterations);
    double tp512 = avx512_fma_throughput(cfg.iterations);
    double tp256 = avx256_fma_throughput(cfg.iterations);

    printf("avx512_fma_latency_cycles=%.4f\n", lat);
    printf("avx512_fma_throughput_cycles=%.4f\n", tp512);
    printf("avx256_fma_throughput_cycles=%.4f\n", tp256);
    printf("zmm_vs_ymm_ratio=%.3f\n", tp512 / tp256);

    if (cfg.csv)
        printf("csv,probe=avx512_fma,lat=%.4f,tp512=%.4f,tp256=%.4f,ratio=%.3f\n",
               lat, tp512, tp256, tp512 / tp256);
    return 0;
}
