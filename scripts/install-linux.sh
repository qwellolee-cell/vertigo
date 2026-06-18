#!/bin/bash
# VERTIGO — Linux installer

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEST="$HOME/.vst3"

echo ""
echo "Installing VERTIGO..."
echo ""

mkdir -p "$DEST"
rm -rf "$DEST/VERTIGO.vst3"
cp -r "$SCRIPT_DIR/VERTIGO.vst3" "$DEST/"

echo "  VST3 → $DEST/VERTIGO.vst3"
echo ""
echo "Done! Rescan plugins in your DAW to find VERTIGO."
echo ""
