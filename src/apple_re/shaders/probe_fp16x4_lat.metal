#include <metal_stdlib>

using namespace metal;

// Quad-half (QH4) dep chain — 4-level cascaded Knuth two-sum on FP16.
//
// Packs four FP16 values into a non-overlapping sum representation:
//   value ≈ h0 + h1 + h2 + h3
//   with |h1| <= 0.5 ulp(h0), |h2| <= 0.5 ulp(h1), |h3| <= 0.5 ulp(h2)
//
// Effective mantissa: ~40 bits (4 × 10-bit FP16 mantissa).
// This is between FP32 (23 bits) and FP64 (52 bits) — not a native FP64
// replacement, but the maximum AGX can provide within FP16 registers.
//
// Each step advances the QH4 accumulator by +delta (a constant half) using
// a 4-level cascaded error-free two-sum:
//
//   s0 = h0 + delta;  e0 = delta - (s0 - h0);   h0 = s0;
//   s1 = h1 + e0;     e1 = e0 - (s1 - h1);       h1 = s1;
//   s2 = h2 + e1;     e2 = e1 - (s2 - h2);       h2 = s2;
//   h3 = h3 + e2;                                // final carry absorbed
//
// The dep chain is strictly serial: e0 feeds s1, e1 feeds s2, e2 feeds h3.
// The critical path is 4 levels × 2 ops each (fadd + fsub) = 8 FP16 ops.
//
// Key questions:
//   1. Is the latency ≈ 4 × probe_fp16x2_dd_lat (depth-4 vs depth-1)?
//      If so, AGX has no out-of-order overlap across the carry levels.
//   2. Is there a speedup ratio from FP16 vs FP32 on the per-level add?
//      (FP16 and FP32 are expected to have the same 1-cycle latency on AGX,
//       so this should track 4× the FP16 2-sum latency.)
//   3. What precision does the accumulator maintain? With delta=1 and 3200 steps,
//      h0 ≈ 3200; h1..h3 hold the sub-ulp residual. The FP16 ulp(3200) = 2,
//      so h1 captures the 0..2 range (10-bit precision), h2 the 0..2^-10 range,
//      h3 the 0..2^-20 range. Together: ~40-bit precision for the accumulated sum.
//
// ops_per_iter = 8  (QH4 steps; each step = 7 FP16 ops on critical path)
//
// Compare:
//   probe_dd_lat.metal      — float32 DD,  4 fp32 ops/step, ~3.968 ns/step
//   probe_fp16x2_dd_lat     — float16 DH,  4 fp16 ops/step
//   probe_fp16x4_lat        — float16 QH4, 7 fp16 ops/step (this probe)
kernel void probe_fp16x4_lat(device uint *out    [[buffer(0)]],
                              constant uint &iters [[buffer(1)]],
                              uint gid             [[thread_position_in_grid]]) {
    half h0 = half(gid + 1u) * 0.0001h + 1.0h;
    half h1 = 0.0h;
    half h2 = 0.0h;
    half h3 = 0.0h;
    const half delta = 1.0h;

    for (uint i = 0; i < iters; ++i) {
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

    // Fold all 4 half components to uint32 output.
    float sum_f = float(h0) + float(h1) + float(h2) + float(h3);
    out[gid] = as_type<uint>(sum_f);
}
