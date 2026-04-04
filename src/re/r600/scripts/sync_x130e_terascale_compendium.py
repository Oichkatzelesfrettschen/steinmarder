#!/usr/bin/env python3
"""
Preserve the current x130e TeraScale/Terakan state into the local compendium.

This script is intentionally preservation-first:
- it inventories the remote Mesa checkout instead of mutating it
- it fetches a curated, decision-grade subset of findings
- it records enough metadata to recreate what was imported later
"""

from __future__ import annotations

import argparse
import hashlib
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "results" / "x130e_terascale" / "imports"
DEFAULT_HOST = "nick@x130e"
REMOTE_HOME = "/home/nick"
REMOTE_MESA = f"{REMOTE_HOME}/mesa-26-debug"
REMOTE_FINDINGS = f"{REMOTE_HOME}/eric/TerakanMesa/findings"


@dataclass(frozen=True)
class RemoteFile:
    remote_path: str
    relative_dest: str
    category: str


CURATED_FILES = [
    RemoteFile(f"{REMOTE_FINDINGS}/COMPREHENSIVE_SCOPE.md", "findings/COMPREHENSIVE_SCOPE.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/FULL_STACK_PROFILE_MESA26.md", "findings/FULL_STACK_PROFILE_MESA26.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/HW_BOUNDARY_MEASUREMENTS.md", "findings/HW_BOUNDARY_MEASUREMENTS.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/HW_CAPABILITIES.md", "findings/HW_CAPABILITIES.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/ISA_SLOT_ANALYSIS.md", "findings/ISA_SLOT_ANALYSIS.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/LATENCY_PROBE_RESULTS.md", "findings/LATENCY_PROBE_RESULTS.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/NOVEL_VLIW_PACKING.md", "findings/NOVEL_VLIW_PACKING.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/NUMERIC_PACKING_RESEARCH.md", "findings/NUMERIC_PACKING_RESEARCH.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/RUSTICL_BASELINE.md", "findings/RUSTICL_BASELINE.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/STUTTER_RCA_AND_FIX.md", "findings/STUTTER_RCA_AND_FIX.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/TERAKAN_CONFORMANCE_PLAN.md", "findings/TERAKAN_CONFORMANCE_PLAN.md", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/TERAKAN_DRIVER_MANIFEST.txt", "findings/TERAKAN_DRIVER_MANIFEST.txt", "finding"),
    RemoteFile(f"{REMOTE_FINDINGS}/TERAKAN_PORT_STATUS.md", "findings/TERAKAN_PORT_STATUS.md", "finding"),
    RemoteFile(f"{REMOTE_HOME}/eric/TerakanMesa/re-toolkit/README.md", "tooling/re-toolkit_README.md", "tooling"),
    RemoteFile(f"{REMOTE_HOME}/eric/TerakanMesa/NOVEL_VLIW_PACKING.md", "tooling/NOVEL_VLIW_PACKING_root.md", "tooling"),
]


INVENTORY_COMMANDS = {
    "remote_mesa_head.txt": (
        f"cd {REMOTE_MESA} && "
        "printf 'HEAD ' && git rev-parse HEAD && "
        "printf '\\nBRANCH ' && (git symbolic-ref --short -q HEAD || echo detached)"
    ),
    "remote_mesa_status.txt": (
        f"cd {REMOTE_MESA} && "
        "git status --short"
    ),
    "remote_terakan_tree.txt": (
        f"find {REMOTE_MESA}/src/amd/terascale -maxdepth 4 -type f 2>/dev/null | sort"
    ),
    "remote_findings_inventory.txt": (
        f"find {REMOTE_FINDINGS} -maxdepth 2 -type f | sort"
    ),
    "remote_findings_hashes.txt": (
        f"find {REMOTE_FINDINGS} -maxdepth 2 -type f "
        "-exec sha256sum {} + 2>/dev/null | sort"
    ),
}


def run_remote(host: str, remote_cmd: str) -> str:
    ssh_remote_cmd = (
        "ssh -o BatchMode=yes -o ConnectTimeout=10 "
        f"{shlex.quote(host)} "
        f"{shlex.quote(remote_cmd)}"
    )
    proc = subprocess.run(
        ssh_remote_cmd,
        check=True,
        shell=True,
        executable="/bin/zsh",
        text=True,
        capture_output=True,
    )
    return proc.stdout


def fetch_remote_file(host: str, remote_path: str) -> bytes:
    remote_cat_cmd = f"cat {shlex.quote(remote_path)}"
    ssh_remote_cmd = (
        "ssh -o BatchMode=yes -o ConnectTimeout=10 "
        f"{shlex.quote(host)} "
        f"{shlex.quote(remote_cat_cmd)}"
    )
    proc = subprocess.run(
        ssh_remote_cmd,
        check=True,
        shell=True,
        executable="/bin/zsh",
        capture_output=True,
    )
    return proc.stdout


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def write_bytes(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=DEFAULT_HOST, help="SSH target")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Base output directory for imported snapshots",
    )
    parser.add_argument(
        "--snapshot-name",
        default=None,
        help="Optional stable snapshot directory name",
    )
    args = parser.parse_args()

    snapshot_name = args.snapshot_name or subprocess.run(
        ["date", "+%Y%m%d_%H%M%S"],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.strip()

    snapshot_dir = args.output_dir / snapshot_name
    snapshot_dir.mkdir(parents=True, exist_ok=True)

    manifest_rows = [
        "kind\trelative_path\tsha256\tremote_source",
    ]

    for name, remote_cmd in INVENTORY_COMMANDS.items():
        output = run_remote(args.host, remote_cmd)
        encoded = output.encode("utf-8")
        write_text(snapshot_dir / "inventory" / name, output)
        manifest_rows.append(
            f"inventory\tinventory/{name}\t{sha256_bytes(encoded)}\t{remote_cmd}"
        )

    for item in CURATED_FILES:
        payload = fetch_remote_file(args.host, item.remote_path)
        write_bytes(snapshot_dir / item.relative_dest, payload)
        manifest_rows.append(
            f"{item.category}\t{item.relative_dest}\t{sha256_bytes(payload)}\t{item.remote_path}"
        )

    write_text(snapshot_dir / "SOURCE_MANIFEST.tsv", "\n".join(manifest_rows) + "\n")

    print(snapshot_dir)
    print(
        "note: if your SSH alias relies on flaky mDNS resolution, rerun with a concrete host "
        "like --host nick@10.0.0.88",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
