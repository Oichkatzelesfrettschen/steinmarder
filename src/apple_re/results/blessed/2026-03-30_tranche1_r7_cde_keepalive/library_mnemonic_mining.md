# Apple Framework Library Mnemonic Mining

- date: 2026-04-01
- machine: Apple M1, 16 GB, macOS 15 (Sequoia / arm64e)
- method: `lldb` runtime disassembly from dyld shared cache + `sample` profiling
- libraries: Accelerate/libvDSP.dylib, Metal.framework (stub only)
- context: Mining real-world instruction usage to validate probe corpus coverage

## Approach and Constraints

Metal.framework, Accelerate.framework, and MPSCore.framework on macOS 13+
reside **only in the dyld shared cache** at
`/System/Volumes/Preboot/Cryptexes/OS/System/Library/dyld/dyld_shared_cache_arm64e`.
They are not present as on-disk `.dylib` files; `otool -tv` fails with "No such file".

**Working method**:
1. `xcrun dyld_info -exports <dylib_path>` — lists exported symbol names (works from shared cache)
2. Build a test binary, run it, profile with `sample`, identify hot code offsets
3. `lldb --batch --source` — disassemble at runtime addresses within the shared cache mapping

## Critical Finding: Accelerate Uses AMX, Not NEON

The `vDSP_vadd` hot loop contains **53 AMX instructions** out of ~300 total instructions
in the function body. Zero standard NEON float instructions (`fadd`, `fmul`, `fmla`,
`fmax`, `fmin`) appear in the hot compute path.

### AMX (Apple Matrix Extension)

AMX is Apple's proprietary co-processor extension, exposed at the ISA level via
an undocumented opcode space: `0x00201000`–`0x00201FFF`. It provides:

| Opcode range | Operation | Count in vDSP_vadd |
|-------------|-----------|-------------------|
| `0x00201000`–`0x0020103F` | `ldx` variants (load X register rows from memory) | 8 |
| `0x00201080`–`0x002010BF` | `ldy` variants (load Y register from memory) | 11 |
| `0x00201180`–`0x002011FF` | Compute: `fma32` / `fma64` (matrix multiply-accumulate) | 10 |
| `0x00201220`–`0x00201221` | Stop/exit AMX mode | 2 |

Distinct AMX opcodes observed in `vDSP_vadd`:
```
0x0020100b, 0x0020100c, 0x00201010, 0x00201013, 0x00201018
0x0020108b, 0x0020108c, 0x0020108d, 0x0020108e
0x00201090, 0x00201091, 0x00201093, 0x00201095, 0x00201098
0x002010a8, 0x002010a9, 0x002010aa
0x00201180, 0x00201182, 0x00201183, 0x00201185, 0x00201186,
0x00201187, 0x0020118a, 0x0020118b, 0x0020118f, 0x00201190
0x00201220, 0x00201221
```

### Non-AMX instructions in the hot path

Alongside the AMX instructions, the hot path contains:
- `bfxil` — bit field insert low (pointer encoding/alignment for AMX addresses)
- `movk x, #N, lsl #48` — pointer packing (high-bit manipulation for AMX arg encoding)
- `cmp` + `b.ne` / `b.lo` / `b.hs` — dispatch checks (stride=1, size thresholds)
- `stp` / `ldp` / `retab` — standard function prologue/epilogue with PAC-protected return
- `add` / `sub` / `and` / `orr` — address arithmetic for AMX buffer arguments

**Notable absence**: No `fadd .4s`, `fmul .4s`, `fmla .4s`, `ld1`, `st1` NEON instructions
in the hot compute region. AMX handles all vectorized float arithmetic.

### How AMX is invoked

The function starts with dispatch logic checking `stride == 1` for all three buffers
and `n >= 0x5000` (20480 elements) before entering the AMX path. For smaller inputs
or non-unit strides, it falls back to a different code path (likely NEON-based at
offset `+8`, which contains the `eor x16, x30, x30, lsl #1` branch).

```
vDSP_vadd entry:
  cmp x6, #0x5000        ; n >= 20480?
  b.hs <AMX_path>        ; yes → use AMX
  <non-AMX path>         ; no → stride check + scalar/NEON fallback
```

## Symbol Inventory (API Surface)

### libvDSP.dylib — 471 exported symbols

Key function families (from `dyld_info -exports`):
- `vDSP_vadd`, `vDSP_vsub`, `vDSP_vmul`, `vDSP_vdiv` — vector arithmetic
- `vDSP_DFT_Execute`, `vDSP_DFT_ExecuteD` — FFT (f32/f64)
- `vDSP_DCT_Execute` — discrete cosine transform
- `vDSP_mmul`, `vDSP_mmulD` — matrix multiplication
- `vDSP_conv`, `vDSP_convD` — convolution
- `vDSP_sve`, `vDSP_svesq` — sum and sum-of-squares
- `vDSP_vclr`, `vDSP_vfill` — vector zero/fill

### libBLAS.dylib — 1,466 exported symbols

Primary families:
- `cblas_sgemm`, `cblas_dgemm` — f32/f64 general matrix multiply
- `cblas_strsm`, `cblas_dtrsm` — triangular solve
- `cblas_snrm2`, `cblas_sdot` — vector norms and dot products

### libBNNS.dylib — 415 exported symbols

Neural network primitives:
- `BNNSComputeInferenceLayer` — layer-level inference dispatch
- `BNNSFilterApplyBatch`, `BNNSFilterApplyBatchTwoInput` — batch application
- `BNNSCreateFilterConvolutionLayer`, `BNNSCreateFilterFullyConnectedLayer` — layer creation

### MPSCore.framework — 566 exported symbols

Metal Performance Shaders C++ classes (mangled names). Key categories:
- `MPSKernelEncodeToCommandBuffer` (base encoder interface)
- `MPSCNNConvolution*`, `MPSCNNFullyConnected*` — convolutional layers
- `MPSMatrix*` — Metal matrix operations
- `MPSImageConversion*` — dtype/format conversion kernels

## Non-AMX Instructions Observed (AArch64 / PAC)

In function prologues and dispatch code:
| Instruction | Context | Notes |
|-------------|---------|-------|
| `retab` | Function epilogue | PAC return (A-key authentication on return address) |
| `bfxil Xd, Xn, #0, #N` | AMX arg encoding | Bit field insert into pointer-packed AMX argument |
| `movk X, #N, lsl #48` | AMX arg encoding | High-byte pointer packing for AMX buffer address encoding |
| `tbz x, #0x3e, <L>` | PAC check | Test bit 62 (PAC bit) before retab |
| `cbz / cbnz` | Null/zero dispatch | Size/pointer zero-checks before AMX entry |
| `stp x29, x30, [sp, ...]` | Function prologue | Standard AArch64 frame setup |
| `ldp x29, x30, [sp], ...` | Function epilogue | Frame teardown before retab |

## Implication for Probe Coverage

| Our probe | What it measures | What Accelerate uses |
|-----------|----------------|---------------------|
| `bench_fmadd_dep_f64` | NEON FMADD latency (4 cyc) | **NOT what vDSP uses** |
| `bench_fmla_f16x8_simd` | NEON FMLA .8h latency (4 cyc) | **NOT what vDSP uses** |
| AMX (undocumented) | — NOT PROBED — | **What vDSP actually uses for n≥20480** |

**Gap**: Our entire probe suite measures the NEON FP pipeline. Apple's Accelerate
library bypasses NEON entirely for large vector operations, using the AMX co-processor
instead. AMX latency and throughput are NOT measured by any current probe.

**AMX is not programmable via standard C/asm on macOS without reverse-engineered
instruction encodings**. The `libAMX.dylib` and AMX headers are not publicly documented.
Projects like `corsix/amx` (GitHub) have reverse-engineered the encoding. Probing AMX
would require raw instruction encoding in inline asm (same approach as our existing
probes) using the community-documented AMX opcode map.

## Build Decision: Do NOT Implement Manual NEON to Match Accelerate Performance

Accelerate's vDSP_vadd (n≥20480) uses AMX, not NEON. Writing NEON FMLA loops to
compete with Accelerate on large vectors is futile — AMX has wider data paths and
higher throughput than NEON for matrix/vector workloads.

**Use Accelerate/BLAS for any large (n>20K) CPU-side float array operations.**
For n<20K (fallback path) or Metal GPU operations, NEON and FP16 probes apply.
