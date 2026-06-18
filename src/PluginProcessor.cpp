#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout VertigoAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "build", 1 }, "BUILD",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "preset", 1 }, "PRESET",
        juce::StringArray{ "Melodic House", "Indie Dance", "Big Room", "Techno" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "DRY/WET",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "hpfDepth", 1 }, "HPF",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "verbDepth", 1 }, "SPACE",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "snareDepth", 1 }, "SNARE",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "riserDepth", 1 }, "RISER",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "gateDepth", 1 }, "GATE",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "driveDepth", 1 }, "DRIVE",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "impactDepth", 1 }, "IMPACT",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "output", 1 }, "OUTPUT",
        juce::NormalisableRange<float>(-24.0f, 6.0f), 0.0f));

    return { params.begin(), params.end() };
}

VertigoAudioProcessor::VertigoAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

VertigoAudioProcessor::~VertigoAudioProcessor() {}

void VertigoAudioProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
}

void VertigoAudioProcessor::releaseResources() {}

bool VertigoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void VertigoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    // M1: pass-through — audio passes unchanged
}

juce::AudioProcessorEditor* VertigoAudioProcessor::createEditor()
{
    return new VertigoAudioProcessorEditor(*this);
}

void VertigoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VertigoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VertigoAudioProcessor();
}
