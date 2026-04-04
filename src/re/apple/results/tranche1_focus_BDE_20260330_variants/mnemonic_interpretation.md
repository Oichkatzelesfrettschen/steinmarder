# Apple Tranche Mnemonic Interpretation

- run_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/tranche1_focus_BDE_20260330_variants`
- cpu_disassembly_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/tranche1_focus_BDE_20260330_variants/disassembly`

## CPU (AArch64) mnemonic frontier

Top observed mnemonics (aggregated):
- `add`: 693
- `mov`: 486
- `adrp`: 329
- `bl`: 296
- `str`: 291
- `ldr`: 262
- `stp`: 252
- `fadd`: 217
- `fmadd`: 193
- `b`: 170
- `ldp`: 163
- `movk`: 108
- `cbnz`: 99
- `ldadd`: 97
- `fmov`: 89
- `cbz`: 78
- `eor`: 77
- `ldur`: 70
- `ror`: 65
- `fdiv`: 60

Interpretation:
- Integer/branch/control lane is clearly active (`add`, `subs`, `cmp`, `cbz/cbnz`, `b`).
- Floating-point lane is visible (`fadd`, `fmadd`, `fmov`) and matches probe intent.
- Load/store traffic (`ldr`, `str`, `ldp`, `stp`) confirms memory/control scaffolding around probe loops.

## Metal AIR opcode frontier

Top observed AIR-level operations:
- `add`: 11
- `xor`: 10
- `shl`: 7
- `zext`: 6
- `getelementptr`: 6
- `br`: 6
- `store`: 5
- `tail_call`: 3
- `ret`: 3
- `mul`: 3
- `phi`: 3
- `and`: 3
- `icmp`: 3
- `load`: 3
- `lshr`: 2

Interpretation:
- Current GPU disassembly surface is AIR/LLVM-style IR (not private hardware ISA).
- This run confirms semantic ops (`shl`, `xor`, `add`, `store`) and sync barrier calls (`air.wg.barrier`).
- Apple evidence should remain counter/timing-led (xctrace + power + host captures) with AIR op checks as semantic guardrails.

## CUDA-method translation status

- CUDA SASS lane analog is now split into: AArch64 mnemonics + Metal AIR ops + xctrace metric exports.
- Root-only host instrumentation (`dtruss`, `powermetrics`, `spindump`) is captured in this run.
- This is CUDA-grade methodology transfer, without claiming CUDA/SASS equivalence on Apple GPU ISA.
