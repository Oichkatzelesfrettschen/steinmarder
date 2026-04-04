#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_avx512_port_contention.c — FMA + gather interleaved scheduling.
 *
 * Zen 4 has 2 FMA pipes and a separate AGU/gather unit. This probe
 * measures whether interleaving FMA and gather instructions achieves
 * overlap or causes port contention.
 *
 * Scenarios:
 *   1. FMA-only burst
 *   2. Gather-only burst
 *   3. Interleaved FMA + gather (should be faster than sum if overlap works)
 */

static double fma_only(uint64_t iterations)
{
    __m512 a0 = _mm512_set1_ps(1.0f);
    __m512 a1 = _mm512_set1_ps(1.0001f);
    __m512 a2 = _mm512_set1_ps(1.0002f);
    __m512 a3 = _mm512_set1_ps(1.0003f);
    __m512 b  = _mm512_set1_ps(1.0000001f);
    __m512 c  = _mm512_set1_ps(0.0000001f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        a0 = _mm512_fmadd_ps(a0, b, c);
        a1 = _mm512_fmadd_ps(a1, b, c);
        a2 = _mm512_fmadd_ps(a2, b, c);
        a3 = _mm512_fmadd_ps(a3, b, c);
        __asm__ volatile("" : "+x"(a0), "+x"(a1), "+x"(a2), "+x"(a3));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(a0);
    (void)sink;
    return (double)(t1 - t0) / (double)iterations;
}

static double gather_only(const int *table, size_t table_elems, uint64_t iterations)
{
    __m512i idx0 = _mm512_set_epi32(0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60);
    __m512i idx1 = _mm512_set_epi32(1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61);
    __m512i mask_val = _mm512_set1_epi32((int)(table_elems - 1));
    idx0 = _mm512_and_epi32(idx0, mask_val);
    idx1 = _mm512_and_epi32(idx1, mask_val);

    __m512i acc = _mm512_setzero_si512();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        __m512i g0 = _mm512_i32gather_epi32(idx0, table, 4);
        __m512i g1 = _mm512_i32gather_epi32(idx1, table, 4);
        acc = _mm512_add_epi32(acc, _mm512_add_epi32(g0, g1));
        idx0 = _mm512_add_epi32(idx0, _mm512_set1_epi32(64));
        idx0 = _mm512_and_epi32(idx0, mask_val);
        idx1 = _mm512_add_epi32(idx1, _mm512_set1_epi32(64));
        idx1 = _mm512_and_epi32(idx1, mask_val);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(t1 - t0) / (double)iterations;
}

static double fma_gather_interleaved(const int *table, size_t table_elems, uint64_t iterations)
{
    __m512 a0 = _mm512_set1_ps(1.0f);
    __m512 a1 = _mm512_set1_ps(1.0001f);
    __m512 b  = _mm512_set1_ps(1.0000001f);
    __m512 c  = _mm512_set1_ps(0.0000001f);

    __m512i idx0 = _mm512_set_epi32(0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60);
    __m512i idx1 = _mm512_set_epi32(1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61);
    __m512i mask_val = _mm512_set1_epi32((int)(table_elems - 1));
    idx0 = _mm512_and_epi32(idx0, mask_val);
    idx1 = _mm512_and_epi32(idx1, mask_val);

    __m512i gacc = _mm512_setzero_si512();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* Interleave: FMA, gather, FMA, gather */
        a0 = _mm512_fmadd_ps(a0, b, c);
        __m512i g0 = _mm512_i32gather_epi32(idx0, table, 4);
        a1 = _mm512_fmadd_ps(a1, b, c);
        __m512i g1 = _mm512_i32gather_epi32(idx1, table, 4);

        a0 = _mm512_fmadd_ps(a0, b, c);
        a1 = _mm512_fmadd_ps(a1, b, c);

        gacc = _mm512_add_epi32(gacc, _mm512_add_epi32(g0, g1));
        idx0 = _mm512_add_epi32(idx0, _mm512_set1_epi32(64));
        idx0 = _mm512_and_epi32(idx0, mask_val);
        idx1 = _mm512_add_epi32(idx1, _mm512_set1_epi32(64));
        idx1 = _mm512_and_epi32(idx1, mask_val);

        __asm__ volatile("" : "+x"(a0), "+x"(a1));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float fsink = _mm512_reduce_add_ps(a0);
    volatile int isink = _mm512_reduce_add_epi32(gacc);
    (void)fsink; (void)isink;
    return (double)(t1 - t0) / (double)iterations;
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    cfg.working_set_bytes = 262144; /* 256 KB — fits in L2 */
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_port_contention\ncpu=%d\n", cfg.cpu);
    printf("working_set_bytes=%zu\n\n", cfg.working_set_bytes);

    size_t table_elems = cfg.working_set_bytes / sizeof(int);
    int *table = (int *)sm_zen_aligned_alloc64(cfg.working_set_bytes);
    sm_zen_fill_random_u32((uint32_t *)table, table_elems, 42);

    double fma_cyc = fma_only(cfg.iterations);
    double gather_cyc = gather_only(table, table_elems, cfg.iterations);
    double interleaved_cyc = fma_gather_interleaved(table, table_elems, cfg.iterations);
    double theoretical_max = fma_cyc > gather_cyc ? fma_cyc : gather_cyc;
    double sum_serial = fma_cyc + gather_cyc;

    printf("fma_only_cycles=%.4f\n", fma_cyc);
    printf("gather_only_cycles=%.4f\n", gather_cyc);
    printf("interleaved_cycles=%.4f\n", interleaved_cyc);
    printf("theoretical_overlap=%.4f\n", theoretical_max);
    printf("serial_sum=%.4f\n", sum_serial);
    printf("overlap_efficiency=%.1f%%\n",
           100.0 * (1.0 - (interleaved_cyc - theoretical_max) /
                          (sum_serial - theoretical_max)));

    if (cfg.csv)
        printf("csv,probe=avx512_port_contention,fma=%.4f,gather=%.4f,interleaved=%.4f,overlap=%.1f%%\n",
               fma_cyc, gather_cyc, interleaved_cyc,
               100.0 * (1.0 - (interleaved_cyc - theoretical_max) /
                              (sum_serial - theoretical_max)));
    free(table);
    return 0;
}
