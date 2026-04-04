#include <metal_stdlib>

using namespace metal;

// Register-light occupancy probe.
// Minimal register footprint: single uint accumulator, no FP,
// no temporary variables beyond the loop counter.
// Designed to maximize GPU occupancy by keeping register pressure
// as low as possible — the opposite end of the spectrum from
// probe_register_pressure.metal (3 accumulators + float conversions).
//
// xctrace density behavior expected:
//   register_pressure: −609.9 r/s on metal-gpu-state-intervals (register-heavy)
//   register_light:    should show POSITIVE delta vs. baseline
//     because the shader retires faster per unit time, leaving room for
//     more state transitions in the same measurement window.
//
// This is the "bracket from below" variant called for in
// FRONTIER_ROADMAP_APPLE.md checklist item "register-light variant".
kernel void probe_register_light(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    uint acc = gid;
    // 32 iterations of a single uint operation — no extra registers needed.
    // The compiler should be able to keep this in one register throughout.
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    acc = acc * 1664525u + 1013904223u;
    out[gid] = acc;
}
