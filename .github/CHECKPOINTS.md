# steinmarder Checkpoints (for AI agents)

This checklist captures the actionable checkpoints described in `.github/copilot-instructions.md` and embeds them as a discoverable repo artifact.

- [ ] Read the big picture: `sm_main.c` -> `render.c` -> primitives/BVH -> denoiser.
- [ ] Inspect renderer threading: `render.c` (`render_scene_st` and `render_scene_mt`, `WorkerLocal`, per-thread RNG).
- [ ] Verify deterministic RNG: `sm_seed_pixel`, `sm_hash_u32`, per-tile seeding in `render.c`.
- [ ] Review adaptive sampling config and stats: `SM_ADAPTIVE`, `SM_SPP_MIN`, `SM_SPP_BATCH`, `SM_REL_ERR`, `SM_ABS_ERR` in `render.c`.
- [ ] Confirm BVH & primitives: `bvh.c`, `sphere.c`, `triangle.c` (look for `sphere_intersect` usage).
- [ ] Check image output & dumps: `image.c`, `sm_main.c` (`SM_DUMP_RGB`, `output.ppm`, `output_color.smb`).
- [ ] Inspect neural/ONNX denoiser integration: `neural_denoise.c`, `onnx_denoise.c`, and `shaders/` for SPV assets.
- [ ] Follow naming conventions: prefer `sm_`-prefixed helpers when adding public APIs.
- [ ] When changing concurrency/state: preserve `_Atomic` counters and `WorkerLocal` alignment (avoid false sharing).
- [ ] Editor-specific limits: `sm_mesh_edit.c` uses `MAX_VERTS` / `MAX_TRIS` -- respect these fixed caps.
- [ ] Build note: project requires C11. Use CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

- [ ] Local run examples:
  - Fast debug: `SM_W=320 SM_H=180 SM_SPP=4 SM_THREADS=0 ./build/bin/steinmarder`
  - Adaptive + denoise: `SM_ADAPTIVE=1 SM_NEURAL_DENOISE=1 ./build/bin/steinmarder`
