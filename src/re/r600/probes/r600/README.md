# r600 / TeraScale Probe Corpus

This directory is the local, source-controlled home for the TeraScale probe
corpus.

## Imported legacy material

- `legacy_glsl/`
  - raw fragment probes migrated from the x130e `TerakanMesa/re-toolkit`
  - covers ALU chains, VLIW5 packing, texture behavior, clause switching, and
    register pressure
- `shader_tests/`
  - piglit `shader_runner` inputs imported from the same toolkit
- `manifests/legacy_manifest.json`
  - imported inventory of the original 18 fragment probes

## Current local starter probes

The root-level `.frag` files in this directory are the earliest local r600
probe subset that already lived in `src/sass_re` before the toolkit migration.

## Intended growth

The next directories to populate are:

- Vulkan graphics / WSI probes
- Vulkan compute probes
- OpenCL probes
- OpenGL compute probes
- system-side queue / DRM / fence measurement helpers

The canonical inventory is emitted by:

```sh
python3 scripts/emit_terascale_probe_manifest.py emit --format tsv --header
```
