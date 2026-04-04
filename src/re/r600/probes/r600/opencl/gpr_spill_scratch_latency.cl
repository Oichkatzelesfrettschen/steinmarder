/*
 * gpr_spill_scratch_latency.cl — force 124+ GPR usage, measure scratch overhead.
 *
 * This kernel intentionally exceeds the ~123 usable GPR limit on TeraScale-2
 * to force the SFN register allocator to spill to scratch memory. By comparing
 * execution time against the occupancy_gpr_96 baseline (which fits in GPRs),
 * we can measure the per-spill cost:
 *
 *   scratch_overhead = (spill_kernel_ns - baseline_96_ns) / estimated_spills
 *
 * The design document (GPR_SPILL_DESIGN.md) predicts:
 *   - Scratch WRITE: ~27 cycles (CF_OP_MEM_SCRATCH, no clause break)
 *   - Scratch READ:  ~32-43 cycles (FETCH_OP_READ_SCRATCH, forces TEX clause)
 *   - Asymmetric: reads are 1.2-1.6x more expensive than writes
 *
 * Three variants at increasing pressure levels:
 *   1. gpr_spill_128: 128 live floats (just over the limit, ~5 spills expected)
 *   2. gpr_spill_160: 160 live floats (moderate spilling, ~37 spills)
 *   3. gpr_spill_192: 192 live floats (heavy spilling, ~69 spills)
 *
 * NOTE: This kernel will CRASH on current Mesa r600 (no spill support).
 * It becomes runnable once SFN GPR spill infrastructure (task #1) is done.
 * Until then, the crash itself is diagnostic: "Register allocation failed."
 *
 * Build: RUSTICL_ENABLE=r600 R600_DEBUG=cs clang -cl-std=CL1.2
 * ISA check: look for MEM_SCRATCH and FETCH_OP_READ_SCRATCH in output
 */

/* Helper: MULADD chain that forces all values to stay live.
 * Each value depends on its left neighbor (a[i-1]) and right neighbor (a[i+1]),
 * creating a circular dependency that prevents dead-code elimination. */
#define CHAIN_STEP(a, b, c, k)  ((a) * (1.0f + 0.03125f * (float)(k)) + 0.125f + (b) * 0.0009765625f + (c) * 0.00048828125f)

/* Variant 1: 128 live floats — just over the GPR limit.
 * Expected: ~5 spills if RA is smart, catastrophic failure if no spill support. */
__kernel void gpr_spill_128(
    __global float *out,
    const uint iterations
)
{
    const uint gid = get_global_id(0);
    const float seed = (float)gid;

    /* Initialize 128 floats with unique values to prevent merging */
    float v[128];
    for (int i = 0; i < 128; i++) {
        v[i] = (float)(i + 1) + seed * (0.0001f * (float)(i + 1));
    }

    /* Iterate: circular dependency chain across all 128 values */
    for (uint iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < 128; i++) {
            int left = (i > 0) ? i - 1 : 127;
            int right = (i < 127) ? i + 1 : 0;
            v[i] = CHAIN_STEP(v[i], v[left], v[right], i & 7);
        }
    }

    /* Reduce: sum all values to prevent dead-code elimination */
    float sum = 0.0f;
    for (int i = 0; i < 128; i++) {
        sum += v[i];
    }
    out[gid] = sum;
}

/* Variant 2: 160 live floats — moderate spill pressure.
 * ~37 values must be spilled. Measures steady-state spill cost. */
__kernel void gpr_spill_160(
    __global float *out,
    const uint iterations
)
{
    const uint gid = get_global_id(0);
    const float seed = (float)gid;

    float v[160];
    for (int i = 0; i < 160; i++) {
        v[i] = (float)(i + 1) + seed * (0.0001f * (float)(i + 1));
    }

    for (uint iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < 160; i++) {
            int left = (i > 0) ? i - 1 : 159;
            int right = (i < 159) ? i + 1 : 0;
            v[i] = CHAIN_STEP(v[i], v[left], v[right], i & 7);
        }
    }

    float sum = 0.0f;
    for (int i = 0; i < 160; i++) {
        sum += v[i];
    }
    out[gid] = sum;
}

/* Variant 3: 192 live floats — heavy spill pressure.
 * ~69 values must be spilled. Tests whether batch reload optimization
 * (GPR_SPILL_DESIGN.md §5.4) amortizes the clause-switch overhead. */
__kernel void gpr_spill_192(
    __global float *out,
    const uint iterations
)
{
    const uint gid = get_global_id(0);
    const float seed = (float)gid;

    float v[192];
    for (int i = 0; i < 192; i++) {
        v[i] = (float)(i + 1) + seed * (0.0001f * (float)(i + 1));
    }

    for (uint iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < 192; i++) {
            int left = (i > 0) ? i - 1 : 191;
            int right = (i < 191) ? i + 1 : 0;
            v[i] = CHAIN_STEP(v[i], v[left], v[right], i & 7);
        }
    }

    float sum = 0.0f;
    for (int i = 0; i < 192; i++) {
        sum += v[i];
    }
    out[gid] = sum;
}
