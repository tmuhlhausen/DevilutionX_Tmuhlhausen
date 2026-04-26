#!/usr/bin/env python3
"""Generate release artifact checksums and provenance metadata."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]


def git_output(*args: str) -> str:
    try:
        return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()
    except Exception:
        return "unknown"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def artifact_record(path: Path) -> dict[str, Any]:
    return {
        "path": str(path),
        "name": path.name,
        "size_bytes": path.stat().st_size,
        "sha256": sha256(path),
    }


def markdown_report(provenance: dict[str, Any]) -> str:
    lines = [
        "# Release Provenance",
        "",
        f"Generated: `{provenance['generated_at_utc']}`",
        "",
        "## Source",
        "",
        f"- Commit: `{provenance['source']['commit']}`",
        f"- Branch/ref: `{provenance['source']['ref']}`",
        f"- Dirty tree: `{provenance['source']['dirty']}`",
        "",
        "## Build environment",
        "",
        f"- System: `{provenance['environment']['system']}`",
        f"- Machine: `{provenance['environment']['machine']}`",
        f"- Python: `{provenance['environment']['python']}`",
        "",
        "## Artifacts",
        "",
        "| Artifact | Size | SHA-256 |",
        "|---|---:|---|",
    ]
    for artifact in provenance["artifacts"]:
        lines.append(f"| `{artifact['name']}` | {artifact['size_bytes']} | `{artifact['sha256']}` |")
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifacts", nargs="+", help="Artifact files to hash")
    parser.add_argument("--out-dir", default="build/release-provenance", help="Output directory")
    args = parser.parse_args()

    artifacts = [Path(item).resolve() for item in args.artifacts]
    missing = [str(path) for path in artifacts if not path.exists()]
    if missing:
        raise SystemExit(f"Missing artifact(s): {', '.join(missing)}")

    provenance = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "source": {
            "commit": git_output("rev-parse", "HEAD"),
            "ref": os.environ.get("GITHUB_REF_NAME") or git_output("branch", "--show-current"),
            "dirty": git_output("status", "--porcelain") != "",
        },
        "environment": {
            "system": platform.platform(),
            "machine": platform.machine(),
            "python": platform.python_version(),
        },
        "artifacts": [artifact_record(path) for path in artifacts],
    }

    out_dir = (ROOT / args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "release-provenance.json").write_text(
        json.dumps(provenance, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (out_dir / "release-provenance.md").write_text(markdown_report(provenance), encoding="utf-8")


if __name__ == "__main__":
    main()
