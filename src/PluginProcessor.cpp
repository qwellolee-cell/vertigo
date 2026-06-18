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

void VertigoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    // M2: HPF
    hpfSweep.prepare(spec);

    // M3: Verb + Drive
    spaceVerb.prepare(spec);
    driveModule.prepare(spec);

    // M4: Snare Rush
    snareRush.prepare(spec);

    // M5: Noise Riser
    noiseRiser.prepare(spec);

    // M6: Capture Buffer
    captureBuffer.prepare(spec);

    // Allocate generators buffer
    generatorsBuffer.setSize(2, samplesPerBlock, false, true, true);

    // Smooth BUILD knob over 50ms to avoid filter frequency jumps
    smoothedBuild.reset(sampleRate, 0.05);
    smoothedBuild.setCurrentAndTargetValue(0.0f);
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

    const int numSamples = buffer.getNumSamples();

    // Read parameters
    const float buildTarget  = apvts.getRawParameterValue("build")->load();
    const int   presetIndex  = static_cast<int>(apvts.getRawParameterValue("preset")->load());
    const float hpfDepth     = apvts.getRawParameterValue("hpfDepth")->load();
    const float verbDepth    = apvts.getRawParameterValue("verbDepth")->load();
    const float driveDepth   = apvts.getRawParameterValue("driveDepth")->load();
    const float snareDepth   = apvts.getRawParameterValue("snareDepth")->load();
    const float riserDepth   = apvts.getRawParameterValue("riserDepth")->load();
    const float gateDepth    = apvts.getRawParameterValue("gateDepth")->load();
    const float outputGainDB = apvts.getRawParameterValue("output")->load();

    const float outputGain   = juce::Decibels::decibelsToGain(outputGainDB);

    // Update smoothed build
    smoothedBuild.setTargetValue(buildTarget);

    // Get current build (block-level approximation — good enough)
    const float build = smoothedBuild.skip(numSamples);

    // Compute macro activations
    const PresetParams& preset = kPresets[presetIndex];
    const float hpfActivation   = smoothstep(preset.hpfOnset,   preset.hpfFull,   build) * hpfDepth;
    const float verbActivation  = smoothstep(preset.verbOnset,  preset.verbFull,  build) * verbDepth;
    const float driveActivation = smoothstep(preset.driveOnset, preset.driveFull, build) * driveDepth;
    const float snareActivation = smoothstep(preset.snareOnset, preset.snareFull, build) * snareDepth;
    const float riserActivation = smoothstep(preset.riserOnset, preset.riserFull, build) * riserDepth;
    const float gateActivation  = smoothstep(preset.gateOnset,  preset.gateFull,  build) * gateDepth;

    // M2: Update HPF parameters based on activation
    hpfSweep.setActivation(hpfActivation, 20.0f, preset.hpfCeilHz, preset.hpfMaxRes);

    // M3: Update verb + drive parameters
    spaceVerb.setActivation(verbActivation, preset.verbCeil);
    driveModule.setActivation(driveActivation, preset.driveCeil);

    // M4: Snare Rush — get BPM from host or fall back to 120
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
    }
    snareRush.setParams(bpm, snareActivation, preset.snareMaxDiv);

    // M5: Noise Riser
    noiseRiser.setActivation(riserActivation, preset.riserCeil);

    // M6: Capture Buffer stutter/freeze
    captureBuffer.setParams(bpm, gateActivation, preset.gateOnset);

    // Build audio block for DSP processing
    auto block = juce::dsp::AudioBlock<float>(buffer);

    // --- THROUGHPUT path ---
    // M2: HPF sweep
    hpfSweep.process(block, hpfActivation);

    // M3: Reverb after HPF (fed by mid/high content)
    if (verbActivation > 0.001f)
        spaceVerb.process(block);

    // M3: Drive after verb (colours the reverb tail too)
    if (driveActivation > 0.001f)
        driveModule.process(block, driveActivation);

    // M6: Capture Buffer — writes the post-HPF/verb/drive signal into ring buffer
    // and replaces output with looped slice when gate is active
    if (gateActivation > 0.001f)
        captureBuffer.process(buffer, numSamples);

    // --- GENERATORS path (M4+) ---
    if (numSamples <= generatorsBuffer.getNumSamples())
    {
        generatorsBuffer.clear(0, numSamples);

        // M4: Snare Rush
        if (snareActivation > 0.001f)
            snareRush.process(generatorsBuffer, numSamples);

        // M5: Noise Riser
        if (riserActivation > 0.001f)
            noiseRiser.process(generatorsBuffer, numSamples);

        // Add generators into main buffer (M7 will apply proper 3-path blend)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, generatorsBuffer, ch, 0, numSamples);
    }

    // Apply output gain
    buffer.applyGain(outputGain);
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
