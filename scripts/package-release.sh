#!/usr/bin/env bash
#
# Builds a release zip from build-universal/.
#
# Usage: scripts/package-release.sh 1.0.0

set -euo pipefail

VERSION="${1:?usage: scripts/package-release.sh <version>}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-universal/IRLoader_artefacts/Release"
STAGE="$ROOT/dist/Simple IR VST $VERSION"
ZIP="$ROOT/dist/Simple-IR-VST-$VERSION-macOS-universal.zip"

if [[ ! -d "$BUILD" ]]; then
    echo "Missing $BUILD - run the universal build first:" >&2
    echo "  cmake -S . -B build-universal -G Ninja -DCMAKE_BUILD_TYPE=Release \\" >&2
    echo "        -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64' -DIRLOADER_BUILD_TESTS=OFF" >&2
    echo "  cmake --build build-universal --parallel" >&2
    exit 1
fi

rm -rf "$ROOT/dist"
mkdir -p "$STAGE"

cp -R "$BUILD/VST3/IR Loader.vst3"        "$STAGE/"
cp -R "$BUILD/AU/IR Loader.component"     "$STAGE/"
cp -R "$BUILD/Standalone/IR Loader.app"   "$STAGE/"
cp     "$ROOT/Packaging/macOS/Install.command"  "$STAGE/"
cp     "$ROOT/Packaging/macOS/README.txt"       "$STAGE/"
chmod +x "$STAGE/Install.command"

# ditto keeps bundle metadata intact, which matters for plugin bundles.
ditto -c -k --sequesterRsrc --keepParent "$STAGE" "$ZIP"

echo "Created $ZIP"
shasum -a 256 "$ZIP"
