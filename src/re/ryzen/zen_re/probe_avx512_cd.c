#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_cd.c -- AVX-512CD (Conflict Detection) probe on Zen 4.
 *
 * Measures vpconflictd/q (conflict detection for safe vectorized scatter/
 * histogram) and vplzcntd/q (per-element leading zero count).
 *
 * vpconflictd is THE signature CD instruction -- for each lane, it produces
 * a bitmask of earlier lanes with the same value.  Essential for vectorizing
 * histogram updates and scatter-with-conflicts.
 */

/* --- vpconflictd zmm latency (dependent chain) --- */
static double vpconflictd_latency(uint64_t iterations)
{
    /* Feed output back as input -- dependency chain */
    __m512i accumulator = _mm512_set_epi32(
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_conflict_epi32(accumulator);
        accumulator = _mm512_conflict_epi32(accumulator);
        accumulator = _mm512_conflict_epi32(accumulator);
        accumulator = _mm512_conflict_epi32(accumulator);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

/* --- vpconflictd zmm throughput, all-different input --- */
static double vpconflictd_throughput_alldiff(uint64_t iterations)
{
    __m512i acc0 = _mm512_set_epi32( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15);
    __m512i acc1 = _mm512_set_epi32(16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31);
    __m512i acc2 = _mm512_set_epi32(32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47);
    __m512i acc3 = _mm512_set_epi32(48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63);
    __m512i acc4 = _mm512_set_epi32(64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79);
    __m512i acc5 = _mm512_set_epi32(80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95);
    __m512i acc6 = _mm512_set_epi32(96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111);
    __m512i acc7 = _mm512_set_epi32(112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_conflict_epi32(acc0);
        acc1 = _mm512_conflict_epi32(acc1);
        acc2 = _mm512_conflict_epi32(acc2);
        acc3 = _mm512_conflict_epi32(acc3);
        acc4 = _mm512_conflict_epi32(acc4);
        acc5 = _mm512_conflict_epi32(acc5);
        acc6 = _mm512_conflict_epi32(acc6);
        acc7 = _mm512_conflict_epi32(acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpconflictd zmm throughput, all-same input (worst case conflicts) --- */
static double vpconflictd_throughput_allsame(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi32(42);
    __m512i acc1 = _mm512_set1_epi32(43);
    __m512i acc2 = _mm512_set1_epi32(44);
    __m512i acc3 = _mm512_set1_epi32(45);
    __m512i acc4 = _mm512_set1_epi32(46);
    __m512i acc5 = _mm512_set1_epi32(47);
    __m512i acc6 = _mm512_set1_epi32(48);
    __m512i acc7 = _mm512_set1_epi32(49);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_conflict_epi32(acc0);
        acc1 = _mm512_conflict_epi32(acc1);
        acc2 = _mm512_conflict_epi32(acc2);
        acc3 = _mm512_conflict_epi32(acc3);
        acc4 = _mm512_conflict_epi32(acc4);
        acc5 = _mm512_conflict_epi32(acc5);
        acc6 = _mm512_conflict_epi32(acc6);
        acc7 = _mm512_conflict_epi32(acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vpconflictq zmm throughput (64-bit, 8 lanes) --- */
static double vpconflictq_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set_epi64(0,1,2,3,4,5,6,7);
    __m512i acc1 = _mm512_set_epi64(8,9,10,11,12,13,14,15);
    __m512i acc2 = _mm512_set_epi64(16,17,18,19,20,21,22,23);
    __m512i acc3 = _mm512_set_epi64(24,25,26,27,28,29,30,31);
    __m512i acc4 = _mm512_set_epi64(32,33,34,35,36,37,38,39);
    __m512i acc5 = _mm512_set_epi64(40,41,42,43,44,45,46,47);
    __m512i acc6 = _mm512_set_epi64(48,49,50,51,52,53,54,55);
    __m512i acc7 = _mm512_set_epi64(56,57,58,59,60,61,62,63);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_conflict_epi64(acc0);
        acc1 = _mm512_conflict_epi64(acc1);
        acc2 = _mm512_conflict_epi64(acc2);
        acc3 = _mm512_conflict_epi64(acc3);
        acc4 = _mm512_conflict_epi64(acc4);
        acc5 = _mm512_conflict_epi64(acc5);
        acc6 = _mm512_conflict_epi64(acc6);
        acc7 = _mm512_conflict_epi64(acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vplzcntd zmm throughput (32-bit per-element leading zero count) --- */
static double vplzcntd_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi32(0x00010000);
    __m512i acc1 = _mm512_set1_epi32(0x00020000);
    __m512i acc2 = _mm512_set1_epi32(0x00040000);
    __m512i acc3 = _mm512_set1_epi32(0x00080000);
    __m512i acc4 = _mm512_set1_epi32(0x00100000);
    __m512i acc5 = _mm512_set1_epi32(0x00200000);
    __m512i acc6 = _mm512_set1_epi32(0x00400000);
    __m512i acc7 = _mm512_set1_epi32(0x00800000);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_lzcnt_epi32(acc0);
        acc1 = _mm512_lzcnt_epi32(acc1);
        acc2 = _mm512_lzcnt_epi32(acc2);
        acc3 = _mm512_lzcnt_epi32(acc3);
        acc4 = _mm512_lzcnt_epi32(acc4);
        acc5 = _mm512_lzcnt_epi32(acc5);
        acc6 = _mm512_lzcnt_epi32(acc6);
        acc7 = _mm512_lzcnt_epi32(acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc0) + _mm512_reduce_add_epi32(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* --- vplzcntq zmm throughput (64-bit per-element leading zero count) --- */
static double vplzcntq_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(0x0000000100000000ULL);
    __m512i acc1 = _mm512_set1_epi64(0x0000000200000000ULL);
    __m512i acc2 = _mm512_set1_epi64(0x0000000400000000ULL);
    __m512i acc3 = _mm512_set1_epi64(0x0000000800000000ULL);
    __m512i acc4 = _mm512_set1_epi64(0x0000001000000000ULL);
    __m512i acc5 = _mm512_set1_epi64(0x0000002000000000ULL);
    __m512i acc6 = _mm512_set1_epi64(0x0000004000000000ULL);
    __m512i acc7 = _mm512_set1_epi64(0x0000008000000000ULL);
    __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                          "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_lzcnt_epi64(acc0);
        acc1 = _mm512_lzcnt_epi64(acc1);
        acc2 = _mm512_lzcnt_epi64(acc2);
        acc3 = _mm512_lzcnt_epi64(acc3);
        acc4 = _mm512_lzcnt_epi64(acc4);
        acc5 = _mm512_lzcnt_epi64(acc5);
        acc6 = _mm512_lzcnt_epi64(acc6);
        acc7 = _mm512_lzcnt_epi64(acc7);
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

    printf("probe=avx512_cd\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double conflict_d_lat          = vpconflictd_latency(cfg.iterations);
    double conflict_d_tp_alldiff   = vpconflictd_throughput_alldiff(cfg.iterations);
    double conflict_d_tp_allsame   = vpconflictd_throughput_allsame(cfg.iterations);
    double conflict_q_tp           = vpconflictq_throughput(cfg.iterations);
    double lzcnt_d_tp              = vplzcntd_throughput(cfg.iterations);
    double lzcnt_q_tp              = vplzcntq_throughput(cfg.iterations);

    printf("=== AVX-512CD: vpconflictd (32-bit conflict detection) ===\n");
    printf("vpconflictd_latency_cycles=%.4f\n", conflict_d_lat);
    printf("vpconflictd_throughput_alldiff_cycles=%.4f\n", conflict_d_tp_alldiff);
    printf("vpconflictd_throughput_allsame_cycles=%.4f\n", conflict_d_tp_allsame);
    printf("vpconflictd_data_dependent_slowdown=%.3fx\n",
           conflict_d_tp_allsame / conflict_d_tp_alldiff);
    printf("\n");

    printf("=== AVX-512CD: vpconflictq (64-bit conflict detection) ===\n");
    printf("vpconflictq_throughput_cycles=%.4f\n", conflict_q_tp);
    printf("\n");

    printf("=== AVX-512CD: vplzcntd/q (per-element leading zero count) ===\n");
    printf("vplzcntd_throughput_cycles=%.4f\n", lzcnt_d_tp);
    printf("vplzcntq_throughput_cycles=%.4f\n", lzcnt_q_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_cd,conflict_d_lat=%.4f,conflict_d_tp_alldiff=%.4f,"
               "conflict_d_tp_allsame=%.4f,conflict_q_tp=%.4f,"
               "lzcnt_d_tp=%.4f,lzcnt_q_tp=%.4f\n",
               conflict_d_lat, conflict_d_tp_alldiff, conflict_d_tp_allsame,
               conflict_q_tp, lzcnt_d_tp, lzcnt_q_tp);
    return 0;
}
