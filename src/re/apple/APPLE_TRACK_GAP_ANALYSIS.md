# Apple Track Gap Analysis vs. CUDA/SASS Track

Date: 2026-04-01
Source runs: `2026-03-30_tranche1_r5_variants_frontier`, `2026-03-30_tranche1_r7_cde_keepalive`

This document compares the Apple track methodology against the mature SASS/SM89
track to identify gaps in scripts, tools, and measurement coverage.

The analysis changed meaningfully after the r7 keepalive bundle and the typed
atlas landing in `src/sass_re`: raw Metal GPU counters, GPU command-buffer
timing, and promoted neural placement artifacts now exist. The main gap is no
longer "can we see anything," but "can we keep every requested type family
honest across CPU, Metal, and neural lanes?"

---

## 1. Tooling Stack Comparison

### SASS/CUDA (mature)

| Tool | Purpose | Apple equivalent |
|------|---------|-----------------|
| `nvcc` / `ptxas` | GPU kernel compilation | `xcrun metal` |
| `cuobjdump` / `nvdisasm` | **Actual GPU ISA** disassembly (SASS) | `xcrun metallib --disassemble` → AIR/LLVM IR only, NOT GPU ISA |
| `ncu` (Nsight Compute) | Per-kernel hardware counters: L1/L2 hit rate, warp efficiency, ALU utilization | `xctrace --instrument 'Metal GPU Counters'` + `MTLCounterSet` host timing/counter hooks |
| `nsys` (Nsight Systems) | Timeline profiling — kernel launch to completion | `xctrace` command buffer / driver / GPU state intervals ✓ |
| `hyperfine` | Host-side timing | `hyperfine` ✓ |
| `perf` / `valgrind` | CPU-side profiling | `sample`, `spindump`, `leaks`, `vmmap` ✓ |
| `objdump` | CPU binary disassembly | `otool -tv`, `llvm-objdump` ✓ |
| `llvm-mca` | Throughput/latency prediction from asm | `llvm-mca` ✓ (promoted in r7) |
| CUDA sanitizers | Memory/UB checks | ASan/UBSan ✓ |
| `strace` / FS traces | Syscall and filesystem event tracing | `fs_usage` ✓ (with sudo keepalive) |
| `probe_manifest.py` | Machine-readable probe inventory | `apple_capability_report.py` (partial) |
| `compare_architectures.py` | Cross-architecture timing deltas | `compare_xctrace_density_runs.py` (density only) |
| `flag_matrix_sweep.sh` | Multi-flag compiler comparison | `compile_matrix.txt` (r5: O0/O2 only, incomplete) |
| `mine_cudnn_library_mnemonics.sh` | Real-world library mnemonic mining | **Missing** — no Metal.framework / MPS mnemonic mining |
| `encoding_analysis.py` | Bit-level SASS encoding analysis | **Not applicable** (AIR is LLVM IR, no ISA encoding to analyze) |
| `auto_explorer.py` | Automated frontier exploration | **Missing** |

### Tools documented in `APPLE_SILICON_RE_GUIDE.md` but not yet in active use

Per `docs/sass/APPLE_SILICON_RE_GUIDE.md`, the recommended Apple tooling is:

```
Full Xcode app (NOT Command Line Tools only)
  xcrun metal         — shader compilation (-S for AIR disassembly)
  xctrace             — Metal timeline and hardware counter capture
  Instruments.app     — GUI counterpart (not CLI-automatable)

Homebrew:
  llvm                — llvm-mca, llvm-objdump (installed, used in r5 only)
  cmake / ninja       — build system
  ccache              — compilation cache

Observation tools:
  otool               — Mach-O disassembly ✓
  dtrace / dtruss     — system call tracing ✓
  fs_usage            — filesystem event tracing ✓ (keepalive)
  sample              — CPU call-graph sampling ✓
  spindump            — thread state capture ✓
  leaks / vmmap       — memory diagnostics ✓ (r7+)
  powermetrics        — GPU power + thermal + performance counters ✓
```

**Resolved baseline: GPU counters and GPU-side timing are now promoted**

The Apple lane now has both:
- `xctrace --instrument 'Metal GPU Counters'` promoted in blessed artifacts
- host-side `GPUStartTime` / `GPUEndTime` timing in the Metal harness

That means the Apple gap has shifted from "no hardware signal" to "typed
surface still too narrow."

**Current top gap: no end-to-end typed matrix runner**

We still lack one runner that:
- preserves raw JSON plus normalized CSV for each requested family
- keeps `int`, `uint`, `fp`, `bf`, `tf`, `mx`, and `more` visible even when
  support is proxy-only or unsupported
- aligns neural placement-only evidence with CPU and Metal direct measurements
- writes the same schema into promoted bundles every time

---

## 2. Probe Coverage Comparison

### CPU lane

| SASS probe family | SASS method | Apple probe | Apple method | Gap |
|------------------|-------------|-------------|-------------|-----|
| ADD latency | dependent 32-chain, `ADD.S32` | `add_dep_u64` | dependent 32-chain, AArch64 `add` ✓ | — |
| FADD latency | dependent 32-chain | `fadd_dep_f64` | dependent 32-chain, `fadd d0,d0,d1` ✓ | — |
| FMADD/FMA latency | dependent 32-chain | `fmadd_dep_f64` | dependent 32-chain, `fmadd d0,d0,d1,d2` ✓ | — |
| Load latency (L1 hit) | pointer-chase array < L1 | `load_store_chain_u64` | volatile array, NOT pointer-chase | **Partial** — not a clean pointer-chase |
| Load latency (L2, LLC, DRAM) | pointer-chase stride sweep | `apple_cpu_cache_pressure.c` | stride sweep EXISTS | **Not promoted** — in probes/ but no blessed results |
| Store throughput | write-back chain | `load_store_chain_u64` | mixed load/modify/store | Partial |
| Shuffle/lane-crossing | byte-level operations | `shuffle_lane_cross_u64` | bswap64 + variable shift chain ✓ | — |
| Atomic CAS | contended CAS chain | **Missing** | — | **Gap** |
| Atomic EXCH | exchange chain | **Missing** | — | **Gap** |
| Atomic ADD | `atomic_add_relaxed_u64` | relaxed fetch_add chain ✓ | — | — |
| Transcendental (sin/cos) | dependent sin→cos chain | `transcendental_sin_cos_f64` ✓ | — | — |
| Transcendental (sqrt, log, exp) | individual dependent chains | **Missing** | — | **Gap** |
| Integer MUL latency | dependent IMUL chain | **Missing** | — | **Gap** |
| FP16 add/mul latency | dependent f16 chain | **Missing** | — | **Gap** |
| CRC / bitfield ops | CRC32, RBIT, CLZ | **Missing** | — | **Gap** |

**CPU lane mnemonic coverage (r5, from `cpu_mnemonic_counts.csv`)**:

The O0 build exposes: `add`, `fadd`, `fmadd`, `ldr`, `ldur`, `str`, `stp`, `ldp`,
`b`, `bl`, `cbz`, `cbnz`, `adrp`, `eor`, `fmov`, `fdiv`, `fmul`.
The O2 build is the probe-hot path: `add`, `subs`, `cmp`, `stp`, `ldp`, `bl`, `mov`, `b`.

**Missing from CPU mnemonic corpus**: `mul`, `umulh`, `madd`, `msub`, `smull`,
`sqrt`, `fsqrt`, `fabs`, `fneg`, `fcvt`, `dup`, `fminnm`, `fmaxnm`, `ldxr`,
`stxr`, `cas`, `casal`, `ldadd`, `swp`, `prfm` (prefetch), SIMD `fmla`, `fmls`,
`ext`, `tbl`, `tbx`, `zip`, `uzp`, `trn`.

### Metal GPU lane

| SASS/CUDA probe | Method | Apple Metal probe | Method | Gap |
|----------------|--------|-------------------|--------|-----|
| FFMA latency (FP32) | 32-deep dependent chain | `probe_fma_lat.metal` | 8-step serial fma dep chain, reassoc(off) ✓ | **RESOLVED** 1.993 ns (2026-04-02) |
| FADD latency (FP32) | dependent chain | `probe_fadd_fmul_lat_cmp.metal` | 8-step serial fadd dep chain ✓ | **RESOLVED** 1.695 ns (2026-04-02) |
| IMUL latency (INT32) | dependent chain | `probe_imul_lat.metal` | 8-step serial imul dep chain ✓ | **RESOLVED** 3.376 ns = 2.0× fadd (2026-04-02) |
| LDS/Threadgroup latency | dependent threadgroup load chain | `probe_threadgroup_lat.metal` | serial store→load dep chain in threadgroup[] ✓ | **RESOLVED** (T5, pending documentation) |
| Global memory L1 latency | dependent load chain (L1 sized buf) | `probe_global_l1_lat.metal` | dep load chain in 4KB output buffer ✓ | **RESOLVED** (T6, pending documentation) |
| Global memory L2 latency | dependent load chain (L2 sized buf) | **Missing** | — | **Gap** (requires larger buffer; host API constraint) |
| Texture sample latency | dependent textureSample chain | **Missing** | — | **Gap** |
| IADD throughput (independent acc) | 8 independent accumulators | `probe_fp32_throughput.metal` | 8 independent fadd accumulators ✓ | **RESOLVED** fadd-tp=1.090 ns, fmul-tp=0.751 ns (2026-04-02) |
| FFMA throughput | 8 independent accumulators | `probe_fp32_throughput.metal` | 8 independent fma accumulators ✓ | **RESOLVED** fma-tp=1.764 ns (half-rate unit confirmed) (2026-04-02) |
| Warp/simdgroup reduce | `__reduce_add_sync` | `probe_simdgroup_reduce.metal` | single barrier + arithmetic ✓ | Partial |
| Simdgroup sum latency | `simd_sum` dependent chain | `probe_simdgroup_lat.metal` | serial simd_sum dep chain ✓ | **RESOLVED** (T7, pending documentation) |
| Simdgroup broadcast | `simd_broadcast` chain | **Missing** | — | **Gap** |
| Simdgroup shuffle | `simd_shuffle` dependent chain | **Missing** | — | **Gap** |
| Register pressure sweep | explicit GPR count variants | `probe_register_pressure.metal` | 3 accumulators + float mix ✓ | Partial |
| Occupancy sweep | varying threadgroup sizes | `probe_occupancy_heavy.metal` | 32-iter LCG loop ✓ | Partial |
| Threadgroup memory bandwidth | read/write threadgroup[] arrays | `probe_threadgroup_heavy.metal` | threadgroup pressure ✓ | Partial |
| Atomic (threadgroup) | `atomic_fetch_add` on threadgroup | `probe_atomic_lat.metal` | serial atomic_fetch_add dep chain ✓ | **RESOLVED** (T8, pending documentation) |
| Atomic (global) | `atomic_fetch_add` on device buffer | `probe_atomic_lat.metal` | serial atomic_fetch_add on device buffer ✓ | **RESOLVED** (T8, pending documentation) |

### Metal AIR opcode coverage (r7, `metal_air_opcode_counts.csv`)

Operations observed across all 5 variants:

```
add: 26    xor: 20    shl: 13    zext: 11    getelementptr: 11
br: 10     phi: 9     store: 8   mul: 8      and: 6
ret: 5     icmp: 5    load: 5    tail_call: 4  lshr: 4
or: 3      fmul: 2    fadd: 1
```

Notable absences from AIR corpus:
- `fdiv`, `frem`, `fsub` — no FP division or subtraction
- `fneg`, `fabs` — no FP negation or absolute value
- `fptoui`, `uitofp` (other than `air.convert.*`) — no explicit FP↔int casts
- `extractelement`, `insertelement`, `shufflevector` — no SIMD vector ops
- `atomicrmw`, `cmpxchg` — no atomic operations
- `call @air.threadgroup*` — no explicit threadgroup memory ops in any shader
- `call @air.simd.*` — no simdgroup intrinsics used

This confirms the current promoted AIR corpus is still narrow relative to the
typed atlas: it has better simdgroup and atomic coverage than the earliest
Apple runs, but it still does not classify the full requested type surface.

---

## 3. Script Coverage Comparison

### What SASS has that Apple lacks

| SASS script | Purpose | Apple equivalent | Status |
|-------------|---------|-----------------|--------|
| `flag_matrix_sweep.sh` | 4+ compiler flag combinations | `compile_matrix.txt` (O0/O2 only) | Incomplete |
| `ncu_profile_all_probes.sh` | Run `ncu` on every probe | `capture_gpu_counters.sh` + tranche integration | Partial |
| `probe_manifest.py` | Machine-readable probe registry | `apple_capability_report.py` | Partial |
| `mine_cudnn_library_mnemonics.sh` | Mine production libraries | **Missing** — no Metal.framework mining | Gap |
| `encoding_analysis.py` | Bit-field layout of ISA encoding | Not applicable (AIR = LLVM IR) | N/A |
| `compare_architectures.py` | Cross-arch timing normalization | `compare_xctrace_density_runs.py` (density only) | Partial |
| `auto_explorer.py` | Automated probe loop | **Missing** | Gap |
| `rank_live_uplop3_contexts.py` | Context-specific frontier ranking | **Missing** | Gap |
| `build_monograph_pdf.sh` | Publication artifact | **Missing** | Low priority |

### What Apple has that SASS lacks (Apple-specific additions)

| Apple script | Purpose | SASS analog |
|-------------|---------|------------|
| `run_apple_tranche1.sh` | 64-step tranche with phase slicing | `run_full_recursive_pipeline.sh` (similar) |
| `analyze_tranche_mnemonics.py` | CPU + AIR mnemonic aggregation | `mnemonic_hunt.py` (SASS-specific) |
| `compare_xctrace_density_runs.py` | Run-to-run density stability check | No direct analog (SASS doesn't need density normalization) |
| `bootstrap_neural_lane.sh` | Python ML stack setup | No analog (CUDA ML = PyTorch w/ CUDA, not research target) |
| `neural_lane_probe.py` | Core ML / MPS / MLX placement sweep | No analog |
| `prime_sudo_cache.sh` | Keepalive for `fs_usage` and `dtruss` | No analog (Linux has different root requirements) |

---

## 4. Missing Tools Summary

### Critical (changes build decisions)

1. **Typed atlas runner parity** — the repo now has canonical Apple type tables,
   but the tranche runner still does not emit them directly. Until the runner
   writes typed JSON/CSV artifacts, the atlas remains partially hand-curated.

2. **Metal type-surface classification** — we now have FFMA, TGSM, simdgroup,
   and atomic probes, but we still do not have compile-and-AIR classification
   for `f16`, `bf16`, `int8`, `uint8`, `fp8`, `tf32`, and `MX`.

3. **Neural typed sweep** — the blessed neural story is still placement-first
   and summary-heavy. We need a real backend/type matrix for `CoreML CPU_ONLY`,
   `CPU_AND_GPU`, `CPU_AND_NE`, `ALL`, PyTorch CPU/MPS, and MLX GPU.

4. **Cross-lane build-decision synthesis** — the Apple lane still needs one
   explicit answer per family: native, lowered, framework proxy, host proxy,
   emulated, unsupported, or unknown.

### Important (fills major gaps)

5. **Metal simdgroup operation probes** — `simd_sum`, `simd_broadcast`, `simd_shuffle`, `simd_prefix_inclusive_sum`. These are Apple GPU's warp-level operations. Currently not in any probe.

6. **Metal threadgroup memory latency probe** — pointer-chase in `threadgroup` memory to measure TGSM latency, comparable to SASS LDS latency measurement.

7. **Metal atomic probes** — `atomic_fetch_add` on both threadgroup and device buffers.

8. **CPU pointer-chase cache probe from blessed results** — `apple_cpu_cache_pressure.c` exists in `probes/` but L1/L2/LLC boundary measurements are not in any promoted blessed run.

9. **Metal `flag_matrix_sweep`** — `metal -O0`, `-O1`, `-O2`, `-Os` comparison on each shader variant. Shows what the compiler does vs. what we write.

### Medium (completes evidence chain)

10. **Neural lane raw artifact preservation** — promoted neural results exist,
    but Phase F still collapses too much into summaries and one-off JSON files.

11. **Metal `register-light` variant** — brackets density behavior from the other side of `register_pressure`. The FRONTIER_ROADMAP_APPLE.md checklist already calls for this.

12. **`MTLCounterSet` host harness** — runtime GPU counter sampling around dispatch. Complements `xctrace` with per-dispatch granularity.

13. **Library mnemonic mining** — `otool -tv /System/Library/Frameworks/Metal.framework/Metal` or equivalent to mine real Apple production Metal code, comparable to `mine_cudnn_library_mnemonics.sh`.

---

## 5. Immediate Actionable Items

In priority order:

```sh
# 1. Replace the thin neural phase with a typed matrix sweep
#    Preserve raw JSON and normalized CSV rows for:
#    CoreML CPU_ONLY / CPU_AND_GPU / CPU_AND_NE / ALL
#    PyTorch CPU / MPS
#    MLX GPU

# 2. Add Metal type-surface classification rows
#    Compile small kernels for:
#    f16, bf16, int8, uint8, fp8, tf32, mxfp8
#    Then classify each row as native / lowered / unsupported from AIR.

# 3. Keep GPU counters and GPUStart/GPUEnd timing in every typed Metal sweep

# 4. Extend CPU reference probes only where Apple exposes direct execution

# 5. Regenerate the typed atlas tables from runner output
src/apple_re/scripts/run_next42_neural_suite.sh \
  --out src/apple_re/results/blessed/2026-04-01_neural_tranche1/
```

---

## 6. CUDA-grade Parity Checklist

| Capability | SASS status | Apple status |
|-----------|-------------|-------------|
| Per-opcode latency table | 448 mnemonics measured | partial GPU, promoted CPU families, typed atlas seeded |
| Per-opcode throughput table | 448 mnemonics | partial GPU, CPU reference lane only |
| Hardware counter capture | ncu (L1/L2/ALU/warp) | xctrace GPU counters + host GPUStart/GPUEnd timing |
| ISA disassembly | Full SASS assembly | AIR/LLVM IR only (no GPU ISA) |
| Mnemonic mining (libraries) | cudnn/cublas mined | **Missing** |
| Encoding analysis | Bit-field layout done | N/A (IR not encoded) |
| Cache hierarchy measured | L1/L2/DRAM latency | CPU cache knees promoted; GPU still missing |
| Simdgroup / warp ops | Measured | partial, not fully typed |
| Threadgroup / LDS latency | Measured | partial, not fully typed |
| Neural / compute placement | N/A | promoted, but not yet typed-matrix complete |
| Cross-run stability | Claims matrix + deltas | Density CSV stable ✓ |
| Publication artifacts | Paper draft + figures | **Missing** |
