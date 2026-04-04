/* probe_threadgroup_lat_v2.metal — T5-fix: genuine AGX threadgroup LDS latency
 *
 * Two variants:
 *
 * probe_tg_lat_v2_volatile  — volatile threadgroup array, no barrier.
 *   Each load/store is a genuine LDS instruction (compiler cannot hoist or
 *   eliminate it), same thread owns its slot. 8 load+store pairs per iter.
 *   ops_per_iter = 8.  Does NOT include barrier cost.
 *
 * probe_tg_lat_v2_barrier   — cross-thread adjacency + threadgroup_barrier.
 *   Thread reads adjacent thread's slot. 8 load+store+barrier triplets per iter.
 *   Each step cost = LDS load + LDS store + barrier.  ops_per_iter = 8.
 *   This measures LDS-round-trip + barrier overhead together.
 */

#include <metal_stdlib>
using namespace metal;

/* ── Variant A: volatile threadgroup — no barrier ───────────────────────────
 * Forces each load and store to be a real LDS instruction.
 * Address is constant per thread (own slot) so the critical path is:
 *   load → fadd → store → load → ...
 * This is the pure LDS load+store latency dep chain. */
kernel void probe_tg_lat_v2_volatile(
    device float      *out   [[buffer(0)]],
    constant uint     &iters [[buffer(1)]],
    uint gid [[thread_position_in_grid]],
    uint lid [[thread_position_in_threadgroup]])
{
    volatile threadgroup float tg_v[1024];

    /* Seed: each thread writes a non-zero initial value. */
    tg_v[lid] = float(gid + 1u) * 1.0e-4f;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    float val = tg_v[lid];
    for (uint iter_idx = 0u; iter_idx < iters; ++iter_idx) {
        /* 8 serial volatile load-add-store steps per iter.
         * Each load address is this thread's own slot; compiler cannot
         * batch them because the store result is the next load's data. */
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
        val = tg_v[lid]; tg_v[lid] = val + 1.0f;
    }
    out[gid] = val;
}

/* ── Variant B: cross-thread adjacency + barrier ────────────────────────────
 * Thread reads from the ADJACENT thread's slot (wrapping at 1024).
 * The compiler cannot eliminate the load because the source address is
 * always a different slot from where the thread writes.
 * Each step: load-from-adjacent + scale-accumulate + store-own + barrier.
 * ops_per_iter = 8 steps. */
kernel void probe_tg_lat_v2_barrier(
    device float      *out   [[buffer(0)]],
    constant uint     &iters [[buffer(1)]],
    uint gid [[thread_position_in_grid]],
    uint lid [[thread_position_in_threadgroup]])
{
    threadgroup float tg[1024];

    /* Seed with distinct per-thread value. */
    tg[lid] = float(gid + 1u) * 1.0e-4f + 1.0f;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    /* Adjacent thread's slot index (compiler sees this as data-dependent on lid). */
    uint src_slot = (lid + 1u) & 1023u;

    float val = tg[lid];
    for (uint iter_idx = 0u; iter_idx < iters; ++iter_idx) {
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
        val = tg[src_slot] + val * 1.0e-9f;  tg[lid] = val;  threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    out[gid] = val;
}
