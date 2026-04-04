/*
 * vtx_fetch_latency.cl — Measure global memory read latency via dependent chain
 *
 * On TeraScale-2, both vertex fetches (VFETCH) and texture samples (SAMPLE)
 * go through the Texture Cache (TC). Global memory reads in compute also
 * go through the TC. This probe measures the TC read latency by creating
 * a pointer-chase chain through global memory.
 *
 * Method: Buffer contains indices forming a linked list.
 *   buf[0] = 7, buf[7] = 3, buf[3] = 12, buf[12] = 1, ...
 * Each read depends on the previous read's result (pointer chase).
 * Total time / chain_length = per-read latency.
 *
 * Expected: ~38 cycles (matching TEX L1 measurement) if TC L1 hits.
 * With stride > L1 size: should see L2 latency (~200+ cycles).
 *
 * Build: gcc -O2 -o vtx_fetch_driver vtx_fetch_driver.c -lOpenCL -lm
 * Run:   RUSTICL_ENABLE=r600 ./vtx_fetch_driver vtx_fetch_latency.cl
 */

/* Pointer-chase through global memory.
 * Each work-item traverses the chain independently. */
__kernel void pointer_chase(
    __global const uint *chain,  /* chain[i] = next index */
    __global uint *result,       /* output: final index after N hops */
    const uint num_hops          /* chain length */
)
{
    uint idx = get_global_id(0) % 64;  /* start position (spread across cache lines) */

    for (uint hop = 0; hop < num_hops; hop++) {
        idx = chain[idx];  /* dependent read: next index depends on current */
    }

    result[get_global_id(0)] = idx;
}

/* Stride-sweep variant: reads at configurable stride to probe L1/L2/DRAM.
 * Small stride (< 8KB) → TC L1 hits
 * Medium stride (8KB-256KB) → TC L2 hits
 * Large stride (> 256KB) → DRAM */
__kernel void strided_fetch(
    __global const float *data,
    __global float *result,
    const uint stride_elements,  /* stride in float elements */
    const uint num_reads
)
{
    uint gid = get_global_id(0);
    float acc = 0.0f;
    uint offset = gid * stride_elements;

    for (uint i = 0; i < num_reads; i++) {
        acc += data[offset];
        offset = (offset + stride_elements) % (stride_elements * 1024);
    }

    result[gid] = acc;
}
