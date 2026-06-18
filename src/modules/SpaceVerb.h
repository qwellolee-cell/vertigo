#pragma once
#include <juce_dsp/juce_dsp.h>

/**
 * SPACE/VERB — M3
 * juce::dsp::Reverb on THROUGHPUT path (after HPF).
 * Wet level scales 0 -> ceilWet by activation.
 * Dry is kept at 1.0 so the caller can blend dry/wet externally.
 */
class SpaceVerb
{
public:
    SpaceVerb() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        reverb.prepare(spec);
        auto params = reverb.getParameters();
        params.roomSize   = 0.85f;
        params.damping    = 0.40f;
        params.width      = 1.00f;
        params.freezeMode = 0.00f;
        params.dryLevel   = 1.00f;
        params.wetLevel   = 0.00f;
        reverb.setParameters(params);
    }

    /** activation 0..1, ceilWet: maximum wet mix level for this preset */
    void setActivation(float activation, float ceilWet)
    {
        auto params = reverb.getParameters();
        params.wetLevel = activation * ceilWet;
        params.dryLevel = 1.0f;
        reverb.setParameters(params);
    }

    /** Process in place */
    void process(juce::dsp::AudioBlock<float>& block)
    {
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        reverb.process(ctx);
    }

private:
    juce::dsp::Reverb reverb;
};
