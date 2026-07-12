#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

// VU meter component — stereo RMS + peak hold display with clip indicator.
// Matches the visual behaviour of the extension's popup VU widget.
class VUMeter : public juce::Component, private juce::Timer
{
public:
    explicit VUMeter(BassNukerProcessor& p) : proc(p)
    {
        startTimerHz(24);
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float channelW = b.getWidth() * 0.42f;
        const float gapW     = b.getWidth() - channelW * 2.0f;

        drawChannel(g, b.removeFromLeft(channelW), displayL, peakL, displayClip);
        g.setColour(juce::Colour(0xff0a0a1a));
        g.fillRect(b.removeFromLeft(gapW));
        drawChannel(g, b, displayR, peakR, displayClip);
    }

    void timerCallback() override
    {
        displayL   = proc.vuLevelL.load();
        displayR   = proc.vuLevelR.load();
        peakL      = proc.vuPeakL.load();
        peakR      = proc.vuPeakR.load();
        displayClip= proc.clipping.load();
        repaint();
    }

private:
    BassNukerProcessor& proc;
    float displayL = 0, displayR = 0;
    float peakL = 0, peakR = 0;
    bool  displayClip = false;

    void drawChannel(juce::Graphics& g, juce::Rectangle<float> b,
                     float rms, float peak, bool clip)
    {
        // Background
        g.setColour(juce::Colour(0xff0a0a1a));
        g.fillRect(b);

        // Color gradient: green -> yellow -> red
        const float barH   = b.getHeight() * std::min(1.0f, rms * 1.4f);
        const float barY   = b.getBottom() - barH;

        const float yellow = b.getBottom() - b.getHeight() * 0.7f;
        const float red    = b.getBottom() - b.getHeight() * 0.9f;

        if (barH > 0)
        {
            const float topY = barY;
            // Green portion (0 – 70%)
            if (topY < yellow)
            {
                g.setColour(juce::Colour(0xff39d353));
                g.fillRect(b.getX(), yellow, b.getWidth(), b.getBottom() - yellow);
            }
            else
            {
                g.setColour(juce::Colour(0xff39d353));
                g.fillRect(b.getX(), topY, b.getWidth(), b.getBottom() - topY);
            }

            // Yellow portion (70% – 90%)
            if (topY < red)
            {
                g.setColour(juce::Colour(0xfff7b731));
                g.fillRect(b.getX(), red, b.getWidth(), yellow - red);
                if (topY < yellow)
                {
                    // Red portion (>90%)
                    g.setColour(clip ? juce::Colour(0xffff3333) : juce::Colour(0xffdd2222));
                    g.fillRect(b.getX(), topY, b.getWidth(), red - topY);
                }
            }
            else if (topY < yellow)
            {
                g.setColour(juce::Colour(0xfff7b731));
                g.fillRect(b.getX(), topY, b.getWidth(), yellow - topY);
            }
        }

        // Peak hold line
        const float pkY = b.getBottom() - b.getHeight() * std::min(1.0f, peak * 1.4f);
        g.setColour(juce::Colour(0xffffffff));
        g.fillRect(b.getX(), pkY - 1.0f, b.getWidth(), 2.0f);

        // Border
        g.setColour(juce::Colour(0xff2a3a5a));
        g.drawRect(b, 1.0f);
    }
};
