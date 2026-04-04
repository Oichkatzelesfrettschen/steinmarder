#include <metal_stdlib>

using namespace metal;

// Double-double (DD) throughput probe — 4 independent float2 (hi, lo) pairs.
//
// Each pair advances independently using the Knuth 2-sum:
//   s   = hi + delta
//   e   = delta - (s - hi)
//   hi  = s
//   lo += e
//
// With 4 independent (hi0..hi3, lo0..lo3) pairs, the AGX SIMD-group can
// pipeline all four chains simultaneously.  This reveals peak DD issue
// bandwidth rather than the 4-hop latency measured by probe_dd_lat.metal.
//
// On the M-series CPU (bench_f64x2_dd_throughput) the 4-accumulator version
// should approach ~throughput/latency times the dep-chain result.  The GPU
// equivalent tests whether AGX SIMD scheduling achieves the same overlap.
//
// 4 accumulators × 8 rounds = 32 DD pair steps per iteration.
// ops_per_iter = 32 (same counting as CPU bench_f64x2_dd_throughput).
// Output sums all hi and lo terms into one float to stay compatible with the
// existing float-output host harness.
kernel void probe_dd_tput(device float *out    [[buffer(0)]],
                          constant uint &iters [[buffer(1)]],
                          uint gid             [[thread_position_in_grid]]) {
    float base = float(gid) * 1.0e-6f;

    // 4 independent (hi, lo) pairs — distinct starting values prevent aliasing.
    float hi0 = base + 1.0f, lo0 = 0.0f;
    float hi1 = base + 2.0f, lo1 = 0.0f;
    float hi2 = base + 3.0f, lo2 = 0.0f;
    float hi3 = base + 4.0f, lo3 = 0.0f;
    const float delta = 1.0f;

    for (uint iter_idx = 0; iter_idx < iters; ++iter_idx) {
        float s0, e0, s1, e1, s2, e2, s3, e3;

        // 8 rounds × 4 independent pairs = 32 DD steps per iter.
        // Interleave pairs so the compiler cannot merge dep chains.
        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;

        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;

        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;

        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;

        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;

        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;

        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;

        s0 = hi0+delta; e0 = delta-(s0-hi0); hi0=s0; lo0+=e0;
        s1 = hi1+delta; e1 = delta-(s1-hi1); hi1=s1; lo1+=e1;
        s2 = hi2+delta; e2 = delta-(s2-hi2); hi2=s2; lo2+=e2;
        s3 = hi3+delta; e3 = delta-(s3-hi3); hi3=s3; lo3+=e3;
    }

    // Sink all 8 registers to prevent DCE.
    out[gid] = (hi0 + hi1 + hi2 + hi3) + (lo0 + lo1 + lo2 + lo3);
}
