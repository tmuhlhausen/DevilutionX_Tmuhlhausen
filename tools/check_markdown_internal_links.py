#!/usr/bin/env python3
"""Fail on broken internal Markdown links.

Checks Markdown files in the repository for broken relative links and anchors.
External links (http/https/mailto/etc.) are ignored.
"""

from __future__ import annotations

import pathlib
import re
import sys
from urllib.parse import unquote

REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
MARKDOWN_LINK_RE = re.compile(r"\[[^\]]*\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)")
HEADING_RE = re.compile(r"^#{1,6}\s+(.+?)\s*$")


def slugify_heading(text: str) -> str:
    text = text.strip().lower()
    text = re.sub(r"[`*_~\[\]()]", "", text)
    text = re.sub(r"[^a-z0-9\s-]", "", text)
    text = re.sub(r"\s+", "-", text)
    text = re.sub(r"-+", "-", text)
    return text.strip("-")


def extract_anchors(markdown_path: pathlib.Path) -> set[str]:
    anchors: set[str] = set()
    for line in markdown_path.read_text(encoding="utf-8").splitlines():
        m = HEADING_RE.match(line)
        if m:
            anchors.add(slugify_heading(m.group(1)))
    return anchors


def is_external(link: str) -> bool:
    lowered = link.lower()
    return (
        "://" in lowered
        or lowered.startswith("mailto:")
        or lowered.startswith("tel:")
        or lowered.startswith("data:")
    )


def validate_link(source: pathlib.Path, raw_link: str, anchor_cache: dict[pathlib.Path, set[str]]) -> list[str]:
    errors: list[str] = []
    link = unquote(raw_link)

    if is_external(link):
        return errors

    if link.startswith("#"):
        target_path = source
        anchor = link[1:]
    else:
        path_part, _, anchor = link.partition("#")
        if path_part.startswith("/"):
            target_path = (REPO_ROOT / path_part.lstrip("/")).resolve()
        else:
            target_path = (source.parent / path_part).resolve()

        if not target_path.exists():
            return [f"{source.relative_to(REPO_ROOT)}: broken link target '{raw_link}'"]

        if target_path.is_dir():
            return errors

    if anchor:
        if target_path.suffix.lower() != ".md":
            return errors
        if target_path not in anchor_cache:
            anchor_cache[target_path] = extract_anchors(target_path)
        if anchor.lower() not in anchor_cache[target_path]:
            errors.append(
                f"{source.relative_to(REPO_ROOT)}: anchor '#{anchor}' not found in "
                f"{target_path.relative_to(REPO_ROOT)}"
            )
    return errors


def main() -> int:
    markdown_files = sorted(REPO_ROOT.rglob("*.md"))
    ignored_parts = {".git", "build"}
    markdown_files = [
        path for path in markdown_files if not any(part in ignored_parts for part in path.parts)
    ]

    anchor_cache: dict[pathlib.Path, set[str]] = {}
    all_errors: list[str] = []

    for path in markdown_files:
        text = path.read_text(encoding="utf-8")
        for link in MARKDOWN_LINK_RE.findall(text):
            all_errors.extend(validate_link(path, link, anchor_cache))

    if all_errors:
        print("Broken internal Markdown links found:")
        for error in all_errors:
            print(f"- {error}")
        return 1

    print(f"Checked {len(markdown_files)} Markdown files: all internal links resolved.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
