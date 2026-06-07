#!/usr/bin/env python3
"""Extract release notes for a given version from CHANGELOG.md."""

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
CHANGELOG_FILE = REPO_ROOT / "CHANGELOG.md"


def extract_notes(version: str) -> str | None:
    content = CHANGELOG_FILE.read_text()
    pattern = r"^## \[" + re.escape(version) + r"\][\s\S]*?(?=^## |\Z)"
    match = re.search(pattern, content, re.MULTILINE)
    if match:
        return match.group(0).strip()
    return None


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Extract release notes from CHANGELOG.md"
    )
    parser.add_argument(
        "--version", required=True, help="Version to extract (e.g. 0.2.0)"
    )
    parser.add_argument("--output", "-o", required=True, help="Output file path")
    args = parser.parse_args()

    notes = extract_notes(args.version)
    if notes is None:
        print(
            f"error: version '{args.version}' not found in CHANGELOG.md",
            file=sys.stderr,
        )
        sys.exit(1)

    Path(args.output).write_text(notes)
    print(f"Release notes for v{args.version} written to {args.output}")


if __name__ == "__main__":
    main()
