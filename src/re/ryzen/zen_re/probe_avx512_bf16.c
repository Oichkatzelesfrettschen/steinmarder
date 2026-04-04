#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_bf16.c — AVX-512 BF16 dot product on Zen 4.
 *
 * Measures vdpbf16ps (BF16 pair dot product accumulate into FP32).
 * Each vdpbf16ps zmm does 32 BF16 multiplies + 16 FP32 adds per instruction.
 * Key for mixed-precision NeRF MLP inference (weights in BF16, accum in FP32).
 */

static double bf16_dp_latency(uint64_t iterations)
{
    __m512 acc = _mm512_set1_ps(1.0f);
    __m512bh a = (__m512bh)_mm512_set1_epi16(0x3F80); /* bf16 1.0 */
    __m512bh b = (__m512bh)_mm512_set1_epi16(0x3F80);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc = _mm512_dpbf16_ps(acc, a, b);
        acc = _mm512_dpbf16_ps(acc, a, b);
        acc = _mm512_dpbf16_ps(acc, a, b);
        acc = _mm512_dpbf16_ps(acc, a, b);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 4);
}

static double bf16_dp_throughput(uint64_t iterations)
{
    __m512 a0 = _mm512_set1_ps(1.0f);
    __m512 a1 = _mm512_set1_ps(1.0001f);
    __m512 a2 = _mm512_set1_ps(1.0002f);
    __m512 a3 = _mm512_set1_ps(1.0003f);
    __m512 a4 = _mm512_set1_ps(1.0004f);
    __m512 a5 = _mm512_set1_ps(1.0005f);
    __m512 a6 = _mm512_set1_ps(1.0006f);
    __m512 a7 = _mm512_set1_ps(1.0007f);
    __m512bh src_a = (__m512bh)_mm512_set1_epi16(0x3F80);
    __m512bh src_b = (__m512bh)_mm512_set1_epi16(0x3C00); /* bf16 ~0.0117 */

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        a0 = _mm512_dpbf16_ps(a0, src_a, src_b);
        a1 = _mm512_dpbf16_ps(a1, src_a, src_b);
        a2 = _mm512_dpbf16_ps(a2, src_a, src_b);
        a3 = _mm512_dpbf16_ps(a3, src_a, src_b);
        a4 = _mm512_dpbf16_ps(a4, src_a, src_b);
        a5 = _mm512_dpbf16_ps(a5, src_a, src_b);
        a6 = _mm512_dpbf16_ps(a6, src_a, src_b);
        a7 = _mm512_dpbf16_ps(a7, src_a, src_b);
        __asm__ volatile("" : "+x"(a0), "+x"(a1), "+x"(a2), "+x"(a3),
                              "+x"(a4), "+x"(a5), "+x"(a6), "+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(a0) + _mm512_reduce_add_ps(a4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* Compare against FP32 FMA throughput for the same accumulator pattern */
static double fp32_fma_throughput(uint64_t iterations)
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
        __asm__ volatile("" : "+x"(a0), "+x"(a1), "+x"(a2), "+x"(a3),
                              "+x"(a4), "+x"(a5), "+x"(a6), "+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(a0) + _mm512_reduce_add_ps(a4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 100000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_bf16\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    double lat = bf16_dp_latency(cfg.iterations);
    double bf16_tp = bf16_dp_throughput(cfg.iterations);
    double fp32_tp = fp32_fma_throughput(cfg.iterations);

    printf("vdpbf16ps_latency_cycles=%.4f\n", lat);
    printf("vdpbf16ps_throughput_cycles=%.4f\n", bf16_tp);
    printf("vfmadd_fp32_throughput_cycles=%.4f\n", fp32_tp);
    printf("bf16_bf16_mul_ops_per_instr=32\n");
    printf("bf16_vs_fp32_throughput_ratio=%.3f\n", bf16_tp / fp32_tp);
    printf("bf16_mul_ops_per_cycle=%.1f\n", 32.0 / bf16_tp);
    printf("fp32_fma_ops_per_cycle=%.1f\n", 16.0 / fp32_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_bf16,lat=%.4f,bf16_tp=%.4f,fp32_tp=%.4f,ratio=%.3f\n",
               lat, bf16_tp, fp32_tp, bf16_tp / fp32_tp);
    return 0;
}
