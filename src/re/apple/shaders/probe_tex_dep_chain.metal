#include <metal_stdlib>
using namespace metal;

// C4: AGX Texture Sample Dep-Chain Latency Probe
//
// Goal: measure dep-chain latency of a 2D RGBA32F texture sample on M1 GPU
//   (L1 texture cache hit — 1×1 texture, same texel every sample).
//
// STRUCTURE: 4-op unrolled dep-chain, 100K outer iters (matches H1b/H1c budget).
//
// MEASUREMENT METHOD: delta against probe_fma4_osc_dep (same metallib via merge):
//   Both kernels run interleaved. FMA dep-chain step = 2.20 cycles (calibrated).
//   probe_tex_dep4 step  = L_TEX + L_FMA  (texture sample + FMA in series)
//   probe_fma4_osc_dep step = L_FMA
//   Δ(tex − fma) = L_TEX   [adj_lat of one texture sample]
//
// DEP CHAIN DESIGN:
//   coord → tex.sample(samp, coord) → s [float4] → fma(s.xy, 0.4, 0.3) → new coord
//   The fma result feeds directly into the next tex.sample as the UV coordinate.
//
// FIXED POINT ANALYSIS:
//   Texture contains {0.5, 0.5, 0.5, 1.0} (RGBA32F, 1×1 pixel).
//   coord_next = fma(s.xy, 0.4, 0.3) = fma({0.5, 0.5}, 0.4, 0.3) = {0.5, 0.5} ✓
//   Stable after first iteration. No degenerate convergence.
//   Using 0.4 and 0.3 (not 0.5) to avoid LLVM constant folding or shift substitution.
//
// LLVM INVARIANCE:
//   `tex.sample(samp, coord)` reads from device memory (texture unit),
//   with runtime-variable coord — LLVM cannot constant-fold or hoist.
//   The fma constants are compile-time but neither operand is a special value
//   (0.0, 0.5, 1.0, power-of-2) that would trigger single-instruction substitution.
//
// OUTPUT:
//   out[gid] = as_type<uint>(coord.x) — forces coord to remain live throughout.
//
// Build:
//   xcrun -sdk macosx metal -c -O2 -o probe_tex_dep_chain.air probe_tex_dep_chain.metal
//
// Merge with SFU dep chain for interleaved run (EXP2/FMA clock reference):
//   xcrun -sdk macosx metallib probe_sfu_dep_chain.air probe_tex_dep_chain.air \
//       -o probe_tex_combined.metallib
//
// Run:
//   ./metal_probe_host --metallib probe_tex_combined.metallib \
//       --kernel probe_loop_only \
//       --kernel probe_fma4_osc_dep \
//       --kernel probe_exp2_dep4 \
//       --kernel probe_tex_dep4 \
//       --width 1024 --iters 200 --shader-iters 100000 --interleave
//
// Expected:
//   fma4_osc_dep step = 2.20 cyc (calibration)
//   exp2_dep4 step    = 6.76 cyc (cross-check)
//   tex_dep4 step     = 2.20 + L_TEX cyc
//   Δ(tex − fma)      = L_TEX cyc   [expected: ~6–9 cyc for L1-warm RGBA32F on M1]

// =====================================================================
// C4 TEXTURE SAMPLE DEP-CHAIN × 4 PER ITER
// =====================================================================
kernel void probe_tex_dep4(
    texture2d<float> tex              [[texture(0)]],
    device uint* out                  [[buffer(0)]],
    constant uint& n_iters            [[buffer(1)]],
    uint gid                          [[thread_position_in_grid]])
{
    // Constexpr sampler: linear filter, clamp_to_edge.
    // Defined in-shader so the host does not need to bind a sampler state.
    constexpr sampler samp(filter::linear,
                           address::clamp_to_edge,
                           mip_filter::none);

    // Initial coord: fixed point of fma(x, 0.4f, 0.3f) is x = 0.5.
    // Use gid-derived perturbation to prevent LLVM from hoisting the sample
    // outside the loop — gid ensures a runtime-variable initial coord.
    float gid_frac = float(gid & 0x3FFu) * (1.0f / 1024.0f);
    float2 coord = float2(0.5f + gid_frac * 1e-9f, 0.5f);  // ≈ (0.5, 0.5)

    for (uint iter = 0u; iter < n_iters; iter++) {
        // Step 0: coord → tex sample → FMA → new coord
        float4 s0 = tex.sample(samp, coord);
        coord = fma(s0.xy, float2(0.4f), float2(0.3f));

        // Step 1
        float4 s1 = tex.sample(samp, coord);
        coord = fma(s1.xy, float2(0.4f), float2(0.3f));

        // Step 2
        float4 s2 = tex.sample(samp, coord);
        coord = fma(s2.xy, float2(0.4f), float2(0.3f));

        // Step 3
        float4 s3 = tex.sample(samp, coord);
        coord = fma(s3.xy, float2(0.4f), float2(0.3f));
    }
    // Fold both components to prevent dead-code elimination of the coord dep chain.
    out[gid] = as_type<uint>(coord.x) ^ as_type<uint>(coord.y);
}
