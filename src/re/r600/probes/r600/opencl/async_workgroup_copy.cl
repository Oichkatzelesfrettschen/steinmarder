/*
 * async_workgroup_copy.cl — async_work_group_copy latency measurement.
 *
 * Tests whether r600/Rusticl's async_work_group_copy implementation actually
 * overlaps data movement with compute, or falls back to synchronous memcpy.
 *
 * TeraScale-2 has 32 KB LDS per SIMD. async_work_group_copy moves data
 * between global memory and LDS. On GCN+ this maps to DMA or LDS_DIRECT;
 * on TeraScale-2 the implementation may be a software loop.
 *
 * Three measurement modes:
 *   1. sync_baseline:  explicit loop copy + barrier (known synchronous cost)
 *   2. async_copy:     async_work_group_copy + wait_group_events (true async?)
 *   3. async_overlap:  async_work_group_copy + ALU work + wait (overlap test)
 *
 * If async is truly asynchronous, mode 3 should be ~equal to max(copy, ALU).
 * If it's synchronous, mode 3 = copy + ALU (no overlap benefit).
 *
 * Build: RUSTICL_ENABLE=r600 R600_DEBUG=cs clang -cl-std=CL1.2
 */

#define TILE_SIZE 256

/* Mode 1: Synchronous baseline — explicit copy loop */
__kernel void async_copy_sync_baseline(
    __global const float *in,
    __global float *out,
    const uint tile_count
)
{
    __local float lds_buf[TILE_SIZE];
    const uint lid = get_local_id(0);
    const uint gid = get_global_id(0);
    const uint group_id = get_group_id(0);

    float acc = 0.0f;

    for (uint tile = 0; tile < tile_count; tile++) {
        uint tile_base = (group_id * tile_count + tile) * TILE_SIZE;

        /* Synchronous copy: each work-item copies one element */
        lds_buf[lid] = in[tile_base + lid];
        barrier(CLK_LOCAL_MEM_FENCE);

        /* Compute: reduction over LDS tile */
        for (uint stride = TILE_SIZE / 2; stride > 0; stride >>= 1) {
            if (lid < stride) {
                lds_buf[lid] += lds_buf[lid + stride];
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }

        if (lid == 0) {
            acc += lds_buf[0];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (lid == 0) {
        out[group_id] = acc;
    }
}

/* Mode 2: async_work_group_copy — direct measurement */
__kernel void async_copy_direct(
    __global const float *in,
    __global float *out,
    const uint tile_count
)
{
    __local float lds_buf[TILE_SIZE];
    const uint lid = get_local_id(0);
    const uint group_id = get_group_id(0);

    float acc = 0.0f;

    for (uint tile = 0; tile < tile_count; tile++) {
        uint tile_base = (group_id * tile_count + tile) * TILE_SIZE;

        /* Async copy: hardware/runtime moves data to LDS */
        event_t ev = async_work_group_copy(
            lds_buf, in + tile_base, TILE_SIZE, 0);
        wait_group_events(1, &ev);

        /* Same reduction as sync baseline */
        for (uint stride = TILE_SIZE / 2; stride > 0; stride >>= 1) {
            if (lid < stride) {
                lds_buf[lid] += lds_buf[lid + stride];
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }

        if (lid == 0) {
            acc += lds_buf[0];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (lid == 0) {
        out[group_id] = acc;
    }
}

/* Mode 3: async_work_group_copy with overlapped ALU work.
 * If async is real, the ALU work hides behind the copy latency. */
__kernel void async_copy_overlap(
    __global const float *in,
    __global float *out,
    const uint tile_count
)
{
    __local float lds_buf[TILE_SIZE];
    __local float lds_prev[TILE_SIZE];
    const uint lid = get_local_id(0);
    const uint group_id = get_group_id(0);

    float acc = 0.0f;
    float local_work = (float)lid * 0.001f;

    /* Prime the first tile synchronously */
    uint tile_base = group_id * tile_count * TILE_SIZE;
    event_t ev = async_work_group_copy(lds_buf, in + tile_base, TILE_SIZE, 0);
    wait_group_events(1, &ev);

    for (uint tile = 1; tile < tile_count; tile++) {
        /* Copy current tile to prev buffer */
        lds_prev[lid] = lds_buf[lid];
        barrier(CLK_LOCAL_MEM_FENCE);

        /* Start async copy of NEXT tile while we compute on PREVIOUS */
        uint next_base = (group_id * tile_count + tile) * TILE_SIZE;
        ev = async_work_group_copy(lds_buf, in + next_base, TILE_SIZE, 0);

        /* ALU work on previous tile (should overlap with copy) */
        float tile_sum = lds_prev[lid];
        for (int i = 0; i < 64; i++) {
            local_work = local_work * 1.0001f + tile_sum * 0.0001f;
        }

        /* Wait for copy to complete */
        wait_group_events(1, &ev);

        /* Reduce current tile */
        for (uint stride = TILE_SIZE / 2; stride > 0; stride >>= 1) {
            if (lid < stride) {
                lds_buf[lid] += lds_buf[lid + stride];
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }

        if (lid == 0) {
            acc += lds_buf[0];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (lid == 0) {
        out[group_id] = acc + local_work;
    }
}
