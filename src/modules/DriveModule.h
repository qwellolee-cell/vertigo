#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * DRIVE — M3
 * Soft tanh waveshaper. drive amount scales 1 → 4 with activation.
 * Compensation gain applied so unity-ish loudness is maintained.
 */
class DriveModule
{
public:
    DriveModule() = default;

    void prepare(const juce::dsp::ProcessSpec& /*spec*/) {}

    /** activation 0..1, driveCeil 0..1 (scales how much drive is applied) */
    void setActivation(float activation, float driveCeil)
    {
        // Drive amount 1 (bypass) → 4 (heavy clipping)
        driveAmount = 1.0f + activation * driveCeil * 3.0f;
        // Compensation: output scaled to roughly match input level
        compensationGain = 1.0f / std::tanh(driveAmount);
    }

    void process(juce::dsp::AudioBlock<float>& block, float activation)
    {
        if (activation <= 0.001f)
            return;

        const int numChannels = static_cast<int>(block.getNumChannels());
        const int numSamples  = static_cast<int>(block.getNumSamples());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = block.getChannelPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                data[i] = std::tanh(data[i] * driveAmount) * compensationGain;
            }
        }
    }

private:
    float driveAmount      = 1.0f;
    float compensationGain = 1.0f;
};
