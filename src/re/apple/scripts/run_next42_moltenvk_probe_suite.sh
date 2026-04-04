#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_moltenvk_$(date +%Y%m%d_%H%M%S)}"
SHADER_DIR="$ROOT_DIR/shaders_vk"

mkdir -p "$OUT_DIR/spirv"

cat > "$OUT_DIR/moltenvk_probe_manifest.csv" <<'EOF'
stage,shader,kind,notes
fp32_pressure,probe_fp32_pressure.comp,compute,fp32 pressure and register/ALU style probe
fp16_convert,probe_fp16_convert.comp,compute,fp16 conversion-heavy probe
int8_pack,probe_int8_pack.comp,compute,int8 pack/unpack and accumulation probe
EOF

{
  echo "preferred_solution=MoltenVK"
  echo "notes=Use MoltenVK as the Vulkan-on-Metal bridge unless a stronger native Vulkan translation path appears on this machine."
} > "$OUT_DIR/moltenvk_strategy.txt"

for candidate in \
    "/opt/homebrew/share/vulkan/icd.d/MoltenVK_icd.json" \
    "/usr/local/share/vulkan/icd.d/MoltenVK_icd.json" \
    "${VULKAN_SDK:-}/share/vulkan/icd.d/MoltenVK_icd.json" \
    "/opt/homebrew/lib/libMoltenVK.dylib" \
    "/usr/local/lib/libMoltenVK.dylib"; do
    [ -n "$candidate" ] || continue
    [ -e "$candidate" ] && echo "$candidate"
done > "$OUT_DIR/moltenvk_candidates.txt" 2>/dev/null || true

if command -v vulkaninfo >/dev/null 2>&1; then
    vulkaninfo > "$OUT_DIR/vulkaninfo.txt" 2>&1 || true
else
    printf 'vulkaninfo_missing\n' > "$OUT_DIR/vulkaninfo.txt"
fi

if command -v glslangValidator >/dev/null 2>&1; then
    for shader in "$SHADER_DIR"/*.comp; do
        [ -f "$shader" ] || continue
        glslangValidator -V "$shader" -o "$OUT_DIR/spirv/$(basename "${shader%.comp}").spv" \
            > "$OUT_DIR/spirv/$(basename "$shader").txt" 2>&1 || true
    done
else
    printf 'glslangValidator_missing\n' > "$OUT_DIR/spirv/validation_status.txt"
fi

cat > "$OUT_DIR/moltenvk_notes.md" <<'EOF'
# Next42 MoltenVK Probe Draft

- preferred bridge: MoltenVK
- expected artifacts: moltenvk_probe_manifest.csv, moltenvk_candidates.txt, vulkaninfo.txt, spirv/*
- current limitation: this draft validates the bridge/toolchain and shader path first; a true host-dispatch lane still needs a Vulkan probe executable
EOF

printf '%s\n' "$OUT_DIR"
