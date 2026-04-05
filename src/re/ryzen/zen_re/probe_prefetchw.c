#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_prefetchw.c -- Probe for PREFETCHW (sole surviving 3DNow! instruction).
 *
 * Measures throughput of PREFETCHW vs PREFETCHT0/T1/T2/NTA for comparison.
 * PREFETCHW is the write-intent prefetch from the AMD 3DNow! extension,
 * retained in Zen 4 while all other 3DNow! instructions were removed.
 *
 * Output: one CSV line per instruction:
 *   mnemonic,operands,latency_cycles,throughput_cycles,category
 *
 * Build:
 *   clang-22 -D_GNU_SOURCE -O2 -march=native -fno-omit-frame-pointer \
 *     -include x86intrin.h -I. common.c probe_prefetchw.c -lm \
 *     -o ../../../../build/bin/zen_probe_prefetchw
 */

/* ========================================================================
 * PREFETCHW -- write-intent prefetch (3DNow! survivor)
 *
 * Throughput-only measurement: prefetch is a hint, has no data output.
 * We measure how many can issue per cycle by running 8 independent
 * prefetch instructions to different cache lines.
 * ======================================================================== */

static double prefetchw_tp(uint64_t iteration_count)
{
    /* Allocate a buffer spanning multiple cache lines */
    uint8_t *prefetch_buffer = (uint8_t *)sm_zen_aligned_alloc64(8 * 64);
    if (!prefetch_buffer) return -1.0;

    /* Touch each line to ensure it is in cache first */
    for (int line_idx = 0; line_idx < 8; line_idx++) {
        ((volatile uint64_t *)(prefetch_buffer + line_idx * 64))[0] = 0;
    }

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 0 * 64));
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 1 * 64));
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 2 * 64));
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 3 * 64));
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 4 * 64));
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 5 * 64));
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 6 * 64));
        __asm__ volatile("prefetchw (%0)" : : "r"(prefetch_buffer + 7 * 64));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    free(prefetch_buffer);
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHT0 for comparison */
static double prefetcht0_tp(uint64_t iteration_count)
{
    uint8_t *prefetch_buffer = (uint8_t *)sm_zen_aligned_alloc64(8 * 64);
    if (!prefetch_buffer) return -1.0;
    for (int line_idx = 0; line_idx < 8; line_idx++) {
        ((volatile uint64_t *)(prefetch_buffer + line_idx * 64))[0] = 0;
    }

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 0 * 64));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 1 * 64));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 2 * 64));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 3 * 64));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 4 * 64));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 5 * 64));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 6 * 64));
        __asm__ volatile("prefetcht0 (%0)" : : "r"(prefetch_buffer + 7 * 64));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    free(prefetch_buffer);
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHT1 for comparison */
static double prefetcht1_tp(uint64_t iteration_count)
{
    uint8_t *prefetch_buffer = (uint8_t *)sm_zen_aligned_alloc64(8 * 64);
    if (!prefetch_buffer) return -1.0;
    for (int line_idx = 0; line_idx < 8; line_idx++) {
        ((volatile uint64_t *)(prefetch_buffer + line_idx * 64))[0] = 0;
    }

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 0 * 64));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 1 * 64));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 2 * 64));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 3 * 64));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 4 * 64));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 5 * 64));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 6 * 64));
        __asm__ volatile("prefetcht1 (%0)" : : "r"(prefetch_buffer + 7 * 64));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    free(prefetch_buffer);
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHT2 for comparison */
static double prefetcht2_tp(uint64_t iteration_count)
{
    uint8_t *prefetch_buffer = (uint8_t *)sm_zen_aligned_alloc64(8 * 64);
    if (!prefetch_buffer) return -1.0;
    for (int line_idx = 0; line_idx < 8; line_idx++) {
        ((volatile uint64_t *)(prefetch_buffer + line_idx * 64))[0] = 0;
    }

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 0 * 64));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 1 * 64));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 2 * 64));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 3 * 64));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 4 * 64));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 5 * 64));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 6 * 64));
        __asm__ volatile("prefetcht2 (%0)" : : "r"(prefetch_buffer + 7 * 64));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    free(prefetch_buffer);
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* PREFETCHNTA for comparison */
static double prefetchnta_tp(uint64_t iteration_count)
{
    uint8_t *prefetch_buffer = (uint8_t *)sm_zen_aligned_alloc64(8 * 64);
    if (!prefetch_buffer) return -1.0;
    for (int line_idx = 0; line_idx < 8; line_idx++) {
        ((volatile uint64_t *)(prefetch_buffer + line_idx * 64))[0] = 0;
    }

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t loop_idx = 0; loop_idx < iteration_count; loop_idx++) {
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 0 * 64));
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 1 * 64));
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 2 * 64));
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 3 * 64));
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 4 * 64));
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 5 * 64));
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 6 * 64));
        __asm__ volatile("prefetchnta (%0)" : : "r"(prefetch_buffer + 7 * 64));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    free(prefetch_buffer);
    return (double)(tsc_stop - tsc_start) / (double)(iteration_count * 8);
}

/* ========================================================================
 * MEASUREMENT TABLE
 * ======================================================================== */

typedef struct {
    const char *mnemonic;
    const char *operand_description;
    const char *instruction_category;
    double (*measure_latency)(uint64_t);
    double (*measure_throughput)(uint64_t);
} PrefetchProbeEntry;

static const PrefetchProbeEntry probe_table[] = {
    {"PREFETCHW",   "[m8]",   "3dnow_prefetch",  NULL,  prefetchw_tp},
    {"PREFETCHT0",  "[m8]",   "sse_prefetch",    NULL,  prefetcht0_tp},
    {"PREFETCHT1",  "[m8]",   "sse_prefetch",    NULL,  prefetcht1_tp},
    {"PREFETCHT2",  "[m8]",   "sse_prefetch",    NULL,  prefetcht2_tp},
    {"PREFETCHNTA", "[m8]",   "sse_prefetch",    NULL,  prefetchnta_tp},
    {NULL, NULL, NULL, NULL, NULL}
};

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 100000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    int csv_mode = probe_config.csv;

    if (!csv_mode) {
        sm_zen_print_header("prefetchw", &probe_config, &detected_features);
        printf("\n");
        printf("Measuring %s instruction forms...\n\n", "prefetch (3DNow! survivor)");
    }

    /* Print CSV header */
    printf("mnemonic,operands,latency_cycles,throughput_cycles,category\n");

    /* Warmup pass */
    for (int warmup_idx = 0; probe_table[warmup_idx].mnemonic != NULL; warmup_idx++) {
        const PrefetchProbeEntry *current_entry = &probe_table[warmup_idx];
        if (current_entry->measure_latency)
            current_entry->measure_latency(1000);
        if (current_entry->measure_throughput)
            current_entry->measure_throughput(1000);
    }

    /* Measurement pass */
    for (int entry_idx = 0; probe_table[entry_idx].mnemonic != NULL; entry_idx++) {
        const PrefetchProbeEntry *current_entry = &probe_table[entry_idx];

        double measured_latency = -1.0;
        double measured_throughput = -1.0;

        if (current_entry->measure_latency)
            measured_latency = current_entry->measure_latency(probe_config.iterations);
        if (current_entry->measure_throughput)
            measured_throughput = current_entry->measure_throughput(probe_config.iterations);

        printf("%s,%s,%.4f,%.4f,%s\n",
               current_entry->mnemonic,
               current_entry->operand_description,
               measured_latency,
               measured_throughput,
               current_entry->instruction_category);
    }

    return 0;
}
