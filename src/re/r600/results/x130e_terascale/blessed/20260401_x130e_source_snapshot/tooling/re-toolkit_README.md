# r600 / TeraScale-2 Reverse Engineering Toolkit

Iterative profiling and ISA inspection toolkit for the AMD Radeon HD 6310
(Wrestler die, TeraScale-2 / VLIW5 architecture) on ThinkPad X130e.

Modeled after the steinmarder ncu/nsys workflow but for pre-GCN hardware.

---

## Hardware

| Item | Value |
|------|-------|
| GPU | AMD Radeon HD 6310 (Wrestler) |
| Architecture | TeraScale-2 / VLIW5 |
| Mesa driver | r600g (Gallium) |
| Kernel driver | radeon (DRM) |
| CPU | AMD E-300 Bobcat (Family 14h) |
| IBS available | YES (Fetch + OP sampling) |
| Core PMC | NO |
| Vulkan | NO (sw ICD only) |

---

## ISA Reference Documents

All in `~/eric/TerakanMesa/isa-docs/`:

| File | What it covers |
|------|---------------|
| `R600_Instruction_Set_Architecture.pdf` | R600/R700 VLIW5 ISA -- ALU encoding, TEX clause, CF |
| `AMD_Evergreen-Family_ISA.pdf` | Evergreen/HD5000-6000 ISA -- extends R600, covers HD 6310 |
| `R600-R700-Evergreen_Assembly_Language_Format.pdf` | Assembly syntax, instruction encoding tables |
| `LLVM_R600_Backend_Stellard_2013.pdf` | LLVM R600 backend internals -- useful for reading Mesa output |

The HD 6310 implements the **Evergreen** ISA (VLIW5: 4 ALU slots + 1 transcendental).
Cross-reference with Mesa's `src/gallium/drivers/r600/r600_asm.c` for encoding details.

---

## Tool Map (steinmarder analogy)

| NVIDIA (steinmarder) | r600 equivalent | Script |
|---------------------|-----------------|--------|
| `ncu --set full` | R600_DEBUG ISA dump | `r600-shader-dump.sh` |
| `nsys profile` | AMDuProfCLI IBS | `r600-uprof-ibs.sh` |
| `nvidia-smi dmon` | radeontop CSV | `r600-monitor.sh` |
| `apitrace trace` | apitrace trace | `r600-gltrace.sh` |
| CUDA Occupancy | GALLIUM_HUD | `r600-hud.sh` |
| All of the above | **One command** | `r600-session.sh` |

---

## Quick Start

```sh
cd ~/eric/TerakanMesa/re-toolkit

# Full session (ISA + GPU monitor + GL trace):
./r600-session.sh --tag first_run glxgears

# ISA dump only (see compiled VLIW5 assembly):
./r600-shader-dump.sh glxgears

# Background GPU monitor:
./r600-monitor.sh &

# OpenCL path (Rusticl on r600):
./r600-shader-dump.sh --cl clinfo

# Inspect ISA output:
grep -A 30 'ALU clause\|TEX clause\|Native code' data/session_*/isa_and_stderr.log | head -60

# CPU IBS profile:
./r600-uprof-ibs.sh glxgears
```

---

## Reading r600 ISA Output

The Evergreen/r600 ISA uses VLIW5 bundles. Each ALU clause contains 4-element
(x/y/z/w) + 1 transcendental (t) instruction slots that issue simultaneously.

```
ALU clause [0]:
  x: MUL R1.x, R2.x, R3.x     ; slot 0
  y: MUL R1.y, R2.y, R3.y     ; slot 1
  z: ADD R1.z, R4.z, R5.z     ; slot 2
  w: MOV R1.w, 1.0             ; slot 3
  t: RSQ R6.x, R6.x            ; transcendental slot
```

Key ISA sections in `AMD_Evergreen-Family_ISA.pdf`:
- Chapter 3: ALU instruction encoding (VLIW5 bundle format)
- Chapter 4: TEX clause (texture fetch instructions)
- Chapter 5: VTX clause (vertex fetch)
- Chapter 6: CF (control flow) instructions
- Appendix A: Instruction opcode tables

---

## uProf Notes

AMD uProf 5.2.606 with IBS on E-300 Bobcat:
- IBS Fetch sampling: YES -- samples instruction fetch address
- IBS OP sampling: YES -- samples instruction retire (includes load/store latency)
- Core PMC: NO -- hardware event counters not available on Bobcat Family 14h
- RAPL: NO -- no power measurement via RAPL on this generation

This means uProf gives true IBS-based CPU hotspot analysis, which is more
accurate than timer-based sampling. See `r600-uprof-ibs.sh`.

---

## Iterative Workflow

1. **Baseline**: `r600-session.sh --tag baseline <workload>`
2. **Inspect**: `grep ALU data/session_baseline_*/isa_and_stderr.log`
3. **Cross-ref**: compare VLIW5 bundles against Evergreen ISA PDF
4. **Hypothesis**: identify underutilized slots or bottleneck stages
5. **Patch**: modify shader or driver code
6. **Measure**: `r600-session.sh --tag patch1 <workload>`
7. **Diff**: `diff data/session_baseline_*/SUMMARY.txt data/session_patch1_*/SUMMARY.txt`
