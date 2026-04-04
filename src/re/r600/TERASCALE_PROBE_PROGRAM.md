# TeraScale Probe Program

This document turns the x130e / TeraScale-2 reverse-engineering lane into a
first-class probe program inside `src/sass_re`.

The goal is not just to keep a pile of GLSL fragments or session logs around.
The goal is to make the TeraScale lane follow the same steinmarder evidence
contract already proven out in the CUDA and Apple tracks:

1. raw probe source
2. manifest-backed inventory
3. runtime measurement
4. disassembly / ISA capture
5. profiler and system sidecars
6. promoted summary tables and decisions

## Method Transfer

### From CUDA / SASS

- dependent-chain latency families
- independent-accumulator throughput families
- width and packing sweeps
- manifest-driven compile / disasm / profile loops
- compiler-vs-manual decision discipline

### From Apple

- tranche-style phased runners
- machine-readable `run_manifest.json`
- promoted result bundles with summaries and health ledgers
- CPU + GPU + host-overhead capture in one run root

### Into TeraScale

The TeraScale lane has to cover more API surfaces than the current fragment
shader starter kit. The canonical families are:

- OpenGL fragment probes
- piglit `shader_runner` probes
- Vulkan graphics and WSI probes
- Vulkan compute probes
- OpenCL probes
- OpenGL compute probes where the stack exposes them
- system probes for CPU / DRM / fence / queue / VRAM / GTT behavior

## Directory Layout

```text
src/sass_re/
  probes/r600/
    legacy_glsl/        imported fragment probes from TerakanMesa re-toolkit
    shader_tests/       imported shader_runner probes
    manifests/          imported and generated manifest artifacts
  scripts/
    emit_terascale_probe_manifest.py
    run_terascale_probe_tranche.sh
    run_terascale_trace_stack.sh
  tables/
    table_terascale_probe_taxonomy.csv
```

## Probe Lanes

### Lane 1: OpenGL fragment latency and packing

Current imported corpus:

- dependent ALU chains
- transcendental chains
- texture fetch latency and bandwidth
- VLIW5 packing probes
- register-pressure sweeps
- clause switching

These are the quickest way to keep learning the ISA and bundle structure.

### Lane 2: shader_runner correctness + ISA

The imported `shader_tests/` lane is the bridge from fragment-only timing into
repeatable correctness, compiler output, and per-probe ISA capture.

### Lane 3: Vulkan graphics / WSI

This lane covers:

- instance / extension / device enumeration
- headless viability
- X11 / DRI3 presentation
- future `VK_KHR_display` coverage

### Lane 4: Vulkan compute

This is where we should port the raw dependent-chain methodology most directly:

- SPIR-V or GLSL compute kernels
- typed sweeps:
  - `u8/i8/u16/i16/u32/i32/u64/i64`
  - `int24/uint24`
  - packed vs unpacked byte lanes
- explicit packing experiments
- register-pressure and local-memory experiments
- occupancy and register-stuffing ladders:
  `occupancy_gpr_{04,08,16,24,32,48,64,96}`

### Lane 5: OpenCL

This lane matters because Rusticl / Clover-style surfaces may expose the older
hardware more directly for arithmetic and memory experiments.

### Lane 6: OpenGL compute

Use only when the stack truly exposes the necessary capability. The lane still
deserves a home in the taxonomy so we stop rediscovering that question.

### Lane 7: System trace stack

This lane keeps CPU, DRM, RAM, IO, and queueing evidence tied to the exact
probe tranche that triggered it.

The canonical capture stack is:

- `perf stat` and `perf record`
- `trace-cmd` DRM and scheduler tracepoints
- `radeontop`
- `AMDuProfCLI`
- `vmstat` and `iostat`

## Evidence Contract

No TeraScale optimization claim should be accepted without these when
applicable:

- probe source path
- generated manifest row
- runtime timing output
- ISA or compiler output
- profiler / system sidecar
- promoted summary note or table row

## Immediate Families

The first tranche should grow these families deliberately:

- ALU latency: `ADD`, `MUL`, `MULADD`, `DOT4`, `RECIPSQRT`, `SIN`, `COS`
- integer latency: `MULLO_INT`, carry / bit-extract / subword convert
- typed integer baselines:
  `u8/i8/u16/i16/u32/i32/u64/i64`, `int24`, `uint24`
- cache / memory: kcache, TEX L1 hit, TEX stride, LDS and scratch once exposed
- bundle utilization: scalar, vec4, vec4+trans, mixed packing
- pressure: GPR 4/8/16/24/32/48/64/96
- WSI / queue: present path, headless path, DRM / DRI3 blocking points

## Current Manual Boundary

As of the 2026-04-02 manual diagnostics bundle:

- Vulkan compute no longer crashes in the common runtime
- a targeted `u32` compute probe now reaches `vkCreateComputePipelines` and
  fails cleanly with `VK_ERROR_FEATURE_NOT_PRESENT`
- the driver now prints an explicit Terakan-side stderr diagnostic explaining
  that the remaining missing work is the real `vk_shader` wrapper plus
  serialize/deserialize and cache integration
- OpenCL integer and occupancy probes execute, but still produce pathological
  floating-point interpretations with high `nan_count`, which makes typed-output
  host paths and packed-vs-unpacked sweeps the next high-value tranche

## Commands

Emit the current local TeraScale probe manifest:

```sh
cd src/sass_re
python3 scripts/emit_terascale_probe_manifest.py emit --format tsv --header | head
```

Run the first TeraScale tranche skeleton:

```sh
cd src/sass_re
bash scripts/run_terascale_probe_tranche.sh --phase all
```

Run the host-side trace bundle:

```sh
cd src/sass_re
bash scripts/run_terascale_trace_stack.sh
```

The tranche runner is intentionally structured like the CUDA and Apple tracks:
phase logs, step status, and run manifest first; widening coverage second.
