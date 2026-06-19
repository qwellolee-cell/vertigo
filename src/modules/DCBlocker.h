#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

/**
 * DCBlocker — one-pole DC blocking highpass (~30 Hz corner).
 * Removes DC offset / sub-rumble accumulated through the wet chain,
 * applied on the final mix just before the limiter.
 *
 * Transfer function: y[n] = x[n] - x[n-1] + R * y[n-1]
 */
class DCBlocker
{
public:
    void prepare(double sr, int numChannels)
    {
        R = 1.0f - (190.0f / (float) sr); // ~30Hz corner
        x1.assign((size_t) numChannels, 0.0f);
        y1.assign((size_t) numChannels, 0.0f);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        if ((size_t) numChannels > x1.size())
            return; // not prepared for this channel count

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            float xm1 = x1[(size_t) ch], ym1 = y1[(size_t) ch];
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float x = d[i];
                float y = x - xm1 + R * ym1;
                xm1 = x; ym1 = y;
                d[i] = y;
            }
            x1[(size_t) ch] = xm1; y1[(size_t) ch] = ym1;
        }
    }

private:
    float R = 0.995f;
    std::vector<float> x1, y1;
};
