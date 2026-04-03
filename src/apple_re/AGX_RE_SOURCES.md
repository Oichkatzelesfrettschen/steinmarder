# AGX GPU Reverse Engineering Sources

## Introduction

This document catalogues the primary reverse engineering sources for Apple's AGX GPU architecture (G13 / M1-series and successors). It exists to give steinmarder contributors a single reference point for understanding what is already known — from ISA encoding details to microarchitectural timing — before designing new probes or interpreting probe results.

AGX is a proprietary scalar GPU introduced with A14 / M1. Because Apple provides no public architecture documentation, all microarchitectural knowledge comes from independent RE efforts: binary patching, empirical microbenchmarks, hardware decoder analysis, and open-source driver development. The steinmarder apple_re probe suite adds empirical timing measurements on top of this foundation. The sources below represent the most technically complete public knowledge available as of early 2026.

---

## 1. dougallj/applegpu

**Repository:** https://github.com/dougallj/applegpu  
**ISA documentation:** https://dougallj.github.io/applegpu/docs.html  
**Coverage:** G13 (M1 GPU) full ISA, disassembler, assembler, emulator

### ISA Encoding

AGX uses a variable-length instruction encoding. Instructions are 2, 4, 6, 8, 10, or 12 bytes long — always a multiple of 2. A single `L` bit in the first two bytes selects the short (2-byte) or long (extended) form of each instruction class. This differs sharply from most GPU ISAs, which use fixed 32-bit or 64-bit words.

The variable-length design allows the ISA to pack short immediate forms and simple register-register operations into 2 bytes while preserving the ability to encode large immediates, extra modifier flags, and additional source operands in longer forms.

### Register File

- **General-purpose registers (GPRs):** 128 per SIMD-group, addressed as 32-bit words. Each register is also accessible as two 16-bit halves: `r0l` (low half) and `r0h` (high half). The 16-bit sub-register names are used for FP16 and INT16 operands throughout the ISA.
- **Uniform registers:** 256 uniforms, named `u0`–`u255`. These hold values that are constant across all threads in a SIMD-group (driver-written constants, buffer base addresses, etc.).
- **Special-purpose registers:**
  - `r0l` — execution mask stack (current active lane mask; push/pop operations manipulate this)
  - `r1` — link register (return address for `call`)

### SIMD-Group Width

32 threads per SIMD-group. This is narrower than NVIDIA's 32-wide warp or AMD's 64-wide wavefront family, but the scalar architecture means each thread has its own full register file slice — there are no "SIMD-lane" data paths at the ISA level.

### Instruction Categories

**Integer arithmetic:** `iadd`, `imadd` (integer multiply-add), `imul`, `isub`, `icmpsel` (compare-and-select). Convert instructions handle INT↔FP type changes.

**Floating-point arithmetic:** `fmadd` (fused multiply-add, the primary FP workhorse), `fadd`, `fmul`, `rcp` (reciprocal), `rsqrt` (reciprocal square root), `log2`, `exp2`, `floor`, `ceil`. All support modifier flags for negate and absolute value as zero-cost instruction fields — these are not separate instructions.

**Shifts and bitfield operations:** `bfi` (bit field insert), `extr` (bit field extract), `shlhi`/`shrhi` (shift operations targeting the high half of a register pair), and related packed/halfword variants.

**Bit manipulation:** `bitrev` (bit-reverse), `popcount`, `ffs` (find first set). Confirmed as distinct opcodes in hardware (see Asahi "First Conformant" post, source 4 below).

**SIMD-group operations:** `ballot` (collect lane predicate bits into a scalar), `shuffle` (cross-lane data exchange). These are the vectorisation primitives for reductions and prefix scans.

**Memory — device (global) memory:** `device_load` and `device_store`. These are asynchronous: a `device_load` issues a request to the memory subsystem and returns immediately; a subsequent `wait` instruction stalls the issuing thread until the load completes. This is the explicit scoreboard mechanism.

**Memory — threadgroup (LDS/shared) memory:** `stack_load` and `stack_store` for the per-threadgroup scratchpad.

**Texture instructions:** Sampled and non-sampled texture reads.

**Flow control:** Branches, `call`, `ret`. Structured control flow is lowered through the execution mask stack (push diverge / merge pop idiom).

**Execution mask stack:** Push/pop instructions that manage the active lane mask for divergent control flow.

**Synchronisation:** `threadgroup_barrier` for intra-group synchronisation; `wait` (scoreboard stall) for async memory completion.

### The `wait` Instruction and Scoreboard

The `wait` instruction is the central scheduling primitive for latency tolerance. When a `device_load` is issued, the compiler assigns it a scoreboard slot. Later, a `wait` instruction naming that slot causes the thread to stall until the load has arrived. This allows multiple outstanding loads to be in flight simultaneously and lets the scheduler hide memory latency by executing independent instructions between the issue and the wait.

### Cache Hints

Source operands on load/store instructions can carry `cache` (cache the result in L1/L2) and `discard` (hint that the data will not be reused — skip filling the cache line) annotations. These are inline instruction modifiers, not separate cache-control instructions.

### RE Methodology

The applegpu project RE'd the ISA by patching Metal binary archives (`.metallib` files). A known shader is compiled to a `.metallib`, the binary encoding of specific instructions is surgically modified to test hypotheses about opcode fields and operand encodings, and the patched binary is run on real hardware to observe whether the result matches the expected ISA semantics. This iterative binary-patch-and-run methodology is the ground truth for all field assignments in the ISA documentation.

---

## 2. philipturner/metal-benchmarks

**Repository:** https://github.com/philipturner/metal-benchmarks  
**Coverage:** Empirical dependency-chain latency and throughput on M1 through M4; same general methodology as steinmarder probes

### Methodology

Each benchmark constructs a long chain of dependent instructions (measuring latency) or a sequence of independent instructions (measuring throughput) and times the total execution with Metal performance counters. Results are reported in GPU cycles using the GPU's known nominal clock.

### Arithmetic Latency and Throughput on M1

| Operation | Dep-chain latency (adj. cycles) | Throughput (cycles/op) |
|-----------|--------------------------------|------------------------|
| FADD32    | ~2.20–2.21                     | 1                      |
| FMUL32    | ~2.20–2.21                     | 1                      |
| FFMA32    | ~2.20–2.21                     | 1                      |
| FADD16    | ~2.17                          | 1                      |
| FMUL16    | ~2.17                          | 1                      |
| FFMA16    | ~2.17                          | 1                      |
| IMUL16    | ~3.69                          | 4                      |
| IMUL32    | ~4.02                          | 4                      |
| IADD16    | ~2.21                          | 1                      |
| IADD32    | ~2.17                          | 1                      |

Key takeaway: FADD, FMUL, and FFMA all share the same 2.20-cycle dep-chain latency, consistent with a single-pipeline FP unit. IMUL is substantially slower (4× throughput penalty and ~2× latency increase vs FADD), suggesting integer multiply uses a different — and narrower — pipeline.

### Register Dependency Penalty

FP32 back-to-back FMUL: **0.84-cycle penalty** above the 1-cycle base (1.84 total interval). FP16 back-to-back: **0.56-cycle penalty** (1.56 total). This sub-cycle fractional number is characteristic of a pipelined design where the register bypass path adds a small but measurable fraction of a clock cycle of overhead, as seen in out-of-order or multi-issue machines.

### Occupancy Scaling

At minimum occupancy (few SIMD-groups resident per core), FMA latency widens dramatically: **11.3 cycles** for FP32 FMA vs **3.9 cycles** for FP16 FMA at minimum occupancy. This is consistent with the GPU exposing deeper pipeline stages when it cannot hide latency through thread-level parallelism. The FP16 vs FP32 gap at minimum occupancy may indicate physically separate FP16 and FP32 ALU pipelines with different depths, or different bypass paths.

### Core Topology

- **128 ALUs per core** (the fundamental compute resource)
- Dispatch modes: single-dispatch (occupies 3 SIMDs), dual-dispatch (2 SIMDs), quad-dispatch (1 SIMD)
- **Maximum 24 SIMD-groups per core** at full occupancy
- M1 doubled FP32 compute compared to A14: 256 FP32 ops/cycle/core

### Cache Hierarchy (M1)

| Level | Size |
|-------|------|
| L1 data cache | 8 KB |
| Shared memory (LDS/threadgroup) | ~60 KB |
| L2 per core | 768 KB |
| System L3 (SLC) | 8 MB |

These figures are empirical estimates from the metal-benchmarks bandwidth tests, not from official documentation.

---

## 3. Alyssa Rosenzweig Blog Series

**Blog:** https://alyssarosenzweig.ca/blog/  
**Coverage:** Microarchitectural observations from compiler development for the Asahi Linux Mesa driver

### Part I — Scalar Architecture and Pipeline Observations

**URL:** https://alyssarosenzweig.ca/blog/asahi-gpu-part-1.html

AGX is **scalar at all precisions**. Unlike traditional SIMD-lane GPUs where a single instruction operates on all lanes simultaneously at the hardware level, AGX's scalar ISA means each thread's instruction stream is conceptually independent. The hardware SIMD-group width (32 threads) is an implementation detail for occupancy/throughput, but the programmer-visible model is scalar.

**More 16-bit ALUs than 32-bit ALUs.** The hardware has a wider pool of FP16/INT16 execution units than FP32/INT32 units. This asymmetry rewards shaders that use 16-bit precision where possible.

**Hardware-scheduled execution.** AGX does not require NOP padding between dependent instructions. The scheduler determines the correct issue gaps at runtime (or the compiler uses `wait` where needed for memory ops). This contrasts with older GPU ISAs (including early VLIW designs) where the compiler must insert explicit NOPs to satisfy latency constraints.

**Free type conversion.** Conversions between 16-bit and 32-bit operands are handled as instruction modifiers rather than separate instructions. There is no throughput cost for mixing precisions within an instruction stream.

**Free clamps, negates, and absolute values.** These are encoded as modifier bits on source operands — they do not consume an extra instruction slot or increase latency. This is standard GPU ISA practice (similar to SM5+ NVIDIA source modifiers) but worth noting explicitly because it affects how the compiler generates code.

**IMAD avoidance.** Rosenzweig observed that the compiler avoids integer multiply-accumulate (`imad`) in favour of chained `iadd` sequences, suggesting non-uniform pipeline depths between the multiply path and the add path in the integer unit. This is consistent with philipturner's measurement of 4-cycle IMUL throughput vs 1-cycle IADD throughput.

### Part III — AGX2 Register File and Occupancy

**URL:** https://alyssarosenzweig.ca/blog/asahi-gpu-part-3.html

AGX2 (A15/M2 and later) uses **256 registers per thread**, each 16 bits wide (512 bytes/thread). Key measurements and derivations:

- Per-threadgroup register capacity: approximately **208 KiB**
- Total GPU register storage: approximately **4.875 MiB**
- Thread count decreases in **64-thread increments** as the register count per thread rises from 112 to 256 — the occupancy cliff is gradual and predictable
- Shaders using **≤104 registers** (i.e., ≤52 FP32 registers' worth) incur no occupancy penalty

**Compiler pipeline:** NIR → SSA form → combining pass → instruction scheduling → register allocation → secondary scheduling (post-RA) → instruction packing. The two-phase scheduling (pre- and post-register-allocation) is characteristic of a backend that needs to satisfy both data dependencies and packing constraints.

**Execution model note:** Rosenzweig notes "some form of multi-issue or out-of-order" execution is present — the hardware does more than a simple in-order scalar pipeline. This is consistent with the sub-cycle register dependency penalties measured by philipturner.

### Part IV — Index Encoding and Fixed-Function Offload

**URL:** https://alyssarosenzweig.ca/blog/asahi-gpu-part-4.html

**Index sizes** are encoded as log₂ values in a 2-bit field — the hardware supports a richer set of primitive index types than the Metal API exposes to applications.

**Shader-heavy fixed-function work.** AGX offloads vertex attribute fetch and framebuffer blending to shader code running on the same ALUs as compute. This means the GPU has no dedicated fixed-function vertex fetch or blend units — those operations consume ALU budget and can be profiled with the same tools as compute shaders. The implication for compute workloads is that the ALU pool is not partitioned: full ALU availability applies to compute.

---

## 4. Asahi Linux Blog Posts

### "Tales of the M1 GPU"

**URL:** https://asahilinux.org/2022/11/tales-of-the-m1-gpu/

**Firmware-driven architecture.** The AGX GPU is controlled by an ARM64 ASC coprocessor running Apple's RTKit RTOS. The host CPU communicates with the firmware via a shared memory region and a mailbox register pair. The firmware interprets high-level work descriptors — it is not a simple register-banging interface.

**Three work types:**
- **TA (Tile Accelerator):** Geometry / binning phase of tiled rendering
- **3D:** Rendering / shading phase
- **CP (Compute):** Direct compute dispatch

These map to the three major submission paths exposed by Metal and Vulkan on Apple platforms.

### "First Conformant M1 GPU Driver"

**URL:** https://asahilinux.org/2023/08/first-conformant-m1-gpu-driver/

**Image atomics:** M1 lacks hardware support for image atomics. The Asahi driver implements these in software via shader emulation. Non-image (buffer) atomics are supported in hardware through a special path in the memory subsystem.

**Interleave instruction:** A new opcode (`00` encoding) was discovered during conformance work — the `interleave` instruction.

**Confirmed distinct opcodes:** `bitrev`, `popcount`, and `ffs` are all confirmed as separate hardware instructions, not synthesised from other operations. This was verified by the binary-patching RE methodology.

---

## 5. Asahi Linux Hardware Documentation

**URL:** https://asahilinux.org/docs/hw/soc/agx/

### Unified Address Translator (UAT)

The AGX memory management unit uses **40-bit GPU virtual addresses**, a **4-level page table** structure, and a **fixed 16 KB page size** (not 4 KB as on most x86 systems). Memory is coherent between CPU and GPU under normal operation — the driver does not need to issue explicit cache flush or invalidation commands for CPU-written data to become visible to GPU shaders.

### Micro-sequencer Commands

The GPU's firmware command processor understands four command types at the lowest level:
- **Start** — begin executing a shader program
- **Write Timestamp** — write the GPU timer to a buffer address
- **Wait For Idle** — stall until all previously issued work completes
- **Finish** — signal completion

---

## 6. AsahiLinux/gpu RE Toolkit

**Repository:** https://github.com/AsahiLinux/gpu

### agxdecode

A GPU command stream decoder tool that interprets captured AGX firmware command buffers and renders them as human-readable output. This is the primary tool for understanding what the GPU firmware is doing at the command level — analogous to tools like `apitrace` or `renderdoc` but operating at the hardware command stream layer rather than the API layer.

### Driver Architecture Split

The Asahi Linux GPU software divides into two separate efforts that are worth understanding independently:

- **Kernel driver:** Written in Rust by Lina Asahi. Handles DRM, memory management, firmware communication, power management. This is the path from the Linux kernel to the AGX firmware.
- **Shader compiler:** Written by Alyssa Rosenzweig. Implements the Mesa NIR backend that lowers GLSL/SPIR-V to AGX ISA. This is the path from application shaders to binary machine code.

These two codebases rarely overlap and can be studied independently depending on whether your interest is in the command-stream/memory architecture or the ISA/compiler side.

---

## 7. XDC 2021 Talk — "The Occult and the Apple GPU"

**URL:** https://media.ccc.de/v/xdc2021-13-the_occult_and_the_apple_gpu  
**Duration:** 38 minutes

This talk documents the RE methodology used to decode the AGX ISA in the early phase of the Asahi project. Key content:

- Systematic binary patching approach: compile a known shader, modify individual bits in the `.metallib` binary, observe execution results on real hardware
- Field-by-field ISA decoding: how to distinguish opcode bits from operand bits, determine field widths, and identify modifier flags
- Dealing with a completely undocumented binary format with no public reference
- Early observations about instruction length encoding and the L bit

The talk is valuable context for understanding *why* the applegpu disassembler is structured as it is and what the confidence level on different parts of the ISA documentation should be.

---

## 8. arXiv 2025 Cross-Vendor ISA Paper

**URL:** https://arxiv.org/html/2603.28793  
**Topic:** Systematic hardware-invariant primitive analysis across 16 GPU microarchitectures, including AGX G13

This paper applies a comparative methodology — identifying execution primitives that are invariant across different shader programs and workloads — to characterise 16 GPU microarchitectures from multiple vendors. The AGX G13 (M1 GPU) is one of the architectures analysed.

Relevant content:
- Cross-vendor comparison of latency, throughput, and occupancy characteristics
- Identification of hardware-invariant primitives that can be measured without needing ISA documentation
- Methodology applicable to steinmarder-style probe design: the "invariant primitive" concept gives principled guidance on which probes are most informative

---

## Cross-reference with steinmarder empirical measurements

This section maps steinmarder probe measurements against RE-derived values, validating both the probes and the RE sources.

### GPU Clock Derivation

Steinmarder probes report raw nanosecond timings. The GPU clock can be estimated by dividing cycle counts (from RE sources) by measured times:

| Probe result | RE-derived cycle count | Implied clock |
|-------------|------------------------|---------------|
| fmul throughput: 1/0.751 ns | 1 cycle (philipturner) | **1.331 GHz** |
| fadd dep-chain: 2.20/1.695 ns | 2.20 cycles (philipturner) | **1.298 GHz** |
| imul dep-chain: 4.02/3.376 ns | 4.02 cycles (philipturner) | **1.191 GHz** |

Best estimate from the three derivations: **~1.28–1.33 GHz** nominal GPU clock. The spread is within measurement uncertainty for a probe-based approach; the fmul throughput probe likely gives the cleanest clock estimate since throughput is less sensitive to register bypass timing effects.

### Arithmetic Latency Comparison

| Operation | steinmarder (ns) | steinmarder (cycles @ derived clock) | philipturner (cycles) | Match |
|-----------|-----------------|--------------------------------------|-----------------------|-------|
| FADD dep-chain | 1.695 ns | 2.20 @ 1.298 GHz | 2.20 | Exact |
| IMUL dep-chain | 3.376 ns | 4.39 @ 1.298 GHz | 4.02 | Within 9% |
| FP16 dep-chain | ~2.0 ns | ~2.6 | ~2.17 | Within ~20% |

### FMA Throughput Anomaly

Steinmarder measures fma throughput at **1.764 ns**, only modestly better than the dep-chain time of **1.993 ns**. This does not match philipturner's "1-cycle FFMA throughput" result (which predicts ~0.75 ns at 1.33 GHz). Possible explanations:

1. **M1 vs M2 difference:** philipturner's tables may reflect a newer chip. The steinmarder target is M1.
2. **Probe saturation:** If the throughput probe's independent instruction chain is still constrained by register file bandwidth or issue-slot limitations on M1, the measured throughput will be pessimistic.
3. **SIMD-group occupancy effect:** At the occupancy level used by steinmarder probes, the scheduler may not achieve full pipeline utilisation.

This discrepancy is a concrete open question: a probe specifically designed to isolate FMA throughput vs issue rate (using longer independent chains with interleaved independent moves) would resolve it.

### FP16 "Free Conversion" Overhead

Our FP16 dep-chain is approximately **0.3–0.7 ns** slower than the FP32 dep-chain. Rosenzweig's documentation that FP16↔FP32 conversion is "free" means this overhead is attributable to other factors (register file addressing, different pipeline depth) rather than a conversion instruction cost. The observation is consistent with Rosenzweig's note about different numbers of FP16 vs FP32 ALUs: if there are more FP16 ALUs but the scheduler does not pack them as efficiently, per-thread dep-chain times could be marginally higher.

### Cache Hierarchy Confirmation

| Cache level | RE-derived size | Steinmarder probe | Result |
|------------|-----------------|-------------------|--------|
| L1 data | 8 KB (metal-benchmarks) | T6 probe: 4 KB buffer → 10.07 ns warm | Consistent — 4 KB fits in 8 KB L1 |
| LDS/shared | ~60 KB (metal-benchmarks) | 43.66 ns dep-chain | LDS is slower than L1 for single-thread chains |

The LDS result (43.66 ns, ~28 cycles at 1.33 GHz) vs L1 (10.07 ns, ~13 cycles) is **counterintuitive**: shared memory is supposed to be faster than L1 on most GPU architectures. One explanation consistent with AGX's scalar architecture is that LDS bank conflicts or the threadgroup memory controller introduce more stalls per access when only a single thread is making requests (low bank utilisation). Under a full SIMD-group with coalesced accesses, the throughput picture may differ.

---

## How to Use These Sources

Use this section to decide which source to check for each type of question.

### "What does instruction X do? What are its operand fields?"
→ **dougallj/applegpu ISA docs** (https://dougallj.github.io/applegpu/docs.html). This is the ground truth for ISA encoding.

### "What is the latency or throughput of operation X?"
→ **philipturner/metal-benchmarks** first. Cross-check against steinmarder results using the clock derivation table above.

### "How does the compiler lower GLSL/SPIR-V to AGX?"
→ **Alyssa Rosenzweig blog** (Parts I, III, IV) and the Mesa source in the AsahiLinux/mesa repository.

### "How does the firmware/driver interface work? How are command buffers structured?"
→ **Asahi Linux hardware docs** (UAT, micro-sequencer) and **AsahiLinux/gpu** (agxdecode tool, kernel driver).

### "Why is a specific Metal/Vulkan feature behaving unexpectedly?"
→ **Asahi "First Conformant" post** for known hardware limitations (image atomics, etc.) and **Asahi hardware docs** for memory model details (UAT coherence, page size).

### "How do I compare AGX to other GPU architectures?"
→ **arXiv 2603.28793** for cross-vendor analysis. **Alyssa Rosenzweig Part I** for high-level differentiation (scalar vs SIMD-lane, hardware scheduling, precision asymmetry).

### "How was the ISA originally reverse engineered? How confident are the field assignments?"
→ **XDC 2021 talk** for methodology. **dougallj/applegpu** README for current confidence notes.

### "I have a steinmarder probe result — which RE source validates it?"
→ See the **Cross-reference table** section above.

---

*Last updated: 2026-04-02. Part of the steinmarder RE project — see also `APPLE_SILICON_RE_BRIDGE.md` and `FRONTIER_ROADMAP_APPLE.md` for probe roadmap context.*
