set shell := ['bash', '-eu', '-o', 'pipefail', '-c']

# Configure
configure type="Debug":
    cmake -S . -B build/{{ type }} -DCMAKE_BUILD_TYPE={{ type }} -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

# Build
build type="Debug":
    cmake --build build/{{ type }}

# Install
install type="Debug":
    cmake --install build/{{ type }}

# Uninstall
uninstall type="Debug":
    cmake --build build/{{ type }} --target uninstall

# Clean
clean:
    rm -rf build/Debug build/Release

# Bump version: updates CMakeLists.txt, CHANGELOG.md, and metainfo stubs
# Usage: just bump 0.3.13
bump version:
    #!/usr/bin/env bash
    set -euo pipefail

    NEW="{{ version }}"
    TODAY=$(date +%Y-%m-%d)
    METAINFO="res/io.marlonn.carafe.metainfo.xml"

    # Validate format
    if ! [[ "${NEW}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "Error: version must be in X.Y.Z format (got '${NEW}')" >&2
        exit 1
    fi

    OLD=$(grep -m1 'project(carafe VERSION' CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/')

    if [ "${OLD}" = "${NEW}" ]; then
        echo "Error: new version (${NEW}) is the same as current version" >&2
        exit 1
    fi

    echo "Bumping ${OLD} -> ${NEW}"

    # 1. Update CMakeLists.txt
    sed -i "s/project(carafe VERSION ${OLD}/project(carafe VERSION ${NEW}/" CMakeLists.txt
    echo "  ✓ CMakeLists.txt"

    # 2. Stub a new versioned section in CHANGELOG.md (keeps [Unreleased] at top)
    CHANGELOG_STUB="## [${NEW}] - ${TODAY}\n\n### Added\n\n- None\n\n### Changed\n\n- None.\n\n### Fixed\n\n- None.\n\n### Removed\n\n- None.\n"
    sed -i "s|## \[Unreleased\]|## [Unreleased]\n\n\n${CHANGELOG_STUB}|" CHANGELOG.md
    echo "  ✓ CHANGELOG.md"

    # 3. Insert a stub <release> entry at the top of <releases> in metainfo
    if grep -q "version=\"${NEW}\"" "${METAINFO}"; then
        echo "  - metainfo already has ${NEW}, skipping"
    else
        METAINFO_STUB="  <release version=\"${NEW}\" date=\"${TODAY}\">\n    <description>\n      <p>TODO: fill in release notes</p>\n    </description>\n  </release>"
        sed -i "s|<releases>|<releases>\n${METAINFO_STUB}|" "${METAINFO}"
        echo "  ✓ metainfo.xml"
    fi

    echo ""
    echo "Done. Next steps:"
    echo "  1. Fill in CHANGELOG.md under [${NEW}]"
    echo "  2. commit: git commit -am \"chore: bump version to ${NEW}\""
    echo "  3. just release"

# Create a GitHub release from the current version and changelog
release:
    #!/usr/bin/env bash
    set -euo pipefail

    METAINFO="res/io.marlonn.carafe.metainfo.xml"

    # Read version from CMakeLists.txt
    VERSION=$(grep -m1 'project(carafe VERSION' CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/')
    TAG="v${VERSION}"
    TODAY=$(date +%Y-%m-%d)

    echo "Releasing ${TAG}..."

    # Check tag doesn't already exist
    if git tag --list | grep -qx "${TAG}"; then
        echo "Error: tag ${TAG} already exists" >&2
        exit 1
    fi

    # Extract release notes for this version from CHANGELOG.md
    NOTES=$(awk "/^## \[${VERSION}\]/{found=1; next} found && /^## \[/{exit} found{print}" CHANGELOG.md)

    if [ -z "$NOTES" ]; then
        echo "Warning: no changelog entry found for ${VERSION}. Proceeding with empty notes."
    fi

    # Guard against uncommitted changes in tracked release files
    for f in CHANGELOG.md CMakeLists.txt "${METAINFO}"; do
        if ! git diff --quiet "$f" || ! git diff --cached --quiet "$f"; then
            echo "Error: uncommitted changes in $f — please commit them first." >&2
            exit 1
        fi
    done

    # Update the metainfo <release> entry: replace the TODO placeholder description
    # with real <p> elements built from the changelog notes
    if grep -q "version=\"${VERSION}\"" "${METAINFO}"; then
        # Build replacement description content from changelog
        DESC_LINES=""
        while IFS= read -r line; do
            [[ -z "$line" || "$line" =~ ^### ]] && continue
            line="${line#- }"
            DESC_LINES+="      <p>${line}</p>\n"
        done <<< "$NOTES"

        if [ -n "$DESC_LINES" ]; then
            # Replace everything between <description> and </description> for this release
            python3 -c "
                        import re, sys
                        xml = open('${METAINFO}').read()
                        new_desc = '    <description>\n${DESC_LINES}    </description>'
                        # Replace the description block inside the matching release element
                        pattern = r'(<release version=\"${VERSION}\"[^>]*>)\s*<description>.*?</description>'
                        replacement = r'\1\n' + new_desc
                        xml = re.sub(pattern, replacement, xml, flags=re.DOTALL)
                        open('${METAINFO}', 'w').write(xml)
                        "
                                    git add "${METAINFO}"
                                    git commit -m "chore: update metainfo for ${TAG}"
                                    git push
                                    echo "  ✓ metainfo.xml updated"
                                fi
                            else
                                echo "Warning: no metainfo entry for ${VERSION} — run 'just bump' next time."
                            fi

                            # Tag and push
                            git tag -a "${TAG}" -m "Release ${TAG}"
                            git push origin "${TAG}"

                            # Create GitHub release
                            gh release create "${TAG}" \
                                --title "${TAG}" \
                                --notes "${NOTES}"

    echo "Done! https://github.com/marlonn/carafe/releases/tag/${TAG}"
