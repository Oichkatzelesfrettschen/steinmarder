#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_popcount_lzcnt.c — Bit manipulation throughput.
 *
 * Translated from SASS probe_clz_tile, probe_popc_tile, and tests
 * AVX-512 BITALG/VPOPCNTDQ extensions.
 *
 * Measures throughput of:
 *   vpopcntd zmm   — per-element 32-bit popcount (AVX512-VPOPCNTDQ)
 *   vpopcntq zmm   — per-element 64-bit popcount (AVX512-VPOPCNTDQ)
 *   vplzcntd zmm   — per-element 32-bit leading zero count (AVX512-CD)
 *   vplzcntq zmm   — per-element 64-bit leading zero count (AVX512-CD)
 *   vpternlogd zmm  — ternary logic (x86 equiv of SASS LOP3)
 *   vpshufb zmm    — byte shuffle (used in software popcount fallback)
 *
 * Critical for hash computations, Bloom filters, and compact operations.
 * All use 8 independent accumulators for throughput, plus 4-deep chains
 * for latency measurement.
 */

/* ---- Throughput benchmarks (8-wide independent streams) ---- */

static double throughput_vpopcntd(uint64_t iterations)
{
    __m512i acc_0 = _mm512_set1_epi32(0xDEADBEEF);
    __m512i acc_1 = _mm512_set1_epi32(0xCAFEBABE);
    __m512i acc_2 = _mm512_set1_epi32(0x12345678);
    __m512i acc_3 = _mm512_set1_epi32(0x9ABCDEF0);
    __m512i acc_4 = _mm512_set1_epi32(0xFFFF0000);
    __m512i acc_5 = _mm512_set1_epi32(0x0000FFFF);
    __m512i acc_6 = _mm512_set1_epi32(0xAAAAAAAA);
    __m512i acc_7 = _mm512_set1_epi32(0x55555555);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc_0 = _mm512_popcnt_epi32(acc_0);
        acc_1 = _mm512_popcnt_epi32(acc_1);
        acc_2 = _mm512_popcnt_epi32(acc_2);
        acc_3 = _mm512_popcnt_epi32(acc_3);
        acc_4 = _mm512_popcnt_epi32(acc_4);
        acc_5 = _mm512_popcnt_epi32(acc_5);
        acc_6 = _mm512_popcnt_epi32(acc_6);
        acc_7 = _mm512_popcnt_epi32(acc_7);
        __asm__ volatile("" : "+x"(acc_0), "+x"(acc_1), "+x"(acc_2), "+x"(acc_3),
                              "+x"(acc_4), "+x"(acc_5), "+x"(acc_6), "+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc_0) + _mm512_reduce_add_epi32(acc_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_vpopcntq(uint64_t iterations)
{
    __m512i acc_0 = _mm512_set1_epi64(0xDEADBEEFCAFEBABEULL);
    __m512i acc_1 = _mm512_set1_epi64(0x123456789ABCDEF0ULL);
    __m512i acc_2 = _mm512_set1_epi64(0xFFFF0000FFFF0000ULL);
    __m512i acc_3 = _mm512_set1_epi64(0xAAAAAAAA55555555ULL);
    __m512i acc_4 = _mm512_set1_epi64(0x0F0F0F0FF0F0F0F0ULL);
    __m512i acc_5 = _mm512_set1_epi64(0x3333CCCC3333CCCCULL);
    __m512i acc_6 = _mm512_set1_epi64(0x1111111122222222ULL);
    __m512i acc_7 = _mm512_set1_epi64(0x8888888877777777ULL);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc_0 = _mm512_popcnt_epi64(acc_0);
        acc_1 = _mm512_popcnt_epi64(acc_1);
        acc_2 = _mm512_popcnt_epi64(acc_2);
        acc_3 = _mm512_popcnt_epi64(acc_3);
        acc_4 = _mm512_popcnt_epi64(acc_4);
        acc_5 = _mm512_popcnt_epi64(acc_5);
        acc_6 = _mm512_popcnt_epi64(acc_6);
        acc_7 = _mm512_popcnt_epi64(acc_7);
        __asm__ volatile("" : "+x"(acc_0), "+x"(acc_1), "+x"(acc_2), "+x"(acc_3),
                              "+x"(acc_4), "+x"(acc_5), "+x"(acc_6), "+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc_0) + _mm512_reduce_add_epi64(acc_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_vplzcntd(uint64_t iterations)
{
    __m512i acc_0 = _mm512_set1_epi32(0x00001000);
    __m512i acc_1 = _mm512_set1_epi32(0x00010000);
    __m512i acc_2 = _mm512_set1_epi32(0x00100000);
    __m512i acc_3 = _mm512_set1_epi32(0x01000000);
    __m512i acc_4 = _mm512_set1_epi32(0x10000000);
    __m512i acc_5 = _mm512_set1_epi32(0x00000100);
    __m512i acc_6 = _mm512_set1_epi32(0x00000010);
    __m512i acc_7 = _mm512_set1_epi32(0x80000000);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc_0 = _mm512_lzcnt_epi32(acc_0);
        acc_1 = _mm512_lzcnt_epi32(acc_1);
        acc_2 = _mm512_lzcnt_epi32(acc_2);
        acc_3 = _mm512_lzcnt_epi32(acc_3);
        acc_4 = _mm512_lzcnt_epi32(acc_4);
        acc_5 = _mm512_lzcnt_epi32(acc_5);
        acc_6 = _mm512_lzcnt_epi32(acc_6);
        acc_7 = _mm512_lzcnt_epi32(acc_7);
        __asm__ volatile("" : "+x"(acc_0), "+x"(acc_1), "+x"(acc_2), "+x"(acc_3),
                              "+x"(acc_4), "+x"(acc_5), "+x"(acc_6), "+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc_0) + _mm512_reduce_add_epi32(acc_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_vplzcntq(uint64_t iterations)
{
    __m512i acc_0 = _mm512_set1_epi64(0x0000100000000000ULL);
    __m512i acc_1 = _mm512_set1_epi64(0x0001000000000000ULL);
    __m512i acc_2 = _mm512_set1_epi64(0x0010000000000000ULL);
    __m512i acc_3 = _mm512_set1_epi64(0x0100000000000000ULL);
    __m512i acc_4 = _mm512_set1_epi64(0x1000000000000000ULL);
    __m512i acc_5 = _mm512_set1_epi64(0x0000010000000000ULL);
    __m512i acc_6 = _mm512_set1_epi64(0x0000001000000000ULL);
    __m512i acc_7 = _mm512_set1_epi64(0x8000000000000000ULL);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc_0 = _mm512_lzcnt_epi64(acc_0);
        acc_1 = _mm512_lzcnt_epi64(acc_1);
        acc_2 = _mm512_lzcnt_epi64(acc_2);
        acc_3 = _mm512_lzcnt_epi64(acc_3);
        acc_4 = _mm512_lzcnt_epi64(acc_4);
        acc_5 = _mm512_lzcnt_epi64(acc_5);
        acc_6 = _mm512_lzcnt_epi64(acc_6);
        acc_7 = _mm512_lzcnt_epi64(acc_7);
        __asm__ volatile("" : "+x"(acc_0), "+x"(acc_1), "+x"(acc_2), "+x"(acc_3),
                              "+x"(acc_4), "+x"(acc_5), "+x"(acc_6), "+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc_0) + _mm512_reduce_add_epi64(acc_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

static double throughput_vpternlogd(uint64_t iterations)
{
    __m512i acc_0 = _mm512_set1_epi32(0xDEADBEEF);
    __m512i acc_1 = _mm512_set1_epi32(0xCAFEBABE);
    __m512i acc_2 = _mm512_set1_epi32(0x12345678);
    __m512i acc_3 = _mm512_set1_epi32(0x9ABCDEF0);
    __m512i acc_4 = _mm512_set1_epi32(0xFFFF0000);
    __m512i acc_5 = _mm512_set1_epi32(0x0000FFFF);
    __m512i acc_6 = _mm512_set1_epi32(0xAAAAAAAA);
    __m512i acc_7 = _mm512_set1_epi32(0x55555555);
    __m512i operand_b = _mm512_set1_epi32(0x0F0F0F0F);
    __m512i operand_c = _mm512_set1_epi32(0xF0F0F0F0);
    __asm__ volatile("" : "+x"(operand_b), "+x"(operand_c));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        /* imm8 = 0x96 = a XOR b XOR c (majority-like function) */
        acc_0 = _mm512_ternarylogic_epi32(acc_0, operand_b, operand_c, 0x96);
        acc_1 = _mm512_ternarylogic_epi32(acc_1, operand_b, operand_c, 0x96);
        acc_2 = _mm512_ternarylogic_epi32(acc_2, operand_b, operand_c, 0x96);
        acc_3 = _mm512_ternarylogic_epi32(acc_3, operand_b, operand_c, 0x96);
        acc_4 = _mm512_ternarylogic_epi32(acc_4, operand_b, operand_c, 0x96);
        acc_5 = _mm512_ternarylogic_epi32(acc_5, operand_b, operand_c, 0x96);
        acc_6 = _mm512_ternarylogic_epi32(acc_6, operand_b, operand_c, 0x96);
        acc_7 = _mm512_ternarylogic_epi32(acc_7, operand_b, operand_c, 0x96);
        __asm__ volatile("" : "+x"(acc_0), "+x"(acc_1), "+x"(acc_2), "+x"(acc_3),
                              "+x"(acc_4), "+x"(acc_5), "+x"(acc_6), "+x"(acc_7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc_0) + _mm512_reduce_add_epi32(acc_4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ---- Latency benchmarks (4-deep dependent chain) ---- */

static double latency_vpopcntd(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi32(0xDEADBEEF);
    __asm__ volatile("" : "+x"(acc));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_popcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_popcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_popcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_popcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double latency_vplzcntd(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi32(0x00001000);
    __asm__ volatile("" : "+x"(acc));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_lzcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_lzcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_lzcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_lzcnt_epi32(acc);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double latency_vpternlogd(uint64_t iterations)
{
    __m512i acc = _mm512_set1_epi32(0xDEADBEEF);
    __m512i operand_b = _mm512_set1_epi32(0x0F0F0F0F);
    __m512i operand_c = _mm512_set1_epi32(0xF0F0F0F0);
    __asm__ volatile("" : "+x"(operand_b), "+x"(operand_c));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iter = 0; iter < iterations; iter++) {
        acc = _mm512_ternarylogic_epi32(acc, operand_b, operand_c, 0x96);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_ternarylogic_epi32(acc, operand_b, operand_c, 0x96);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_ternarylogic_epi32(acc, operand_b, operand_c, 0x96);
        __asm__ volatile("" : "+x"(acc));
        acc = _mm512_ternarylogic_epi32(acc, operand_b, operand_c, 0x96);
        __asm__ volatile("" : "+x"(acc));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    sm_zen_print_header("popcount_lzcnt", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    /* Throughput */
    double popcntd_tp    = throughput_vpopcntd(cfg.iterations);
    double popcntq_tp    = throughput_vpopcntq(cfg.iterations);
    double lzcntd_tp     = throughput_vplzcntd(cfg.iterations);
    double lzcntq_tp     = throughput_vplzcntq(cfg.iterations);
    double ternlogd_tp   = throughput_vpternlogd(cfg.iterations);

    /* Latency */
    double popcntd_lat   = latency_vpopcntd(cfg.iterations);
    double lzcntd_lat    = latency_vplzcntd(cfg.iterations);
    double ternlogd_lat  = latency_vpternlogd(cfg.iterations);

    printf("--- Throughput (cycles per op, 8 independent streams) ---\n");
    printf("vpopcntd_zmm_tp=%.4f\n", popcntd_tp);
    printf("vpopcntq_zmm_tp=%.4f\n", popcntq_tp);
    printf("vplzcntd_zmm_tp=%.4f\n", lzcntd_tp);
    printf("vplzcntq_zmm_tp=%.4f\n", lzcntq_tp);
    printf("vpternlogd_zmm_tp=%.4f\n", ternlogd_tp);

    printf("\n--- Latency (cycles per dependent op, 4-deep chain) ---\n");
    printf("vpopcntd_zmm_lat=%.4f\n", popcntd_lat);
    printf("vplzcntd_zmm_lat=%.4f\n", lzcntd_lat);
    printf("vpternlogd_zmm_lat=%.4f\n", ternlogd_lat);

    printf("\n--- Summary ---\n");
    printf("vpopcntd zmm:   tp=%.2f cy  lat=%.2f cy  (32-bit per-element popcount)\n", popcntd_tp, popcntd_lat);
    printf("vpopcntq zmm:   tp=%.2f cy              (64-bit per-element popcount)\n", popcntq_tp);
    printf("vplzcntd zmm:   tp=%.2f cy  lat=%.2f cy  (32-bit leading zero count)\n", lzcntd_tp, lzcntd_lat);
    printf("vplzcntq zmm:   tp=%.2f cy              (64-bit leading zero count)\n", lzcntq_tp);
    printf("vpternlogd zmm: tp=%.2f cy  lat=%.2f cy  (ternary logic, equiv to SASS LOP3)\n", ternlogd_tp, ternlogd_lat);
    printf("\nvpopcntd processes 16x 32-bit popcounts per instruction.\n");
    printf("vpternlogd implements any 3-input boolean function in 1 uop.\n");

    if (cfg.csv)
        printf("csv,probe=popcount_lzcnt,popcntd_tp=%.4f,popcntq_tp=%.4f,lzcntd_tp=%.4f,lzcntq_tp=%.4f,ternlogd_tp=%.4f,popcntd_lat=%.4f,lzcntd_lat=%.4f,ternlogd_lat=%.4f\n",
               popcntd_tp, popcntq_tp, lzcntd_tp, lzcntq_tp, ternlogd_tp,
               popcntd_lat, lzcntd_lat, ternlogd_lat);

    return 0;
}
