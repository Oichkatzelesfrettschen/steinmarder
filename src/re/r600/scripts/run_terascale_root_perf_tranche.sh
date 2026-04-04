#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
HOST="${HOST:-x130e}"
PROMPT_HOST="${PROMPT_HOST:-x130e-x11}"
OUT_DIR="${1:-$ROOT_DIR/results/terascale_root_perf_tranche_$(date +%Y%m%d_%H%M%S)}"
REMOTE_WORK="${REMOTE_WORK:-/tmp/terascale_root_perf_$$}"
REMOTE_ASKPASS="${REMOTE_ASKPASS:-/home/eirikr/.local/bin/sudo-askpass}"
REMOTE_MESA="${REMOTE_MESA:-/home/eirikr/workspaces/mesa/mesa-26-debug}"
REMOTE_BUILD="${REMOTE_BUILD:-$REMOTE_MESA/build-terakan-distcc}"
REMOTE_ICD="${REMOTE_ICD:-$REMOTE_BUILD/src/amd/terascale/vulkan/terascale_devenv_icd.x86_64.json}"

mkdir -p "$OUT_DIR/artifacts"

STATUS_CSV="$OUT_DIR/step_status.csv"
MANIFEST_JSON="$OUT_DIR/run_manifest.json"
REMOTE_STDOUT="$OUT_DIR/remote_driver.stdout.log"
REMOTE_STDERR="$OUT_DIR/remote_driver.stderr.log"

printf '%s\n' 'timestamp_utc,step,status,artifact' > "$STATUS_CSV"

printf '[01] root_perf_stack\n'
if ssh "$PROMPT_HOST" "export SUDO_ASKPASS='$REMOTE_ASKPASS'; sudo -A env REMOTE_WORK='$REMOTE_WORK' REMOTE_ICD='$REMOTE_ICD' sh -se" >"$REMOTE_STDOUT" 2>"$REMOTE_STDERR" <<'EOSUDO'
set -eu
mkdir -p "$REMOTE_WORK"
id > "$REMOTE_WORK/root_id.txt"
uname -a > "$REMOTE_WORK/uname.txt"
cat /proc/sys/kernel/perf_event_paranoid > "$REMOTE_WORK/perf_event_paranoid.txt"
perf version > "$REMOTE_WORK/perf_version.txt"

perf stat -x, -o "$REMOTE_WORK/perf_stat_sleep.csv" -a \
  -e cycles,instructions,branches,branch-misses,cache-misses \
  -- sleep 1

env VK_ICD_FILENAMES="$REMOTE_ICD" \
  perf stat -x, -o "$REMOTE_WORK/perf_stat_vulkaninfo.csv" \
  -- vulkaninfo --summary \
  > "$REMOTE_WORK/vulkaninfo.stdout" 2> "$REMOTE_WORK/vulkaninfo.stderr" || true

perf stat -x, -o "$REMOTE_WORK/perf_stat_clinfo.csv" \
  -- clinfo \
  > "$REMOTE_WORK/clinfo.stdout" 2> "$REMOTE_WORK/clinfo.stderr" || true

env VK_ICD_FILENAMES="$REMOTE_ICD" \
  perf record -o "$REMOTE_WORK/perf_vulkaninfo.data" --call-graph fp -- \
  vulkaninfo --summary \
  > "$REMOTE_WORK/perf_vulkaninfo_record.stdout" 2> "$REMOTE_WORK/perf_vulkaninfo_record.stderr" || true

perf report --stdio -i "$REMOTE_WORK/perf_vulkaninfo.data" \
  > "$REMOTE_WORK/perf_vulkaninfo.report.txt" 2> "$REMOTE_WORK/perf_vulkaninfo.report.stderr" || true

perf record -o "$REMOTE_WORK/perf_clinfo.data" --call-graph fp -- \
  clinfo \
  > "$REMOTE_WORK/perf_clinfo_record.stdout" 2> "$REMOTE_WORK/perf_clinfo_record.stderr" || true

perf report --stdio -i "$REMOTE_WORK/perf_clinfo.data" \
  > "$REMOTE_WORK/perf_clinfo.report.txt" 2> "$REMOTE_WORK/perf_clinfo.report.stderr" || true
EOSUDO
then
  status="ok"
else
  status="fail"
fi
printf '%s,%s,%s,%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "root_perf_stack" "$status" "$OUT_DIR/artifacts" >> "$STATUS_CSV"

printf '[02] collect_remote_artifacts\n'
ssh "$HOST" "sudo -n chown -R eirikr:eirikr '$REMOTE_WORK'" >/dev/null 2>&1 || true
if ssh "$HOST" "test -d '$REMOTE_WORK' && tar -C '$REMOTE_WORK' -cf - ." | tar -C "$OUT_DIR/artifacts" -xf -; then
  status="ok"
else
  status="fail"
fi
printf '%s,%s,%s,%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "collect_remote_artifacts" "$status" "$OUT_DIR/artifacts" >> "$STATUS_CSV"

ssh "$HOST" "rm -rf '$REMOTE_WORK'" >/dev/null 2>&1 || true

python3 - <<PY > "$MANIFEST_JSON"
import json
payload = {
    "host": "$HOST",
    "prompt_host": "$PROMPT_HOST",
    "remote_work": "$REMOTE_WORK",
    "remote_icd": "$REMOTE_ICD",
    "status_csv": "$STATUS_CSV",
    "stdout_log": "$REMOTE_STDOUT",
    "stderr_log": "$REMOTE_STDERR",
}
print(json.dumps(payload, indent=2))
PY

printf '%s\n' "$OUT_DIR"
