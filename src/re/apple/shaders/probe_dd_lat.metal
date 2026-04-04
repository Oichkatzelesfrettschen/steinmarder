#include <metal_stdlib>

using namespace metal;

// Double-double (DD) latency probe using float2 (hi, lo) pairs.
//
// Metal/MSL has no fp64 or fp128.  Extended precision is therefore modelled as
// a Knuth 2-sum (Shewchuk) double-double using two float values:
//   float2 pair  where  pair.x = hi,  pair.y = lo
//   value = hi + lo,  |lo| <= 0.5 ulp(hi)
//
// Each step advances one (hi, lo) pair by +delta using the error-free 2-sum:
//   s   = hi + delta
//   e   = delta - (s - hi)     // exact error term (Knuth condition)
//   hi  = s
//   lo += e
//
// This is 4 dependent FP32 ops per step on a single (hi,lo) pair.
// Critical path: hi -> s -> (s-hi) -> e -> hi  (4 hops).
// Compare with probe_dd_tput.metal (4 independent pairs) for throughput.
//
// ops_per_iter counted as DD steps (not individual FP32 ops).
// Use 8 steps per iteration to match the CPU bench_f64x2_dd_twosum_dep unroll.
//
// Expected: AGX SIMD-group scheduling may collapse the 4-op chain similarly
// to the M-series OOO P-core (~3.3 cycles observed CPU-side).
// Output is the hi component only (dominant term); lo is O(eps*|hi|) and omitted
// to stay compatible with the existing float-output host harness.
kernel void probe_dd_lat(device float *out    [[buffer(0)]],
                         constant uint &iters [[buffer(1)]],
                         uint gid             [[thread_position_in_grid]]) {
    // Each thread starts with a distinct hi so the compiler cannot fold threads.
    float hi = float(gid) * 1.0e-6f + 1.0f;
    float lo = 0.0f;
    const float delta = 1.0f;

    for (uint iter_idx = 0; iter_idx < iters; ++iter_idx) {
        // 8 dependent DD 2-sum steps.
        // Using local scalar variables forces a true dep chain; the compiler
        // cannot vectorise across steps because hi feeds the next step's s.
        float s, e;

        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
    }

    out[gid] = hi + lo;
}
