# TeraScale Manual Diagnostics — 2026-04-02

This bundle captures the first post-bridge manual diagnostics after wiring a
minimal Terakan `vk_device_shader_ops` guard path and generating the new
integer-baseline plus occupancy probe families.

## Vulkan Compute Boundary

Targeted probe:

- shader: `probes/r600/vulkan_compute/depchain_u32_add.comp`
- host: `host/terascale_vk_probe_host.c`
- ICD:
  `/home/eirikr/workspaces/mesa/mesa-26-debug/build-terakan-distcc/src/amd/terascale/vulkan/terascale_devenv_icd.x86_64.json`

Observed boundary:

- the old common-runtime segfault is gone
- pipeline creation now reaches Terakan's guarded shader-ops path
- `vkCreateComputePipelines` returns `VK_ERROR_FEATURE_NOT_PRESENT (-8)`
- Terakan now prints an explicit diagnostic before returning:

```text
TERAKAN: compute/common pipeline compile bridge missing; shader_count=1.
Need vk_shader wrapper + serialize/deserialize + pipeline cache integration.
Returning VK_ERROR_FEATURE_NOT_PRESENT instead of crashing.
```

This is the correct next-stage failure boundary. The missing work is now
concentrated in the common Vulkan shader bridge instead of a null-vtable crash.

## OpenCL Integer and Occupancy Probes

Manual OpenCL probes were rerun against Rusticl PALM after generating the new
`u8/i8/u16/i16/u32/i32/u64/i64` and `occupancy_gpr_*` families.

### depchain_u16_add

- elements: `4096`
- local size: `64`
- build ns: `70342278`
- kernel ns: `15632010`
- read ns: `2073426`
- `nan_count`: `1075`
- checksum:
  `-931912935778360404733355782941045686272.000000000`

Compiler notes:

- ISA shows explicit subword handling via `AND_INT` and `BFE_INT`
- conversion path ends in `UINT_TO_FLT` before the writeback lane

### depchain_u64_add

- elements: `4096`
- local size: `64`
- build ns: `7433348`
- kernel ns: `24972492`
- read ns: `2210095`
- `nan_count`: `1653`
- checksum:
  `-473678940364481199874494040888594923520.000000000`

Compiler notes:

- ISA is substantially larger and uses long integer/control sequences
- the path still degrades into pathological floating-point output when the host
  interprets the output buffer as `float`

### occupancy_gpr_32

- elements: `2048`
- local size: `64`
- build ns: `128217578`
- kernel ns: `37762868`
- read ns: `1930098`
- `nan_count`: `766`
- checksum:
  `-24502284117678455697999422819542331392.000000000`

### occupancy_gpr_64

- elements: `2048`
- local size: `64`
- build ns: `195678881`
- kernel ns: `92028430`
- read ns: `5932534`
- `nan_count`: `272`
- checksum:
  `-3273907440721623698241087599114452992.000000000`

Compiler notes:

- generated ISA reached `880` dwords
- compiler reported `48 gprs`
- kernel body is dominated by long `MULADD_IEEE` chains with a final
  reduction/writeback sweep

## Current Interpretation

- the Vulkan lane has moved from crash triage to an explicit bridge-implementation
  task
- the OpenCL lane is now rich enough to study:
  - subword integer lowering
  - 64-bit legalization cost
  - register-pressure and occupancy scaling
- numeric stability is still poor enough that the next tranche should keep
  prioritizing packed vs unpacked integer, `int24` / `uint24`, and typed-output
  host paths before making strong arithmetic conclusions
