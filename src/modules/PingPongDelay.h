#pragma once
#include <juce_dsp/juce_dsp.h>

class PingPongDelay {
public:
    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;
        // max delay: 2 seconds (covers very slow BPMs)
        leftDelay.prepare(spec);
        rightDelay.prepare(spec);
        leftDelay.setMaximumDelayInSamples((int)(spec.sampleRate * 2.0));
        rightDelay.setMaximumDelayInSamples((int)(spec.sampleRate * 2.0));
        leftFeedback = 0.0f;
        rightFeedback = 0.0f;
        setBpm(120.0);
    }

    void setBpm(double bpm) {
        // dotted 8th = 3 * (60/bpm) / 4 seconds  → 3/8 of a beat
        float delaySec = (float)(3.0 * 60.0 / (bpm * 8.0));
        delaySamples = juce::jlimit(1.0f, (float)(sampleRate * 1.9), delaySec * (float)sampleRate);
        leftDelay.setDelay(delaySamples);
        rightDelay.setDelay(delaySamples);
    }

    // process stereo buffer in-place, wet = activation * wetCeil * depth
    void process(juce::AudioBuffer<float>& buffer, float wet) {
        if (wet <= 0.0f) {
            leftFeedback = 0.0f;
            rightFeedback = 0.0f;
            return;
        }
        const float fb = 0.45f;
        const int n = buffer.getNumSamples();
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1);

        for (int i = 0; i < n; ++i) {
            float dryL = L[i], dryR = R[i];

            // ping: L input + right feedback → left delay
            // pong: R input + left feedback → right delay
            float inL = dryL + rightFeedback * fb;
            float inR = dryR + leftFeedback * fb;

            leftDelay.pushSample(0, inL);
            rightDelay.pushSample(0, inR);

            float outL = leftDelay.popSample(0);
            float outR = rightDelay.popSample(0);

            leftFeedback  = outL;
            rightFeedback = outR;

            // soft clip feedback to prevent runaway
            leftFeedback  = std::tanh(leftFeedback);
            rightFeedback = std::tanh(rightFeedback);

            L[i] = dryL + wet * (outR - dryL);  // ping output on opposite side
            R[i] = dryR + wet * (outL - dryR);
        }
    }

    void reset() {
        leftDelay.reset();
        rightDelay.reset();
        leftFeedback = rightFeedback = 0.0f;
    }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> leftDelay{(int)(48000 * 2)};
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> rightDelay{(int)(48000 * 2)};
    float leftFeedback = 0.0f;
    float rightFeedback = 0.0f;
    float delaySamples = 0.0f;
    double sampleRate = 44100.0;
};
