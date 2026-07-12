#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ─────────────────────────────────────────────────────────────────────────────
// Bass Nuker custom LookAndFeel
//
// Visual style:
//   Dark charcoal background (#1a1a2e), neon-blue/cyan accent (#00d4ff),
//   orange secondary accent (#ff6b35).  Inspired by Creative's dark UI aesthetic.
//   All knobs are flat with a luminous arc track; sliders are thin with a
//   highlighted filled-to-thumb region.
// ─────────────────────────────────────────────────────────────────────────────
class BassNukerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // ── Color palette ────────────────────────────────────────────────────────
    static constexpr auto kBg          = 0xff1a1a2e;  // deep navy/charcoal
    static constexpr auto kPanel       = 0xff16213e;  // panel background
    static constexpr auto kPanelBright = 0xff0f3460;  // slightly lighter panel
    static constexpr auto kAccent      = 0xff00d4ff;  // neon cyan
    static constexpr auto kAccentAlt   = 0xffff6b35;  // orange
    static constexpr auto kAccentGreen = 0xff39d353;  // green (ON state)
    static constexpr auto kTextBright  = 0xfff0f0f0;
    static constexpr auto kTextDim     = 0xff8899aa;
    static constexpr auto kSeparator   = 0xff2a3a5a;
    static constexpr auto kDisabled    = 0xff445566;

    BassNukerLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(kBg));
        setColour(juce::Label::textColourId,                 juce::Colour(kTextBright));
        setColour(juce::Label::backgroundColourId,           juce::Colour(0));
        setColour(juce::TextButton::buttonColourId,          juce::Colour(kPanelBright));
        setColour(juce::TextButton::buttonOnColourId,        juce::Colour(kAccentGreen));
        setColour(juce::TextButton::textColourOffId,         juce::Colour(kTextDim));
        setColour(juce::TextButton::textColourOnId,          juce::Colour(kBg));
        setColour(juce::ComboBox::backgroundColourId,        juce::Colour(kPanel));
        setColour(juce::ComboBox::textColourId,              juce::Colour(kTextBright));
        setColour(juce::ComboBox::outlineColourId,           juce::Colour(kSeparator));
        setColour(juce::ComboBox::arrowColourId,             juce::Colour(kAccent));
        setColour(juce::PopupMenu::backgroundColourId,       juce::Colour(kPanel));
        setColour(juce::PopupMenu::textColourId,             juce::Colour(kTextBright));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(kPanelBright));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colour(kAccent));
        setColour(juce::Slider::thumbColourId,               juce::Colour(kAccent));
        setColour(juce::Slider::trackColourId,               juce::Colour(kSeparator));
        setColour(juce::ToggleButton::tickColourId,          juce::Colour(kAccentGreen));
        setColour(juce::ScrollBar::thumbColourId,            juce::Colour(kPanelBright));
        setColour(juce::GroupComponent::outlineColourId,     juce::Colour(kSeparator));
        setColour(juce::GroupComponent::textColourId,        juce::Colour(kAccent));
    }

    // ── Rotary knob ──────────────────────────────────────────────────────────
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        const float radius  = std::min(width, height) * 0.45f;
        const float centreX = x + width  * 0.5f;
        const float centreY = y + height * 0.5f;

        // Background circle
        g.setColour(juce::Colour(kPanel));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        // Outer ring
        g.setColour(juce::Colour(kSeparator));
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.5f);

        // Arc track (full range, dim)
        const float arcThick = radius * 0.14f;
        juce::Path track;
        track.addArc(centreX - radius * 0.85f, centreY - radius * 0.85f,
                     radius * 1.7f, radius * 1.7f,
                     rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(kSeparator));
        g.strokePath(track, juce::PathStrokeType(arcThick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arc track (filled portion, accent)
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        juce::Path filled;
        filled.addArc(centreX - radius * 0.85f, centreY - radius * 0.85f,
                      radius * 1.7f, radius * 1.7f,
                      rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(kAccent));
        g.strokePath(filled, juce::PathStrokeType(arcThick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer line
        const float pr = radius * 0.55f;
        const float px = centreX + pr * std::cos(angle - juce::MathConstants<float>::halfPi);
        const float py = centreY + pr * std::sin(angle - juce::MathConstants<float>::halfPi);
        g.setColour(juce::Colour(kTextBright));
        g.drawLine(centreX, centreY, px, py, 2.0f);

        // Centre dot
        g.setColour(juce::Colour(kAccent));
        g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
    }

    // ── Linear slider ────────────────────────────────────────────────────────
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearHorizontal)
        {
            const float trackH   = 4.0f;
            const float trackY   = y + height * 0.5f - trackH * 0.5f;
            const float thumbW   = 10.0f, thumbH = 18.0f;

            // Background track
            g.setColour(juce::Colour(kSeparator));
            g.fillRoundedRectangle(static_cast<float>(x), trackY,
                                   static_cast<float>(width), trackH, trackH * 0.5f);

            // Filled portion
            g.setColour(juce::Colour(kAccent));
            g.fillRoundedRectangle(static_cast<float>(x), trackY,
                                   sliderPos - x, trackH, trackH * 0.5f);

            // Thumb
            g.setColour(juce::Colour(kAccent));
            g.fillRoundedRectangle(sliderPos - thumbW * 0.5f,
                                   y + height * 0.5f - thumbH * 0.5f,
                                   thumbW, thumbH, 3.0f);
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height,
                                                   sliderPos, minSliderPos, maxSliderPos,
                                                   style, slider);
        }
    }

    // ── Toggle button ────────────────────────────────────────────────────────
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
                          bool isMouseOverButton, bool isButtonDown) override
    {
        const bool isOn = btn.getToggleState();
        const auto bounds = btn.getLocalBounds().toFloat();
        const float r = 5.0f;

        // Background pill
        g.setColour(isOn ? juce::Colour(kAccentGreen).withAlpha(0.2f)
                         : juce::Colour(kPanel));
        g.fillRoundedRectangle(bounds, r);

        // Outline
        g.setColour(isOn ? juce::Colour(kAccentGreen) : juce::Colour(kSeparator));
        g.drawRoundedRectangle(bounds.reduced(0.5f), r, 1.5f);

        // Label
        g.setColour(isOn ? juce::Colour(kAccentGreen) : juce::Colour(kTextDim));
        g.setFont(juce::Font("Segoe UI", 11.0f, juce::Font::bold));
        g.drawText(btn.getButtonText(), bounds, juce::Justification::centred, false);
    }

    // ── TextButton ───────────────────────────────────────────────────────────
    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& /*bg*/, bool /*over*/, bool /*down*/) override
    {
        const auto bounds = btn.getLocalBounds().toFloat().reduced(0.5f);
        const bool isOn   = btn.getToggleState();
        const float r     = 4.0f;

        g.setColour(isOn ? juce::Colour(kAccentGreen).withAlpha(0.15f) : juce::Colour(kPanel));
        g.fillRoundedRectangle(bounds, r);
        g.setColour(isOn ? juce::Colour(kAccentGreen) : juce::Colour(kSeparator));
        g.drawRoundedRectangle(bounds, r, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool /*over*/, bool /*down*/) override
    {
        const bool isOn = btn.getToggleState();
        g.setColour(isOn ? juce::Colour(kAccentGreen) : juce::Colour(kTextBright));
        g.setFont(juce::Font("Segoe UI", 12.0f, juce::Font::plain));
        g.drawText(btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred, true);
    }

    // ── ComboBox ─────────────────────────────────────────────────────────────
    void drawComboBox(juce::Graphics& g, int width, int height,
                      bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/,
                      int /*buttonW*/, int /*buttonH*/, juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
        g.setColour(juce::Colour(kPanel));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(kSeparator));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        // Arrow
        const float arrowX = width - 14.0f, arrowY = height * 0.5f;
        juce::Path arrow;
        arrow.addTriangle(arrowX - 4, arrowY - 2, arrowX + 4, arrowY - 2, arrowX, arrowY + 3);
        g.setColour(juce::Colour(kAccent));
        g.fillPath(arrow);
    }
};
