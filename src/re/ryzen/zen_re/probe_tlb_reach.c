#define _GNU_SOURCE
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <x86intrin.h>

/*
 * probe_tlb_reach.c -- TLB miss penalty and page walk cost on Zen 4.
 *
 * Translated from:
 *   - SASS microbench_cache_topology.cu (TLB-level measurement)
 *   - Apple probe_global_mem_lat.metal (memory latency hierarchy)
 *
 * Pointer chase with stride = PAGE_SIZE (4 KB).
 * Sweep working set from 2 MB (within L1 TLB) to 2 GB (beyond L2 TLB).
 * Latency jumps reveal TLB level boundaries.
 *
 * Also attempts 2M hugepages (MAP_HUGETLB) to show TLB reach extension.
 *
 * Zen 4 TLB structure (per core):
 *   L1 dTLB: 72 entries (4K), 72 entries (2M)
 *   L2 TLB:  3072 entries (4K/2M unified)
 */

/* Build a pointer-chase ring with stride = page_size through the buffer.
   Each node is at offset (i * page_size), and they form a shuffled cycle. */
static void build_page_stride_chase(char *arena, size_t arena_bytes, size_t page_size)
{
    size_t num_pages = arena_bytes / page_size;
    if (num_pages < 2) num_pages = 2;

    /* Allocate index array for Fisher-Yates shuffle */
    size_t *page_indices = (size_t *)malloc(num_pages * sizeof(size_t));
    for (size_t page_index = 0; page_index < num_pages; page_index++)
        page_indices[page_index] = page_index;

    /* Shuffle to randomize TLB access pattern */
    uint32_t shuffle_seed = 0xCAFEBABE;
    for (size_t i = num_pages - 1; i > 0; i--) {
        shuffle_seed = shuffle_seed * 1103515245u + 12345u;
        size_t j = shuffle_seed % (i + 1);
        size_t temp = page_indices[i];
        page_indices[i] = page_indices[j];
        page_indices[j] = temp;
    }

    /* Wire up the pointer chase: each page points to the next in the shuffle */
    for (size_t i = 0; i < num_pages; i++) {
        size_t current_offset = page_indices[i] * page_size;
        size_t next_offset = page_indices[(i + 1) % num_pages] * page_size;
        *(char **)(arena + current_offset) = arena + next_offset;
    }

    free(page_indices);
}

static double measure_tlb_latency(char *arena, size_t arena_bytes, size_t page_size,
                                  uint64_t chase_steps)
{
    build_page_stride_chase(arena, arena_bytes, page_size);

    /* Warmup: walk the ring once */
    size_t num_pages = arena_bytes / page_size;
    volatile char *pointer = (volatile char *)arena;
    for (size_t warmup_step = 0; warmup_step < num_pages * 2; warmup_step++)
        pointer = *(volatile char **)pointer;

    /* Timed chase */
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t step_index = 0; step_index < chase_steps; step_index++)
        pointer = *(volatile char **)pointer;
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile char sink_value = *pointer;
    (void)sink_value;

    return (double)(tsc_stop - tsc_start) / (double)chase_steps;
}

static char *try_mmap_hugepages(size_t bytes)
{
#ifdef MAP_HUGETLB
    /* Try 2M hugepages */
    void *mapped_region = mmap(NULL, bytes,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                               -1, 0);
    if (mapped_region != MAP_FAILED)
        return (char *)mapped_region;
#endif
    return NULL;
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    SMZenFeatures features = sm_zen_detect_features();
    sm_zen_print_header("tlb_reach", &cfg, &features);

    static const size_t working_set_sizes[] = {
        2ULL * 1024 * 1024,       /*   2 MB -- well within L1 dTLB (72 pages = 288 KB) */
        4ULL * 1024 * 1024,       /*   4 MB */
        8ULL * 1024 * 1024,       /*   8 MB */
        16ULL * 1024 * 1024,      /*  16 MB -- L2 TLB boundary (3072 * 4K = 12 MB) */
        32ULL * 1024 * 1024,      /*  32 MB */
        64ULL * 1024 * 1024,      /*  64 MB */
        128ULL * 1024 * 1024,     /* 128 MB */
        256ULL * 1024 * 1024,     /* 256 MB */
        512ULL * 1024 * 1024,     /* 512 MB */
        1024ULL * 1024 * 1024,    /*   1 GB */
        2048ULL * 1024 * 1024,    /*   2 GB */
    };
    int num_sizes = sizeof(working_set_sizes) / sizeof(working_set_sizes[0]);

    /* --- 4K page tests --- */
    if (!cfg.csv)
        printf("\n--- 4K pages, pointer chase stride=4096 ---\n");
    printf("working_set_MB,latency_cycles_4k\n");

    for (int size_index = 0; size_index < num_sizes; size_index++) {
        size_t working_set = working_set_sizes[size_index];
        uint64_t chase_steps = cfg.iterations;
        /* Scale down for very large working sets to keep runtime sane */
        if (working_set > 128ULL * 1024 * 1024) chase_steps /= 2;
        if (working_set > 512ULL * 1024 * 1024) chase_steps /= 2;

        char *arena = (char *)mmap(NULL, working_set,
                                   PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS,
                                   -1, 0);
        if (arena == MAP_FAILED) {
            printf("%zu,MMAP_FAILED\n", working_set / (1024 * 1024));
            continue;
        }
        /* Touch all pages to ensure they are faulted in */
        memset(arena, 0, working_set);

        double latency_cycles = measure_tlb_latency(arena, working_set, 4096, chase_steps);
        printf("%zu,%.2f\n", working_set / (1024 * 1024), latency_cycles);
        fflush(stdout);

        munmap(arena, working_set);
    }

    /* --- 2M hugepage tests --- */
    if (!cfg.csv)
        printf("\n--- 2M hugepages, pointer chase stride=2097152 ---\n");
    printf("working_set_MB,latency_cycles_2m\n");

    int hugepage_available = 0;
    for (int size_index = 0; size_index < num_sizes; size_index++) {
        size_t working_set = working_set_sizes[size_index];
        /* Need at least a few hugepages to be meaningful */
        if (working_set < 4ULL * 1024 * 1024) continue;

        uint64_t chase_steps = cfg.iterations;
        if (working_set > 128ULL * 1024 * 1024) chase_steps /= 2;
        if (working_set > 512ULL * 1024 * 1024) chase_steps /= 2;

        char *arena = try_mmap_hugepages(working_set);
        if (!arena) {
            if (!hugepage_available)
                printf("# hugepages not available (try: echo 1024 > /proc/sys/vm/nr_hugepages)\n");
            break;
        }
        hugepage_available = 1;
        memset(arena, 0, working_set);

        size_t hugepage_size = 2ULL * 1024 * 1024;
        double latency_cycles = measure_tlb_latency(arena, working_set, hugepage_size, chase_steps);
        printf("%zu,%.2f\n", working_set / (1024 * 1024), latency_cycles);
        fflush(stdout);

        munmap(arena, working_set);
    }

    return 0;
}
