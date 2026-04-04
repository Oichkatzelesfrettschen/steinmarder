# Memory Notes — steinmarder RE Program

> Auto-generated from Claude Code session memory. Keep in sync with
> `~/.claude/projects/-Users-eirikr-Documents-GitHub-steinmarder-src-sass-re/memory/`.

---

## x130e TeraScale-2 RE Campaign

Active r600/TeraScale-2 RE track on x130e (SSH: `nick@x130e`).

**Hardware**: AMD E-300 APU (Family 14h Bobcat, 2 cores 1.3GHz, NO AVX, IBS available).
GPU: Radeon HD 6310 VLIW5, 2 SIMD, 487MHz, 384MB UMA VRAM + 1GB GTT. 16GB DDR3 shared.

**Current scores (2026-03-31)**:
- GL (r600 native, modesetting DDX): **420 FPS** at load ~2.5
- VK headless (Terakan): **522 FPS** at load ~2.5  *(debug build; ~776–803 FPS observed in later sessions)*
- Target: **2000 FPS** vkmark headless

**Build workflow**: Build on DESKTOP-CKP9KB6 (32 cores, ~40s), rsync to x130e. Source on both at `~/mesa-26-debug/`. Clang-19, -O2 -g (debugoptimized), no LTO (LTO hurts on E-300's 32KB L1i).

**Key blockers**:
- Zink over Terakan: BLOCKED by `DRM_CAP_SYNCOBJ=0` in radeon kernel (no DRI3 GLX). Needs Terakan VK_KHR_swapchain for drisw path.
- LTO counterproductive on E-300 (code bloat > optimization for tiny L1i)
- AMDuProfCLI IBS: reported available but collect rejects on Family 14h

**Patches applied**:
- `terakan_instance.c`: added `.EXT_headless_surface = true` (enables vkmark headless)
- `terakan_instance.c`: fixed broken string literal in `vk_icdGetInstanceProcAddr`
- `zink_screen.c`: added Terakan to `can_do_invalid_linear_modifier` whitelist (partial fix, DRI3 still blocked by kernel)

**Xorg config**: Switched to modesetting DDX at `/etc/X11/xorg.conf.d/10-modesetting.conf`. Old radeon config disabled. DRI3 extension loads but GLX uses DRI2 (kernel syncobj missing).

**SSH mesh**: x130e ↔ DESKTOP-CKP9KB6 ↔ MacBook Air, all by hostname. distcc configured (DESKTOP-CKP9KB6/32 + localhost/2). MacBook is ARM64 — cannot be distcc compile host for x86.

**Findings location**: `~/eric/TerakanMesa/findings/` on x130e. Latency probes at `~/eric/TerakanMesa/re-toolkit/probes/`.

---

## x130e Tooling Capabilities

**AMDuProfCLI v5.2.606 on E-300 (Family 14h)**:
- IBS-fetch: YES
- IBS-op: YES
- Core PMC: NO
- GPU profiling: NO
- Use for CPU-side Mesa driver hotspot analysis only.

**GPU profiling tools that work**: `R600_DEBUG`, `GALLIUM_HUD`, `radeontop`, `apitrace`, radeon ftrace tracepoints.

**Do NOT use**: GPUPerfAPI, amdgpu_top, NVTOP, UMR — all require the `amdgpu` driver, not `radeon`.

**CodeXL 1.9**: Last official AMD pre-GCN GPU profiler. Archived, buildable with containerization. LOW priority.

**Key radeon ftrace tracepoints** (all confirmed available via `perf list`):
- `radeon_cs`, `radeon_fence_emit`, `radeon_fence_wait_begin/end`
- `radeon_vm_bo_update`, `radeon_vm_flush`, `radeon_vm_set_page`

---

## x130e distcc Configuration

### distcc hosts (`~/.distcc/hosts` on x130e — persistent, survives reboot)
```
10.0.0.180/8         # MacBook ARM (cross-compiling to x86_64, 8 slots)
DESKTOP-CKP9KB6/32   # Windows desktop (32 slots)
localhost/2          # x130e itself (2 slots, Bobcat)
```
Total: 42 slots → `ninja -j42` (or `-j40` to be safe)

### Compiler wrappers (`/tmp/distcc-wrap/` on x130e — **cleared on reboot, must recreate**)
```bash
mkdir -p /tmp/distcc-wrap
printf '#!/bin/sh\nexec distcc /usr/bin/clang-19 "$@"\n' > /tmp/distcc-wrap/cc
printf '#!/bin/sh\nexec distcc /usr/bin/clang++-19 "$@"\n' > /tmp/distcc-wrap/c++
chmod +x /tmp/distcc-wrap/cc /tmp/distcc-wrap/c++
```

### Build command
```bash
# On x130e:
cd /home/nick/mesa-26-debug/build-debug && ninja -j42
# Install after successful build:
sudo ninja install
```

### Mac distccd (10.0.0.180)
Started with: `distccd --daemon --allow 10.0.0.0/24 --log-file /tmp/distccd-mingw.log --verbose`
Has `clang-19` in whitelist. Cross-compiles for `-target x86_64-pc-linux-gnu`.
Verify: `nc -z 10.0.0.180 3632 && echo reachable`

---

## Rusticl r600g Compute Fixes (10 bugs, 2026-04-01)

The r600g Rusticl compute path now works for RAT writes and INT8/INT16. Verified with clpeak on AMD PALM HD 6310.

### clpeak results
| Test | Peak | Scalar v1 |
|------|------|-----------|
| INT8 char (GIOPS) | 15.21 (char4) | 7.62 |
| INT32 int (GIOPS) | 14.97 (int2) | 7.34 |
| FP32 (GFLOPS) | 58.62 (float4) | 14.96 |

INT8 ≈ INT32 peak — no 8-bit acceleration on TeraScale-2; both use MULLO_INT (trans slot).
FP32 v4 peak = 58.62 GFLOPS ≈ 4× INT peak — confirms 4-wide VLIW packing for MULADD_IEEE.

### Root causes fixed

**RAT write fixes (7 bugs)**:
1. `compute_emit_cs` never emitted CB0 hardware registers
2. `rat_mask` from wrong struct (`cb_misc_state` vs `compute_cb_target_mask`)
3. `r600_need_cs_space(rctx, 0, ...)` not accounting for CB0 block → abort
4. No CS_PARTIAL_FLUSH for Evergreen after dispatch
5. No CS flush after `evergreen_launch_grid`
6. `fence_finish` crash when called synchronously inside dispatch
7. Debug trace `pipe_buffer_map_range` inside CS construction flushed mid-packet

**Sub-32-bit INT conversion fixes (2 bugs)**:
8. `AluInstr::from_nir` hit `assert(0)` for `u2u8`, `u2u16`, `i2i8`, `i2i16`, `u2u32`, `i2i32`
   - Fix: `r600_lower_bit_size_callback` in `sfn_nir.cpp` + explicit `emit_alu_trunc_u2uN` / `emit_alu_sext_i2iN` handlers
9. `evergreen_get_compute_state_info` hardcoded `max_threads = 128` — fixed to compute from ngpr

**Wide-vector `load_global` fix (1 bug)**:
10. `emit_load_global` wide vector crash — float16 channels 4-15 never registered
    - Fix: one `LoadFromBuffer(fmt_32_32_32_32, MFC=15)` per 4-component group with `temp_vec4(pin_group)`, then MOV to `dest(def, comp_base+i, pin_free)`

### Global memory bandwidth
| Vector width | GBPS |
|---|---|
| float | 10.38 |
| float2 | 18.22 |
| float4 | 30.35 |
| **float8** | **36–38.85 (peak)** |
| float16 | 28.79 (extra GPRs from temp_vec4+MOV) |

### Patch locations (Mesa 26.0.3 on x130e)
- `evergreen_compute.c`: RAT fixes, max_threads from ngpr
- `sfn_nir.cpp`: `r600_lower_bit_size_callback` + `nir_lower_bit_size`
- `sfn_instr_alu.cpp`: `emit_alu_trunc_u2uN`, `emit_alu_sext_i2iN`

**Re-apply scripts** (in `/tmp/` on x130e — recreate after reboot):
- `/tmp/patch_all.py` — RAT patches 1-7
- `/tmp/patch_sub32bit_and_occupancy.py` — fixes 8-9
- `/tmp/patch_emit_load_global_v2.py` — fix 10

**Pending**:
- Scratch spill (MEM_SCRATCH): infrastructure exists but RA failure path not wired
- VLIW t-slot packing: `schedule_alu` emits 1 op per group (break after first success). True 5-slot bundles require pre-packed `AluGroup` at emission

---

## Terakan VLIW Debug Infrastructure

`TERAKAN_DEBUG=shaders,perf,cs,pipeline` activates debug output in the Terakan Vulkan driver. Implemented 2026-04-01.

### Debug flags (`terakan_instance.h`)
```c
TERAKAN_DEBUG_STARTUP  = (uint64_t)1 << 0   // existing
TERAKAN_DEBUG_SHADERS  = (uint64_t)1 << 1   // ISA dump + VLIW utilization stats
TERAKAN_DEBUG_PERF     = (uint64_t)1 << 2   // warn if VLIW < 70% utilization
TERAKAN_DEBUG_CS_DUMP  = (uint64_t)1 << 3   // CS dump on error
TERAKAN_DEBUG_PIPELINE = (uint64_t)1 << 4   // pipeline creation log
```

### VLIW stats walker (`terakan_shader_sfn.cpp`)
- Walks CF/ALU linked list BEFORE `r600_bytecode_clear`
- Counts bundles, slot histogram (1–5 slot fill), total ALU instructions
- Computes utilization = `total_alu / (bundles × 5)`
- Prints: `TERAKAN_VLIW [stage]: GPR=N ndw=N cf_alu=N bundles=N alu=N util=X.X%`
- Prints: `  hist: 1=N 2=N 3=N 4=N 5=N` + `5-slot%=X  avg=X.XX`

### Key fix: `list_container_of` C++ compatibility
`src/util/list.h:204` — changed `(void *)` cast to `(__typeof__(sample))`. Makes ALL `LIST_FOR_EACH_ENTRY` macros C++ compatible, allowing the VLIW walker to live in `terakan_shader_sfn.cpp`.

### Activation
```bash
TERAKAN_DEBUG=shaders VK_ICD_FILENAMES=.../terascale_icd.x86_64.json \
  vkmark --winsys headless -b 'vertex:duration=2'

TERAKAN_DEBUG=perf    # only prints if util < 70%
```

**VLIW 65.7% utilization is near the theoretical maximum** for the vertex shader — caused by:
1. Hardware 4-literals-per-bundle limit (forces 1-slot bundle when 5th instruction has its own literal)
2. PV forwarding chains (bundle N reads PV.w from bundle N-1 → must be sequential)

No scheduler improvement can fix these; they are hardware constraints.

---

## Terakan `copy_sync_payloads` RCA (2026-04-01)

`copy_sync_payloads` was attempted to eliminate the 3rd RADEON_CS per frame (present-throttle fence signal, ~77µs, ~6% FPS gain at ~800 FPS). Implementation was reverted due to fundamental Mesa sync model incompatibility.

### Three cascading errors

| Error | Root cause |
|-------|-----------|
| `Assertion 'value < pending_value' failed` in `terakan_sync_completion_signal` | `vk_sync_signal_many` called without `pending_value` ever advanced by queue_submit (no GPU work in fast path) |
| `Assertion 'CPU_RESET' failed` in `assert_reset_valid` | `vk_device_copy_semaphore_payloads` resets waited-but-not-signaled syncs; `binary->timeline` lacks CPU_RESET |
| Mesa `vk_sync_type_validate` forbids CPU_RESET on timeline types | Cannot add CPU_RESET to `terakan_sync_completion_type` |

### Root cause
`vk_device_copy_semaphore_payloads` (Mesa runtime) resets waited syncs after copying payloads. The wait sync — a binary semaphore — is unwrapped via `vk_sync_wait_unwrap` to `binary->timeline` (a `terakan_sync_completion` with `VK_SYNC_IS_TIMELINE`). Mesa forbids `VK_SYNC_FEATURE_CPU_RESET` on timeline types. `vk_sync_reset_many` asserts both `CPU_RESET` AND `!IS_TIMELINE`.

**What other Mesa drivers do**: They use `vk_drm_syncobj` — a single type with `BINARY|CPU_RESET` (no `vk_sync_binary` wrapper). `vk_sync_as_binary()` returns NULL → no unwrapping → the binary sync itself is in the resets array and can be reset.

### Required fix: `terakan_sync_binary_type`

New struct:
```c
struct terakan_sync_binary {
    struct vk_sync vk;
    uint64_t next_point;
    struct terakan_sync_completion timeline;  // embedded
};
```

Features: `BINARY | GPU_WAIT | CPU_WAIT | CPU_RESET | CPU_SIGNAL | WAIT_ANY | WAIT_PENDING` (NOT TIMELINE)

Required: `init`, `finish`, `reset` (`++next_point`), `signal` (delegate to timeline), `wait_many` (delegate to timeline), `move`.

- Replace `device->sync_type_binary = vk_sync_binary_get_type(...)` with `&terakan_sync_binary_type`
- Modify `terakan_queue_submit` to accept `terakan_sync_binary_type` signals (extract inner timeline)
- ~200 lines across `terakan_sync_binary.{c,h}` + `terakan_physical_device_drm_radeon.c` + `terakan_queue.c`

Once `terakan_sync_binary_type` is implemented, also wire:
```c
device->vk.copy_sync_payloads = terakan_copy_sync_payloads;
```
where `terakan_copy_sync_payloads` advances `pending_value` + `current_value` directly under `completion_mutex` (no call to `vk_sync_signal_many`).

---

## Apple Track Confirmed Measurements

### CPU (M-series arm64, ~3.2 GHz)
- Integer ADD: **1 cycle**
- Integer MUL/MADD/MSUB/UMULH/SMULL: **3 cycles** (UMULH same cost as MUL — use freely for 128-bit arithmetic)
- f64 FMADD: **~4 cycles** (MCA predicts 3.18 — use measured value)
- Relaxed atomic add: **~6 cycles**
- sin+cos pair (libm): **~60 cycles** (~30 each) — MCA predicts 7 (8× wrong)
- Variable-shift chain: **4.30 cyc/op** — MCA predicts 1.41 (3× wrong)

### llvm-mca reliability
- ADD: ±20% (ok with caution)
- Integer multiply (all forms): MCA predicts 5 cyc, measured 3 — **never use for multiply**
- Variable-shift / atomic / transcendental: 3–8× wrong — never use

### Cache hierarchy (stream bandwidth)
- L1/L2: ≤128 KB, 0.65–0.82 ns/access (2.1–2.6 cyc)
- SLC (LLC): 256 KB–8 MB, 0.95–1.02 ns/access (3.0–3.3 cyc)
- DRAM: ≥32 MB, 2.28 ns/access (7.3 cyc)
- L1→SLC knee: ~128–256 KB (49% jump at 256 KB)

### GPU timing
- GPU-actual is **14–22× faster** than CPU wall-clock (due to `waitUntilCompleted` overhead)
- Use `--counters timestamp` (`commandBuffer.GPUStartTime/GPUEndTime`) for GPU-actual measurements
- Batch multiple dispatches per `MTLCommandBuffer` to amortize dispatch overhead

### FP16
- FCVT f32↔f16: ~3 cyc; FADD f16 scalar: ~3 cyc; FMLA f16×8: ~4 cyc — full speed, same as f64
- PyTorch MPS f16 8× anomaly at 512×512 is NOT hardware — likely JIT on first dispatch; re-measure with warmup

### Accelerate / AMX
- `vDSP_vadd` uses AMX, not NEON (53 AMX opcodes, 0 NEON float) for n ≥ 20480 elements
- CPU probes measure NEON pipeline; AMX is a separate unprobed co-processor
- Use Accelerate/BLAS for n > 20K; don't write NEON to beat Accelerate

### MTLCounterSet on AGXG13G (M1)
- `supportsCounterSampling:AtDispatchBoundary` = NO
- Only counter set: `timestamp` with `GPUTimestamp`
- Use `xcrun xctrace record --instrument 'Metal GPU Counters'` (NOT `--template`)

### Metal GPU counters (occupancy_heavy, 60,703 samples)
- ALU Utilization: peak 79%, avg 1.6% — NOT ALU-bound
- Fragment Occupancy: peak 100% (P99 90%) — M1 TBDR routes compute through fragment pipeline; this IS compute occupancy
- GPU Read/Write Bandwidth: peak 45%/54% — NOT memory-bound
- Total Occupancy avg 16% → host dispatch overhead dominates

### Neural / ML crossover
- PyTorch MPS: crossover at ~1024×1024 (MPS 1.29× faster)
- Element-wise ops on 1M elements via MPS: 15× slower than CPU (dispatch overhead)
- coremltools 9.0: ALL compute units including ANE available
- MLX 0.31.1 functional; JAX 0.4.38 with Metal experimental backend functional
