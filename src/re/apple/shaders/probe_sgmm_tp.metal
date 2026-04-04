#include <metal_stdlib>
using namespace metal;

// D4b: AGX simdgroup_matrix multiply throughput — 4 independent C accumulators.
//
// Dep-chain result (D4): SGMM f32 8×8 = 24.235 ns = ~31.5 cycles.
// This probe measures throughput by maintaining 4 independent C matrices,
// each updated by a separate simdgroup_multiply_accumulate call.
//
// Register budget: each simdgroup_float8x8 = 64 elements / 32 threads = 2 floats/thread.
// 4 matrices × 2 floats/thread = 8 regs/thread (6.25% of 128 GPR budget) → no spill.
//
// If the SGMM unit has true pipelining, throughput with K accumulators should approach
// T_throughput = L / K. L = 31.5 cyc, K = 4 → expect ~7.9 cyc/call if pipelineable.
//
// Threadgroup memory: 4 matrices × 64 floats/simdgroup × 32 simdgroups = 8192 floats = 32KB.
//
// ops_per_iter = 4 SGMM calls per iteration (one per independent accumulator per inner iter).

kernel void probe_sgmm_f32_tp(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]],
    uint simd_lane_id [[thread_index_in_simdgroup]],
    uint simdgroup_id [[simdgroup_index_in_threadgroup]])
{
    // 4 independent C matrices × 64 floats/simdgroup × 32 simdgroups = 8192 floats
    threadgroup float tg_staging[8192];

    {
        #pragma clang fp reassociate(off)

        simdgroup_float8x8 A = make_filled_simdgroup_matrix<float, 8, 8>(0.5f);

        // Four independent accumulators — unique seeds to prevent constant folding.
        float seed = float(simdgroup_id + 1u) * 1.0e-7f + 0.01f;
        simdgroup_float8x8 C0 = make_filled_simdgroup_matrix<float, 8, 8>(seed);
        simdgroup_float8x8 C1 = make_filled_simdgroup_matrix<float, 8, 8>(seed * 1.00001f);
        simdgroup_float8x8 C2 = make_filled_simdgroup_matrix<float, 8, 8>(seed * 1.00002f);
        simdgroup_float8x8 C3 = make_filled_simdgroup_matrix<float, 8, 8>(seed * 1.00003f);

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            // Four independent SGMM calls — each feeds only into itself (no cross-acc deps).
            simdgroup_multiply_accumulate(C0, A, A, C0);
            simdgroup_multiply_accumulate(C1, A, A, C1);
            simdgroup_multiply_accumulate(C2, A, A, C2);
            simdgroup_multiply_accumulate(C3, A, A, C3);
        }

        // Store C0–C3 to threadgroup staging to prevent elision.
        uint sg_staging_base = simdgroup_id * 256u;  // 4 × 64 floats per simdgroup
        simdgroup_store(C0, tg_staging + sg_staging_base +   0u, 8u);
        simdgroup_store(C1, tg_staging + sg_staging_base +  64u, 8u);
        simdgroup_store(C2, tg_staging + sg_staging_base + 128u, 8u);
        simdgroup_store(C3, tg_staging + sg_staging_base + 192u, 8u);
        threadgroup_barrier(mem_flags::mem_threadgroup);

        // Write one float per thread to output.
        out[gid] = tg_staging[sg_staging_base + simd_lane_id % 256u];
    }
}
