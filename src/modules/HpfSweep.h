#pragma once
#include <juce_dsp/juce_dsp.h>

/**
 * HPF Sweep — M2
 * StateVariableTPTFilter in highpass mode.
 * Cutoff log-mapped: freq = 20 * pow(ceilHz / 20, activation)
 * Resonance rises 0.7 → 2.0 in top 30% of activation travel.
 */
class HpfSweep
{
public:
    HpfSweep() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        filter.prepare(spec);
        filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        setActivation(0.0f, 20.0f, 780.0f, 0.40f); // Indie Dance defaults
    }

    /** activation 0..1, onset/full already computed by caller via smoothstep */
    void setActivation(float activation, float minHz, float ceilHz, float maxResonance)
    {
        if (activation <= 0.0f)
        {
            // No filtering — set to 20 Hz (effectively bypassed)
            filter.setCutoffFrequency(20.0f);
            filter.setResonance(0.7f);
            return;
        }

        // Log-mapped cutoff
        float freq = 20.0f * std::pow(ceilHz / 20.0f, activation);
        freq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), freq);
        filter.setCutoffFrequency(freq);

        // Resonance rises in top 30% of travel
        float resActivation = juce::jlimit(0.0f, 1.0f, (activation - 0.7f) / 0.3f);
        float resonance = 0.7f + resActivation * (maxResonance - 0.7f);
        filter.setResonance(resonance);
    }

    /** Process a stereo AudioBlock in place */
    void process(juce::dsp::AudioBlock<float>& block, float activation)
    {
        if (activation <= 0.0f)
            return; // bypass when not active

        juce::dsp::ProcessContextReplacing<float> ctx(block);
        filter.process(ctx);
    }

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
    double sampleRate = 44100.0;
};
