#include <metal_stdlib>

using namespace metal;

// AGX FP32 FMA latency isolation — serial dep chain on fma(a, b, c).
//
// Measures genuine fma(acc, step, delta) latency where all inputs are
// runtime-variable (gid-seeded). The fma(a,b,c) → a*b+c instruction is
// a single-issue op on most GPU architectures; its latency may differ
// from fadd (1.695 ns) or fmul (1.347 ns) measured separately.
//
// Hypothesis: AGX FMA latency ≈ max(fmul_lat, fadd_lat) = fadd_lat = 1.695 ns
//             (since fma completes multiply-accumulate in one pass).
//             Alternatively, FMA may have its own pipeline depth.
//
// Critical path per step: fma(acc, step, delta) — single op, serial.
// 8 unrolled fma steps per outer iteration.
//
// With baseline overhead C = 3,153,956 ns and iters=100000:
//   gpu_ns = C + 100000 × 8 × fma_lat_ns
//   fma_lat_ns = (gpu_ns - C) / 800000
//
// Compare with:
//   probe_fadd8_lat:  gpu_ns = 4,510,334  → fadd = 1.695 ns/step
//   probe_fmul8_lat:  gpu_ns = 4,231,204  → fmul = 1.347 ns/step
//   probe_fma_lat:    gpu_ns = ?
//
// ops_per_iter = 8  (serial fma chain steps)
kernel void probe_fma_lat(device float *out     [[buffer(0)]],
                           constant uint &iters  [[buffer(1)]],
                           uint gid              [[thread_position_in_grid]]) {
    // Runtime-variable operands — not compile-time constants — prevent folding.
    float acc   = float(gid + 1u) * 1.0e-6f + 1.0f;
    float step  = float(gid + 1u) * 1.0e-9f + 1.000001f;   // ≈1, varies per thread
    float delta = float(gid + 1u) * 1.0e-10f + 1.0e-7f;    // small positive bias

    for (uint i = 0; i < iters; ++i) {
        #pragma clang fp reassociate(off)
        {
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
            acc = fma(acc, step, delta);
        }
    }

    out[gid] = acc;
}
