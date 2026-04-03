#include <metal_stdlib>

using namespace metal;

// Genuine quad-half (QH4) dep chain — 4-level cascaded Knuth two-sum on FP16.
// Uses #pragma clang fp reassociate(off) to prevent fast-math folding.
//
// Metal folds probe_fp16x4_lat to ~1 half-fadd/iter (confirmed: 3.28ms ≈ baseline).
// This probe preserves the genuine 4-level carry propagation dep chain.
//
// Critical path analysis for 1 step of the 4-level cascade:
//   Level 0: s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
//   Level 1: s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
//   Level 2: s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
//   Level 3: h3 = h3 + e2;
//
// Each level: 3 half-fp ops on critical path (fadd → fsub → fadd for error; fadd for lo).
// The cascade is strictly SERIAL: e0 feeds level 1, e1 feeds level 2, e2 feeds level 3.
//
// Critical path per step: 3 levels × (fadd + 2-op error chain) ≈ deeply serial.
// Predicted depth per step ≈ 9 ops (vs 4 ops for QH2 genuine).
// Expected: significantly slower than probe_fp16x2_dd_genuine_lat.
//
// 8 unrolled steps per outer iteration.
//
// Compare:
//   probe_fp16x4_lat            — FOLDED (1 fadd/iter), 3.28ms baseline
//   probe_fp16x2_dd_genuine_lat — GENUINE QH2, 2×deep dep chain
//   probe_fp16x4_genuine_lat    — GENUINE QH4, 4×deep dep chain (this probe)
//
// ops_per_iter = 8  (QH4 steps; each step = ~13 FP16 ops on critical path)
kernel void probe_fp16x4_genuine_lat(device uint *out    [[buffer(0)]],
                                      constant uint &iters [[buffer(1)]],
                                      uint gid             [[thread_position_in_grid]]) {
    half h0 = half(gid + 1u) * 0.0001h + 1.0h;
    half h1 = 0.0h;
    half h2 = 0.0h;
    half h3 = 0.0h;
    const half delta = 1.0h;

    for (uint i = 0; i < iters; ++i) {
        #pragma clang fp reassociate(off)
        {
            half s0, e0, s1, e1, s2, e2;

            // Step 1
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;

            // Step 2
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;

            // Step 3
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;

            // Step 4
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;

            // Step 5
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;

            // Step 6
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;

            // Step 7
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;

            // Step 8
            s0 = h0 + delta;  e0 = delta - (s0 - h0);  h0 = s0;
            s1 = h1 + e0;     e1 = e0 - (s1 - h1);     h1 = s1;
            s2 = h2 + e1;     e2 = e1 - (s2 - h2);     h2 = s2;
            h3 = h3 + e2;
        }
    }

    float sum_f = float(h0) + float(h1) + float(h2) + float(h3);
    out[gid] = as_type<uint>(sum_f);
}
