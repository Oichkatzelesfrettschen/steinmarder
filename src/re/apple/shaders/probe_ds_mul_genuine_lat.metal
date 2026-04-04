#include <metal_stdlib>

using namespace metal;

// Genuine DS-multiply (Veltkamp Q.q) dep chain — BOTH H and L paths serial.
//
// In probe_ds_mul_lat.metal, LLVM's fast-math+reassoc parallelizes the H-chain:
//   `H_{n+1} = H_n * step` (serial fmul)  →  LLVM precomputes step^1..step^8 as
//   constants and applies them all to the original H_0, removing the serial dep chain
//   on H. The L-chain remains serial (fmul+fadd per step on L).
//
// Fix: #pragma clang fp reassociate(off) prevents the constant-folding substitution
// H_n = H_0 * step^n, preserving the genuine H-chain dep chain (serial fmul per step).
//
// With reassociate(off):
//   H dep chain: H → Q = H*step (fmul) → H = Q (serial fmul, 1 per step)
//   L dep chain: q = err + L*step  where  err = fma(H, step, -Q)
//                Each step: fmul(L, step) + fma(H, step, -Q) + fadd(err, L*step)
//                L dep chain bottleneck: 3 FP ops (fmul + fma + fadd) per step
//
// Critical path per step (from H_n, L_n inputs):
//   Q = H * step         [fmul, cycle L]           (H dep chain)
//   err = fma(H,step,-Q) [fma,  cycle 2L]           (waits for Q)
//   L*step               [fmul, cycle L from L_n]   (parallel with H chain)
//   q = err + L*step     [fadd, cycle max(2L, L)+L = 3L]
//   H_{n+1} = Q          at cycle L   (bottleneck: 1 fmul/step)
//   L_{n+1} = q          at cycle 3L  (bottleneck: 3 ops/step)
//
// Loop-carried bottleneck: L chain at 3L cycles per step (assuming uniform FP latency L).
// For 8 unrolled steps: L_8 ready at 8*3L = 24L cycles from loop iter start.
// Compare: probe_ds_mul_lat (H parallelized) L-chain: q_n grows with step n due to
//          err being available earlier when H is precomputed.
//
// Expected: genuinely slower than probe_ds_mul_lat (H-parallel),
//           comparable to probe_dd_genuine_lat (both measure 3-op dep chain per step).
//
// ops_per_iter = 8  (DS-multiply steps; each step = ~4 FP32 ops on critical path)
kernel void probe_ds_mul_genuine_lat(device float *out     [[buffer(0)]],
                                      constant uint &iters  [[buffer(1)]],
                                      uint gid              [[thread_position_in_grid]]) {
    const float step = 1.0f + 0x1p-23f;   // 1 + 2^-23 = nextafterf(1.0, 2.0)

    float H_val = float(gid + 1u) * 1.0e-4f + 1.0f;
    float L_val = 0.0f;

    for (uint i = 0; i < iters; ++i) {
        // reassociate(off) prevents LLVM from computing H_n = H_0 * step^n
        // as compile-time constants (multiplication is non-associative under
        // reassociate, so substitution of H_n→H_0*step^n is blocked).
        #pragma clang fp reassociate(off)
        {
            float Q, err, q;

            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
            Q = H_val * step;  err = fma(H_val, step, -Q);  q = err + L_val * step;  H_val = Q;  L_val = q;
        }
    }

    out[gid] = as_type<uint>(H_val + L_val);
}
