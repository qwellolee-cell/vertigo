#include "PluginEditor.h"

VertigoAudioProcessorEditor::VertigoAudioProcessorEditor(VertigoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 300);
}

VertigoAudioProcessorEditor::~VertigoAudioProcessorEditor() {}

void VertigoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("VERTIGO", getLocalBounds(), juce::Justification::centred, 1);
}

void VertigoAudioProcessorEditor::resized() {}
