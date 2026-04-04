#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass
from datetime import UTC, datetime
from typing import Any


@dataclass(frozen=True)
class ToolProbe:
    name: str
    command: list[str]


def run_command(cmd: list[str]) -> tuple[int, str, str]:
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    return proc.returncode, proc.stdout.strip(), proc.stderr.strip()


def find_tool(name: str) -> str | None:
    return shutil.which(name)


def probe_path_tools(names: list[str]) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for name in names:
        path = find_tool(name)
        result[name] = {"available": path is not None, "path": path}
    return result


def probe_metal_tool(name: str) -> dict[str, Any]:
    code, out, err = run_command(["xcrun", "--toolchain", "Metal", "--find", name])
    if code == 0 and out:
        return {"available": True, "path": out}
    return {"available": False, "path": None, "error": err or out}


def command_version(cmd: list[str]) -> str | None:
    try:
        code, out, err = run_command(cmd)
    except FileNotFoundError:
        return None
    if code != 0:
        text = err if err else out
        return text.splitlines()[0] if text else None
    text = out if out else err
    return text.splitlines()[0] if text else None


def main() -> int:
    apple_tools = [
        "xcodebuild",
        "xcode-select",
        "xcrun",
        "xctrace",
        "dtrace",
        "dtruss",
        "sample",
        "spindump",
        "powermetrics",
        "leaks",
        "vmmap",
        "fs_usage",
        "lldb",
    ]
    nvidia_tools = [
        "ncu",
        "nsys",
        "nvcc",
        "cuobjdump",
        "nvdisasm",
        "compute-sanitizer",
        "nvidia-smi",
    ]
    amd_tools = ["uprof", "rocprof", "rocprofv2", "rocm-smi"]
    unix_tools = ["valgrind", "bpftrace", "perf", "strace", "ltrace", "gdb", "hyperfine"]

    report: dict[str, Any] = {
        "generated_at_utc": datetime.now(UTC).isoformat(),
        "host": {
            "platform": platform.platform(),
            "machine": platform.machine(),
            "python": sys.version.split()[0],
            "cwd": os.getcwd(),
            "sudo_askpass": os.environ.get("SUDO_ASKPASS"),
        },
        "toolchains": {
            "apple": probe_path_tools(apple_tools),
            "metal": {
                "metal": probe_metal_tool("metal"),
                "metallib": probe_metal_tool("metallib"),
            },
            "nvidia": probe_path_tools(nvidia_tools),
            "amd": probe_path_tools(amd_tools),
            "unix": probe_path_tools(unix_tools),
        },
        "package_managers": probe_path_tools(["brew", "port", "pkgutil"]),
        "versions": {
            "xcodebuild": command_version(["xcodebuild", "-version"]),
            "xcode_select": command_version(["xcode-select", "-p"]),
            "xctrace": command_version(["xctrace", "version"]),
            "brew": command_version(["brew", "--version"]),
            "port": command_version(["port", "version"]),
            "clang": command_version(["clang", "--version"]),
            "python3": command_version(["python3", "--version"]),
        },
        "substitution_map": {
            "ncu_or_nsys_class": "xctrace + app timing CSV + powermetrics",
            "uprof_class": "xctrace Time Profiler + sample + spindump",
            "valgrind_class": "ASan/UBSan + leaks + vmmap",
        },
    }

    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
