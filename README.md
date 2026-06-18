# VERTIGO

One-knob build-up effect VST3. Automate **BUILD** 0→1 over a break bus (pad + lead + vocal + light perc, **no kick**) and it sweeps HPF, opens reverb, fires an accelerating snare rush, lifts a riser, and drops a short gap right before the drop.

---

## Download & Install (for friends)

Go to **[Releases](../../releases)** and grab the zip for your platform:

| Platform | File | Install |
|----------|------|---------|
| macOS (Intel + Apple Silicon) | `VERTIGO-mac.zip` | Unzip → double-click `install-mac.command` |
| Windows | `VERTIGO-windows.zip` | Unzip → double-click `install-windows.bat` |
| Linux | `VERTIGO-linux.zip` | Unzip → run `./install-linux.sh` in terminal |

After install, rescan plugins in your DAW and look for **VERTIGO** in the VST3 list.

> **macOS note:** If the install script is blocked by Gatekeeper, right-click it → Open → Open.

---

## Build from source

**Prerequisites:** CMake 3.22+, C++17 compiler, internet access (JUCE downloads automatically).

```bash
git clone https://github.com/qwellolee-cell/vertigo.git
cd vertigo
git checkout claude/vertigo-vst3-buildup-suf37r

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

Artifact: `build/VERTIGO_artefacts/Release/VST3/VERTIGO.vst3`

### Manual install
```bash
# macOS
cp -r build/VERTIGO_artefacts/Release/VST3/VERTIGO.vst3 ~/Library/Audio/Plug-Ins/VST3/

# Windows
copy build\VERTIGO_artefacts\Release\VST3\VERTIGO.vst3 "%APPDATA%\VST3\"

# Linux
cp -r build/VERTIGO_artefacts/Release/VST3/VERTIGO.vst3 ~/.vst3/
```

---

## Releasing a new version

```bash
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions will build all three platforms and publish a release automatically.

---

## Usage

1. Insert VERTIGO on a break bus (pad + lead + vocal + light perc, no kick)
2. Draw automation on **BUILD**: 0 → 1 over 8 bars
3. Choose a preset to set the character:
   - **Melodic House** — subtle HPF + gentle space, late snare
   - **Indie Dance** *(default)* — balanced across all modules
   - **Big Room** — aggressive sweep, loud snare rush, full stutter
   - **Techno** — hard HPF, no riser, heavy gate/stutter
4. Trim individual layers with the depth knobs (HPF, SPACE, SNARE, RISER, GATE, DRIVE, IMPACT)
