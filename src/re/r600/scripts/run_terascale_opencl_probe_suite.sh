#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
HOST="${HOST:-x130e}"
OUT_DIR="${1:-$ROOT_DIR/results/terascale_opencl_probe_suite_$(date +%Y%m%d_%H%M%S)}"
REMOTE_WORK="${REMOTE_WORK:-/tmp/terascale_opencl_probe_$$}"
VK_RESULTS_CSV="${VK_RESULTS_CSV:-}"
PROFILE="${PROFILE:-default}"
OPENCL_BUILD_OPTIONS="${OPENCL_BUILD_OPTIONS:-}"
HOST_CC="${HOST_CC:-cc}"
HOST_CFLAGS="${HOST_CFLAGS:--O2 -std=c11 -Wall -Wextra}"

mkdir -p "$OUT_DIR"/{runs,build}
SUMMARY_CSV="$OUT_DIR/suite_results.csv"
MANIFEST_JSON="$OUT_DIR/run_manifest.json"
BUILD_LOG="$OUT_DIR/build/host_build.log"

cat > "$SUMMARY_CSV" <<'EOF'
lane,probe_group,probe_name,status,profile,mode,elements,local_size,workgroups_x,build_ns,dispatch_ns,kernel_ns,read_ns,checksum,device_name,platform_name,artifact
EOF

ssh "$HOST" "rm -rf '$REMOTE_WORK'; mkdir -p '$REMOTE_WORK/host' '$REMOTE_WORK/probes/r600'"
scp -q "$ROOT_DIR/host/terascale_opencl_probe_host.c" "$HOST:$REMOTE_WORK/host/"
ssh "$HOST" "mkdir -p '$REMOTE_WORK/probes/r600/opencl'"
for probe_file in "$ROOT_DIR"/probes/r600/opencl/*.cl; do
  scp -q "$probe_file" "$HOST:$REMOTE_WORK/probes/r600/opencl/"
done

ssh "$HOST" "if pkg-config --exists OpenCL 2>/dev/null; then \
  $HOST_CC $HOST_CFLAGS '$REMOTE_WORK/host/terascale_opencl_probe_host.c' -o '$REMOTE_WORK/terascale_opencl_probe_host' \$(pkg-config --cflags --libs OpenCL); \
else \
  $HOST_CC $HOST_CFLAGS '$REMOTE_WORK/host/terascale_opencl_probe_host.c' -o '$REMOTE_WORK/terascale_opencl_probe_host' -lOpenCL; \
fi" > "$BUILD_LOG" 2>&1

append_fail_csv() {
  probe_group="$1"
  probe_name="$2"
  artifact="$3"
  printf '%s,%s,%s,%s,%s,%s,,,,,,,,,,%s\n' \
    "opencl" "$probe_group" "$probe_name" "fail" "$PROFILE" "" "$artifact" >> "$SUMMARY_CSV"
}

run_probe() {
  probe_name="$1"
  probe_group="$2"
  mode="$3"
  elements="$4"
  local_size="$5"
  src="$REMOTE_WORK/probes/r600/opencl/$probe_name.cl"
  raw="$OUT_DIR/runs/$probe_name.raw.log"
  err="$OUT_DIR/runs/$probe_name.stderr.log"

  build_opts_arg=""
  if [ -n "$OPENCL_BUILD_OPTIONS" ]; then
    build_opts_arg=" --build-options '$OPENCL_BUILD_OPTIONS'"
  fi

  if ssh "$HOST" "'$REMOTE_WORK/terascale_opencl_probe_host' --source '$src' --kernel '$probe_name' --elements '$elements' --local-size '$local_size' --mode '$mode'$build_opts_arg" > "$raw" 2> "$err"; then
    if ! python3 - "$raw" "$probe_group" "$probe_name" "$PROFILE" >> "$SUMMARY_CSV" <<'PY'
import json
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text()
start = text.find("{")
end = text.rfind("}")
if start == -1 or end == -1 or end < start:
    raise SystemExit(1)
payload = json.loads(text[start : end + 1])
probe_group = sys.argv[2]
probe_name = sys.argv[3]
profile = sys.argv[4]
row = [
    "opencl",
    probe_group,
    probe_name,
    "ok",
    profile,
    payload["mode"],
    str(payload["elements"]),
    str(payload["local_size"]),
    str(payload.get("workgroups_x", "")),
    str(payload["build_ns"]),
    "",
    str(payload["kernel_ns"]),
    str(payload["read_ns"]),
    f"{payload['checksum']:.9f}",
    payload["device_name"],
    payload["platform_name"],
    sys.argv[1],
]
print(",".join(row))
PY
    then
      append_fail_csv "$probe_group" "$probe_name" "$raw"
    fi
  else
    append_fail_csv "$probe_group" "$probe_name" "$raw"
  fi
}

run_probe "depchain_add" "depchain_add" "scalar_f32" "4096" "64"
run_probe "depchain_mul" "depchain_mul" "scalar_f32" "4096" "64"
run_probe "depchain_u8_add" "depchain_u8" "scalar_f32" "4096" "64"
run_probe "depchain_i8_add" "depchain_i8" "scalar_f32" "4096" "64"
run_probe "depchain_u16_add" "depchain_u16" "scalar_f32" "4096" "64"
run_probe "depchain_i16_add" "depchain_i16" "scalar_f32" "4096" "64"
run_probe "depchain_u32_add" "depchain_u32" "scalar_f32" "4096" "64"
run_probe "depchain_i32_add" "depchain_i32" "scalar_f32" "4096" "64"
run_probe "depchain_u64_add" "depchain_u64" "scalar_f32" "4096" "64"
run_probe "depchain_i64_add" "depchain_i64" "scalar_f32" "4096" "64"
run_probe "depchain_uint24_mul" "depchain_uint24" "scalar_f32" "4096" "64"
run_probe "depchain_int24_mul" "depchain_int24" "scalar_f32" "4096" "64"
run_probe "throughput_vec4_mad" "throughput_vec4" "vec4_f32" "4096" "64"
run_probe "throughput_uvec4_mad24" "throughput_uvec4" "vec4_f32" "4096" "64"
run_probe "throughput_ivec4_mad24" "throughput_ivec4" "vec4_f32" "4096" "64"
run_probe "throughput_u8_packed_mad" "throughput_u8_packed" "vec4_f32" "4096" "64"
run_probe "throughput_i8_packed_mad" "throughput_i8_packed" "vec4_f32" "4096" "64"
run_probe "occupancy_gpr_04" "occupancy_gpr_04" "scalar_f32" "2048" "64"
run_probe "occupancy_gpr_08" "occupancy_gpr_08" "scalar_f32" "2048" "64"
run_probe "occupancy_gpr_16" "occupancy_gpr_16" "scalar_f32" "2048" "64"
run_probe "occupancy_gpr_24" "occupancy_gpr_24" "scalar_f32" "2048" "64"
run_probe "occupancy_gpr_32" "occupancy_gpr_32" "scalar_f32" "2048" "64"
run_probe "occupancy_gpr_48" "occupancy_gpr_48" "scalar_f32" "2048" "64"
run_probe "occupancy_gpr_64" "occupancy_gpr_64" "scalar_f32" "2048" "64"
run_probe "occupancy_gpr_96" "occupancy_gpr_96" "scalar_f32" "2048" "64"

if [ -n "$VK_RESULTS_CSV" ]; then
  python3 "$SCRIPT_DIR/compare_terascale_lane_results.py" \
    --vulkan-csv "$VK_RESULTS_CSV" \
    --opencl-csv "$SUMMARY_CSV" \
    --output "$OUT_DIR/lane_compare.csv"
fi

ssh "$HOST" "rm -rf '$REMOTE_WORK'" >/dev/null 2>&1 || true

python3 - <<PY > "$MANIFEST_JSON"
import json
payload = {
    "host": "$HOST",
    "remote_work": "$REMOTE_WORK",
    "build_log": "$BUILD_LOG",
    "summary_csv": "$SUMMARY_CSV",
    "vk_results_csv": "$VK_RESULTS_CSV",
    "profile": "$PROFILE",
    "opencl_build_options": "$OPENCL_BUILD_OPTIONS",
    "host_cflags": "$HOST_CFLAGS",
}
print(json.dumps(payload, indent=2))
PY

printf '%s\n' "$OUT_DIR"
