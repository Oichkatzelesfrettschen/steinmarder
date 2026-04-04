# Zen RE Sources

Cached external reference set for the Ryzen / Zen reverse-engineering lane.

## Agner Fog baseline

- `data/external/zen_re/agner_optimizing_cpp.pdf`
- `data/external/zen_re/agner_optimizing_assembly.pdf`
- `data/external/zen_re/agner_microarchitecture.pdf`
- `data/external/zen_re/agner_instruction_tables.pdf`

## AMD official sources

- `data/external/zen_re/amd_gpuopen_ryzen_software_optimization_gdc2024.pdf`
- `data/external/zen_re/amd_uprof_page.html`
- `data/external/zen_re/amd_uprof_user_guide.html`
- `data/external/zen_re/amd_uprof_release_notes.html`
- `data/external/zen_re/amd_uprof_getting_started.html`
- `data/external/zen_re/amd_uprof_quick_reference.html`
- `data/external/zen_re/amd_apm_combined_volumes_40332_pub_4_08.html`

## Trace artifacts

- `data/external/zen_re/amd_documentation_hub_trace.html`

## Notes

- The current host CPU is an AMD Ryzen 5 5600X3D, so Zen 3 / Family 19h docs are
  the primary hardware target.
- AMD's newer docs portal exposes some manuals through HTML viewers and short
  links rather than simple static PDFs. The cached HTML pages preserve the
  authoritative official content path even when a direct PDF URL is not obvious.
- `data/external/zen_re/PROVENANCE.json` contains the file hashes and retrieval
  metadata for the current cache.
