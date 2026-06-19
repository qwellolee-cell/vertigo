#include "PluginEditor.h"

//==============================================================================
// Desert Vision palette
//==============================================================================
namespace VertigoColors
{
    const juce::Colour bg        = juce::Colour(0xffEAE8E1); // warm beige
    const juce::Colour bgPanel   = juce::Colour(0xffE0DDD3); // slightly darker beige for subtle panels
    const juce::Colour ink       = juce::Colour(0xff1A1A1A); // near-black line-art / text
    const juce::Colour inkSoft   = juce::Colour(0xff8A8780); // muted grey for inactive arcs / secondary text
    const juce::Colour accent    = juce::Colour(0xff6B2D8B); // restrained deep purple — BUILD hero active arc
}

namespace
{
    // A light, generously-tracked font for the editorial look. JUCE's tracking is
    // expressed as a fraction of the font height via withExtraKerningFactor.
    juce::Font lightFont(float height, float tracking = 0.18f)
    {
        return juce::Font(juce::FontOptions(height)
                              .withStyle("Regular"))
            .withExtraKerningFactor(tracking);
    }

    // Draws a small Archimedean spiral centred at (cx, cy). Used both for the
    // BUILD vortex and (smaller) inside the brand-mark eye.
    void drawSpiral(juce::Graphics& g, float cx, float cy, float maxR,
                    int turns, float strokeW, juce::Colour colour, float step = 0.12f)
    {
        juce::Path spiral;
        const float total = (float) turns * juce::MathConstants<float>::twoPi;
        bool first = true;
        for (float t = 0.0f; t <= total; t += step)
        {
            const float r  = maxR * (t / total);
            const float px = cx + std::cos(t) * r;
            const float py = cy + std::sin(t) * r;
            if (first) { spiral.startNewSubPath(px, py); first = false; }
            else        spiral.lineTo(px, py);
        }
        g.setColour(colour);
        g.strokePath(spiral, juce::PathStrokeType(strokeW, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
}

//==============================================================================
// VertigoLookAndFeel
//==============================================================================

VertigoAudioProcessorEditor::VertigoLookAndFeel::VertigoLookAndFeel()
{
    using namespace VertigoColors;

    // Rotary
    setColour(juce::Slider::rotarySliderFillColourId,    accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, inkSoft);
    setColour(juce::Slider::thumbColourId,               ink);

    // Labels
    setColour(juce::Label::textColourId,                 ink);

    // Combo box
    setColour(juce::ComboBox::backgroundColourId,        bgPanel);
    setColour(juce::ComboBox::textColourId,              ink);
    setColour(juce::ComboBox::outlineColourId,           ink);
    setColour(juce::ComboBox::arrowColourId,             ink);

    // Popup menu — beige with ink text, accent highlight
    setColour(juce::PopupMenu::backgroundColourId,           bg);
    setColour(juce::PopupMenu::textColourId,                 ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour(juce::PopupMenu::highlightedTextColourId,      bg);

    // Slider value bubble — readable on beige
    setColour(juce::BubbleComponent::backgroundColourId, bgPanel);
    setColour(juce::BubbleComponent::outlineColourId,    ink);
    setColour(juce::TooltipWindow::backgroundColourId,   bgPanel);
    setColour(juce::TooltipWindow::textColourId,         ink);
    setColour(juce::TooltipWindow::outlineColourId,      ink);
}

juce::Font VertigoAudioProcessorEditor::VertigoLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return lightFont(14.0f, 0.12f);
}

juce::Font VertigoAudioProcessorEditor::VertigoLookAndFeel::getPopupMenuFont()
{
    return lightFont(14.0f, 0.10f);
}

void VertigoAudioProcessorEditor::VertigoLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& slider)
{
    using namespace VertigoColors;

    const float radius  = juce::jmin(w / 2.0f, h / 2.0f) - 4.0f;
    const float centreX = x + w * 0.5f;
    const float centreY = y + h * 0.5f;
    const float angle   = startAngle + sliderPos * (endAngle - startAngle);

    const bool isHero = (slider.getName() == "BUILD") || (radius > 70.0f);

    if (isHero)
    {
        //======================= THE VORTEX ===========================
        // Concentric thin rings, denser toward the centre, for the vertigo
        // illusion. Use a non-linear spacing so rings crowd inward.
        const int rings = 7;
        for (int i = 1; i <= rings; ++i)
        {
            const float frac = (float) i / (float) rings;
            // bias rings toward the centre (squared) so they get denser inward
            const float rr = radius * (0.20f + 0.62f * (frac * frac));
            const float a  = 0.18f + 0.30f * frac; // outer rings a touch stronger
            g.setColour(ink.withAlpha(a));
            g.drawEllipse(centreX - rr, centreY - rr, rr * 2.0f, rr * 2.0f, 1.2f);
        }

        // Archimedean spiral on top — the soul of the mark.
        drawSpiral(g, centreX, centreY, radius * 0.78f, 5, 1.2f, ink.withAlpha(0.55f));

        // A second, counter-rotated faint spiral adds depth / shimmer.
        {
            juce::Path s2;
            const int turns = 5;
            const float total = (float) turns * juce::MathConstants<float>::twoPi;
            bool first = true;
            for (float t = 0.0f; t <= total; t += 0.12f)
            {
                const float r  = (radius * 0.70f) * (t / total);
                const float px = centreX + std::cos(-t + juce::MathConstants<float>::pi) * r;
                const float py = centreY + std::sin(-t + juce::MathConstants<float>::pi) * r;
                if (first) { s2.startNewSubPath(px, py); first = false; }
                else        s2.lineTo(px, py);
            }
            g.setColour(ink.withAlpha(0.20f));
            g.strokePath(s2, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        }

        // Outer inactive ring (full sweep) — soft grey guide.
        {
            juce::Path track;
            track.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                                startAngle, endAngle, true);
            g.setColour(inkSoft.withAlpha(0.55f));
            g.strokePath(track, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        // Value arc in restrained purple — the BUILD position.
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                                   startAngle, angle, true);
            g.setColour(accent);
            g.strokePath(valueArc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        // Thin radial indicator from centre to the current angle.
        {
            const float ix = centreX + std::sin(angle) * radius;
            const float iy = centreY - std::cos(angle) * radius;
            g.setColour(ink.withAlpha(0.85f));
            g.drawLine(centreX, centreY, ix, iy, 1.4f);
            // tiny indicator dot at the rim
            g.setColour(accent);
            g.fillEllipse(ix - 2.6f, iy - 2.6f, 5.2f, 5.2f);
        }

        // Numeric value, centred, light font.
        g.setColour(ink);
        g.setFont(lightFont(radius * 0.26f, 0.06f));
        g.drawText(juce::String(slider.getValue(), 2),
                   juce::Rectangle<float>(centreX - radius, centreY - radius * 0.30f,
                                          radius * 2.0f, radius * 0.60f),
                   juce::Justification::centred);
    }
    else
    {
        //==================== MINIMAL TRIM KNOB =======================
        // Light inactive background arc (270° via start/end angle).
        {
            juce::Path track;
            track.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                                startAngle, endAngle, true);
            g.setColour(inkSoft.withAlpha(0.65f));
            g.strokePath(track, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        // Ink value arc.
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                                   startAngle, angle, true);
            g.setColour(ink);
            g.strokePath(valueArc, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        // Tiny indicator dot at the current angle.
        {
            const float dotR = radius * 0.86f;
            const float dx = centreX + std::sin(angle) * dotR;
            const float dy = centreY - std::cos(angle) * dotR;
            g.setColour(ink);
            g.fillEllipse(dx - 2.0f, dy - 2.0f, 4.0f, 4.0f);
        }
    }
}

void VertigoAudioProcessorEditor::VertigoLookAndFeel::drawComboBox(
    juce::Graphics& g, int w, int h, bool /*isDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box)
{
    using namespace VertigoColors;

    juce::Rectangle<float> boxBounds(0.0f, 0.0f, (float) w, (float) h);

    g.setColour(bgPanel);
    g.fillRoundedRectangle(boxBounds, 3.0f);

    g.setColour(ink);
    g.drawRoundedRectangle(boxBounds.reduced(0.6f), 3.0f, 1.2f);

    // Thin chevron.
    juce::Rectangle<float> arrowZone((float) (w - 22), 0.0f, 16.0f, (float) h);
    juce::Path chevron;
    chevron.startNewSubPath(arrowZone.getX() + 3.0f,    arrowZone.getCentreY() - 2.5f);
    chevron.lineTo(arrowZone.getCentreX(),               arrowZone.getCentreY() + 3.0f);
    chevron.lineTo(arrowZone.getRight() - 3.0f,          arrowZone.getCentreY() - 2.5f);
    g.setColour(ink.withAlpha(0.85f));
    g.strokePath(chevron, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::ignoreUnused(box);
}

//==============================================================================
// VertigoAudioProcessorEditor
//==============================================================================

VertigoAudioProcessorEditor::VertigoAudioProcessorEditor(VertigoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&laf);
    setSize(660, 500);

    // ---- BUILD knob (hero vortex, centred) ----
    setupKnob(buildKnob, buildLabel, "build", true);
    buildKnob.setName("BUILD");

    // ---- Preset selector ----
    presetBox.addItem("Melodic House", 1);
    presetBox.addItem("Indie Dance",   2);
    presetBox.addItem("Big Room",      3);
    presetBox.addItem("Techno",        4);
    presetBox.setSelectedId(2);
    presetBox.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetBox);

    // The preset label is intentionally hidden in the minimal layout (the combo
    // text speaks for itself), but kept wired for completeness.
    presetLabel.setText("preset", juce::dontSendNotification);
    presetLabel.setFont(lightFont(10.0f, 0.25f));
    presetLabel.setJustificationType(juce::Justification::centred);
    presetLabel.setColour(juce::Label::textColourId, VertigoColors::inkSoft);
    addAndMakeVisible(presetLabel);

    // ---- Depth trim knobs (lowercase, editorial labels) ----
    setupKnob(hpfKnob,     hpfLabel,     "hpf");
    setupKnob(verbKnob,    verbLabel,    "space");
    setupKnob(snareKnob,   snareLabel,   "snare");
    setupKnob(riserKnob,   riserLabel,   "riser");
    setupKnob(gateKnob,    gateLabel,    "gate");
    setupKnob(driveKnob,   driveLabel,   "drive");
    setupKnob(ppDelayKnob, ppDelayLabel, "ping pong");
    setupKnob(impactKnob,  impactLabel,  "impact");

    // ---- Mix / Output (global) ----
    setupKnob(mixKnob,    mixLabel,    "mix");
    setupKnob(outputKnob, outputLabel, "output");

    // ---- APVTS Attachments (UNCHANGED — same IDs / wiring) ----
    auto& apvts = audioProcessor.apvts;
    buildAtt  = std::make_unique<SliderAttachment>(apvts, "build",       buildKnob);
    mixAtt    = std::make_unique<SliderAttachment>(apvts, "mix",         mixKnob);
    outputAtt = std::make_unique<SliderAttachment>(apvts, "output",      outputKnob);
    hpfAtt      = std::make_unique<SliderAttachment>(apvts, "hpfDepth",     hpfKnob);
    verbAtt     = std::make_unique<SliderAttachment>(apvts, "verbDepth",    verbKnob);
    snareAtt    = std::make_unique<SliderAttachment>(apvts, "snareDepth",   snareKnob);
    riserAtt    = std::make_unique<SliderAttachment>(apvts, "riserDepth",   riserKnob);
    gateAtt     = std::make_unique<SliderAttachment>(apvts, "gateDepth",    gateKnob);
    driveAtt    = std::make_unique<SliderAttachment>(apvts, "driveDepth",   driveKnob);
    ppDelayAtt  = std::make_unique<SliderAttachment>(apvts, "pingPongDepth", ppDelayKnob);
    impactAtt   = std::make_unique<SliderAttachment>(apvts, "impactDepth",  impactKnob);
    presetAtt   = std::make_unique<ComboAttachment> (apvts, "preset",       presetBox);
}

VertigoAudioProcessorEditor::~VertigoAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VertigoAudioProcessorEditor::setupKnob(juce::Slider& s, juce::Label& l,
                                             const juce::String& labelText, bool isBig)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                          juce::MathConstants<float>::pi * 2.75f, true); // 270° sweep
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setPopupDisplayEnabled(true, false, this);
    addAndMakeVisible(s);

    l.setText(labelText, juce::dontSendNotification);
    l.setFont(lightFont(isBig ? 14.0f : 10.5f, isBig ? 0.32f : 0.22f));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, VertigoColors::ink);
    addAndMakeVisible(l);
}

//==============================================================================
// Brand mark — the Desert Vision "eye + spiral" + lowercase wordmark.
//==============================================================================
void VertigoAudioProcessorEditor::drawBrandMark(juce::Graphics& g,
                                                juce::Rectangle<float> area)
{
    using namespace VertigoColors;

    const float cx = area.getCentreX();
    const float cy = area.getCentreY();

    // Eye dimensions.
    const float eyeW = 64.0f;   // half-width
    const float eyeH = 22.0f;   // half-height (vertical opening)

    // Two opposing arcs meeting at sharp points left & right.
    // Build the upper lid as a quadratic from left point to right point, then
    // the lower lid back, so the corners come to a point.
    const float leftX  = cx - eyeW;
    const float rightX = cx + eyeW;

    juce::Path eye;
    eye.startNewSubPath(leftX, cy);
    eye.quadraticTo(cx, cy - eyeH, rightX, cy);   // upper lid
    eye.quadraticTo(cx, cy + eyeH, leftX, cy);    // lower lid
    eye.closeSubPath();

    g.setColour(ink);
    g.strokePath(eye, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    // Small spiral "pupil" at the centre — ties the mark to "vertigo".
    drawSpiral(g, cx, cy, 11.0f, 3, 1.2f, ink.withAlpha(0.85f), 0.10f);

    // Wordmark below the eye: lowercase, wide tracking.
    g.setColour(ink);
    g.setFont(lightFont(22.0f, 0.55f));
    g.drawText("vertigo",
               juce::Rectangle<float>(area.getX(), cy + eyeH + 6.0f,
                                      area.getWidth(), 26.0f),
               juce::Justification::centred);

    // A whisper-thin tagline.
    g.setColour(inkSoft);
    g.setFont(lightFont(8.5f, 0.45f));
    g.drawText("build macro",
               juce::Rectangle<float>(area.getX(), cy + eyeH + 32.0f,
                                      area.getWidth(), 12.0f),
               juce::Justification::centred);
}

void VertigoAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace VertigoColors;

    // Solid beige base.
    g.fillAll(bg);

    // Very subtle radial lightening toward the centre for depth.
    {
        const float cx = getWidth() * 0.5f;
        const float cy = getHeight() * 0.46f;
        juce::ColourGradient glow(bg.brighter(0.05f), cx, cy,
                                  bg, cx, (float) getHeight(), true);
        glow.addColour(0.55, bg);
        g.setGradientFill(glow);
        g.fillRect(getLocalBounds());
    }

    // Brand mark header.
    drawBrandMark(g, juce::Rectangle<float>(0.0f, 6.0f, (float) getWidth(), 80.0f));

    // A hairline separator beneath the global mix/output pair, to read them as
    // a distinct "master" group. Positioned just above that bottom row.
    const float sepY = (float) (getHeight() - 118);
    g.setColour(inkSoft.withAlpha(0.45f));
    g.drawLine(getWidth() * 0.5f - 150.0f, sepY,
               getWidth() * 0.5f + 150.0f, sepY, 1.0f);
}

void VertigoAudioProcessorEditor::resized()
{
    const int w      = getWidth();
    const int margin = 24;

    // === Header occupies the top ~86px (drawn in paint()). ===
    const int headerH = 86;

    // === BUILD vortex — large, centred. ===
    const int buildSize = 200;
    const int buildX    = (w - buildSize) / 2;
    const int buildY    = headerH + 6;
    buildKnob.setBounds(buildX, buildY, buildSize, buildSize);
    buildLabel.setBounds(buildX, buildY + buildSize - 2, buildSize, 20);

    // === Preset selector — centred, below the build label. ===
    const int presetW = 150;
    const int presetH = 26;
    const int presetY = buildY + buildSize + 22;
    presetBox.setBounds((w - presetW) / 2, presetY, presetW, presetH);
    // Keep the (muted) preset caption out of the way, hidden under the combo.
    presetLabel.setBounds((w - presetW) / 2, presetY - 1, presetW, 1);
    presetLabel.setVisible(false);

    // === Row of 8 monoline trim knobs across the width. ===
    const int trimSize = 62;
    const int labelH   = 14;
    const int numTrims = 8;
    const int usableW  = w - 2 * margin;
    // Distribute 8 knobs with even spacing across the usable width.
    const float slot   = usableW / (float) numTrims;
    const int trimY    = presetY + presetH + 26;

    juce::Slider* trimKnobs[numTrims]  = { &hpfKnob, &verbKnob, &snareKnob, &riserKnob,
                                           &gateKnob, &driveKnob, &ppDelayKnob, &impactKnob };
    juce::Label*  trimLabels[numTrims] = { &hpfLabel, &verbLabel, &snareLabel, &riserLabel,
                                           &gateLabel, &driveLabel, &ppDelayLabel, &impactLabel };

    for (int i = 0; i < numTrims; ++i)
    {
        const int slotX  = margin + (int) std::round(i * slot);
        const int centre = slotX + (int) std::round(slot * 0.5f);
        const int kx     = centre - trimSize / 2;
        trimKnobs[i]->setBounds(kx, trimY, trimSize, trimSize);
        // Give labels the full slot width so "ping pong" doesn't clip.
        trimLabels[i]->setBounds(slotX, trimY + trimSize, (int) std::round(slot), labelH);
    }

    // === Bottom: mix + output (global master group). ===
    const int bottomSize = 62;
    const int bottomY    = getHeight() - bottomSize - labelH - 16;
    const int pairGap    = 96;
    const int mixX       = w / 2 - pairGap / 2 - bottomSize;
    const int outX       = w / 2 + pairGap / 2;

    mixKnob.setBounds(mixX, bottomY, bottomSize, bottomSize);
    mixLabel.setBounds(mixX, bottomY + bottomSize, bottomSize, labelH);

    outputKnob.setBounds(outX, bottomY, bottomSize, bottomSize);
    outputLabel.setBounds(outX, bottomY + bottomSize, bottomSize, labelH);
}
