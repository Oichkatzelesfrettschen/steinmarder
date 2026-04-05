#define _GNU_SOURCE
#include "common.h"
#include <immintrin.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h>

/* Measure the last 13 genuine gaps: 10 AVX moves, 2 AVX-512F, 1 SSSE3 */

#define LAT4(op) do { op; op; op; op; } while(0)
#define TP8_BARRIER __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),"+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7))

/* --- AVX move throughput (no meaningful latency for moves) --- */

static double measure_VMOVSS_tp(uint64_t iters) {
    __m128 a0=_mm_set1_ps(1),a1=_mm_set1_ps(2),a2=_mm_set1_ps(3),a3=_mm_set1_ps(4);
    __m128 a4=_mm_set1_ps(5),a5=_mm_set1_ps(6),a6=_mm_set1_ps(7),a7=_mm_set1_ps(8);
    __m128 b=_mm_set1_ps(0.5f);
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm_move_ss(a0,b); a1=_mm_move_ss(a1,b); a2=_mm_move_ss(a2,b); a3=_mm_move_ss(a3,b);
        a4=_mm_move_ss(a4,b); a5=_mm_move_ss(a5,b); a6=_mm_move_ss(a6,b); a7=_mm_move_ss(a7,b);
        TP8_BARRIER;
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile float s = _mm_cvtss_f32(a0)+_mm_cvtss_f32(a4); (void)s;
    return (double)(t1-t0)/(double)(iters*8);
}

static double measure_VMOVSD_tp(uint64_t iters) {
    __m128d a0=_mm_set1_pd(1),a1=_mm_set1_pd(2),a2=_mm_set1_pd(3),a3=_mm_set1_pd(4);
    __m128d a4=_mm_set1_pd(5),a5=_mm_set1_pd(6),a6=_mm_set1_pd(7),a7=_mm_set1_pd(8);
    __m128d b=_mm_set1_pd(0.5);
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm_move_sd(a0,b); a1=_mm_move_sd(a1,b); a2=_mm_move_sd(a2,b); a3=_mm_move_sd(a3,b);
        a4=_mm_move_sd(a4,b); a5=_mm_move_sd(a5,b); a6=_mm_move_sd(a6,b); a7=_mm_move_sd(a7,b);
        __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),"+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile double s = _mm_cvtsd_f64(a0)+_mm_cvtsd_f64(a4); (void)s;
    return (double)(t1-t0)/(double)(iters*8);
}

static double measure_VMOVUPS_tp(uint64_t iters, const float *buf) {
    __m256 a0,a1,a2,a3,a4,a5,a6,a7;
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm256_loadu_ps(buf);    a1=_mm256_loadu_ps(buf+8);
        a2=_mm256_loadu_ps(buf+16); a3=_mm256_loadu_ps(buf+24);
        a4=_mm256_loadu_ps(buf+32); a5=_mm256_loadu_ps(buf+40);
        a6=_mm256_loadu_ps(buf+48); a7=_mm256_loadu_ps(buf+56);
        TP8_BARRIER;
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile float s; _mm_store_ss(&s, _mm256_castps256_ps128(a0));
    return (double)(t1-t0)/(double)(iters*8);
}

static double measure_VMOVUPD_tp(uint64_t iters, const double *buf) {
    __m256d a0,a1,a2,a3,a4,a5,a6,a7;
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm256_loadu_pd(buf);    a1=_mm256_loadu_pd(buf+4);
        a2=_mm256_loadu_pd(buf+8);  a3=_mm256_loadu_pd(buf+12);
        a4=_mm256_loadu_pd(buf+16); a5=_mm256_loadu_pd(buf+20);
        a6=_mm256_loadu_pd(buf+24); a7=_mm256_loadu_pd(buf+28);
        __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),"+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile double s = _mm256_cvtsd_f64(a0); (void)s;
    return (double)(t1-t0)/(double)(iters*8);
}

static double measure_VMOVHPS_tp(uint64_t iters, const float *buf) {
    __m128 a0=_mm_setzero_ps(),a1=_mm_setzero_ps(),a2=_mm_setzero_ps(),a3=_mm_setzero_ps();
    __m128 a4=_mm_setzero_ps(),a5=_mm_setzero_ps(),a6=_mm_setzero_ps(),a7=_mm_setzero_ps();
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm_loadh_pi(a0,(__m64*)(buf));   a1=_mm_loadh_pi(a1,(__m64*)(buf+2));
        a2=_mm_loadh_pi(a2,(__m64*)(buf+4)); a3=_mm_loadh_pi(a3,(__m64*)(buf+6));
        a4=_mm_loadh_pi(a4,(__m64*)(buf+8)); a5=_mm_loadh_pi(a5,(__m64*)(buf+10));
        a6=_mm_loadh_pi(a6,(__m64*)(buf+12));a7=_mm_loadh_pi(a7,(__m64*)(buf+14));
        TP8_BARRIER;
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile float s = _mm_cvtss_f32(a0); (void)s;
    return (double)(t1-t0)/(double)(iters*8);
}

static double measure_VMOVHPD_tp(uint64_t iters, const double *buf) {
    __m128d a0=_mm_setzero_pd(),a1=_mm_setzero_pd(),a2=_mm_setzero_pd(),a3=_mm_setzero_pd();
    __m128d a4=_mm_setzero_pd(),a5=_mm_setzero_pd(),a6=_mm_setzero_pd(),a7=_mm_setzero_pd();
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm_loadh_pd(a0,buf);   a1=_mm_loadh_pd(a1,buf+1);
        a2=_mm_loadh_pd(a2,buf+2); a3=_mm_loadh_pd(a3,buf+3);
        a4=_mm_loadh_pd(a4,buf+4); a5=_mm_loadh_pd(a5,buf+5);
        a6=_mm_loadh_pd(a6,buf+6); a7=_mm_loadh_pd(a7,buf+7);
        __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),"+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile double s = _mm_cvtsd_f64(a0); (void)s;
    return (double)(t1-t0)/(double)(iters*8);
}

static double measure_VTESTPS_tp(uint64_t iters) {
    __m256 a = _mm256_set1_ps(1.0f);
    __m256 b = _mm256_set1_ps(2.0f);
    volatile int sink = 0;
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        sink += _mm256_testz_ps(a, b);
        sink += _mm256_testz_ps(a, b);
        sink += _mm256_testz_ps(a, b);
        sink += _mm256_testz_ps(a, b);
    }
    uint64_t t1 = sm_zen_tsc_end();
    (void)sink;
    return (double)(t1-t0)/(double)(iters*4);
}

static double measure_VTESTPD_tp(uint64_t iters) {
    __m256d a = _mm256_set1_pd(1.0);
    __m256d b = _mm256_set1_pd(2.0);
    volatile int sink = 0;
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        sink += _mm256_testz_pd(a, b);
        sink += _mm256_testz_pd(a, b);
        sink += _mm256_testz_pd(a, b);
        sink += _mm256_testz_pd(a, b);
    }
    uint64_t t1 = sm_zen_tsc_end();
    (void)sink;
    return (double)(t1-t0)/(double)(iters*4);
}

static double measure_VZEROALL_tp(uint64_t iters) {
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        _mm256_zeroall();
        _mm256_zeroall();
        _mm256_zeroall();
        _mm256_zeroall();
    }
    uint64_t t1 = sm_zen_tsc_end();
    return (double)(t1-t0)/(double)(iters*4);
}

static double measure_VBROADCASTF128_tp(uint64_t iters, const float *buf) {
    __m256 a0,a1,a2,a3,a4,a5,a6,a7;
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm256_broadcast_ps((__m128*)(buf));    a1=_mm256_broadcast_ps((__m128*)(buf+4));
        a2=_mm256_broadcast_ps((__m128*)(buf+8));  a3=_mm256_broadcast_ps((__m128*)(buf+12));
        a4=_mm256_broadcast_ps((__m128*)(buf+16)); a5=_mm256_broadcast_ps((__m128*)(buf+20));
        a6=_mm256_broadcast_ps((__m128*)(buf+24)); a7=_mm256_broadcast_ps((__m128*)(buf+28));
        TP8_BARRIER;
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile float s; _mm_store_ss(&s, _mm256_castps256_ps128(a0));
    return (double)(t1-t0)/(double)(iters*8);
}

/* --- AVX-512F: VFIXUPIMMPD, VFPCLASSPD --- */

static double measure_VFIXUPIMMPD_tp(uint64_t iters) {
    __m512d a0=_mm512_set1_pd(1.0),a1=_mm512_set1_pd(2.0),a2=_mm512_set1_pd(3.0),a3=_mm512_set1_pd(4.0);
    __m512d a4=_mm512_set1_pd(5.0),a5=_mm512_set1_pd(6.0),a6=_mm512_set1_pd(7.0),a7=_mm512_set1_pd(8.0);
    __m512d b=_mm512_set1_pd(1.0);
    __m512i tbl=_mm512_set1_epi64(0);
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm512_fixupimm_pd(a0,b,tbl,0); a1=_mm512_fixupimm_pd(a1,b,tbl,0);
        a2=_mm512_fixupimm_pd(a2,b,tbl,0); a3=_mm512_fixupimm_pd(a3,b,tbl,0);
        a4=_mm512_fixupimm_pd(a4,b,tbl,0); a5=_mm512_fixupimm_pd(a5,b,tbl,0);
        a6=_mm512_fixupimm_pd(a6,b,tbl,0); a7=_mm512_fixupimm_pd(a7,b,tbl,0);
        __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),"+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile double s = _mm512_reduce_add_pd(a0); (void)s;
    return (double)(t1-t0)/(double)(iters*8);
}

static double measure_VFPCLASSPD_tp(uint64_t iters) {
    __m512d a = _mm512_set1_pd(1.0);
    volatile int sink = 0;
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        sink += _mm512_fpclass_pd_mask(a, 0x01);
        sink += _mm512_fpclass_pd_mask(a, 0x01);
        sink += _mm512_fpclass_pd_mask(a, 0x01);
        sink += _mm512_fpclass_pd_mask(a, 0x01);
    }
    uint64_t t1 = sm_zen_tsc_end();
    (void)sink;
    return (double)(t1-t0)/(double)(iters*4);
}

/* --- SSSE3: PMULHRSW --- */

static double measure_PMULHRSW_lat(uint64_t iters) {
    __m128i acc = _mm_set1_epi16(1000);
    __m128i b = _mm_set1_epi16(16384);
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        acc = _mm_mulhrs_epi16(acc, b);
        acc = _mm_mulhrs_epi16(acc, b);
        acc = _mm_mulhrs_epi16(acc, b);
        acc = _mm_mulhrs_epi16(acc, b);
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile int s = _mm_extract_epi16(acc, 0); (void)s;
    return (double)(t1-t0)/(double)(iters*4);
}

static double measure_PMULHRSW_tp(uint64_t iters) {
    __m128i a0=_mm_set1_epi16(1),a1=_mm_set1_epi16(2),a2=_mm_set1_epi16(3),a3=_mm_set1_epi16(4);
    __m128i a4=_mm_set1_epi16(5),a5=_mm_set1_epi16(6),a6=_mm_set1_epi16(7),a7=_mm_set1_epi16(8);
    __m128i b=_mm_set1_epi16(16384);
    uint64_t t0 = sm_zen_tsc_begin();
    for (uint64_t i = 0; i < iters; i++) {
        a0=_mm_mulhrs_epi16(a0,b); a1=_mm_mulhrs_epi16(a1,b);
        a2=_mm_mulhrs_epi16(a2,b); a3=_mm_mulhrs_epi16(a3,b);
        a4=_mm_mulhrs_epi16(a4,b); a5=_mm_mulhrs_epi16(a5,b);
        a6=_mm_mulhrs_epi16(a6,b); a7=_mm_mulhrs_epi16(a7,b);
        __asm__ volatile("" : "+x"(a0),"+x"(a1),"+x"(a2),"+x"(a3),"+x"(a4),"+x"(a5),"+x"(a6),"+x"(a7));
    }
    uint64_t t1 = sm_zen_tsc_end();
    volatile int s = _mm_extract_epi16(a0, 0) + _mm_extract_epi16(a4, 0); (void)s;
    return (double)(t1-t0)/(double)(iters*8);
}

int main(int argc, char **argv) {
    SMZenProbeConfig cfg = sm_zen_default_config();
    cfg.iterations = 50000;
    sm_zen_parse_common_args(argc, argv, &cfg);
    sm_zen_pin_to_cpu(cfg.cpu);

    float *fbuf = (float *)sm_zen_aligned_alloc64(4096);
    double *dbuf = (double *)sm_zen_aligned_alloc64(4096);
    memset(fbuf, 0x3F, 4096); memset(dbuf, 0x3F, 4096);

    printf("mnemonic,operands,width_bits,latency_cycles,throughput_cycles,extension\n");

    printf("VMOVSS,xmm xmm,128,-1.0000,%.4f,AVX\n", measure_VMOVSS_tp(cfg.iterations));
    printf("VMOVSD,xmm xmm,128,-1.0000,%.4f,AVX\n", measure_VMOVSD_tp(cfg.iterations));
    printf("VMOVUPS,ymm m256,256,-1.0000,%.4f,AVX\n", measure_VMOVUPS_tp(cfg.iterations, fbuf));
    printf("VMOVUPD,ymm m256,256,-1.0000,%.4f,AVX\n", measure_VMOVUPD_tp(cfg.iterations, dbuf));
    printf("VMOVHPS,xmm m64,128,-1.0000,%.4f,AVX\n", measure_VMOVHPS_tp(cfg.iterations, fbuf));
    printf("VMOVHPD,xmm m64,128,-1.0000,%.4f,AVX\n", measure_VMOVHPD_tp(cfg.iterations, dbuf));
    printf("VTESTPS,ymm ymm,256,-1.0000,%.4f,AVX\n", measure_VTESTPS_tp(cfg.iterations));
    printf("VTESTPD,ymm ymm,256,-1.0000,%.4f,AVX\n", measure_VTESTPD_tp(cfg.iterations));
    printf("VZEROALL,,256,-1.0000,%.4f,AVX\n", measure_VZEROALL_tp(cfg.iterations));
    printf("VBROADCASTF128,ymm m128,256,-1.0000,%.4f,AVX\n", measure_VBROADCASTF128_tp(cfg.iterations, fbuf));
    printf("VFIXUPIMMPD,zmm zmm zmm imm,512,-1.0000,%.4f,AVX-512F\n", measure_VFIXUPIMMPD_tp(cfg.iterations));
    printf("VFPCLASSPD,k zmm imm,512,-1.0000,%.4f,AVX-512F\n", measure_VFPCLASSPD_tp(cfg.iterations));
    printf("PMULHRSW,xmm xmm,128,%.4f,%.4f,SSSE3\n", measure_PMULHRSW_lat(cfg.iterations), measure_PMULHRSW_tp(cfg.iterations));

    free(fbuf); free(dbuf);
    return 0;
}
