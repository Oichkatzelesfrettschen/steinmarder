#include <metal_stdlib>

using namespace metal;

// FP8x8 software-packed dep chain.
//
// Metal/AGX has no native FP8 type.  This probe packs 8 FP8_e4m3 values
// into 2×uint32 (4 bytes each = 32-bit word), decodes them to float32 for
// arithmetic, then re-encodes and repacks.  This is a throughput and
// latency benchmark for the FULL software FP8 round-trip on AGX, not a
// native-type probe.
//
// FP8 e4m3 format (as used in OCP MX spec, no infinities):
//   bit 7:     sign
//   bits 6:4:  exponent (4 bits, bias = 7)
//   bits 3:0:  mantissa (3 bits, implicit leading 1 for normal numbers)
//   special:   0x7F = NaN; exp=0 → subnormal (no inf)
//   max value: 448.0  (0x7E = 0 1111 110 = 1.110 × 2^7 = 240; actually 0x7E = 126dec)
//   Wait: e4m3 has exponent bias 7, max normal exp = 1111_2 - 1 = 14.
//   Actual max = 0b 0 1111 110 = 1.110_2 × 2^(15-7) = 1.75 × 256 = 448.
//
// Representation:
//   pack0 = fp8[0..3] in bytes [7:0], [15:8], [23:16], [31:24]
//   pack1 = fp8[4..7] in bytes [7:0], [15:8], [23:16], [31:24]
//
// Dep chain strategy:
//   Each step decodes all 8 FP8 values to float32, sums them (8 additions),
//   adds a constant delta, then re-encodes and repacks.
//   This is a TRUE dep chain: encode(step n) → decode(step n+1).
//
// Precision:  8 × e4m3 in non-overlapping representation:
//   fp8[0] holds the most significant 3 mantissa bits
//   fp8[i] is scaled by 2^(-3i) relative to fp8[0]
//   Total mantissa bits: 8 × 3 = 24 bits — approximately float32 precision.
//   This is NOT FP64 equivalent; for FP64 (52 mantissa bits) you would need
//   at least 18 FP8_e4m3 values.
//
// Key measurement question:
//   How expensive is the FP8 software round-trip per accumulation step on AGX?
//   Compare with probe_fp16x2_dd_lat (hardware half-float, 4 ops/step)
//   and probe_dd_lat (hardware float32, 4 ops/step).
//   Expected: FP8x8 is significantly slower due to encode/decode overhead.
//
// ops_per_iter = 8  (FP8x8 accumulation steps, each step = 1 decode+add+encode)
//
// Useful as a baseline for "what is the cost of software FP8 in Metal shaders?"
// Relevant for GPU ML workloads that emulate FP8 on hardware without native FP8.

// ── FP8 e4m3 codec (inline, no NaN/subnormal handling for perf) ─────────────

inline float fp8e4m3_decode(uint8_t raw) {
    uint sign     = (raw >> 7u) & 1u;
    uint exp_bits = (raw >> 3u) & 0xFu;
    uint man_bits = raw & 0x7u;

    if (exp_bits == 0u) {
        // Subnormal: value = (-1)^sign × 0.mantissa × 2^(-6)
        float mag = float(man_bits) * (1.0f / 512.0f);   // man/8 × 2^(-6)
        return sign ? -mag : mag;
    }
    // Normal: value = (-1)^sign × 1.mantissa × 2^(exp - 7)
    float mantissa = 1.0f + float(man_bits) * (1.0f / 8.0f);
    float exponent = exp2(float(int(exp_bits) - 7));
    float mag = mantissa * exponent;
    return sign ? -mag : mag;
}

inline uint8_t fp8e4m3_encode(float v) {
    uint raw = as_type<uint>(v);
    uint sign = (raw >> 31u) & 1u;
    int  biased_exp = int((raw >> 23u) & 0xFFu) - 127;
    uint mantissa24 = raw & 0x7FFFFFu;

    // Clamp to FP8 range (max normal = 448.0)
    if (v != v || v >= 448.0f)  return uint8_t((sign << 7u) | 0x7Eu);
    if (v <= -448.0f)           return uint8_t((sign << 7u) | 0x7Eu);

    if (biased_exp < -9) return uint8_t(sign << 7u);   // underflow to ±0

    if (biased_exp < -6) {
        // Subnormal FP8: shift mantissa right
        uint shift = uint(-6 - biased_exp);
        uint man3 = ((mantissa24 | 0x800000u) >> (21u + shift)) & 0x7u;
        return uint8_t((sign << 7u) | man3);
    }

    // Normal FP8
    int fp8_exp = biased_exp + 7;
    if (fp8_exp > 14) { fp8_exp = 15; }  // clamp exponent
    uint man3 = (mantissa24 >> 20u) & 0x7u;  // top 3 mantissa bits
    return uint8_t((sign << 7u) | (uint(fp8_exp) << 3u) | man3);
}

// ── kernel ───────────────────────────────────────────────────────────────────

kernel void probe_fp8x8_lat(device uint *out     [[buffer(0)]],
                             constant uint &iters [[buffer(1)]],
                             uint gid             [[thread_position_in_grid]]) {
    // Initial state: fp8[0] = 1.0, fp8[1..7] = 0 (packed as uint32 pairs).
    // fp8e4m3_encode(1.0f) = 0b 0_0111_000 = 0x38
    uint pack0 = 0x00000038u;   // [fp8[3], fp8[2], fp8[1], fp8[0]] = {0,0,0,1.0}
    uint pack1 = 0x00000000u;   // [fp8[7], fp8[6], fp8[5], fp8[4]] = {0,0,0,0}

    // Per-thread offset so threads produce distinct outputs.
    float thread_offset = float(gid) * (1.0f / 65536.0f);
    pack0 = (pack0 & 0xFFFFFF00u) | uint(fp8e4m3_encode(1.0f + thread_offset));

    // delta = fp8e4m3_encode(0.125f) = 0b 0_0100_000 = 0x20
    const uint8_t fp8_delta = 0x20u;   // +0.125 in FP8 e4m3

    for (uint i = 0; i < iters; ++i) {
        // 8 FP8x8 accumulation steps.
        // Each step: decode 8 FP8 values → sum + delta → re-encode & repack.

        for (uint step = 0u; step < 8u; ++step) {
            // Decode 8 FP8 values from pack0 and pack1.
            float f0 = fp8e4m3_decode(uint8_t( pack0        & 0xFFu));
            float f1 = fp8e4m3_decode(uint8_t((pack0 >>  8) & 0xFFu));
            float f2 = fp8e4m3_decode(uint8_t((pack0 >> 16) & 0xFFu));
            float f3 = fp8e4m3_decode(uint8_t((pack0 >> 24) & 0xFFu));
            float f4 = fp8e4m3_decode(uint8_t( pack1        & 0xFFu));
            float f5 = fp8e4m3_decode(uint8_t((pack1 >>  8) & 0xFFu));
            float f6 = fp8e4m3_decode(uint8_t((pack1 >> 16) & 0xFFu));
            float f7 = fp8e4m3_decode(uint8_t((pack1 >> 24) & 0xFFu));

            // Add delta to f0 (the highest-significance component).
            f0 += fp8e4m3_decode(fp8_delta);

            // Re-encode all 8 floats back to FP8.
            uint8_t e0 = fp8e4m3_encode(f0);
            uint8_t e1 = fp8e4m3_encode(f1);
            uint8_t e2 = fp8e4m3_encode(f2);
            uint8_t e3 = fp8e4m3_encode(f3);
            uint8_t e4 = fp8e4m3_encode(f4);
            uint8_t e5 = fp8e4m3_encode(f5);
            uint8_t e6 = fp8e4m3_encode(f6);
            uint8_t e7 = fp8e4m3_encode(f7);

            // Repack into 2×uint32.
            pack0 = uint(e0) | (uint(e1) << 8) | (uint(e2) << 16) | (uint(e3) << 24);
            pack1 = uint(e4) | (uint(e5) << 8) | (uint(e6) << 16) | (uint(e7) << 24);
        }
    }

    out[gid] = pack0 ^ pack1;
}
