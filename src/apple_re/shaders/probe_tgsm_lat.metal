#include <metal_stdlib>

using namespace metal;

// Threadgroup (TGSM / shared memory) latency probe.
// A single thread performs a dependent-load chain through threadgroup memory.
// Each load result is used to compute the next address index.
// Measures the round-trip latency of a threadgroup memory read.
//
// SASS analog: LDS.U32 dependent chain.
// Expected: ~1-2 cycles on most Apple GPU families (threadgroup = on-chip SRAM).

constant uint TGSM_WORDS = 256;

kernel void probe_tgsm_lat(device uint *out [[buffer(0)]],
                           constant uint &iters [[buffer(1)]],
                           uint gid [[thread_position_in_grid]],
                           uint tid [[thread_index_in_threadgroup]]) {
    threadgroup uint tgsm[TGSM_WORDS];

    // Thread 0 initializes the chase array: tgsm[i] = (i + 1) % TGSM_WORDS
    // This forms a cycle that forces every load to depend on the prior one.
    if (tid == 0) {
        for (uint k = 0; k < TGSM_WORDS; ++k) {
            tgsm[k] = (k + 1u) % TGSM_WORDS;
        }
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);

    uint idx = gid % TGSM_WORDS;
    for (uint i = 0; i < iters; ++i) {
        // 32 dependent threadgroup loads per iteration.
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
        idx = tgsm[idx];
    }

    out[gid] = idx;
}
