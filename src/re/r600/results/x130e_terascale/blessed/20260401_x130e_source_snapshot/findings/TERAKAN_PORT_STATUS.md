=== TERAKAN PORT STATUS ===
# Terakan Port to Mesa 26.0.3 — Status

Source: gitlab.freedesktop.org/Triang3l/mesa.git branch Terakan (commit 38146e0)
Target: Mesa 26.0.3

## Completed
- [x] Terakan source tree copied to src/amd/terascale/
- [x] meson.build / meson.options patched for amd_terascale driver
- [x] inc_mapi removal (removed in Mesa 26)
- [x] gl_shader_stage -> mesa_shader_stage rename
- [x] vk_image_view_init signature update (added driver_internal param)
- [x] nir_copy_prop -> nir_opt_copy_prop rename
- [x] vk_icd_gen.py argument changes (--icd-lib-path/--icd-filename)
- [x] Register define compatibility (copied Terakan eg_sq.h/evergreend.h)
- [x] Custom NIR intrinsic indices (id_base, mega_fetch_count_r600, etc.)
- [x] Custom NIR intrinsic loads (texture_resource_r600, kcache_r600)
- [x] NIR divergence analysis cases for custom intrinsics

## Remaining (blocks build)
- [ ] NIR builder macro API changes for custom intrinsics
      terakan_nir_lower_bindings.c uses .field=value syntax in
      nir_load_texture_resource_r600() and nir_uav_instr_r600() calls
      that requires Terakan branch modifications to nir_builder_opcodes.h
      generation (nir_builder_opcodes_h.py)
- [ ] nir_uav_instr_r600 custom intrinsic (store/atomic UAV operations)
      Not yet added to nir_intrinsics.py

## Build Status
- r600 GL driver: BUILDS OK
- Rusticl: BUILDS OK (with 8-bit lowering fix)
- VA-API: BUILDS OK
- llvmpipe/softpipe: BUILDS OK
- Terakan Vulkan: FAILS on terakan_nir_lower_bindings.c (NIR builder API)

## Files
- ~/mesa-26-debug/ — Mesa 26 with Terakan source integrated
- ~/mesa-terakan/ — Original Terakan branch (for reference)
