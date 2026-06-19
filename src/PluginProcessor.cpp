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
        juce::ParameterID{ "pingPongDepth", 1 }, "PING PONG",
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

    // M8: Impact Cut
    impactCut.prepare(spec);

    // Ping Pong Delay
    pingPongDelay.prepare(spec);

    // Output Limiter — threshold -1 dBFS, 100ms release, always on
    limiter.prepare(spec);
    limiter.setThreshold(-1.0f);
    limiter.setRelease(100.0f);

    // Dry buffer for DRY/WET blend
    dryBuffer.setSize(2, samplesPerBlock, false, true, true);

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
    // --- M7: Read all parameters ---
    const float buildTarget    = apvts.getRawParameterValue("build")->load();
    const int   presetIndex    = static_cast<int>(apvts.getRawParameterValue("preset")->load());
    const float mixWet         = apvts.getRawParameterValue("mix")->load();
    const float hpfDepth       = apvts.getRawParameterValue("hpfDepth")->load();
    const float verbDepth      = apvts.getRawParameterValue("verbDepth")->load();
    const float driveDepth     = apvts.getRawParameterValue("driveDepth")->load();
    const float pingPongDepth  = apvts.getRawParameterValue("pingPongDepth")->load();
    const float snareDepth     = apvts.getRawParameterValue("snareDepth")->load();
    const float riserDepth     = apvts.getRawParameterValue("riserDepth")->load();
    const float gateDepth      = apvts.getRawParameterValue("gateDepth")->load();
    const float impactDepth    = apvts.getRawParameterValue("impactDepth")->load();
    const float outputGainDB   = apvts.getRawParameterValue("output")->load();
    const float outputGain     = juce::Decibels::decibelsToGain(outputGainDB);

    // Smooth BUILD over 50ms
    smoothedBuild.setTargetValue(buildTarget);
    const float build = smoothedBuild.skip(numSamples);

    // --- Macro activations via smoothstep + preset table ---
    const PresetParams& preset = kPresets[presetIndex];
    const float hpfActivation    = smoothstep(preset.hpfOnset,    preset.hpfFull,    build) * hpfDepth;
    const float verbActivation   = smoothstep(preset.verbOnset,   preset.verbFull,   build) * verbDepth;
    const float driveActivation  = smoothstep(preset.driveOnset,  preset.driveFull,  build) * driveDepth;
    const float snareActivation  = smoothstep(preset.snareOnset,  preset.snareFull,  build) * snareDepth;
    const float riserActivation  = smoothstep(preset.riserOnset,  preset.riserFull,  build) * riserDepth;
    const float gateActivation   = smoothstep(preset.gateOnset,   preset.gateFull,   build) * gateDepth;
    const float impactActivation = smoothstep(preset.impactOnset, 1.0f,              build) * impactDepth;

    // --- Update DSP module parameters ---
    hpfSweep.setActivation(hpfActivation, 20.0f, preset.hpfCeilHz, preset.hpfMaxRes);
    spaceVerb.setActivation(verbActivation, preset.verbCeil);
    driveModule.setActivation(driveActivation, preset.driveCeil);
    impactCut.setActivation(impactActivation, preset.impactGapDepth);

    // BPM and PPQ position from host or fallback
    double bpm = 120.0;
    double ppqPosition = 0.0;
    bool hasPpq = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
            if (pos->getPpqPosition().hasValue())
            {
                ppqPosition = *pos->getPpqPosition();
                hasPpq = true;
            }
        }
    }

    snareRush.setParams(bpm, snareActivation, preset.snareMaxDiv);
    pingPongDelay.setSync(bpm, snareRush.getCurrentDiv());
    noiseRiser.setActivation(riserActivation, preset.riserCeil);
    captureBuffer.setParams(bpm, gateActivation, preset.gateOnset);

    // --- Save dry signal before processing (for DRY/WET blend) ---
    if (numSamples <= dryBuffer.getNumSamples())
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // =====================================================================
    // PATH 1: THROUGHPUT (live input processed)
    // =====================================================================
    auto block = juce::dsp::AudioBlock<float>(buffer);

    // HPF sweep (M2)
    hpfSweep.process(block, hpfActivation);

    // Reverb after HPF (M3)
    if (verbActivation > 0.001f)
        spaceVerb.process(block);

    // Ping Pong Delay — after SpaceVerb, before Drive
    {
        const float ppActivation = smoothstep(preset.ppOnset, preset.ppFull, build);
        const float ppWet = ppActivation * preset.ppWetCeil * pingPongDepth;
        pingPongDelay.process(buffer, ppWet);
    }

    // Drive after verb + ping pong (M3)
    if (driveActivation > 0.001f)
        driveModule.process(block, driveActivation);

    // Capture Buffer: stutter/freeze from throughput signal (M6)
    if (gateActivation > 0.001f)
        captureBuffer.process(buffer, numSamples);

    // =====================================================================
    // PATH 2: GENERATORS (snare rush + noise riser)
    // =====================================================================
    if (numSamples <= generatorsBuffer.getNumSamples())
    {
        generatorsBuffer.clear(0, numSamples);

        if (snareActivation > 0.001f)
        {
            if (hasPpq)
                snareRush.process(generatorsBuffer, snareActivation, snareDepth,
                                  getSampleRate(), bpm, ppqPosition, numSamples);
            else
                snareRush.process(generatorsBuffer, numSamples);
        }

        if (riserActivation > 0.001f)
            noiseRiser.process(generatorsBuffer, numSamples);

        // Sum GENERATORS into the wet signal
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, generatorsBuffer, ch, 0, numSamples);
    }

    // =====================================================================
    // PATH 3: DRY/WET blend — mix dry original with processed wet
    // =====================================================================
    if (mixWet < 0.9999f && numSamples <= dryBuffer.getNumSamples())
    {
        const float dryGain = 1.0f - mixWet;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.applyGain(ch, 0, numSamples, mixWet);
            buffer.addFromWithRamp(ch, 0, dryBuffer.getReadPointer(ch), numSamples,
                                   dryGain, dryGain);
        }
    }

    // =====================================================================
    // Impact cut (M8) — applied to final mix
    // =====================================================================
    if (impactActivation > 0.001f)
        impactCut.process(buffer, numSamples);

    // Output gain
    buffer.applyGain(outputGain);

    // Output Limiter — always on, last in chain
    {
        auto limiterBlock = juce::dsp::AudioBlock<float>(buffer);
        auto limiterCtx   = juce::dsp::ProcessContextReplacing<float>(limiterBlock);
        limiter.process(limiterCtx);
    }
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
