#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_mask.c — AVX-512 masked operation throughput on Zen 4.
 *
 * Measures:
 *   1. vfmadd231ps zmm {k1} — masked FMA (zero-masking)
 *   2. vblendmps zmm {k1}  — masked blend
 *   3. kand/kor/knot k      — mask register arithmetic
 *   4. Unmasked FMA baseline for comparison
 *
 * Masked ops are needed for:
 *   - W0 layer: 27 inputs (not divisible by 16), last ZMM needs a mask
 *   - W_out layer: only 4 outputs, needs a mask to avoid writing garbage
 */

/* --- Unmasked FMA throughput baseline --- */
static double fma_unmasked_throughput(uint64_t iterations)
{
    __m512 acc0 = _mm512_set1_ps(1.0f);
    __m512 acc1 = _mm512_set1_ps(1.0001f);
    __m512 acc2 = _mm512_set1_ps(1.0002f);
    __m512 acc3 = _mm512_set1_ps(1.0003f);
    __m512 acc4 = _mm512_set1_ps(1.0004f);
    __m512 acc5 = _mm512_set1_ps(1.0005f);
    __m512 acc6 = _mm512_set1_ps(1.0006f);
    __m512 acc7 = _mm512_set1_ps(1.0007f);
    __m512 mul_operand = _mm512_set1_ps(1.0000001f);
    __m512 add_operand = _mm512_set1_ps(0.0000001f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_fmadd_ps(acc0, mul_operand, add_operand);
        acc1 = _mm512_fmadd_ps(acc1, mul_operand, add_operand);
        acc2 = _mm512_fmadd_ps(acc2, mul_operand, add_operand);
        acc3 = _mm512_fmadd_ps(acc3, mul_operand, add_operand);
        acc4 = _mm512_fmadd_ps(acc4, mul_operand, add_operand);
        acc5 = _mm512_fmadd_ps(acc5, mul_operand, add_operand);
        acc6 = _mm512_fmadd_ps(acc6, mul_operand, add_operand);
        acc7 = _mm512_fmadd_ps(acc7, mul_operand, add_operand);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc0) + _mm512_reduce_add_ps(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- Masked FMA throughput (zero-masking, k1 = 0x07FF = 11 of 16 lanes) --- */
static double fma_masked_throughput(uint64_t iterations)
{
    /* Mask simulating W0 tail: 27 mod 16 = 11 active lanes */
    __mmask16 tail_mask = (__mmask16)0x07FF;

    __m512 acc0 = _mm512_set1_ps(1.0f);
    __m512 acc1 = _mm512_set1_ps(1.0001f);
    __m512 acc2 = _mm512_set1_ps(1.0002f);
    __m512 acc3 = _mm512_set1_ps(1.0003f);
    __m512 acc4 = _mm512_set1_ps(1.0004f);
    __m512 acc5 = _mm512_set1_ps(1.0005f);
    __m512 acc6 = _mm512_set1_ps(1.0006f);
    __m512 acc7 = _mm512_set1_ps(1.0007f);
    __m512 mul_operand = _mm512_set1_ps(1.0000001f);
    __m512 add_operand = _mm512_set1_ps(0.0000001f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_maskz_fmadd_ps(tail_mask, acc0, mul_operand, add_operand);
        acc1 = _mm512_maskz_fmadd_ps(tail_mask, acc1, mul_operand, add_operand);
        acc2 = _mm512_maskz_fmadd_ps(tail_mask, acc2, mul_operand, add_operand);
        acc3 = _mm512_maskz_fmadd_ps(tail_mask, acc3, mul_operand, add_operand);
        acc4 = _mm512_maskz_fmadd_ps(tail_mask, acc4, mul_operand, add_operand);
        acc5 = _mm512_maskz_fmadd_ps(tail_mask, acc5, mul_operand, add_operand);
        acc6 = _mm512_maskz_fmadd_ps(tail_mask, acc6, mul_operand, add_operand);
        acc7 = _mm512_maskz_fmadd_ps(tail_mask, acc7, mul_operand, add_operand);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc0) + _mm512_reduce_add_ps(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- Masked FMA with narrow mask (k1 = 0x000F = 4 lanes, W_out scenario) --- */
static double fma_narrow_mask_throughput(uint64_t iterations)
{
    __mmask16 narrow_mask = (__mmask16)0x000F; /* Only 4 output lanes */

    __m512 acc0 = _mm512_set1_ps(1.0f);
    __m512 acc1 = _mm512_set1_ps(1.0001f);
    __m512 acc2 = _mm512_set1_ps(1.0002f);
    __m512 acc3 = _mm512_set1_ps(1.0003f);
    __m512 acc4 = _mm512_set1_ps(1.0004f);
    __m512 acc5 = _mm512_set1_ps(1.0005f);
    __m512 acc6 = _mm512_set1_ps(1.0006f);
    __m512 acc7 = _mm512_set1_ps(1.0007f);
    __m512 mul_operand = _mm512_set1_ps(1.0000001f);
    __m512 add_operand = _mm512_set1_ps(0.0000001f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_maskz_fmadd_ps(narrow_mask, acc0, mul_operand, add_operand);
        acc1 = _mm512_maskz_fmadd_ps(narrow_mask, acc1, mul_operand, add_operand);
        acc2 = _mm512_maskz_fmadd_ps(narrow_mask, acc2, mul_operand, add_operand);
        acc3 = _mm512_maskz_fmadd_ps(narrow_mask, acc3, mul_operand, add_operand);
        acc4 = _mm512_maskz_fmadd_ps(narrow_mask, acc4, mul_operand, add_operand);
        acc5 = _mm512_maskz_fmadd_ps(narrow_mask, acc5, mul_operand, add_operand);
        acc6 = _mm512_maskz_fmadd_ps(narrow_mask, acc6, mul_operand, add_operand);
        acc7 = _mm512_maskz_fmadd_ps(narrow_mask, acc7, mul_operand, add_operand);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc0) + _mm512_reduce_add_ps(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- vblendmps zmm {k1} throughput --- */
static double blendm_throughput(uint64_t iterations)
{
    __mmask16 blend_mask = (__mmask16)0xAAAA; /* alternating lanes */

    __m512 src_a0 = _mm512_set1_ps(1.0f);
    __m512 src_a1 = _mm512_set1_ps(2.0f);
    __m512 src_a2 = _mm512_set1_ps(3.0f);
    __m512 src_a3 = _mm512_set1_ps(4.0f);
    __m512 src_a4 = _mm512_set1_ps(5.0f);
    __m512 src_a5 = _mm512_set1_ps(6.0f);
    __m512 src_a6 = _mm512_set1_ps(7.0f);
    __m512 src_a7 = _mm512_set1_ps(8.0f);
    __m512 src_b  = _mm512_set1_ps(99.0f);
    __m512 acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7;

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        acc0 = _mm512_mask_blend_ps(blend_mask, src_a0, src_b);
        acc1 = _mm512_mask_blend_ps(blend_mask, src_a1, src_b);
        acc2 = _mm512_mask_blend_ps(blend_mask, src_a2, src_b);
        acc3 = _mm512_mask_blend_ps(blend_mask, src_a3, src_b);
        acc4 = _mm512_mask_blend_ps(blend_mask, src_a4, src_b);
        acc5 = _mm512_mask_blend_ps(blend_mask, src_a5, src_b);
        acc6 = _mm512_mask_blend_ps(blend_mask, src_a6, src_b);
        acc7 = _mm512_mask_blend_ps(blend_mask, src_a7, src_b);
        __asm__ volatile("" : "+x"(acc0), "+x"(acc1), "+x"(acc2), "+x"(acc3),
                              "+x"(acc4), "+x"(acc5), "+x"(acc6), "+x"(acc7));
        src_a0 = acc0; src_a1 = acc1; src_a2 = acc2; src_a3 = acc3;
        src_a4 = acc4; src_a5 = acc5; src_a6 = acc6; src_a7 = acc7;
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(acc0) + _mm512_reduce_add_ps(acc4);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

/* --- kand/kor/knot mask register arithmetic throughput --- */
static double mask_arith_throughput(uint64_t iterations)
{
    __mmask16 mask_k0 = (__mmask16)0xAAAA;
    __mmask16 mask_k1 = (__mmask16)0x5555;
    __mmask16 mask_k2 = (__mmask16)0xFF00;
    __mmask16 mask_k3 = (__mmask16)0x00FF;
    __mmask16 mask_k4 = (__mmask16)0xF0F0;
    __mmask16 mask_k5 = (__mmask16)0x0F0F;
    __mmask16 mask_k6 = (__mmask16)0xC3C3;
    __mmask16 mask_k7 = (__mmask16)0x3C3C;

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* Mix of kand, kor, knot — 8 ops per iteration */
        mask_k0 = _kand_mask16(mask_k0, mask_k1);
        mask_k1 = _kor_mask16(mask_k1, mask_k2);
        mask_k2 = _knot_mask16(mask_k2);
        mask_k3 = _kand_mask16(mask_k3, mask_k4);
        mask_k4 = _kor_mask16(mask_k4, mask_k5);
        mask_k5 = _knot_mask16(mask_k5);
        mask_k6 = _kand_mask16(mask_k6, mask_k7);
        mask_k7 = _kor_mask16(mask_k7, mask_k0);
        /* Barrier: split into two blocks; x86_64 only has k0-k7 */
        __asm__ volatile("" : "+Yk"(mask_k0), "+Yk"(mask_k1),
                              "+Yk"(mask_k2), "+Yk"(mask_k3));
        __asm__ volatile("" : "+Yk"(mask_k4), "+Yk"(mask_k5),
                              "+Yk"(mask_k6), "+Yk"(mask_k7));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile unsigned int sink = (unsigned int)mask_k0 ^ (unsigned int)mask_k4;
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_mask\ncpu=%d\navx512f=%d\n\n", cfg.cpu, feat.has_avx512f);

    if (!feat.has_avx512f) {
        printf("ERROR: AVX-512F not supported\n");
        return 1;
    }

    double fma_unmask_tp     = fma_unmasked_throughput(cfg.iterations);
    double fma_mask_tp       = fma_masked_throughput(cfg.iterations);
    double fma_narrow_tp     = fma_narrow_mask_throughput(cfg.iterations);
    double blend_tp          = blendm_throughput(cfg.iterations);
    double mask_arith_tp     = mask_arith_throughput(cfg.iterations);

    printf("fma_unmasked_throughput_cycles=%.4f\n", fma_unmask_tp);
    printf("fma_masked_11of16_throughput_cycles=%.4f\n", fma_mask_tp);
    printf("fma_masked_4of16_throughput_cycles=%.4f\n", fma_narrow_tp);
    printf("masked_vs_unmasked_fma_ratio=%.3f\n", fma_mask_tp / fma_unmask_tp);
    printf("narrow4_vs_unmasked_fma_ratio=%.3f\n", fma_narrow_tp / fma_unmask_tp);
    printf("vblendmps_throughput_cycles=%.4f\n", blend_tp);
    printf("mask_arith_throughput_cycles=%.4f\n", mask_arith_tp);

    if (cfg.csv)
        printf("csv,probe=avx512_mask,fma_unmask=%.4f,fma_mask11=%.4f,"
               "fma_mask4=%.4f,mask_ratio=%.3f,narrow_ratio=%.3f,"
               "blend=%.4f,mask_arith=%.4f\n",
               fma_unmask_tp, fma_mask_tp, fma_narrow_tp,
               fma_mask_tp / fma_unmask_tp, fma_narrow_tp / fma_unmask_tp,
               blend_tp, mask_arith_tp);
    return 0;
}
