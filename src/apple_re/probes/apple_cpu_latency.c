#include "apple_re_time.h"

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    APPLE_RE_UNROLL = 32,
    APPLE_RE_TRANSC_UNROLL = 4,
    APPLE_RE_LOAD_STORE_UNROLL = 32,
    APPLE_RE_ATOMIC_UNROLL = 32,
};

typedef uint64_t (*bench_fn_t)(uint64_t iters);

struct probe_def {
    const char *name;
    bench_fn_t fn;
    uint32_t ops_per_iter;
};

static volatile uint64_t g_u64_sink = 0;
static volatile double g_f64_sink = 0.0;
static _Atomic uint64_t g_atomic_counter = 1;

static float quantize_bfloat16_proxy(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    bits += 0x00008000u;
    bits &= 0xffff0000u;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint64_t bench_add_dep(uint64_t iters) {
    uint64_t x = 1;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x += 1;
        }
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

static uint64_t bench_fadd_dep(uint64_t iters) {
    double x = 1.0;
    const double one = 1.0;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\t"
            "fadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1\n\tfadd %d0, %d0, %d1"
            : "+w"(x)
            : "w"(one));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x += one;
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = x;
    return end - start;
}

static uint64_t bench_fadd_f32_dep(uint64_t iters) {
    float x = 1.0f;
    const float one = 1.0f;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\t"
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\t"
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\t"
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\t"
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\t"
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\t"
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\t"
            "fadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1\n\tfadd %s0, %s0, %s1"
            : "+w"(x)
            : "w"(one));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x += one;
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = (double)x;
    return end - start;
}

static uint64_t bench_bfloat16_roundtrip_proxy(uint64_t iters) {
    float acc = 1.0f;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc = quantize_bfloat16_proxy(acc + 0.125f);
        }
    }
    end = apple_re_now_ns();
    g_f64_sink = (double)acc;
    return end - start;
}

static uint64_t bench_fmadd_dep(uint64_t iters) {
    double acc = 1.0;
    const double a = 1.00000011920928955078125;
    const double b = 0.999999940395355224609375;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\t"
            "fmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2\n\tfmadd %d0, %d0, %d1, %d2"
            : "+w"(acc)
            : "w"(a), "w"(b));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc = (acc * a) + b;
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = acc;
    return end - start;
}

static uint64_t bench_load_store_chain(uint64_t iters) {
    volatile uint64_t lanes[APPLE_RE_LOAD_STORE_UNROLL];
    uint64_t x = 0x9e3779b97f4a7c15ull;
    uint64_t start;
    uint64_t end;

    for (uint32_t lane = 0; lane < APPLE_RE_LOAD_STORE_UNROLL; ++lane) {
        lanes[lane] = (uint64_t)(lane + 1u);
    }

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_LOAD_STORE_UNROLL; ++lane) {
            uint64_t loaded = lanes[lane];
            x ^= loaded + (uint64_t)lane;
            x = (x << 1) | (x >> 63);
            lanes[lane] = x + loaded;
        }
    }
    end = apple_re_now_ns();
    g_u64_sink = x ^ lanes[(uint32_t)(x & (APPLE_RE_LOAD_STORE_UNROLL - 1u))];
    return end - start;
}

/* FP16 probes — FCVT (f32↔f16 conversion), FMLA (SIMD half-precision FMA).
 *
 * Motivation: PyTorch MPS f16 matmul at 512×512 is 8× slower than f32 —
 * either JIT compilation cost or the f16 path not hitting hardware FP16.
 * These CPU probes baseline the AArch64 hardware FP16 path so the MPS
 * anomaly can be assessed against a known reference.
 *
 * AArch64 FP16 (FEAT_FP16, armv8.2-a):
 *   FCVT   Hd, Ss     f32 → f16 (scalar, rounds to nearest)
 *   FCVT   Sd, Hh     f16 → f32 (scalar, exact widening)
 *   FADD   Hd, Hn, Hm f16 scalar add
 *   FMUL   Hd, Hn, Hm f16 scalar multiply
 *   FMADD  Hd, Hn, Hm, Ha  f16 scalar fused multiply-add
 *   FMLA   Vd.4h, Vn.4h, Vm.4h  f16 SIMD (4-wide)
 *   FMLA   Vd.8h, Vn.8h, Vm.8h  f16 SIMD (8-wide)
 *
 * Note: FEAT_FP16 is required (armv8.2-a). Apple M1 supports this.
 * The _Float16 type is available in Clang on M-series; the SIMD probes use
 * __fp16 / vector types from <arm_neon.h>.
 */

#include <arm_neon.h>  /* float16x8_t, vfmaq_f16, etc. */

static uint64_t bench_fcvt_f32_to_f16_dep(uint64_t iters) {
    /* f32 → f16 → f32 → f16 ... dep chain via FCVT round-trip.
     * Each FCVT (f32→f16) truncates and the FCVT (f16→f32) widens.
     * The two-step round-trip keeps the value in a representable range while
     * creating a dependency: the f32 value after widening feeds the next f32→f16. */
    float acc_f32 = 1.0f;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__) && defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC)
        /* FCVT H, S then FCVT S, H — 32-deep dep chain (16 round-trips).
         * %h0 = H-register view of operand 0; %s0 = S-register view.
         * Both refer to the same underlying FP register. */
        asm volatile(
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0\n\t"
            "fcvt %h0, %s0\n\tfcvt %s0, %h0"
            : "+w"(acc_f32));
#else
        for (int lane = 0; lane < 32; ++lane) {
            _Float16 h = (_Float16)acc_f32;
            acc_f32 = (float)h;
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = (double)acc_f32;
    return end - start;
}

static uint64_t bench_fadd_f16_dep(uint64_t iters) {
    /* f16 scalar FADD dep chain — measures scalar f16 add latency.
     * Uses _Float16 (Clang extension) which emits scalar FADD H-register ops. */
    _Float16 acc = (_Float16)1.0f;
    const _Float16 increment = (_Float16)0.001f;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__) && defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC)
        /* %h0 = H-register view of acc; %h1 = H-register view of increment. */
        asm volatile(
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1"
            : "+w"(acc)
            : "w"(increment));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc += increment;
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = (double)(float)acc;
    return end - start;
}

static uint64_t bench_fmla_f16x8_dep(uint64_t iters) {
    /* FMLA .8h — 8-wide SIMD f16 fused multiply-add dep chain.
     * All 8 lanes use the same scalar multiplier and addend constant.
     * The dep chain is across all 8 lanes in lockstep — each FMLA reads
     * the result of the previous FMLA (all lanes).
     * This measures f16 SIMD FMA latency on the NEON unit.
     *
     * Expected: same pipeline latency as FMLA .4s (FP32 SIMD) since M-series
     * has native f16 NEON. If f16 is slower, that is the anomaly. */
    float16x8_t acc = vdupq_n_f16(1.0f);
    const float16x8_t mul_val = vdupq_n_f16(1.0001f);
    const float16x8_t add_val = vdupq_n_f16(0.0001f);
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__) && defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
        /* 32 dependent FMLA .8h instructions. */
        asm volatile(
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h\n\t"
            "fmla %0.8h, %1.8h, %2.8h\n\tfmla %0.8h, %1.8h, %2.8h"
            : "+w"(acc)
            : "w"(mul_val), "w"(add_val));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc = vfmaq_f16(add_val, acc, mul_val);
        }
#endif
    }
    end = apple_re_now_ns();
    g_f64_sink = (double)(float)vgetq_lane_f16(acc, 0);
    return end - start;
}

/* Integer multiply probes — MUL, MADD, MSUB, UMULH, SMULL dep chains.
 *
 * On AArch64:
 *   MUL    Xd, Xn, Xm       ≡ MADD Xd, Xn, Xm, XZR  (lower 64 bits, dep on Xd)
 *   MADD   Xd, Xn, Xm, Xa   Xd = Xa + Xn*Xm          (dep on both Xd and Xa)
 *   MSUB   Xd, Xn, Xm, Xa   Xd = Xa - Xn*Xm          (dep on both Xd and Xa)
 *   UMULH  Xd, Xn, Xm       upper 64 bits of 64×64 unsigned multiply
 *   SMULL  Xd, Wn, Wm       Xd = sign-extend(Wn) * sign-extend(Wm)  (32×32→64)
 *
 * Each bench uses a 32-deep dependent chain (each result feeds the next
 * operation as its primary input) to measure LATENCY, not throughput.
 * The multiplier constant is chosen to keep results non-trivially bounded.
 */

static uint64_t bench_mul_dep(uint64_t iters) {
    uint64_t acc = 0x9e3779b97f4a7c15ull;
    const uint64_t multiplier = 0x6c62272e07bb0142ull; /* Knuth LCG constant */
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        /* 32 dependent MUL instructions: each output feeds the next multiplicand. */
        asm volatile(
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1"
            : "+r"(acc)
            : "r"(multiplier));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc *= multiplier;
        }
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

/* MADD: Xd = Xa + Xn*Xm.  We use acc as BOTH the accumulator (Xa) and the
 * multiplicand (Xn), with a fixed Xm.  This creates a dependency on the
 * previous result in both the add input and the multiply input. */
static uint64_t bench_madd_dep(uint64_t iters) {
    uint64_t acc = 0x9e3779b97f4a7c15ull;
    const uint64_t multiplier = 0x6c62272e07bb0142ull;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        /* MADD Xd, Xacc, Xmul, Xacc: Xd = Xacc + Xacc*Xmul.
         * Each result feeds both the next multiplicand and accumulator. */
        asm volatile(
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\t"
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\t"
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\t"
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\t"
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\t"
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\t"
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\t"
            "madd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0\n\tmadd %x0, %x0, %x1, %x0"
            : "+r"(acc)
            : "r"(multiplier));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc = acc + acc * multiplier;
        }
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

/* MSUB: Xd = Xa - Xn*Xm.  Same dep structure as MADD but subtraction.
 * On M-series MSUB is a separate execution unit path from MADD — probing
 * both lets us check whether MADD and MSUB share the same latency. */
static uint64_t bench_msub_dep(uint64_t iters) {
    uint64_t acc = 0x9e3779b97f4a7c15ull;
    const uint64_t multiplier = 3ull; /* small odd constant prevents zero collapse */
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        /* MSUB Xd, Xacc, Xmul, Xacc: Xd = Xacc - Xacc*Xmul. */
        asm volatile(
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\t"
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\t"
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\t"
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\t"
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\t"
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\t"
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\t"
            "msub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0\n\tmsub %x0, %x0, %x1, %x0"
            : "+r"(acc)
            : "r"(multiplier));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc = acc - acc * multiplier;
        }
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

/* UMULH: unsigned multiply-high — upper 64 bits of a 64×64 product.
 * Used in 128-bit arithmetic, modular reduction, and crypto (GHASH, Montgomery).
 * The dep chain forces each result into the next multiplicand. */
static uint64_t bench_umulh_dep(uint64_t iters) {
    uint64_t acc = 0x9e3779b97f4a7c15ull;
    const uint64_t multiplier = 0xbf58476d1ce4e5b9ull; /* Murmur3 mix constant */
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\t"
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\t"
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\t"
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\t"
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\t"
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\t"
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\t"
            "umulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1\n\tumulh %x0, %x0, %x1"
            : "+r"(acc)
            : "r"(multiplier));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            /* Emulate UMULH via __uint128_t — not a latency measurement on non-AArch64. */
            acc = (uint64_t)(((__uint128_t)acc * (__uint128_t)multiplier) >> 64);
        }
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

/* SMULL: 32-bit signed multiply long → 64-bit result.
 * Tests the 32→64 multiply path, separate from the 64-bit MUL path on M-series.
 * The dep chain truncates acc to 32 bits before each multiply via the Wn form. */
static uint64_t bench_smull_dep(uint64_t iters) {
    int64_t acc = (int64_t)0x9e3779b9u; /* fits in int32_t range — sign-safe */
    const int64_t multiplier = (int64_t)0x6c62272eu;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        /* SMULL Xd, Wn, Wm: uses lower 32 bits of 64-bit registers.
         * The dep: each 64-bit result feeds back as Wn (lower 32 bits used).
         * This exercises the 32×32→64 multiply unit. */
        asm volatile(
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\t"
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\t"
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\t"
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\t"
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\t"
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\t"
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\t"
            "smull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1\n\tsmull %x0, %w0, %w1"
            : "+r"(acc)
            : "r"(multiplier));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            acc = (int64_t)((int32_t)(acc & 0xffffffff)) * (int64_t)((int32_t)(multiplier & 0xffffffff));
        }
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = (uint64_t)acc;
    return end - start;
}

static uint64_t bench_shuffle_dep(uint64_t iters) {
    uint64_t x = 0x0123456789abcdefull;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            uint64_t mixed = x ^ ((uint64_t)lane * 0x9e3779b97f4a7c15ull);
            uint32_t shift = (lane & 15u) + 1u;
            mixed = (mixed >> shift) | (mixed << (64u - shift));
            x = __builtin_bswap64(mixed);
        }
    }
    end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

static uint64_t bench_add_dep_u32(uint64_t iters) {
    uint32_t x = 1u;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\t"
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\t"
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\t"
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\t"
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\t"
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\t"
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\t"
            "add %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1\n\tadd %w0, %w0, #1"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x = (uint32_t)(x + 1u);
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

static uint64_t bench_add_dep_i64(uint64_t iters) {
    int64_t x = 1;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\t"
            "add %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1\n\tadd %x0, %x0, #1"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x += 1;
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = (uint64_t)x;
    return end - start;
}

static uint64_t bench_add_dep_i16(uint64_t iters) {
    int16_t x = 1;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxth %w0, %w0\n\tadd %w0, %w0, #1\n\tsxth %w0, %w0"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x = (int16_t)(x + 1);
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = (uint16_t)x;
    return end - start;
}

static uint64_t bench_add_dep_i8(uint64_t iters) {
    int8_t x = 1;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tsxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tsxtb %w0, %w0"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x = (int8_t)(x + 1);
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = (uint8_t)x;
    return end - start;
}

static uint64_t bench_add_dep_u16(uint64_t iters) {
    uint16_t x = 1u;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxth %w0, %w0\n\tadd %w0, %w0, #1\n\tuxth %w0, %w0"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x = (uint16_t)(x + 1u);
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

static uint64_t bench_add_dep_u8(uint64_t iters) {
    uint8_t x = 1u;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0\n\t"
            "add %w0, %w0, #1\n\tuxtb %w0, %w0\n\tadd %w0, %w0, #1\n\tuxtb %w0, %w0"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x = (uint8_t)(x + 1u);
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

/* ── Extended / wide-precision dep-chain probes ─────────────────────────────
 *
 * On Apple AArch64: __float128 and _Float128 are NOT available (Apple Clang
 * does not support quad-precision types). long double == float64 (binary64).
 * The "software emulation" fastpaths for >64-bit precision are therefore:
 *
 *   Level 0 – software integer softfp: not measurable here (no type exists)
 *   Level 1 – double-double scalar (Knuth 2-sum): ~3 FP ops/step in FPU
 *   Level 2 – double-double with FMA: error term is exact in 1 FMA → ~2.5 ops/step
 *   Level 3 – NEON float64x2_t: 2×f64 per Q-register, hardware dep chain
 *   Level 4 – NEON float32x4_t FMA: 4×f32 per Q-register, higher throughput
 *   Level 5 – NEON vectorised double-double: 2 (hi,lo) pairs per iteration
 *
 * The key insight: all levels 1-5 stay entirely within the hardware FPU/NEON
 * units. None route through the integer ALU. The 128-bit NEON register IS the
 * "fp128 hardware" for any workload that tolerates double-double rounding.
 */

/* Level 1 — double-double 2-sum (Knuth) dep chain.
 * Each step: s = hi + 1.0; e = 1.0 - (s - hi); hi = s; lo += e
 * 4 dependent FP ops per step. The 'lo' accumulates exact round-off.
 * Represents the minimum software overhead for >64-bit precision in the FPU.
 * Written in C so the compiler emits a tight scalar dep chain. */
static uint64_t bench_dd_twosum_dep(uint64_t iters) {
    volatile double hi = 1.0;
    volatile double lo = 0.0;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        /* 8 unrolled 2-sum steps — each is 4 dependent FADD/FSUB ops */
        double s, e, h = hi, l = lo;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        s = h + 1.0; e = 1.0 - (s - h); h = s; l += e;
        hi = h; lo = l;
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = hi + lo;
    return end - start;
}

/* Level 2 — double-double 2-sum with FMA dep chain.
 * Error term via FMA: e = fma(1.0, hi, -s) + 1.0 — exact in 1 instruction.
 * Reduces the dep chain to 3 FP ops/step vs 4 for plain 2-sum.
 * FMA is the hardware-accelerated fastpath for double-double on AGX/Apple. */
static uint64_t bench_dd_twosum_fma_dep(uint64_t iters) {
    volatile double hi = 1.0;
    volatile double lo = 0.0;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        double s, e, h = hi, l = lo;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        s = h + 1.0; e = __builtin_fma(1.0, h, -s) + 1.0; h = s; l += e;
        hi = h; lo = l;
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = hi + lo;
    return end - start;
}

/* Level 3 — NEON float64x2_t vectorised dep chain.
 * Two float64s in one 128-bit Q register, processed together per instruction.
 * Each vaddq_f64 is 1 NEON op on 2 doubles — same latency as scalar fadd.
 * ops_per_iter = APPLE_RE_UNROLL (32 vaddq instructions). */
static uint64_t bench_f64x2_vadd_dep(uint64_t iters) {
#if defined(__aarch64__)
    float64x2_t v = vdupq_n_f64(1.0);
    const float64x2_t one = vdupq_n_f64(1.0);
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
        v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one); v = vaddq_f64(v, one);
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = vgetq_lane_f64(v, 0);
    return end - start;
#else
    (void)iters; return 0;
#endif
}

/* Level 4 — NEON float32x4_t FMA dep chain.
 * Four f32s per Q-register: v = vfmaq_f32(v, v, one) dep chain.
 * Measures 4-wide f32 FMA latency — fastpath for packing 4 floats as a
 * higher-dynamic-range accumulator (Kahan or compensated reduction). */
static uint64_t bench_f32x4_vfma_dep(uint64_t iters) {
#if defined(__aarch64__)
    float32x4_t v = vdupq_n_f32(1.0f);
    const float32x4_t one = vdupq_n_f32(1.0f);
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
        v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one); v = vfmaq_f32(v, v, one);
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = (double)vgetq_lane_f32(v, 0);
    return end - start;
#else
    (void)iters; return 0;
#endif
}

/* Level 5 — NEON vectorised double-double 2-sum (2 pairs per cycle).
 * float64x2_t hi holds {hi0,hi1}, lo holds {lo0,lo1}: 2 independent dd-pairs.
 * Each step: s=hi+1; e=1-(s-hi); hi=s; lo+=e — entirely in NEON 64×2 units.
 * This is the peak fastpath: 2 extended-precision values advanced per instruction.
 * The 128-bit NEON register IS fp128-equivalent precision for this workload. */
static uint64_t bench_f64x2_dd_twosum_dep(uint64_t iters) {
#if defined(__aarch64__)
    float64x2_t hi = vdupq_n_f64(1.0);
    float64x2_t lo = vdupq_n_f64(0.0);
    const float64x2_t one = vdupq_n_f64(1.0);
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        /* 8 vectorised 2-sum steps: each is 4 vaddq/vsubq ops on 2 dd-pairs */
        float64x2_t s, e;
#define DD2_STEP \
        s = vaddq_f64(hi, one); \
        e = vsubq_f64(one, vsubq_f64(s, hi)); \
        hi = s; lo = vsubq_f64(e, lo);
        DD2_STEP DD2_STEP DD2_STEP DD2_STEP
        DD2_STEP DD2_STEP DD2_STEP DD2_STEP
#undef DD2_STEP
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = vgetq_lane_f64(hi, 0) + vgetq_lane_f64(lo, 0);
    return end - start;
#else
    (void)iters; return 0;
#endif
}

/* Level 5 throughput — 4 independent NEON DD accumulators.
 * Breaks the dep chain across 4 float64x2_t (hi,lo) pairs = 8 independent
 * scalar DD values in flight simultaneously.  Reveals peak DD issue bandwidth
 * vs the latency-bound ~3.3 cycle result from bench_f64x2_dd_twosum_dep.
 * 8 rounds × 4 accumulators = 32 DD pair steps per iter (ops_per_iter = 32). */
static uint64_t bench_f64x2_dd_throughput(uint64_t iters) {
#if defined(__aarch64__)
    float64x2_t hi0 = vdupq_n_f64(1.0), lo0 = vdupq_n_f64(0.0);
    float64x2_t hi1 = vdupq_n_f64(2.0), lo1 = vdupq_n_f64(0.0);
    float64x2_t hi2 = vdupq_n_f64(3.0), lo2 = vdupq_n_f64(0.0);
    float64x2_t hi3 = vdupq_n_f64(4.0), lo3 = vdupq_n_f64(0.0);
    const float64x2_t one = vdupq_n_f64(1.0);
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        float64x2_t s0, e0, s1, e1, s2, e2, s3, e3;
        /* 4 independent chains interleaved — OOO can pipeline all simultaneously */
#define DD2_TP(H, L, S, E) \
        (S) = vaddq_f64((H), one); \
        (E) = vsubq_f64(one, vsubq_f64((S), (H))); \
        (H) = (S); (L) = vsubq_f64((E), (L));
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
        DD2_TP(hi0,lo0,s0,e0) DD2_TP(hi1,lo1,s1,e1) DD2_TP(hi2,lo2,s2,e2) DD2_TP(hi3,lo3,s3,e3)
#undef DD2_TP
    }
    uint64_t end = apple_re_now_ns();
    /* Sink all accumulators to prevent DCE */
    g_f64_sink = vgetq_lane_f64(hi0, 0) + vgetq_lane_f64(hi1, 0) +
                 vgetq_lane_f64(hi2, 0) + vgetq_lane_f64(hi3, 0) +
                 vgetq_lane_f64(lo0, 0) + vgetq_lane_f64(lo1, 0);
    return end - start;
#else
    (void)iters; return 0;
#endif
}

/* NEON DD FMA throughput probe — 4 independent NEON DD accumulators using vfmaq_f64
 * for the error term, replacing the original two-vsubq form.
 * FMA form: E = fma(H, -1, S) + one = H - S + one = one - (S - H).
 * This is mathematically identical to the plain vsub version but routes the error
 * computation through the FMA pipeline (fmla, latency 4cy) instead of fsub (3cy).
 * If FMA is genuinely neutral for DD (confirmed by scalar levels 1 and 2), the
 * throughput here should be ≈ bench_f64x2_dd_throughput (≈ 0.280 ns/step).
 * 8 rounds × 4 accumulators = 32 DD pair steps per iter.  ops_per_iter = 32. */
static uint64_t bench_neon_dd_fma_throughput(uint64_t iters) {
#if defined(__aarch64__)
    float64x2_t hi0 = vdupq_n_f64(1.0), lo0 = vdupq_n_f64(0.0);
    float64x2_t hi1 = vdupq_n_f64(2.0), lo1 = vdupq_n_f64(0.0);
    float64x2_t hi2 = vdupq_n_f64(3.0), lo2 = vdupq_n_f64(0.0);
    float64x2_t hi3 = vdupq_n_f64(4.0), lo3 = vdupq_n_f64(0.0);
    const float64x2_t one     = vdupq_n_f64(1.0);
    const float64x2_t neg_one = vdupq_n_f64(-1.0);
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        float64x2_t s0, e0, s1, e1, s2, e2, s3, e3;
        /* FMA form: E = fma(H, -1, S) + one  = H - S + one = one - (S-H) */
#define DD2_FMA(H, L, S, E) \
        (S) = vaddq_f64((H), one); \
        (E) = vaddq_f64(vfmaq_f64((H), neg_one, (S)), one); \
        (H) = (S); (L) = vsubq_f64((E), (L));
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
        DD2_FMA(hi0,lo0,s0,e0) DD2_FMA(hi1,lo1,s1,e1) DD2_FMA(hi2,lo2,s2,e2) DD2_FMA(hi3,lo3,s3,e3)
#undef DD2_FMA
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = vgetq_lane_f64(hi0, 0) + vgetq_lane_f64(hi1, 0) +
                 vgetq_lane_f64(hi2, 0) + vgetq_lane_f64(hi3, 0) +
                 vgetq_lane_f64(lo0, 0) + vgetq_lane_f64(lo1, 0);
    return end - start;
#else
    (void)iters; return 0;
#endif
}

/* Veltkamp product-splitting dep chain.
 * Splits x into a (xhi, xlo) pair using the Veltkamp constant 2^27+1 = 134217729.
 * Each split has the critical path: x → p=x*C → q=p−x → xhi=p−q → x=xhi+delta
 * = 4 dependent FP ops (fmul, fsub, fsub, fadd) per step, expected ~4 cycles.
 * This is the first half of a full DD multiply (Dekker–Veltkamp product).
 * ops_per_iter = 8 (matching dd_twosum_dep convention). */
static uint64_t bench_veltkamp_split_dep(uint64_t iters) {
    double x = 1.000000001;
    double xhi = 0.0;
    double xlo = 0.0;
    const double veltkamp_c = 134217729.0; /* 2^27 + 1 */
    const double step_delta = 1.0e-13;    /* keeps x non-trivial between steps */
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        /* dep chain per step: x→p→q→xhi→x  (4 hops, ~4 cycles) */
#define VK_STEP(X, XHI, XLO) do { \
            double _p = (X) * veltkamp_c; \
            double _q = _p - (X); \
            (XHI) = _p - _q; \
            (XLO) = (X) - (XHI); \
            (X) = (XHI) + step_delta; \
        } while (0)
        VK_STEP(x, xhi, xlo); VK_STEP(x, xhi, xlo); VK_STEP(x, xhi, xlo); VK_STEP(x, xhi, xlo);
        VK_STEP(x, xhi, xlo); VK_STEP(x, xhi, xlo); VK_STEP(x, xhi, xlo); VK_STEP(x, xhi, xlo);
#undef VK_STEP
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = x + xhi + xlo;
    return end - start;
}

/* T13-A — 4 independent scalar double-double pairs (throughput probe, no NEON).
 * Breaks the serial dep chain of a single DD accumulator by using 4 independent
 * (hi,lo) pairs interleaved in the loop body.  The M-series OOO engine can issue
 * all 4 pairs simultaneously, revealing peak scalar DD throughput.
 * Each loop iteration performs exactly 4 DD steps (one per pair), so ops_per_iter=4.
 * Compare against f64x2_dd_twosum_dep (latency-bound, ~1.348 ns/step). */
static uint64_t bench_f64x2_dd_tp4_dep(uint64_t iters) {
    volatile uint64_t init_seed = 1;  /* prevent DCE of initial values */
    double hi0 = 1.0 + (double)(init_seed + 0u) * 1e-9;
    double lo0 = 0.0;
    double hi1 = 1.0 + (double)(init_seed + 1u) * 1e-9;
    double lo1 = 0.0;
    double hi2 = 1.0 + (double)(init_seed + 2u) * 1e-9;
    double lo2 = 0.0;
    double hi3 = 1.0 + (double)(init_seed + 3u) * 1e-9;
    double lo3 = 0.0;
    const double delta = 1.0;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        double s, e;
        /* pair 0 — independent dep chain */
        s = hi0 + delta;  e = delta - (s - hi0);  hi0 = s;  lo0 += e;
        /* pair 1 — independent dep chain */
        s = hi1 + delta;  e = delta - (s - hi1);  hi1 = s;  lo1 += e;
        /* pair 2 — independent dep chain */
        s = hi2 + delta;  e = delta - (s - hi2);  hi2 = s;  lo2 += e;
        /* pair 3 — independent dep chain */
        s = hi3 + delta;  e = delta - (s - hi3);  hi3 = s;  lo3 += e;
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = (*(uint64_t*)&hi0) ^ (*(uint64_t*)&hi1) ^
                 (*(uint64_t*)&hi2) ^ (*(uint64_t*)&hi3);
    g_f64_sink = lo0 + lo1 + lo2 + lo3;
    return end - start;
}

/* T13-B — 4 independent scalar DD pairs, FMA error term.
 * Identical to f64x2_dd_tp4_dep but uses fma(1.0, hi, delta-s) for the exact
 * error term.  On M-series the FMA latency is ~4 cy vs fsub ~3 cy, but FMA has
 * higher throughput; this probe isolates whether FMA buys anything for 4-wide
 * scalar DD throughput.
 * ops_per_iter = 4 (one step per pair per iteration). */
static uint64_t bench_f64x2_dd_fma_tp4_dep(uint64_t iters) {
    volatile uint64_t init_seed = 1;
    double hi0 = 1.0 + (double)(init_seed + 0u) * 1e-9;
    double lo0 = 0.0;
    double hi1 = 1.0 + (double)(init_seed + 1u) * 1e-9;
    double lo1 = 0.0;
    double hi2 = 1.0 + (double)(init_seed + 2u) * 1e-9;
    double lo2 = 0.0;
    double hi3 = 1.0 + (double)(init_seed + 3u) * 1e-9;
    double lo3 = 0.0;
    const double delta = 1.0;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        double s, e;
        /* pair 0 — FMA error: e = fma(1.0, hi, delta - s) */
        s = hi0 + delta;  e = __builtin_fma(1.0, hi0, delta - s);  hi0 = s;  lo0 += e;
        /* pair 1 */
        s = hi1 + delta;  e = __builtin_fma(1.0, hi1, delta - s);  hi1 = s;  lo1 += e;
        /* pair 2 */
        s = hi2 + delta;  e = __builtin_fma(1.0, hi2, delta - s);  hi2 = s;  lo2 += e;
        /* pair 3 */
        s = hi3 + delta;  e = __builtin_fma(1.0, hi3, delta - s);  hi3 = s;  lo3 += e;
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = (*(uint64_t*)&hi0) ^ (*(uint64_t*)&hi1) ^
                 (*(uint64_t*)&hi2) ^ (*(uint64_t*)&hi3);
    g_f64_sink = lo0 + lo1 + lo2 + lo3;
    return end - start;
}

/* T13-C — Veltkamp DS-multiply dep chain (full product split, scalar double).
 * Each step: Veltkamp-split a into (a_hi, a_lo), then compute DS product of
 * a * step_factor — the exact Dekker–Veltkamp multiply for double-double output.
 * Critical dep path: a → prod_hi → a (via Veltkamp split + DS mul) ≈ 8 FP ops.
 * Compare against Metal GPU baseline: 11.739 ns/step.
 * ops_per_iter = 8 (8 DS-multiply steps per loop iteration). */
static uint64_t bench_veltkamp_ds_mul_dep(uint64_t iters) {
    double a = 1.000000001;
    double b_lo = 0.0;
    const double step_factor = 1.0 + 1e-9;
    const double veltkamp_constant = 134217729.0;  /* 2^27 + 1 */
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#define VK_DS_STEP(A, B_LO) do { \
            double _gamma, _a_hi, _a_lo, _prod_hi, _prod_err; \
            _gamma   = veltkamp_constant * (A); \
            _a_hi    = _gamma - (_gamma - (A)); \
            _a_lo    = (A) - _a_hi; \
            _prod_hi = (A) * step_factor; \
            _prod_err = __builtin_fma((A), step_factor, -_prod_hi); \
            (A)      = _prod_hi + _a_lo * step_factor; \
            (B_LO)   = _prod_err + (B_LO) * step_factor; \
        } while (0)
        VK_DS_STEP(a, b_lo); VK_DS_STEP(a, b_lo); VK_DS_STEP(a, b_lo); VK_DS_STEP(a, b_lo);
        VK_DS_STEP(a, b_lo); VK_DS_STEP(a, b_lo); VK_DS_STEP(a, b_lo); VK_DS_STEP(a, b_lo);
#undef VK_DS_STEP
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = a + b_lo;
    return end - start;
}

/* Unsigned nibble-unpack dep chain: add #1 then mask to 4 bits.
 * Models the minimum per-element cost of working with u4 nibble-packed data
 * on AArch64 where no native 4-bit integer arithmetic exists.
 * Each op pair = add + and #0xf (unsigned nibble clamp). */
static uint64_t bench_nibble_unpack_u4(uint64_t iters) {
    uint32_t x = 1u;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf\n\t"
            "add %w0, %w0, #1\n\tand %w0, %w0, #0xf"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x = (x + 1u) & 0xfu;
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = x;
    return end - start;
}

/* Signed nibble-unpack dep chain: add #1 then sbfx to 4 bits.
 * Models signed int4 nibble extraction cost on AArch64.
 * sbfx wx, wx, #0, #4 sign-extends bits [3:0] — the canonical int4 unpack. */
static uint64_t bench_nibble_unpack_i4(uint64_t iters) {
    int32_t x = 1;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            "add %w0, %w0, #1\n\tsbfx %w0, %w0, #0, #4\n\t"
            : "+r"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            /* Software sign-extension of 4-bit value: shift left 28, then arithmetic right 28 */
            x = (int32_t)((uint32_t)(x + 1) << 28) >> 28;
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_u64_sink = (uint32_t)x;
    return end - start;
}

/* ── T9: FP16 scalar, INT32/INT64 multiply, transcendental, CRC32, CLZ probes ─
 *
 * fadd_dep_f16      — scalar _Float16 add dep chain (FADD H-reg), 8 ops/iter
 * fmul_dep_f16      — scalar _Float16 multiply dep chain (FMUL H-reg), 8 ops/iter
 * imul_dep_u32      — 32-bit unsigned MUL dep chain (MUL Wd), 8 ops/iter
 * imul_dep_u64      — 64-bit unsigned MUL dep chain (MUL Xd), 8 ops/iter
 * fsqrt_dep_f64     — sqrt() dep chain (FSQRT Dd), 2 ops/iter
 * flog_dep_f64      — log() dep chain, 2 ops/iter
 * fexp_dep_f64      — exp() dep chain, 2 ops/iter
 * crc32_dep         — CRC32W dep chain (__builtin_arm_crc32w), 4 ops/iter
 * clz_dep_u64       — CLZ dep chain (__builtin_clzll), 4 ops/iter
 */

/* fadd_dep_f16: scalar _Float16 FADD dep chain, 8 per iter.
 * Uses FADD H-register instructions on AArch64 FEAT_FP16.
 * NOTE: bench_fadd_f16_dep already exists (32 unroll). This uses 8-unroll
 * variant to match the ops_per_iter = 8 convention for T9 probes. */
static uint64_t bench_fadd_dep_f16(uint64_t iters) {
    _Float16 acc = (_Float16)1.001f;
    const _Float16 delta = (_Float16)1.0f;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__) && defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC)
        asm volatile(
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\t"
            "fadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1\n\tfadd %h0, %h0, %h1"
            : "+w"(acc)
            : "w"(delta));
#else
        acc += delta; acc += delta; acc += delta; acc += delta;
        acc += delta; acc += delta; acc += delta; acc += delta;
#endif
    }
    end = apple_re_now_ns();
    uint16_t bits_f16;
    __builtin_memcpy(&bits_f16, &acc, sizeof(bits_f16));
    g_u64_sink = (uint64_t)bits_f16;
    return end - start;
}

/* fmul_dep_f16: scalar _Float16 FMUL dep chain, 8 per iter. */
static uint64_t bench_fmul_dep_f16(uint64_t iters) {
    _Float16 acc = (_Float16)1.0001f;
    const _Float16 step_val = (_Float16)1.00001f;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__) && defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC)
        asm volatile(
            "fmul %h0, %h0, %h1\n\tfmul %h0, %h0, %h1\n\tfmul %h0, %h0, %h1\n\tfmul %h0, %h0, %h1\n\t"
            "fmul %h0, %h0, %h1\n\tfmul %h0, %h0, %h1\n\tfmul %h0, %h0, %h1\n\tfmul %h0, %h0, %h1"
            : "+w"(acc)
            : "w"(step_val));
#else
        acc *= step_val; acc *= step_val; acc *= step_val; acc *= step_val;
        acc *= step_val; acc *= step_val; acc *= step_val; acc *= step_val;
#endif
    }
    end = apple_re_now_ns();
    uint16_t bits_f16;
    __builtin_memcpy(&bits_f16, &acc, sizeof(bits_f16));
    g_u64_sink = (uint64_t)bits_f16;
    return end - start;
}

/* imul_dep_u32: 32-bit unsigned MUL dep chain, 8 per iter.
 * Uses MUL Wd, Wn, Wm (lower 32 bits of product). */
static uint64_t bench_imul_dep_u32(uint64_t iters) {
    uint32_t acc = 0x9e3779b9u;
    /* Odd multiplier keeps result non-trivially non-zero */
    const uint32_t step_multiplier = 1664525u;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "mul %w0, %w0, %w1\n\tmul %w0, %w0, %w1\n\tmul %w0, %w0, %w1\n\tmul %w0, %w0, %w1\n\t"
            "mul %w0, %w0, %w1\n\tmul %w0, %w0, %w1\n\tmul %w0, %w0, %w1\n\tmul %w0, %w0, %w1"
            : "+r"(acc)
            : "r"(step_multiplier));
#else
        acc *= step_multiplier; acc *= step_multiplier;
        acc *= step_multiplier; acc *= step_multiplier;
        acc *= step_multiplier; acc *= step_multiplier;
        acc *= step_multiplier; acc *= step_multiplier;
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = (uint64_t)acc;
    return end - start;
}

/* imul_dep_u64: 64-bit unsigned MUL dep chain, 8 per iter.
 * Uses MUL Xd, Xn, Xm.  Same multiplier as bench_mul_dep for comparison. */
static uint64_t bench_imul_dep_u64(uint64_t iters) {
    uint64_t acc = 0x9e3779b97f4a7c15ull;
    const uint64_t step_multiplier = 0x6c62272e07bb0142ull;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\t"
            "mul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1\n\tmul %x0, %x0, %x1"
            : "+r"(acc)
            : "r"(step_multiplier));
#else
        acc *= step_multiplier; acc *= step_multiplier;
        acc *= step_multiplier; acc *= step_multiplier;
        acc *= step_multiplier; acc *= step_multiplier;
        acc *= step_multiplier; acc *= step_multiplier;
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

/* fsqrt_dep_f64: sqrt() dep chain.  2 ops per iter.
 * Keeps acc in (1, 2): sqrt(x) < x for x > 1, so adding 1.0 resets range. */
static uint64_t bench_fsqrt_dep_f64(uint64_t iters) {
    double acc = 1.5;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        acc = sqrt(acc);
        acc = sqrt(acc + 1.0);
    }
    end = apple_re_now_ns();
    g_f64_sink = acc;
    return end - start;
}

/* flog_dep_f64: log() dep chain.  2 ops per iter.
 * +1.0 inside the loop keeps argument > 1 so log() is positive and bounded. */
static uint64_t bench_flog_dep_f64(uint64_t iters) {
    double acc = 1.5;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        acc = log(acc + 1.0);
        acc = log(acc + 1.0);
    }
    end = apple_re_now_ns();
    g_f64_sink = acc;
    return end - start;
}

/* fexp_dep_f64: exp() dep chain.  2 ops per iter.
 * Alternating negation prevents overflow/underflow: exp(-x) for x near 1
 * stays in (0, e^-1) ≈ (0, 0.368), then exp(-0.368) ≈ 0.692, oscillates. */
static uint64_t bench_fexp_dep_f64(uint64_t iters) {
    double acc = 0.0001;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        acc = exp(-acc);
        acc = exp(-acc);
    }
    end = apple_re_now_ns();
    g_f64_sink = acc;
    return end - start;
}

/* crc32_dep: CRC32W dep chain via inline asm / __builtin_arm_crc32w.
 * 4 ops per iter.  Requires FEAT_CRC32 (present on all Apple M-series). */
static uint64_t bench_crc32_dep(uint64_t iters) {
    uint32_t acc = 0xdeadbeef;
    const uint32_t crc_data = 0x12345678u;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
        asm volatile(
            "crc32w %w0, %w0, %w1\n\tcrc32w %w0, %w0, %w1\n\t"
            "crc32w %w0, %w0, %w1\n\tcrc32w %w0, %w0, %w1"
            : "+r"(acc)
            : "r"(crc_data));
#else
        acc = __builtin_arm_crc32w(acc, crc_data);
        acc = __builtin_arm_crc32w(acc, crc_data);
        acc = __builtin_arm_crc32w(acc, crc_data);
        acc = __builtin_arm_crc32w(acc, crc_data);
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = (uint64_t)acc;
    return end - start;
}

/* clz_dep_u64: CLZ dep chain.  4 ops per iter.
 * +1 after each CLZ keeps acc non-zero (clzll(0) is UB). */
static uint64_t bench_clz_dep_u64(uint64_t iters) {
    uint64_t acc = 0x9e3779b97f4a7c15ull;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        asm volatile(
            "clz %x0, %x0\n\tadd %x0, %x0, #1\n\t"
            "clz %x0, %x0\n\tadd %x0, %x0, #1\n\t"
            "clz %x0, %x0\n\tadd %x0, %x0, #1\n\t"
            "clz %x0, %x0\n\tadd %x0, %x0, #1"
            : "+r"(acc));
#else
        acc = (uint64_t)__builtin_clzll(acc) + 1u;
        acc = (uint64_t)__builtin_clzll(acc) + 1u;
        acc = (uint64_t)__builtin_clzll(acc) + 1u;
        acc = (uint64_t)__builtin_clzll(acc) + 1u;
#endif
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

/* Long-double dep chain: on Apple AArch64 `long double` is 64-bit (same as double).
 * This probe verifies that — it should produce the same latency as fadd_dep_f64.
 * On x86 it would be 80-bit x87 extended precision; on AArch64 it is IEEE 754 binary64. */
static uint64_t bench_fadd_ld_dep(uint64_t iters) {
    long double x = 1.0L;
    uint64_t start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
#if defined(__aarch64__)
        /* long double == double on AArch64 ABI — these are plain double fadd ops.
         * 32 ops matching APPLE_RE_UNROLL so ns/op is directly comparable to fadd_dep_f64. */
        asm volatile(
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\t"
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\t"
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\t"
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\t"
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\t"
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\t"
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\t"
            "fadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0\n\tfadd %d0, %d0, %d0"
            : "+w"(x));
#else
        for (int lane = 0; lane < APPLE_RE_UNROLL; ++lane) {
            x = x + x;
        }
#endif
    }
    uint64_t end = apple_re_now_ns();
    g_f64_sink = (double)x;
    return end - start;
}

static uint64_t bench_atomic_add_dep(uint64_t iters) {
    uint64_t acc = 0;
    uint64_t start;
    uint64_t end;

    atomic_store_explicit(&g_atomic_counter, 1, memory_order_relaxed);
    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_ATOMIC_UNROLL; ++lane) {
            acc = atomic_fetch_add_explicit(&g_atomic_counter, 1, memory_order_relaxed);
        }
    }
    end = apple_re_now_ns();
    g_u64_sink = acc;
    return end - start;
}

static uint64_t bench_transcendental_dep(uint64_t iters) {
    double x = 0.123456789;
    uint64_t start;
    uint64_t end;

    start = apple_re_now_ns();
    for (uint64_t i = 0; i < iters; ++i) {
        for (uint32_t lane = 0; lane < APPLE_RE_TRANSC_UNROLL; ++lane) {
            const double t = sin(x + (double)(lane + 1u) * 0.001);
            x = cos(t + x);
        }
    }
    end = apple_re_now_ns();
    g_f64_sink = x;
    return end - start;
}

static void print_result(const char *name, uint64_t elapsed_ns, uint64_t iters, uint32_t ops_per_iter, bool csv) {
    const double ops = (double)iters * (double)ops_per_iter;
    const double ns_per_op = (ops > 0.0) ? ((double)elapsed_ns / ops) : 0.0;
    const double ops_per_ns = (elapsed_ns > 0) ? (ops / (double)elapsed_ns) : 0.0;
    const double gops = ops_per_ns;

    if (csv) {
        printf("%s,%" PRIu64 ",%u,%" PRIu64 ",%.6f,%.6f\n",
               name, iters, ops_per_iter, elapsed_ns, ns_per_op, gops);
        return;
    }

    printf("%-24s elapsed_ns=%" PRIu64 " ns/op=%.6f approx_Gops=%.6f\n",
           name, elapsed_ns, ns_per_op, gops);
}

static const struct probe_def g_probes[] = {
    {"add_dep_u64",               bench_add_dep,                 APPLE_RE_UNROLL},
    {"add_dep_u32",               bench_add_dep_u32,             APPLE_RE_UNROLL},
    {"add_dep_i64",               bench_add_dep_i64,             APPLE_RE_UNROLL},
    {"add_dep_i16",               bench_add_dep_i16,             APPLE_RE_UNROLL},
    {"add_dep_i8",                bench_add_dep_i8,              APPLE_RE_UNROLL},
    {"add_dep_u16",               bench_add_dep_u16,             APPLE_RE_UNROLL},
    {"add_dep_u8",                bench_add_dep_u8,              APPLE_RE_UNROLL},
    {"fadd_dep_f64",              bench_fadd_dep,                APPLE_RE_UNROLL},
    {"fadd_dep_f32",              bench_fadd_f32_dep,            APPLE_RE_UNROLL},
    {"fmadd_dep_f64",             bench_fmadd_dep,               APPLE_RE_UNROLL},
    {"bf16_f32_roundtrip_proxy",  bench_bfloat16_roundtrip_proxy,APPLE_RE_UNROLL},
    {"fcvt_f32_f16_roundtrip",    bench_fcvt_f32_to_f16_dep,     32u},
    {"fadd_dep_f16_scalar",       bench_fadd_f16_dep,            APPLE_RE_UNROLL},
    {"fmla_dep_f16x8_simd",       bench_fmla_f16x8_dep,         APPLE_RE_UNROLL},
    {"mul_dep_u64",               bench_mul_dep,                 APPLE_RE_UNROLL},
    {"madd_dep_u64",              bench_madd_dep,                APPLE_RE_UNROLL},
    {"msub_dep_u64",              bench_msub_dep,                APPLE_RE_UNROLL},
    {"umulh_dep_u64",             bench_umulh_dep,               APPLE_RE_UNROLL},
    {"smull_dep_i32_to_i64",      bench_smull_dep,               APPLE_RE_UNROLL},
    {"load_store_chain_u64",      bench_load_store_chain,        APPLE_RE_LOAD_STORE_UNROLL * 3u},
    {"shuffle_lane_cross_u64",    bench_shuffle_dep,             APPLE_RE_UNROLL},
    {"atomic_add_relaxed_u64",    bench_atomic_add_dep,          APPLE_RE_ATOMIC_UNROLL},
    {"transcendental_sin_cos_f64",bench_transcendental_dep,      APPLE_RE_TRANSC_UNROLL * 2u},
    {"nibble_unpack_u4",          bench_nibble_unpack_u4,        APPLE_RE_UNROLL},
    {"nibble_unpack_i4",          bench_nibble_unpack_i4,        APPLE_RE_UNROLL},
    {"fadd_dep_ld_aarch64",       bench_fadd_ld_dep,             APPLE_RE_UNROLL},
    /* extended/wide precision fastpath ladder */
    {"dd_twosum_dep",             bench_dd_twosum_dep,           8u},
    {"dd_twosum_fma_dep",         bench_dd_twosum_fma_dep,       8u},
    {"f64x2_vadd_dep",            bench_f64x2_vadd_dep,          APPLE_RE_UNROLL},
    {"f32x4_vfma_dep",            bench_f32x4_vfma_dep,          APPLE_RE_UNROLL},
    {"f64x2_dd_twosum_dep",       bench_f64x2_dd_twosum_dep,     8u},
    {"f64x2_dd_throughput",       bench_f64x2_dd_throughput,     32u},
    {"neon_dd_fma_throughput",    bench_neon_dd_fma_throughput,  32u},
    {"veltkamp_split_dep",        bench_veltkamp_split_dep,      8u},
    /* T13: scalar DD throughput (4 independent pairs) + DS-multiply dep chain */
    {"f64x2_dd_tp4_dep",          bench_f64x2_dd_tp4_dep,        4u},
    {"f64x2_dd_fma_tp4_dep",      bench_f64x2_dd_fma_tp4_dep,    4u},
    {"veltkamp_ds_mul_dep",       bench_veltkamp_ds_mul_dep,      8u},
    /* T9: FP16 scalar, INT32/INT64 multiply, transcendental, CRC32, CLZ */
    {"fadd_dep_f16",              bench_fadd_dep_f16,             8u},
    {"fmul_dep_f16",              bench_fmul_dep_f16,             8u},
    {"imul_dep_u32",              bench_imul_dep_u32,             8u},
    {"imul_dep_u64",              bench_imul_dep_u64,             8u},
    {"fsqrt_dep_f64",             bench_fsqrt_dep_f64,            2u},
    {"flog_dep_f64",              bench_flog_dep_f64,             2u},
    {"fexp_dep_f64",              bench_fexp_dep_f64,             2u},
    {"crc32_dep",                 bench_crc32_dep,                4u},
    {"clz_dep_u64",               bench_clz_dep_u64,              4u},
};

int main(int argc, char **argv) {
    uint64_t iters = 1000000;
    bool csv = false;
    const char *probe_filter = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--iters") == 0 && i + 1 < argc) {
            iters = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--probe") == 0 && i + 1 < argc) {
            probe_filter = argv[++i];
        } else if (strcmp(argv[i], "--csv") == 0) {
            csv = true;
        } else if (strcmp(argv[i], "--list-probes") == 0) {
            for (size_t j = 0; j < (sizeof(g_probes) / sizeof(g_probes[0])); ++j) {
                puts(g_probes[j].name);
            }
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [--iters N] [--probe NAME] [--list-probes] [--csv]\n", argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (csv) {
        printf("probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops\n");
    }

    bool ran_any = false;
    for (size_t i = 0; i < (sizeof(g_probes) / sizeof(g_probes[0])); ++i) {
        const struct probe_def *probe = &g_probes[i];
        if (probe_filter != NULL && strcmp(probe_filter, probe->name) != 0) {
            continue;
        }
        ran_any = true;
        print_result(probe->name, probe->fn(iters), iters, probe->ops_per_iter, csv);
    }

    if (!ran_any) {
        fprintf(stderr, "no probe matched --probe %s\n", probe_filter ? probe_filter : "(null)");
        return 1;
    }

    return 0;
}
