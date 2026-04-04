#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_vnni.c — AVX-512 VNNI (Vector Neural Network Instructions) on Zen 4.
 *
 * Measures vpdpbusd (uint8 × int8 dot product accumulate) and
 * vpdpwssd (int16 × int16 dot product accumulate) throughput/latency.
 * These are the key integer quantized inference instructions.
 */

static double vnni_busd_latency(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi32(1);
    __m512i a   = _mm512_set1_epi32(0x01010101);
    __m512i b   = _mm512_set1_epi32(0x01010101);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc = _mm512_dpbusd_epi32(acc, a, b);
        acc = _mm512_dpbusd_epi32(acc, a, b);
        acc = _mm512_dpbusd_epi32(acc, a, b);
        acc = _mm512_dpbusd_epi32(acc, a, b);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 4);
}

static double vnni_busd_throughput(uint64_t iterations)
{
    __m512i a0 = _mm512_set1_epi32(0x01020304);
    __m512i a1 = _mm512_set1_epi32(0x05060708);
    __m512i a2 = _mm512_set1_epi32(0x090A0B0C);
    __m512i a3 = _mm512_set1_epi32(0x0D0E0F10);
    __m512i a4 = _mm512_set1_epi32(0x11121314);
    __m512i a5 = _mm512_set1_epi32(0x15161718);
    __m512i a6 = _mm512_set1_epi32(0x191A1B1C);
    __m512i a7 = _mm512_set1_epi32(0x1D1E1F20);
    __m512i b  = _mm512_set1_epi32(0x01010101);
    __m512i c  = _mm512_setzero_si512();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        a0 = _mm512_dpbusd_epi32(a0, b, c);
        a1 = _mm512_dpbusd_epi32(a1, b, c);
        a2 = _mm512_dpbusd_epi32(a2, b, c);
        a3 = _mm512_dpbusd_epi32(a3, b, c);
        a4 = _mm512_dpbusd_epi32(a4, b, c);
        a5 = _mm512_dpbusd_epi32(a5, b, c);
        a6 = _mm512_dpbusd_epi32(a6, b, c);
        a7 = _mm512_dpbusd_epi32(a7, b, c);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(a0) + _mm512_reduce_add_epi32(a4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

static double vnni_wssd_throughput(uint64_t iterations)
{
    __m512i a0 = _mm512_set1_epi32(1);
    __m512i a1 = _mm512_set1_epi32(2);
    __m512i a2 = _mm512_set1_epi32(3);
    __m512i a3 = _mm512_set1_epi32(4);
    __m512i a4 = _mm512_set1_epi32(5);
    __m512i a5 = _mm512_set1_epi32(6);
    __m512i a6 = _mm512_set1_epi32(7);
    __m512i a7 = _mm512_set1_epi32(8);
    __m512i b  = _mm512_set1_epi32(0x00010001); /* two int16 = 1 */
    __m512i c  = _mm512_setzero_si512();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        a0 = _mm512_dpwssd_epi32(a0, b, c);
        a1 = _mm512_dpwssd_epi32(a1, b, c);
        a2 = _mm512_dpwssd_epi32(a2, b, c);
        a3 = _mm512_dpwssd_epi32(a3, b, c);
        a4 = _mm512_dpwssd_epi32(a4, b, c);
        a5 = _mm512_dpwssd_epi32(a5, b, c);
        a6 = _mm512_dpwssd_epi32(a6, b, c);
        a7 = _mm512_dpwssd_epi32(a7, b, c);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(a0) + _mm512_reduce_add_epi32(a4);
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

    printf("probe=avx512_vnni\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double busd_lat = vnni_busd_latency(cfg.iterations);
    double busd_tp  = vnni_busd_throughput(cfg.iterations);
    double wssd_tp  = vnni_wssd_throughput(cfg.iterations);

    printf("vpdpbusd_latency_cycles=%.4f\n", busd_lat);
    printf("vpdpbusd_throughput_cycles=%.4f\n", busd_tp);
    printf("vpdpwssd_throughput_cycles=%.4f\n", wssd_tp);
    printf("busd_int8_ops_per_cycle=%.1f\n", 64.0 / busd_tp);
    printf("wssd_int16_ops_per_cycle=%.1f\n", 32.0 / wssd_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_vnni,busd_lat=%.4f,busd_tp=%.4f,wssd_tp=%.4f\n",
               busd_lat, busd_tp, wssd_tp);
    return 0;
}
