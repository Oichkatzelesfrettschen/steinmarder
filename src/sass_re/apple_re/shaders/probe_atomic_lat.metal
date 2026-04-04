#include <metal_stdlib>

using namespace metal;

// T8a: Serial atomic_fetch_add latency on a THREADGROUP atomic.
//
// Only thread 0 performs the serial dep chain to avoid contention from other
// threads (contended atomics would measure contention cost, not pure latency).
// The return value of each fetch_add is used as the addend for the next call,
// forcing a sequential dep chain.
//
// 8 serial atomic_fetch_add_explicit calls per outer iteration.
// ops_per_iter = 8

kernel void probe_atomic_tg_lat(device uint   *out   [[buffer(0)]],
                                constant uint &iters [[buffer(1)]],
                                uint gid [[thread_position_in_grid]],
                                uint lid [[thread_index_in_threadgroup]]) {
    threadgroup atomic_uint tg_atom;

    if (lid == 0) {
        atomic_store_explicit(&tg_atom, 0u, memory_order_relaxed);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);

    uint val = 0u;

    if (lid == 0) {
        for (uint iter_idx = 0; iter_idx < iters; ++iter_idx) {
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(&tg_atom, val + 1u, memory_order_relaxed);
        }
    }

    out[gid] = atomic_load_explicit(&tg_atom, memory_order_relaxed);
}

// T8b: Serial atomic_fetch_add latency on a DEVICE (global) atomic.
//
// Only thread gid==0 performs the serial dep chain.  The output buffer slot 0
// is reinterpreted as a device atomic_uint.  All other threads write their gid
// to their own output slot so the buffer write is not completely dead.
//
// 8 serial atomic_fetch_add_explicit calls per outer iteration.
// ops_per_iter = 8

kernel void probe_atomic_global_lat(device uint   *out   [[buffer(0)]],
                                    constant uint &iters [[buffer(1)]],
                                    uint gid [[thread_position_in_grid]]) {
    if (gid == 0) {
        device atomic_uint *atom_ptr = (device atomic_uint *)&out[0];
        atomic_store_explicit(atom_ptr, 0u, memory_order_relaxed);

        uint val = 0u;
        for (uint iter_idx = 0; iter_idx < iters; ++iter_idx) {
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
            val = atomic_fetch_add_explicit(atom_ptr, val + 1u, memory_order_relaxed);
        }
        // val written implicitly through atom_ptr aliasing out[0]
    } else {
        out[gid] = gid;
    }
}
