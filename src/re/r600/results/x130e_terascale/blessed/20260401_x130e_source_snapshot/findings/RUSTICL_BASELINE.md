# Rusticl OpenCL Baseline — Mesa 26.0.3 Debug Build
Date: 2026-03-30

## Device Info
- Device: AMD PALM (DRM 2.51.0)
- OpenCL Version: 1.2
- Driver: 26.0.3 (git-3f173c02d1)
- Compute Units: 2
- Global Memory: 384 MB
- Local Memory: 32 KB (mapped to global)
- Platform: rusticl, OpenCL 3.0

## Extensions (22)
cl_khr_icd, cl_khr_byte_addressable_store, cl_khr_create_command_queue,
cl_khr_expect_assume, cl_khr_extended_bit_ops, cl_khr_extended_versioning,
cl_khr_global_int32_base_atomics, cl_khr_global_int32_extended_atomics,
cl_khr_il_program, cl_khr_local_int32_base_atomics,
cl_khr_local_int32_extended_atomics, cl_khr_integer_dot_product,
cl_khr_spirv_no_integer_wrap_decoration, cl_khr_spirv_queries,
cl_khr_suggested_local_work_size, cl_ext_immutable_memory_objects,
cl_khr_spirv_linkonce_odr, cles_khr_int64, cl_khr_kernel_clock,
cl_khr_depth_images, cl_ext_image_unorm_int_2_101010, cl_khr_pci_bus_info

## clpeak Results
- clinfo: PASS (device detected, all info queried)
- Global memory bandwidth: CRASH (SFN register overflow assertion)
  Error: value.sel() < g_clause_local_end (123 register limit exceeded)
  This indicates the compute kernel requires more GPRs than available.

## Status
- Device enumeration: WORKING
- Kernel compilation: PARTIAL (simple kernels work, complex kernels hit GPR limit)
- 8-bit lowering fix: IN PLACE (nir_lower_bit_size callback added)
- Need: Investigate GPR spilling for compute kernels


## GPR Spill RCA (2026-03-30)

### Root Cause
The SFN register allocator (sfn_ra.cpp) has NO spill-to-scratch fallback.
When coloring fails (all 123 GPRs used), it leaves registers with the
sentinel value g_registers_unused (0x7FFFFFFF). The assembler then hits
an assertion or generates invalid code.

### Register Limits
- GPR 0-122: Usable (123 vec4 = 492 scalars)
- GPR 123: Last usable GPR
- GPR 124-127: Clause-local temporaries
- g_registers_end = 123
- g_registers_unused = 0x7FFFFFFF (sentinel for uncolored registers)

### Fix Attempted
1. Added nir_opt_sink + nir_opt_move before RA (reduces live ranges)
2. Lowered scratch threshold from 40 to 16 bytes
3. Softened assembler assertion to warning
4. Result: kernel compiles without crash but RA still fails to color
   all registers (sentinel values appear in assembler output)

### Proper Fix Needed
The SFN RA needs one of:
- A spill-to-scratch pass that demotes excess registers to MEM_SCRATCH ops
- Live range splitting to break long-lived temporaries
- Rematerialization of cheap ops instead of keeping them in registers

### Clock Frequency Fix
Added sysfs fallback in radeon_drm_winsys.c for when DRM RADEON_INFO_MAX_SCLK
returns 0. Reads from /sys/class/drm/card0/device/pp_dpm_sclk.
