#include <metal_stdlib>

using namespace metal;

// Genuine Knuth two-sum double-double dep chain — NOT foldable.
//
// Metal always enables fast-math ("unsafe-fp-math"="true" in AIR attributes),
// which causes LLVM to eliminate the Knuth error term via the `reassoc` flag:
//   e = delta - (s - hi)  →  [reassoc: substitute s = hi+delta]
//                         →  delta - ((hi+delta) - hi) = delta - delta = 0
// and folds the 8-step chain to:  hi += 8 * delta  (1 fadd per inner iter).
//
// Fix: #pragma clang fp reassociate(off) at the inner compound statement.
// This disables ONLY the `reassoc` flag for ops in that scope, leaving all other
// fast-math flags enabled (nnan ninf nsz arcp contract afn). Result:
//   - All variables remain register-allocated (no alloca overhead)
//   - Error term e = delta - (s - hi) is preserved as fsub + fsub + fadd per step
//   - Full 4-op Knuth two-sum dep chain intact in AIR
//
// NOTE: [[clang::optnone]] was tried first but forces ALL locals through alloca
// (stack memory), causing ~30,000x overhead vs expected. The reassociate pragma
// is the correct surgical fix.
//
// Verified AIR flags: ops inside pragma scope have
//   `nnan ninf nsz arcp contract afn float` (all fast-math EXCEPT reassoc).
//
// Critical path analysis for 8-step unrolled body:
//   hi chain: s1(fadd) → s2 → ... → s8  [8 serial faddS]
//   lo chain: lo += e_n; e_n available at cycle n+2 (from s_{n-1} and s_n)
//             lo bottleneck: lo8 ready at cycle 11 (serial fadd per step)
//   Total dep chain depth ≈ 11 cycles per outer iter.
//
// Expected: ~11× slower than folded probe_dd_lat (1 fadd/iter dep chain).
//   folded: ~7,350 ns/iter → genuine predicted: ~80,000 ns/iter.
//
// Compare:
//   probe_dd_lat.metal: FOLDED to 1 fadd/iter (fast-math+reassoc)
//   probe_dd_genuine_lat: GENUINE two-sum, 4 ops/step × 8 steps/iter = 32 ops/iter
//
// ops_per_iter = 8  (DD two-sum steps; each step = 4 FP32 ops)
kernel void probe_dd_genuine_lat(device float *out    [[buffer(0)]],
                                  constant uint &iters [[buffer(1)]],
                                  uint gid             [[thread_position_in_grid]]) {
    float hi = float(gid + 1u) * 1.0e-6f + 1.0f;
    float lo = 0.0f;
    const float delta = 1.0f;

    for (uint i = 0; i < iters; ++i) {
        // #pragma clang fp reassociate(off) must appear at the start of a compound
        // statement. It disables the `reassoc` fast-math flag within the block,
        // preventing LLVM from substituting s = hi+delta into (s - hi) → delta
        // and thus folding the error term e to 0.
        #pragma clang fp reassociate(off)
        {
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
    }

    out[gid] = hi + lo;
}
