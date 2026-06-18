#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * Noise/Riser — M5
 *
 * Two components mixed together:
 * 1. White noise through a rising bandpass filter
 *    - Center frequency sweeps from ~200 Hz to ~8 kHz as activation rises
 *    - Bandwidth is fairly wide (Q ~0.8)
 * 2. Sine uplifter
 *    - Frequency sweeps 1 → 3 octaves over activation (base = 80 Hz)
 *    - Amplitude scales with activation
 *
 * Both scaled by activation * riserCeil (preset intensity ceiling).
 * Output is added into the GENERATORS buffer.
 */
class NoiseRiser
{
public:
    NoiseRiser() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        noiseBP.prepare(spec);
        noiseBP.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        noiseBP.setResonance(0.8f);

        sinePhase = 0.0f;
        currentActivation = 0.0f;
        currentCeil = 0.0f;
    }

    void setActivation(float activation, float riserCeil)
    {
        currentActivation = activation;
        currentCeil = riserCeil;

        if (activation <= 0.001f)
            return;

        // Rising bandpass center: 200 Hz → 8000 Hz, log-mapped
        float bpCenter = 200.0f * std::pow(8000.0f / 200.0f, activation);
        bpCenter = juce::jlimit(200.0f, static_cast<float>(sampleRate * 0.4), bpCenter);
        noiseBP.setCutoffFrequency(bpCenter);
    }

    void process(juce::AudioBuffer<float>& outputBuffer, int numSamples)
    {
        if (currentActivation <= 0.001f)
            return;

        const int numChannels = outputBuffer.getNumChannels();
        const float scale = currentActivation * currentCeil;

        // Sine uplifter: base 80 Hz sweeping up to 3 octaves
        // freq = 80 * 2^(activation * 3)
        float sineFreq = 80.0f * std::pow(2.0f, currentActivation * 3.0f);
        float sinePhaseInc = juce::MathConstants<float>::twoPi
                             * sineFreq / static_cast<float>(sampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            // White noise
            float noise = (random.nextFloat() * 2.0f - 1.0f);

            // Sine uplifter
            float sine = std::sin(sinePhase);
            sinePhase += sinePhaseInc;
            if (sinePhase >= juce::MathConstants<float>::twoPi)
                sinePhase -= juce::MathConstants<float>::twoPi;

            float sample = (noise * 0.6f + sine * 0.4f) * scale;

            for (int ch = 0; ch < numChannels; ++ch)
                outputBuffer.addSample(ch, i, sample);
        }

        // Apply rising bandpass to just the noise component
        // (We process the whole buffer through BP — sine adds texture but also
        //  gets coloured by the rising filter which is musically appropriate)
        auto block = juce::dsp::AudioBlock<float>(outputBuffer).getSubBlock(0, static_cast<size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        noiseBP.process(ctx);
    }

private:
    juce::dsp::StateVariableTPTFilter<float> noiseBP;
    juce::Random random;

    double sampleRate         = 44100.0;
    float  sinePhase          = 0.0f;
    float  currentActivation  = 0.0f;
    float  currentCeil        = 0.0f;
};
