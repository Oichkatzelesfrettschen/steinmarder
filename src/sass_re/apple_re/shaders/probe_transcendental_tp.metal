#include <metal_stdlib>
using namespace metal;

// H1a: AGX transcendental unit throughput probes.
//
// Measures dep-chain latency for hardware EXP2, LOG2, and SQRT.
// Philip Turner: EXP2_32/LOG2_32 adj_lat ~4.31 cyc (TP=4), SQRT32 ~8.57–11.13 cyc (TP=8).
// GPU hardware transcendentals are dramatically faster than CPU libm (log CPU = 46 cyc).
//
// Design: 8 independent dep chains to suppress noise; tree-reduce to out to prevent DCE.
// ops_per_iter = 8 for all kernels.
//
// Build:
//   xcrun -sdk macosx metal -O2 -o probe_transcendental_tp.air probe_transcendental_tp.metal
//   xcrun -sdk macosx metallib -o probe_transcendental_tp.metallib probe_transcendental_tp.air

// --- EXP2 dep-chain (8 independent chains) ---
// Each chain: a_k = exp2(a_k)
// ops_per_iter = 8
kernel void probe_exp2_lat(
    device float* out        [[buffer(0)]],
    constant uint& n_iters   [[buffer(1)]],
    uint gid                 [[thread_position_in_grid]])
{
    float a0 = 0.5f + float(gid) * 1.0e-6f;
    float a1 = a0 + 1.0e-4f;
    float a2 = a0 + 2.0e-4f;
    float a3 = a0 + 3.0e-4f;
    float a4 = a0 + 4.0e-4f;
    float a5 = a0 + 5.0e-4f;
    float a6 = a0 + 6.0e-4f;
    float a7 = a0 + 7.0e-4f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a0 = exp2(a0); a0 = fract(a0) + 0.25f;
            a1 = exp2(a1); a1 = fract(a1) + 0.25f;
            a2 = exp2(a2); a2 = fract(a2) + 0.25f;
            a3 = exp2(a3); a3 = fract(a3) + 0.25f;
            a4 = exp2(a4); a4 = fract(a4) + 0.25f;
            a5 = exp2(a5); a5 = fract(a5) + 0.25f;
            a6 = exp2(a6); a6 = fract(a6) + 0.25f;
            a7 = exp2(a7); a7 = fract(a7) + 0.25f;
        }
    }
    out[gid] = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
}

// --- LOG2 dep-chain (8 independent chains) ---
// Each chain: a_k = log2(abs(a_k) + 1.0)  (keep positive)
// ops_per_iter = 8
kernel void probe_log2_lat(
    device float* out        [[buffer(0)]],
    constant uint& n_iters   [[buffer(1)]],
    uint gid                 [[thread_position_in_grid]])
{
    float a0 = 1.5f + float(gid) * 1.0e-6f;
    float a1 = a0 + 0.1f;
    float a2 = a0 + 0.2f;
    float a3 = a0 + 0.3f;
    float a4 = a0 + 0.4f;
    float a5 = a0 + 0.5f;
    float a6 = a0 + 0.6f;
    float a7 = a0 + 0.7f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a0 = log2(a0 * a0 + 1.0f);
            a1 = log2(a1 * a1 + 1.0f);
            a2 = log2(a2 * a2 + 1.0f);
            a3 = log2(a3 * a3 + 1.0f);
            a4 = log2(a4 * a4 + 1.0f);
            a5 = log2(a5 * a5 + 1.0f);
            a6 = log2(a6 * a6 + 1.0f);
            a7 = log2(a7 * a7 + 1.0f);
        }
    }
    out[gid] = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
}

// --- SQRT32 dep-chain (8 independent chains) ---
// Each chain: a_k = sqrt(a_k * a_k + 0.1)  (keep positive, prevent fixed point)
// ops_per_iter = 8
kernel void probe_sqrt_lat(
    device float* out        [[buffer(0)]],
    constant uint& n_iters   [[buffer(1)]],
    uint gid                 [[thread_position_in_grid]])
{
    float a0 = 0.5f + float(gid) * 1.0e-6f;
    float a1 = a0 + 0.1f;
    float a2 = a0 + 0.2f;
    float a3 = a0 + 0.3f;
    float a4 = a0 + 0.4f;
    float a5 = a0 + 0.5f;
    float a6 = a0 + 0.6f;
    float a7 = a0 + 0.7f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a0 = sqrt(a0 * a0 + 0.1f);
            a1 = sqrt(a1 * a1 + 0.1f);
            a2 = sqrt(a2 * a2 + 0.1f);
            a3 = sqrt(a3 * a3 + 0.1f);
            a4 = sqrt(a4 * a4 + 0.1f);
            a5 = sqrt(a5 * a5 + 0.1f);
            a6 = sqrt(a6 * a6 + 0.1f);
            a7 = sqrt(a7 * a7 + 0.1f);
        }
    }
    out[gid] = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
}

// --- FMAX32 dep-chain ---
// Each chain: a_k = fmax(a_k, a_k * 0.9999f + 0.0001f)
// One dep per accumulator — the second arg depends on a_k to prevent constant-folding.
// ops_per_iter = 8
kernel void probe_fmax_lat(
    device float* out        [[buffer(0)]],
    constant uint& n_iters   [[buffer(1)]],
    uint gid                 [[thread_position_in_grid]])
{
    float a0 = 0.5f + float(gid) * 1.0e-6f;
    float a1 = a0 + 0.1f;
    float a2 = a0 + 0.2f;
    float a3 = a0 + 0.3f;
    float a4 = a0 + 0.4f;
    float a5 = a0 + 0.5f;
    float a6 = a0 + 0.6f;
    float a7 = a0 + 0.7f;

    for (uint iter = 0u; iter < n_iters; iter++) {
        #pragma clang fp reassociate(off)
        {
            a0 = fmax(a0, a0 * 0.9999f + 0.0001f);
            a1 = fmax(a1, a1 * 0.9999f + 0.0001f);
            a2 = fmax(a2, a2 * 0.9999f + 0.0001f);
            a3 = fmax(a3, a3 * 0.9999f + 0.0001f);
            a4 = fmax(a4, a4 * 0.9999f + 0.0001f);
            a5 = fmax(a5, a5 * 0.9999f + 0.0001f);
            a6 = fmax(a6, a6 * 0.9999f + 0.0001f);
            a7 = fmax(a7, a7 * 0.9999f + 0.0001f);
        }
    }
    out[gid] = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
}
