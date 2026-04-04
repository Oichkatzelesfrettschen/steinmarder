#!/usr/bin/env python3
"""
Build the first-pass TeraScale project state tables.

This script inventories:
- the steinmarder repo's open-format document corpus
- the remote x130e home/doc corpus under /home/nick

It emits normalized CSV tables in src/sass_re/tables/ so the x130e / TeraScale
track has a machine-readable control plane instead of a loose pile of notes.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import subprocess
import sys
from collections import Counter
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path


SASS_RE_ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = SASS_RE_ROOT.parents[1]
TABLES_DIR = SASS_RE_ROOT / "tables"

LOCAL_EXCLUDED_DIRS = {
    ".git",
    ".venv",
    "__pycache__",
    "node_modules",
}

REMOTE_EXCLUDED_DIRS = {
    ".cache",
    ".cargo",
    ".local",
    ".npm",
    ".rustup",
    ".steam",
    ".vscode-server",
    "__pycache__",
}

DOC_EXTENSIONS = {
    ".cfg",
    ".conf",
    ".csv",
    ".ini",
    ".json",
    ".log",
    ".md",
    ".pdf",
    ".qpa",
    ".rst",
    ".svg",
    ".tex",
    ".toml",
    ".tsv",
    ".txt",
    ".xml",
    ".yaml",
    ".yml",
}

TEXTISH_EXTENSIONS = DOC_EXTENSIONS - {".pdf"}

REPO_FIELDS = [
    "corpus",
    "path",
    "relative_path",
    "extension",
    "size_bytes",
    "mtime_iso",
    "classification",
    "ingest_tier",
    "source_track",
    "project_alignment",
    "format_group",
    "title_hint",
]

REMOTE_FIELDS = [
    "corpus",
    "host",
    "path",
    "relative_path",
    "extension",
    "size_bytes",
    "mtime_iso",
    "classification",
    "ingest_tier",
    "source_track",
    "project_alignment",
    "format_group",
    "title_hint",
]


@dataclass(frozen=True)
class LocalRecord:
    corpus: str
    path: str
    relative_path: str
    extension: str
    size_bytes: int
    mtime_iso: str
    classification: str
    ingest_tier: str
    source_track: str
    project_alignment: str
    format_group: str
    title_hint: str


@dataclass(frozen=True)
class RemoteRecord:
    corpus: str
    host: str
    path: str
    relative_path: str
    extension: str
    size_bytes: int
    mtime_iso: str
    classification: str
    ingest_tier: str
    source_track: str
    project_alignment: str
    format_group: str
    title_hint: str


def is_textlike_no_extension(path: Path) -> bool:
    try:
        with path.open("rb") as handle:
            sample = handle.read(2048)
    except OSError:
        return False
    if not sample:
        return False
    if b"\x00" in sample:
        return False
    printable = sum(
        1
        for byte in sample
        if byte in b"\t\n\r" or 32 <= byte <= 126 or byte >= 128
    )
    return printable / max(len(sample), 1) >= 0.95


def title_hint_from_text(path: Path) -> str:
    try:
        with path.open("r", encoding="utf-8", errors="replace") as handle:
            for line in handle:
                stripped = line.strip()
                if stripped:
                    return stripped[:160]
    except OSError:
        return ""
    return ""


def iso_mtime(timestamp: float) -> str:
    return datetime.fromtimestamp(timestamp, tz=timezone.utc).isoformat()


def format_group_for_extension(ext: str) -> str:
    if ext == ".pdf":
        return "reference-pdf"
    if ext in {".md", ".rst", ".tex", ".txt"}:
        return "prose"
    if ext in {".csv", ".tsv"}:
        return "tabular"
    if ext in {".json", ".yaml", ".yml", ".toml", ".ini", ".conf"}:
        return "config"
    if ext in {".xml", ".qpa", ".log"}:
        return "trace"
    if ext == ".svg":
        return "vector"
    return "text"


def classify_repo_path(rel_path: str, ext: str) -> tuple[str, str, str, str]:
    parts = rel_path.split("/")
    top = parts[0]
    format_group = format_group_for_extension(ext)

    if top == "src" and len(parts) > 1 and parts[1] == "sass_re":
        source_track = "sass-re"
    elif top == "src" and len(parts) > 1 and parts[1] == "cuda_lbm":
        source_track = "cuda-lbm"
    elif top == "docs":
        source_track = "repo-docs"
    else:
        source_track = "repo-root"

    project_alignment = "canonical" if rel_path.startswith("src/sass_re/") else "supporting"

    if "results/" in rel_path or rel_path.startswith("results/"):
        if ext in {".csv", ".tsv"}:
            return "results-table", "structured-read", source_track, project_alignment
        if ext in {".xml", ".qpa", ".log"}:
            return "machine-trace", "mine", source_track, project_alignment
        return "machine-summary", "full-read", source_track, project_alignment

    if ext == ".pdf":
        return "reference-pdf", "extract-text", source_track, project_alignment
    if ext in {".json", ".yaml", ".yml", ".toml", ".ini", ".conf"}:
        return "config-spec", "structured-read", source_track, project_alignment
    if ext in {".csv", ".tsv"}:
        return "results-table", "structured-read", source_track, project_alignment
    if ext in {".xml", ".qpa", ".log"}:
        return "machine-trace", "mine", source_track, project_alignment
    return "authored-doc", "full-read", source_track, project_alignment


def inventory_repo_docs() -> list[LocalRecord]:
    records: list[LocalRecord] = []

    for root, dirs, files in os.walk(REPO_ROOT):
        dirs[:] = [
            d
            for d in dirs
            if d not in LOCAL_EXCLUDED_DIRS
            and not (root == str(REPO_ROOT) and d == "build")
        ]
        for file_name in files:
            path = Path(root) / file_name
            rel_path = path.relative_to(REPO_ROOT).as_posix()
            ext = path.suffix.lower()
            include = ext in DOC_EXTENSIONS or (not ext and is_textlike_no_extension(path))
            if not include:
                continue

            stat = path.stat()
            classification, ingest_tier, source_track, project_alignment = classify_repo_path(rel_path, ext)
            records.append(
                LocalRecord(
                    corpus="repo-doc-corpus",
                    path=str(path),
                    relative_path=rel_path,
                    extension=ext or "[none]",
                    size_bytes=stat.st_size,
                    mtime_iso=iso_mtime(stat.st_mtime),
                    classification=classification,
                    ingest_tier=ingest_tier,
                    source_track=source_track,
                    project_alignment=project_alignment,
                    format_group=format_group_for_extension(ext),
                    title_hint=title_hint_from_text(path) if ext != ".pdf" else "",
                )
            )
    return sorted(records, key=lambda r: r.relative_path)


REMOTE_SCRIPT = r"""
import json
import os
import sys
from datetime import datetime, timezone

ROOT = __ROOT__
EXCLUDED = set(__EXCLUDED__)
DOC_EXTS = set(__DOC_EXTS__)

def iso_mtime(ts):
    return datetime.fromtimestamp(ts, tz=timezone.utc).isoformat()

def looks_textlike(path):
    try:
        with open(path, "rb") as handle:
            sample = handle.read(2048)
    except OSError:
        return False
    if not sample or b"\x00" in sample:
        return False
    printable = sum(
        1
        for b in sample
        if b in b"\t\n\r" or 32 <= b <= 126 or b >= 128
    )
    return printable / max(len(sample), 1) >= 0.95

def title_hint(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            for line in handle:
                stripped = line.strip()
                if stripped:
                    return stripped[:160]
    except OSError:
        return ""
    return ""

def format_group(ext):
    if ext == ".pdf":
        return "reference-pdf"
    if ext in {".md", ".rst", ".tex", ".txt"}:
        return "prose"
    if ext in {".csv", ".tsv"}:
        return "tabular"
    if ext in {".json", ".yaml", ".yml", ".toml", ".ini", ".conf"}:
        return "config"
    if ext in {".xml", ".qpa", ".log"}:
        return "trace"
    if ext == ".svg":
        return "vector"
    return "text"

def classify(rel_path, ext):
    norm = rel_path
    if norm.startswith("eric/TerakanMesa"):
        track = "r600-terascale"
        alignment = "canonical"
    elif norm.startswith("mesa-26-debug"):
        track = "mesa-source"
        alignment = "implementation-source"
    elif norm.startswith("deqp"):
        track = "deqp"
        alignment = "validation-lane"
    elif norm.startswith("dxvk"):
        track = "dxvk"
        alignment = "validation-lane"
    elif norm.startswith(".claude"):
        track = "agent-state"
        alignment = "agent-state"
    elif norm.startswith("eric/analysis") or norm.startswith("eric/docs"):
        track = "system-analysis"
        alignment = "graphics-archive"
    elif norm.startswith("eric/mesa-bench") or norm.startswith("eric/mesa-target-info") or norm.startswith("eric/mesa-vulkan-audit"):
        track = "r600-terascale"
        alignment = "graphics-archive"
    elif norm.startswith("eric/scripts") or norm.startswith("eric/extra-debs"):
        track = "r600-terascale"
        alignment = "graphics-archive"
    elif norm.startswith("eric/1_media") or norm.startswith("eric/archive") or norm.startswith("eric/cinnamon-backup") or norm.startswith("eric/display-fix-backup") or norm.startswith("eric/dotfiles"):
        track = "machine-history"
        alignment = "archive-only"
    elif ext == ".pdf":
        track = "graphics-reference"
        alignment = "canonical-reference"
    else:
        track = "home-straggler"
        alignment = "review"

    if ext == ".pdf":
        return "reference-pdf", "extract-text", track, alignment
    if ext in {".json", ".yaml", ".yml", ".toml", ".ini", ".conf"}:
        return "config-spec", "structured-read", track, alignment
    if ext in {".csv", ".tsv"}:
        return "results-table", "structured-read", track, alignment
    if ext in {".xml", ".qpa", ".log"}:
        return "machine-trace", "mine", track, alignment
    if "results" in norm or "findings" in norm:
        return "machine-summary", "full-read", track, alignment
    return "authored-doc", "full-read", track, alignment

rows = []
for root, dirs, files in os.walk(ROOT):
    dirs[:] = [d for d in dirs if d not in EXCLUDED]
    for file_name in files:
        path = os.path.join(root, file_name)
        rel_path = os.path.relpath(path, ROOT).replace(os.sep, "/")
        ext = os.path.splitext(file_name)[1].lower()
        include = ext in DOC_EXTS
        if not include and not ext:
            include = looks_textlike(path)
        if not include:
            continue
        try:
            st = os.stat(path)
        except OSError:
            continue
        classification, ingest_tier, source_track, project_alignment = classify(rel_path, ext)
        rows.append({
            "corpus": "x130e-home-corpus",
            "host": "x130e",
            "path": path,
            "relative_path": rel_path,
            "extension": ext or "[none]",
            "size_bytes": st.st_size,
            "mtime_iso": iso_mtime(st.st_mtime),
            "classification": classification,
            "ingest_tier": ingest_tier,
            "source_track": source_track,
            "project_alignment": project_alignment,
            "format_group": format_group(ext),
            "title_hint": "" if ext == ".pdf" else title_hint(path),
        })

rows.sort(key=lambda row: row["relative_path"])
json.dump(rows, sys.stdout)
"""


def inventory_remote_home(host: str, remote_root: str) -> list[RemoteRecord]:
    remote_script = (
        REMOTE_SCRIPT.replace("__ROOT__", json.dumps(remote_root))
        .replace("__EXCLUDED__", json.dumps(sorted(REMOTE_EXCLUDED_DIRS), separators=(",", ":")))
        .replace("__DOC_EXTS__", json.dumps(sorted(DOC_EXTENSIONS), separators=(",", ":")))
    )
    proc = subprocess.run(
        ["ssh", host, "python3", "-"],
        input=remote_script,
        text=True,
        capture_output=True,
        check=True,
    )
    raw_rows = json.loads(proc.stdout)
    return [RemoteRecord(**row) for row in raw_rows]


def write_csv(path: Path, fieldnames: list[str], rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def summarize_counter(rows: list[object], attr: str) -> str:
    counter = Counter(getattr(row, attr) for row in rows)
    return ", ".join(f"{key}={counter[key]}" for key in sorted(counter))


def print_summary(label: str, rows: list[object]) -> None:
    print(f"{label}: {len(rows)} rows")
    print(f"  classifications: {summarize_counter(rows, 'classification')}")
    print(f"  alignments:      {summarize_counter(rows, 'project_alignment')}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="x130e", help="SSH host for the x130e inventory")
    parser.add_argument("--remote-root", default="/home/nick", help="Remote root to inventory")
    args = parser.parse_args()

    repo_rows = inventory_repo_docs()
    remote_rows = inventory_remote_home(args.host, args.remote_root)

    repo_table = TABLES_DIR / "table_repo_doc_corpus.csv"
    remote_table = TABLES_DIR / "table_x130e_home_corpus.csv"
    remote_project_table = TABLES_DIR / "table_x130e_file_inventory.csv"

    write_csv(repo_table, REPO_FIELDS, [row.__dict__ for row in repo_rows])
    write_csv(remote_table, REMOTE_FIELDS, [row.__dict__ for row in remote_rows])

    project_rows = [
        row.__dict__
        for row in remote_rows
        if row.project_alignment in {
            "canonical",
            "canonical-reference",
            "graphics-archive",
            "implementation-source",
            "validation-lane",
            "agent-state",
            "review",
        }
    ]
    write_csv(remote_project_table, REMOTE_FIELDS, project_rows)

    print_summary("repo_doc_corpus", repo_rows)
    print_summary("x130e_home_corpus", remote_rows)
    print(f"wrote: {repo_table}")
    print(f"wrote: {remote_table}")
    print(f"wrote: {remote_project_table}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
