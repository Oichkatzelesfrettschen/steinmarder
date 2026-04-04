#!/usr/bin/env bash
set -euo pipefail

MODE="dry-run"
ALLOW_SUDO=0

usage() {
  cat <<'EOF'
Usage:
  migrate_x130e_workspace.sh [--dry-run] [--execute] [--sudo]

Behavior:
  - defaults to --dry-run
  - moves known x130e project roots from /home/nick into /home/eirikr
  - does not create compatibility symlinks
  - reports leftover /home/nick/eric content for manual review instead of
    guessing where non-project archives should land

Flags:
  --dry-run   print the planned moves and validation checks
  --execute   perform the moves
  --sudo      run directory creation and move operations via sudo
  -h, --help  show this help
EOF
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --dry-run)
      MODE="dry-run"
      ;;
    --execute)
      MODE="execute"
      ;;
    --sudo)
      ALLOW_SUDO=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'error: unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

SOURCE_HOME="/home/nick"
TARGET_HOME="/home/eirikr"

TARGET_DIRS=(
  "$TARGET_HOME/workspaces/mesa"
  "$TARGET_HOME/workspaces/research"
  "$TARGET_HOME/workspaces/research/x130e-eric-archive"
  "$TARGET_HOME/workspaces/research/x130e-eric-archive/root-files"
  "$TARGET_HOME/workspaces/tools"
  "$TARGET_HOME/artifacts/mesa/builds"
  "$TARGET_HOME/artifacts/mesa/stage"
  "$TARGET_HOME/artifacts/mesa/traces"
  "$TARGET_HOME/artifacts/mesa/conformance"
  "$TARGET_HOME/artifacts/mesa/benchmarks"
)

MOVE_SPECS=(
  "$SOURCE_HOME/mesa-26-debug|$TARGET_HOME/workspaces/mesa/mesa-26-debug|main Mesa 26 debug checkout with Terakan integration"
  "$SOURCE_HOME/deqp|$TARGET_HOME/workspaces/mesa/deqp|Vulkan CTS / dEQP working tree"
  "$SOURCE_HOME/dxvk|$TARGET_HOME/workspaces/mesa/dxvk|DXVK validation lane for Terakan/Zink follow-up"
  "$SOURCE_HOME/.claude|$TARGET_HOME/.claude|primary Claude project memory and local agent state"
  "$SOURCE_HOME/eric/TerakanMesa|$TARGET_HOME/workspaces/research/TerakanMesa|raw TeraScale/Terakan findings, audits, toolkit, and staged Vulkan material"
  "$SOURCE_HOME/eric/mesa-bench|$TARGET_HOME/workspaces/research/x130e-eric-archive/mesa-bench|legacy Mesa benchmark notes"
  "$SOURCE_HOME/eric/mesa-target-info|$TARGET_HOME/workspaces/research/x130e-eric-archive/mesa-target-info|target inventory snapshots"
  "$SOURCE_HOME/eric/mesa-vulkan-audit|$TARGET_HOME/workspaces/research/x130e-eric-archive/mesa-vulkan-audit|prior Vulkan audit bundles"
  "$SOURCE_HOME/eric/scripts|$TARGET_HOME/workspaces/research/x130e-eric-archive/scripts|helper scripts from legacy depot"
  "$SOURCE_HOME/eric/analysis|$TARGET_HOME/workspaces/research/x130e-eric-archive/analysis|system analysis and profiling notes"
  "$SOURCE_HOME/eric/docs|$TARGET_HOME/workspaces/research/x130e-eric-archive/docs|legacy docs and tuning notes"
  "$SOURCE_HOME/eric/extra-debs|$TARGET_HOME/workspaces/research/x130e-eric-archive/extra-debs|captured Mesa package artifacts"
  "$SOURCE_HOME/eric/.claude|$TARGET_HOME/workspaces/research/x130e-eric-archive/eric-dot-claude|legacy Claude state stored under eric depot"
)

POST_MOVE_TARGETS=(
  "$TARGET_HOME/workspaces/mesa/mesa-26-debug"
  "$TARGET_HOME/workspaces/mesa/deqp"
  "$TARGET_HOME/workspaces/mesa/dxvk"
  "$TARGET_HOME/.claude"
  "$TARGET_HOME/workspaces/research/TerakanMesa"
  "$TARGET_HOME/workspaces/research/x130e-eric-archive"
)

run_cmd() {
  if [ "$ALLOW_SUDO" -eq 1 ]; then
    sudo "$@"
  else
    "$@"
  fi
}

human_size() {
  if path_exists "$1"; then
    if [ "$ALLOW_SUDO" -eq 1 ]; then
      sudo du -sh "$1" 2>/dev/null | awk '{print $1}'
    else
      du -sh "$1" 2>/dev/null | awk '{print $1}'
    fi
  else
    printf 'missing'
  fi
}

path_exists() {
  if [ "$ALLOW_SUDO" -eq 1 ]; then
    sudo test -e "$1"
  else
    test -e "$1"
  fi
}

report_move_spec() {
  local src="$1"
  local dst="$2"
  local note="$3"
  local src_state="missing"
  local dst_state="missing"
  local src_size="missing"
  local dst_parent
  dst_parent="$(dirname "$dst")"

  if path_exists "$src"; then
    src_state="present"
    src_size="$(human_size "$src")"
  fi
  if [ -e "$dst" ]; then
    dst_state="exists"
  fi

  printf '%s\n' "MOVE"
  printf '  source: %s\n' "$src"
  printf '  target: %s\n' "$dst"
  printf '  note:   %s\n' "$note"
  printf '  source_state: %s\n' "$src_state"
  printf '  source_size:  %s\n' "$src_size"
  printf '  target_state: %s\n' "$dst_state"
  printf '  target_parent: %s\n' "$dst_parent"
}

ensure_target_dirs() {
  for dir in "${TARGET_DIRS[@]}"; do
    if [ "$MODE" = "dry-run" ]; then
      printf 'MKDIR %s\n' "$dir"
    else
      run_cmd mkdir -p "$dir"
    fi
  done
}

execute_moves() {
  local spec src dst note dst_parent
  for spec in "${MOVE_SPECS[@]}"; do
    IFS='|' read -r src dst note <<EOF
$spec
EOF
    if ! path_exists "$src"; then
      printf 'SKIP missing source: %s\n' "$src"
      continue
    fi
    if [ -e "$dst" ]; then
      printf 'ERROR target already exists: %s\n' "$dst" >&2
      exit 1
    fi
    dst_parent="$(dirname "$dst")"
    run_cmd mkdir -p "$dst_parent"
    printf 'MV %s -> %s\n' "$src" "$dst"
    run_cmd mv "$src" "$dst"
  done
}

report_leftovers() {
  local reviewed
  reviewed=(
    "TerakanMesa"
    "mesa-bench"
    "mesa-target-info"
    "mesa-vulkan-audit"
    "scripts"
    "analysis"
    "docs"
    "extra-debs"
    ".claude"
  )

  printf '\nLEFTOVER /home/nick/eric ENTRIES FOR MANUAL REVIEW\n'
  printf '%s\n' '  entries listed here are not auto-moved by default'
  printf '%s\n' '  because they look like general archives, desktop backups, or non-graphics material'

  python3 - "$SOURCE_HOME/eric" "${reviewed[@]}" <<'PY'
import os
import sys

root = sys.argv[1]
reviewed = set(sys.argv[2:])

if not os.path.isdir(root):
    print("  source root missing:", root)
    raise SystemExit(0)

for name in sorted(os.listdir(root)):
    if name in reviewed:
        continue
    path = os.path.join(root, name)
    if os.path.isdir(path):
        print(f"  dir: {path}")
PY
}

report_eric_root_files() {
  python3 - "$SOURCE_HOME/eric" <<'PY'
import os
import sys

root = sys.argv[1]
if not os.path.isdir(root):
    print("\nERIC ROOT FILE PLAN")
    print("  source root missing:", root)
    raise SystemExit(0)

files = []
for name in sorted(os.listdir(root)):
    path = os.path.join(root, name)
    if os.path.isfile(path):
        files.append(path)

print("\nERIC ROOT FILE PLAN")
print("  target directory: /home/eirikr/workspaces/research/x130e-eric-archive/root-files")
if not files:
    print("  no standalone root files detected")
else:
    for path in files:
        print(f"  ROOTFILE {path}")
PY
}

execute_eric_root_files() {
  python3 - "$SOURCE_HOME/eric" "$TARGET_HOME/workspaces/research/x130e-eric-archive/root-files" "$ALLOW_SUDO" <<'PY'
import os
import shutil
import subprocess
import sys

src_root = sys.argv[1]
dst_root = sys.argv[2]
use_sudo = sys.argv[3] == "1"

if not os.path.isdir(src_root):
    print(f"SKIP missing source root: {src_root}")
    raise SystemExit(0)

files = []
for name in sorted(os.listdir(src_root)):
    path = os.path.join(src_root, name)
    if os.path.isfile(path):
        files.append(path)

if not files:
    print("SKIP no standalone eric root files detected")
    raise SystemExit(0)

cmd = ["mkdir", "-p", dst_root]
if use_sudo:
    cmd.insert(0, "sudo")
subprocess.run(cmd, check=True)

for src in files:
    dst = os.path.join(dst_root, os.path.basename(src))
    if os.path.exists(dst):
      print(f"ERROR target already exists: {dst}", file=sys.stderr)
      raise SystemExit(1)
    print(f"MV {src} -> {dst}")
    cmd = ["mv", src, dst]
    if use_sudo:
        cmd.insert(0, "sudo")
    subprocess.run(cmd, check=True)
PY
}

normalize_ownership() {
  local target
  for target in "${POST_MOVE_TARGETS[@]}"; do
    if [ ! -e "$target" ]; then
      continue
    fi
    if [ "$MODE" = "dry-run" ]; then
      printf 'CHOWN -R eirikr:eirikr %s\n' "$target"
    else
      printf 'CHOWN -R eirikr:eirikr %s\n' "$target"
      run_cmd chown -R eirikr:eirikr "$target"
    fi
  done
}

validate_targets() {
  local target
  printf '\nPOST-MIGRATION VALIDATION\n'
  for target in "${POST_MOVE_TARGETS[@]}"; do
    if [ "$MODE" = "dry-run" ]; then
      printf 'CHECK %s\n' "$target"
      continue
    fi
    if [ -e "$target" ]; then
      printf 'OK %s\n' "$target"
      ls -ld "$target"
    else
      printf 'MISSING %s\n' "$target"
    fi
  done
}

printf 'x130e workspace migration mode: %s\n' "$MODE"
printf 'use sudo: %s\n\n' "$ALLOW_SUDO"

printf 'TARGET DIRECTORY PLAN\n'
ensure_target_dirs

printf '\nMOVE PLAN\n'
for spec in "${MOVE_SPECS[@]}"; do
  IFS='|' read -r src dst note <<EOF
$spec
EOF
  report_move_spec "$src" "$dst" "$note"
done

report_leftovers
report_eric_root_files

if [ "$MODE" = "execute" ]; then
  printf '\nEXECUTING MOVES\n'
  execute_moves
  execute_eric_root_files
  printf '\nNORMALIZING OWNERSHIP\n'
  normalize_ownership
  validate_targets
  printf '\nMigration complete.\n'
else
  printf '\nOWNERSHIP NORMALIZATION PLAN\n'
  normalize_ownership
  validate_targets
  printf '\nDry run only. Re-run with --execute to perform the moves.\n'
fi
