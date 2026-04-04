#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/results/next42_neural_raw_$(date +%Y%m%d_%H%M%S)}"
PYTHON_BIN="${PYTHON_BIN:-$ROOT_DIR/../../.venv/bin/python}"

mkdir -p "$OUT_DIR"

cat > "$OUT_DIR/neural_probe_manifest.csv" <<'EOF'
probe,entrypoint,notes
stack_probe,scripts/neural_lane_probe.py,library and device availability sweep
typed_smoke,scripts/neural_typed_matrix.py,small typed backend smoke matrix
EOF

if [ ! -x "$PYTHON_BIN" ]; then
    PYTHON_BIN=python3
fi

"$ROOT_DIR/scripts/bootstrap_neural_lane.sh" >/dev/null 2>&1 || true

"$PYTHON_BIN" "$ROOT_DIR/scripts/neural_lane_probe.py" > "$OUT_DIR/neural_lane_probe.json" 2> "$OUT_DIR/neural_lane_probe.stderr.txt" || true
"$PYTHON_BIN" "$ROOT_DIR/scripts/neural_typed_matrix.py" \
    --out-dir "$OUT_DIR/typed_smoke" \
    --workloads gemm_256 reduce_1m \
    --warmup-runs 1 \
    --measured-runs 1 \
    > "$OUT_DIR/neural_typed_matrix.stdout.txt" 2> "$OUT_DIR/neural_typed_matrix.stderr.txt" || true

cat > "$OUT_DIR/neural_notes.md" <<'EOF'
# Next42 Neural Raw Probes

- stack probe: neural_lane_probe.py
- typed smoke: neural_typed_matrix.py with reduced warmup/measured counts
- expected artifacts: neural_lane_probe.json, typed_smoke/*, stderr/stdout sidecars
EOF

printf '%s\n' "$OUT_DIR"
