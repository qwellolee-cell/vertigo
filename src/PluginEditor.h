#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

/**
 * VERTIGO Editor — M8
 *
 * Layout:
 *   - Large BUILD knob (centred, top half)
 *   - PRESET combo box below BUILD
 *   - Row of depth trim knobs: HPF, SPACE, SNARE, RISER, GATE, DRIVE, IMPACT
 *   - DRY/WET and OUTPUT at bottom
 *
 * Dark blue-purple theme: bg #1a1a2e, accent #e94560
 */
class VertigoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VertigoAudioProcessorEditor(VertigoAudioProcessor&);
    ~VertigoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VertigoAudioProcessor& audioProcessor;

    // Custom look and feel
    struct VertigoLookAndFeel : public juce::LookAndFeel_V4
    {
        VertigoLookAndFeel();
        void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                              float sliderPos, float startAngle, float endAngle,
                              juce::Slider&) override;
        void drawComboBox(juce::Graphics&, int w, int h, bool isDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox&) override;
    };

    VertigoLookAndFeel laf;

    // ---- Widgets ----
    juce::Slider buildKnob;
    juce::Label  buildLabel;

    juce::ComboBox  presetBox;
    juce::Label     presetLabel;

    // Depth trims
    juce::Slider hpfKnob, verbKnob, snareKnob, riserKnob, gateKnob, driveKnob, ppDelayKnob, impactKnob;
    juce::Label  hpfLabel, verbLabel, snareLabel, riserLabel, gateLabel, driveLabel, ppDelayLabel, impactLabel;

    juce::Slider mixKnob, outputKnob;
    juce::Label  mixLabel, outputLabel;

    // ---- APVTS attachments ----
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> buildAtt, mixAtt, outputAtt;
    std::unique_ptr<SliderAttachment> hpfAtt, verbAtt, snareAtt, riserAtt, gateAtt, driveAtt, ppDelayAtt, impactAtt;
    std::unique_ptr<ComboAttachment>  presetAtt;

    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& labelText,
                   bool isBig = false);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertigoAudioProcessorEditor)
};
