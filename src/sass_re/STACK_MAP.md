# Cross-Architecture Stack Map

This living document keeps the repo's major architecture tracks aligned around
one question: what evidence changes a build decision, and which wrapper or
artifact family produces it?

Use this map as the side-by-side entrypoint for the current core tracks and as
the landing zone for future architecture slots.

## Living tracks

| Track | Scope | Canonical entrypoints | Core tool stack | Decision artifacts | Status |
|-------|-------|-----------------------|-----------------|-------------------|--------|
| SASS / SM89 | Ada Lovelace SASS RE, inline PTX, instant-NGP hot loops | [`README.md`](README.md), [`RESULTS.md`](RESULTS.md), [`Thought_Processes.md`](Thought_Processes.md) | `nvcc`, `ptxas`, `cuobjdump`, `nvdisasm`, `ncu`, `nsys`, `hyperfine`, `objdump` | mnemonic tables, latency and throughput results, claims matrix, synthesis notes | mature |
| Apple CPU + Metal + Neural | macOS CPU latency/cache, Metal hardware counter capture, Core ML / MPS / MLX placement | [`../apple_re/scripts/run_apple_tranche1.sh`](../apple_re/scripts/run_apple_tranche1.sh), [`../apple_re/scripts/run_next42_cpu_probes.sh`](../apple_re/scripts/run_next42_cpu_probes.sh), [`../apple_re/scripts/run_next42_cpu_cache_probes.sh`](../apple_re/scripts/run_next42_cpu_cache_probes.sh), [`../apple_re/scripts/run_next42_metal_probes.sh`](../apple_re/scripts/run_next42_metal_probes.sh), [`../apple_re/scripts/run_next42_neural_suite.sh`](../apple_re/scripts/run_next42_neural_suite.sh) | `xcodebuild`, `xcrun`, `xctrace --instrument 'Metal GPU Counters'`, `dtrace`, `fs_usage`, `sample`, `leaks`, `vmmap`, `powermetrics`, `llvm-mca`, `lldb`, `xcrun dyld_info`, `.venv`, `torch`, `mlx`, `jax-metal`, `coremltools` | `gpu_hardware_counters.csv` (ALU 79% peak, TBDR compute routing confirmed), `xctrace_row_density.csv`, `counter_latency_report.md`, `cache_pressure.csv`, `cpu_runs/*.csv`, `library_mnemonic_mining.md` (AMX discovery), `KEEPALIVE_SUMMARY.md` | active (all roadmap gaps closed) |
| Ryzen 5600X3D | Zen 3 execution behavior, cache hierarchy, 3D V-Cache boundaries | [`FRONTIER_ROADMAP_RYZEN_5600X3D.md`](FRONTIER_ROADMAP_RYZEN_5600X3D.md), [`../../docs/sass/RYZEN_5600X3D_RE_GUIDE.md`](../../docs/sass/RYZEN_5600X3D_RE_GUIDE.md) | `perf`, `uProf`, `objdump`, `llvm-mca`, sanitizers, optional `valgrind` | cache sweep tables, compiler-vs-manual ledger, runtime and spill summaries | scoped |
| CUDA LBM | LBM kernel inventory, benchmark matrix, Nsight profiling, and disassembly | [`../cuda_lbm/README.md`](../cuda_lbm/README.md), [`../cuda_lbm/scripts/run_cuda_decision_tranche.sh`](../cuda_lbm/scripts/run_cuda_decision_tranche.sh), [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md) | `cmake`, `nvcc`, `ncu`, `nsys`, `cuobjdump`, `nvdisasm`, `hyperfine` | `cuda_variant_matrix.csv`, `cuda_launch_health.csv`, `cuda_occupancy_vs_bw.csv`, `cuda_decision_summary.md` | active |
| r600 / TeraScale-2 | AMD Radeon HD 6310 Evergreen VLIW5 RE, Mesa 26 driver optimization, Terakan Vulkan | [`FRONTIER_ROADMAP_R600_TERASCALE.md`](FRONTIER_ROADMAP_R600_TERASCALE.md), `~/eric/TerakanMesa/findings/` | `R600_DEBUG`, `GALLIUM_HUD`, `AMDuProfCLI`, `radeontop`, `apitrace`, `perf`, `gdb`, `trace-cmd` | ISA slot analysis, pipeline utilization (62-75% idle), stutter RCA (2.1× fix), 426-opcode inventory, GL 148 FPS / VK 619 FPS baseline, perf flamegraph, IBS profiling, DRM fence tracing | active |
| Future arch slot 1 | TBD | TBD | TBD | TBD | placeholder |

## How to use this map

- Start from the canonical entrypoints for the track you are touching.
- Follow the decision artifacts, not just the raw captures.
- Keep the evidence normalized so track-to-track comparisons stay meaningful.
- Treat the placeholder rows as open slots for the next architecture family we
  decide to formalize.

## Companion docs

- [`BUILD_DECISION_MATRIX.md`](BUILD_DECISION_MATRIX.md) for the compiler-vs-manual ledger
- [`APPLE_SILICON_RE_BRIDGE.md`](APPLE_SILICON_RE_BRIDGE.md) for the Apple translation of the SASS method
- [`APPLE_TRACK_GAP_ANALYSIS.md`](APPLE_TRACK_GAP_ANALYSIS.md) for the Apple vs. CUDA methodology gap analysis
- [`FRONTIER_ROADMAP_APPLE.md`](FRONTIER_ROADMAP_APPLE.md) for the Apple runbook
- [`FRONTIER_ROADMAP_RYZEN_5600X3D.md`](FRONTIER_ROADMAP_RYZEN_5600X3D.md) for the Ryzen runbook
- [`../apple_re/README.md`](../apple_re/README.md) for the Apple subtree overview
- [`../cuda_lbm/README.md`](../cuda_lbm/README.md) for the CUDA LBM inventory and results
