#!/usr/bin/env python3
"""Update version strings across the project for a new release."""

import argparse
import re
import sys
from datetime import date
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent

CMAKE_FILE = REPO_ROOT / "CMakeLists.txt"
PKGBUILD_FILE = REPO_ROOT / "packaging" / "arch" / "PKGBUILD"
CHANGELOG_FILE = REPO_ROOT / "CHANGELOG.md"
METAINFO_FILE = REPO_ROOT / "res" / "io.marlonn.carafe.metainfo.xml"

_PAT_NONE = re.compile(r"^[Nn]one\.?$")


def update_cmake(version: str) -> None:
    content = CMAKE_FILE.read_text()
    content = re.sub(
        r"^project\(carafe VERSION \d+\.\d+\.\d+",
        f"project(carafe VERSION {version}",
        content,
        count=1,
        flags=re.MULTILINE,
    )
    CMAKE_FILE.write_text(content)
    print(f"  CMakeLists.txt -> {version}")


def update_pkgbuild(version: str) -> None:
    content = PKGBUILD_FILE.read_text()
    content = re.sub(r"^pkgver=\d+\.\d+\.\d+", f"pkgver={version}", content, count=1, flags=re.MULTILINE)
    content = re.sub(r"^pkgrel=\d+", "pkgrel=1", content, count=1, flags=re.MULTILINE)
    PKGBUILD_FILE.write_text(content)
    print(f"  PKGBUILD     -> {version}")


def update_changelog(version: str) -> list[str]:
    content = CHANGELOG_FILE.read_text()
    today = date.today().isoformat()
    new_section = f"## [{version}] - {today}"
    content = re.sub(
        r"^## \[Unreleased\]$",
        f"## [Unreleased]\n\n{new_section}",
        content,
        count=1,
        flags=re.MULTILINE,
    )
    CHANGELOG_FILE.write_text(content)
    notes = _extract_release_notes(content, version)
    count = len(notes)
    print(f"  CHANGELOG.md -> {version} ({today})" + (f" ({count} change{'s' if count != 1 else ''})" if count else ""))
    return notes


def _extract_release_notes(content: str, version: str) -> list[str]:
    lines = content.split("\n")
    version_heading = re.compile(r"^## \[" + re.escape(version) + r"\] - \d{4}-\d{2}-\d{2}$")
    next_heading = re.compile(r"^## ")

    start = None
    for i, line in enumerate(lines):
        if version_heading.match(line.strip()):
            start = i
            break
    if start is None:
        return []

    end = len(lines)
    for i in range(start + 1, len(lines)):
        if next_heading.match(lines[i].strip()):
            end = i
            break

    notes = []
    for line in lines[start:end]:
        m = re.match(r"^[-*]\s+(.+)$", line.strip())
        if m:
            item = m.group(1).strip()
            if not _PAT_NONE.match(item):
                notes.append(item)
    return notes


def _xml_description(notes: list[str]) -> str:
    if not notes:
        return ""
    parts = ["    <description>"]
    for note in notes:
        parts.append(f"      <p>{note}</p>")
    parts.append("    </description>")
    return "\n" + "\n".join(parts)


def update_metainfo(version: str, notes: list[str]) -> None:
    content = METAINFO_FILE.read_text()
    today = date.today().isoformat()
    desc = _xml_description(notes)
    label = f" ({len(notes)} change{'s' if len(notes) != 1 else ''})" if notes else ""

    if desc:
        release_tag = f'  <release version="{version}" date="{today}">{desc}\n  </release>'
    else:
        release_tag = f'  <release version="{version}" date="{today}"/>'

    content = content.replace("  <releases>\n", f"  <releases>\n{release_tag}\n", 1)
    METAINFO_FILE.write_text(content)
    print(f"  metainfo.xml -> {version} ({today})" + label)


def main() -> None:
    parser = argparse.ArgumentParser(description="Prepare a release by bumping version strings.")
    parser.add_argument("--version", required=True, help="New version (semver, e.g. 0.2.0)")
    args = parser.parse_args()

    if not re.match(r"^\d+\.\d+\.\d+$", args.version):
        print(f"error: '{args.version}' is not a valid semver", file=sys.stderr)
        sys.exit(1)

    print("Updating version files:")
    update_cmake(args.version)
    update_pkgbuild(args.version)
    notes = update_changelog(args.version)
    update_metainfo(args.version, notes)
    print("Done.")


if __name__ == "__main__":
    main()
