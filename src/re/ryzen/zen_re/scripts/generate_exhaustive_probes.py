#!/usr/bin/env python3
"""Generate exhaustive latency/throughput probes for every x86-64 instruction.

Phase 1: Scalar integer (probe_scalar_exhaustive.c) -- hand-written, this
          script validates it against instructions_zen4.csv and reports coverage.

Phase 2 (future): SIMD instructions -- this script will auto-generate the
          C probe files from the CSV definitions.

Usage:
    python3 generate_exhaustive_probes.py [--csv instructions_zen4.csv] [--check]
"""

import argparse
import csv
import os
import subprocess
import sys

SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
ZEN_RE_DIRECTORY = os.path.dirname(SCRIPT_DIRECTORY)
REPO_ROOT_DIRECTORY = os.path.abspath(os.path.join(ZEN_RE_DIRECTORY, '..', '..', '..', '..'))
BUILD_BINARY_DIRECTORY = os.path.join(REPO_ROOT_DIRECTORY, 'build', 'bin')

DEFAULT_CSV_PATH = os.path.join(ZEN_RE_DIRECTORY, 'instructions_zen4.csv')
PROBE_SOURCE_PATH = os.path.join(ZEN_RE_DIRECTORY, 'probe_scalar_exhaustive.c')


def load_instruction_definitions(csv_file_path):
    """Load instruction definitions from the CSV file."""
    instruction_entries = []
    with open(csv_file_path, 'r') as csv_file:
        csv_reader = csv.DictReader(csv_file)
        for row_data in csv_reader:
            mnemonic_value = row_data.get('mnemonic', '').strip()
            if not mnemonic_value or mnemonic_value.startswith('#'):
                continue
            instruction_entries.append({
                'mnemonic': mnemonic_value,
                'operand_types': row_data.get('operand_types', '').strip(),
                'simd_width': int(row_data.get('simd_width', '0').strip()),
                'category': row_data.get('category', '').strip(),
                'notes': row_data.get('notes', '').strip(),
            })
    return instruction_entries


def count_probe_coverage(probe_source_path):
    """Count how many instruction forms are measured in the probe C file."""
    measured_instruction_forms = []
    if not os.path.exists(probe_source_path):
        return measured_instruction_forms

    with open(probe_source_path, 'r') as source_file:
        source_content = source_file.read()

    # Count entries in the probe_table[] array
    in_table_section = False
    for source_line in source_content.split('\n'):
        stripped_line = source_line.strip()
        if 'probe_table[]' in stripped_line:
            in_table_section = True
            continue
        if in_table_section:
            if stripped_line.startswith('{NULL'):
                break
            if stripped_line.startswith('{'):
                # Parse: {"MNEMONIC", "operands", "category", ...}
                field_parts = stripped_line.strip('{},' ).split(',')
                if len(field_parts) >= 3:
                    mnemonic_field = field_parts[0].strip().strip('"')
                    operand_field = field_parts[1].strip().strip('"')
                    measured_instruction_forms.append(
                        f"{mnemonic_field} {operand_field}"
                    )
    return measured_instruction_forms


def build_probe(compiler_command='clang-22'):
    """Build the scalar exhaustive probe binary."""
    output_binary_path = os.path.join(BUILD_BINARY_DIRECTORY, 'zen_probe_scalar_exhaustive')
    os.makedirs(BUILD_BINARY_DIRECTORY, exist_ok=True)

    compile_command = [
        compiler_command,
        '-D_GNU_SOURCE',
        '-O2',
        '-march=native',
        '-fno-omit-frame-pointer',
        '-include', 'x86intrin.h',
        '-I', ZEN_RE_DIRECTORY,
        os.path.join(ZEN_RE_DIRECTORY, 'common.c'),
        PROBE_SOURCE_PATH,
        '-lm',
        '-o', output_binary_path,
    ]

    print(f"Building: {' '.join(compile_command)}")
    build_result = subprocess.run(compile_command, capture_output=True, text=True)
    if build_result.returncode != 0:
        print(f"Build FAILED:\n{build_result.stderr}", file=sys.stderr)
        return None
    print(f"Built: {output_binary_path}")
    return output_binary_path


def run_probe(binary_path, iteration_count=100000, cpu_affinity=0):
    """Run the probe binary and return stdout."""
    run_command = [binary_path, '--iterations', str(iteration_count),
                   '--cpu', str(cpu_affinity), '--csv']
    print(f"Running: {' '.join(run_command)}")
    run_result = subprocess.run(run_command, capture_output=True, text=True, timeout=300)
    if run_result.returncode != 0:
        print(f"Run FAILED:\n{run_result.stderr}", file=sys.stderr)
        return None
    return run_result.stdout


def report_coverage(csv_file_path, probe_source_path):
    """Report coverage statistics."""
    all_definitions = load_instruction_definitions(csv_file_path)
    measured_forms = count_probe_coverage(probe_source_path)

    scalar_definition_count = sum(
        1 for definition in all_definitions if definition['simd_width'] == 0
    )
    total_definition_count = len(all_definitions)
    measured_form_count = len(measured_forms)

    print("=" * 60)
    print("INSTRUCTION COVERAGE REPORT")
    print("=" * 60)
    print(f"CSV definitions (scalar):  {scalar_definition_count}")
    print(f"CSV definitions (total):   {total_definition_count}")
    print(f"Probe measurements:        {measured_form_count}")
    print(f"Coverage (of scalar):      {measured_form_count}/{scalar_definition_count}"
          f" ({100.0*measured_form_count/max(1,scalar_definition_count):.1f}%)")
    print()

    # Group by category
    category_definition_counts = {}
    for definition in all_definitions:
        category_name = definition['category']
        category_definition_counts[category_name] = \
            category_definition_counts.get(category_name, 0) + 1

    print("By category (definitions in CSV):")
    for category_name, definition_count in sorted(category_definition_counts.items()):
        print(f"  {category_name:12s}: {definition_count}")

    print()
    print("Measured instruction forms:")
    for measured_form in measured_forms:
        print(f"  {measured_form}")


def main():
    argument_parser = argparse.ArgumentParser(
        description='Generate and validate exhaustive x86-64 instruction probes'
    )
    argument_parser.add_argument(
        '--csv', default=DEFAULT_CSV_PATH,
        help='Path to instruction definition CSV'
    )
    argument_parser.add_argument(
        '--check', action='store_true',
        help='Check coverage without building/running'
    )
    argument_parser.add_argument(
        '--build', action='store_true',
        help='Build the probe binary'
    )
    argument_parser.add_argument(
        '--run', action='store_true',
        help='Build and run the probe, output results'
    )
    argument_parser.add_argument(
        '--compiler', default='clang-22',
        help='Compiler to use (default: clang-22)'
    )
    argument_parser.add_argument(
        '--iterations', type=int, default=100000,
        help='Number of measurement iterations'
    )

    parsed_arguments = argument_parser.parse_args()

    if parsed_arguments.check:
        report_coverage(parsed_arguments.csv, PROBE_SOURCE_PATH)
        return

    if parsed_arguments.build or parsed_arguments.run:
        built_binary_path = build_probe(parsed_arguments.compiler)
        if not built_binary_path:
            sys.exit(1)

        if parsed_arguments.run:
            probe_output = run_probe(
                built_binary_path,
                iteration_count=parsed_arguments.iterations
            )
            if probe_output:
                print(probe_output)
        return

    # Default: just report coverage
    report_coverage(parsed_arguments.csv, PROBE_SOURCE_PATH)


if __name__ == '__main__':
    main()
