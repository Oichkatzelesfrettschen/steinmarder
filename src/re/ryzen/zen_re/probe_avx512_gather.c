#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_gather.c — 512-bit gather throughput/latency on Zen 4.
 *
 * Measures vpgatherdd zmm (16 elements) vs vpgatherdd ymm (8 elements)
 * to quantify the Zen 4 split penalty for 512-bit gathers.
 */

static double gather_zmm(const int *table, size_t table_elems,
                         uint64_t iterations)
{
    __m512i idx = _mm512_set_epi32(
        0, 17, 34, 51, 68, 85, 102, 119,
        136, 153, 170, 187, 204, 221, 238, 255);
    /* Keep indices within table bounds */
    __m512i mask_val = _mm512_set1_epi32((int)(table_elems - 1));
    idx = _mm512_and_epi32(idx, mask_val);

    __m512i acc = _mm512_setzero_si512();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        __m512i gathered = _mm512_i32gather_epi32(idx, table, 4);
        acc = _mm512_add_epi32(acc, gathered);
        /* Rotate indices to touch different cache lines */
        idx = _mm512_add_epi32(idx, _mm512_set1_epi32(16));
        idx = _mm512_and_epi32(idx, mask_val);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile int sink = _mm512_reduce_add_epi32(acc);
    (void)sink;
    return (double)(t1 - t0) / (double)iterations;
}

static double gather_ymm(const int *table, size_t table_elems,
                         uint64_t iterations)
{
    __m256i idx = _mm256_set_epi32(0, 17, 34, 51, 68, 85, 102, 119);
    __m256i mask_val = _mm256_set1_epi32((int)(table_elems - 1));
    idx = _mm256_and_si256(idx, mask_val);

    __m256i acc = _mm256_setzero_si256();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        __m256i gathered = _mm256_i32gather_epi32(table, idx, 4);
        acc = _mm256_add_epi32(acc, gathered);
        idx = _mm256_add_epi32(idx, _mm256_set1_epi32(8));
        idx = _mm256_and_si256(idx, mask_val);
    }
    uint64_t t1 = sm_zen_tsc_end();

    int tmp[8];
    _mm256_storeu_si256((__m256i *)tmp, acc);
    volatile int sink = tmp[0] + tmp[7];
    (void)sink;
    return (double)(t1 - t0) / (double)iterations;
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    cfg.working_set_bytes = 1048576;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_gather\ncpu=%d\navx512f=%d\n", cfg.cpu, feat.has_avx512f);
    printf("working_set_bytes=%zu\n\n", cfg.working_set_bytes);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    size_t table_elems = cfg.working_set_bytes / sizeof(int);
    int *table = (int *)sm_zen_aligned_alloc64(cfg.working_set_bytes);
    sm_zen_fill_random_u32((uint32_t *)table, table_elems, 42);

    double cyc_zmm = gather_zmm(table, table_elems, cfg.iterations);
    double cyc_ymm = gather_ymm(table, table_elems, cfg.iterations);

    printf("gather_zmm_16elem_cycles=%.4f\n", cyc_zmm);
    printf("gather_ymm_8elem_cycles=%.4f\n", cyc_ymm);
    printf("zmm_per_element=%.4f\n", cyc_zmm / 16.0);
    printf("ymm_per_element=%.4f\n", cyc_ymm / 8.0);
    printf("zmm_vs_ymm_ratio=%.3f\n", cyc_zmm / cyc_ymm);

    if (cfg.csv)
        printf("csv,probe=avx512_gather,zmm=%.4f,ymm=%.4f,ratio=%.3f\n",
               cyc_zmm, cyc_ymm, cyc_zmm / cyc_ymm);

    free(table);
    return 0;
}
