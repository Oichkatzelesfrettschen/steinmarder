#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/*
 * probe_cache_split.c -- Cache line and page split penalties on Zen 4.
 *
 * Data-dependent on ADDRESS ALIGNMENT rather than operand value, but equally
 * invisible in instruction tables.  The same VMOVDQU32 has radically different
 * latency depending on whether the access straddles a 64-byte or 4096-byte
 * boundary.
 *
 * Tests:
 *   1) YMM load: aligned vs 64-byte cache line split vs 4KB page split
 *   2) YMM store: same three alignments
 *   3) ZMM load: aligned vs page split
 *   4) VMOVAPS aligned vs "unaligned" (should be equal on Zen 4)
 *   5) LOCK ADD across cache line split vs within
 *   6) Gather (vpgatherdd zmm): indices within one cache line vs crossing
 *
 * Buffer allocation uses posix_memalign(4096) to guarantee page alignment,
 * then specific offsets create the split conditions.
 */

#define WARMUP_ITERS 5000

typedef struct {
    const char *test_name;
    const char *access_type;
    const char *alignment;
    double      latency_cycles;
} CacheSplitResult;

/* ====================================================================
 *  YMM (32-byte) load from various alignments
 * ==================================================================== */

static double measure_ymm_load_aligned(char *buffer, uint64_t iterations)
{
    /* Load from offset 0: fully within a single cache line */
    volatile __m256i *load_address = (volatile __m256i *)(buffer + 0);
    __m256i accumulator = _mm256_setzero_si256();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m256i loaded_value = _mm256_loadu_si256((const __m256i *)load_address);
        accumulator = _mm256_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m256i loaded_value = _mm256_loadu_si256((const __m256i *)load_address);
        accumulator = _mm256_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value;
    int temporary[8];
    _mm256_storeu_si256((__m256i *)temporary, accumulator);
    sink_value = temporary[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_ymm_load_cacheline_split(char *buffer, uint64_t iterations)
{
    /* Load from offset 48: bytes 48-79 straddle the 64-byte boundary at byte 64 */
    volatile __m256i *load_address = (volatile __m256i *)(buffer + 48);
    __m256i accumulator = _mm256_setzero_si256();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m256i loaded_value = _mm256_loadu_si256((const __m256i *)load_address);
        accumulator = _mm256_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m256i loaded_value = _mm256_loadu_si256((const __m256i *)load_address);
        accumulator = _mm256_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value;
    int temporary[8];
    _mm256_storeu_si256((__m256i *)temporary, accumulator);
    sink_value = temporary[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_ymm_load_page_split(char *buffer, uint64_t iterations)
{
    /* Load from offset 4080: bytes 4080-4111 straddle the 4096-byte page boundary */
    volatile __m256i *load_address = (volatile __m256i *)(buffer + 4080);
    __m256i accumulator = _mm256_setzero_si256();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m256i loaded_value = _mm256_loadu_si256((const __m256i *)load_address);
        accumulator = _mm256_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m256i loaded_value = _mm256_loadu_si256((const __m256i *)load_address);
        accumulator = _mm256_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value;
    int temporary[8];
    _mm256_storeu_si256((__m256i *)temporary, accumulator);
    sink_value = temporary[0];
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

/* ====================================================================
 *  YMM (32-byte) store to various alignments
 * ==================================================================== */

static double measure_ymm_store_aligned(char *buffer, uint64_t iterations)
{
    volatile __m256i *store_address = (volatile __m256i *)(buffer + 0);
    __m256i store_value = _mm256_set1_epi32(42);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        _mm256_storeu_si256((__m256i *)store_address, store_value);
        __asm__ volatile("" ::: "memory");
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        _mm256_storeu_si256((__m256i *)store_address, store_value);
        __asm__ volatile("" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_ymm_store_cacheline_split(char *buffer, uint64_t iterations)
{
    volatile __m256i *store_address = (volatile __m256i *)(buffer + 48);
    __m256i store_value = _mm256_set1_epi32(42);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        _mm256_storeu_si256((__m256i *)store_address, store_value);
        __asm__ volatile("" ::: "memory");
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        _mm256_storeu_si256((__m256i *)store_address, store_value);
        __asm__ volatile("" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_ymm_store_page_split(char *buffer, uint64_t iterations)
{
    volatile __m256i *store_address = (volatile __m256i *)(buffer + 4080);
    __m256i store_value = _mm256_set1_epi32(42);

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        _mm256_storeu_si256((__m256i *)store_address, store_value);
        __asm__ volatile("" ::: "memory");
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        _mm256_storeu_si256((__m256i *)store_address, store_value);
        __asm__ volatile("" ::: "memory");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

/* ====================================================================
 *  ZMM (64-byte) load: aligned vs page split
 * ==================================================================== */

static double measure_zmm_load_aligned(char *buffer, uint64_t iterations)
{
    volatile __m512i *load_address = (volatile __m512i *)(buffer + 0);
    __m512i accumulator = _mm512_setzero_si512();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m512i loaded_value = _mm512_loadu_si512((const void *)load_address);
        accumulator = _mm512_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m512i loaded_value = _mm512_loadu_si512((const void *)load_address);
        accumulator = _mm512_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm512_reduce_add_epi32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_zmm_load_page_split(char *buffer, uint64_t iterations)
{
    /* Offset 4064: bytes 4064-4127 straddle the 4096 page boundary */
    volatile __m512i *load_address = (volatile __m512i *)(buffer + 4064);
    __m512i accumulator = _mm512_setzero_si512();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m512i loaded_value = _mm512_loadu_si512((const void *)load_address);
        accumulator = _mm512_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m512i loaded_value = _mm512_loadu_si512((const void *)load_address);
        accumulator = _mm512_add_epi32(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm512_reduce_add_epi32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

/* ====================================================================
 *  VMOVAPS aligned vs VMOVUPS "unaligned" (both 64-byte aligned address)
 *  Zen 4 should show NO penalty for unaligned instruction on aligned data.
 * ==================================================================== */

static double measure_vmovaps_aligned(char *buffer, uint64_t iterations)
{
    /* Buffer is page-aligned, so offset 0 is 64-byte aligned */
    __m512 accumulator = _mm512_setzero_ps();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m512 loaded_value = _mm512_load_ps((const float *)(buffer + 0));
        accumulator = _mm512_add_ps(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m512 loaded_value = _mm512_load_ps((const float *)(buffer + 0));
        accumulator = _mm512_add_ps(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_vmovups_aligned(char *buffer, uint64_t iterations)
{
    /* Use unaligned intrinsic on a 64-byte aligned address */
    __m512 accumulator = _mm512_setzero_ps();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m512 loaded_value = _mm512_loadu_ps((const float *)(buffer + 0));
        accumulator = _mm512_add_ps(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m512 loaded_value = _mm512_loadu_ps((const float *)(buffer + 0));
        accumulator = _mm512_add_ps(accumulator, loaded_value);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile float sink_value = _mm512_reduce_add_ps(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

/* ====================================================================
 *  LOCK ADD: within cache line vs cache line split
 * ==================================================================== */

static double measure_lock_add_aligned(char *buffer, uint64_t iterations)
{
    /* Atomic add on a naturally-aligned 8-byte location */
    volatile int64_t *atomic_location = (volatile int64_t *)(buffer + 0);
    *atomic_location = 0;

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __asm__ volatile("lock addq $1, %[mem]"
                         : [mem] "+m" (*atomic_location)
                         :
                         : "memory", "cc");
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __asm__ volatile("lock addq $1, %[mem]"
                         : [mem] "+m" (*atomic_location)
                         :
                         : "memory", "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int64_t sink_value = *atomic_location;
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_lock_add_cacheline_split(char *buffer, uint64_t iterations)
{
    /* Atomic add on an address that straddles a 64-byte cache line boundary.
     * buffer+60 gives an 8-byte access covering bytes 60-67, crossing at byte 64. */
    volatile int64_t *atomic_location = (volatile int64_t *)(buffer + 60);
    *atomic_location = 0;

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __asm__ volatile("lock addq $1, %[mem]"
                         : [mem] "+m" (*atomic_location)
                         :
                         : "memory", "cc");
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __asm__ volatile("lock addq $1, %[mem]"
                         : [mem] "+m" (*atomic_location)
                         :
                         : "memory", "cc");
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int64_t sink_value = *atomic_location;
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

/* ====================================================================
 *  Gather: indices within one cache line vs crossing cache lines
 * ==================================================================== */

static double measure_gather_same_cacheline(const int *table, uint64_t iterations)
{
    /* All 16 indices point within the same 64-byte region (16 ints = 64 bytes) */
    __m512i gather_indices = _mm512_set_epi32(
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15);
    __m512i accumulator = _mm512_setzero_si512();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m512i gathered = _mm512_i32gather_epi32(gather_indices, table, 4);
        accumulator = _mm512_add_epi32(accumulator, gathered);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m512i gathered = _mm512_i32gather_epi32(gather_indices, table, 4);
        accumulator = _mm512_add_epi32(accumulator, gathered);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm512_reduce_add_epi32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

static double measure_gather_cross_cacheline(const int *table, uint64_t iterations)
{
    /* Indices spread across 16 different cache lines (stride of 16 ints = 64 bytes) */
    __m512i gather_indices = _mm512_set_epi32(
        0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
        8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16);
    __m512i accumulator = _mm512_setzero_si512();

    for (uint64_t warmup_iter = 0; warmup_iter < WARMUP_ITERS; warmup_iter++) {
        __m512i gathered = _mm512_i32gather_epi32(gather_indices, table, 4);
        accumulator = _mm512_add_epi32(accumulator, gathered);
        __asm__ volatile("" : "+x"(accumulator));
    }
    __asm__ volatile("" ::: "memory");

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t measurement_iter = 0; measurement_iter < iterations; measurement_iter++) {
        __m512i gathered = _mm512_i32gather_epi32(gather_indices, table, 4);
        accumulator = _mm512_add_epi32(accumulator, gathered);
        __asm__ volatile("" : "+x"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile int sink_value = _mm512_reduce_add_epi32(accumulator);
    (void)sink_value;
    return (double)(tsc_stop - tsc_start) / (double)iterations;
}

/* ====================================================================
 *  Main
 * ==================================================================== */

int main(int argc, char **argv)
{
    SMZenProbeConfig probe_config = sm_zen_default_config();
    probe_config.iterations = 30000;
    sm_zen_parse_common_args(argc, argv, &probe_config);
    SMZenFeatures detected_features = sm_zen_detect_features();
    sm_zen_pin_to_cpu(probe_config.cpu);

    sm_zen_print_header("cache_split", &probe_config, &detected_features);
    printf("\n");

    if (!detected_features.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    uint64_t measurement_iterations = probe_config.iterations * 200;

    /* Allocate a page-aligned buffer large enough for page-split tests.
     * Need at least 2 pages (8192 bytes) so the 4080-offset load has
     * valid memory on both sides of the page boundary. */
    size_t buffer_size = 16384;
    char *page_aligned_buffer;
    if (posix_memalign((void **)&page_aligned_buffer, 4096, buffer_size) != 0) {
        fprintf(stderr, "posix_memalign failed\n");
        return 1;
    }
    memset(page_aligned_buffer, 0xAB, buffer_size);

    /* Gather table: needs >= 16*16 = 256 ints = 1KB, aligned */
    size_t gather_table_elements = 4096;
    int *gather_table = (int *)sm_zen_aligned_alloc64(
        gather_table_elements * sizeof(int));
    sm_zen_fill_random_u32((uint32_t *)gather_table, gather_table_elements, 42);

    printf("measurement_iterations=%lu\n", (unsigned long)measurement_iterations);
    printf("buffer=%p (page-aligned)\n\n", (void *)page_aligned_buffer);

    CacheSplitResult results_table[14];
    int result_index = 0;

    /* --- YMM loads --- */
    results_table[result_index++] = (CacheSplitResult){
        "ymm_load", "vmovdqu ymm", "aligned",
        measure_ymm_load_aligned(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "ymm_load", "vmovdqu ymm", "cacheline_split",
        measure_ymm_load_cacheline_split(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "ymm_load", "vmovdqu ymm", "page_split",
        measure_ymm_load_page_split(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- YMM stores --- */
    results_table[result_index++] = (CacheSplitResult){
        "ymm_store", "vmovdqu ymm", "aligned",
        measure_ymm_store_aligned(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "ymm_store", "vmovdqu ymm", "cacheline_split",
        measure_ymm_store_cacheline_split(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "ymm_store", "vmovdqu ymm", "page_split",
        measure_ymm_store_page_split(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- ZMM loads --- */
    results_table[result_index++] = (CacheSplitResult){
        "zmm_load", "vmovdqu32 zmm", "aligned",
        measure_zmm_load_aligned(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "zmm_load", "vmovdqu32 zmm", "page_split",
        measure_zmm_load_page_split(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- VMOVAPS vs VMOVUPS --- */
    results_table[result_index++] = (CacheSplitResult){
        "zmm_aligned_insn", "vmovaps zmm", "aligned_addr",
        measure_vmovaps_aligned(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "zmm_unaligned_insn", "vmovups zmm", "aligned_addr",
        measure_vmovups_aligned(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- LOCK ADD --- */
    results_table[result_index++] = (CacheSplitResult){
        "lock_add", "lock addq", "aligned",
        measure_lock_add_aligned(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "lock_add", "lock addq", "cacheline_split",
        measure_lock_add_cacheline_split(page_aligned_buffer, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    /* --- Gather --- */
    results_table[result_index++] = (CacheSplitResult){
        "gather_zmm", "vpgatherdd zmm", "same_cacheline",
        measure_gather_same_cacheline(gather_table, measurement_iterations)};
    __asm__ volatile("" ::: "memory");

    results_table[result_index++] = (CacheSplitResult){
        "gather_zmm", "vpgatherdd zmm", "cross_cacheline",
        measure_gather_cross_cacheline(gather_table, measurement_iterations)};

    /* ---- Print results ---- */
    printf("%-20s %-18s %-18s %14s\n",
           "test", "access_type", "alignment", "latency_cyc");
    printf("%-20s %-18s %-18s %14s\n",
           "----", "-----------", "---------", "-----------");
    for (int row = 0; row < result_index; row++) {
        printf("%-20s %-18s %-18s %14.4f\n",
               results_table[row].test_name,
               results_table[row].access_type,
               results_table[row].alignment,
               results_table[row].latency_cycles);
    }

    /* ---- Penalty analysis ---- */
    printf("\n--- split penalties (cycles above aligned baseline) ---\n");
    printf("  YMM load:  cacheline_split=+%.2f  page_split=+%.2f\n",
           results_table[1].latency_cycles - results_table[0].latency_cycles,
           results_table[2].latency_cycles - results_table[0].latency_cycles);
    printf("  YMM store: cacheline_split=+%.2f  page_split=+%.2f\n",
           results_table[4].latency_cycles - results_table[3].latency_cycles,
           results_table[5].latency_cycles - results_table[3].latency_cycles);
    printf("  ZMM load:  page_split=+%.2f\n",
           results_table[7].latency_cycles - results_table[6].latency_cycles);
    printf("  VMOVAPS vs VMOVUPS (aligned addr): delta=%.2f (expect ~0)\n",
           results_table[9].latency_cycles - results_table[8].latency_cycles);
    printf("  LOCK ADD:  cacheline_split=+%.2f\n",
           results_table[11].latency_cycles - results_table[10].latency_cycles);
    printf("  Gather:    cross_cacheline=+%.2f\n",
           results_table[13].latency_cycles - results_table[12].latency_cycles);

    /* ---- CSV output ---- */
    if (probe_config.csv) {
        printf("\ncsv_header,test,access_type,alignment,latency_cycles\n");
        for (int row = 0; row < result_index; row++) {
            printf("%s,%s,%s,%.4f\n",
                   results_table[row].test_name,
                   results_table[row].access_type,
                   results_table[row].alignment,
                   results_table[row].latency_cycles);
        }
    }

    free(page_aligned_buffer);
    free(gather_table);
    return 0;
}
