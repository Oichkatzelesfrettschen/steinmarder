# Apple M-series Cache Hierarchy Analysis

- date: 2026-04-01
- machine: Apple M-series (arm64), ~3.2 GHz boost
- compiler: clang -O2 (arm64-apple-macosx)
- probe: apple_cpu_cache_pressure.c
- working-set range: 4 KB – 32 MB
- stride: 64 bytes (cache line size)

## Summary: Cache Tier Boundaries

| Tier | Size range | Stream ns/access | Stream cyc/access | Interpretation |
|------|-----------|-----------------|-------------------|----------------|
| L1 + L2 | ≤ 128 KB | 0.65–0.82 ns | 2.1–2.6 cyc | Fast tier; hardware prefetcher saturates bandwidth |
| LLC (SLC) | 256 KB – 8 MB | 0.95–1.02 ns | 3.0–3.3 cyc | System-Level Cache; ~50% slower than L2 |
| LLC edge / DRAM mix | 16 MB | 1.15 ns | 3.7 cyc | SLC capacity nearly exhausted |
| DRAM | 32 MB | 2.28 ns | 7.3 cyc | Full DRAM latency visible in bandwidth |

Knee points observed at:
- **~128 KB → 256 KB**: stream ns/access jumps from 0.654 to 0.972 (+49%). L1/L2 → SLC transition.
- **~8 MB → 16 MB**: stream ns/access rises from 1.020 to 1.153 (+13%). SLC ceiling.
- **~16 MB → 32 MB**: stream ns/access jumps from 1.153 to 2.278 (+98%). SLC → DRAM.

These boundaries are consistent with published M-series cache sizes:
- M1/M2/M3 P-core: 128–192 KB L1D + 16–32 MB SLC (System-Level Cache, shared across CPU+GPU)

## Family Descriptions

### `stream` — sequential read-modify-write bandwidth

```c
for (i = 0; i < elems; i += stride_elems) {
    acc ^= buf[i] + i + pass;
    buf[i] = acc + buf[i] + i;
}
```

Measures **memory bandwidth** per access. Hardware prefetcher handles the sequential
stride and hides latency. Results reflect bandwidth, not load-use latency.
**This family shows the clearest cache hierarchy boundaries.**

### `pointer` — sequential stride linked-list chain

```c
buf[i] = (i + stride_elems) % elems;   // init: sequential stride chain
idx = buf[idx];                          // chase
```

Intended to measure **pointer-chase latency**. However, the chain advances by a
fixed stride (64 bytes = 8 elements), which the M-series hardware prefetcher
predicts trivially. Results are flat (1.45–1.98 ns) across all working-set sizes,
showing NO cache hierarchy — the prefetcher completely hides miss latency.

**Conclusion: this family does NOT measure cache latency on M-series.** The
prefetcher wins. True latency measurement requires a **random permutation** of the
pointer chain (Fisher-Yates shuffle at init) so that `buf[idx]` is only known after
the load completes. A sequential stride chain is indistinguishable from a stride
prefetch pattern for the hardware prefetcher.

### `reuse` — stride-inner, pass-outer pattern

The reuse family iterates with an inner loop that walks the stride positions within
each element before advancing to the next. Results are flat at ~3.1–3.8 ns/access
(10–12 cycles) across all sizes — slower than stream but with no hierarchy signal.
The dominant cost is the per-access arithmetic and the write-back, not memory latency.

## Key Measurements

### Stream bandwidth by tier

| Working set | ns/access | cyc/access | tier |
|------------|-----------|------------|------|
| 4 KB | 0.820 | 2.62 | L1 |
| 64 KB | 0.673 | 2.15 | L1/L2 |
| 128 KB | 0.654 | 2.09 | L2 peak |
| 256 KB | 0.972 | 3.11 | SLC (jump!) |
| 1 MB | 0.975 | 3.12 | SLC |
| 8 MB | 1.020 | 3.26 | SLC |
| 16 MB | 1.153 | 3.69 | SLC edge |
| 32 MB | 2.278 | 7.29 | DRAM |

### Latency reference (from apple_cpu_latency.c, llvm_mca_analysis.md)

The `load_store_chain_u64` probe (load + XOR + shift + store chain) gave **1.18 cycles/op**
measured — lower than even L1 stream bandwidth. This confirms the arithmetic masks
memory latency: the dependent arithmetic dominates the critical path, not the load.

True M-series L1 load-use latency is published as **~4 cycles** (integer, L1D hit).
This probe does not measure it directly. To isolate pure load latency, use a
random pointer-chase (random permutation init) in `apple_cpu_cache_pressure.c`.

## Implications for Metal GPU probes

| CPU finding | GPU analog | Implication |
|------------|-----------|-------------|
| L2 bandwidth boundary at ~128 KB | AGX L1 tile cache | TGSM probe (probe_tgsm_lat.metal) should fit in tile cache |
| SLC bandwidth 3.0–3.3 cyc at 256 KB–8 MB | AGX SLC shared with GPU | Metal texture cache miss goes to same SLC |
| DRAM at 7.3 cyc stream (bandwidth) | GPU DRAM bandwidth | GPU shaders in DRAM-bound regime will see >50 cycle latency |
| Sequential stride fully prefetchable | GPU TEX cache prefetch | Texture sampler prefetches linear patterns; random sampling needed for miss measurement |
| Hardware prefetcher hides all stream latency | AGX prefetcher | Pointer-chase Metal probe (probe_tgsm_lat) must use random access pattern, not sequential |

## Methodology Notes

- All measurements use `mach_absolute_time()` via `apple_re_time.h` for cycle-accurate timing
- Working set allocated with `posix_memalign(64)` for cache-line alignment
- stride_bytes = 64 (one cache line per access)
- For stream: passes = 50; for pointer: passes = 200 (more accesses needed for stability)
- Timing excludes buffer initialization
- Results stable to ±5% across runs (verified by re-running 4 KB and 32 MB cases)

## Missing: Random Pointer-Chase

The critical missing measurement is a **random pointer-chase** to isolate cache
miss penalty. Required modification to `apple_cpu_cache_pressure.c`:

```c
// Replace sequential init with Fisher-Yates random permutation:
for (size_t i = elems - 1; i > 0; --i) {
    size_t j = (size_t)arc4random_uniform((uint32_t)(i + 1));
    uint64_t tmp = buf[i]; buf[i] = buf[j]; buf[j] = tmp;
}
```

Expected results with random pointer-chase on M1/M2/M3:
- L1D hit (≤ 128 KB): ~4 cycles load-use latency
- L2 hit (128 KB–few MB): ~12 cycles
- SLC hit (up to ~32 MB): ~30–40 cycles
- DRAM miss (> SLC): ~100–200 cycles

These would match the reference latencies from published M-series ISA documentation
and complement the stream bandwidth measurements above.
