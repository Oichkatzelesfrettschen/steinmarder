# Apple Tranche Mnemonic Interpretation

- run_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive`
- cpu_disassembly_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/blessed/2026-03-30_tranche1_r7_cde_keepalive/disassembly`

## CPU (AArch64) mnemonic frontier

- No CPU mnemonics parsed from disassembly files.

Interpretation:
- Integer/branch/control lane is clearly active (`add`, `subs`, `cmp`, `cbz/cbnz`, `b`).
- Floating-point lane is visible (`fadd`, `fmadd`, `fmov`) and matches probe intent.
- Load/store traffic (`ldr`, `str`, `ldp`, `stp`) confirms memory/control scaffolding around probe loops.

## Metal AIR opcode frontier

Top observed AIR-level operations:
- `add`: 26
- `xor`: 20
- `shl`: 13
- `zext`: 11
- `getelementptr`: 11
- `br`: 10
- `phi`: 9
- `store`: 8
- `mul`: 8
- `and`: 6
- `ret`: 5
- `icmp`: 5
- `load`: 5
- `tail_call`: 4
- `lshr`: 4
- `tail`: 4
- `or`: 3
- `fmul`: 2
- `fadd`: 1

Interpretation:
- Current GPU disassembly surface is AIR/LLVM-style IR (not private hardware ISA).
- This run confirms semantic ops (`shl`, `xor`, `add`, `store`) and sync barrier calls (`air.wg.barrier`).
- Apple evidence should remain counter/timing-led (xctrace + power + host captures) with AIR op checks as semantic guardrails.

## CUDA-method translation status

- CUDA SASS lane analog is now split into: AArch64 mnemonics + Metal AIR ops + xctrace metric exports.
- Root-only host instrumentation (`dtruss`, `powermetrics`, `spindump`) is captured in this run.
- This is CUDA-grade methodology transfer, without claiming CUDA/SASS equivalence on Apple GPU ISA.
