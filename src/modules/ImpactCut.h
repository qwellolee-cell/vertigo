#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * Impact Cut — M8 (stub present for M7 compile; activated in M8)
 *
 * Last ~5% of BUILD travel: hard gap (duck to near-silence) for ~1/8–1/4
 * note before build=1.
 *
 * When build > impactOnset:
 *   gapGain = 1 - smoothstep(impactOnset, 1.0, build) * gapDepth
 * Fade in/out with 5ms crossfade to avoid clicks.
 */
class ImpactCut
{
public:
    ImpactCut() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        fadeSamples = static_cast<int>(0.005 * sampleRate); // 5ms
        currentGain = 1.0f;
        targetGain  = 1.0f;
    }

    /** activation 0..1 from smoothstep(impactOnset, 1.0, build) */
    void setActivation(float activation, float gapDepth)
    {
        targetGain = 1.0f - activation * gapDepth;
        if (targetGain < 0.0f) targetGain = 0.0f;
    }

    /** Apply duck gain with smooth fade to output buffer */
    void process(juce::AudioBuffer<float>& buffer, int numSamples)
    {
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            // Ramp currentGain toward targetGain
            float diff = targetGain - currentGain;
            float step = diff / static_cast<float>(fadeSamples + 1);
            currentGain += step;
            currentGain = juce::jlimit(0.0f, 1.0f, currentGain);

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * currentGain);
        }
    }

private:
    double sampleRate  = 44100.0;
    int    fadeSamples = 220;
    float  currentGain = 1.0f;
    float  targetGain  = 1.0f;
};
