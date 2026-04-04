#include <metal_stdlib>

using namespace metal;

// Double-half (DH) dep chain — Knuth two-sum on FP16 (half) pairs.
//
// Extends the wide-precision fastpath ladder DOWN from the FP32 double-double
// (probe_dd_lat.metal) to a FP16 double-half (DH) base.
//
// Representation: (hi: half, lo: half) where |lo| <= 0.5 ulp(hi).
// FP16 has 10-bit mantissa → a (hi, lo) pair gives ~20 effective mantissa bits.
// This is between FP16 (10 bits) and FP32 (23 bits), NOT a replacement for FP64.
//
// Each step uses the Knuth/Shewchuk error-free two-sum on half precision:
//   s   = hi + delta
//   e   = delta - (s - hi)    // exact error term
//   hi  = s
//   lo += e
//
// Key questions:
//   1. Is AGX half-precision addition latency the same as float32?
//      (Metal compiles `half` arithmetic to `half` AIR types; if AGX has the
//       same 1-cycle ALU latency for half as for float, the dep-chain time
//       should match probe_dd_lat.metal.)
//   2. Does the compiler widen half to float32 internally?
//      Check the compiled AIR with `llvm-dis` — look for `fadd half` vs `fadd float`.
//
// ops_per_iter = 8  (DH two-sum steps; each step = 4 FP16 ops)
//
// Compare:
//   probe_dd_lat.metal   — float32 DD (Knuth two-sum), ~3.968 ns/step
//   probe_fp16x2_dd_lat  — float16 DH (Knuth two-sum), this probe
//   probe_fp16x4_lat     — quad-half (4-level carry propagation), 40-bit precision
kernel void probe_fp16x2_dd_lat(device uint *out    [[buffer(0)]],
                                 constant uint &iters [[buffer(1)]],
                                 uint gid             [[thread_position_in_grid]]) {
    // Use a per-thread starting value to prevent cross-thread folding.
    half hi = half(gid + 1u) * 0.0001h + 1.0h;
    half lo = 0.0h;
    const half delta = 1.0h;

    for (uint i = 0; i < iters; ++i) {
        half s, e;

        // 8 dependent DH two-sum steps.
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
        s = hi + delta;  e = delta - (s - hi);  hi = s;  lo += e;
    }

    // Fold DH pair to uint32 output (hi widened to float, lo as float, xored).
    // The as_type cast avoids any FP conversion on the output path.
    float hi_f = float(hi);
    float lo_f = float(lo);
    out[gid] = as_type<uint>(hi_f) ^ as_type<uint>(lo_f);
}
