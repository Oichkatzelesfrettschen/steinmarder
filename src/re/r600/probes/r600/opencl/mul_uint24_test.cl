/*
 * mul_uint24_test.cl — Verify MUL_UINT24 emission on TeraScale-2
 *
 * MUL_UINT24 is a single-cycle 24-bit unsigned integer multiply that
 * executes in any vec slot (x,y,z,w), leaving the t-slot free for
 * transcendentals. Standard MULLO_INT is multi-cycle and t-slot-only.
 *
 * SFN emits MUL_UINT24 when nir_op_imul has at least one constant
 * operand < 2^24. This kernel tests:
 * 1. Constant * constant (both < 2^24)
 * 2. Variable * small constant (constant < 2^24)
 * 3. Q8.16 fixed-point multiplication pattern
 *
 * Run: RUSTICL_ENABLE=r600 R600_DEBUG=cs ./mul_uint24_driver mul_uint24_test.cl
 * Verify: grep 'MUL_UINT24' in ISA output (should appear, NOT MULLO_INT)
 *
 * Build driver: gcc -O2 -o mul_uint24_driver mul_uint24_driver.c -lOpenCL -lm
 */

/* Test 1: Integer multiply with small constant → MUL_UINT24 */
__kernel void mul_uint24_basic(
    __global const uint *input,
    __global uint *output,
    const uint scale       /* must be < 2^24 for MUL_UINT24 */
)
{
    uint gid = get_global_id(0);
    /* scale is a kernel arg constant < 2^24 → should emit MUL_UINT24 */
    output[gid] = input[gid] * scale;
}

/* Test 2: Q8.16 fixed-point multiply
 * Q8.16: 8 bits integer, 16 bits fraction, fits in 24 bits.
 * mul(a, b) = (a * b) >> 16
 *
 * Example: 1.5 * 2.0 in Q8.16:
 *   1.5 = 1.5 * 65536 = 98304 (fits in 24 bits: 0x018000)
 *   2.0 = 2.0 * 65536 = 131072 (fits in 24 bits: 0x020000)
 *   But 98304 * 131072 = 12884901888 which overflows 32 bits!
 *
 * So for Q8.16 × Q8.16, we need smaller values or use Q4.8:
 *   Q4.8: 4 bits integer, 8 bits fraction, fits in 12 bits
 *   1.5 in Q4.8 = 384 (0x180)
 *   2.0 in Q4.8 = 512 (0x200)
 *   384 * 512 = 196608, >> 8 = 768 = 3.0 in Q4.8 ✓
 */
__kernel void q4_8_fixed_point_mul(
    __global const uint *a_fixed,    /* Q4.8 values */
    __global const uint *b_fixed,    /* Q4.8 values */
    __global uint *result_fixed,     /* Q4.8 result */
    __global float *result_float     /* float for verification */
)
{
    uint gid = get_global_id(0);

    uint a = a_fixed[gid];   /* Q4.8 */
    uint b = b_fixed[gid];   /* Q4.8 */

    /* Both operands fit in 12 bits, well under 24.
     * SFN should emit MUL_UINT24 for this multiply. */
    uint product = a * b;

    /* Shift right by 8 to get Q4.8 result */
    uint q_result = product >> 8;

    result_fixed[gid] = q_result;

    /* Convert to float for verification: divide by 256 (2^8) */
    result_float[gid] = (float)q_result / 256.0f;
}
