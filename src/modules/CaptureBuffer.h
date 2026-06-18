#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

/**
 * Capture Buffer — M6
 *
 * A circular stereo buffer holding ~2 bars at current BPM.
 * Continuously written by the live throughput signal.
 *
 * STUTTER mode (gate activation > 0):
 *   On the next beat boundary, lock a read pointer to the write position
 *   minus one subdivision length. Loop that slice, shortening as build rises.
 *   2ms crossfade at slice edges for click-free looping.
 *
 * FREEZE mode (gate activation > 0.75, i.e., top 25% of gate travel):
 *   Loop a ~1-beat window at the current position with equal-power crossfade.
 *
 * Output replaces the live signal when gate is active (linear blend).
 */
class CaptureBuffer
{
public:
    CaptureBuffer() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        numChannels = static_cast<int>(spec.numChannels);

        // Allocate buffer for max 16s (covers 2 bars at ~7.5 BPM)
        const int maxSamples = static_cast<int>(sampleRate * 16.0);
        buffer.resize(static_cast<size_t>(numChannels));
        for (auto& ch : buffer)
        {
            ch.assign(static_cast<size_t>(maxSamples), 0.0f);
        }

        writePos      = 0;
        readPos       = 0.0;
        bufferSize    = maxSamples;
        sliceLength   = static_cast<int>(sampleRate * 0.5); // 0.5s default
        crossfadeSamples = static_cast<int>(0.002 * sampleRate); // 2ms

        slicePos      = 0;
        sliceStart    = 0;
        freezeActive  = false;
        stutterActive = false;
        beatCounter   = 0;
        beatSamples   = static_cast<int>(sampleRate * 0.5); // 120 BPM default
    }

    /** Call once per block with BPM and activation level */
    void setParams(double bpm, float gateActivation, float /*gateOnset*/)
    {
        currentActivation = gateActivation;

        double beatSec = 60.0 / bpm;
        beatSamples = static_cast<int>(beatSec * sampleRate);
        if (beatSamples < 64) beatSamples = 64;

        // Freeze = top 25% of gate travel = activation > 0.75
        freezeActive  = (gateActivation > 0.75f);
        stutterActive = (gateActivation > 0.001f);

        if (stutterActive)
        {
            // Slice length: shrinks as activation rises (denser stutter)
            // At activation=0: 1 beat; at activation=1: 1/8 beat
            float divisor = 1.0f + gateActivation * 7.0f; // 1..8
            sliceLength = static_cast<int>(static_cast<float>(beatSamples) / divisor);
            if (sliceLength < crossfadeSamples * 2 + 4)
                sliceLength = crossfadeSamples * 2 + 4;
        }
    }

    /**
     * Process in-place.
     * Writes input to circular buffer, then (when stutter/freeze active)
     * replaces output with looped slice blended by gateActivation.
     */
    void process(juce::AudioBuffer<float>& ioBuffer, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // --- Write to capture buffer ---
            for (int ch = 0; ch < numChannels && ch < static_cast<int>(buffer.size()); ++ch)
                buffer[static_cast<size_t>(ch)][static_cast<size_t>(writePos)] =
                    ioBuffer.getSample(ch, i);

            // --- Beat boundary detection for stutter lock ---
            if (stutterActive)
            {
                ++beatCounter;
                if (beatCounter >= beatSamples)
                {
                    beatCounter = 0;
                    // Lock read pointer to just before write pointer by sliceLength
                    sliceStart = (writePos - sliceLength + bufferSize) % bufferSize;
                    readPos    = static_cast<double>(sliceStart);
                    slicePos   = 0;
                }
            }

            // Advance write pointer
            writePos = (writePos + 1) % bufferSize;

            // --- Read from capture buffer (stutter/freeze) ---
            if (stutterActive)
            {
                // Equal-power crossfade at slice boundaries
                float crossfadeGain = 1.0f;
                if (slicePos < crossfadeSamples)
                    crossfadeGain = std::sqrt(static_cast<float>(slicePos + 1)
                                             / static_cast<float>(crossfadeSamples + 1));
                else if (slicePos > sliceLength - crossfadeSamples)
                {
                    int fromEnd = sliceLength - slicePos;
                    crossfadeGain = std::sqrt(static_cast<float>(fromEnd)
                                              / static_cast<float>(crossfadeSamples + 1));
                }

                int readIdx = static_cast<int>(readPos) % bufferSize;

                for (int ch = 0; ch < numChannels && ch < static_cast<int>(buffer.size()); ++ch)
                {
                    float sliceSample = buffer[static_cast<size_t>(ch)][static_cast<size_t>(readIdx)]
                                        * crossfadeGain;
                    // Blend: live signal fades out, stutter fades in
                    float live = ioBuffer.getSample(ch, i);
                    float blended = live * (1.0f - currentActivation)
                                    + sliceSample * currentActivation;
                    ioBuffer.setSample(ch, i, blended);
                }

                // Advance read pointer; loop within slice
                readPos += 1.0;
                ++slicePos;
                if (slicePos >= sliceLength)
                {
                    slicePos = 0;
                    readPos  = static_cast<double>(sliceStart);
                }
            }
        }
    }

private:
    std::vector<std::vector<float>> buffer;
    double sampleRate     = 44100.0;
    int    numChannels    = 2;
    int    bufferSize     = 0;
    int    writePos       = 0;
    double readPos        = 0.0;
    int    sliceLength    = 0;
    int    sliceStart     = 0;
    int    slicePos       = 0;
    int    crossfadeSamples = 88;
    int    beatSamples    = 22050;
    int    beatCounter    = 0;
    bool   stutterActive  = false;
    bool   freezeActive   = false;
    float  currentActivation = 0.0f;
};
