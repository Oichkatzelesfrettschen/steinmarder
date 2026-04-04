#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * probe_avx512_throttle.c — Detect AVX-512 frequency throttling on Zen 4.
 *
 * Intel CPUs aggressively downclock on AVX-512 workloads. Zen 4 is
 * documented as NOT throttling, but WSL2/Hyper-V may impose limits.
 *
 * Method: run 10 timed bursts of zmm FMA, each 50M iterations.
 * If frequency throttles, later bursts show higher cycle counts.
 * Also measures warmup: short scalar burst, then zmm burst, to detect
 * transition penalty.
 */

static double scalar_burst(uint64_t iterations)
{
    volatile float acc = 1.0f;
    float mul = 1.0000001f;

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++)
        acc = acc * mul + 0.0000001f;
    uint64_t t1 = sm_zen_tsc_end();

    return (double)(t1 - t0) / (double)iterations;
}

static double zmm_burst(uint64_t iterations)
{
    __m512 a0 = _mm512_set1_ps(1.0f);
    __m512 a1 = _mm512_set1_ps(1.0001f);
    __m512 a2 = _mm512_set1_ps(1.0002f);
    __m512 a3 = _mm512_set1_ps(1.0003f);
    __m512 b  = _mm512_set1_ps(1.0000001f);
    __m512 c  = _mm512_set1_ps(0.0000001f);

    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iterations; i++) {
        a0 = _mm512_fmadd_ps(a0, b, c);
        a1 = _mm512_fmadd_ps(a1, b, c);
        a2 = _mm512_fmadd_ps(a2, b, c);
        a3 = _mm512_fmadd_ps(a3, b, c);
        __asm__ volatile("" : "+x"(a0), "+x"(a1), "+x"(a2), "+x"(a3));
    }
    uint64_t t1 = sm_zen_tsc_end();

    volatile float sink = _mm512_reduce_add_ps(a0);
    (void)sink;
    return (double)(t1 - t0) / (double)(iterations * 4);
}

int main(int argc, char **argv)
{
    SMZenProbeConfig cfg = sm_zen_default_config();
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    printf("probe=avx512_throttle\ncpu=%d\n\n", cfg.cpu);

    /* Phase 1: scalar warmup then immediate zmm transition */
    printf("=== scalar-to-zmm transition ===\n");
    double scalar_cyc = scalar_burst(10000000);
    printf("scalar_warmup_cyc_per_op=%.4f\n", scalar_cyc);
    double first_zmm = zmm_burst(10000000);
    printf("first_zmm_burst_cyc_per_fma=%.4f\n", first_zmm);

    /* Phase 2: sustained zmm bursts — detect drift */
    printf("\n=== sustained zmm bursts ===\n");
    printf("burst,cyc_per_fma\n");
    double min_cyc = 1e9, max_cyc = 0;
    for (int burst = 0; burst < 10; burst++) {
        double cyc = zmm_burst(50000000);
        printf("%d,%.4f\n", burst, cyc);
        fflush(stdout);
        if (cyc < min_cyc) min_cyc = cyc;
        if (cyc > max_cyc) max_cyc = cyc;
    }

    printf("\nmin_cyc=%.4f\n", min_cyc);
    printf("max_cyc=%.4f\n", max_cyc);
    printf("drift_pct=%.2f%%\n", 100.0 * (max_cyc - min_cyc) / min_cyc);
    printf("throttle_detected=%s\n",
           (max_cyc - min_cyc) / min_cyc > 0.05 ? "YES" : "NO");

    if (cfg.csv)
        printf("csv,probe=avx512_throttle,min=%.4f,max=%.4f,drift=%.2f%%,throttle=%s\n",
               min_cyc, max_cyc, 100.0 * (max_cyc - min_cyc) / min_cyc,
               (max_cyc - min_cyc) / min_cyc > 0.05 ? "YES" : "NO");
    return 0;
}
