# Apple M-series CPU — Integer Multiply Latency

- date: 2026-04-01
- machine: Apple M1, 16 GB, arm64, ~3.2 GHz
- tool: `apple_cpu_latency` (32-deep dependent-chain probes, 2M iters)
- build: `clang -std=gnu11 -O2`, inline asm dep chains on aarch64
- comparison: `llvm-mca --mcpu=apple-m1 -march=aarch64 -iterations=100`

## Measured latency — integer multiply family

| Probe | Instruction | ns/op | cyc/op @3.2GHz | MCA cyc/op | MCA error |
|-------|-------------|-------|---------------|------------|-----------|
| `add_dep_u64` | ADD | 0.382 | ~1.2 | ~1.0 | ±20% (overhead) |
| `mul_dep_u64` | MUL (MADD xzr) | **0.950** | **~3.0** | ~5.0 | +67% over-predict |
| `madd_dep_u64` | MADD (acc feeds Xa+Xn) | **0.963** | **~3.1** | ~5.0 | +63% over-predict |
| `msub_dep_u64` | MSUB (acc feeds Xa−Xn) | **0.961** | **~3.1** | ~5.0 | +63% over-predict |
| `umulh_dep_u64` | UMULH (upper 64 of 64×64) | **0.949** | **~3.0** | ~5.0 | +67% over-predict |
| `smull_dep_i32_to_i64` | SMULL (32×32→64) | **0.943** | **~3.0** | ~5.0 | +67% over-predict |

## Key findings

### 1. All integer multiply variants: 3-cycle latency

MUL, MADD, MSUB, UMULH, and SMULL are all **~3 cycles** on M-series Apple Silicon.
No penalty for:
- Upper-half 64-bit result (UMULH same as MUL)
- 32-bit signed multiply long (SMULL same as 64-bit MUL)
- Fused multiply-add / multiply-subtract (MADD/MSUB same as MUL)

This is consistent with a unified integer multiply unit that handles all integer
multiply variants in a single pipeline at the same 3-cycle latency.

### 2. MCA over-predicts integer multiply latency by 60–70%

llvm-mca `--mcpu=apple-m1` predicts ~5 cycles for all integer multiply variants.
Measured is 3 cycles. MCA is **wrong by 60–70%** for integer multiply chains.

This adds to the MCA reliability taxonomy:

| Category | MCA accuracy | Use MCA? |
|----------|-------------|----------|
| Simple integer ADD/SUB | ±20% (overhead-limited) | **Yes, with caution** |
| Integer MUL/MADD/MSUB/UMULH/SMULL | **60–70% over-predict** | **No** |
| f64 FMADD | −23% under-predict | **Partial** |
| Variable-shift (bswap+shift chain) | 3× error | **No** |
| Atomic add (library call) | Cannot model | **No** |
| sin/cos (libm) | 8× under-predict | **No** |

### 3. UMULH not slower than MUL — use for 128-bit and Montgomery arithmetic

The upper-half multiply UMULH (which computes the upper 64 bits of a 128-bit
product) has the same 3-cycle latency as regular MUL. This means:

- 128-bit multiplication via `MUL` + `UMULH` = **3 cycles** (limited by UMULH
  since MUL result is not needed by UMULH — they are independent in practice)
- Montgomery reduction, Barrett reduction, and GHASH via UMULH incur no extra
  latency penalty over pure MUL chains
- Use UMULH freely — no "expensive" special-case avoidance needed

### 4. SMULL (32×32→64) not slower than 64-bit MUL

SMULL with 32-bit inputs and a 64-bit result is the same 3 cycles as a 64-bit MUL.
This confirms a single multiply unit shared between the 32-bit and 64-bit multiply
variants — no downgrade when using the Wn operand form.

## Disassembly verification (objdump)

```
# MUL dep chain (excerpt):
# apple_cpu_latency -O2, bench_mul_dep inner loop:
#   mul  x8, x8, x9     <- 32 of these back-to-back
#   mul  x8, x8, x9
#   ...

# MADD dep chain:
#   madd x8, x8, x9, x8  <- Xd = X8*X9 + X8 (acc feeds both multiplicand and addend)
#   madd x8, x8, x9, x8
#   ...

# UMULH dep chain:
#   umulh x8, x8, x9     <- upper 64 bits of x8*x9
#   umulh x8, x8, x9
#   ...

# SMULL dep chain:
#   smull x8, w8, w9     <- x8 = sign_extend(w8) * sign_extend(w9)
#   smull x8, w8, w9
#   ...
```

## llvm-mca raw output (apple-m1, 100 iterations)

```
# MUL chain (16 ops × 100 iters):
Total Cycles:      8003     → 80 cycles/iter / 16 ops = 5.0 cyc/op  WRONG (measured: 3)
IPC:               0.21
Block RThroughput: 17.0

# UMULH chain (16 ops × 100 iters):
Total Cycles:      8003     → 80 cycles/iter / 16 ops = 5.0 cyc/op  WRONG (measured: 3)
IPC:               0.21
Block RThroughput: 17.0
```

## Build decision implication

| Use case | Decision |
|----------|----------|
| 64-bit multiply-intensive loops | MUL latency = 3 cyc; throughput can exceed 1/cyc with independent chains |
| 128-bit arithmetic (crypto, finance) | MUL + UMULH in parallel: effective latency 3 cyc per pair |
| Polynomial evaluation (Horner scheme) | Each step = 1 MADD = 3 cyc; no benefit of manual MADD vs MUL+ADD |
| Cost estimate from llvm-mca | **Never use MCA for multiply cost** — 67% over-estimate |
| SMULL for 32-bit input workloads | Same cost as 64-bit MUL — use freely without routing around |
