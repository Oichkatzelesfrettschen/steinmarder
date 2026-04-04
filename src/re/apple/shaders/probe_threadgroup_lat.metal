#include <metal_stdlib>

using namespace metal;

// T5: Threadgroup (LDS) memory read-write latency dep chain.
//
// Each thread owns its own slot in threadgroup memory (slot = lid & 1023,
// which is just lid since width=1024).  A serial store→load dep chain is
// built without barriers so we measure raw LDS round-trip latency rather
// than barrier cost.
//
// 8 load-add-store triplets per outer iteration.  Each load directly depends
// on the result of the previous store to the same address.
//
// ops_per_iter = 8

kernel void probe_threadgroup_lat(device float   *out   [[buffer(0)]],
                                  constant uint  &iters [[buffer(1)]],
                                  uint gid [[thread_position_in_grid]],
                                  uint lid [[thread_index_in_threadgroup]]) {
    threadgroup float tg[1024];

    // Initialise this thread's slot.
    tg[lid] = float(gid + 1u) * 1.0e-4f + 1.0f;

    // No barrier — we deliberately keep all ops within a single thread so
    // the dep chain is thread-local: store→load to the same address.
    float val;

    for (uint iter_idx = 0; iter_idx < iters; ++iter_idx) {
        val = tg[lid]; val += 1.0f; tg[lid] = val;
        val = tg[lid]; val += 1.0f; tg[lid] = val;
        val = tg[lid]; val += 1.0f; tg[lid] = val;
        val = tg[lid]; val += 1.0f; tg[lid] = val;
        val = tg[lid]; val += 1.0f; tg[lid] = val;
        val = tg[lid]; val += 1.0f; tg[lid] = val;
        val = tg[lid]; val += 1.0f; tg[lid] = val;
        val = tg[lid]; val += 1.0f; tg[lid] = val;
    }

    out[gid] = val;
}
