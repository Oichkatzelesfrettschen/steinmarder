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
    uint64_t cycles_latency;
    uint64_t cycles_throughput;
    double latency_per_vec;
    double throughput_per_vec;

    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);
    sm_zen_print_header("fma", &cfg, &features);

#if defined(__AVX2__) && defined(__FMA__)
    {
        __m256 a = _mm256_set1_ps(1.00013f);
        __m256 b = _mm256_set1_ps(1.00037f);
        __m256 c = _mm256_set1_ps(0.99991f);
        __m256 acc = _mm256_set1_ps(1.0f);
        uint64_t i;

        begin = sm_zen_tsc_begin();
        for(i = 0; i < cfg.iterations; ++i) {
            acc = _mm256_fmadd_ps(acc, a, b);
            acc = _mm256_fmadd_ps(acc, c, b);
        }
        end = sm_zen_tsc_end();
        cycles_latency = end - begin;
        sink += ((float *)&acc)[0];

        {
            __m256 acc0 = _mm256_set1_ps(1.0f);
            __m256 acc1 = _mm256_set1_ps(2.0f);
            __m256 acc2 = _mm256_set1_ps(3.0f);
            __m256 acc3 = _mm256_set1_ps(4.0f);
            __m256 acc4 = _mm256_set1_ps(5.0f);
            __m256 acc5 = _mm256_set1_ps(6.0f);
            __m256 acc6 = _mm256_set1_ps(7.0f);
            __m256 acc7 = _mm256_set1_ps(8.0f);

            begin = sm_zen_tsc_begin();
            for(i = 0; i < cfg.iterations; ++i) {
                acc0 = _mm256_fmadd_ps(acc0, a, b);
                acc1 = _mm256_fmadd_ps(acc1, a, b);
                acc2 = _mm256_fmadd_ps(acc2, a, b);
                acc3 = _mm256_fmadd_ps(acc3, a, b);
                acc4 = _mm256_fmadd_ps(acc4, a, b);
                acc5 = _mm256_fmadd_ps(acc5, a, b);
                acc6 = _mm256_fmadd_ps(acc6, a, b);
                acc7 = _mm256_fmadd_ps(acc7, a, b);
            }
            end = sm_zen_tsc_end();
            cycles_throughput = end - begin;
            sink += ((float *)&acc7)[0];
        }
    }
#else
    {
        float a = 1.00013f;
        float b = 1.00037f;
        float c = 0.99991f;
        float acc = 1.0f;
        float accs[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
        uint64_t i;

        begin = sm_zen_tsc_begin();
        for(i = 0; i < cfg.iterations; ++i) {
            acc = acc * a + b;
            acc = acc * c + b;
        }
        end = sm_zen_tsc_end();
        cycles_latency = end - begin;
        sink += acc;

        begin = sm_zen_tsc_begin();
        for(i = 0; i < cfg.iterations; ++i) {
            int j;
            for(j = 0; j < 8; ++j) {
                accs[j] = accs[j] * a + b;
            }
        }
        end = sm_zen_tsc_end();
        cycles_throughput = end - begin;
        sink += accs[7];
    }
#endif

    latency_per_vec = sm_zen_cycles_per_iter(cycles_latency, cfg.iterations * 2u);
    throughput_per_vec = sm_zen_cycles_per_iter(cycles_throughput, cfg.iterations * 8u);

    printf("latency_cycles_per_fma=%0.4f\n", latency_per_vec);
    printf("throughput_cycles_per_fma=%0.4f\n", throughput_per_vec);
    printf("sink=%f\n", sink);
    if(cfg.csv) {
        printf("csv,probe=fma,latency_cycles_per_fma=%0.6f,throughput_cycles_per_fma=%0.6f\n",
               latency_per_vec, throughput_per_vec);
    }
    return 0;
}
