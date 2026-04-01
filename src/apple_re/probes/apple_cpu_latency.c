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
    {"fadd_dep_f64",              bench_fadd_dep,                APPLE_RE_UNROLL},
    {"fmadd_dep_f64",             bench_fmadd_dep,               APPLE_RE_UNROLL},
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
