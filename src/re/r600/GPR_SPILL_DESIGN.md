# GPR Spill-to-Scratch Design Document
## TeraScale-2 / Evergreen VLIW5 — SFN Register Allocator Extension

**Author**: steinmarder project  
**Date**: 2026-04-04  
**Status**: DESIGN PHASE  
**Blocks**: Task #42 (SFN GPR spill implementation), Task #68 (scratch latency probe)

---

## 1. Problem Statement

The SFN register allocator (`sfn_ra.cpp`) has **no spill fallback**. When graph coloring
fails (live ranges exceed the 128-GPR file), `register_allocation()` returns `false` and
the shader compilation fails with "Register allocation failed." Complex compute kernels
(heavy ML layers, LBM simulations, deep control flow) crash on this path.

### Current RA Architecture
```
LiveRangeEvaluator::run() → LiveRangeMap
    ↓
register_allocation(LiveRangeMap&)
    ├── group_allocation()      → returns false if coloring fails
    ├── scalar_clause_local_allocation()
    └── scalar_allocation()     → returns false if coloring fails
    ↓
(no spill path — hard failure)
```

## 2. Hardware Constraints

### 2.1 Register File
- **128 GPRs** total (R0-R127), each 4-component (xyzw)
- System values consume R0-R4 typically (position, thread ID, etc.)
- **~123 usable GPRs** for shader computation
- Each component (x,y,z,w) is independently allocatable

### 2.2 Scratch Memory Access (Measured/Documented)

| Operation | Instruction | Type | Clause Impact | Latency |
|-----------|-------------|------|---------------|---------|
| **WRITE** | `CF_OP_MEM_SCRATCH` | CF instruction | **None** — interleaves with ALU | ~27 cyc (TC L1) |
| **READ** | `FETCH_OP_READ_SCRATCH` | VTX/TEX fetch | **FORCES TEX clause break** | ~27-38 cyc (TC) |

**Critical asymmetry**: Scratch writes are cheap (CF instruction, no clause break).
Scratch reads are expensive (forces a TEX clause insertion between ALU clauses).

### 2.3 Clause Architecture
TeraScale executes in strict clause types:
- **ALU clause**: Up to 128 VLIW5 bundles of ALU operations
- **TEX clause**: Up to 16 texture/vertex fetch operations
- **CF instructions**: Control flow, exports, scratch writes

Transitions between clause types have overhead (~5 cycles per clause switch,
measured in clause_switch_alu_tex probe). A scratch reload forces:
```
[ALU clause N] → [TEX clause: FETCH_OP_READ_SCRATCH] → [ALU clause N+1]
```
Adding ~5 cycles switching overhead + ~27-38 cycles fetch latency = **~32-43 cycles per reload**.

### 2.4 Per-Thread Scratch Addressing
```
scratch_base_addr = ring_base_reg (per-SE, set via PM4)
per_thread_offset = thread_id × scratch_item_size
total_offset = scratch_base_addr + per_thread_offset + spill_slot_offset
```
- `scratch_item_size` = `shader->scratch_space_needed * 4` bytes
- Hardware computes `thread_id` automatically
- `spill_slot_offset` encoded in the CF/VTX instruction's `array_base` or `index_gpr`

## 3. Design: Asymmetric Spill Algorithm

### 3.1 Core Principle: "Spill Eagerly, Reload Lazily"

Since writes are cheap (CF) and reads are expensive (TEX clause break), the algorithm
should:
1. **Spill (write) as soon as a value's vec-slot use is done** — the CF_OP_MEM_SCRATCH
   can be inserted immediately after the producing ALU clause with minimal overhead
2. **Defer reload (read) as long as possible** — batch multiple pending reloads into a
   single TEX clause to amortize the clause-break penalty
3. **Exploit natural clause boundaries** — if an ALU→TEX transition already exists
   (e.g., texture sampling), piggyback scratch reads into that TEX clause for free

### 3.2 Spill Candidate Selection Heuristic

When coloring fails, select the live range with **maximum spill benefit**:

```
spill_benefit(LR) = dead_range_length(LR) × interference_count(LR)
                    ─────────────────────────────────────────────────
                    reload_cost(LR)

where:
  dead_range_length = distance (in bundles) between last def and next use
  interference_count = number of other live ranges LR interferes with
  reload_cost = 1 if a natural TEX clause boundary exists before next use
                clause_switch_penalty (~5 cyc) + fetch_latency (~30 cyc) otherwise
```

Prefer spilling values with:
- **Long dead ranges** (more bundles freed for other values)
- **High interference** (unblocks more coloring candidates)
- **Low reload cost** (natural TEX boundary available nearby)

### 3.3 The Iterated Spill-Recolor Loop

```
function spill_and_recolor(LiveRangeMap lrm, Interference ifg):
    max_iterations = 10  // safety bound
    
    for iteration in 0..max_iterations:
        if try_color(lrm, ifg):
            return SUCCESS
        
        // Coloring failed — select spill candidate
        candidate = select_spill_candidate(lrm, ifg)
        if candidate is None:
            return FAILURE  // nothing left to spill
        
        // Split the live range at the spill point
        split_live_range(candidate, lrm):
            1. Insert CF_OP_MEM_SCRATCH write after candidate's last def
            2. Insert FETCH_OP_READ_SCRATCH before candidate's next use
            3. Create a NEW short live range for the reloaded value
            4. Remove candidate from interference graph
            5. Add the new short live range to interference graph
        
        // Rebuild interference and retry
        ifg = rebuild_interference(lrm)
    
    return FAILURE
```

### 3.4 Clause Insertion Mechanics

#### Spill Store (cheap — CF instruction)
```
After the ALU bundle that last defines the spilled register:
  CF_OP_MEM_SCRATCH {
    gpr = spilled_register.sel(),
    comp_mask = spilled_register.writemask(),
    array_base = spill_slot_index,
    type = 0 (direct addressing)
  }
```
This is a CF instruction — it does NOT break the current ALU clause.

#### Spill Reload (expensive — TEX clause)
```
Before the ALU bundle that next uses the spilled register:
  [End current ALU clause]
  TEX clause {
    FETCH_OP_READ_SCRATCH {
      dst_gpr = reload_register.sel(),
      src_gpr = (unused for direct mode),
      array_base = spill_slot_index,
      type = 2 (Evergreen direct read)
    }
  }
  [Begin new ALU clause]
```
This FORCES an ALU clause break. The reload register is a **new, short-lived GPR**
that exists only from the TEX clause to the consuming ALU bundle.

### 3.5 Batched Reload Optimization

When multiple spilled values need reloading near the same program point, batch them
into a single TEX clause:

```
[ALU clause N ends]
TEX clause {
  FETCH_OP_READ_SCRATCH  slot=0  → R_reload_A
  FETCH_OP_READ_SCRATCH  slot=1  → R_reload_B
  FETCH_OP_READ_SCRATCH  slot=2  → R_reload_C
}
[ALU clause N+1 begins, uses R_reload_A, R_reload_B, R_reload_C]
```

This amortizes the clause-switch penalty across multiple reloads.
TEX clauses can hold up to 16 fetch instructions — so up to 16 simultaneous
reloads per clause break.

### 3.6 Natural Boundary Exploitation

If the shader already has a TEX clause (e.g., texture sampling), append scratch
reads to it:

```
TEX clause {
  SAMPLE    R5.xyzw, R2.xy   // existing texture fetch
  FETCH_OP_READ_SCRATCH slot=3 → R_reload_D  // FREE piggyback
}
```

This eliminates the clause-switch penalty entirely for these reloads.

## 4. SFN Integration Points

### 4.1 Pipeline Position
```
NIR → SFN translation → Optimization → Scheduling → LIVENESS → RA
                                                       ↓
                                               [SPILL if RA fails]
                                                       ↓
                                               Re-schedule → Re-RA
```

Spill insertion happens **after** the initial RA failure but **before** final
bytecode emission. The spill-modified IR must be re-scheduled and re-allocated.

### 4.2 Files to Modify

| File | Change |
|------|--------|
| `sfn_ra.cpp` | Add `spill_and_recolor()` loop around `register_allocation()` |
| `sfn_ra.h` | Add spill candidate selection interface |
| `sfn_shader.cpp` | Call spill loop, set `scratch_space_needed` |
| `sfn_scheduler.cpp` | Handle scratch instructions in clause scheduling |
| `sfn_assembler.cpp` | Already handles `ScratchIOInstr` — no changes needed |
| `sfn_liverangeevaluator.cpp` | May need re-run after spill insertion |

### 4.3 Scratch Slot Management

Each spilled live range gets a unique slot index:
```
scratch_slot_allocator:
  next_slot = 0
  
  allocate_slot(live_range) → slot_index:
    slot = next_slot
    next_slot += 1  // each slot is 1 vec4 (16 bytes)
    return slot
  
  total_scratch_needed = next_slot  // in vec4 units
```

Set `shader->scratch_space_needed = total_scratch_needed` after spilling.
The driver's `evergreen_setup_scratch_buffers()` allocates the physical buffer.

## 5. Correctness Verification

### 5.1 Piglit Pressure Test
Write a shader that uses **125+ independent vec4 values** (exceeding the ~123 usable
GPRs), computes a result from all of them, and verifies correctness:

```glsl
// piglit_spill_pressure.shader_test
// Declare 125 independent vec4 multiplications
// Store all 125 results to output
// Verify: each result = input[i] * scale[i]
```

If the spill algorithm works, this shader compiles and produces correct output.
If it fails, the shader compilation crashes (current behavior).

### 5.2 shader-db Regression Test
Run shader-db before and after spill implementation:
- **No shader should NEWLY spill** that previously fit in registers
- **Spill count** should be reported in shader-db output for tracking
- **GPR count** should not increase for non-spilling shaders

### 5.3 Rusticl Compute Kernels
The primary use case: complex OpenCL kernels that currently crash:
- clpeak global memory bandwidth test (uses >128 GPRs internally)
- Heavy ML kernels (matrix multiply with large tile sizes)
- D2Q9 LBM simulation (#79, blocked by #42)

## 6. Performance Considerations

### 6.1 Expected Overhead
- Each spill slot: 16 bytes per thread (vec4)
- Each reload: ~32-43 cycles (clause switch + TC fetch)
- Each store: ~27 cycles (CF instruction, no clause switch)
- Batched reloads: overhead amortized across up to 16 values

### 6.2 Register Pressure vs Spill Tradeoff
For shaders near the 123-GPR limit, spilling 1-2 values adds ~60-80 cycles
of overhead but enables the shader to compile at all. This is acceptable
because the alternative is a hard crash.

### 6.3 Future Optimization: Rematerialization
For values that are cheap to recompute (e.g., `x * constant`), rematerialization
(recomputing instead of reloading from scratch) avoids the TEX clause penalty
entirely. This is a Phase 2 optimization after basic spilling works.

## 7. References

- AMD Evergreen Family ISA Manual — Section 10: Memory Operations
- Chaitin et al., "Register Allocation via Coloring" (1981)
- Touati & Dupont de Dinechin, "Register Pressure in VLIW" (2004)
- LLVM R600 Backend — `R600MachineScheduler.h` (prior art)
- Mesa SFN source: `sfn_ra.cpp`, `sfn_liverangeevaluator.cpp`, `sfn_assembler.cpp`
- Measured latencies: VFETCH 27.2 cyc, TEX SAMPLE 38 cyc, clause switch ~5 cyc
