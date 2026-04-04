#define _GNU_SOURCE
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/* Fisher-Yates shuffle for the pointer-chase ring */
static void shuffle_ring(uint64_t *ring, size_t n_nodes, uint32_t seed)
{
    for (size_t i = n_nodes - 1; i > 0; i--) {
        seed = seed * 1103515245u + 12345u;
        size_t j = seed % (i + 1);
        uint64_t tmp = ring[i];
        ring[i] = ring[j];
        ring[j] = tmp;
    }
}

static double measure_chase(size_t working_set_bytes, uint64_t iterations)
{
    const size_t stride = 64;
    size_t n_nodes = working_set_bytes / stride;
    if (n_nodes < 4) n_nodes = 4;

    uint64_t *indices = (uint64_t *)sm_zen_aligned_alloc64(n_nodes * sizeof(uint64_t));
    for (size_t i = 0; i < n_nodes; i++)
        indices[i] = i;
    shuffle_ring(indices, n_nodes, 0xDEADBEEF);

    char *arena = (char *)sm_zen_aligned_alloc64(n_nodes * stride);
    memset(arena, 0, n_nodes * stride);
    for (size_t i = 0; i < n_nodes; i++) {
        size_t next = indices[(i + 1) % n_nodes];
        *(char **)(arena + indices[i] * stride) = arena + next * stride;
    }
    free(indices);

    volatile char *p = (volatile char *)arena;
    for (size_t i = 0; i < n_nodes * 2; i++)
        p = *(volatile char **)p;

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++)
        p = *(volatile char **)p;
    uint64_t t1 = sm_zen_tsc_end();

    volatile char sink_val = *p;
    (void)sink_val;
    free(arena);
    return (double)(t1 - t0) / (double)iterations;
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 2000000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=cache_hierarchy\ncpu=%d\n", cfg.cpu);

    static const size_t sizes[] = {
        4096, 8192, 16384, 32768,
        65536, 131072, 262144, 524288,
        1048576, 2097152, 4194304,
        8388608, 16777216, 33554432,
        67108864
    };
    int n_sizes = sizeof(sizes) / sizeof(sizes[0]);

    printf("\nworking_set_KB,latency_cycles\n");
    for (int i = 0; i < n_sizes; i++) {
        uint64_t iters = cfg.iterations;
        if (sizes[i] > 4194304) iters /= 4;
        if (sizes[i] > 33554432) iters /= 4;
        double lat = measure_chase(sizes[i], iters);
        printf("%zu,%.2f\n", sizes[i] / 1024, lat);
        fflush(stdout);
    }
    return 0;
}
