#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

/*
 * probe_bmi.c -- BMI1/BMI2 bit manipulation instructions on Zen 4.
 *
 * Measures latency and throughput of scalar (GPR) bit operations:
 * tzcnt, lzcnt, popcnt, andn, bextr, bzhi, pdep, pext, mulx, rorx,
 * sarx, shlx, shrx.
 *
 * KEY QUESTION: Zen 1-3 had microcoded pdep/pext (~18 cycles).
 * Did Zen 4 fix this to single-cycle?
 */

/* ── Helper macros for latency (dep chain) and throughput (8 independent) ── */

#define BMI_LATENCY_UNARY(name, asm_mnemonic)                                   \
static double name##_latency(uint64_t iterations)                               \
{                                                                               \
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;                              \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t i = 0; i < iterations; i++) {                                \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(accumulator));          \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(accumulator));          \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(accumulator));          \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(accumulator));          \
    }                                                                           \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile uint64_t sink = accumulator; (void)sink;                           \
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);          \
}

#define BMI_THROUGHPUT_UNARY(name, asm_mnemonic)                                 \
static double name##_throughput(uint64_t iterations)                             \
{                                                                               \
    uint64_t acc0 = 0x0123456789ABCDEFULL;                                     \
    uint64_t acc1 = 0x1122334455667788ULL;                                     \
    uint64_t acc2 = 0x2233445566778899ULL;                                     \
    uint64_t acc3 = 0x33445566778899AAULL;                                     \
    uint64_t acc4 = 0x445566778899AABBULL;                                     \
    uint64_t acc5 = 0x5566778899AABBCCULL;                                     \
    uint64_t acc6 = 0x66778899AABBCCDDULL;                                     \
    uint64_t acc7 = 0x778899AABBCCDDEULL;                                      \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t i = 0; i < iterations; i++) {                                \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc0));                 \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc1));                 \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc2));                 \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc3));                 \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc4));                 \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc5));                 \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc6));                 \
        __asm__ volatile(asm_mnemonic " %0, %0" : "+r"(acc7));                 \
        __asm__ volatile("" : "+r"(acc0), "+r"(acc1), "+r"(acc2), "+r"(acc3),  \
                              "+r"(acc4), "+r"(acc5), "+r"(acc6), "+r"(acc7)); \
    }                                                                           \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile uint64_t sink = acc0 ^ acc4; (void)sink;                           \
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);          \
}

/* ── Helper macros for binary ops (two-input: op dst, src1, src2) ── */

#define BMI_LATENCY_BINARY(name, asm_mnemonic)                                   \
static double name##_latency(uint64_t iterations)                               \
{                                                                               \
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;                              \
    uint64_t operand_b   = 0x0F0F0F0F0F0F0F0FULL;                              \
    __asm__ volatile("" : "+r"(operand_b));                                     \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t i = 0; i < iterations; i++) {                                \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(accumulator) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(accumulator) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(accumulator) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(accumulator) : "r"(operand_b)); \
    }                                                                           \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile uint64_t sink = accumulator; (void)sink;                           \
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);          \
}

#define BMI_THROUGHPUT_BINARY(name, asm_mnemonic)                                \
static double name##_throughput(uint64_t iterations)                             \
{                                                                               \
    uint64_t acc0 = 0x0123456789ABCDEFULL;                                     \
    uint64_t acc1 = 0x1122334455667788ULL;                                     \
    uint64_t acc2 = 0x2233445566778899ULL;                                     \
    uint64_t acc3 = 0x33445566778899AAULL;                                     \
    uint64_t acc4 = 0x445566778899AABBULL;                                     \
    uint64_t acc5 = 0x5566778899AABBCCULL;                                     \
    uint64_t acc6 = 0x66778899AABBCCDDULL;                                     \
    uint64_t acc7 = 0x778899AABBCCDDEULL;                                      \
    uint64_t operand_b = 0x0F0F0F0F0F0F0F0FULL;                                \
    __asm__ volatile("" : "+r"(operand_b));                                     \
    uint64_t tsc_start = sm_zen_tsc_begin();                                   \
    for (uint64_t i = 0; i < iterations; i++) {                                \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc0) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc1) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc2) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc3) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc4) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc5) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc6) : "r"(operand_b)); \
        __asm__ volatile(asm_mnemonic " %1, %0, %0" : "+r"(acc7) : "r"(operand_b)); \
        __asm__ volatile("" : "+r"(acc0), "+r"(acc1), "+r"(acc2), "+r"(acc3),  \
                              "+r"(acc4), "+r"(acc5), "+r"(acc6), "+r"(acc7)); \
    }                                                                           \
    uint64_t tsc_stop = sm_zen_tsc_end();                                      \
    volatile uint64_t sink = acc0 ^ acc4; (void)sink;                           \
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);          \
}

/* ── Instantiate all BMI ops ── */

/* BMI1 / ABM unary */
BMI_LATENCY_UNARY(tzcnt, "tzcnt")
BMI_THROUGHPUT_UNARY(tzcnt, "tzcnt")
BMI_LATENCY_UNARY(lzcnt, "lzcnt")
BMI_THROUGHPUT_UNARY(lzcnt, "lzcnt")
BMI_LATENCY_UNARY(popcnt, "popcnt")
BMI_THROUGHPUT_UNARY(popcnt, "popcnt")

/* BMI1 binary */
BMI_LATENCY_BINARY(andn, "andn")
BMI_THROUGHPUT_BINARY(andn, "andn")
BMI_LATENCY_BINARY(bextr, "bextr")
BMI_THROUGHPUT_BINARY(bextr, "bextr")

/* BMI2 binary */
BMI_LATENCY_BINARY(bzhi, "bzhi")
BMI_THROUGHPUT_BINARY(bzhi, "bzhi")
BMI_LATENCY_BINARY(sarx, "sarx")
BMI_THROUGHPUT_BINARY(sarx, "sarx")
BMI_LATENCY_BINARY(shlx, "shlx")
BMI_THROUGHPUT_BINARY(shlx, "shlx")
BMI_LATENCY_BINARY(shrx, "shrx")
BMI_THROUGHPUT_BINARY(shrx, "shrx")

/* pdep/pext: the key Zen 4 question — fixed or still microcoded? */
BMI_LATENCY_BINARY(pdep, "pdep")
BMI_THROUGHPUT_BINARY(pdep, "pdep")
BMI_LATENCY_BINARY(pext, "pext")
BMI_THROUGHPUT_BINARY(pext, "pext")

/* mulx: unsigned multiply r64 * rdx -> r64:r64 (no flags)
   For latency: chain through low half. For throughput: independent mulx ops. */

static double mulx_latency(uint64_t iterations)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t high_half;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        /* mulx high, low, src  -- implicit rdx as other multiplicand */
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
        __asm__ volatile("mulx %2, %0, %1" : "+d"(accumulator), "=r"(high_half) : "r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator ^ high_half; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double mulx_throughput(uint64_t iterations)
{
    uint64_t rdx_input = 0xDEADBEEFCAFEBABEULL;
    uint64_t acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7;
    uint64_t hi0, hi1, hi2, hi3, hi4, hi5, hi6, hi7;
    uint64_t src0 = 0x0123456789ABCDEFULL;
    uint64_t src1 = 0x1122334455667788ULL;
    uint64_t src2 = 0x2233445566778899ULL;
    uint64_t src3 = 0x33445566778899AAULL;
    uint64_t src4 = 0x445566778899AABBULL;
    uint64_t src5 = 0x5566778899AABBCCULL;
    uint64_t src6 = 0x66778899AABBCCDDULL;
    uint64_t src7 = 0x778899AABBCCDDEULL;
    __asm__ volatile("" : "+r"(src0), "+r"(src1), "+r"(src2), "+r"(src3),
                          "+r"(src4), "+r"(src5), "+r"(src6), "+r"(src7));

    uint64_t tsc_start = sm_zen_tsc_begin();
    __asm__ volatile("mov %0, %%rdx" : : "r"(rdx_input) : "rdx");
    for (uint64_t i = 0; i < iterations; i++) {
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc0), "=r"(hi0) : "r"(src0) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc1), "=r"(hi1) : "r"(src1) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc2), "=r"(hi2) : "r"(src2) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc3), "=r"(hi3) : "r"(src3) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc4), "=r"(hi4) : "r"(src4) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc5), "=r"(hi5) : "r"(src5) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc6), "=r"(hi6) : "r"(src6) : "rdx");
        __asm__ volatile("mulx %2, %0, %1" : "=r"(acc7), "=r"(hi7) : "r"(src7) : "rdx");
        __asm__ volatile("" : "+r"(acc0), "+r"(acc1), "+r"(acc2), "+r"(acc3),
                              "+r"(acc4), "+r"(acc5), "+r"(acc6), "+r"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = acc0 ^ acc4 ^ hi0 ^ hi4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

/* rorx: rotate right by immediate (no flags) — use inline asm with constant imm */

static double rorx_latency(uint64_t iterations)
{
    uint64_t accumulator = 0xDEADBEEFCAFEBABEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        __asm__ volatile("rorx $17, %0, %0" : "+r"(accumulator));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(accumulator));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(accumulator));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(accumulator));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = accumulator; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 4);
}

static double rorx_throughput(uint64_t iterations)
{
    uint64_t acc0 = 0x0123456789ABCDEFULL;
    uint64_t acc1 = 0x1122334455667788ULL;
    uint64_t acc2 = 0x2233445566778899ULL;
    uint64_t acc3 = 0x33445566778899AAULL;
    uint64_t acc4 = 0x445566778899AABBULL;
    uint64_t acc5 = 0x5566778899AABBCCULL;
    uint64_t acc6 = 0x66778899AABBCCDDULL;
    uint64_t acc7 = 0x778899AABBCCDDEULL;
    uint64_t tsc_start = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc0));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc1));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc2));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc3));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc4));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc5));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc6));
        __asm__ volatile("rorx $17, %0, %0" : "+r"(acc7));
        __asm__ volatile("" : "+r"(acc0), "+r"(acc1), "+r"(acc2), "+r"(acc3),
                              "+r"(acc4), "+r"(acc5), "+r"(acc6), "+r"(acc7));
    }
    uint64_t tsc_stop = sm_zen_tsc_end();
    volatile uint64_t sink = acc0 ^ acc4; (void)sink;
    return (double)(tsc_stop - tsc_start) / (double)(iterations * 8);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    SMZenFeatures feat = sm_zen_detect_features();
    sm_zen_pin_to_cpu(cfg.cpu);

    sm_zen_print_header("bmi", &cfg, &feat);
    printf("\n");

    if (!feat.has_bmi2) {
        printf("ERROR: BMI2 not supported on this CPU\n");
        return 1;
    }

    /* Measure all ops */
    printf("=== BMI1 / ABM / POPCNT (unary) ===\n\n");

    double tzcnt_lat = tzcnt_latency(cfg.iterations);
    double tzcnt_tp  = tzcnt_throughput(cfg.iterations);
    printf("tzcnt_r64_latency_cycles=%.4f\n", tzcnt_lat);
    printf("tzcnt_r64_throughput_cycles=%.4f\n\n", tzcnt_tp);

    double lzcnt_lat = lzcnt_latency(cfg.iterations);
    double lzcnt_tp  = lzcnt_throughput(cfg.iterations);
    printf("lzcnt_r64_latency_cycles=%.4f\n", lzcnt_lat);
    printf("lzcnt_r64_throughput_cycles=%.4f\n\n", lzcnt_tp);

    double popcnt_lat = popcnt_latency(cfg.iterations);
    double popcnt_tp  = popcnt_throughput(cfg.iterations);
    printf("popcnt_r64_latency_cycles=%.4f\n", popcnt_lat);
    printf("popcnt_r64_throughput_cycles=%.4f\n\n", popcnt_tp);

    printf("=== BMI1 (binary) ===\n\n");

    double andn_lat = andn_latency(cfg.iterations);
    double andn_tp  = andn_throughput(cfg.iterations);
    printf("andn_r64_latency_cycles=%.4f\n", andn_lat);
    printf("andn_r64_throughput_cycles=%.4f\n\n", andn_tp);

    double bextr_lat = bextr_latency(cfg.iterations);
    double bextr_tp  = bextr_throughput(cfg.iterations);
    printf("bextr_r64_latency_cycles=%.4f\n", bextr_lat);
    printf("bextr_r64_throughput_cycles=%.4f\n\n", bextr_tp);

    printf("=== BMI2 ===\n\n");

    double bzhi_lat = bzhi_latency(cfg.iterations);
    double bzhi_tp  = bzhi_throughput(cfg.iterations);
    printf("bzhi_r64_latency_cycles=%.4f\n", bzhi_lat);
    printf("bzhi_r64_throughput_cycles=%.4f\n\n", bzhi_tp);

    printf("--- pdep/pext (THE Zen 4 question: fixed or still microcoded?) ---\n");
    double pdep_lat = pdep_latency(cfg.iterations);
    double pdep_tp  = pdep_throughput(cfg.iterations);
    printf("pdep_r64_latency_cycles=%.4f\n", pdep_lat);
    printf("pdep_r64_throughput_cycles=%.4f\n", pdep_tp);
    if (pdep_lat < 5.0)
        printf("  -> FAST pdep (single-cycle class) -- Zen 4 FIX CONFIRMED\n");
    else
        printf("  -> SLOW pdep (%.0f+ cycles) -- still microcoded\n", pdep_lat);
    printf("\n");

    double pext_lat = pext_latency(cfg.iterations);
    double pext_tp  = pext_throughput(cfg.iterations);
    printf("pext_r64_latency_cycles=%.4f\n", pext_lat);
    printf("pext_r64_throughput_cycles=%.4f\n", pext_tp);
    if (pext_lat < 5.0)
        printf("  -> FAST pext (single-cycle class) -- Zen 4 FIX CONFIRMED\n");
    else
        printf("  -> SLOW pext (%.0f+ cycles) -- still microcoded\n", pext_lat);
    printf("\n");

    double mulx_lat = mulx_latency(cfg.iterations);
    double mulx_tp  = mulx_throughput(cfg.iterations);
    printf("mulx_r64_latency_cycles=%.4f\n", mulx_lat);
    printf("mulx_r64_throughput_cycles=%.4f\n\n", mulx_tp);

    double rorx_lat = rorx_latency(cfg.iterations);
    double rorx_tp  = rorx_throughput(cfg.iterations);
    printf("rorx_r64_latency_cycles=%.4f\n", rorx_lat);
    printf("rorx_r64_throughput_cycles=%.4f\n\n", rorx_tp);

    printf("--- BMI2 shift-without-flags ---\n");
    double sarx_lat = sarx_latency(cfg.iterations);
    double sarx_tp  = sarx_throughput(cfg.iterations);
    printf("sarx_r64_latency_cycles=%.4f\n", sarx_lat);
    printf("sarx_r64_throughput_cycles=%.4f\n\n", sarx_tp);

    double shlx_lat = shlx_latency(cfg.iterations);
    double shlx_tp  = shlx_throughput(cfg.iterations);
    printf("shlx_r64_latency_cycles=%.4f\n", shlx_lat);
    printf("shlx_r64_throughput_cycles=%.4f\n\n", shlx_tp);

    double shrx_lat = shrx_latency(cfg.iterations);
    double shrx_tp  = shrx_throughput(cfg.iterations);
    printf("shrx_r64_latency_cycles=%.4f\n", shrx_lat);
    printf("shrx_r64_throughput_cycles=%.4f\n\n", shrx_tp);

    if (cfg.csv)
        printf("csv,probe=bmi,"
               "tzcnt_lat=%.4f,tzcnt_tp=%.4f,"
               "lzcnt_lat=%.4f,lzcnt_tp=%.4f,"
               "popcnt_lat=%.4f,popcnt_tp=%.4f,"
               "andn_lat=%.4f,andn_tp=%.4f,"
               "bextr_lat=%.4f,bextr_tp=%.4f,"
               "bzhi_lat=%.4f,bzhi_tp=%.4f,"
               "pdep_lat=%.4f,pdep_tp=%.4f,"
               "pext_lat=%.4f,pext_tp=%.4f,"
               "mulx_lat=%.4f,mulx_tp=%.4f,"
               "rorx_lat=%.4f,rorx_tp=%.4f,"
               "sarx_lat=%.4f,sarx_tp=%.4f,"
               "shlx_lat=%.4f,shlx_tp=%.4f,"
               "shrx_lat=%.4f,shrx_tp=%.4f\n",
               tzcnt_lat, tzcnt_tp,
               lzcnt_lat, lzcnt_tp,
               popcnt_lat, popcnt_tp,
               andn_lat, andn_tp,
               bextr_lat, bextr_tp,
               bzhi_lat, bzhi_tp,
               pdep_lat, pdep_tp,
               pext_lat, pext_tp,
               mulx_lat, mulx_tp,
               rorx_lat, rorx_tp,
               sarx_lat, sarx_tp,
               shlx_lat, shlx_tp,
               shrx_lat, shrx_tp);
    return 0;
}
