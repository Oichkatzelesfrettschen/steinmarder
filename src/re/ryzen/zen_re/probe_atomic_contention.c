#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

/*
 * probe_atomic_contention.c -- Atomic operation throughput under contention on Zen 4.
 *
 * Translated from:
 *   - SASS probe_atoms_basic_ops.cu, probe_satom_int32_add.cu
 *   - r600 atomic_cas_throughput.cl
 *   - Apple probe_atomic_add.metal
 *
 * Single-threaded: throughput of lock xadd, lock cmpxchg, lock xchg
 * on a single cache line.
 * Multi-threaded: 2/4/8/16 threads atomically incrementing the same counter.
 * Reports ops/cycle for each configuration.
 */

/* Shared counter aligned to its own cache line to avoid noise */
typedef struct {
    volatile int64_t counter;
    char padding[56];
} AlignedCounter __attribute__((aligned(64)));

static AlignedCounter shared_counter;

/* ---------- single-threaded throughput ---------- */

static double measure_lock_xadd_throughput(volatile int64_t *target, uint64_t iterations)
{
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        __asm__ volatile(
            "lock xaddq %[inc], %[mem]"
            : [mem] "+m" (*target)
            : [inc] "r" ((int64_t)1)
            : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)iterations / (double)(tsc_stop - tsc_start);
}

static double measure_lock_cmpxchg_throughput(volatile int64_t *target, uint64_t iterations)
{
    int64_t expected_value = *target;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        int64_t desired_value = expected_value + 1;
        __asm__ volatile(
            "lock cmpxchgq %[desired], %[mem]"
            : [mem] "+m" (*target), "+a" (expected_value)
            : [desired] "r" (desired_value)
            : "memory"
        );
        /* On failure expected_value holds the current value, which is fine
           for the next attempt. On success we advance by 1. */
        expected_value = *target;
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    return (double)iterations / (double)(tsc_stop - tsc_start);
}

static double measure_lock_xchg_throughput(volatile int64_t *target, uint64_t iterations)
{
    int64_t swap_value = 42;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < iterations; iteration_index++) {
        __asm__ volatile(
            "xchgq %[val], %[mem]"
            : [val] "+r" (swap_value), [mem] "+m" (*target)
            :
            : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    (void)swap_value;
    return (double)iterations / (double)(tsc_stop - tsc_start);
}

/* ---------- multi-threaded contention ---------- */

typedef struct {
    int core_id;
    uint64_t atomic_iterations;
    volatile int64_t *shared_target;
    pthread_barrier_t *start_barrier;
    double ops_per_cycle_result;
} AtomicWorkerArgs;

static void *atomic_add_contention_worker(void *arg)
{
    AtomicWorkerArgs *worker_args = (AtomicWorkerArgs *)arg;
    sm_zen_pin_to_cpu(worker_args->core_id);

    pthread_barrier_wait(worker_args->start_barrier);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t iteration_index = 0; iteration_index < worker_args->atomic_iterations; iteration_index++) {
        __asm__ volatile(
            "lock xaddq %[inc], %[mem]"
            : [mem] "+m" (*worker_args->shared_target)
            : [inc] "r" ((int64_t)1)
            : "memory"
        );
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    worker_args->ops_per_cycle_result =
        (double)worker_args->atomic_iterations / (double)(tsc_stop - tsc_start);
    return NULL;
}

static void run_contention_test(int num_threads, uint64_t iterations_per_thread, int csv_mode)
{
    pthread_t *thread_handles = malloc(num_threads * sizeof(pthread_t));
    AtomicWorkerArgs *worker_arguments = malloc(num_threads * sizeof(AtomicWorkerArgs));
    pthread_barrier_t start_barrier;

    shared_counter.counter = 0;
    pthread_barrier_init(&start_barrier, NULL, num_threads);

    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        worker_arguments[thread_index].core_id = thread_index;
        worker_arguments[thread_index].atomic_iterations = iterations_per_thread;
        worker_arguments[thread_index].shared_target = &shared_counter.counter;
        worker_arguments[thread_index].start_barrier = &start_barrier;
    }

    struct timespec wall_start_time;
    clock_gettime(CLOCK_MONOTONIC, &wall_start_time);

    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        pthread_create(&thread_handles[thread_index], NULL,
                       atomic_add_contention_worker, &worker_arguments[thread_index]);
    }
    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        pthread_join(thread_handles[thread_index], NULL);
    }

    struct timespec wall_end_time;
    clock_gettime(CLOCK_MONOTONIC, &wall_end_time);
    double wall_elapsed_seconds =
        (wall_end_time.tv_sec - wall_start_time.tv_sec) +
        (wall_end_time.tv_nsec - wall_start_time.tv_nsec) * 1e-9;

    double total_ops = (double)num_threads * (double)iterations_per_thread;
    double worst_ops_per_cycle = 1e30;

    if (!csv_mode)
        printf("\n--- %d thread(s), lock xadd contention ---\n", num_threads);

    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        double thread_ops_per_cycle = worker_arguments[thread_index].ops_per_cycle_result;
        if (thread_ops_per_cycle < worst_ops_per_cycle)
            worst_ops_per_cycle = thread_ops_per_cycle;
        if (!csv_mode)
            printf("  core %2d: %.4f ops/cyc\n",
                   worker_arguments[thread_index].core_id, thread_ops_per_cycle);
    }

    double aggregate_ops_per_second = total_ops / wall_elapsed_seconds;
    if (!csv_mode) {
        printf("  worst_per_thread_ops_per_cyc=%.6f\n", worst_ops_per_cycle);
        printf("  aggregate_Mops_per_sec=%.2f\n", aggregate_ops_per_second / 1e6);
        printf("  wall_sec=%.6f\n", wall_elapsed_seconds);
    }

    if (csv_mode)
        printf("contention,%d,%.6f,%.2f,%.6f\n",
               num_threads, worst_ops_per_cycle,
               aggregate_ops_per_second / 1e6, wall_elapsed_seconds);

    pthread_barrier_destroy(&start_barrier);
    free(thread_handles);
    free(worker_arguments);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    SMZenFeatures features = sm_zen_detect_features();
    sm_zen_print_header("atomic_contention", &cfg, &features);

    /* --- Single-threaded atomic throughput --- */
    volatile int64_t single_thread_target __attribute__((aligned(64))) = 0;

    double xadd_ops_per_cycle = measure_lock_xadd_throughput(&single_thread_target, cfg.iterations);
    double cmpxchg_ops_per_cycle = measure_lock_cmpxchg_throughput(&single_thread_target, cfg.iterations);
    double xchg_ops_per_cycle = measure_lock_xchg_throughput(&single_thread_target, cfg.iterations);

    if (!cfg.csv) {
        printf("\n--- single-threaded atomic throughput ---\n");
        printf("  lock_xadd_ops_per_cyc=%.6f\n", xadd_ops_per_cycle);
        printf("  lock_cmpxchg_ops_per_cyc=%.6f\n", cmpxchg_ops_per_cycle);
        printf("  lock_xchg_ops_per_cyc=%.6f\n", xchg_ops_per_cycle);
    }

    if (cfg.csv) {
        printf("csv_header,test,threads,worst_ops_per_cyc,agg_Mops_sec,wall_sec\n");
        printf("single,1,xadd=%.6f,cmpxchg=%.6f,xchg=%.6f\n",
               xadd_ops_per_cycle, cmpxchg_ops_per_cycle, xchg_ops_per_cycle);
    }

    /* --- Multi-threaded contention --- */
    static const int thread_counts[] = { 2, 4, 8, 16 };
    int num_configurations = sizeof(thread_counts) / sizeof(thread_counts[0]);

    for (int config_index = 0; config_index < num_configurations; config_index++) {
        run_contention_test(thread_counts[config_index], cfg.iterations, cfg.csv);
    }

    return 0;
}
