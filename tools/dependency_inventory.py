#!/usr/bin/env python3
"""Generate a lightweight dependency inventory for CI artifacts.

The script intentionally avoids network access. It reads repository-local metadata
so it can run in pull requests and release workflows without contacting package
registries.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]


def read_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as file:
        return json.load(file)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def vcpkg_inventory() -> dict[str, Any]:
    manifest_path = ROOT / "vcpkg.json"
    if not manifest_path.exists():
        return {"present": False}

    manifest = read_json(manifest_path)
    return {
        "present": True,
        "name": manifest.get("name"),
        "version-string": manifest.get("version-string"),
        "builtin-baseline": manifest.get("builtin-baseline"),
        "dependencies": manifest.get("dependencies", []),
        "features": manifest.get("features", {}),
    }


def github_actions_inventory() -> list[dict[str, str]]:
    workflows_dir = ROOT / ".github" / "workflows"
    if not workflows_dir.exists():
        return []

    actions: list[dict[str, str]] = []
    for workflow in sorted(workflows_dir.glob("*.yml")) + sorted(workflows_dir.glob("*.yaml")):
        for raw_line in read_text(workflow).splitlines():
            line = raw_line.strip()
            if not line.startswith("uses:"):
                continue
            uses = line.split("uses:", 1)[1].strip().strip('"\'')
            actions.append({"workflow": str(workflow.relative_to(ROOT)), "uses": uses})
    return actions


def third_party_inventory() -> list[dict[str, str]]:
    third_party_dir = ROOT / "3rdParty"
    if not third_party_dir.exists():
        return []

    items: list[dict[str, str]] = []
    for path in sorted(third_party_dir.iterdir()):
        if not path.is_dir():
            continue
        license_files = sorted(
            child.name
            for child in path.iterdir()
            if child.is_file() and child.name.lower().startswith(("license", "copying", "notice"))
        )
        items.append({
            "path": str(path.relative_to(ROOT)),
            "license_files": ", ".join(license_files) if license_files else "not detected",
        })
    return items


def markdown_report(inventory: dict[str, Any]) -> str:
    lines = [
        "# Dependency Inventory",
        "",
        "Generated from repository-local metadata. No package registry lookup was performed.",
        "",
        "## vcpkg",
        "",
    ]

    vcpkg = inventory["vcpkg"]
    if not vcpkg["present"]:
        lines.append("No `vcpkg.json` manifest found.")
    else:
        lines.extend([
            f"- Name: `{vcpkg.get('name')}`",
            f"- Version: `{vcpkg.get('version-string')}`",
            f"- Builtin baseline: `{vcpkg.get('builtin-baseline')}`",
            "",
            "### Manifest dependencies",
            "",
        ])
        for dependency in vcpkg.get("dependencies", []):
            if isinstance(dependency, str):
                lines.append(f"- `{dependency}`")
            else:
                lines.append(f"- `{dependency.get('name')}`: `{dependency}`")
        lines.extend(["", "### Manifest features", ""])
        for feature, data in sorted(vcpkg.get("features", {}).items()):
            dependencies = data.get("dependencies", []) if isinstance(data, dict) else []
            lines.append(f"- `{feature}`: {len(dependencies)} dependencies")

    lines.extend(["", "## GitHub Actions", ""])
    actions = inventory["github_actions"]
    if not actions:
        lines.append("No workflow actions detected.")
    else:
        lines.append("| Workflow | Action |")
        lines.append("|---|---|")
        for action in actions:
            lines.append(f"| `{action['workflow']}` | `{action['uses']}` |")

    lines.extend(["", "## Vendored third-party directories", ""])
    third_party = inventory["third_party"]
    if not third_party:
        lines.append("No `3rdParty/` directories detected.")
    else:
        lines.append("| Path | License files |")
        lines.append("|---|---|")
        for item in third_party:
            lines.append(f"| `{item['path']}` | {item['license_files']} |")

    lines.append("")
    return "\n".join(lines)


def main() -> None:
    inventory = {
        "vcpkg": vcpkg_inventory(),
        "github_actions": github_actions_inventory(),
        "third_party": third_party_inventory(),
    }

    out_dir = ROOT / "build" / "dependency-inventory"
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "dependency-inventory.json").write_text(
        json.dumps(inventory, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (out_dir / "dependency-inventory.md").write_text(markdown_report(inventory), encoding="utf-8")


if __name__ == "__main__":
    main()
