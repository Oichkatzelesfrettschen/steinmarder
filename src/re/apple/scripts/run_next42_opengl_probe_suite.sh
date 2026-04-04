#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_opengl_$(date +%Y%m%d_%H%M%S)}"
SHADER_DIR="$ROOT_DIR/shaders_gl"

mkdir -p "$OUT_DIR/compiled"

cat > "$OUT_DIR/opengl_probe_manifest.csv" <<'EOF'
stage,shader,kind,notes
baseline,probe_fullscreen.vert,vertex,fullscreen triangle vertex stage
register_pressure,probe_register_pressure.frag,fragment,ALU/register-heavy fragment probe
texture_stride,probe_texture_stride.frag,fragment,texture/cache-like address stride probe
int_quantize,probe_int_quantize.frag,fragment,integer and quantization style fragment probe
EOF

{
  echo "legacy_api=yes"
  echo "notes=macOS OpenGL remains a legacy API; use this lane mainly for fragment/texture-pressure analogues, not compute claims."
} > "$OUT_DIR/opengl_capability_notes.txt"

system_profiler SPDisplaysDataType > "$OUT_DIR/system_profiler_displays.txt" 2>&1 || true
system_profiler SPSoftwareDataType > "$OUT_DIR/system_profiler_software.txt" 2>&1 || true
otool -L /System/Library/Frameworks/OpenGL.framework/OpenGL > "$OUT_DIR/opengl_framework_links.txt" 2>&1 || true

if command -v glslangValidator >/dev/null 2>&1; then
    for shader in "$SHADER_DIR"/*; do
        [ -f "$shader" ] || continue
        stage=frag
        case "$shader" in
            *.vert) stage=vert ;;
            *.frag) stage=frag ;;
        esac
        glslangValidator -S "$stage" "$shader" > "$OUT_DIR/compiled/$(basename "$shader").txt" 2>&1 || true
    done
else
    printf 'glslangValidator_missing\n' > "$OUT_DIR/compiled/validation_status.txt"
fi

cat > "$OUT_DIR/opengl_notes.md" <<'EOF'
# Next42 OpenGL Probe Draft

- lane intent: fragment/texture/register-pressure analogues on macOS legacy OpenGL
- expected artifacts: opengl_probe_manifest.csv, compiled/*.txt, system_profiler_* sidecars
- current limitation: no OpenGL compute claim path on stock macOS; keep this lane shader-centric unless a real host harness is added
EOF

printf '%s\n' "$OUT_DIR"
