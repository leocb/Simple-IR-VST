#!/bin/bash
#
# Installs IR Loader (VST3 + AU) into the current user's plugin folders.
# Right-click this file and choose Open if macOS blocks it.

set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"

echo "Installing IR Loader..."

mkdir -p "$VST3_DIR" "$AU_DIR"

# Clear the download quarantine flag so hosts will load the plugin bundles.
xattr -dr com.apple.quarantine "$HERE/IR Loader.vst3" 2>/dev/null || true
xattr -dr com.apple.quarantine "$HERE/IR Loader.component" 2>/dev/null || true

rm -rf "$VST3_DIR/IR Loader.vst3" "$AU_DIR/IR Loader.component"
cp -R "$HERE/IR Loader.vst3" "$VST3_DIR/"
cp -R "$HERE/IR Loader.component" "$AU_DIR/"

echo ""
echo "Installed:"
echo "  VST3: $VST3_DIR/IR Loader.vst3"
echo "  AU:   $AU_DIR/IR Loader.component"
echo ""
echo "The standalone app is in this folder; drag it to /Applications if you"
echo "want to keep it. Restart your DAW and rescan plugins if needed."
echo ""

if [ -z "${CI:-}" ]; then
    read -n 1 -s -r -p "Press any key to close..."
    echo ""
fi
