#include <metal_stdlib>

using namespace metal;

// Atomic add latency probe — threadgroup buffer.
//
// 32 serial atomic_fetch_add_explicit calls per iteration on a single
// threadgroup counter.  Each call has a carry dependency on the return value,
// forcing the compiler to emit a dependent atomicrmw chain rather than
// coalescing or reordering the ops.
//
// All threads in the threadgroup target the SAME address (index 0) so every
// atomic is a contested operation, not a scattered one.  This measures
// threadgroup atomic latency under full-contention, the worst-case path that
// matters when every thread in a simdgroup tries to accumulate into one slot.
//
// Compare with probe_atomic_add_device.metal which probes the device-buffer
// (global) atomic path.
//
// Confirmed AIR (O2): Metal does NOT emit generic LLVM atomicrmw instructions.
// Instead the Metal compiler emits AIR-specific atomic intrinsics:
//   @air.atomic.local.add.u.i32  — addrspace(3), threadgroup atomics
//   @air.atomic.local.store.i32  — addrspace(3), init of tgsm counter
//   @air.wg.barrier              — threadgroup_barrier
// 33 calls to @air.atomic.local.add.u.i32 confirmed (32 loop body + loop
// overhead absorbed into straight-line unrolled ops at -O2).
kernel void probe_atomic_tgsm_lat(device uint *out            [[buffer(0)]],
                                  constant uint &iters         [[buffer(1)]],
                                  uint gid                     [[thread_position_in_grid]],
                                  uint tid                     [[thread_index_in_threadgroup]]) {
    threadgroup atomic_uint tgsm_counter;
    if (tid == 0) {
        atomic_store_explicit(&tgsm_counter, 0u, memory_order_relaxed);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);

    uint acc = 0u;
    const uint delta = 1u;

    for (uint i = 0; i < iters; ++i) {
        // 32 serial atomic adds — each uses the previous return value so the
        // compiler cannot reorder or batch them.
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(&tgsm_counter, delta + acc, memory_order_relaxed);
    }

    // Prevent dead-code elimination: write accumulator to device output.
    out[gid] = acc;
}

// Atomic add latency probe — device (global) buffer.
//
// Same 32-deep serial chain but targeting a device-address atomic_uint.
// Device atomics map to a different hardware path than threadgroup atomics —
// on Apple GPUs, device atomics go through the tile memory / L2 while
// threadgroup atomics are local to the TGSM block.  This probe captures the
// device-atomic path so the two can be compared directly.
//
// Confirmed AIR (O2): Metal emits @air.atomic.global.add.u.i32 (addrspace 1)
// for device atomics — a different intrinsic from the threadgroup path.
// 33 calls to @air.atomic.global.add.u.i32 confirmed.
kernel void probe_atomic_device_lat(device atomic_uint *counter [[buffer(0)]],
                                    constant uint      &iters   [[buffer(1)]],
                                    device uint        *out     [[buffer(2)]],
                                    uint gid                    [[thread_position_in_grid]]) {
    uint acc = 0u;
    const uint delta = 1u;

    for (uint i = 0; i < iters; ++i) {
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
        acc = atomic_fetch_add_explicit(counter, delta + acc, memory_order_relaxed);
    }

    out[gid] = acc;
}
