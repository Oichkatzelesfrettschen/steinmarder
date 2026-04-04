#!/bin/sh
# Fetch and cache external Zen/Ryzen reference documents with simple provenance.

set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)"
OUT="${ROOT}/data/external/zen_re"
INDEX="${ROOT}/docs/external_sources/ZEN_RE_SOURCES.md"
PROV="${OUT}/PROVENANCE.json"

mkdir -p "$OUT"

fetch() {
    url="$1"
    file="$2"
    curl -A 'Mozilla/5.0' -L --fail --max-time 120 "$url" -o "${OUT}/${file}"
}

# Agner Fog baseline manuals
fetch "https://www.agner.org/optimize/optimizing_cpp.pdf" "agner_optimizing_cpp.pdf"
fetch "https://www.agner.org/optimize/optimizing_assembly.pdf" "agner_optimizing_assembly.pdf"
fetch "https://www.agner.org/optimize/microarchitecture.pdf" "agner_microarchitecture.pdf"
fetch "https://www.agner.org/optimize/instruction_tables.pdf" "agner_instruction_tables.pdf"

# AMD official / OEM sources that are directly fetchable
fetch "https://gpuopen.com/download/GDC2024_AMD_Ryzen_Processor_Software_Optimization.pdf" \
      "amd_gpuopen_ryzen_software_optimization_gdc2024.pdf"
fetch "https://www.amd.com/en/developer/uprof.html" "amd_uprof_page.html"
fetch "https://docs.amd.com/go/en-US/57368-uProf-user-guide" "amd_uprof_user_guide.html"
fetch "https://docs.amd.com/go/en-US/63856-uProf-release-notes" "amd_uprof_release_notes.html"
fetch "https://docs.amd.com/r/en-US/68658-uProf-getting-started-guide" "amd_uprof_getting_started.html"
fetch "https://docs.amd.com/v/u/en-US/uProf-Quick-Reference-Guide" "amd_uprof_quick_reference.html"
fetch "https://docs.amd.com/v/u/en-US/40332-PUB_4.08" "amd_apm_combined_volumes_40332_pub_4_08.html"

# Official-but-fragile references: cache a stable landing page / trace artifact.
fetch "https://www.amd.com/en/search/documentation/hub.html#sortCriteria=%40amd_release_date%20descending&f-amd_archive_status=Active&f-amd_audience=Technical" \
      "amd_documentation_hub_trace.html"

python - <<'PY' >"${PROV}"
import datetime as dt
import hashlib
import json
from pathlib import Path

root = Path("data/external/zen_re")
entries = []
for path in sorted(root.iterdir()):
    if not path.is_file() or path.name == "PROVENANCE.json":
        continue
    data = path.read_bytes()
    entries.append({
        "file": path.name,
        "sha256": hashlib.sha256(data).hexdigest(),
        "bytes": len(data),
        "retrieved_utc": dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
    })
print(json.dumps({"topic": "zen_re", "entries": entries}, indent=2))
PY

cat >"${INDEX}" <<'EOF'
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
EOF

echo "Fetched Zen / Ryzen reference documents into ${OUT}"
