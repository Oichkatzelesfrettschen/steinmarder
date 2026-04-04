#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_avx512_bandwidth.c — Sustained load/store bandwidth at each cache level.
 *
 * Compares vmovaps zmm (512-bit) vs vmovaps ymm (256-bit) throughput
 * across L1, L2, L3, and DRAM working set sizes.
 */

static double bandwidth_zmm_load(const char *buf, size_t bytes, uint64_t iterations)
{
    size_t n_lines = bytes / 64;
    __m512 acc = _mm512_setzero_ps();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t it = 0; it < iterations; it++) {
        const char *p = buf;
        for (size_t i = 0; i < n_lines; i++, p += 64)
            acc = _mm512_add_ps(acc, _mm512_load_ps((const float *)p));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc);
    (void)sink;
    return (double)(iterations * bytes) / (double)(t1 - t0);
}

static double bandwidth_ymm_load(const char *buf, size_t bytes, uint64_t iterations)
{
    size_t n_lines = bytes / 64;
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t it = 0; it < iterations; it++) {
        const char *p = buf;
        for (size_t i = 0; i < n_lines; i++, p += 64) {
            acc0 = _mm256_add_ps(acc0, _mm256_load_ps((const float *)p));
            acc1 = _mm256_add_ps(acc1, _mm256_load_ps((const float *)(p + 32)));
            __asm__ volatile("" : "+x"(acc0), "+x"(acc1));
        }
    }
    uint64_t t1 = sm_zen_tsc_end();

    __m256 sum = _mm256_add_ps(acc0, acc1);
    volatile float sink;
    _mm_store_ss((float *)&sink, _mm256_castps256_ps128(sum));
    return (double)(iterations * bytes) / (double)(t1 - t0);
}

static double bandwidth_zmm_store(char *buf, size_t bytes, uint64_t iterations)
{
    size_t n_lines = bytes / 64;
    __m512 val = _mm512_set1_ps(1.0f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t it = 0; it < iterations; it++) {
        char *p = buf;
        for (size_t i = 0; i < n_lines; i++, p += 64)
            _mm512_store_ps(p, val);
    }
    uint64_t t1 = sm_zen_tsc_end();

    return (double)(iterations * bytes) / (double)(t1 - t0);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 10000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_bandwidth\ncpu=%d\n\n", cfg.cpu);

    static const size_t sizes[] = {
        16384, 32768,                      /* L1d */
        262144, 524288, 1048576,           /* L2 */
        4194304, 16777216, 33554432,       /* L3 */
        67108864                            /* DRAM */
    };
    int n = sizeof(sizes) / sizeof(sizes[0]);

    printf("working_set_KB,zmm_load_B_per_cyc,ymm_load_B_per_cyc,zmm_store_B_per_cyc\n");
    for (int i = 0; i < n; i++) {
        char *buf = (char *)sm_zen_aligned_alloc64(sizes[i]);
        memset(buf, 0, sizes[i]);

        uint64_t iters = cfg.iterations;
        if (sizes[i] > 1048576) iters /= 4;
        if (sizes[i] > 16777216) iters /= 4;

        double zmm_ld = bandwidth_zmm_load(buf, sizes[i], iters);
        double ymm_ld = bandwidth_ymm_load(buf, sizes[i], iters);
        double zmm_st = bandwidth_zmm_store(buf, sizes[i], iters);

        printf("%zu,%.2f,%.2f,%.2f\n", sizes[i] / 1024, zmm_ld, ymm_ld, zmm_st);
        fflush(stdout);
        free(buf);
    }
    return 0;
}
