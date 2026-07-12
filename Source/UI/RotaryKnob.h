#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Thin wrapper around juce::Slider configured as a rotary knob
// with a value label shown on hover. Used throughout the plugin's UI.
class RotaryKnob : public juce::Slider
{
public:
    RotaryKnob()
    {
        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 14);
        setVelocityBasedMode(false);
        setScrollWheelEnabled(true);
    }

    explicit RotaryKnob(const juce::String& name) : RotaryKnob()
    {
        setName(name);
    }
};
