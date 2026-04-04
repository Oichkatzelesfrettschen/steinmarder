# VLIW5 ISA Slot Analysis — AMD Radeon HD 6310 (TeraScale-2/Evergreen)

**Date**: 2026-03-29
**Hardware**: AMD Radeon HD 6310 (Wrestler die, VLIW5, 2 CU)
**Driver**: Mesa 25.2.6-1~bpo13+1 r600g Gallium, kernel 6.18.20-x64v1-xanmod1
**ISA Reference**: AMD_Evergreen-Family_ISA.pdf (Chapter 3: ALU Encoding)
**Tool**: r600-session.sh RE toolkit (R600_DEBUG=vs,ps,cs,precompile)

---

## 1. Executive Summary

Across three profiling sessions (glxgears, glmark2 offscreen, glmark2 on-screen),
the Mesa r600 shader compiler achieves **64-74% VLIW5 slot utilization** on
non-trivial shaders. This means **26-36% of ALU capacity is wasted** per cycle
due to unfilled VLIW5 slots.

The transcendental (t) slot is well-utilized at 62-75% of bundles, mainly by
RECIPSQRT_IEEE and MUL_IEEE (overflow from x/y/z/w).

The biggest optimization opportunities are:
1. Better instruction scheduling to fill 2-3 slot bundles up to 4-5
2. Reducing the number of trivial export-only shader variants
3. Exploiting the t-slot for more transcendental math

---

## 2. VLIW5 Architecture Background

The Evergreen VLIW5 unit issues 5 ALU operations per cycle per SIMD:
- **x, y, z, w**: 4 general-purpose ALU slots (IEEE MUL/ADD/MAD)
- **t**: 1 transcendental slot (SIN, COS, LOG, EXP, RSQ, RCP, SQRT, plus MUL)

Each ALU "group" (bundle) executes all 5 slots simultaneously. Unfilled slots
are NOPs — pure waste. The HD 6310 has 2 SIMD engines x 5 ALU slots = 10 ALU
ops/cycle theoretical max.

Reference: AMD_Evergreen-Family_ISA.pdf, Chapter 3, Section 3.1

---

## 3. Session Data

### 3.1 glxgears (baseline_gears, 15 seconds)

| Metric | Value |
|--------|-------|
| FPS | 59 (vsync-locked) |
| GPU Utilization | 100% (ee, ta, sx, sh, spi, sc, db, cb all 100%) |
| ISA Lines | 846 |
| GL Trace | 203 KB |
| Non-trivial shaders | ~3 |

**VLIW5 Slot Utilization:**
```
Slot x: 4 instructions
Slot y: 3 instructions
Slot z: 2 instructions
Slot w: 1 instructions
Slot t: 0 instructions
Bundles: 4 | Fills: 10/20 | Utilization: 50% | Avg: 2.50 slots/bundle
```

**Observation**: glxgears is trivially simple — only 4 ALU bundles in the
hot-path vertex shader (MVP transform). The t-slot is completely unused because
the shader is all MAD operations (matrix multiply). 50% utilization shows the
scheduler is barely packing — groups 1-2 have 3 slots but groups 3-4 have only 2.

### 3.2 glmark2 offscreen (baseline_glmark2, 120s, all benchmarks)

| Metric | Value |
|--------|-------|
| FPS | ~155 avg (with apitrace overhead) |
| GPU Utilization | 0% (offscreen bypasses display path) |
| ISA Lines | 7563 |
| GL Trace | 204 MB |
| Total shaders compiled | 92 |
| Non-trivial (have ALU) | 21 |
| GLSL shaders captured | 14 (8 FS + 6 VS) |

**VLIW5 Slot Utilization:**
```
Slot x: 30 instructions
Slot y: 17 instructions
Slot z: 10 instructions
Slot w: 29 instructions
Slot t: 25 instructions
Bundles: 30-32 | Fills: 103-111/150-160 | Utilization: 64-74% | Avg: 3.40-3.70 slots/bundle
```

**Bundle fill distribution (32 analyzed bundles):**
```
1 slot:   2 bundles ( 6.2%)  ###
2 slots: 10 bundles (31.2%)  ###############
3 slots: 12 bundles (37.5%)  ##################
4 slots:  8 bundles (25.0%)  ############
5 slots:  0 bundles ( 0.0%)
```

**Critical finding: ZERO bundles achieve full 5-slot packing.**

**t-slot usage**: 62.5% of bundles use the transcendental slot.
- Opcodes on t: RECIPSQRT_IEEE (10), MUL_IEEE (15)
- MUL_IEEE overflow to t-slot indicates the 4 main slots are full in those cases

### 3.3 glmark2 on-screen (onscreen_glmark2, 50s, selected benchmarks)

| Metric | Value |
|--------|-------|
| FPS | ~155 avg (with apitrace overhead) |
| GPU Utilization | 100% sustained |
| ISA Lines | 3474 |
| GL Trace | 3.1 MB |
| Shaders captured | 8 GLSL |

**VLIW5 Slot Utilization:**
```
Slot x: 10 | y: 5 | z: 2 | w: 8 | t: 9
Bundles: 10 | Fills: 34/50 | Utilization: 68% | Avg: 3.40 slots/bundle
```

---

## 4. Per-Shader Analysis (offscreen glmark2)

### 4.1 Shader Complexity Table

```
 ID Ty   DW GPR Bnd Fill Max  Util tSl TopOp
----------------------------------------------------------------------
  9 VS   88   4   2    7  10   70%   2 MUL_IEEE       (build:use-vbo)
 16 VS   88   4   2    7  10   70%   2 MUL_IEEE       (texture)
 23 VS   92   5   2    7  10   70%   3 MUL_IEEE       (gouraud)
 30 VS   92   5   2    7  10   70%   3 MUL_IEEE       (blinn-phong)
 40 VS   92   5   2    7  10   70%   3 MUL_IEEE       (phong)
 47 VS   88   4   2    7  10   70%   2 MUL_IEEE       (cel)
 54 VS   78   4   2    7  10   70%   2 MUL_IEEE       (bump high-poly)
 61 VS  112   6   2   10  10  100%   2 MUL_IEEE       (bump normals)
 68 VS  104   6   3   10  15   66%   3 MUL_IEEE       (bump height)
 75 VS   78   4   2    7  10   70%   2 MUL_IEEE       (effect2d)
 82 VS   40   4   1    3   5   60%   0 MUL_IEEE       (desktop blur)
 83 FS  100   3   1    1   5   20%   0 MULADD_IEEE    (desktop blur FS)
 84 FS  100   3   1    1   5   20%   0 MULADD_IEEE    (desktop blur FS dup)
 89 VS  174   6   0    8   0   --    1 (partial dump)
 90 FS  152   7   1    2   5   40%   0 ADD            (convolution FS)
 91 FS  152   7   1    2   5   40%   0 ADD            (convolution FS dup)
```

### 4.2 Key Observations

**Shader 61 (bump normals VS) = best utilization at 100%**
- 112 dw, 6 GPRs, 2 bundles, 10/10 fills
- This is the only shader that achieves full VLIW5 packing
- Uses all 5 slots in every bundle

**Shaders 83-84 (desktop blur FS) = worst utilization at 20%**
- 100 dw but only 1 bundle with 1 instruction captured
- Likely dominated by TEX clauses (texture sampling for blur kernel)
- The ALU work is minimal — bottleneck is texture fetch latency

**Shaders 90-91 (convolution FS) = 40% utilization**
- Largest fragment shaders at 152 dw / 7 GPRs
- Only 2 ALU fills in 5 slots
- The 5x5 convolution kernel is likely serialized through TEX fetches
- Opportunity: batch texture coordinates and compute more in ALU

---

## 5. Opcode Frequency Analysis

### Global (all sessions combined)
```
 313  MUL_IEEE        (IEEE float multiply)
 265  MULADD_IEEE     (fused multiply-add — the core MAD op)
 138  DOT_IEEE        (dot product, uses x+y+z slots)
 124  ADD             (float add)
 112  INTERP_XY       (fragment interpolation, x/y barycentrics)
  66  MOV             (register move — scheduling inefficiency?)
  43  MOV             (additional from glxgears)
  40  INTERP_ZW       (fragment interpolation, z/w barycentrics)
  33  RECIPSQRT_IEEE  (1/sqrt, always on t-slot)
  30  MAX             (clamp/max operations)
  12  INTERP_X        (single-component interpolation)
  12  EXP_IEEE        (exponent, t-slot)
   8  TRUNC           (float truncation)
   8  CNDE_INT        (conditional execution)
   6  FLT_TO_INT      (type conversion)
   6  SETGT           (comparison)
   2  SETGE           (comparison)
   2  RECIP_IEEE      (reciprocal, t-slot)
   2  AND_INT         (bitwise ops)
```

### Notable patterns:
- **MUL_IEEE + MULADD_IEEE** dominate = 578 of ~1100 total (~53%)
  - These are the matrix/vector transform workhorses
  - MULADD is a fused MAD — already efficient for VLIW
- **DOT_IEEE** at 138 uses 3 of 5 slots (x, y, z for dot3) — wastes w and t
- **MOV at 109** is concerning — pure data movement, no computation
  - Could indicate register pressure causing spills
  - Or suboptimal register allocation by the compiler
- **INTERP_XY/ZW** pairs always occupy 4 slots but 2 are dead writes (__.x/__.y)
  - This is by hardware design — barycentric interpolation requires paired ops
  - The dead writes are unavoidable

---

## 6. GPU Pipeline Utilization (radeontop data)

From on-screen glmark2 session (53 samples @ 1Hz):

| Block | Utilization | Meaning |
|-------|------------|---------|
| gpu | 100% | Overall GPU busy |
| ee (execution engine) | 100% | Shader cores fully busy |
| vgt (vertex grouper) | 0-100%, mostly 0% | Not geometry-bound |
| ta (texture address) | 100% | Texture unit saturated |
| sx (shader export) | 100% | Fragment output busy |
| sh (shader) | 100% | Shader cores at max |
| spi (shader processor input) | 100% | Shader dispatch full |
| sc (scan converter) | 100% | Rasterizer pegged |
| pa (primitive assembly) | 0%, occasional 100% | Not vertex-bound |
| db (depth buffer) | 100% | Depth testing active |
| cb (color buffer) | 100% | Color writes active |
| vram | ~83 MB / 384 MB (22%) | Low memory pressure |

**Interpretation**: The GPU is **fully saturated on shader execution and
texture fetch**. The bottleneck is raw compute throughput (VLIW5 ALU) and
texture bandwidth (ta). Geometry processing (vgt/pa) is NOT the bottleneck.

This means **improving VLIW5 slot utilization would directly translate to
higher FPS** — we are shader-bound, not memory-bound or geometry-bound.

---

## 7. Optimization Opportunities

### 7.1 HIGH IMPACT: Better VLIW5 Scheduling (compiler-level)

**Current**: 64-74% slot utilization, 0% 5-slot bundles
**Target**: 80%+ utilization, >10% 5-slot bundles

The Mesa r600 backend (src/gallium/drivers/r600/r600_asm.c and r600_shader.c)
does instruction scheduling to pack VLIW5 bundles. Improvements:

1. **DOT product packing**: DOT_IEEE uses x/y/z for the 3-component dot, leaving
   w and t empty. If there is a subsequent independent MUL or ADD, it could be
   scheduled into w or t of the same bundle.

2. **MOV elimination**: 109 MOV instructions suggest the register allocator is
   generating unnecessary copies. A copy-propagation pass before scheduling could
   eliminate many of these.

3. **INTERP dead-write recovery**: INTERP_XY writes to x/y but must also issue
   __.z and __.w. The z/w slots are dead but unavailable. However, the t-slot
   IS available during INTERP bundles and could run an independent transcendental.

### 7.2 MEDIUM IMPACT: Texture Fetch Optimization

The blur/convolution shaders (83-84, 90-91) are dominated by TEX clauses
with minimal ALU. On TeraScale-2, texture fetch latency is hidden by
switching between pixel quads. Strategies:

1. **Batch texture coordinates**: Pre-compute all sample offsets in ALU, then
   issue TEX clause with multiple fetches to maximize parallelism.
2. **Separable kernels**: For 2D convolution, use separable horizontal+vertical
   passes instead of NxN samples. Reduces fetches from N^2 to 2N.

### 7.3 LOW IMPACT: Shader Variant Reduction

92 shaders compiled, 71 are trivial export-only variants. These are
internal Mesa pipeline plumbing shaders (clear, blit, resolve). They have
zero ALU cost but their compilation adds startup latency. Shader caching
(already in Mesa) helps, but reducing variant explosion in the first place
would be better.

---

## 8. Cross-Reference with ISA Docs

### VLIW5 Bundle Encoding (Evergreen ISA, Chapter 3)

Each ALU group is encoded as:
```
[CF_ALU_WORD0] [CF_ALU_WORD1]   -- control flow header (clause start)
  [ALU_WORD0] [ALU_WORD1_OP2/3] -- per-instruction, 2 DWs each
  ...
```

The LAST bit (bit 31 of ALU_WORD0) marks the end of a VLIW5 group.
Looking at the captured bytecode, we can see this:
```
0008 00804485 00000110     1 x:     MUL_IEEE  -- LAST=0 (more in bundle)
0010 00804885 20000110       y:     MUL_IEEE  -- LAST=0
0012 80804085 60000110       w:     MUL_IEEE  -- LAST=1 (0x80... = bit 31 set)
```

The 0x80 prefix on the last instruction of each bundle confirms LAST=1.
This is useful for building a more precise bundle boundary detector.

### Slot Assignment Rules (Evergreen ISA, Ch 3, Section 3.3)

- Transcendental ops (SIN, COS, LOG, EXP, RSQ, RCP, SQRT) MUST go in t-slot
- DOT ops occupy multiple slots: DOT4 uses x,y,z,w; DOT3 uses x,y,z
- INTERP ops are paired: XY and ZW must be in the same bundle
- Any ALU op CAN go in the t-slot if no transcendental is needed

---

## 9. Raw Data Locations

```
~/eric/TerakanMesa/re-toolkit/data/
  session_baseline_gears_20260329T201543/
    isa_and_stderr.log    -- 846 lines, glxgears VLIW5 ISA
    radeontop.csv         -- 15 samples GPU utilization
    gltrace.trace         -- 203K GL API trace
  session_baseline_glmark2_20260329T201615/
    isa_and_stderr.log    -- 7563 lines, glmark2 full VLIW5 ISA
    radeontop.csv         -- 120 samples (offscreen = 0% GPU)
    gltrace.trace         -- 204M GL API trace
    shaders/              -- 14 GLSL source files
  session_onscreen_glmark2_20260329T201847/
    isa_and_stderr.log    -- 3474 lines, glmark2 selected benchmarks
    radeontop.csv         -- 53 samples, 100% GPU
    gltrace.trace         -- 3.1M GL API trace
    shaders/              -- 8 GLSL source files
```

---

## 10. Next Steps

1. **IBS CPU profiling** (r600-uprof-ibs.sh) to find CPU-side bottlenecks
   in the Mesa compiler and GL state management
2. **Vulkan Terakan driver analysis** — audit the semaphore sync bug and
   check its VLIW5 scheduling against r600g
3. **Rusticl compute shader ISA** — profile OpenCL kernel compilation
   for slot utilization vs GL shaders
4. **Mesa source dive** — read r600_asm.c bundle scheduling code to
   understand why 5-slot packing never happens
5. **Patch experiment** — modify scheduler to aggressively pack DOT+MUL
   into the same bundle, measure FPS delta
6. **WebGL** — profile Firefox WebGL workloads through the same toolkit
   to identify WebGL-specific shader patterns

---

*Generated by r600 RE toolkit analysis, 2026-03-29*
*Cross-reference: ~/eric/TerakanMesa/isa-docs/AMD_Evergreen-Family_ISA.pdf*


---

## APPENDIX A: Annotated Shader 6 -- glxgears Main Vertex Shader

This is the hot-path vertex shader from glxgears. It computes:
- MVP matrix * vertex position (4x4 mat * vec4)
- Normal matrix * normal vector + light direction dot product
- Diffuse lighting (clamped dot product * material color)

```
=== CF: CALL_FS @0  (vertex fetch subroutine)
=== CF: ALU 38 @8  KC0[CB0:0-16]  (38 ALU instructions, constant buffer 0)

Bundle 1: [x,y,z] = 3/5 slots = 60% utilization
  x: DOT_IEEE    R0.x,  KC0[8].x, KC0[8].x    ; light_dir.x^2
  y: DOT_IEEE    __.y,  KC0[8].y, KC0[8].y     ; light_dir.y^2 (dead write, DOT accum)
  z: MUL_IEEE    __.z,  KC0[8].z, KC0[8].z     ; light_dir.z^2 (dead write, DOT accum)
  w: EMPTY                                       ; ** WASTED **
  t: EMPTY                                       ; ** WASTED **
  NOTE: DOT3 requires x+y+z but result only in x. w and t are free but
  scheduler found no independent ops to fill them. The ADD in bundle 2
  could have been split -- one ADD here in w-slot.

Bundle 2: [z,w,t] = 3/5 slots = 60% utilization
  x: EMPTY                                       ; ** WASTED **
  y: EMPTY                                       ; ** WASTED **
  z: ADD         T1.z,  KC0[1].z, KC0[0].z      ; normal_mat[2] + light_offset[2]
  w: MOV_sat     R0.w,  KC0[0].w                 ; material alpha (clamped 0-1)
  t: RECIPSQRT   T0.w,  PV.x                     ; 1/sqrt(dot(light_dir, light_dir))
  NOTE: t-slot well used for RSQ. But x,y are empty. The two ADDs in
  bundle 3 could have been scheduled here.

Bundle 3: [x,z] = 2/5 slots = 40% utilization  ** WORST BUNDLE **
  x: ADD         T0.x,  KC0[1].y, KC0[0].y      ; normal_mat[1] + light_offset[1]
  y: EMPTY                                       ; ** WASTED **
  z: ADD         T0.z,  KC0[1].x, KC0[0].x      ; normal_mat[0] + light_offset[0]
  w: EMPTY                                       ; ** WASTED **
  t: EMPTY                                       ; ** WASTED **
  NOTE: These two ADDs have NO data dependency on bundle 2's results.
  They COULD have been scheduled into bundle 1 (w,t) or bundle 2 (x,y).
  This is a clear scheduler miss.

Bundle 4: [x,y,z,w] = 4/5 slots = 80% utilization
  x: MUL_IEEE    R0.x,  KC0[4].y, R1.x          ; MVP col1.y * pos.x
  y: MUL_IEEE    R0.y,  KC0[4].z, R1.x          ; MVP col1.z * pos.x
  z: MUL_IEEE    R0.z,  KC0[4].x, R1.x          ; MVP col1.x * pos.x
  w: MUL_IEEE    R2.w,  KC0[4].w, R1.x          ; MVP col1.w * pos.x
  t: EMPTY                                       ; (could do RSQ/RCP prep)

Bundle 5: [x,y,z,w] = 4/5 slots = 80% utilization
  x: MULADD      R1.x,  KC0[5].w, R1.y, PV.w   ; MVP col2.w * pos.y + prev.w
  y: MULADD      R2.y,  KC0[5].x, R1.y, PV.z   ; MVP col2.x * pos.y + prev.z
  z: MULADD      R2.z,  KC0[5].y, R1.y, PV.x   ; MVP col2.y * pos.y + prev.x
  w: MULADD      R3.w,  KC0[5].z, R1.y, PV.y   ; MVP col2.z * pos.y + prev.y
  t: EMPTY

Bundle 6: [x,y,z,w] = 4/5 slots = 80% utilization
  x: MULADD      R0.x,  KC0[6].w, R1.z, PV.x   ; MVP col3.w * pos.z + prev
  y: MULADD      R0.y,  KC0[6].x, R1.z, PV.y
  z: MULADD      R0.z,  KC0[6].y, R1.z, PV.z
  w: MULADD      R4.w,  KC0[6].z, R1.z, PV.w

Bundle 7: [x,y,z,w] = 4/5 slots = 80% utilization
  x: MULADD      R2.x,  KC0[7].y, R1.w, PV.z   ; MVP col4 * pos.w + prev (final)
  y: MULADD      R2.y,  KC0[7].x, R1.w, PV.y
  z: MULADD      R2.z,  KC0[7].z, R1.w, PV.w
  w: MULADD      R2.w,  KC0[7].w, R1.w, PV.x
  NOTE: Bundles 5-7 are the 4x4 matrix multiply core. All use 4 slots
  for the 4-component MAD. t-slot unused because no transcendental is
  needed during mat*vec, and the PV (previous vector) dependency chain
  prevents scheduling independent ops from elsewhere.

Bundle 8: [y,z,w] = 3/5 slots = 60% utilization
  y: MUL_IEEE    R0.y,  KC0[8].z, T0.w          ; normalized_light.z = light.z * rsq
  z: MUL_IEEE    R0.z,  KC0[8].x, T0.w          ; normalized_light.x = light.x * rsq
  w: MUL_IEEE    R1.w,  KC0[8].y, T0.w          ; normalized_light.y = light.y * rsq
  NOTE: x and t empty. The DOT in bundle 9 reads PV.z/w/y from here.

Bundle 9: [x,y,z] = 3/5 slots = 60% utilization (DOT3 again)
  x: DOT_IEEE    R0.x,  PV.z, KC0[9].x          ; dot(normalized_light, normal)
  y: DOT_IEEE    __.y,  PV.w, KC0[9].y
  z: MUL_IEEE    __.z,  PV.y, KC0[9].z

Bundle 10: [z,w] = 2/5 slots = 40% utilization  ** TIED WORST **
  z: SETGT       R1.z,  PV.x, 0                  ; light_facing = (dot > 0)
  w: MAX_DX10    R1.w,  0, PV.x                   ; clamped_dot = max(0, dot)

Bundle 11: [x,y,w] = 3/5 slots = 60% utilization
  x: MULADD      R1.x,  PV.w, KC0[2].y, T0.x    ; lit_color.y = dot*color.y + offset
  y: MULADD      R1.y,  PV.w, KC0[2].z, T1.z    ; lit_color.z
  w: MULADD      R3.w,  PV.w, KC0[2].x, T0.z    ; lit_color.x

Bundle 12: [x,y,z] = 3/5 slots = 60% utilization
  x: MULADD_sat  R0.x,  R1.z, KC0[3].y, PV.x    ; final.r = clamp(face*mat + lit)
  y: MULADD_sat  R0.y,  R1.z, KC0[3].x, PV.w    ; final.g
  z: MULADD_sat  R0.z,  R1.z, KC0[3].z, PV.y    ; final.b

=== CF: EXPORT_DONE PARAM 0  R0.yxzw  (color output, swizzled)
=== CF: EXPORT_DONE POS  60  R2.yxzw  (position output, swizzled)
```

### Bundle Utilization Summary for Shader 6:

| Bundle | Slots | Util | Issue |
|--------|-------|------|-------|
| 1 | 3/5 | 60% | DOT3 leaves w,t empty; independent ADDs exist |
| 2 | 3/5 | 60% | Good t-slot RSQ; x,y could hold ADDs from bundle 3 |
| 3 | 2/5 | 40% | ** Two ADDs with no deps -- should merge into 1 or 2 |
| 4 | 4/5 | 80% | Good; t-slot available but no transcendental needed |
| 5 | 4/5 | 80% | Good |
| 6 | 4/5 | 80% | Good |
| 7 | 4/5 | 80% | Good |
| 8 | 3/5 | 60% | x,t empty; normalize MULs |
| 9 | 3/5 | 60% | DOT3 again; w,t empty |
| 10 | 2/5 | 40% | ** SETGT+MAX only; 3 slots wasted |
| 11 | 3/5 | 60% | |
| 12 | 3/5 | 60% | |
| **Total** | **38/60** | **63.3%** | **22 wasted slot-cycles** |

### Key Scheduling Failures:

1. **Bundles 1-3**: The 3 ADDs in bundles 2-3 are independent of the DOT in
   bundle 1. An ideal scheduler would pack: bundle1=[DOT3 x,y,z + ADD w + ADD t]
   eliminating bundle 3 entirely and saving 2 cycles.

2. **Bundle 10**: Only 2 ops. The MULADD ops in bundle 11 read PV.w from
   bundle 10, creating a dependency. But the SETGT result (R1.z) is not read
   until bundle 12. So SETGT could move to bundle 8 or 9.

3. **t-slot underuse**: Only bundle 2 uses the transcendental slot (for RSQ).
   For a 12-bundle shader, that is 11 wasted t-cycles. In a shader with more
   transcendental math (SIN/COS for animation, EXP/LOG for fog), this would
   be better utilized.

### Estimated FPS gain if scheduling were ideal:

- Current: 38 slots used, 12 bundles
- Ideal: same 38 slots, approx 9-10 bundles (merge 1+3, merge 10 into 8/9)
- = 17-25% fewer ALU cycles for this shader
- Since GPU is 100% shader-bound, this could translate to approx 10-15% FPS
  uplift on glxgears (limited by non-ALU bottlenecks too)

---

*Analysis performed 2026-03-29 using r600-session.sh RE toolkit*

---

## APPENDIX B: R600_DEBUG Flag Discovery and Full Clause Coverage

### Problem
Initial ISA captures used `R600_DEBUG=vs,ps,cs,precompile` which only expanded
ALU clause contents. VTX (vertex fetch) and TEX (texture fetch) clauses were
shown as CF-level references (e.g., `VTX 2 @4`, `TEX 1 @20`) without expanding
the individual VFETCH/SAMPLE instructions inside them.

This caused the parser to undercount total instructions, especially for
TEX-heavy fragment shaders (blur, convolution, height-map bump mapping) where
the majority of the bytecode is TEX/VTX clause data, not ALU.

### Root Cause
Mesa r600g has separate debug flags for each clause type:
- `vs,ps,cs` -- print vertex/pixel/compute shader ALU clause disassembly
- `fs` -- "Print fetch shaders" -- expands VTX clause VFETCH instructions
- `tex` -- "Print texture info" -- expands TEX clause SAMPLE instructions

Without `fs` and `tex`, the VTX/TEX clauses are emitted only as CF-level
references showing clause address and count, not the individual instructions.

### Resolution
The correct R600_DEBUG flag set for complete ISA coverage is:
```
R600_DEBUG=vs,ps,cs,fs,tex,compute,precompile
```

This produces:
- ALU clause expansion (x/y/z/w/t slot assignments, opcodes)
- VTX clause expansion (VFETCH with register, resource ID, format)
- TEX clause expansion (SAMPLE with register, resource ID, sampler ID)
- CF instruction listing (EXPORT, CALL_FS, RET, LOOP, JUMP)

### Verification (session_full_glmark2)
With full flags, the same glmark2 benchmarks now report:
```
VFETCH instructions: 52
TEX SAMPLE instructions: 56 (including SET_GRADIENTS)
ALU clauses: 29
TEX clauses: 6
VTX clauses: 40
EXPORT: 65
Total shaders: 93
```

Compare to previous (without fs,tex):
```
VFETCH instructions: 0  (hidden)
TEX SAMPLE instructions: 0  (hidden)
ALU clauses: 29  (same)
TEX clauses: 0  (hidden as CF-level only)
VTX clauses: 0  (hidden as CF-level only)
```

### Updated r600-session.sh
The session script should be updated to use:
```sh
export R600_DEBUG="${R600_DEBUG:-vs,ps,cs,fs,tex,compute,precompile}"
```

### All Available R600_DEBUG Flags (Mesa 25.2.6):
```
vs           Print vertex shaders
ps           Print pixel shaders
gs           Print geometry shaders
cs           Print compute shaders
tcs          Print tessellation control shaders
tes          Print tessellation evaluation shaders
fs           Print fetch shaders (VTX clause expansion)
tex          Print texture info (TEX clause expansion + allocation info)
nir          Enable experimental NIR shaders
compute      Print compute info
vm           Print virtual addresses when creating resources
info         Print driver information
preoptir     Print the LLVM IR before initial optimizations
checkir      Enable additional sanity checks on shader IR
nodma        Disable asynchronous DMA
nohyperz     Disable Hyper-Z
no2d         Disable 2D tiling
notiling     Disable tiling
nocpdma      Disable CP DMA
forcedma     Use async DMA for all ops
nowc         Disable GTT write combining
check_vm     Check VM faults and dump debug info
testdma      Invoke SDMA tests and exit
```

### Impact on Analysis
The `tex` flag also prints texture allocation metadata (npix, layout, tiling,
scanout) for every texture created. This is valuable for memory bandwidth
analysis but increases log volume. For ISA-focused analysis, `fs` is essential
but `tex` can be filtered post-capture if noise is a concern.


---

## APPENDIX C: Complete Instruction Mix with Full Clause Coverage

After resolving the VTX/TEX visibility gap (Appendix B), the definitive
instruction mix for glmark2 (selected benchmarks: build, texture, shading,
bump, effect2d) with full R600_DEBUG flags is:

### Instruction Type Breakdown

| Type | Count | Pct | Description |
|------|-------|-----|-------------|
| ALU  | 34    | 38% | Compute (MUL, MULADD, DOT, RSQ, etc.) |
| VTX  | 52    | 58% | Vertex fetch (VFETCH from vertex buffers) |
| TEX  | 4     |  4% | Texture sample (SAMPLE from textures) |

### Key Insight: This workload is VTX-dominated

The selected benchmarks (build, texture, shading, bump, effect2d) are
58% vertex fetch by instruction count. This means:

1. Vertex buffer bandwidth is the primary throughput constraint for
   simple geometry benchmarks, not ALU.
2. The 68% VLIW5 ALU utilization matters less than expected because ALU
   is only 38% of total instructions.
3. TEX is only 4% -- the texture and effect2d benchmarks issue very
   few TEX SAMPLE instructions because the texture benchmark uses a
   single sample per fragment.

### Revised Bottleneck Analysis

For this benchmark selection:
```
  Bottleneck hierarchy:
  1. VTX fetch bandwidth (58% of instructions)
  2. ALU throughput (38%, 68% VLIW5 utilization = ~26% of total capacity)
  3. TEX fetch latency (4%, minimal)
```

For more fragment-heavy workloads (desktop blur, refract, shadow, jellyfish):
```
  Expected bottleneck hierarchy:
  1. TEX fetch bandwidth (convolution, shadow maps, reflections)
  2. ALU throughput (complex lighting, per-pixel math)
  3. VTX fetch (minimal, simple geometry)
```

### Fetch Shader Pattern (VTX clauses)

The 52 VFETCH instructions break down into a repeated pattern per shader:
- 5 fetch shaders x ~10 shaders = ~50 VFETCHes
- Per shader: 1 full VFETCH (R1.xyzw position) + 1 offset VFETCH (R2.xyzw)
- Fetch shader variants for different vertex formats:
  - R1.x001  (R32_UINT -- index)
  - R1.xy01  (R32G32_UINT -- 2D texcoord)
  - R1.xyz1  (R32G32B32_UINT -- 3D normal)
  - R1.xyzw  (R32G32B32A32_UINT -- full vec4)
  - R1.xyzw + R2.xyzw (interleaved pos+normal, stride 32)

Each fetch shader is a small subroutine (8-12 dw) called by CALL_FS.
The hardware executes VTX clause fetches in parallel with ALU work from
other wavefronts, so VTX latency is partially hidden. However, the sheer
number of fetches (52 total) still consumes significant memory bandwidth.

---

*Updated 2026-03-29 with full VTX/TEX clause data*
