<!--
@file SFN_GLOBAL_ATOMIC_FIX.md
@brief Root cause analysis and fix for nir_intrinsic_global_atomic crash in r600 SFN backend.
-->
# r600 SFN: global_atomic / global_atomic_swap Fix

**Date:** 2026-04-04  
**Status:** Fixed and installed  
**Affected binary:** `/usr/local/mesa-debug/lib/x86_64-linux-gnu/libRusticlOpenCL.so.1.0.0`

---

## Symptom

OpenCL kernels that use `atomic_add` / `atomic_cmpxchg` on global memory pointers crash with:

```
r600_sfn.cpp:103: Assertion `0' failed.
```

The NIR dump shows the failing intrinsic as `@global_atomic (ptr, value) (atomic_op=iadd)` or
`@global_atomic_swap (ptr, cmp, swap)`. The `assert(0)` at line 103 fires because
`r600_shader_from_nir` returns null after `translate_from_nir` fails.

---

## Root Causes

### 1. Missing dispatch in `RatInstr::emit()` (`sfn_instr_mem.cpp`)

`nir_intrinsic_global_atomic` (enum 166) and `nir_intrinsic_global_atomic_swap` (enum 171) had
no case in the switch. They fell through to `default: return false`, which propagated to
`process_block`'s "Unsupported instruction" path and ultimately to `assert(0)`.

### 2. Missing `sh_needs_sbo_ret_address` flag in `scan_instruction()` (`sfn_shader.cpp`)

`Shader::allocate_reserved_registers()` only allocates `m_rat_return_address` (the per-wave
return ring buffer register) when `sh_needs_sbo_ret_address` is set. This flag was set by
`scan_instruction()` for `ssbo_atomic`, `image_atomic`, etc., but NOT for `global_atomic`.

When `emit_global_atomic_op` executed (after fix #1), it called `shader.rat_return_address()`
which has `assert(m_rat_return_address)`. Since the flag was never set, the register was never
allocated, and the assert fired.

---

## Diagnostic Path

```
strace -e openat ./cl_canary_atomics | grep '\.icd\|RusticlOpenCL'
```

Revealed the system ICD at `/etc/OpenCL/vendors/mesa-rusticl-debug.icd` was loading
`/usr/local/mesa-debug/lib/x86_64-linux-gnu/libRusticlOpenCL.so` (an old build), ignoring
`OCL_ICD_FILENAMES`. The correct override is:

```bash
OPENCL_VENDOR_PATH=/tmp/ocl-icd ./test
# where /tmp/ocl-icd/my-rusticl.icd contains the path to the target .so
```

Jump table analysis via `objdump -d` confirmed entry 2 (intrinsic 166) routed to the correct
code path once root cause #1 was fixed, and `assert(m_rat_return_address)` was the only
remaining failure.

---

## Fix

Three files changed in `~/workspaces/mesa/mesa-26-debug/` on x130e:

### `sfn_instr_mem.h`

```cpp
// Added in private RatInstr section:
static bool emit_global_atomic_op(nir_intrinsic_instr *intr, Shader& shader);
```

### `sfn_instr_mem.cpp`

**In `RatInstr::emit()` switch:**
```cpp
case nir_intrinsic_global_atomic:
case nir_intrinsic_global_atomic_swap:
   return emit_global_atomic_op(intr, shader);
```

**New function** (after `emit_global_store`, modeled on `emit_ssbo_atomic_op`):
```cpp
bool
RatInstr::emit_global_atomic_op(nir_intrinsic_instr *intr, Shader& shader)
{
   auto& vf = shader.value_factory();

   bool read_result = !list_is_empty(&intr->def.uses);
   auto opcode = read_result ? get_rat_opcode(nir_intrinsic_atomic_op(intr))
                             : get_rat_opcode_wo(nir_intrinsic_atomic_op(intr));

   auto coord_orig = vf.src(intr->src[0], 0);   // raw global pointer
   auto coord = vf.temp_register(0);
   auto data_vec4 = vf.temp_vec4(pin_chgr, {0, 1, 2, 3});

   // address is a byte pointer; RAT expects dword index
   shader.emit_instruction(
      new AluInstr(op2_lshr_int, coord, coord_orig, vf.literal(2), AluInstr::write));

   // thread return address in slot 1 (required by hardware even for write-only ops)
   shader.emit_instruction(
      new AluInstr(op1_mov, data_vec4[1], shader.rat_return_address(), AluInstr::write));

   if (intr->intrinsic == nir_intrinsic_global_atomic_swap) {
      // CAS: src[1]=compare, src[2]=new_value
      shader.emit_instruction(
         new AluInstr(op1_mov, data_vec4[0], vf.src(intr->src[2], 0), AluInstr::write));
      shader.emit_instruction(
         new AluInstr(op1_mov,
                      data_vec4[shader.chip_class() == ISA_CC_CAYMAN ? 2 : 3],
                      vf.src(intr->src[1], 0),
                      AluInstr::write));
   } else {
      // non-swap: src[1]=value
      shader.emit_instruction(
         new AluInstr(op1_mov, data_vec4[0], vf.src(intr->src[1], 0), AluInstr::write));
   }

   RegisterVec4 out_vec(coord, coord, coord, coord, pin_chgr);
   auto atomic = new RatInstr(cf_mem_rat, opcode, data_vec4, out_vec,
                              shader.ssbo_image_offset(), nullptr, 1, 0xf, 0);
   shader.emit_instruction(atomic);

   atomic->set_ack();
   if (read_result) {
      atomic->set_instr_flag(ack_rat_return_write);
      auto dest = vf.dest_vec4(intr->def, pin_group);
      auto wait = new ControlFlowInstr(ControlFlowInstr::cf_wait_ack);
      wait->add_required_instr(atomic);
      shader.emit_instruction(wait);

      auto fetch = new FetchInstr(vc_fetch, dest, {0, 1, 2, 3},
                                  shader.rat_return_address(), 0, no_index_offset,
                                  fmt_32, vtx_nf_int, vtx_es_none,
                                  R600_IMAGE_IMMED_RESOURCE_OFFSET + shader.ssbo_image_offset(),
                                  nullptr);
      fetch->set_mfc(15);
      fetch->set_fetch_flag(FetchInstr::srf_mode);
      fetch->set_fetch_flag(FetchInstr::use_tc);
      fetch->set_fetch_flag(FetchInstr::vpm);
      fetch->add_required_instr(wait);
      shader.chain_ssbo_read(fetch);
      shader.emit_instruction(fetch);
   }
   return true;
}
```

**Key differences from `emit_ssbo_atomic_op`:**
- No `evaluate_resource_offset` — address is a raw global pointer, not a buffer-relative offset
- RAT slot = `shader.ssbo_image_offset()` directly (global memory RAT, same as `store_global`)
- `image_offset = nullptr`
- `src[0]` = address, `src[1]` = value (not `src[1]`/`src[2]`/`src[3]` as in SSBO)

### `sfn_shader.cpp`

**In `scan_instruction()` switch:**
```cpp
case nir_intrinsic_global_atomic:
case nir_intrinsic_global_atomic_swap:
// ... (before the existing ssbo_atomic cases)
case nir_intrinsic_ssbo_atomic:
case nir_intrinsic_ssbo_atomic_swap:
...
   m_flags.set(sh_needs_sbo_ret_address);
   FALLTHROUGH;
```

---

## Test Result

```
dispatch_global_atomic: global=2048 local=32 groups=64
global_atomic_inc=OK n=64 expected=32
```

Generated ISA for `iadd` (write-only, `read_result=false`):
```
MEM_RAT WRITE_IND_ACK RAT0 ADD 0 R2.xyzw R3.xyz ES:0 AS:0 MARK EOP
```

---

## Remaining Issues

- `local_atomic_inc_to_global=FAIL` — pre-existing, unrelated to this fix. Affects the LDS
  accumulate + `store_global` kernel path; gives wrong values for multi-group dispatches due
  to a race in the LDS→global store pattern. Not a global_atomic regression.

- `global_atomic_swap` (CAS path) — code is in place but not yet verified by a dedicated
  `atomic_cmpxchg` kernel test. The `atomic_cas_driver` test that triggered the original crash
  should now compile; result correctness is pending verification.
