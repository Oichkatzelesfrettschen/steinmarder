#include <metal_stdlib>
using namespace metal;

// B4 redesign: AGX rsqrt throughput with independent per-iteration inputs.
//
// Root cause of v2 failure: acc = rsqrt(acc) has a fixed point at 1.0
// (fast_rsqrt(1.0) = 1.0 on AGX). After a few iterations, all 8 chains
// converge to 1.0, and the AGX JIT collapses the dep-chain to a constant.
// The 16-chain v2 probe confirmed 16 genuine phi-nodes in AIR but still
// measured latency (~12 cycles), not throughput (~8 cycles, philip).
//
// Fix: use per-iteration inputs derived from the outer loop counter.
//   x_k(iter) = float(iter) * 1e-6f + base_k
// These change by ~16 ULPs per step at base ≈ 0.5, so the compiler cannot
// fold them to a constant. All 16 rsqrt ops per iteration are independent.
//
// Accumulation: tree-reduce 16 results into 4-wide partial sums to avoid
// a 16-deep serial fadd dep chain on `acc` becoming the bottleneck.
//
// Expected result:
//   If RSQRT32 TP = 8 cyc (philip): 16 ops × 8 cyc/op = 128 cyc per iter
//   → ns_per_op = 128 / (1.3 GHz × 16) ≈ 6.15 ns/op ≈ 8 cycles
//
// ops_per_iter = 16 (16 independent precise::rsqrt calls per outer iteration)

kernel void probe_rsqrt_precise_tp(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]])
{
    float base_offset = float(gid + 1u) * 1.0e-8f;

    // 16 independent base values: small offsets ensure they differ enough
    // to prevent the compiler from aliasing them.
    float b0  = 0.50f + base_offset;
    float b1  = 0.51f + base_offset;
    float b2  = 0.52f + base_offset;
    float b3  = 0.53f + base_offset;
    float b4  = 0.54f + base_offset;
    float b5  = 0.55f + base_offset;
    float b6  = 0.56f + base_offset;
    float b7  = 0.57f + base_offset;
    float b8  = 0.58f + base_offset;
    float b9  = 0.59f + base_offset;
    float b10 = 0.60f + base_offset;
    float b11 = 0.61f + base_offset;
    float b12 = 0.62f + base_offset;
    float b13 = 0.63f + base_offset;
    float b14 = 0.64f + base_offset;
    float b15 = 0.65f + base_offset;

    float acc = 0.0f;

    for (uint32_t outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
        #pragma clang fp reassociate(off)
        {
            // iter_delta changes by ~16 ULP per iteration at base 0.5 — compiler
            // cannot constant-fold rsqrt across iterations.
            float iter_delta = float(outer_iter) * 1.0e-6f;

            // 16 independent rsqrt inputs; none depends on acc or each other.
            float x0  = b0  + iter_delta;
            float x1  = b1  + iter_delta;
            float x2  = b2  + iter_delta;
            float x3  = b3  + iter_delta;
            float x4  = b4  + iter_delta;
            float x5  = b5  + iter_delta;
            float x6  = b6  + iter_delta;
            float x7  = b7  + iter_delta;
            float x8  = b8  + iter_delta;
            float x9  = b9  + iter_delta;
            float x10 = b10 + iter_delta;
            float x11 = b11 + iter_delta;
            float x12 = b12 + iter_delta;
            float x13 = b13 + iter_delta;
            float x14 = b14 + iter_delta;
            float x15 = b15 + iter_delta;

            // 16 independent precise rsqrt calls — bottleneck is the rsqrt unit.
            float r0  = metal::precise::rsqrt(x0);
            float r1  = metal::precise::rsqrt(x1);
            float r2  = metal::precise::rsqrt(x2);
            float r3  = metal::precise::rsqrt(x3);
            float r4  = metal::precise::rsqrt(x4);
            float r5  = metal::precise::rsqrt(x5);
            float r6  = metal::precise::rsqrt(x6);
            float r7  = metal::precise::rsqrt(x7);
            float r8  = metal::precise::rsqrt(x8);
            float r9  = metal::precise::rsqrt(x9);
            float r10 = metal::precise::rsqrt(x10);
            float r11 = metal::precise::rsqrt(x11);
            float r12 = metal::precise::rsqrt(x12);
            float r13 = metal::precise::rsqrt(x13);
            float r14 = metal::precise::rsqrt(x14);
            float r15 = metal::precise::rsqrt(x15);

            // Tree reduce to a single value: log2(16)=4 levels of fadd.
            // Each level has latency ~2 cycles; total = 8 cycles.
            // This is < rsqrt issue latency so rsqrt is still the bottleneck.
            float t0 = r0  + r1;
            float t1 = r2  + r3;
            float t2 = r4  + r5;
            float t3 = r6  + r7;
            float t4 = r8  + r9;
            float t5 = r10 + r11;
            float t6 = r12 + r13;
            float t7 = r14 + r15;
            float u0 = t0 + t1;
            float u1 = t2 + t3;
            float u2 = t4 + t5;
            float u3 = t6 + t7;
            float v0 = u0 + u1;
            float v1 = u2 + u3;
            acc += v0 + v1;
        }
    }

    out[gid] = acc;
}
