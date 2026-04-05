#define _GNU_SOURCE
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_store_forwarding.c -- Store-to-load forwarding latency on Zen 4.
 *
 * No direct GPU equivalent (GPUs have register files, not store buffers).
 * This is x86-specific but critical for understanding stack/spill behavior.
 * Maps conceptually to the GPR spill penalties measured in r600 probes.
 *
 * Tests:
 *   1) Same-size forwarding (64-bit store -> 64-bit load): fast path
 *   2) Narrow-to-wide forwarding (32-bit store -> 64-bit load): may stall
 *   3) Wide-to-narrow forwarding (64-bit store -> 32-bit load): usually ok
 *   4) Misaligned forwarding (store at offset, load crossing): penalty
 *   5) Baseline: pure register-to-register (no memory) for comparison
 */

/* All test buffers aligned to cache line boundary */
typedef struct {
    char data[128];
} ForwardingBuffer __attribute__((aligned(64)));

static double measure_same_size_forwarding(uint64_t iterations)
{
    ForwardingBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    volatile uint64_t *memory_location = (volatile uint64_t *)&buffer.data[0];

    uint64_t read_back_value = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        /* Store 64-bit, then load 64-bit from same address -- should forward */
        __asm__ volatile(
            "movq %[val], %[mem]\n\t"
            "movq %[mem], %[out]\n\t"
            : [out] "=r" (read_back_value), [mem] "+m" (*memory_location)
            : [val] "r" (iteration_index)
            : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = read_back_value;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_narrow_to_wide_forwarding(uint64_t iterations)
{
    ForwardingBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    volatile uint32_t *narrow_location = (volatile uint32_t *)&buffer.data[0];
    volatile uint64_t *wide_location = (volatile uint64_t *)&buffer.data[0];

    uint64_t read_back_value = 0;
    uint32_t store_value = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        /* Store 32-bit, then load 64-bit from same base address */
        __asm__ volatile(
            "movl %[val], %[mem32]\n\t"
            "movq %[mem64], %[out]\n\t"
            : [out] "=r" (read_back_value),
              [mem32] "+m" (*narrow_location),
              [mem64] "+m" (*wide_location)
            : [val] "r" (store_value)
            : "memory"
        );
        store_value = (uint32_t)read_back_value;
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = read_back_value;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_wide_to_narrow_forwarding(uint64_t iterations)
{
    ForwardingBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    volatile uint64_t *wide_location = (volatile uint64_t *)&buffer.data[0];
    volatile uint32_t *narrow_location = (volatile uint32_t *)&buffer.data[0];

    uint32_t read_back_value = 0;
    uint64_t store_value = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        /* Store 64-bit, then load 32-bit from same base address */
        __asm__ volatile(
            "movq %[val], %[mem64]\n\t"
            "movl %[mem32], %[out]\n\t"
            : [out] "=r" (read_back_value),
              [mem64] "+m" (*wide_location),
              [mem32] "+m" (*narrow_location)
            : [val] "r" (store_value)
            : "memory"
        );
        store_value = (uint64_t)read_back_value;
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint32_t sink = read_back_value;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_misaligned_forwarding(uint64_t iterations)
{
    ForwardingBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    /* Store at offset 0 (aligned), load at offset 1 (misaligned, partially overlapping) */
    volatile uint64_t *aligned_location = (volatile uint64_t *)&buffer.data[0];
    volatile uint64_t *misaligned_location = (volatile uint64_t *)&buffer.data[1];

    uint64_t read_back_value = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        /* Store 64-bit aligned, load 64-bit at +1 byte offset -- forwarding fails */
        __asm__ volatile(
            "movq %[val], %[mem_aligned]\n\t"
            "movq %[mem_misaligned], %[out]\n\t"
            : [out] "=r" (read_back_value),
              [mem_aligned] "+m" (*aligned_location),
              [mem_misaligned] "+m" (*misaligned_location)
            : [val] "r" (iteration_index)
            : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = read_back_value;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_register_baseline(uint64_t iterations)
{
    /* Pure register-to-register dependency chain, no memory */
    uint64_t accumulator_value = 1;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        __asm__ volatile(
            "addq $1, %[acc]\n\t"
            : [acc] "+r" (accumulator_value)
            :
            :
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator_value;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_store_load_different_address(uint64_t iterations)
{
    /* Store to one address, load from a different address (L1 hit, no forwarding) */
    ForwardingBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    volatile uint64_t *store_location = (volatile uint64_t *)&buffer.data[0];
    volatile uint64_t *load_location = (volatile uint64_t *)&buffer.data[64]; /* different cache line */

    uint64_t read_back_value = 0;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        __asm__ volatile(
            "movq %[val], %[mem_st]\n\t"
            "movq %[mem_ld], %[out]\n\t"
            : [out] "=r" (read_back_value),
              [mem_st] "+m" (*store_location),
              [mem_ld] "+m" (*load_location)
            : [val] "r" (iteration_index)
            : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = read_back_value;
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    SMZenFeatures features = sm_zen_detect_features();
    sm_zen_print_header("store_forwarding", &cfg, &features);

    /* Use a large iteration count for tight latency measurement */
    uint64_t measurement_iterations = cfg.iterations * 200;

    double register_baseline_latency = measure_register_baseline(measurement_iterations);
    double same_size_forwarding_latency = measure_same_size_forwarding(measurement_iterations);
    double narrow_to_wide_latency = measure_narrow_to_wide_forwarding(measurement_iterations);
    double wide_to_narrow_latency = measure_wide_to_narrow_forwarding(measurement_iterations);
    double misaligned_forwarding_latency = measure_misaligned_forwarding(measurement_iterations);
    double different_address_latency = measure_store_load_different_address(measurement_iterations);

    if (!cfg.csv) {
        printf("\nmeasurement_iterations=%lu\n\n", (unsigned long)measurement_iterations);

        printf("--- store-to-load forwarding latency (cycles) ---\n");
        printf("  register_baseline (add r,r):           %.2f cyc\n", register_baseline_latency);
        printf("  same_size (mov q->q forwarding):       %.2f cyc\n", same_size_forwarding_latency);
        printf("  narrow_to_wide (mov d->q forwarding):  %.2f cyc\n", narrow_to_wide_latency);
        printf("  wide_to_narrow (mov q->d forwarding):  %.2f cyc\n", wide_to_narrow_latency);
        printf("  misaligned (offset +1 byte):           %.2f cyc\n", misaligned_forwarding_latency);
        printf("  different_address (store!=load, L1):   %.2f cyc\n", different_address_latency);

        printf("\n--- forwarding overhead vs register baseline ---\n");
        printf("  same_size_overhead:     +%.2f cyc\n",
               same_size_forwarding_latency - register_baseline_latency);
        printf("  narrow_to_wide_overhead: +%.2f cyc\n",
               narrow_to_wide_latency - register_baseline_latency);
        printf("  wide_to_narrow_overhead: +%.2f cyc\n",
               wide_to_narrow_latency - register_baseline_latency);
        printf("  misaligned_overhead:    +%.2f cyc\n",
               misaligned_forwarding_latency - register_baseline_latency);
    }

    if (cfg.csv) {
        printf("csv_header,test,latency_cycles,overhead_vs_reg\n");
        printf("register_baseline,%.4f,0.0000\n", register_baseline_latency);
        printf("same_size,%.4f,%.4f\n", same_size_forwarding_latency,
               same_size_forwarding_latency - register_baseline_latency);
        printf("narrow_to_wide,%.4f,%.4f\n", narrow_to_wide_latency,
               narrow_to_wide_latency - register_baseline_latency);
        printf("wide_to_narrow,%.4f,%.4f\n", wide_to_narrow_latency,
               wide_to_narrow_latency - register_baseline_latency);
        printf("misaligned,%.4f,%.4f\n", misaligned_forwarding_latency,
               misaligned_forwarding_latency - register_baseline_latency);
        printf("different_address,%.4f,%.4f\n", different_address_latency,
               different_address_latency - register_baseline_latency);
    }

    return 0;
}
