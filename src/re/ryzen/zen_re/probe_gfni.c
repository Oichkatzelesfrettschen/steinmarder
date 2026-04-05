#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_gfni.c -- Galois Field New Instructions on Zen 4.
 *
 * Measures vgf2p8mulb, vgf2p8affineqb, vgf2p8affineinvqb at zmm width.
 * vgf2p8affineqb can replace ANY 8-bit LUT with a single instruction —
 * extremely powerful for crypto, error correction, and bit-level transforms.
 */

/* ── vgf2p8mulb zmm (GF(2^8) byte multiply) ── */

static double gf2p8mulb_zmm_latency(uint64_t iterations)
{
    __m512i accumulator = _mm512_set1_epi8(0x53);
    __m512i operand_b   = _mm512_set1_epi8(0x1B);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_gf2p8mul_epi8(accumulator, operand_b);
        accumulator = _mm512_gf2p8mul_epi8(accumulator, operand_b);
        accumulator = _mm512_gf2p8mul_epi8(accumulator, operand_b);
        accumulator = _mm512_gf2p8mul_epi8(accumulator, operand_b);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double gf2p8mulb_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi8(0x01);
    __m512i acc1 = _mm512_set1_epi8(0x02);
    __m512i acc2 = _mm512_set1_epi8(0x03);
    __m512i acc3 = _mm512_set1_epi8(0x04);
    __m512i acc4 = _mm512_set1_epi8(0x05);
    __m512i acc5 = _mm512_set1_epi8(0x06);
    __m512i acc6 = _mm512_set1_epi8(0x07);
    __m512i acc7 = _mm512_set1_epi8(0x08);
    __m512i operand_b = _mm512_set1_epi8(0x1B);
    __asm__ volatile("" : "+x"(operand_b));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_gf2p8mul_epi8(acc0, operand_b);
        acc1 = _mm512_gf2p8mul_epi8(acc1, operand_b);
        acc2 = _mm512_gf2p8mul_epi8(acc2, operand_b);
        acc3 = _mm512_gf2p8mul_epi8(acc3, operand_b);
        acc4 = _mm512_gf2p8mul_epi8(acc4, operand_b);
        acc5 = _mm512_gf2p8mul_epi8(acc5, operand_b);
        acc6 = _mm512_gf2p8mul_epi8(acc6, operand_b);
        acc7 = _mm512_gf2p8mul_epi8(acc7, operand_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── vgf2p8affineqb zmm (GF(2^8) affine transformation) ── */

static double gf2p8affineqb_zmm_latency(uint64_t iterations)
{
    __m512i accumulator    = _mm512_set1_epi64(0xDEADBEEFCAFEBABE);
    __m512i affine_matrix  = _mm512_set1_epi64(0x8040201008040201ULL); /* identity matrix */
    __asm__ volatile("" : "+x"(affine_matrix));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_gf2p8affine_epi64_epi8(accumulator, affine_matrix, 0);
        accumulator = _mm512_gf2p8affine_epi64_epi8(accumulator, affine_matrix, 0);
        accumulator = _mm512_gf2p8affine_epi64_epi8(accumulator, affine_matrix, 0);
        accumulator = _mm512_gf2p8affine_epi64_epi8(accumulator, affine_matrix, 0);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double gf2p8affineqb_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(0x0102030405060708ULL);
    __m512i acc1 = _mm512_set1_epi64(0x1112131415161718ULL);
    __m512i acc2 = _mm512_set1_epi64(0x2122232425262728ULL);
    __m512i acc3 = _mm512_set1_epi64(0x3132333435363738ULL);
    __m512i acc4 = _mm512_set1_epi64(0x4142434445464748ULL);
    __m512i acc5 = _mm512_set1_epi64(0x5152535455565758ULL);
    __m512i acc6 = _mm512_set1_epi64(0x6162636465666768ULL);
    __m512i acc7 = _mm512_set1_epi64(0x7172737475767778ULL);
    __m512i affine_matrix = _mm512_set1_epi64(0x8040201008040201ULL);
    __asm__ volatile("" : "+x"(affine_matrix));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_gf2p8affine_epi64_epi8(acc0, affine_matrix, 0);
        acc1 = _mm512_gf2p8affine_epi64_epi8(acc1, affine_matrix, 0);
        acc2 = _mm512_gf2p8affine_epi64_epi8(acc2, affine_matrix, 0);
        acc3 = _mm512_gf2p8affine_epi64_epi8(acc3, affine_matrix, 0);
        acc4 = _mm512_gf2p8affine_epi64_epi8(acc4, affine_matrix, 0);
        acc5 = _mm512_gf2p8affine_epi64_epi8(acc5, affine_matrix, 0);
        acc6 = _mm512_gf2p8affine_epi64_epi8(acc6, affine_matrix, 0);
        acc7 = _mm512_gf2p8affine_epi64_epi8(acc7, affine_matrix, 0);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(acc0) +
                              _mm512_reduce_add_epi64(acc4);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* ── vgf2p8affineinvqb zmm (GF(2^8) affine + GF inverse) ── */

static double gf2p8affineinvqb_zmm_latency(uint64_t iterations)
{
    __m512i accumulator    = _mm512_set1_epi64(0xDEADBEEFCAFEBABE);
    __m512i affine_matrix  = _mm512_set1_epi64(0x8040201008040201ULL);
    __asm__ volatile("" : "+x"(affine_matrix));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        accumulator = _mm512_gf2p8affineinv_epi64_epi8(accumulator, affine_matrix, 0);
        accumulator = _mm512_gf2p8affineinv_epi64_epi8(accumulator, affine_matrix, 0);
        accumulator = _mm512_gf2p8affineinv_epi64_epi8(accumulator, affine_matrix, 0);
        accumulator = _mm512_gf2p8affineinv_epi64_epi8(accumulator, affine_matrix, 0);
    }
    uint64_t tsc_stop = sm_zen_tsc_end();

    volatile long long sink = _mm512_reduce_add_epi64(accumulator);
    (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double gf2p8affineinvqb_zmm_throughput(uint64_t iterations)
{
    __m512i acc0 = _mm512_set1_epi64(0x0102030405060708ULL);
    __m512i acc1 = _mm512_set1_epi64(0x1112131415161718ULL);
    __m512i acc2 = _mm512_set1_epi64(0x2122232425262728ULL);
    __m512i acc3 = _mm512_set1_epi64(0x3132333435363738ULL);
    __m512i acc4 = _mm512_set1_epi64(0x4142434445464748ULL);
    __m512i acc5 = _mm512_set1_epi64(0x5152535455565758ULL);
    __m512i acc6 = _mm512_set1_epi64(0x6162636465666768ULL);
    __m512i acc7 = _mm512_set1_epi64(0x7172737475767778ULL);
    __m512i affine_matrix = _mm512_set1_epi64(0x8040201008040201ULL);
    __asm__ volatile("" : "+x"(affine_matrix));

    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_gf2p8affineinv_epi64_epi8(acc0, affine_matrix, 0);
        acc1 = _mm512_gf2p8affineinv_epi64_epi8(acc1, affine_matrix, 0);
        acc2 = _mm512_gf2p8affineinv_epi64_epi8(acc2, affine_matrix, 0);
        acc3 = _mm512_gf2p8affineinv_epi64_epi8(acc3, affine_matrix, 0);
        acc4 = _mm512_gf2p8affineinv_epi64_epi8(acc4, affine_matrix, 0);
        acc5 = _mm512_gf2p8affineinv_epi64_epi8(acc5, affine_matrix, 0);
        acc6 = _mm512_gf2p8affineinv_epi64_epi8(acc6, affine_matrix, 0);
        acc7 = _mm512_gf2p8affineinv_epi64_epi8(acc7, affine_matrix, 0);
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

    sm_zen_print_header("gfni", &cfg, &feat);
    printf("\n");

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported on this CPU\n");
        return 1;
    }

    printf("--- vgf2p8mulb zmm (GF(2^8) multiply, 64 bytes/instr) ---\n");
    double mulb_latency    = gf2p8mulb_zmm_latency(cfg.iterations);
    double mulb_throughput  = gf2p8mulb_zmm_throughput(cfg.iterations);
    printf("gf2p8mulb_zmm_latency_cycles=%.4f\n", mulb_latency);
    printf("gf2p8mulb_zmm_throughput_cycles=%.4f\n", mulb_throughput);
    printf("gf2p8mulb_zmm_ops_per_cycle=%.2f\n\n", 1.0 / mulb_throughput);

    printf("--- vgf2p8affineqb zmm (affine transform, replaces 8-bit LUT) ---\n");
    double affine_latency    = gf2p8affineqb_zmm_latency(cfg.iterations);
    double affine_throughput  = gf2p8affineqb_zmm_throughput(cfg.iterations);
    printf("gf2p8affineqb_zmm_latency_cycles=%.4f\n", affine_latency);
    printf("gf2p8affineqb_zmm_throughput_cycles=%.4f\n", affine_throughput);
    printf("gf2p8affineqb_zmm_ops_per_cycle=%.2f\n\n", 1.0 / affine_throughput);

    printf("--- vgf2p8affineinvqb zmm (affine + GF inverse) ---\n");
    double affineinv_latency    = gf2p8affineinvqb_zmm_latency(cfg.iterations);
    double affineinv_throughput  = gf2p8affineinvqb_zmm_throughput(cfg.iterations);
    printf("gf2p8affineinvqb_zmm_latency_cycles=%.4f\n", affineinv_latency);
    printf("gf2p8affineinvqb_zmm_throughput_cycles=%.4f\n", affineinv_throughput);
    printf("gf2p8affineinvqb_zmm_ops_per_cycle=%.2f\n\n", 1.0 / affineinv_throughput);

    if (cfg.csv)
        printf("csv,probe=gfni,"
               "mulb_lat=%.4f,mulb_tp=%.4f,"
               "affine_lat=%.4f,affine_tp=%.4f,"
               "affineinv_lat=%.4f,affineinv_tp=%.4f\n",
               mulb_latency, mulb_throughput,
               affine_latency, affine_throughput,
               affineinv_latency, affineinv_throughput);
    return 0;
}
