#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

/*
 * probe_avx512_multicore.c -- Multi-core AVX-512 FMA scaling on Zen 4.
 *
 * Spawns 1, 2, 4, 8, 16 worker threads, each pinned to a different core,
 * each running a sustained zmm FMA throughput loop. Measures wall-clock time
 * and reports per-thread and aggregate throughput.
 *
 * 7945HX = 2 CCDs x 8 cores (cores 0-7 = CCD0, cores 8-15 = CCD1).
 * WSL2 may remap core IDs; we use sequential IDs and note the topology.
 */

typedef struct {
    int core_id;
    uint64_t fma_iterations;
    double elapsed_cycles;
    double cycles_per_fma;
} WorkerResult;

typedef struct {
    int core_id;
    uint64_t fma_iterations;
    WorkerResult result;
    pthread_barrier_t *start_barrier;
} WorkerArgs;

static void *fma_worker_thread(void *arg)
{
    WorkerArgs *worker_args = (WorkerArgs *)arg;
    sm_zen_pin_to_cpu(worker_args->core_id);

    /* 8 independent zmm FMA accumulators -- same pattern as probe_avx512_fma throughput */
    __m512 accumulator0 = _mm512_set1_ps(1.0f);
    __m512 accumulator1 = _mm512_set1_ps(1.0001f);
    __m512 accumulator2 = _mm512_set1_ps(1.0002f);
    __m512 accumulator3 = _mm512_set1_ps(1.0003f);
    __m512 accumulator4 = _mm512_set1_ps(1.0004f);
    __m512 accumulator5 = _mm512_set1_ps(1.0005f);
    __m512 accumulator6 = _mm512_set1_ps(1.0006f);
    __m512 accumulator7 = _mm512_set1_ps(1.0007f);
    __m512 fma_multiplier = _mm512_set1_ps(1.0000001f);
    __m512 fma_addend     = _mm512_set1_ps(0.0000001f);

    /* Synchronize all threads to start at the same time */
    pthread_barrier_wait(worker_args->start_barrier);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < worker_args->fma_iterations; i++) {
        accumulator0 = _mm512_fmadd_ps(accumulator0, fma_multiplier, fma_addend);
        accumulator1 = _mm512_fmadd_ps(accumulator1, fma_multiplier, fma_addend);
        accumulator2 = _mm512_fmadd_ps(accumulator2, fma_multiplier, fma_addend);
        accumulator3 = _mm512_fmadd_ps(accumulator3, fma_multiplier, fma_addend);
        accumulator4 = _mm512_fmadd_ps(accumulator4, fma_multiplier, fma_addend);
        accumulator5 = _mm512_fmadd_ps(accumulator5, fma_multiplier, fma_addend);
        accumulator6 = _mm512_fmadd_ps(accumulator6, fma_multiplier, fma_addend);
        accumulator7 = _mm512_fmadd_ps(accumulator7, fma_multiplier, fma_addend);
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(accumulator0) +
                          _mm512_reduce_add_ps(accumulator1) +
                          _mm512_reduce_add_ps(accumulator2) +
                          _mm512_reduce_add_ps(accumulator3) +
                          _mm512_reduce_add_ps(accumulator4) +
                          _mm512_reduce_add_ps(accumulator5) +
                          _mm512_reduce_add_ps(accumulator6) +
                          _mm512_reduce_add_ps(accumulator7);
    (void)sink;

    worker_args->result.core_id = worker_args->core_id;
    worker_args->result.fma_iterations = worker_args->fma_iterations;
    worker_args->result.elapsed_cycles = (double)(t1 - t0);
    worker_args->result.cycles_per_fma = (double)(t1 - t0) / (double)(worker_args->fma_iterations * 8);

    return NULL;
}

static double get_wall_time_seconds(void)
{
    struct timespec wall_clock;
    clock_gettime(CLOCK_MONOTONIC, &wall_clock);
    return wall_clock.tv_sec + wall_clock.tv_nsec * 1e-9;
}

static void run_multicore_test(int num_threads, uint64_t fma_iterations_per_thread)
{
    pthread_t *thread_handles = malloc(num_threads * sizeof(pthread_t));
    WorkerArgs *worker_arguments = malloc(num_threads * sizeof(WorkerArgs));
    pthread_barrier_t start_barrier;

    pthread_barrier_init(&start_barrier, NULL, num_threads);

    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        worker_arguments[thread_index].core_id = thread_index;
        worker_arguments[thread_index].fma_iterations = fma_iterations_per_thread;
        worker_arguments[thread_index].start_barrier = &start_barrier;
    }

    double wall_start = get_wall_time_seconds();

    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        pthread_create(&thread_handles[thread_index], NULL, fma_worker_thread,
                       &worker_arguments[thread_index]);
    }

    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        pthread_join(thread_handles[thread_index], NULL);
    }

    double wall_elapsed = get_wall_time_seconds() - wall_start;

    /* Per-thread results */
    double total_fma_ops = 0;
    double worst_cycles_per_fma = 0;

    printf("\n--- %d thread(s) ---\n", num_threads);
    for (int thread_index = 0; thread_index < num_threads; thread_index++) {
        WorkerResult *thread_result = &worker_arguments[thread_index].result;
        printf("  core %2d: %.4f cyc/fma  (%.0f total cycles)\n",
               thread_result->core_id, thread_result->cycles_per_fma,
               thread_result->elapsed_cycles);
        total_fma_ops += (double)(thread_result->fma_iterations * 8);
        if (thread_result->cycles_per_fma > worst_cycles_per_fma)
            worst_cycles_per_fma = thread_result->cycles_per_fma;
    }

    /* Each FMA on 16 floats = 32 FLOP (16 multiplies + 16 adds) */
    double total_flops = total_fma_ops * 32.0;
    double gflops = total_flops / (wall_elapsed * 1e9);

    printf("  wall_time_sec=%.6f\n", wall_elapsed);
    printf("  worst_cyc_per_fma=%.4f\n", worst_cycles_per_fma);
    printf("  aggregate_GFLOPS=%.2f\n", gflops);
    printf("  per_thread_GFLOPS=%.2f\n", gflops / num_threads);

    pthread_barrier_destroy(&start_barrier);
    free(thread_handles);
    free(worker_arguments);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 500000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();

    printf("probe=avx512_multicore\ncpu_count=16\navx512f=%d\n", feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    static const int thread_counts[] = { 1, 2, 4, 8, 16 };
    int num_configurations = sizeof(thread_counts) / sizeof(thread_counts[0]);

    printf("\nFMA iterations per thread: %lu\n", (unsigned long)cfg.iterations);
    printf("FMAs per iteration: 8 (independent zmm accumulators)\n");
    printf("FLOPs per FMA: 32 (16 mul + 16 add on 512-bit)\n");

    for (int config_index = 0; config_index < num_configurations; config_index++) {
        run_multicore_test(thread_counts[config_index], cfg.iterations);
    }

    if (cfg.csv) {
        printf("\ncsv_header,threads,worst_cyc_per_fma,wall_sec,agg_gflops\n");
        /* Re-run would be expensive; CSV output is advisory in header above */
    }

    return 0;
}
