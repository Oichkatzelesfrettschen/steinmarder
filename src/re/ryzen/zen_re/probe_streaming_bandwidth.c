#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_streaming_bandwidth.c -- Sustained bandwidth: non-temporal vs regular stores/loads.
 *
 * Translated from:
 *   - SASS probe_stg_cache_hints.cu (cache bypass store semantics)
 *   - r600 async_workgroup_copy.cl (bandwidth measurement concept)
 *
 * Measures at 128 MB (DRAM-resident):
 *   - vmovntps zmm (non-temporal 512-bit store)
 *   - vmovaps zmm  (regular 512-bit store)
 *   - vmovntdqa zmm (non-temporal 512-bit load)
 *   - vmovaps zmm   (regular 512-bit load)
 *
 * Reports GB/s for each variant.
 */

#define WORKING_SET_BYTES (128ULL * 1024 * 1024)

/* We estimate ~5 GHz for the 7945HX to convert cycles to seconds.
   The user can check actual frequency; ratio comparisons are cycle-accurate. */
#define ASSUMED_GHZ 5.1

static double measure_nt_store_bandwidth(char *buffer, size_t buffer_bytes, uint64_t iterations)
{
    size_t num_cache_lines = buffer_bytes / 64;
    __m512 store_value = _mm512_set1_ps(1.0f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        char *write_pointer = buffer;
        for (size_t line_index = 0; line_index < num_cache_lines; line_index++, write_pointer += 64) {
            _mm512_stream_ps((float *)write_pointer, store_value);
        }
    }
    _mm_sfence();
    uint64_t tsc_stop = sm_zen_tsc_end();

    double total_bytes = (double)(iterations * buffer_bytes);
    double elapsed_cycles = (double)(tsc_stop - tsc_start);
    return total_bytes / elapsed_cycles;  /* bytes per cycle */
}

static double measure_regular_store_bandwidth(char *buffer, size_t buffer_bytes, uint64_t iterations)
{
    size_t num_cache_lines = buffer_bytes / 64;
    __m512 store_value = _mm512_set1_ps(1.0f);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        char *write_pointer = buffer;
        for (size_t line_index = 0; line_index < num_cache_lines; line_index++, write_pointer += 64) {
            _mm512_store_ps((float *)write_pointer, store_value);
        }
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    double total_bytes = (double)(iterations * buffer_bytes);
    double elapsed_cycles = (double)(tsc_stop - tsc_start);
    return total_bytes / elapsed_cycles;
}

static double measure_nt_load_bandwidth(const char *buffer, size_t buffer_bytes, uint64_t iterations)
{
    size_t num_cache_lines = buffer_bytes / 64;
    __m512i accumulator = _mm512_setzero_si512();

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        const char *read_pointer = buffer;
        for (size_t line_index = 0; line_index < num_cache_lines; line_index++, read_pointer += 64) {
            __m512i loaded_data = _mm512_stream_load_si512((const __m512i *)read_pointer);
            accumulator = _mm512_add_epi64(accumulator, loaded_data);
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int64_t sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;

    double total_bytes = (double)(iterations * buffer_bytes);
    double elapsed_cycles = (double)(tsc_stop - tsc_start);
    return total_bytes / elapsed_cycles;
}

static double measure_regular_load_bandwidth(const char *buffer, size_t buffer_bytes, uint64_t iterations)
{
    size_t num_cache_lines = buffer_bytes / 64;
    __m512 accumulator = _mm512_setzero_ps();

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        const char *read_pointer = buffer;
        for (size_t line_index = 0; line_index < num_cache_lines; line_index++, read_pointer += 64) {
            accumulator = _mm512_add_ps(accumulator, _mm512_load_ps((const float *)read_pointer));
            __asm__ volatile("" : "+x"(accumulator));
        }
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(accumulator);
    (void)sink;

    double total_bytes = (double)(iterations * buffer_bytes);
    double elapsed_cycles = (double)(tsc_stop - tsc_start);
    return total_bytes / elapsed_cycles;
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    cfg.working_set_bytes = WORKING_SET_BYTES;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    SMZenFeatures features = sm_zen_detect_features();
    sm_zen_print_header("streaming_bandwidth", &cfg, &features);

    if (!features.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    size_t working_set = cfg.working_set_bytes;
    char *buffer = (char *)sm_zen_aligned_alloc64(working_set);
    memset(buffer, 0, working_set);

    /* Scale iterations so total work is tractable at 128 MB.
       Each iteration touches the full working set once. */
    uint64_t store_iterations = 20;
    uint64_t load_iterations = 20;

    printf("\nworking_set_MB=%zu\n", working_set / (1024 * 1024));
    printf("store_passes=%lu  load_passes=%lu\n\n",
           (unsigned long)store_iterations, (unsigned long)load_iterations);

    double nt_store_bytes_per_cycle = measure_nt_store_bandwidth(buffer, working_set, store_iterations);
    /* Flush cache between tests */
    memset(buffer, 0xAA, working_set);
    _mm_mfence();

    double reg_store_bytes_per_cycle = measure_regular_store_bandwidth(buffer, working_set, store_iterations);
    memset(buffer, 0x55, working_set);
    _mm_mfence();

    double nt_load_bytes_per_cycle = measure_nt_load_bandwidth(buffer, working_set, load_iterations);
    memset(buffer, 0x33, working_set);
    _mm_mfence();

    double reg_load_bytes_per_cycle = measure_regular_load_bandwidth(buffer, working_set, load_iterations);

    double ghz = ASSUMED_GHZ;
    double nt_store_gbps = nt_store_bytes_per_cycle * ghz;
    double reg_store_gbps = reg_store_bytes_per_cycle * ghz;
    double nt_load_gbps = nt_load_bytes_per_cycle * ghz;
    double reg_load_gbps = reg_load_bytes_per_cycle * ghz;

    if (!cfg.csv) {
        printf("--- store bandwidth (128 MB) ---\n");
        printf("  vmovntps_zmm_bytes_per_cyc=%.4f  (~%.2f GB/s at %.1f GHz)\n",
               nt_store_bytes_per_cycle, nt_store_gbps, ghz);
        printf("  vmovaps_zmm_bytes_per_cyc=%.4f  (~%.2f GB/s at %.1f GHz)\n",
               reg_store_bytes_per_cycle, reg_store_gbps, ghz);
        printf("  nt_store_advantage=%.2fx\n\n",
               nt_store_bytes_per_cycle / reg_store_bytes_per_cycle);

        printf("--- load bandwidth (128 MB) ---\n");
        printf("  vmovntdqa_zmm_bytes_per_cyc=%.4f  (~%.2f GB/s at %.1f GHz)\n",
               nt_load_bytes_per_cycle, nt_load_gbps, ghz);
        printf("  vmovaps_zmm_bytes_per_cyc=%.4f  (~%.2f GB/s at %.1f GHz)\n",
               reg_load_bytes_per_cycle, reg_load_gbps, ghz);
        printf("  nt_load_advantage=%.2fx\n",
               nt_load_bytes_per_cycle / reg_load_bytes_per_cycle);
    }

    if (cfg.csv) {
        printf("csv_header,test,bytes_per_cyc,est_gbps\n");
        printf("nt_store,%.4f,%.2f\n", nt_store_bytes_per_cycle, nt_store_gbps);
        printf("reg_store,%.4f,%.2f\n", reg_store_bytes_per_cycle, reg_store_gbps);
        printf("nt_load,%.4f,%.2f\n", nt_load_bytes_per_cycle, nt_load_gbps);
        printf("reg_load,%.4f,%.2f\n", reg_load_bytes_per_cycle, reg_load_gbps);
    }

    free(buffer);
    return 0;
}
