#include <metal_stdlib>

using namespace metal;

// Genuine double-half (DH) dep chain — Knuth two-sum on FP16 (half) pairs.
// Uses #pragma clang fp reassociate(off) to prevent fast-math folding.
//
// Metal enables fast-math (unsafe-fp-math=true), which folds:
//   e = delta - (s - hi)  →  delta - delta = 0  (reassoc substitutes s = hi+delta)
// Folded probe_fp16x2_dd_lat measures 1 half-fadd/iter (same as folded float32 DD).
// Confirmed: gpu_ns/dispatch = 3.47ms ≈ baseline 3.43ms → folded.
//
// Fix: #pragma clang fp reassociate(off) on the inner compound statement.
// AIR flags inside pragma: nnan ninf nsz arcp contract afn half (no reassoc).
// Register allocation preserved; no alloca overhead.
//
// Dep chain depth: same structure as probe_dd_genuine_lat (11 cycles for 8 steps)
// but using half precision. Key question: does AGX have same fadd latency for half
// vs float32? Expected: yes (same 1-cycle ALU, same dep chain depth → same ratio).
//
// Compare:
//   probe_fp16x2_dd_lat   — FOLDED (1 fadd/iter), same timing as probe_dd_lat
//   probe_fp16x2_dd_genuine_lat — GENUINE 2-sum, should match probe_dd_genuine_lat ratio
//   probe_dd_genuine_lat  — GENUINE float32 2-sum, 1.70× baseline
//
// ops_per_iter = 8  (DH two-sum steps; each step = 4 FP16 ops)
kernel void probe_fp16x2_dd_genuine_lat(device uint *out    [[buffer(0)]],
                                         constant uint &iters [[buffer(1)]],
                                         uint gid             [[thread_position_in_grid]]) {
    half hi = half(gid + 1u) * 0.0001h + 1.0h;
    half lo = 0.0h;
    const half delta = 1.0h;

    for (uint i = 0; i < iters; ++i) {
        #pragma clang fp reassociate(off)
        {
            half s, e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
            s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        }
    }

    float hi_f = float(hi);
    float lo_f = float(lo);
    out[gid] = as_type<uint>(hi_f) ^ as_type<uint>(lo_f);
}
