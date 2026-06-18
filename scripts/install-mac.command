#!/bin/bash
# VERTIGO — macOS installer
# Double-click this file to install VERTIGO to your user VST3 folder.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VST3_SRC="$SCRIPT_DIR/VERTIGO.vst3"
AU_SRC="$SCRIPT_DIR/VERTIGO.component"

VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"

echo ""
echo "Installing VERTIGO..."
echo ""

# VST3
mkdir -p "$VST3_DEST"
rm -rf "$VST3_DEST/VERTIGO.vst3"
cp -r "$VST3_SRC" "$VST3_DEST/"
echo "  VST3 → $VST3_DEST/VERTIGO.vst3"

# AU (if bundled)
if [ -d "$AU_SRC" ]; then
    mkdir -p "$AU_DEST"
    rm -rf "$AU_DEST/VERTIGO.component"
    cp -r "$AU_SRC" "$AU_DEST/"
    echo "  AU   → $AU_DEST/VERTIGO.component"
fi

echo ""
echo "Done! Rescan plugins in your DAW to find VERTIGO."
echo "  Ableton: Options → Rescan Plug-ins"
echo "  Logic:   quit and reopen Logic"
echo ""
read -p "Press Enter to close..."
