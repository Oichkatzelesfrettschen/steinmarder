#define _GNU_SOURCE
#include "common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

/*
 * probe_false_sharing.c -- Cross-core cache line contention (false sharing).
 *
 * Translated from:
 *   - SASS probe_atom_sys_scope_all.cu (scope-based visibility concept)
 *
 * Two threads each write to a 4-byte variable:
 *   1) Both in the SAME 64-byte cache line (false sharing)
 *   2) In DIFFERENT cache lines (no sharing)
 *
 * This is the x86 equivalent of warp-level vs system-scope atomics on GPU:
 * cache coherence protocol overhead manifests as throughput degradation.
 */

typedef struct {
    volatile uint32_t variable_a;
    volatile uint32_t variable_b;  /* 4 bytes apart -- same cache line */
    char padding_remainder[56];
} FalseSharingLayout __attribute__((aligned(64)));

typedef struct {
    volatile uint32_t variable_a;
    char padding_to_next_line[60];
    volatile uint32_t variable_b;  /* 64 bytes apart -- different cache line */
    char padding_remainder[60];
} NoSharingLayout __attribute__((aligned(64)));

typedef struct {
    int core_id;
    uint64_t write_iterations;
    volatile uint32_t *target_variable;
    pthread_barrier_t *start_barrier;
    double cycles_elapsed;
    double ops_per_cycle;
} WriterThreadArgs;

static void *writer_thread_function(void *arg)
{
    WriterThreadArgs *writer_args = (WriterThreadArgs *)arg;
    sm_zen_pin_to_cpu(writer_args->core_id);

    volatile uint32_t *target = writer_args->target_variable;
    uint64_t num_iterations = writer_args->write_iterations;

    pthread_barrier_wait(writer_args->start_barrier);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < num_iterations; iteration_index++) {
        (*target)++;
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    writer_args->cycles_elapsed = (double)(tsc_stop - tsc_start);
    writer_args->ops_per_cycle = (double)num_iterations / writer_args->cycles_elapsed;
    return NULL;
}

static void run_sharing_test(const char *label, volatile uint32_t *target_a,
                             volatile uint32_t *target_b, int core_a, int core_b,
                             uint64_t iterations, int csv_mode)
{
    pthread_t thread_handles[2];
    WriterThreadArgs writer_arguments[2];
    pthread_barrier_t start_barrier;

    *target_a = 0;
    *target_b = 0;

    pthread_barrier_init(&start_barrier, NULL, 2);

    writer_arguments[0].core_id = core_a;
    writer_arguments[0].write_iterations = iterations;
    writer_arguments[0].target_variable = target_a;
    writer_arguments[0].start_barrier = &start_barrier;

    writer_arguments[1].core_id = core_b;
    writer_arguments[1].write_iterations = iterations;
    writer_arguments[1].target_variable = target_b;
    writer_arguments[1].start_barrier = &start_barrier;

    struct timespec wall_start_time;
    clock_gettime(CLOCK_MONOTONIC, &wall_start_time);

    pthread_create(&thread_handles[0], NULL, writer_thread_function, &writer_arguments[0]);
    pthread_create(&thread_handles[1], NULL, writer_thread_function, &writer_arguments[1]);

    pthread_join(thread_handles[0], NULL);
    pthread_join(thread_handles[1], NULL);

    struct timespec wall_end_time;
    clock_gettime(CLOCK_MONOTONIC, &wall_end_time);
    double wall_elapsed_seconds =
        (wall_end_time.tv_sec - wall_start_time.tv_sec) +
        (wall_end_time.tv_nsec - wall_start_time.tv_nsec) * 1e-9;

    double worst_ops_per_cycle = writer_arguments[0].ops_per_cycle;
    if (writer_arguments[1].ops_per_cycle < worst_ops_per_cycle)
        worst_ops_per_cycle = writer_arguments[1].ops_per_cycle;

    if (!csv_mode) {
        printf("\n--- %s (cores %d, %d) ---\n", label, core_a, core_b);
        printf("  core %d: %.4f ops/cyc  (%.0f cycles)\n",
               core_a, writer_arguments[0].ops_per_cycle, writer_arguments[0].cycles_elapsed);
        printf("  core %d: %.4f ops/cyc  (%.0f cycles)\n",
               core_b, writer_arguments[1].ops_per_cycle, writer_arguments[1].cycles_elapsed);
        printf("  worst_ops_per_cyc=%.6f\n", worst_ops_per_cycle);
        printf("  wall_sec=%.6f\n", wall_elapsed_seconds);
    }

    if (csv_mode) {
        printf("%s,%d,%d,%.6f,%.6f,%.6f\n",
               label, core_a, core_b,
               writer_arguments[0].ops_per_cycle,
               writer_arguments[1].ops_per_cycle,
               wall_elapsed_seconds);
    }

    pthread_barrier_destroy(&start_barrier);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    SMZenFeatures features = sm_zen_detect_features();
    sm_zen_print_header("false_sharing", &cfg, &features);

    /* Scale: each thread does this many increments */
    uint64_t per_thread_iterations = cfg.iterations * 100;

    FalseSharingLayout *false_sharing_data =
        (FalseSharingLayout *)sm_zen_aligned_alloc64(sizeof(FalseSharingLayout));
    memset(false_sharing_data, 0, sizeof(FalseSharingLayout));

    NoSharingLayout *no_sharing_data =
        (NoSharingLayout *)sm_zen_aligned_alloc64(sizeof(NoSharingLayout));
    memset(no_sharing_data, 0, sizeof(NoSharingLayout));

    if (!cfg.csv) {
        printf("\niterations_per_thread=%lu\n", (unsigned long)per_thread_iterations);
        printf("false_sharing: two uint32 at offsets 0,4 in same 64B line\n");
        printf("no_sharing:    two uint32 in separate 64B lines\n");
    }

    if (cfg.csv)
        printf("csv_header,test,core_a,core_b,ops_per_cyc_a,ops_per_cyc_b,wall_sec\n");

    /* Same-CCD test: cores 0 and 1 (both on CCD0 for 7945HX) */
    run_sharing_test("false_sharing_same_ccd",
                     &false_sharing_data->variable_a,
                     &false_sharing_data->variable_b,
                     0, 1, per_thread_iterations, cfg.csv);

    run_sharing_test("no_sharing_same_ccd",
                     &no_sharing_data->variable_a,
                     &no_sharing_data->variable_b,
                     0, 1, per_thread_iterations, cfg.csv);

    /* Cross-CCD test: cores 0 and 8 (CCD0 vs CCD1 for 7945HX) */
    run_sharing_test("false_sharing_cross_ccd",
                     &false_sharing_data->variable_a,
                     &false_sharing_data->variable_b,
                     0, 8, per_thread_iterations, cfg.csv);

    run_sharing_test("no_sharing_cross_ccd",
                     &no_sharing_data->variable_a,
                     &no_sharing_data->variable_b,
                     0, 8, per_thread_iterations, cfg.csv);

    if (!cfg.csv) {
        /* Compute and report the false sharing penalty */
        printf("\n--- summary ---\n");
        printf("  (Compare same-CCD false vs no sharing for protocol overhead.)\n");
        printf("  (Compare same-CCD vs cross-CCD for inter-die penalty.)\n");
    }

    free(false_sharing_data);
    free(no_sharing_data);
    return 0;
}
