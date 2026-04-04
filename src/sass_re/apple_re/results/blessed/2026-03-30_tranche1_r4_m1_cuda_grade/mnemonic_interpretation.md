# Apple Tranche Mnemonic Interpretation

- run_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/tranche1_raw_20260330_m1_cuda_grade_r4`
- cpu_disassembly_dir: `/Users/eirikr/Documents/GitHub/steinmarder/src/apple_re/results/tranche1_raw_20260330_m1_cuda_grade_r4/disassembly`

## CPU (AArch64) mnemonic frontier

Top observed mnemonics (aggregated):
- `add`: 252
- `mov`: 129
- `fadd`: 128
- `fmadd`: 128
- `adrp`: 106
- `bl`: 94
- `ldr`: 77
- `stp`: 72
- `b`: 66
- `str`: 55
- `cbnz`: 49
- `ldp`: 49
- `ldur`: 33
- `fmov`: 31
- `cmp`: 29
- `cbz`: 25
- `ucvtf`: 24
- `stur`: 22
- `fdiv`: 20
- `movi`: 20

Interpretation:
- Integer/branch/control lane is clearly active (`add`, `subs`, `cmp`, `cbz/cbnz`, `b`).
- Floating-point lane is visible (`fadd`, `fmadd`, `fmov`) and matches probe intent.
- Load/store traffic (`ldr`, `str`, `ldp`, `stp`) confirms memory/control scaffolding around probe loops.

## Metal AIR opcode frontier

Top observed AIR-level operations:
- `shl`: 2
- `xor`: 2
- `tail_call`: 1
- `add`: 1
- `zext`: 1
- `getelementptr`: 1
- `store`: 1
- `ret`: 1

Interpretation:
- Current GPU disassembly surface is AIR/LLVM-style IR (not private hardware ISA).
- This run confirms semantic ops (`shl`, `xor`, `add`, `store`) and sync barrier calls (`air.wg.barrier`).
- Apple evidence should remain counter/timing-led (xctrace + power + host captures) with AIR op checks as semantic guardrails.

## CUDA-method translation status

- CUDA SASS lane analog is now split into: AArch64 mnemonics + Metal AIR ops + xctrace metric exports.
- Root-only host instrumentation (`dtruss`, `powermetrics`, `spindump`) is captured in this run.
- This is CUDA-grade methodology transfer, without claiming CUDA/SASS equivalence on Apple GPU ISA.
