#include "common.h"

#include <stdio.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

int main(int argc, char **argv) {
    SMZenProbeConfig cfg = sm_zen_default_config();
    SMZenFeatures features = sm_zen_detect_features();
    volatile float sink = 0.0f;
    uint64_t begin;
    uint64_t end;
    double cycles_per_permute;

    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);
    sm_zen_print_header("permute", &cfg, &features);

#if defined(__AVX2__)
    {
        __m256 acc = _mm256_set_ps(8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
        __m256i control = _mm256_set_epi32(0, 7, 2, 5, 4, 1, 6, 3);
        uint64_t i;
        begin = sm_zen_tsc_begin();
        for(i = 0; i < cfg.iterations; ++i) {
            acc = _mm256_permutevar8x32_ps(acc, control);
        }
        end = sm_zen_tsc_end();
        sink += ((float *)&acc)[0];
    }
#else
    {
        float acc[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        float tmp[8];
        const int order[8] = {3, 6, 1, 4, 5, 2, 7, 0};
        uint64_t i;
        int lane;
        begin = sm_zen_tsc_begin();
        for(i = 0; i < cfg.iterations; ++i) {
            for(lane = 0; lane < 8; ++lane) {
                tmp[lane] = acc[order[lane]];
            }
            for(lane = 0; lane < 8; ++lane) {
                acc[lane] = tmp[lane];
            }
        }
        end = sm_zen_tsc_end();
        sink += acc[0];
    }
#endif

    cycles_per_permute = sm_zen_cycles_per_iter(end - begin, cfg.iterations);
    printf("cycles_per_permute=%0.4f\n", cycles_per_permute);
    printf("sink=%f\n", sink);
    if(cfg.csv) {
        printf("csv,probe=permute,cycles_per_permute=%0.6f\n", cycles_per_permute);
    }
    return 0;
}
