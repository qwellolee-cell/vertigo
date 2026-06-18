#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class VertigoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VertigoAudioProcessorEditor(VertigoAudioProcessor&);
    ~VertigoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VertigoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertigoAudioProcessorEditor)
};
