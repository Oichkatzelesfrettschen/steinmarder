/*
 * atomic_cas_throughput.cl — LDS vs global atomic CAS latency on TeraScale-2
 *
 * Measures:
 * 1. LDS atomic_cmpxchg latency (dependent chain — serial)
 * 2. LDS atomic_cmpxchg throughput (independent — parallel)
 * 3. Global atomic_cmpxchg latency (dependent chain)
 * 4. Global atomic_cmpxchg throughput (independent)
 *
 * On Evergreen, LDS atomics use the LDS unit (32KB shared per SIMD).
 * Global atomics use MEM_RAT_NOCACHE with ATOMIC_CMPXCHG opcode.
 *
 * Expected: LDS atomic ~8-16 cycles, global atomic ~100+ cycles
 * Cross-reference with Apple Metal atomic_cas probe for architecture comparison.
 *
 * Build: gcc -O2 -o atomic_cas_driver atomic_cas_driver.c -lOpenCL -lm
 * Run:   RUSTICL_ENABLE=r600 R600_DEBUG=cs ./atomic_cas_driver atomic_cas_throughput.cl
 */

/* Test 1: LDS atomic CAS — dependent chain (measures latency)
 * Each CAS depends on the previous CAS result → serialized */
__kernel void lds_atomic_cas_latency(
    __global uint *result,
    __local uint *lds_val,
    const uint num_iterations
)
{
    uint lid = get_local_id(0);

    /* Initialize LDS */
    if (lid == 0)
        lds_val[0] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);

    /* Only thread 0 does the dependent chain to avoid contention artifacts */
    if (lid == 0) {
        uint expected = 0;
        for (uint i = 0; i < num_iterations; i++) {
            /* CAS: if lds_val[0] == expected, set to expected+1, return old.
             * Each iteration depends on the previous result. */
            uint old = atomic_cmpxchg(lds_val, expected, expected + 1);
            expected = old + 1;
        }
        result[0] = expected;
    }
}

/* Test 2: LDS atomic CAS — independent (measures throughput)
 * Each thread atomically increments its own LDS slot → no contention */
__kernel void lds_atomic_cas_throughput(
    __global uint *result,
    __local uint *lds_slots,
    const uint num_iterations
)
{
    uint lid = get_local_id(0);

    /* Each thread gets its own LDS slot */
    lds_slots[lid] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (uint i = 0; i < num_iterations; i++) {
        uint expected = i;
        atomic_cmpxchg(&lds_slots[lid], expected, expected + 1);
    }

    barrier(CLK_LOCAL_MEM_FENCE);
    result[get_global_id(0)] = lds_slots[lid];
}

/* Test 3: Global atomic CAS — dependent chain (measures latency)
 * Single-threaded CAS chain on global memory */
__kernel void global_atomic_cas_latency(
    __global volatile uint *atomic_var,
    __global uint *result,
    const uint num_iterations
)
{
    uint gid = get_global_id(0);

    if (gid == 0) {
        atomic_var[0] = 0;
        barrier(CLK_GLOBAL_MEM_FENCE);

        uint expected = 0;
        for (uint i = 0; i < num_iterations; i++) {
            uint old = atomic_cmpxchg(atomic_var, expected, expected + 1);
            expected = old + 1;
        }
        result[0] = expected;
    }
}
