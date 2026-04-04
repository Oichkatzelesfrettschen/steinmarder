#include <metal_stdlib>

using namespace metal;

// Double-single (Q.q) MULTIPLY dep chain — r600/Veltkamp pattern on Metal/AGX.
//
// This is the MULTIPLY analogue of probe_dd_lat.metal (which uses the Knuth
// two-sum ADD path).  The r600 GPU ISA uses this pattern for double-single
// arithmetic at the shader level; here we borrow it to probe AGX's FMA pipeline
// in the multiply dep-chain configuration.
//
// Each step scales the (H, L) pair by a constant `step` (a non-power-of-2
// scalar near 1.0 so the pair stays bounded over thousands of iterations):
//
//   Q   = H * step                      // approximate product  (fmul)
//   err = fma(H, step, -Q)              // exact Veltkamp error term (fma)
//   q   = err + L * step                // full correction (fmul + fadd)
//   H   = Q                             // promote Q to new high word
//   L   = q                             // carry q to new low word
//
// LLVM does NOT fold this into a closed form because floating-point
// multiplication is non-associative and `step` is not a power of two.
// The dep chain is: H_n → Q_n → H_{n+1} (fmul per step),
// and the L path: L_n → q_n → L_{n+1} (fmul+fadd, overlapped OOO).
//
// Critical path depth per step: 1 fmul (H→Q) + 1 assign (Q→H)
// With the err/q feedback the compiler must keep all four ops.
//
// 8 steps per inner iteration × iters outer iterations.
// ops_per_iter = 8   (DS-multiply steps; each step = ~4 FP32 ops)
//
// Compare with probe_dd_lat.metal (Knuth two-sum ADD) for:
//   - fmul vs fadd latency
//   - fma pipeline utilisation
//   - Q.q multiply overhead vs add on AGX
kernel void probe_ds_mul_lat(device float *out     [[buffer(0)]],
                              constant uint &iters  [[buffer(1)]],
                              uint gid              [[thread_position_in_grid]]) {
    // step = 1 + 2^-23 = nextafterf(1.0, 2.0)  — just above 1.0 so the pair
    // grows by ~0.04% after 3200 steps, well within float range.
    const float step = 1.0f + 0x1p-23f;

    float H = float(gid + 1u) * 1.0e-4f + 1.0f;
    float L = 0.0f;

    for (uint i = 0; i < iters; ++i) {
        // 8 dependent DS-multiply steps (Veltkamp / r600 Q.q pattern).
        // Variables Q, err, q are named to match the r600 double-single notation.
        float Q, err, q;

        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
        Q = H * step;  err = fma(H, step, -Q);  q = err + L * step;  H = Q;  L = q;
    }

    // Fold Q.q pair to float to stay compatible with uint-output harness.
    out[gid] = as_type<uint>(H + L);
}
