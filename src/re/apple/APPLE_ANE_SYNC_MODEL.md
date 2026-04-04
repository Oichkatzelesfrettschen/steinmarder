# Apple Neural Engine (ANE) Synchronization Model

**Researched**: 2026-04-03 via `aeb42174e7e1f69ff` agent  
**RE sources**: `eiln/ane` kernel driver RE, `hollance/neural-engine`, PyTorch MPS source, Apple `ml-ane-transformers`  
**Cross-reference**: [`APPLE_CROSS_DOMAIN_PERF_ATLAS.md`](APPLE_CROSS_DOMAIN_PERF_ATLAS.md) — full synchronization cost table in Table 5

---

## TL;DR

**ANE atomics do not exist as a user-visible concept.** The ANE is a fixed-function bulk-tensor accelerator with no per-thread programmable execution model. The question "what is ANE `atomic_fetch_add` latency?" has no answer — the primitive does not exist at this hardware level. The relevant cost domain is **CoreML dispatch latency** (task-level, millisecond-scale), not per-element atomics (nanosecond-scale).

---

## 1. ANE Microarchitecture — Fixed-Function Bulk Tensor Engine

**What is known (from `eiln/ane`, `hollance/neural-engine`, `apple/ml-ane-transformers`):**

- The ANE is a dedicated NPU sharing the same die as the CPU and GPU cores in M-series SoCs. It is **not a programmable shader processor**.
- It processes a full inference graph compiled to a private binary format as a unit. There is no instruction-dispatch pipeline with per-thread register files, no warp/simdgroup model, and no per-element execution unit.
- No public ISA. Apple exposes no assembler, kernel-level shader compiler, or instruction set documentation. `eiln/ane` confirmed the kernel driver uses:
  - `engine_lock` mutex for serialized task submissions
  - `iommu_lock` mutex for DART IOMMU operations
  - Three DART (IOMMU) regions
  - An interrupt line for completion signaling
- The ANE accepts work via a private task-descriptor format (`ane_request`: `qid`, `nid`, `td_size`, `td_count`, `btsp_iova`, per-tile BARs). The driver submits a batch task submission pointer; hardware processes it to completion and signals via interrupt.
- **Consequence**: Because the execution model is bulk-tensor (not per-thread), atomics in the CPU/GPU sense are architecturally meaningless. There is no contention between concurrent threads over a shared counter — the ANE doesn't have threads.

**Why ANE atomics are a category error:**  
GPU atomics exist because GPUs have thousands of parallel threads competing to update shared state. The hardware needs per-element serialization at the memory level. The ANE has no parallel threads contending over shared counters. The synchronization primitive at the ANE level is the **task descriptor boundary** — one full inference graph submitted as a unit, signaling completion via interrupt.

---

## 2. CoreML Synchronization Model — What Replaces Atomics

CoreML exposes four `MLComputeUnits` placement tiers on M-series (confirmed in `neural_backend_matrix.csv`):

| ComputeUnit | Description |
|-------------|-------------|
| `CPU_ONLY` | BNNS (Accelerate/AMX paths) |
| `CPU_AND_GPU` | CPU + Metal compute |
| `CPU_AND_NE` | CPU + ANE (private framework) |
| `ALL` | Framework chooses per-layer |

When a model is split across compute units (e.g., embedding lookup on CPU, transformer blocks on ANE — as documented in `apple/ml-ane-transformers`), CoreML handles the inter-domain synchronization entirely inside the framework. The user has **no access to the synchronization primitive itself** — no fence, no event, no semaphore at the application layer. Synchronization cost is folded into per-layer dispatch overhead.

**Runtime flow for ANE execution:**
1. CoreML serializes inputs
2. Dispatches subgraph to ANE via private IOKit call
3. Blocks waiting for ANE interrupt (kernel-mediated)
4. Passes outputs to next subgraph (CPU/GPU)

This is coarse-grained, task-level synchronization — not fine-grained per-element.

---

## 3. Metal Synchronization Primitives — CPU↔GPU Cross-Domain Costs

| Primitive | Scope | Mechanism | Latency class |
|-----------|-------|-----------|---------------|
| `MTLFence` | Within a single command queue, between encoders | GPU-side only; no CPU visibility | Sub-µs GPU pipeline ordering |
| `MTLEvent` | GPU-to-GPU, same device | GPU command buffer level | GPU pipeline latency only |
| `MTLSharedEvent` | CPU↔GPU, cross-process, multi-device | `signaledValue` + `MTLSharedEventListener` callback | OS scheduling latency (~5–50 µs) |

**`MTLFence`**: Intra-queue, between two encoders. Purely GPU-side ordering signal — no CPU involvement. Cost is a GPU pipeline barrier (cache flush/invalidate + execution dependency). Expected similar to our measured `threadgroup_barrier` (25.44 ns) in best case.

**`MTLSharedEvent`**: The cross-domain synchronization primitive. Implemented as:
- GPU executes to signal point
- `MTLSharedEventListener` callback dispatches on Metal's internal queue
- Mutex acquisition + `std::condition_variable::notify_one()` (confirmed from PyTorch MPS `MPSEvent.mm`)
- CPU thread wakes from OS scheduler

Round-trip is **kernel-mediated** (condition variable + OS scheduler), not a spinning poll. Latency dominated by OS scheduling (~5–50 µs).

**MLX** uses `MTLSharedEvent` with lazy evaluation barriers. MLX `mx.eval()` flushes the lazy graph and blocks via `waitUntilSignaledValue()`.

---

## 4. Measured and Estimated Latencies

### Directly measured in our probes

| Domain | Primitive | Latency (ns) | Notes |
|--------|-----------|-------------|-------|
| CPU | `atomic_fetch_add` relaxed (u64) | **1.912** | Serial dep-chain; ~2 cycles |
| GPU threadgroup | `@air.atomic.local.add.u.i32` | **89,600** | Serial dep-chain; 47× CPU |
| GPU global | `@air.atomic.global.add.u.i32` | **143,800** | Serial dep-chain; 75× CPU |
| GPU | `threadgroup_barrier` | **25,440** | ~33 GPU cycles |
| GPU | `simdgroup_sum` (32-lane) | **28,310** | ~36 GPU cycles |
| GPU L1 | global load (warm, per-thread) | **10,070** | ~13 GPU cycles |
| GPU LDS | threadgroup load (volatile) | **43,660** | ~28 GPU cycles (21.8/access) |
| CPU→GPU (warm) | `commandBuffer waitUntilCompleted` overhead | **~130–215** | 13.9–22.5× GPU-actual in our single-dispatch harness |

### Estimated from external sources

| Domain | Primitive | Latency estimate | Basis |
|--------|-----------|-----------------|-------|
| CPU↔GPU | `MTLSharedEvent` CPU wakeup | **5,000–50,000 ns** (5–50 µs) | OS scheduling latency (general knowledge; no direct measurement) |
| CPU→GPU | MPS dispatch overhead (small matmul) | **500,000–3,000,000 ns** (0.5–3 ms) | Our `neural_lane_results.md` |
| CPU→ANE | CoreML model dispatch (first call) | **~1,000,000–10,000,000 ns** (1–10 ms) | Estimated from IOMMU setup + task-descriptor compilation + interrupt; no direct measurement |
| CPU→ANE | CoreML model prediction (steady-state) | **~1,000,000–3,000,000 ns** (1–3 ms/inference) | Framework-level; includes execution, not just dispatch |

### Not known / not publicly documented

| Primitive | Status | Notes |
|-----------|--------|-------|
| `MTLFence` latency (ns) | **NOT MEASURED** | No public measurement found |
| `MTLEvent` latency (ns) | **NOT MEASURED** | No public measurement found |
| `MTLSharedEvent` wakeup (ns, exact) | **NOT MEASURED** | Estimated 5–50 µs; needs direct probe |
| CoreML ANE dispatch (µs, exact) | **NOT MEASURED** | Private framework; no public data |
| ANE-internal pipeline synchronization | **UNDOCUMENTED** | Hidden behind private task-descriptor format |
| CoreML CPU→ANE inter-domain sync primitive | **UNKNOWN** | May be `MTLSharedEvent` or private IPC |
| AGX ISA CAS instruction | **UNCONFIRMED** | `@air.atomic.*.cas.*` never seen in our AIR corpus |

---

## 5. The Complete Cross-Domain Synchronization Picture

```
Domain              Primitive/operation                Latency (ns)      Ratio vs CPU atomic
────────────────────────────────────────────────────────────────────────────────────────────
CPU                 int ADD (u32/u64)                  0.319             0.17×
CPU                 f32/f64 fadd (dep chain)           0.945             0.49×
CPU                 atomic_fetch_add (relaxed, u64)    1.912             1.00× (BASELINE)
────────────────────────────────────────────────────────────────────────────────────────────
GPU                 threadgroup_barrier                25,440            13,300×
GPU                 simdgroup_sum (32-lane)            28,310            14,800×
GPU threadgroup     atomic_fetch_add                   89,600            46,800×
GPU global          atomic_fetch_add                   143,800           75,200×
────────────────────────────────────────────────────────────────────────────────────────────
CPU→GPU (warm)      commandBuffer overhead component   ~130–215          68–112×
CPU↔GPU             MTLSharedEvent CPU wakeup          ~5,000–50,000     2,600–26,100×
CPU→GPU             MPS dispatch (small matmul)        500,000–3,000,000 260,000–1,570,000×
────────────────────────────────────────────────────────────────────────────────────────────
CPU→ANE             CoreML model dispatch (estimated)  ~1,000,000–10,000,000  520,000–5,200,000×
ANE                 atomic_fetch_add (per element)     DOES NOT EXIST    N/A
ANE                 intra-engine synchronization       UNDOCUMENTED      N/A
```

**Scale observation**: The cost hierarchy spans 7 orders of magnitude — from CPU int ADD (0.3 ns) to CoreML ANE dispatch (~1–10 ms). GPU atomics sit at ~90–145 ns (2–3 orders of magnitude above CPU atomics, 4 orders of magnitude below CoreML dispatch).

---

## 6. AGX ISA Atomics — What We Know from AIR

From our `metal_air_opcode_inventory.md` (results/blessed/2026-03-30_tranche1_r7_cde_keepalive):
- Metal does NOT emit LLVM `atomicrmw` instructions
- Uses AGX-specific AIR intrinsics: `@air.atomic.local.add.u.i32`, `@air.atomic.global.add.u.i32`
- `@air.wg.barrier` (threadgroup_barrier) confirmed at AIR level
- **`@air.atomic.*.cas.*` has never appeared in our AIR corpus** — CAS existence unconfirmed at ISA level

From `dougallj/applegpu`:
- `wait` instruction stalls a thread until pending async device loads complete (underlying primitive for load latency measurements)
- No documented per-element compare-and-swap at the ISA level
- Hardware scheduling (not NOP-padded) means dep-chain stalls are real pipeline stalls

---

## 7. Practical Implications

1. **Do not use GPU atomics in serial dep chains.** 89.6–143.8 ns vs 1.9 ns on CPU = 47–75× penalty. Even a single atomic add in a dep chain moves the bottleneck to memory serialization.

2. **ANE is throughput-optimized, not latency-optimized.** CoreML dispatch at ~1–10 ms means the ANE only wins when running large inference graphs where the compute time >> dispatch overhead. For tiny operations, CPU BLAS/NEON or Metal are faster.

3. **`MTLSharedEvent` is expensive (OS-scale).** Do not use for fine-grained synchronization. Use it once per batch (command buffer), not per-operation.

4. **CPU→GPU is expensive at small scale.** MPS dispatch overhead is 0.5–3 ms for matmuls below ~1024×1024. CPU BLAS+NEON dominates for small compute — crossover is around 1024×1024.

5. **ANE steady-state throughput is ~10× faster than CPU/GPU for large transformers.** The 1–10 ms dispatch overhead amortizes when inference takes 10–100 ms. ANE wins on energy efficiency and throughput at scale, not latency at small scale.

---

*Last updated: 2026-04-03. Source: agent aeb42174e7e1f69ff neural atomics research.*  
*See also: [`APPLE_CROSS_DOMAIN_PERF_ATLAS.md`](APPLE_CROSS_DOMAIN_PERF_ATLAS.md) Table 5 for the condensed cross-domain sync table.*
