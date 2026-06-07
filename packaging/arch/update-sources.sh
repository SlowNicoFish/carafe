#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# 1. Update checksums in PKGBUILD
updpkgsums

# 2. Regenerate .SRCINFO
makepkg --printsrcinfo > .SRCINFO

# 3. Clean up downloaded sources
rm -f ./*.tar.gz

echo "Done. Checksums updated and .SRCINFO regenerated."
