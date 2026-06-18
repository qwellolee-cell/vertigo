# VERTIGO — Design Decisions

## M1: CMake + JUCE FetchContent, Pass-through VST3

- Using JUCE 8.0.4 via FetchContent (no Projucer dependency)
- VST3 only for Linux; AU added on macOS via CMake detection
- Plugin code: Vtgo / Vrtg
- Stereo in/out, not a synth, no MIDI
- All 11 APVTS parameters wired up from the start so state save/load works immediately
- Pass-through in M1 ensures always-compilable base

## M2: HPF Sweep

- Using juce::dsp::StateVariableTPTFilter in highpass mode
- Cutoff log-mapped: freq = 20 * pow(ceilHz/20, activation)
- Resonance rises 0.7→2.0 in top 30% of BUILD travel (activation > 0.7)
- smoothstep(onset, full, build) maps BUILD → per-effect activation
- Preset table hardcoded per spec

## M3: Reverb + Drive

- juce::dsp::Reverb on THROUGHPUT path after HPF
- Reverb wet only; dry signal kept separate for final mix blend
- Drive: soft-clip via tanh, drive amount 1→4 with activation
- Drive applied after reverb to colour the verb tail too

## M4: Snare Rush

- Synthesized: bandpassed noise (1.5–4 kHz SVF) + sine body at ~180 Hz
- AD envelope: ~1ms attack, 60–120ms decay (preset-dependent)
- Tempo-synced via AudioPlayHead BPM; fallback 120 BPM
- Subdivision ladder [8, 16, 32, 64] — higher build → denser subdivisions
- Snare amplitude follows smoothstep activation

## M5: Noise Riser

- White noise through rising bandpass (SVF) sweeping low→high
- Sine uplifter sweeping 1→3 octaves over the activation range
- Bandpass center frequency log-mapped to activation
- Both mixed and amplitude-scaled by smoothstep activation

## M6: Capture Buffer + Stutter + Freeze

- Circular stereo buffer: 2 bars at current BPM (min 4s, max 16s)
- Stutter: on next beat boundary, lock read pointer to slice start
- Slice length shrinks as build rises (coarser → finer subdivisions)
- 2ms crossfade at every slice edge for click-free looping
- Freeze: top 25% of BUILD travel, ~1-beat window, equal-power crossfade

## M7: Full Macro Mapping + Presets + 3-Path Blend

- Three signal paths: THROUGHPUT, GENERATORS (snare+riser), CAPTURE (stutter/freeze)
- Final mix: wet = (throughput_processed + generators + capture) blended by DRY/WET
- Per-preset macro table applied to all 7 effect activation values
- Depth trim parameters multiply the activation value (0→1 scale)

## Distribution / Installer

- GitHub Actions CI builds macOS (universal), Windows, Linux on every `v*.*.*` tag push
- Each platform produces a zip: VST3 bundle + platform install script
- Install scripts target user-level plugin folders (no admin rights needed):
  - macOS: `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`
  - Windows: `%APPDATA%\VST3\` (scanned by all major DAWs; no elevation required)
  - Linux: `~/.vst3/`
- macOS build uses `CMAKE_OSX_ARCHITECTURES="x86_64;arm64"` for a universal binary
- Release body auto-includes install table; `generate_release_notes: false` used to keep it clean
- `softprops/action-gh-release@v2` chosen over the deprecated v1 for reliability

## M8: Impact Cut + Minimal Editor

- Impact cut: hard duck applied to final mix when build > impactOnset
- Duck depth = smoothstep(onset, 1.0, build) * impactDepth * gapDepth
- 5ms linear fade in/out around gap to avoid clicks
- Editor: large BUILD knob centred, preset combo, depth trim sliders in row
- Background: dark blue-purple gradient (#1a1a2e → #16213e)
