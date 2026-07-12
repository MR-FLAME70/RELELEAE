#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>

// EQ frequency response display — draws the combined magnitude curve
// from the 10-band peaking filter settings.
class EQDisplay : public juce::Component
{
public:
    static constexpr int kBands = 10;
    static constexpr int kBandHz[kBands] = { 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
    static constexpr float kQ = 1.4f;
    static constexpr float kMaxGainDb = 12.0f;

    void setBandGain(int band, float dB)
    {
        if (band >= 0 && band < kBands)
        {
            gains[band] = dB;
            repaint();
        }
    }

    void setAllGains(const float* g, int count)
    {
        for (int i = 0; i < std::min(count, kBands); i++) gains[i] = g[i];
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float w = b.getWidth(), h = b.getHeight();

        // Background
        g.setColour(juce::Colour(0xff0d1520));
        g.fillRoundedRectangle(b, 5.0f);

        // Grid lines — 0dB, ±6dB, ±12dB
        g.setColour(juce::Colour(0xff2a3a5a));
        for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
        {
            const float y = dbToY(db, h);
            g.drawHorizontalLine(static_cast<int>(y), b.getX(), b.getRight());
        }
        // 0dB line slightly brighter
        g.setColour(juce::Colour(0xff3a5a7a));
        g.drawHorizontalLine(static_cast<int>(dbToY(0.0f, h)), b.getX(), b.getRight());

        // Magnitude response curve
        const int numPts = static_cast<int>(w);
        const double sr  = 48000.0; // display at 48kHz

        juce::Path curve;
        bool first = true;
        for (int px = 0; px < numPts; px++)
        {
            const float frac   = static_cast<float>(px) / w;
            const double freqHz = 20.0 * std::pow(20000.0 / 20.0, static_cast<double>(frac));
            double magSq = 1.0;
            for (int band = 0; band < kBands; band++)
            {
                if (std::abs(gains[band]) < 0.01f) continue;
                magSq *= peakingMagSq(freqHz, kBandHz[band], kQ, gains[band], sr);
            }
            const double magDb  = 10.0 * std::log10(std::max(1e-12, magSq));
            const float  y      = dbToY(static_cast<float>(std::clamp(magDb, -kMaxGainDb - 1.0, static_cast<double>(kMaxGainDb) + 1.0)), h);
            if (first) { curve.startNewSubPath(b.getX() + px, y); first = false; }
            else        curve.lineTo(b.getX() + px, y);
        }

        // Filled area under curve (above/below 0dB)
        juce::Path filled = curve;
        const float zero = dbToY(0.0f, h);
        filled.lineTo(b.getRight(), zero);
        filled.lineTo(b.getX(), zero);
        filled.closeSubPath();
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.10f));
        g.fillPath(filled);

        // Curve stroke
        g.setColour(juce::Colour(0xff00d4ff));
        g.strokePath(curve, juce::PathStrokeType(1.5f));

        // Frequency labels
        g.setColour(juce::Colour(0xff445566));
        g.setFont(juce::Font("Segoe UI", 9.0f, juce::Font::plain));
        for (int band = 0; band < kBands; band++)
        {
            const float fx = static_cast<float>(std::log10(kBandHz[band] / 20.0) / std::log10(20000.0 / 20.0)) * w;
            juce::String label;
            if      (kBandHz[band] < 1000)  label = juce::String(kBandHz[band]);
            else if (kBandHz[band] == 16000) label = "16k";
            else                             label = juce::String(kBandHz[band] / 1000) + "k";
            g.drawText(label, static_cast<int>(b.getX() + fx - 12), static_cast<int>(b.getBottom() - 12), 24, 10,
                       juce::Justification::centred);
        }

        // Border
        g.setColour(juce::Colour(0xff2a3a5a));
        g.drawRoundedRectangle(b, 5.0f, 1.0f);
    }

private:
    float gains[kBands] = {};

    static float dbToY(float db, float h) noexcept
    {
        return h * 0.5f - db / kMaxGainDb * h * 0.47f;
    }

    // Squared magnitude response of a peaking biquad at frequency freqHz
    static double peakingMagSq(double f, double fc, double Q, double gainDb, double sr) noexcept
    {
        const double A  = std::pow(10.0, gainDb / 40.0);
        const double w  = 2.0 * M_PI * f / sr;
        const double wc = 2.0 * M_PI * fc / sr;
        const double b0 = 1.0 + A * wc / Q;
        const double b1 = -2.0 * std::cos(wc);
        const double b2 = 1.0 - A * wc / Q;
        const double a0 = 1.0 + wc / (A * Q);
        const double a1 = -2.0 * std::cos(wc);
        const double a2 = 1.0 - wc / (A * Q);

        const double cosW = std::cos(w);
        const double sinW = std::sin(w);

        auto evalPoly = [&](double c0, double c1, double c2) -> std::complex<double> {
            return { c0 + c1 * cosW + c2 * (2*cosW*cosW - 1), (c1 + 2*c2*cosW) * sinW };
        };
        const auto num = evalPoly(b0 / a0, b1 / a0, b2 / a0);
        const auto den = evalPoly(1.0,     a1 / a0, a2 / a0);
        const double dR = den.real(), dI = den.imag();
        const double d2 = dR * dR + dI * dI;
        if (d2 < 1e-24) return 1.0;
        const double nR = num.real(), nI = num.imag();
        return (nR * nR + nI * nI) / d2;
    }
};
