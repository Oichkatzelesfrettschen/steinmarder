# Tooling Stack — Maximal Feature Activation Guide
## AMD E-300 APU + Radeon HD 6310 (TeraScale-2) on x130e

**Purpose**: Exhaustive, non-overlapping, synergistically stacked feature map
for every tool in the r600/TeraScale-2 reverse engineering toolkit.

Each tool covers a unique observability layer. Together they form a complete
pipeline from instruction-level CPU µarch → GPU ISA → DRM kernel → API calls.

---

## Layer 1: GPU Shader ISA — `R600_DEBUG`

**What it sees**: Compiled shader bytecode, register allocation, instruction scheduling.
**Nothing else provides this.** This is the only way to see what the SFN compiler emits.

### Flags (env var `R600_DEBUG`, comma-separated)

| Flag | Purpose | When to use |
|------|---------|-------------|
| `vs` | Dump vertex shader ISA | Shader analysis |
| `ps` | Dump pixel/fragment shader ISA | Shader analysis (most common) |
| `gs` | Dump geometry shader ISA | GS analysis |
| `tcs` | Dump tessellation control shader | TCS analysis |
| `tes` | Dump tessellation evaluation shader | TES analysis |
| `cs` | Dump compute shader ISA | Rusticl/compute analysis |
| `fs` | Dump fetch shader | Vertex fetch analysis |
| `tex` | Dump texture operations | TEX clause analysis |
| `precompile` | Compile shaders at creation (not deferred) | Get ISA at shader creation time |
| `nocpdma` | Disable CP DMA | Isolate CP DMA overhead |
| `noasyncdma` | Disable async DMA | Force synchronous transfers |
| `nohyperz` | Disable HyperZ depth compression | Isolate depth optimization |
| `no2dtiling` | Disable 2D tiling | Test linear vs tiled |
| `notiling` | Disable all tiling | Full linear mode |
| `switchoneop` | Switch on end of packet | Debug CS batching |
| `forcedma` | Force DMA for copies | Test DMA vs CP copy path |
| `info` | Print driver info at init | Verify driver configuration |
| `nowc` | Disable write-combining | Measure WC impact |
| `checkvm` | Validate VM operations | Debug GART issues |
| `shaderdb` | Output ShaderDB-format stats | Automated shader analysis |
| `compute` | Enable compute debug output | Debug Rusticl/OpenCL |

**Maximal ISA capture**: `R600_DEBUG=vs,ps,gs,cs,fs,tex,precompile,info`

### SFN Compiler Debug — `R600_NIR_DEBUG`

| Flag | Purpose |
|------|---------|
| `instr` | Log all consumed NIR instructions |
| `ir` | Log created R600 IR |
| `cc` | Log R600 IR → assembly code creation |
| `si` | Log shader info (non-zero values) |
| `reg` | Log register allocation and lookup |
| `io` | Log shader input/output |
| `ass` | Log IR → assembly conversion |
| `flow` | Log flow control instructions |
| `merge` | Log register merge operations |
| `nomerge` | Skip register merge step (for A/B testing) |
| `tex` | Log texture operations |
| `trans` | Log generic translation messages |
| `schedule` | Log VLIW5 scheduling decisions |
| `opt` | Log optimization passes |
| `steps` | Log shaders at each transformation step |
| `noopt` | Disable backend optimizations (for A/B testing) |
| `warn` | Print compiler warnings |

**Maximal compiler analysis**: `R600_NIR_DEBUG=instr,ir,cc,reg,io,ass,schedule,opt,steps,warn`
**Optimization A/B testing**: `R600_NIR_DEBUG=noopt` vs default (then diff ISA)

---

## Layer 2: GPU Pipeline Counters — `GALLIUM_HUD`

**What it sees**: Per-frame GPU pipeline activity, memory stats, system telemetry.
**Synergy with R600_DEBUG**: HUD shows runtime behavior, R600_DEBUG shows static ISA.

### Available counters (verified on x130e Mesa 26.0.3 debug)

**GPU Pipeline:**
| Counter | What it measures |
|---------|-----------------|
| `fps` | Frames per second |
| `frametime` | Per-frame wall time (ms) |
| `samples-passed` | Occlusion query samples |
| `primitives-generated` | Geometry output count |
| `ia-vertices` | Input assembler vertex count |
| `ia-primitives` | Input assembler primitive count |
| `vs-invocations` | Vertex shader invocation count |
| `gs-invocations` | Geometry shader invocation count |
| `gs-primitives` | GS output primitive count |
| `clipper-invocations` | Clipper input count |
| `clipper-primitives-generated` | Clipper output count |
| `ps-invocations` | Pixel shader invocation count |
| `cs-invocations` | Compute shader invocation count |
| `hs-invocations` | Hull shader invocations |
| `ds-invocations` | Domain shader invocations |

**System:**
| Counter | What it measures |
|---------|-----------------|
| `cpu` | Total CPU utilization |
| `cpu0` / `cpu1` | Per-core utilization |
| `cpufreq-cur-cpu0` / `cpufreq-cur-cpu1` | Current CPU frequency |
| `sensors_temp_cu-k10temp-pci-00c3.temp1` | CPU/APU temperature |
| `sensors_temp_cu-thinkpad-isa-0000.temp1` | Chassis temp sensor 1 |
| `sensors_temp_cu-thinkpad-isa-0000.temp2` | Chassis temp sensor 2 |

**Disk/Network (available but rarely needed for GPU RE):**
| Counter | What |
|---------|------|
| `diskstat-rd-sda` / `diskstat-wr-sda` | Disk I/O |
| `nic-rx-*` / `nic-tx-*` | Network I/O |
| `nic-rssi-wlp1s0` | WiFi signal strength |

**Output modes:**
- `stdout` — print values to stdout (scriptable)
- `csv` — CSV format (for automated analysis)
- On-screen overlay (default)

**Maximal GPU pipeline capture (CSV)**:
```bash
GALLIUM_HUD="csv,fps+frametime+cpu+cpu0+cpu1,\
vs-invocations+ps-invocations+cs-invocations,\
primitives-generated+ia-vertices+ia-primitives,\
sensors_temp_cu-k10temp-pci-00c3.temp1+cpufreq-cur-cpu0"
```

**Note**: GPU-specific counters (GPU-load, GPU-shaders-busy, VRAM-usage, etc.)
are available via the system Mesa but may need `r600_pipe_common.c` driver
query registration to appear in the debug build. The system Mesa 25.2.6
exposes: GPU-load, GPU-shaders-busy, GPU-ta-busy, GPU-vgt-busy, GPU-sc-busy,
GPU-db-busy, GPU-cb-busy, GPU-cp-busy, GPU-sdma-busy, GPU-scratch-ram-busy,
temperature, shader-clock, memory-clock, VRAM-usage, GTT-usage,
num-bytes-moved, num-evictions, buffer-wait-time, num-mapped-buffers.

---

## Layer 3: GPU Block Utilization — `radeontop`

**What it sees**: Real-time GPU execution engine utilization at ~120 Hz.
**Unique capability**: Shows per-block activity that GALLIUM_HUD doesn't expose
in the debug build.

### Blocks monitored

| Block | Abbreviation | What it does |
|-------|-------------|--------------|
| Graphics Pipeline | `gpu` | Overall GPU utilization |
| Event Engine | `ee` | Event/sync processing |
| Vertex Grouper/Tessellator | `vgt` | Primitive assembly |
| Texture Addresser | `ta` | Texture coordinate calculation |
| Shader Export | `sx` | Shader output routing |
| Shader/Sequencer | `sh` | Shader program execution |
| Shader Processor Interpolator | `spi` | Pixel shader interpolation |
| Scan Converter | `sc` | Rasterization |
| Primitive Assembly | `pa` | Triangle setup |
| Depth Block | `db` | Z-buffer operations |
| Color Block | `cb` | Framebuffer write |
| VRAM usage | `vram` | MB used / total |
| GTT usage | `gtt` | MB used / total |

### Flags

```bash
radeontop -d /tmp/radeontop.csv -l 100 -i 1 -t 120
#   -d: dump to file (CSV-parseable)
#   -l: limit to N lines
#   -i: dump interval in seconds
#   -t: ticks (samples/sec), default 120
```

**Maximal capture**: `radeontop -d /tmp/gpu_util.csv -i 1 -t 120 -l 30`

---

## Layer 4: CPU Microarchitecture — `perf`

**What it sees**: CPU hardware performance counters, call graphs, tracepoints.
**Nothing else provides hardware PMU data.**

### Available hardware events (E-300 Bobcat Family 14h)

| Event | What it measures |
|-------|-----------------|
| `cycles` | CPU clock cycles |
| `instructions` | Retired instructions |
| `cache-references` | LLC references |
| `cache-misses` | LLC misses |
| `branch-instructions` | Branches executed |
| `branch-misses` | Branch mispredictions |
| `stalled-cycles-frontend` | Frontend stalls (fetch/decode) |
| `stalled-cycles-backend` | Backend stalls (execution/retire) |

### Available cache events

| Event | What |
|-------|------|
| `L1-dcache-loads` / `L1-dcache-load-misses` | L1 data cache |
| `L1-icache-loads` / `L1-icache-load-misses` | L1 instruction cache |
| `L1-dcache-prefetches` / `L1-dcache-prefetch-misses` | HW prefetch |
| `LLC-loads` / `LLC-load-misses` | Last-level (L2) cache |
| `LLC-stores` | L2 store operations |
| `dTLB-loads` / `dTLB-load-misses` | Data TLB |
| `iTLB-loads` / `iTLB-load-misses` | Instruction TLB |
| `branch-loads` / `branch-load-misses` | Branch predictor |
| `node-loads` / `node-load-misses` | NUMA node (single node on E-300) |

### Available radeon kernel tracepoints

| Tracepoint | What it captures |
|------------|-----------------|
| `radeon:radeon_cs` | Command stream submission (ring, dword count) |
| `radeon:radeon_fence_emit` | GPU fence creation (ring, sequence number) |
| `radeon:radeon_fence_wait_begin` | CPU starts waiting for GPU |
| `radeon:radeon_fence_wait_end` | CPU done waiting for GPU |
| `radeon:radeon_bo_create` | Buffer object allocation |
| `radeon:radeon_vm_bo_update` | GART page table update |
| `radeon:radeon_vm_flush` | GART TLB flush |
| `radeon:radeon_vm_grab_id` | VM context acquisition |
| `radeon:radeon_vm_set_page` | Individual GART page mapping |
| `radeon:radeon_semaphore_signale` | Inter-ring semaphore signal |
| `radeon:radeon_semaphore_wait` | Inter-ring semaphore wait |

### IBS (Instruction-Based Sampling)

| Probe | Capability |
|-------|-----------|
| `ibs_fetch` | Instruction fetch: I-cache misses, ITLB misses, fetch latency |
| `ibs_op` | µop execution: D-cache misses, branch mispredicts, execution latency |

**Note**: `perf record -e ibs_op//` captures raw IBS data but `perf report`
may have format compatibility issues on xanmod kernels. Use `perf script`
for raw event extraction.

### Maximal perf recipes

**Full CPU characterization** (aggregate):
```bash
perf stat -e cycles,instructions,cache-references,cache-misses,\
L1-dcache-loads,L1-dcache-load-misses,L1-icache-load-misses,\
LLC-loads,LLC-load-misses,branches,branch-misses,\
stalled-cycles-frontend,stalled-cycles-backend,\
dTLB-load-misses,iTLB-load-misses -- <command>
```

**DRM activity** (per-event count):
```bash
perf stat -e radeon:radeon_cs,radeon:radeon_fence_emit,\
radeon:radeon_fence_wait_begin,radeon:radeon_fence_wait_end,\
radeon:radeon_bo_create,radeon:radeon_vm_bo_update,\
radeon:radeon_vm_flush,radeon:radeon_vm_set_page -a -- <command>
```

**Call graph profiling**:
```bash
perf record -g -F 997 -o /tmp/perf.data -- <command>
perf report -i /tmp/perf.data --no-children --sort=dso,symbol --stdio
```

---

## Layer 5: CPU Hotspots — `AMDuProfCLI`

**What it sees**: Timer-based hotspot profiling with call stacks.
**Unique vs perf**: AMD-specific IBS analysis engine (when supported).

### Capabilities on E-300 (Family 14h)

| Feature | Status |
|---------|--------|
| IBS Fetch | Reported YES by `info --system`, but `collect` says "not supported" |
| IBS Op | Same — reported available but collect rejects it |
| Core PMC | NO (Family 14h not in PMC model) |
| Timer/Hotspots | YES — `collect --config hotspots` |
| TBP (Time-Based) | YES — `collect --config tbp` |
| GPU Profiling | NO (requires MI200/MI300) |

### Usage

```bash
# Timer-based hotspot (the only reliable mode on E-300):
AMDuProfCLI collect --config hotspots -o /tmp/hotspots -- <command>
AMDuProfCLI report -i /tmp/hotspots/

# Time-based sampling:
AMDuProfCLI collect --config tbp -d 10 -o /tmp/tbp -- <command>
```

**Verdict**: On Family 14h, `perf` provides strictly more data than AMDuProfCLI.
Use AMDuProfCLI only if its report format is more convenient.

---

## Layer 6: DRM Kernel Tracing — `trace-cmd` / `bpftrace`

**What it sees**: Kernel-level DRM operations with nanosecond timestamps.
**Unique**: Only way to measure exact fence wait durations and CS submission timing.

### trace-cmd

```bash
# List available radeon events:
trace-cmd list -e radeon

# Record all radeon events system-wide:
sudo trace-cmd record -e 'radeon:*' -o /tmp/trace.dat &
# ... run workload ...
sudo kill -INT $PID
trace-cmd report -i /tmp/trace.dat

# Record with timestamp deltas:
trace-cmd report -i /tmp/trace.dat --ts-diff
```

### bpftrace (requires root)

```bash
# Count CS submissions per second:
sudo bpftrace -e 'tracepoint:radeon:radeon_cs { @cs = count(); } interval:s:1 { print(@cs); clear(@cs); }'

# Measure fence wait duration (ns):
sudo bpftrace -e '
  tracepoint:radeon:radeon_fence_wait_begin { @start[tid] = nsecs; }
  tracepoint:radeon:radeon_fence_wait_end { @wait_ns = hist(nsecs - @start[tid]); delete(@start[tid]); }
'

# Histogram of CS dword sizes:
sudo bpftrace -e 'tracepoint:radeon:radeon_cs { @dw = hist(args->dw); }'
```

---

## Layer 7: API Tracing — `apitrace`

**What it sees**: Every GL/VK API call with arguments, state, and timing.
**Unique**: Only tool that captures the full API-level interaction.

### Commands

| Command | Purpose |
|---------|---------|
| `apitrace trace <program>` | Capture API calls to trace file |
| `apitrace dump <file>` | Print trace as text |
| `apitrace replay <file>` | Replay trace (with glretrace) |
| `apitrace diff <a> <b>` | Compare two traces |
| `apitrace trim <file>` | Trim to specific frame range |
| `apitrace info <file>` | Print trace metadata (JSON) |
| `apitrace leaks <file>` | Check for GL object leaks |

### Maximal capture

```bash
# Full GL trace with timestamps:
apitrace trace --api gl glmark2 -s 400x300 -b shading:shading=phong

# Replay with per-call profiling:
glretrace --pcpu --pgpu <trace.trace>
#   --pcpu: per-call CPU time
#   --pgpu: per-call GPU time (via GL queries)

# Dump specific calls:
apitrace dump --calls=1000-2000 <trace.trace>
```

---

## Layer 8: Shader Correctness — `piglit`

**What it sees**: Per-shader correctness validation with ISA capture side-channel.

### Usage for r600 RE

```bash
# Run shader_test with ISA capture:
R600_DEBUG=ps,vs shader_runner <test.shader_test>

# Run full piglit suite:
piglit run gpu -o /tmp/piglit_results
piglit summary console /tmp/piglit_results
```

---

## Layer 9: Memory Debugging — `valgrind`

**What it sees**: Memory errors, leaks, cache simulation.

### Callgrind (CPU cache simulation)

```bash
# Simulate L1i/L1d/L2 cache behavior:
valgrind --tool=callgrind --cache-sim=yes --branch-sim=yes \
  --I1=32768,8,64 --D1=32768,8,64 --LL=524288,16,64 \
  <command>

# Visualize:
callgrind_annotate callgrind.out.<pid>
```

The cache parameters above match the E-300: 32KB L1i (8-way, 64B line),
32KB L1d (8-way, 64B line), 512KB L2 (16-way, 64B line).

---

## Layer 10: Microbenchmarking — `hyperfine`

**What it sees**: Statistical wall-clock timing with warmup and variance.

```bash
# Compare GL vs VK performance:
hyperfine --warmup 2 --min-runs 5 \
  'LIBGL_DRIVERS_PATH=... glmark2 -s 100x100 -b shading:shading=phong' \
  'VK_ICD_FILENAMES=... vkmark --winsys headless'
```

---

## Synergy Matrix

How tools feed into each other:

```
R600_DEBUG (ISA)
  ├──→ ISA_SLOT_ANALYSIS.md (VLIW5 utilization, slot fill histogram)
  ├──→ LATENCY_PROBE_RESULTS.md (PV forwarding confirmation)
  └──→ BUILD_DECISION_MATRIX.md (compiler vs manual pattern)

GALLIUM_HUD (pipeline counters)
  ├──→ HW_BOUNDARY_MEASUREMENTS.md (GPU utilization under load)
  ├──→ FULL_STACK_PROFILE_MESA26.md (cross-stack comparison)
  └──→ Confirms GPU idle time (feeds CPU optimization priority)

radeontop (block utilization)
  ├──→ Real-time GPU activity during profiled workloads
  └──→ Cross-validates GALLIUM_HUD GPU-load numbers

perf stat (CPU counters)
  ├──→ IPC, cache miss rate, branch misprediction (THE CPU diagnosis)
  ├──→ radeon tracepoints (CS submission rate, fence wait count)
  └──→ Feeds priority ranking for optimization (CPU > GPU)

perf record (call graph)
  ├──→ Flat profile (top functions by CPU time)
  ├──→ Call tree (who calls the hot functions)
  └──→ Flamegraph generation (visual bottleneck identification)

trace-cmd / bpftrace (DRM kernel)
  ├──→ Fence wait duration histogram (GPU→CPU sync cost)
  ├──→ CS submission rate (kernel mode transition overhead)
  └──→ GART page table activity (memory management overhead)

apitrace (API calls)
  ├──→ Per-call CPU/GPU time (apitrace replay --pcpu --pgpu)
  ├──→ Draw call batching analysis
  └──→ State change frequency (redundant state elimination)

valgrind callgrind (cache simulation)
  ├──→ Per-function cache miss attribution
  ├──→ Branch prediction simulation
  └──→ Validates perf stat aggregate numbers at function level
```

## Maximal Single-Session Capture Recipe

Run all non-interfering tools simultaneously for a single glmark2 benchmark:

```bash
export DISPLAY=:0 && xhost +local: 2>/dev/null
export LIBGL_DRIVERS_PATH=/usr/local/mesa-debug/lib/x86_64-linux-gnu/dri
export LD_LIBRARY_PATH=/usr/local/mesa-debug/lib/x86_64-linux-gnu
export vblank_mode=0

# Terminal 1: radeontop (GPU block utilization)
radeontop -d /tmp/session_gpu.csv -i 1 -t 120 -l 15 &

# Terminal 2: perf stat (CPU + DRM tracepoints)
perf stat -e cycles,instructions,cache-misses,cache-references,\
L1-dcache-load-misses,L1-icache-load-misses,branches,branch-misses,\
stalled-cycles-frontend,stalled-cycles-backend,\
radeon:radeon_cs,radeon:radeon_fence_emit,\
radeon:radeon_fence_wait_begin,radeon:radeon_fence_wait_end \
-a -o /tmp/session_perf.txt -- timeout 12 \

# Terminal 3: GALLIUM_HUD (pipeline counters to stdout)
GALLIUM_HUD="csv,fps+frametime+cpu,\
vs-invocations+ps-invocations+primitives-generated" \
R600_DEBUG=ps,precompile \
glmark2 -s 400x300 -b shading:shading=phong \
  2>/tmp/session_isa.log | tee /tmp/session_hud.csv

# Post-processing:
# Parse ISA from session_isa.log (R600_DEBUG output)
# Parse HUD CSV from session_hud.csv
# Parse perf stats from session_perf.txt
# Parse GPU utilization from session_gpu.csv
```

## What's NOT Available (Wrong Generation)

| Tool | Why it doesn't work |
|------|-------------------|
| AMDuProfCLI GPU profiling | Requires MI200/MI300 accelerators |
| AMDuProfCLI Core PMC | Family 14h not in PMC event model |
| AMDuProfCLI IBS (collect) | Reports available but rejects on collect |
| GPUPerfAPI | Requires Graphics IP 10+ (GCN minimum) |
| amdgpu_top | Requires amdgpu kernel driver |
| NVTOP AMD backend | Requires amdgpu driver |
| UMR | Requires amdgpu driver |
| ROCm / HIP | Wrong architecture entirely |
| renderdoc GPU counters | May work for API replay but no GPU perf counters |
