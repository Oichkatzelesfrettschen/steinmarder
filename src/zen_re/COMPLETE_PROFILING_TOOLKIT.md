# Complete CPU/GPU Profiling Toolkit

This note expands the Ryzen reverse-engineering lane from "optimize NeRF" into
a full-stack toolkit that mirrors the CUDA/SASS workflow as closely as a CPU
environment allows.

## 1. Translation scope in this repo

NeRF is not the only CUDA/SASS-inspired structure we should translate to CPU.

### Primary translation targets

1. `src/nerf/nerf_simd.c`
   - hash-grid lookup
   - occupancy skipping
   - single-ray vs batch MLP structure
   - prefetch, alignment, and layout experiments
2. `src/nerf/nerf_batch.c` and `src/nerf/nerf_scheduler.c`
   - coherence grouping
   - queue shape
   - cache-sensitive batch ordering
3. `src/render/bvh.c`
   - branch-heavy traversal
   - pointer chasing
   - load-use hazards
4. `src/render/render.c`
   - material dispatch
   - invariant hoisting
   - coherent vs incoherent hot paths
5. `src/render/sm_mt.c`
   - thread wakeups
   - tile migration
   - cross-core locality

### CUDA/SASS-to-CPU pattern transfers

From `src/sass_re/` and `src/cuda_lbm/`, the important transferable ideas are:
- SoA vs AoS layout pressure
- precomputed invariants like `inv_tau`
- occupancy/latency-hiding translated into cache/ILP/threading decisions
- coarsening and batching translated into CPU-side cache-aware work shaping
- register-pressure thinking translated into front-end pressure, spill pressure,
  and vector register residency

This means the CPU lane should eventually cover:
- NeRF
- CPU renderer traversal and shading
- CPU scheduling
- CPU-side versions of LBM layout/coarsening ideas where they make sense

## 2. ISA coverage strategy

We should measure widely, but optimize selectively.

### Tier A: production hot-path priority

- scalar x86-64 integer and FP
- SSE2 baseline ABI floor
- SSE3 / SSSE3 / SSE4.1 / SSE4.2 where they provide specific wins
- AVX / AVX2
- FMA3
- BMI1 / BMI2
- POPCNT / LZCNT / TZCNT
- software prefetch variants

These are the main lanes for modern Ryzen optimization in this repo.

### Tier B: targeted archaeology / compatibility lane

- x87
- MMX
- 3DNow-prefetch-related behavior
- SSE4a

These matter for reverse-engineering completeness, compiler/codegen knowledge,
and ABI edge cases, but they are not the first production optimization targets
for the current renderer/NeRF stack.

### Tier C: analysis support

- `objdump`
- `llvm-objdump`
- `readelf`
- `nm`
- `llvm-mca`
- `cpuid`
- `lscpu`

These are the CPU-side equivalents of cubin/SASS inspection and static issue
modeling. They are required to make the CPU lane resemble the GPU SASS lane.

## 3. API / ABI capability layers

The toolkit should be designed as non-overlapping layers.

### A. Static code shape and ISA surface

- `objdump`, `llvm-objdump`
  - emitted instructions
  - addressing modes
  - alignment-sensitive codegen
- `readelf`, `nm`
  - symbol layout
  - relocation / ABI shape
  - visibility and section placement
- `llvm-mca`
  - static throughput and dependency modeling
  - port pressure estimates

### B. PMU and hardware-counter profiling

- `perf`
  - cycles, instructions, branches, cache events
  - callgraph / annotate / record / stat
- `hotspot`
  - GUI for `perf` captures
- `AMDuProfCLI` (`/opt/amduprof/bin/AMDuProfCLI`, version `5.2.606.0`)
  - AMD-specific PMU and IBS-oriented views
  - Zen-centric analysis that `perf` alone does not always surface cleanly

### C. Timeline and kernel/user tracing

- `trace-cmd`
  - ftrace-level system tracing
- `bpftrace`
  - dynamic tracing and low-overhead live probes
- `strace`
  - syscall boundary tracing
- `dtrace`
  - scriptable dynamic instrumentation surface

These are the CPU-side timeline analogues to `nsys`, but with different
strengths at kernel, scheduler, and syscall boundaries.

### D. Memory and allocation behavior

- `valgrind`
  - correctness, leaks, invalid access, massif-style memory work
- `heaptrack`
  - allocation-site growth and heap lifetime
- `memray`
  - Python allocation tracing

### E. Python support

- `py-spy`
  - Python CPU sampling
- `memray`
  - Python memory

### F. GPU anchor tools

- `ncu`
  - kernel-level GPU measurements
- `nsys`
  - GPU timeline
- `nvidia-smi`
  - GPU status / clocks / driver context

These remain essential because the CPU translation lane should stay tied to the
original CUDA/SASS evidence rather than drifting into generic CPU tuning.

### G. Visualization

- `inferno-flamegraph`
  - flamegraph generation from profiling stacks

## 4. Local tool status

Verified locally on this machine:

| Category | Tool | Local anchor |
|----------|------|--------------|
| GPU kernel | `ncu` | `/usr/bin/ncu`, `2026.1.0.0` |
| GPU timeline | `nsys` | `/usr/bin/nsys`, `2025.6.3.343` |
| GPU status | `nvidia-smi` | `/usr/bin/nvidia-smi`, driver `595.45.04` |
| CPU profiling | `perf` | `/usr/bin/perf`, `6.19.9-1` |
| CPU profiling GUI | `hotspot` | `/usr/bin/hotspot`, `1.6.0` |
| CPU+GPU profiling | `AMDuProfCLI` | `/opt/amduprof/bin/AMDuProfCLI`, `5.2.606.0` |
| Python profiling | `py-spy` | `/usr/bin/py-spy`, `0.4.1` |
| Python memory | `memray` | `/home/eirikr/.local/bin/memray`, `1.19.2` |
| Memory profiling | `valgrind` | `/usr/bin/valgrind`, `3.25.1` |
| Memory profiling | `heaptrack` | `/usr/bin/heaptrack`, `1.5.0` |
| Tracing | `bpftrace` | `/usr/bin/bpftrace`, `v0.25.0` |
| Tracing | `trace-cmd` | `/usr/bin/trace-cmd`, `3.4.0` |
| Tracing | `strace` | `/usr/bin/strace`, `6.19` |
| Tracing | `dtrace` | `/usr/bin/dtrace` |
| Visualization | `inferno-flamegraph` | `/home/eirikr/.cargo/bin/inferno-flamegraph` |

Useful supporting tools also present:
- `objdump`
- `llvm-objdump`
- `readelf`
- `nm`
- `llvm-mca`
- `cpuid`
- `lscpu`

## 5. Hyper-complementary toolkit design

The toolkit should not just be a list of tools. It should be staged:

1. ISA shape
   - `objdump`, `llvm-objdump`, `readelf`, `nm`
2. static pipeline estimate
   - `llvm-mca`
3. microbench reality
   - `src/zen_re/isa/` probes and `perf stat`
4. Zen-specific PMU/IBS confirmation
   - `AMDuProfCLI`
5. whole-program hotspot attribution
   - `perf record`, `hotspot`, `inferno-flamegraph`
6. scheduler/timeline tracing
   - `trace-cmd`, `bpftrace`, `dtrace`, `strace`
7. memory correctness and allocation pressure
   - `valgrind`, `heaptrack`, `memray`
8. GPU anchor comparison
   - `ncu`, `nsys`, `nvidia-smi`

This is how we avoid overlap:
- `perf` for broad counter and sampling work
- `uProf` for AMD-specific detail
- `trace-cmd`/`bpftrace` for scheduler/kernel questions
- `valgrind`/`heaptrack`/`memray` for memory behavior
- `objdump`/`llvm-mca` for static instruction-level reasoning

## 6. Immediate design decision

The CPU reverse-engineering lane should optimize in this order:

1. AVX2/FMA/BMI/POPCNT/TZCNT/LZCNT production lanes
2. SSE-family fallback lanes where the ABI floor matters
3. tracing + PMU + memory integration
4. x87/MMX/SSE4a archaeology only after the modern lane is well measured

x87 and MMX are worth measuring and documenting, but they should not dominate
the current optimization budget for `steinmarder`.
