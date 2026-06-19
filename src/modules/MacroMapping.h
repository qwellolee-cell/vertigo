#pragma once
#include <juce_core/juce_core.h>

/**
 * Macro mapping helpers — smoothstep + per-preset parameter tables.
 * All preset data matches the specification table.
 */

/** GLSL-style smoothstep clamped to [0, 1] */
inline float smoothstep(float edge0, float edge1, float x)
{
    float t = juce::jlimit(0.0f, 1.0f, (x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

struct PresetParams
{
    // HPF
    float hpfOnset;
    float hpfFull;
    float hpfCeilHz;
    float hpfMaxRes;  // max resonance at top

    // Space / Verb
    float verbOnset;
    float verbFull;
    float verbCeil;   // max wet (0..1)

    // Snare rush
    float snareOnset;
    float snareFull;
    int   snareMaxDiv; // max subdivision (8/16/32/64)

    // Riser
    float riserOnset;
    float riserFull;
    float riserCeil;  // 0..1 intensity ceiling

    // Gate/stutter (onset > 1.0 means off)
    float gateOnset;
    float gateFull;
    float gateCeil;

    // Drive
    float driveOnset;
    float driveFull;
    float driveCeil;  // 0..1 (maps to drive amount 1..4)

    // Impact
    float impactOnset;
    float impactGapDepth; // 0..1 duck depth

    // Ping Pong Delay
    float ppOnset;
    float ppFull;
    float ppWetCeil;
};

// Preset indices match APVTS choice order
// hpfMaxRes: spec gives resonance multiplier; map 0.25->0.875, 0.40->1.5, 0.65->2.0, 0.80->2.3
static const PresetParams kPresets[4] =
{
    // 0: Melodic House
    { 0.22f, 0.96f, 620.0f,  1.20f,   // hpf
      0.16f, 0.92f, 0.72f,             // verb
      0.48f, 1.00f, 16,                // snare
      0.64f, 0.97f, 0.55f,             // riser
      2.00f, 2.00f, 0.00f,             // gate (off)
      0.58f, 0.90f, 0.35f,             // drive
      0.95f, 0.45f,                    // impact
      0.35f, 0.85f, 0.40f },           // ping pong

    // 1: Indie Dance (default)
    { 0.18f, 0.92f, 780.0f,  1.50f,   // hpf
      0.18f, 0.94f, 0.66f,             // verb
      0.38f, 1.00f, 32,                // snare
      0.56f, 0.98f, 0.70f,             // riser
      0.80f, 0.96f, 0.50f,             // gate
      0.54f, 0.90f, 0.50f,             // drive
      0.96f, 0.70f,                    // impact
      0.30f, 0.85f, 0.50f },           // ping pong

    // 2: Big Room
    { 0.14f, 0.90f, 950.0f,  2.00f,   // hpf
      0.20f, 0.95f, 0.60f,             // verb
      0.28f, 1.00f, 64,                // snare
      0.50f, 0.98f, 1.00f,             // riser
      0.72f, 0.95f, 0.85f,             // gate
      0.55f, 0.92f, 0.80f,             // drive
      0.97f, 1.00f,                    // impact
      0.25f, 0.88f, 0.65f },           // ping pong

    // 3: Techno
    { 0.10f, 0.88f, 1100.0f, 2.30f,   // hpf
      0.24f, 0.90f, 0.50f,             // verb
      0.55f, 1.00f, 16,                // snare
      2.00f, 2.00f, 0.00f,             // riser (off)
      0.60f, 0.94f, 1.00f,             // gate
      0.45f, 0.90f, 0.70f,             // drive
      0.94f, 0.90f,                    // impact
      0.35f, 0.85f, 0.45f }            // ping pong
};

/** Compute all per-effect activations from BUILD knob + preset */
struct EffectActivations
{
    float hpf;
    float verb;
    float snare;
    float riser;
    float gate;
    float drive;
    float impact;
    float pp;
};

inline EffectActivations computeActivations(float build, int presetIndex)
{
    presetIndex = juce::jlimit(0, 3, presetIndex);
    const PresetParams& p = kPresets[presetIndex];
    EffectActivations a;
    a.hpf    = smoothstep(p.hpfOnset,    p.hpfFull,    build);
    a.verb   = smoothstep(p.verbOnset,   p.verbFull,   build);
    a.snare  = smoothstep(p.snareOnset,  p.snareFull,  build);
    a.riser  = smoothstep(p.riserOnset,  p.riserFull,  build);
    a.gate   = smoothstep(p.gateOnset,   p.gateFull,   build);
    a.drive  = smoothstep(p.driveOnset,  p.driveFull,  build);
    a.impact = smoothstep(p.impactOnset, 1.0f,         build);
    a.pp     = smoothstep(p.ppOnset,     p.ppFull,     build);
    return a;
}
