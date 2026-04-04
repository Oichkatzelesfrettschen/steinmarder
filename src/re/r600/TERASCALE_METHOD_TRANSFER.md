# TeraScale Method Transfer

This doc turns the repo’s mature optimization patterns into explicit rules for
the x130e / Terakan / r600 program.

## Transfer Rules

| Source pattern | Existing strong track | TeraScale analog |
| --- | --- | --- |
| Manifest-backed tranches | SM89 probe + paper pipeline | dEQP / vkmark / clpeak / perf / radeontop / GALLIUM_HUD wrappers |
| Runtime + code-shape + profiler + correctness | CUDA LBM + SASS RE | Mesa/NIR/SFN changes with benchmark + validation + profiler bundle |
| Layout-first optimization | LBM SoA / pull-scheme wins | state emission shape, memory layout, submission reduction |
| Hoist recurring expensive math | `inv_tau` in LBM | compiler lowering, precomputed constants, schedule-friendly code motion |
| Hardware-native packing | SASS inline-PTX path selection | VLIW5 bundle packing, PV forwarding, literal pressure awareness |
| Honest roofline / hard-limit documentation | SM89 monograph | TeraScale capability ledger and hardware ceiling documentation |

## Where This Matters Immediately

- NIR custom intrinsic and builder API work
- SFN register allocation and spill strategy
- Terakan shader scheduling / VLIW packing interpretation
- winsys and queue cleanup overhead reduction
- cross-stack benchmark normalization

## Acceptance Standard

An optimization is not “real” for this track unless it:

1. improves a measured workload or removes a validated blocker
2. survives correctness or conformance checks
3. has enough artifact trail to be reproduced later
