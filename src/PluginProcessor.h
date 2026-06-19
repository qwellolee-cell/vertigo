#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "modules/MacroMapping.h"
#include "modules/HpfSweep.h"
#include "modules/SpaceVerb.h"
#include "modules/DriveModule.h"
#include "modules/SnareRush.h"
#include "modules/NoiseRiser.h"
#include "modules/CaptureBuffer.h"
#include "modules/ImpactCut.h"
#include "modules/PingPongDelay.h"

class VertigoAudioProcessor : public juce::AudioProcessor
{
public:
    VertigoAudioProcessor();
    ~VertigoAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP — M2: HPF
    HpfSweep hpfSweep;

    // DSP — M3: Reverb + Drive
    SpaceVerb  spaceVerb;
    DriveModule driveModule;

    // DSP — M4: Snare Rush generator
    SnareRush snareRush;

    // DSP — M5: Noise Riser generator
    NoiseRiser noiseRiser;

    // DSP — M6: Capture Buffer (stutter/freeze)
    CaptureBuffer captureBuffer;

    // DSP — M8: Impact Cut (wired in M7 for correct signal flow)
    ImpactCut impactCut;

    // DSP — Ping Pong Delay (on throughput, after SpaceVerb, before Drive)
    PingPongDelay pingPongDelay;

    // DSP — Output Limiter (always on, last in chain)
    juce::dsp::Limiter<float> limiter;

    // Dry buffer for DRY/WET blend
    juce::AudioBuffer<float> dryBuffer;

    // Generators buffer (stereo, summed into main buffer)
    juce::AudioBuffer<float> generatorsBuffer;

    // Smoothed build value to avoid zipper noise
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBuild;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertigoAudioProcessor)
};
