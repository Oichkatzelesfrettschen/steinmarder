# `open_gororoba` Transfer Targets for `steinmarder`

This note scopes what is actually worth importing conceptually from
`~/Github/open_gororoba` into the Ryzen/NeRF lane here.

## High-value transfer ideas

### 1. Aligned workspace and prepacked data

Most directly useful:
- `TurboQuantWorkspace + AlignedWorkspace (256-byte align)`
- explicit aligned scratch and reusable temp arrays
- runtime SIMD dispatch with a prepared fast path

Why it fits here:
- the first real NeRF MLP win in `steinmarder` came from aligned/prepacked
  single-ray weights, not from a naive extra-FMA rewrite
- this mirrors TurboQuant's bias toward preparing data once so the hot loop does
  less pointer chasing and fewer awkward loads

Concrete `steinmarder` uses:
- prepacked single-ray MLP weights for `27 -> 64 -> 64 -> 4`
- aligned hash-grid working buffers for future lookup experiments
- reusable per-thread scratch for coherent-ray bridge prototypes

### 2. Dispatch and autotune discipline

Most directly useful:
- CPUID-based dispatch
- small number of explicit backends/variants
- measurement-driven method selection

Why it fits here:
- `steinmarder` already has AVX2 detection and runtime toggles
- the new Ryzen lane should choose among `generic`, `prepacked`, and future
  coherent-ray bridge variants using measured evidence rather than intuition

Concrete `steinmarder` uses:
- formalize `SM_NERF_MLP_VARIANT` beyond a temporary env knob
- add an autotune-smoke step that ranks MLP variants on startup or offline
- keep batch/single/prepacked/bridge variants explicit and reproducible

### 3. Dimension-specific specialization discipline

Most directly useful:
- the `16D -> 32D -> 64D` ladder in `open_gororoba` is a strong reminder that
  "wider" or "more generic" is not automatically better
- the checked-in notes consistently point to `32D` as the sweet spot while
  `64D` dilutes signal

Why it fits here:
- the failed generic rewrite and the successful prepacked fixed-shape path show
  the same lesson in a different domain
- for this NeRF MLP, the useful specialization is the exact shape
  `27 -> 64 -> 64 -> 4`, not a broad "make the code more vectorized" rewrite

Concrete `steinmarder` uses:
- keep exact-shape kernels where the model shape is stable
- resist over-generalizing early optimization work
- benchmark each shape/layout frontier explicitly

### 4. Codebook/sign-pack style thinking

Most directly useful:
- `sign_pack.rs` shows compact, structure-aware precomputation can pay off
- `simd_codebook.rs` shows that changing data representation can be more
  important than changing the arithmetic syntax

Why it fits here:
- the NeRF MLP and hash-grid work already points toward layout and representation
  being more important than raw FMA count on this Ryzen box

Concrete `steinmarder` uses:
- future compact MLP weight storage experiments
- possible output-layer or feature-block pretransposition
- careful bit/pack experiments for occupancy or branch masks

## Lower-value transfers

These are interesting but not immediate priorities for `steinmarder`:
- Cayley-Dickson algebra machinery itself
- 16D/32D/64D embeddings as a literal algorithmic transplant
- exceptional algebra rotations as a direct renderer optimization

They matter as methodological inspiration, not as direct code to port.

## Immediate recommendation

The next transfer step should be modest and local:
1. keep the new prepacked single-ray MLP path,
2. formalize a small variant registry for NeRF CPU kernels,
3. add reusable aligned scratch for a coherent-ray bridge prototype,
4. treat the `16D/32D/64D` lesson as a warning against "bigger/more generic"
   optimization passes that are not anchored to the exact hot shape.
