#include "common.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

int main(int argc, char **argv) {
    SMZenProbeConfig cfg = sm_zen_default_config();
    SMZenFeatures features = sm_zen_detect_features();
    uint32_t *table;
    uint32_t *indices;
    size_t count;
    volatile uint32_t sink = 0u;
    uint64_t begin;
    uint64_t end;
    double cycles_per_gather;

    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);
    sm_zen_print_header("gather", &cfg, &features);

    count = cfg.working_set_bytes / sizeof(uint32_t);
    if(count < 8192u) {
        count = 8192u;
    }

    table = (uint32_t *)sm_zen_aligned_alloc64(count * sizeof(uint32_t));
    indices = (uint32_t *)sm_zen_aligned_alloc64(8u * sizeof(uint32_t));
    sm_zen_fill_random_u32(table, count, 0x1234u);
    sm_zen_fill_random_u32(indices, 8u, 0xC0FFEEu);
    for(size_t i = 0; i < 8u; ++i) {
        indices[i] %= (uint32_t)count;
    }

#if defined(__AVX2__)
    {
        __m256i idx = _mm256_load_si256((const __m256i *)indices);
        __m256i acc = _mm256_setzero_si256();
        uint64_t i;
        begin = sm_zen_tsc_begin();
        for(i = 0; i < cfg.iterations; ++i) {
            __m256i g = _mm256_i32gather_epi32((const int *)table, idx, 4);
            acc = _mm256_add_epi32(acc, g);
            idx = _mm256_add_epi32(idx, _mm256_set1_epi32(13));
            idx = _mm256_and_si256(idx, _mm256_set1_epi32((int)(count - 1)));
        }
        end = sm_zen_tsc_end();
        sink = (uint32_t)_mm256_extract_epi32(acc, 0);
    }
#else
    {
        uint32_t acc = 0u;
        uint64_t i;
        begin = sm_zen_tsc_begin();
        for(i = 0; i < cfg.iterations; ++i) {
            int lane;
            for(lane = 0; lane < 8; ++lane) {
                acc += table[indices[lane]];
                indices[lane] = (indices[lane] + 13u) & (uint32_t)(count - 1u);
            }
        }
        end = sm_zen_tsc_end();
        sink = acc;
    }
#endif

    cycles_per_gather = sm_zen_cycles_per_iter(end - begin, cfg.iterations);
    printf("cycles_per_vector_gather=%0.4f\n", cycles_per_gather);
    printf("sink=%u\n", sink);
    if(cfg.csv) {
        printf("csv,probe=gather,cycles_per_vector_gather=%0.6f\n", cycles_per_gather);
    }

    free(indices);
    free(table);
    return 0;
}
