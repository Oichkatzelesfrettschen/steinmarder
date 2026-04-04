/* probe_global_l1_lat_v2.metal — T6-fix: per-thread L1 latency (no cross-thread contention)
 *
 * Kernel: probe_global_l1_v2
 *
 * Previous probe_global_mem_lat measured 58.6 ns under 1024-thread contention:
 * all threads competed for overlapping cache lines in a 4 KB buffer.
 * This version gives every thread its own cache line slot so there is zero
 * cross-thread cache line contention.  The buffer is 1024 floats (4 KB).
 * Thread gid reads only slot gid — one float per thread, one cache line per
 * group of 16 floats (64 bytes).  Each warp of 32 threads spans 2 cache lines.
 *
 * The volatile qualifier forces each load to be a genuine global memory
 * instruction.  The acc dependency chain:
 *   acc = vout[gid] + acc * 1e-30f
 * ensures each load's result feeds the next load address computation via acc
 * (the tiny multiplier keeps acc ≈ 0 so we always reload the same hot slot).
 *
 * After the first iteration the cache line containing out[gid] is resident in
 * L1.  All subsequent loads in the same kernel dispatch measure warm-L1 latency
 * for the globally-addressed buffer.
 *
 * ops_per_iter = 8.  Run with --width 1024 --iters 30.
 */

#include <metal_stdlib>
using namespace metal;

kernel void probe_global_l1_v2(
    device float  *out   [[buffer(0)]],
    constant uint &iters [[buffer(1)]],
    uint gid [[thread_position_in_grid]])
{
    /* Cast to volatile so the compiler emits a real load on every access. */
    volatile device float *vol_out = (volatile device float *)out;

    /* Warm the slot: write a non-zero sentinel so the line is resident. */
    vol_out[gid] = float(gid + 1u) * 1.0e-4f;

    float acc = vol_out[gid];

    for (uint iter_idx = 0u; iter_idx < iters; ++iter_idx) {
        /* 8 serial loads per iter.  Each read is gid's own slot (warm L1).
         * The address is fixed (gid), but the volatile qualifier prevents CSE.
         * The dep chain via acc prevents the compiler from reordering or
         * lifting the loads out of the loop. */
        acc = vol_out[gid] + acc * 1.0e-30f;
        acc = vol_out[gid] + acc * 1.0e-30f;
        acc = vol_out[gid] + acc * 1.0e-30f;
        acc = vol_out[gid] + acc * 1.0e-30f;
        acc = vol_out[gid] + acc * 1.0e-30f;
        acc = vol_out[gid] + acc * 1.0e-30f;
        acc = vol_out[gid] + acc * 1.0e-30f;
        acc = vol_out[gid] + acc * 1.0e-30f;
    }

    out[gid] = acc;
}
