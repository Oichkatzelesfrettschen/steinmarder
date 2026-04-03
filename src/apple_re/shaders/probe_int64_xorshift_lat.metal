#include <metal_stdlib>

using namespace metal;

// INT64 XOR-shift dep chain — non-foldable because each step depends non-linearly
// on the accumulator.  LLVM cannot simplify  acc ^= (acc >> k)  to a closed form.
//
// Each "step" = 3 ops: xorshift-64 round (xor+shift, xor+shift, xor+shift).
// 8 rounds × 3 ops = 24 ALU ops per inner iter.
// With iters=100000: 2.4M ops per dispatch.
// Compare probe_int32_xorshift_lat.metal to determine 32- vs 64-bit latency tier.
// If AGX has native 64-bit ALU, timing ≈ int32 xorshift.
// If AGX emulates i64 via i32 pairs, timing should be ~2-3× int32 xorshift.
// ops_per_iter = 24
kernel void probe_int64_xorshift_lat(device uint *out     [[buffer(0)]],
                                      constant uint &iters [[buffer(1)]],
                                      uint gid             [[thread_position_in_grid]]) {
    ulong acc = (ulong)(gid + 1u);

    for (uint i = 0; i < iters; ++i) {
        // Xorshift64 — each step is a true dep chain; no closed form exists
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
        acc ^= acc << 13ul; acc ^= acc >> 7ul;  acc ^= acc << 17ul;
    }

    // Fold 64-bit result to 32-bit to stay compatible with host harness (uint output)
    out[gid] = uint(acc) ^ uint(acc >> 32);
}
