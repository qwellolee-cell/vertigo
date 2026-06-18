#include "PluginEditor.h"

//==============================================================================
// VertigoLookAndFeel
//==============================================================================

VertigoAudioProcessorEditor::VertigoLookAndFeel::VertigoLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId,   juce::Colour(0xffe94560));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff3a3a5c));
    setColour(juce::Slider::thumbColourId,              juce::Colour(0xffe94560));
    setColour(juce::Label::textColourId,                juce::Colour(0xffccccdd));
    setColour(juce::ComboBox::backgroundColourId,       juce::Colour(0xff16213e));
    setColour(juce::ComboBox::textColourId,             juce::Colour(0xffccccdd));
    setColour(juce::ComboBox::outlineColourId,          juce::Colour(0xff3a3a5c));
    setColour(juce::PopupMenu::backgroundColourId,      juce::Colour(0xff16213e));
    setColour(juce::PopupMenu::textColourId,            juce::Colour(0xffccccdd));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffe94560));
}

void VertigoAudioProcessorEditor::VertigoLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& slider)
{
    const float radius = juce::jmin(w / 2.0f, h / 2.0f) - 4.0f;
    const float centreX = x + w * 0.5f;
    const float centreY = y + h * 0.5f;

    // Background track
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY, radius, radius,
                                  0.0f, startAngle, endAngle, true);
    g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
    g.strokePath(backgroundArc, juce::PathStrokeType(radius * 0.12f,
                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius,
                            0.0f, startAngle, angle, true);
    g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(valueArc, juce::PathStrokeType(radius * 0.12f,
                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Thumb line
    const float thumbX = centreX + std::sin(angle) * radius * 0.80f;
    const float thumbY = centreY - std::cos(angle) * radius * 0.80f;
    g.setColour(juce::Colours::white);
    g.drawLine(centreX, centreY, thumbX, thumbY, radius * 0.08f);

    // Centre dot
    g.fillEllipse(centreX - radius * 0.08f, centreY - radius * 0.08f,
                  radius * 0.16f, radius * 0.16f);

    // For the big build knob, show current value
    if (slider.getName() == "BUILD")
    {
        g.setColour(juce::Colour(0xffccccdd));
        g.setFont(radius * 0.30f);
        g.drawFittedText(juce::String(slider.getValue(), 2),
                         (int)(centreX - radius), (int)(centreY - radius * 0.3f),
                         (int)(radius * 2.0f), (int)(radius * 0.6f),
                         juce::Justification::centred, 1);
    }
}

void VertigoAudioProcessorEditor::VertigoLookAndFeel::drawComboBox(
    juce::Graphics& g, int w, int h, bool /*isDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box)
{
    juce::Rectangle<int> boxBounds(0, 0, w, h);
    g.setColour(findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), 4.0f);
    g.setColour(findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), 4.0f, 1.0f);

    juce::Rectangle<int> arrowZone(w - 20, 0, 15, h);
    juce::Path path;
    path.startNewSubPath((float)arrowZone.getX() + 3.0f,    (float)arrowZone.getCentreY() - 2.0f);
    path.lineTo((float)arrowZone.getCentreX(),               (float)arrowZone.getCentreY() + 3.0f);
    path.lineTo((float)arrowZone.getRight() - 3.0f,          (float)arrowZone.getCentreY() - 2.0f);

    g.setColour(findColour(juce::ComboBox::textColourId).withAlpha(0.8f));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    juce::ignoreUnused(box);
}

//==============================================================================
// VertigoAudioProcessorEditor
//==============================================================================

VertigoAudioProcessorEditor::VertigoAudioProcessorEditor(VertigoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&laf);
    setSize(560, 440);

    // ---- BUILD knob (big, centred) ----
    setupKnob(buildKnob, buildLabel, "BUILD", true);
    buildKnob.setName("BUILD");

    // ---- Preset selector ----
    presetBox.addItem("Melodic House", 1);
    presetBox.addItem("Indie Dance",   2);
    presetBox.addItem("Big Room",      3);
    presetBox.addItem("Techno",        4);
    presetBox.setSelectedId(2);
    addAndMakeVisible(presetBox);

    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    // ---- Depth trim knobs ----
    setupKnob(hpfKnob,    hpfLabel,    "HPF");
    setupKnob(verbKnob,   verbLabel,   "SPACE");
    setupKnob(snareKnob,  snareLabel,  "SNARE");
    setupKnob(riserKnob,  riserLabel,  "RISER");
    setupKnob(gateKnob,   gateLabel,   "GATE");
    setupKnob(driveKnob,  driveLabel,  "DRIVE");
    setupKnob(impactKnob, impactLabel, "IMPACT");

    // ---- Mix / Output ----
    setupKnob(mixKnob,    mixLabel,    "MIX");
    setupKnob(outputKnob, outputLabel, "OUTPUT");

    // ---- APVTS Attachments ----
    auto& apvts = audioProcessor.apvts;
    buildAtt  = std::make_unique<SliderAttachment>(apvts, "build",       buildKnob);
    mixAtt    = std::make_unique<SliderAttachment>(apvts, "mix",         mixKnob);
    outputAtt = std::make_unique<SliderAttachment>(apvts, "output",      outputKnob);
    hpfAtt    = std::make_unique<SliderAttachment>(apvts, "hpfDepth",    hpfKnob);
    verbAtt   = std::make_unique<SliderAttachment>(apvts, "verbDepth",   verbKnob);
    snareAtt  = std::make_unique<SliderAttachment>(apvts, "snareDepth",  snareKnob);
    riserAtt  = std::make_unique<SliderAttachment>(apvts, "riserDepth",  riserKnob);
    gateAtt   = std::make_unique<SliderAttachment>(apvts, "gateDepth",   gateKnob);
    driveAtt  = std::make_unique<SliderAttachment>(apvts, "driveDepth",  driveKnob);
    impactAtt = std::make_unique<SliderAttachment>(apvts, "impactDepth", impactKnob);
    presetAtt = std::make_unique<ComboAttachment> (apvts, "preset",      presetBox);
}

VertigoAudioProcessorEditor::~VertigoAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VertigoAudioProcessorEditor::setupKnob(juce::Slider& s, juce::Label& l,
                                             const juce::String& labelText, bool isBig)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setPopupDisplayEnabled(true, false, this);
    addAndMakeVisible(s);

    l.setText(labelText, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions(isBig ? 13.0f : 10.0f).withStyle("Bold")));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colour(0xffccccdd));
    addAndMakeVisible(l);
}

void VertigoAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark gradient background
    juce::ColourGradient bg(juce::Colour(0xff1a1a2e), 0.0f, 0.0f,
                             juce::Colour(0xff16213e), 0.0f, (float)getHeight(),
                             false);
    g.setGradientFill(bg);
    g.fillRect(getLocalBounds());

    // Title
    g.setColour(juce::Colour(0xffe94560));
    g.setFont(juce::Font(juce::FontOptions(32.0f).withStyle("Bold")));
    g.drawText("VERTIGO", juce::Rectangle<int>(0, 8, getWidth(), 42),
               juce::Justification::centred);

    // Subtitle
    g.setColour(juce::Colour(0xff6666aa));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("ONE KNOB BUILD MACRO", juce::Rectangle<int>(0, 42, getWidth(), 18),
               juce::Justification::centred);

    // Separator line
    g.setColour(juce::Colour(0xff3a3a5c));
    g.drawHorizontalLine(64, 20.0f, (float)(getWidth() - 20));
}

void VertigoAudioProcessorEditor::resized()
{
    const int w = getWidth();

    // === BUILD knob — large, centred top ===
    const int buildSize = 160;
    const int buildX = (w - buildSize) / 2;
    buildKnob.setBounds(buildX, 72, buildSize, buildSize);
    buildLabel.setBounds(buildX, 72 + buildSize, buildSize, 18);

    // === PRESET selector — below BUILD label ===
    const int presetW = 200;
    presetBox.setBounds((w - presetW) / 2, 258, presetW, 26);
    presetLabel.setBounds((w - presetW) / 2 - 54, 258, 50, 26);

    // === Depth trim row ===
    const int trimSize = 54;
    const int trimY    = 298;
    const int labelH   = 14;
    const int numTrims = 7;
    const int totalTrimW = numTrims * trimSize + (numTrims - 1) * 8;
    int trimX = (w - totalTrimW) / 2;

    auto placeTrim = [&](juce::Slider& s, juce::Label& l)
    {
        s.setBounds(trimX, trimY, trimSize, trimSize);
        l.setBounds(trimX, trimY + trimSize, trimSize, labelH);
        trimX += trimSize + 8;
    };

    placeTrim(hpfKnob,    hpfLabel);
    placeTrim(verbKnob,   verbLabel);
    placeTrim(snareKnob,  snareLabel);
    placeTrim(riserKnob,  riserLabel);
    placeTrim(gateKnob,   gateLabel);
    placeTrim(driveKnob,  driveLabel);
    placeTrim(impactKnob, impactLabel);

    // === Bottom row: MIX + OUTPUT ===
    const int bottomSize = 54;
    const int bottomY    = 372;
    const int bottomGap  = 80;
    const int mixX       = (w - bottomGap) / 2 - bottomSize - 8;
    const int outX       = (w + bottomGap) / 2 + 8;

    mixKnob.setBounds(mixX, bottomY, bottomSize, bottomSize);
    mixLabel.setBounds(mixX, bottomY + bottomSize, bottomSize, labelH);

    outputKnob.setBounds(outX, bottomY, bottomSize, bottomSize);
    outputLabel.setBounds(outX, bottomY + bottomSize, bottomSize, labelH);
}
