#include <metal_stdlib>
using namespace metal;

// D4: AGX simdgroup_matrix multiply (SGMM) dep-chain latency probe.
//
// Metal simdgroup_multiply_accumulate: C = A * B + C
// Supported on M1+: T = float, Rows=Cols=8 → 8×8 × 8×8 = 512 FMAD/call.
//
// Dep chain: each call feeds its output (C) as the accumulator of the next.
// A is constant; only C changes each iteration.
//
// ops_per_iter = 4 (SGMM calls per inner iteration)
// Report ns/SGMM-call.
//
// Anti-elision: store C to threadgroup memory and write one element to output.

kernel void probe_sgmm_f32_lat(
    device float* out [[buffer(0)]],
    uint gid [[thread_position_in_grid]],
    uint simd_lane_id [[thread_index_in_simdgroup]],
    uint simdgroup_id [[simdgroup_index_in_threadgroup]])
{
    // 32 simdgroups × 64 floats each = 2048 floats (8KB threadgroup memory).
    threadgroup float tg_staging[2048];

    {
        #pragma clang fp reassociate(off)

        simdgroup_float8x8 A = make_filled_simdgroup_matrix<float, 8, 8>(0.5f);

        // Unique seed per simdgroup to prevent constant-folding across groups.
        float seed = float(simdgroup_id + 1u) * 1.0e-7f + 0.01f;
        simdgroup_float8x8 C = make_filled_simdgroup_matrix<float, 8, 8>(seed);

        for (uint outer_iter = 0u; outer_iter < 100000u; outer_iter++) {
            simdgroup_multiply_accumulate(C, A, A, C);
            simdgroup_multiply_accumulate(C, A, A, C);
            simdgroup_multiply_accumulate(C, A, A, C);
            simdgroup_multiply_accumulate(C, A, A, C);
        }

        // Store C → threadgroup staging (8×8 = 64 floats per simdgroup).
        // Each simdgroup gets its own 64-element region.
        threadgroup float* sg_staging = tg_staging + simdgroup_id * 64u;
        simdgroup_store(C, sg_staging, 8u);
        threadgroup_barrier(mem_flags::mem_threadgroup);

        // Write one element per thread to output.
        out[gid] = sg_staging[simd_lane_id % 64u];
    }
}
