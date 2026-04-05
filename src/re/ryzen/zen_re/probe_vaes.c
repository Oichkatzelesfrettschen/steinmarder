#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_vaes.c -- Vectorized AES (AES-NI + VAES) on Zen 4.
 *
 * Measures vaesenc/vaesdec/vaesimc/vaesenclast across xmm/ymm/zmm widths.
 * Key question: does Zen 4 achieve 1-cycle throughput for zmm (like Intel),
 * or does the 256-bit double-pump impose a cost on 512-bit AES?
 */

/* ── AES-NI xmm (128-bit, 1 block) ── */

static double vaesenc_xmm_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set1_epi64x(0xDEADBEEFCAFEBABE);
    __m128i round_key   = _mm_set1_epi64x(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_aesenc_si128(accumulator, round_key);
        accumulator = _mm_aesenc_si128(accumulator, round_key);
        accumulator = _mm_aesenc_si128(accumulator, round_key);
        accumulator = _mm_aesenc_si128(accumulator, round_key);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vaesenc_xmm_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi64x(1);
    __m128i acc1 = _mm_set1_epi64x(2);
    __m128i acc2 = _mm_set1_epi64x(3);
    __m128i acc3 = _mm_set1_epi64x(4);
    __m128i acc4 = _mm_set1_epi64x(5);
    __m128i acc5 = _mm_set1_epi64x(6);
    __m128i acc6 = _mm_set1_epi64x(7);
    __m128i acc7 = _mm_set1_epi64x(8);
    __m128i round_key = _mm_set1_epi64x(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_aesenc_si128(acc0, round_key);
        acc1 = _mm_aesenc_si128(acc1, round_key);
        acc2 = _mm_aesenc_si128(acc2, round_key);
        acc3 = _mm_aesenc_si128(acc3, round_key);
        acc4 = _mm_aesenc_si128(acc4, round_key);
        acc5 = _mm_aesenc_si128(acc5, round_key);
        acc6 = _mm_aesenc_si128(acc6, round_key);
        acc7 = _mm_aesenc_si128(acc7, round_key);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── VAES ymm (256-bit, 2 blocks) ── */

static double vaesenc_ymm_latency(uint64_t iterations)
{
    __m256i accumulator = _mm256_set1_epi64x(0xDEADBEEFCAFEBABE);
    __m256i round_key   = _mm256_set1_epi64x(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm256_aesenc_epi128(accumulator, round_key);
        accumulator = _mm256_aesenc_epi128(accumulator, round_key);
        accumulator = _mm256_aesenc_epi128(accumulator, round_key);
        accumulator = _mm256_aesenc_epi128(accumulator, round_key);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm256_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vaesenc_ymm_throughput(uint64_t iterations)
{
    __m256i acc0 = _mm256_set1_epi64x(1);
    __m256i acc1 = _mm256_set1_epi64x(2);
    __m256i acc2 = _mm256_set1_epi64x(3);
    __m256i acc3 = _mm256_set1_epi64x(4);
    __m256i acc4 = _mm256_set1_epi64x(5);
    __m256i acc5 = _mm256_set1_epi64x(6);
    __m256i acc6 = _mm256_set1_epi64x(7);
    __m256i acc7 = _mm256_set1_epi64x(8);
    __m256i round_key = _mm256_set1_epi64x(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm256_aesenc_epi128(acc0, round_key);
        acc1 = _mm256_aesenc_epi128(acc1, round_key);
        acc2 = _mm256_aesenc_epi128(acc2, round_key);
        acc3 = _mm256_aesenc_epi128(acc3, round_key);
        acc4 = _mm256_aesenc_epi128(acc4, round_key);
        acc5 = _mm256_aesenc_epi128(acc5, round_key);
        acc6 = _mm256_aesenc_epi128(acc6, round_key);
        acc7 = _mm256_aesenc_epi128(acc7, round_key);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm256_extract_epi64(acc0, 0) + _mm256_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── VAES zmm (512-bit, 4 blocks) ── */

static double vaesenc_zmm_latency(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi64(0xDEADBEEFCAFEBABE);
    __m512i round_key   = _mm512_set1_epi64(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_aesenc_epi128(accumulator, round_key);
        accumulator = _mm512_aesenc_epi128(accumulator, round_key);
        accumulator = _mm512_aesenc_epi128(accumulator, round_key);
        accumulator = _mm512_aesenc_epi128(accumulator, round_key);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vaesenc_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(1);
    __m512i acc1 = _mm512_set1_epi64(2);
    __m512i acc2 = _mm512_set1_epi64(3);
    __m512i acc3 = _mm512_set1_epi64(4);
    __m512i acc4 = _mm512_set1_epi64(5);
    __m512i acc5 = _mm512_set1_epi64(6);
    __m512i acc6 = _mm512_set1_epi64(7);
    __m512i acc7 = _mm512_set1_epi64(8);
    __m512i round_key = _mm512_set1_epi64(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_aesenc_epi128(acc0, round_key);
        acc1 = _mm512_aesenc_epi128(acc1, round_key);
        acc2 = _mm512_aesenc_epi128(acc2, round_key);
        acc3 = _mm512_aesenc_epi128(acc3, round_key);
        acc4 = _mm512_aesenc_epi128(acc4, round_key);
        acc5 = _mm512_aesenc_epi128(acc5, round_key);
        acc6 = _mm512_aesenc_epi128(acc6, round_key);
        acc7 = _mm512_aesenc_epi128(acc7, round_key);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── vaesdec zmm (decrypt) ── */

static double vaesdec_zmm_latency(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi64(0xDEADBEEFCAFEBABE);
    __m512i round_key   = _mm512_set1_epi64(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_aesdec_epi128(accumulator, round_key);
        accumulator = _mm512_aesdec_epi128(accumulator, round_key);
        accumulator = _mm512_aesdec_epi128(accumulator, round_key);
        accumulator = _mm512_aesdec_epi128(accumulator, round_key);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vaesdec_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(1);
    __m512i acc1 = _mm512_set1_epi64(2);
    __m512i acc2 = _mm512_set1_epi64(3);
    __m512i acc3 = _mm512_set1_epi64(4);
    __m512i acc4 = _mm512_set1_epi64(5);
    __m512i acc5 = _mm512_set1_epi64(6);
    __m512i acc6 = _mm512_set1_epi64(7);
    __m512i acc7 = _mm512_set1_epi64(8);
    __m512i round_key = _mm512_set1_epi64(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_aesdec_epi128(acc0, round_key);
        acc1 = _mm512_aesdec_epi128(acc1, round_key);
        acc2 = _mm512_aesdec_epi128(acc2, round_key);
        acc3 = _mm512_aesdec_epi128(acc3, round_key);
        acc4 = _mm512_aesdec_epi128(acc4, round_key);
        acc5 = _mm512_aesdec_epi128(acc5, round_key);
        acc6 = _mm512_aesdec_epi128(acc6, round_key);
        acc7 = _mm512_aesdec_epi128(acc7, round_key);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── aesimc xmm (inverse mix columns — AES-NI, 128-bit only) ── */

static double vaesimc_xmm_latency(uint64_t iterations)
{
    __m128i accumulator = _mm_set1_epi64x(0xDEADBEEFCAFEBABE);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm_aesimc_si128(accumulator);
        accumulator = _mm_aesimc_si128(accumulator);
        accumulator = _mm_aesimc_si128(accumulator);
        accumulator = _mm_aesimc_si128(accumulator);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(accumulator, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vaesimc_xmm_throughput(uint64_t iterations)
{
    __m128i acc0 = _mm_set1_epi64x(1);
    __m128i acc1 = _mm_set1_epi64x(2);
    __m128i acc2 = _mm_set1_epi64x(3);
    __m128i acc3 = _mm_set1_epi64x(4);
    __m128i acc4 = _mm_set1_epi64x(5);
    __m128i acc5 = _mm_set1_epi64x(6);
    __m128i acc6 = _mm_set1_epi64x(7);
    __m128i acc7 = _mm_set1_epi64x(8);

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm_aesimc_si128(acc0);
        acc1 = _mm_aesimc_si128(acc1);
        acc2 = _mm_aesimc_si128(acc2);
        acc3 = _mm_aesimc_si128(acc3);
        acc4 = _mm_aesimc_si128(acc4);
        acc5 = _mm_aesimc_si128(acc5);
        acc6 = _mm_aesimc_si128(acc6);
        acc7 = _mm_aesimc_si128(acc7);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm_extract_epi64(acc0, 0) + _mm_extract_epi64(acc4, 0);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── vaesenclast zmm (final round encrypt) ── */

static double vaesenclast_zmm_latency(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi64(0xDEADBEEFCAFEBABE);
    __m512i round_key   = _mm512_set1_epi64(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_aesenclast_epi128(accumulator, round_key);
        accumulator = _mm512_aesenclast_epi128(accumulator, round_key);
        accumulator = _mm512_aesenclast_epi128(accumulator, round_key);
        accumulator = _mm512_aesenclast_epi128(accumulator, round_key);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double vaesenclast_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(1);
    __m512i acc1 = _mm512_set1_epi64(2);
    __m512i acc2 = _mm512_set1_epi64(3);
    __m512i acc3 = _mm512_set1_epi64(4);
    __m512i acc4 = _mm512_set1_epi64(5);
    __m512i acc5 = _mm512_set1_epi64(6);
    __m512i acc6 = _mm512_set1_epi64(7);
    __m512i acc7 = _mm512_set1_epi64(8);
    __m512i round_key = _mm512_set1_epi64(0x0123456789ABCDEF);
    __asm__ volatile("" : "+x"(round_key));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_aesenclast_epi128(acc0, round_key);
        acc1 = _mm512_aesenclast_epi128(acc1, round_key);
        acc2 = _mm512_aesenclast_epi128(acc2, round_key);
        acc3 = _mm512_aesenclast_epi128(acc3, round_key);
        acc4 = _mm512_aesenclast_epi128(acc4, round_key);
        acc5 = _mm512_aesenclast_epi128(acc5, round_key);
        acc6 = _mm512_aesenclast_epi128(acc6, round_key);
        acc7 = _mm512_aesenclast_epi128(acc7, round_key);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    sm_zen_print_header("vaes", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported on this CPU\n");
        return 1;
    }

    printf("--- AES-NI xmm (128-bit, 1 block/instr) ---\n");
    double xmm_enc_latency    = vaesenc_xmm_latency(cfg.iterations);
    double xmm_enc_throughput  = vaesenc_xmm_throughput(cfg.iterations);
    printf("vaesenc_xmm_latency_cycles=%.4f\n", xmm_enc_latency);
    printf("vaesenc_xmm_throughput_cycles=%.4f\n", xmm_enc_throughput);
    printf("vaesenc_xmm_blocks_per_cycle=%.2f\n\n", 1.0 / xmm_enc_throughput);

    printf("--- VAES ymm (256-bit, 2 blocks/instr) ---\n");
    double ymm_enc_latency    = vaesenc_ymm_latency(cfg.iterations);
    double ymm_enc_throughput  = vaesenc_ymm_throughput(cfg.iterations);
    printf("vaesenc_ymm_latency_cycles=%.4f\n", ymm_enc_latency);
    printf("vaesenc_ymm_throughput_cycles=%.4f\n", ymm_enc_throughput);
    printf("vaesenc_ymm_blocks_per_cycle=%.2f\n\n", 2.0 / ymm_enc_throughput);

    printf("--- VAES zmm (512-bit, 4 blocks/instr) ---\n");
    double zmm_enc_latency    = vaesenc_zmm_latency(cfg.iterations);
    double zmm_enc_throughput  = vaesenc_zmm_throughput(cfg.iterations);
    printf("vaesenc_zmm_latency_cycles=%.4f\n", zmm_enc_latency);
    printf("vaesenc_zmm_throughput_cycles=%.4f\n", zmm_enc_throughput);
    printf("vaesenc_zmm_blocks_per_cycle=%.2f\n\n", 4.0 / zmm_enc_throughput);

    printf("--- vaesdec zmm (512-bit decrypt) ---\n");
    double zmm_dec_latency    = vaesdec_zmm_latency(cfg.iterations);
    double zmm_dec_throughput  = vaesdec_zmm_throughput(cfg.iterations);
    printf("vaesdec_zmm_latency_cycles=%.4f\n", zmm_dec_latency);
    printf("vaesdec_zmm_throughput_cycles=%.4f\n", zmm_dec_throughput);
    printf("vaesdec_zmm_blocks_per_cycle=%.2f\n\n", 4.0 / zmm_dec_throughput);

    printf("--- aesimc xmm (inverse mix columns, 128-bit only) ---\n");
    double imc_latency    = vaesimc_xmm_latency(cfg.iterations);
    double imc_throughput  = vaesimc_xmm_throughput(cfg.iterations);
    printf("aesimc_xmm_latency_cycles=%.4f\n", imc_latency);
    printf("aesimc_xmm_throughput_cycles=%.4f\n\n", imc_throughput);

    printf("--- vaesenclast zmm (final round, 512-bit) ---\n");
    double last_latency    = vaesenclast_zmm_latency(cfg.iterations);
    double last_throughput  = vaesenclast_zmm_throughput(cfg.iterations);
    printf("vaesenclast_zmm_latency_cycles=%.4f\n", last_latency);
    printf("vaesenclast_zmm_throughput_cycles=%.4f\n", last_throughput);
    printf("vaesenclast_zmm_blocks_per_cycle=%.2f\n\n", 4.0 / last_throughput);

    printf("--- double-pump analysis ---\n");
    printf("zmm_vs_xmm_throughput_ratio=%.2fx (ideal=1.0x for no double-pump cost)\n",
           zmm_enc_throughput / xmm_enc_throughput);
    printf("zmm_vs_ymm_throughput_ratio=%.2fx\n",
           zmm_enc_throughput / ymm_enc_throughput);

    if (cfg.csv)
        printf("csv,probe=vaes,"
               "xmm_enc_lat=%.4f,xmm_enc_tp=%.4f,"
               "ymm_enc_lat=%.4f,ymm_enc_tp=%.4f,"
               "zmm_enc_lat=%.4f,zmm_enc_tp=%.4f,"
               "zmm_dec_lat=%.4f,zmm_dec_tp=%.4f,"
               "imc_lat=%.4f,imc_tp=%.4f,"
               "last_lat=%.4f,last_tp=%.4f\n",
               xmm_enc_latency, xmm_enc_throughput,
               ymm_enc_latency, ymm_enc_throughput,
               zmm_enc_latency, zmm_enc_throughput,
               zmm_dec_latency, zmm_dec_throughput,
               imc_latency, imc_throughput,
               last_latency, last_throughput);
    return 0;
}
