#!/usr/bin/env python3
"""
Emit a machine-readable manifest for the TeraScale / r600 probe corpus.

The immediate job is to inventory the imported GLSL + shader_runner probes and
reserve a stable schema for future Vulkan/OpenCL/OpenGL-compute lanes.
"""

from __future__ import annotations

import argparse
import csv
import json
import pathlib
from dataclasses import asdict, dataclass
from typing import Iterable


ROOT = pathlib.Path(__file__).resolve().parent.parent
PROBE_ROOT = ROOT / "probes" / "r600"


@dataclass
class Probe:
    probe_id: str
    relative_path: str
    lane: str
    api: str
    stage_or_mode: str
    source_format: str
    family: str
    migration_source: str
    runner_hint: str
    status: str


def classify_family(stem: str) -> str:
    if stem.startswith("latency_"):
        return "latency"
    if stem.startswith("throughput_"):
        return "throughput"
    if stem.startswith("depchain_"):
        return "latency"
    if stem.startswith("gpr_pressure_"):
        return "gpr_pressure"
    if stem.startswith("pressure_"):
        return "gpr_pressure"
    if stem.startswith("vliw5_"):
        return "vliw5_packing"
    if stem.startswith("cache_"):
        return "cache"
    if stem.startswith("tex_"):
        return "texture"
    if stem.startswith("alu_"):
        return "alu"
    if stem.startswith("clause_switch_"):
        return "clause_switch"
    if stem.startswith("trace_"):
        return "system_trace"
    return "misc"


def build_probe(path: pathlib.Path) -> Probe:
    rel = path.relative_to(ROOT).as_posix()
    stem = path.stem

    if "shader_tests/" in rel:
        lane = "shader_runner"
        api = "OpenGL"
        stage = "piglit_shader_runner"
        fmt = "shader_test"
        runner = "piglit shader_runner"
    elif "legacy_glsl/" in rel or path.suffix == ".frag":
        lane = "legacy_glsl"
        api = "OpenGL"
        stage = "fragment"
        fmt = "GLSL"
        runner = "shader_runner_or_gl_harness"
    elif "vulkan_compute/" in rel:
        lane = "vulkan_compute"
        api = "Vulkan"
        stage = "compute"
        fmt = path.suffix.lstrip(".") or "unknown"
        runner = "glslangvalidator_or_spirv_toolchain"
    elif "vulkan_graphics/" in rel:
        lane = "vulkan_graphics"
        api = "Vulkan"
        stage = "graphics"
        fmt = path.suffix.lstrip(".") or "unknown"
        runner = "glslangvalidator_or_spirv_toolchain"
    elif "opencl/" in rel:
        lane = "opencl"
        api = "OpenCL"
        stage = "kernel"
        fmt = path.suffix.lstrip(".") or "unknown"
        runner = "clinfo_or_custom_harness"
    elif "opengl_compute/" in rel:
        lane = "opengl_compute"
        api = "OpenGL"
        stage = "compute"
        fmt = path.suffix.lstrip(".") or "unknown"
        runner = "gl_compute_harness"
    elif "system/" in rel:
        lane = "system"
        api = "host_drm"
        stage = "trace"
        fmt = path.suffix.lstrip(".") or "unknown"
        runner = "trace_stack_runner"
    else:
        lane = "unknown"
        api = "unknown"
        stage = "unknown"
        fmt = path.suffix.lstrip(".") or "unknown"
        runner = "manual"

    migration_source = "x130e_re_toolkit" if ("legacy_glsl/" in rel or "shader_tests/" in rel or "legacy_manifest.json" in rel) else "sass_re_local"
    status = "imported" if migration_source == "x130e_re_toolkit" else "seeded"
    probe_id = rel.replace("/", "__").replace(".", "_")
    return Probe(
        probe_id=probe_id,
        relative_path=rel,
        lane=lane,
        api=api,
        stage_or_mode=stage,
        source_format=fmt,
        family=classify_family(stem),
        migration_source=migration_source,
        runner_hint=runner,
        status=status,
    )


def iter_probe_files() -> Iterable[pathlib.Path]:
    for ext in ("*.frag", "*.shader_test", "*.comp", "*.vert", "*.frag", "*.cl", "*.spv"):
        for path in sorted(PROBE_ROOT.rglob(ext)):
            yield path


def collect_probes() -> list[Probe]:
    seen: set[pathlib.Path] = set()
    probes: list[Probe] = []
    for path in iter_probe_files():
        if path in seen or not path.is_file():
            continue
        seen.add(path)
        probes.append(build_probe(path))
    return probes


def emit_csv(probes: list[Probe], delimiter: str, header: bool) -> None:
    writer = csv.DictWriter(
        __import__("sys").stdout,
        fieldnames=list(asdict(probes[0]).keys()) if probes else list(Probe.__annotations__.keys()),
        delimiter=delimiter,
    )
    if header:
        writer.writeheader()
    for probe in probes:
        writer.writerow(asdict(probe))


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="cmd", required=True)

    emit = subparsers.add_parser("emit")
    emit.add_argument("--format", choices=("json", "csv", "tsv"), default="json")
    emit.add_argument("--header", action="store_true")

    args = parser.parse_args()
    probes = collect_probes()

    if args.cmd == "emit":
        if args.format == "json":
            json.dump([asdict(p) for p in probes], __import__("sys").stdout, indent=2)
            print()
        elif args.format == "csv":
            emit_csv(probes, ",", args.header)
        else:
            emit_csv(probes, "\t", args.header)
        return 0

    return 1


if __name__ == "__main__":
    raise SystemExit(main())
