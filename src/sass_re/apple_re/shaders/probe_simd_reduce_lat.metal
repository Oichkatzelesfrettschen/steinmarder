#include <metal_stdlib>

using namespace metal;

// Simdgroup operation latency probes.
// Measures Apple GPU simdgroup intrinsic latencies:
//   - simd_sum (reduction across 32 lanes)
//   - simd_broadcast (broadcast from lane 0)
//   - simd_shuffle (lane permutation)
//
// SASS analogs: REDUX.SUM, SHFL.IDX, SHFL.BFLY (warp-level operations).
// Each chain makes the output of one simd op the input of the next.

kernel void probe_simd_sum_lat(device float *out [[buffer(0)]],
                               constant uint &iters [[buffer(1)]],
                               uint gid [[thread_position_in_grid]],
                               uint lane [[thread_index_in_simdgroup]]) {
    float acc = float(lane) + float(gid) * 0.0001f;

    for (uint i = 0; i < iters; ++i) {
        // 8 dependent simd_sum reductions per iteration.
        // Result of each sum feeds the next sum (dependent chain).
        acc = simd_sum(acc) * 0.03125f; // divide by 32 to prevent overflow
        acc = simd_sum(acc) * 0.03125f;
        acc = simd_sum(acc) * 0.03125f;
        acc = simd_sum(acc) * 0.03125f;
        acc = simd_sum(acc) * 0.03125f;
        acc = simd_sum(acc) * 0.03125f;
        acc = simd_sum(acc) * 0.03125f;
        acc = simd_sum(acc) * 0.03125f;
    }

    out[gid] = acc;
}

kernel void probe_simd_shuffle_lat(device uint *out [[buffer(0)]],
                                   constant uint &iters [[buffer(1)]],
                                   uint gid [[thread_position_in_grid]],
                                   uint lane [[thread_index_in_simdgroup]]) {
    uint val = gid ^ (lane << 5);

    for (uint i = 0; i < iters; ++i) {
        // 16 dependent simd_shuffle ops per iteration.
        // Each shuffle index is derived from the previous shuffle result
        // so the chain is fully dependent.
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
        val = simd_shuffle(val, (val ^ lane) & 31u);
    }

    out[gid] = val;
}

kernel void probe_simd_broadcast_lat(device float *out [[buffer(0)]],
                                     constant uint &iters [[buffer(1)]],
                                     uint gid [[thread_position_in_grid]],
                                     uint lane [[thread_index_in_simdgroup]]) {
    float val = float(gid) + float(lane) * 0.001f;

    for (uint i = 0; i < iters; ++i) {
        // 16 dependent simd_broadcast ops per iteration.
        // Source lane alternates between 0 and 1 based on prior value parity
        // to keep the dependency chain alive.
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
        val = simd_broadcast(val, 0u);
        val = val * 1.0000001f + 0.00001f;
    }

    out[gid] = val;
}
