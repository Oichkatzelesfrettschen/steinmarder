#!/usr/bin/env bash
# run_metal_flag_sweep.sh — Compare Metal AIR IR across optimization levels.
#
# Compiles each .metal shader with -O0, -O1, -O2 (default), and -Os,
# then diffs the AIR IR opcode counts to reveal what the optimizer does.
#
# Output artifacts:
#   <output_dir>/<probe>_<flag>.ll    — AIR LLVM IR per flag level
#   <output_dir>/flag_sweep_opcodes.csv — opcode counts per probe × flag
#   <output_dir>/flag_sweep_analysis.md — human-readable diff summary
#
# Usage:
#   ./run_metal_flag_sweep.sh [output_dir] [--shaders glob]
#
# Example:
#   ./run_metal_flag_sweep.sh /tmp/metal_flag_sweep
#   ./run_metal_flag_sweep.sh /tmp/sweep --shaders "probe_ffma*.metal"

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHADERS_DIR="$(cd "$SCRIPT_DIR/../shaders" && pwd)"

OUTPUT_DIR="${1:-/tmp/metal_flag_sweep_$(date +%Y%m%d_%H%M%S)}"
SHADER_GLOB="*.metal"

shift || true
while [[ $# -gt 0 ]]; do
    case "$1" in
        --shaders) SHADER_GLOB="$2"; shift 2 ;;
        *) echo "unknown arg: $1"; exit 1 ;;
    esac
done

mkdir -p "$OUTPUT_DIR"
echo "Output dir: $OUTPUT_DIR"
echo "Shaders:    $SHADERS_DIR/$SHADER_GLOB"

FLAGS=(-O0 -O1 -O2 -Os)

# Compile all shaders at each flag level, emit AIR LLVM IR
for shader_path in "$SHADERS_DIR"/$SHADER_GLOB; do
    [[ -f "$shader_path" ]] || continue
    probe="$(basename "${shader_path%.metal}")"
    for flag in "${FLAGS[@]}"; do
        flag_name="${flag//-/}"   # O0, O1, O2, Os
        out_ll="$OUTPUT_DIR/${probe}_${flag_name}.ll"
        xcrun -sdk macosx metal -S -emit-llvm "$flag" \
            -o "$out_ll" \
            "$shader_path" 2>&1 || {
            echo "  WARNING: $probe $flag compilation failed — skipping"
            continue
        }
    done
    echo "compiled: $probe (${FLAGS[*]})"
done

# Python analysis: count opcodes per (probe, flag) and produce CSV + MD
python3 - "$OUTPUT_DIR" <<'PYEOF'
import re, csv, pathlib, sys, collections

output_dir = pathlib.Path(sys.argv[1])

def extract_ops(ll_path):
    ops = collections.Counter()
    for line in ll_path.read_text().splitlines():
        line = line.strip()
        m = re.search(r'\btail call\b.*@(air\.[.\w]+|llvm\.[.\w]+)', line)
        if m:
            ops['call @' + m.group(1)] += 1; continue
        m = re.search(r'\bcall\b.*@(air\.[.\w]+|llvm\.[.\w]+)', line)
        if m:
            ops['call @' + m.group(1)] += 1; continue
        m = re.match(r'%\S+ = ([\w.]+)', line)
        if m:
            ops[m.group(1)] += 1; continue
        m = re.match(r'(store|br|ret|fence|switch|unreachable)\b', line)
        if m:
            ops[m.group(1)] += 1
    return ops

flags = ['O0', 'O1', 'O2', 'Os']
ll_files = sorted(output_dir.glob('*.ll'))

# Group by probe
probes = {}
for ll_file in ll_files:
    parts = ll_file.stem.rsplit('_', 1)
    if len(parts) == 2 and parts[1] in flags:
        probe, flag = parts
        probes.setdefault(probe, {})[flag] = ll_file

csv_rows = []
md_lines = [
    '# Metal Flag Sweep — AIR Opcode Count Analysis',
    '',
    f'- output_dir: `{output_dir}`',
    f'- flags: {", ".join(flags)}',
    '',
]

for probe in sorted(probes):
    flag_ops = {}
    for flag in flags:
        if flag in probes[probe]:
            flag_ops[flag] = extract_ops(probes[probe][flag])
    if not flag_ops:
        continue

    # Collect all ops seen in any flag level
    all_ops = set()
    for ops in flag_ops.values():
        all_ops.update(ops.keys())

    # Find ops that differ between O2 and other flags
    o2_ops = flag_ops.get('O2', collections.Counter())
    changed_ops = [op for op in all_ops if any(
        flag_ops.get(f, collections.Counter())[op] != o2_ops[op]
        for f in flags if f != 'O2'
    )]

    md_lines += [f'## {probe}', '']
    if changed_ops:
        md_lines.append('| Opcode | O0 | O1 | O2 | Os | O0→O2 delta |')
        md_lines.append('|--------|----|----|----|----|-------------|')
        for op in sorted(changed_ops, key=lambda x: -abs(
            flag_ops.get('O2', collections.Counter())[x] -
            flag_ops.get('O0', collections.Counter())[x]
        )):
            vals = [flag_ops.get(f, collections.Counter())[op] for f in flags]
            delta = vals[2] - vals[0]  # O2 - O0
            sign = '+' if delta > 0 else ''
            md_lines.append(f'| `{op}` | {vals[0]} | {vals[1]} | {vals[2]} | {vals[3]} | {sign}{delta} |')
        md_lines.append('')
    else:
        md_lines.append('No opcode count changes across flag levels.\n')

    # Total instruction count per flag
    totals = {f: sum(flag_ops.get(f, collections.Counter()).values()) for f in flags}
    total_row = ' | '.join(f'{totals.get(f, 0)}' for f in flags)
    md_lines += [f'Total instructions: O0={totals.get("O0","?")} | O1={totals.get("O1","?")} | O2={totals.get("O2","?")} | Os={totals.get("Os","?")}\n', '']

    for op in sorted(all_ops):
        row = {
            'probe': probe,
            'opcode': op,
        }
        for flag in flags:
            row[flag] = flag_ops.get(flag, collections.Counter())[op]
        csv_rows.append(row)

# Write CSV
csv_path = output_dir / 'flag_sweep_opcodes.csv'
if csv_rows:
    fieldnames = ['probe', 'opcode'] + flags
    with csv_path.open('w', newline='') as fh:
        w = csv.DictWriter(fh, fieldnames=fieldnames)
        w.writeheader()
        w.writerows(csv_rows)
    print(f'Wrote {csv_path} ({len(csv_rows)} rows)')
else:
    print('No data rows for CSV.')

# Write MD
md_path = output_dir / 'flag_sweep_analysis.md'
md_path.write_text('\n'.join(md_lines))
print(f'Wrote {md_path}')
PYEOF

echo ""
echo "=== Summary ==="
echo "AIR IR files: $OUTPUT_DIR/*.ll"
echo "Opcode CSV:   $OUTPUT_DIR/flag_sweep_opcodes.csv"
echo "Analysis MD:  $OUTPUT_DIR/flag_sweep_analysis.md"
