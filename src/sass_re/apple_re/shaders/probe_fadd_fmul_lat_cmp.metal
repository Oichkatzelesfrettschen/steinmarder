#include <metal_stdlib>
using namespace metal;

// Isolated fadd dep chain: 8 serial fadds per inner iter.
// Uses reassociate(off) to prevent constant folding of `acc += delta` × 8 → `acc += 8*delta`.
// With reassociate(off) and per-step: `acc = acc + delta; acc = acc + delta; ...`
// LLVM sees these as strictly left-to-right accumulations with loop-carried dependency.
// NOTE: `acc += delta` × 8 folds to `acc += 8*delta` even without reassoc if LLVM
// recognizes the constant. Using a gid-seeded delta to force runtime value.
kernel void probe_fadd8_lat(device float *out    [[buffer(0)]],
                             constant uint &iters [[buffer(1)]],
                             uint gid             [[thread_position_in_grid]]) {
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;
    // delta is a tiny per-thread constant derived at runtime — LLVM can't fold
    // acc += delta × 8 to acc += 8*delta because delta is not a compile-time constant.
    float delta = float(gid + 1u) * 1.0e-9f + 1.0f;
    for (uint i = 0; i < iters; ++i) {
        #pragma clang fp reassociate(off)
        {
            acc += delta; acc += delta; acc += delta; acc += delta;
            acc += delta; acc += delta; acc += delta; acc += delta;
        }
    }
    out[gid] = acc;
}

// Isolated fmul dep chain: 8 serial fmuls per inner iter.
// `step` is runtime (per-thread), preventing constant power-folding.
kernel void probe_fmul8_lat(device float *out    [[buffer(0)]],
                             constant uint &iters [[buffer(1)]],
                             uint gid             [[thread_position_in_grid]]) {
    float acc = float(gid + 1u) * 1.0e-6f + 1.0f;
    // step slightly above 1 so acc stays bounded. Runtime value → not foldable.
    float step = 1.0f + float(gid + 1u) * 1.0e-9f;
    for (uint i = 0; i < iters; ++i) {
        #pragma clang fp reassociate(off)
        {
            acc *= step; acc *= step; acc *= step; acc *= step;
            acc *= step; acc *= step; acc *= step; acc *= step;
        }
    }
    out[gid] = acc;
}
