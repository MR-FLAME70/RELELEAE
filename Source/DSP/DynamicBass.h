#pragma once
#include "BiquadFilter.h"
#include <cmath>
#include <algorithm>

// Dynamic Bass enhancer — envelope-driven additive low-end boost.
// Port of createDynamicBassModule() from offscreen.js.
//
// Signal flow:
//   input --> [lowpass 150Hz] --> [compressor] --> [makeup gain] --(+)--> output
//         |------------------------------------------------------------> output (always dry)
//
// The compressed/boosted low band is added on top of the untouched dry signal.
// Sensitivity lowers the compressor threshold (engages on quieter bass content).
// Strength scales how much of the enhanced low band is mixed back in.
class DynamicBass
{
public:
    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        lowpass.prepare(sampleRate);
        lowpass.setLowPass(150.0);

        // Compressor state
        envL = envR = 0.0;
        reset();
    }

    void reset()
    {
        lowpass.reset();
        envL = envR = 0.0;
    }

    void setSensitivity(double s) noexcept   // 0–100
    {
        const double norm = std::clamp(s, 0.0, 100.0) / 100.0;
        threshold = -10.0 - norm * 40.0; // -10 .. -50 dBFS
    }

    void setStrength(double s) noexcept      // 0–100
    {
        const double norm = std::clamp(s, 0.0, 100.0) / 100.0;
        makeupGain = static_cast<float>(norm * 4.0); // 0 .. 4x
    }

    // Processes one stereo frame, adding the processed low band to L/R in-place.
    void processStereo(float& L, float& R) noexcept
    {
        // Lowpass to extract bass band
        float bassL = lowpass.processSampleL(L);
        float bassR = lowpass.processSampleR(R);

        // Simple feed-forward RMS compressor with ratio=6, knee=10dB
        compressSample(bassL, bassR);

        L += bassL * makeupGain;
        R += bassR * makeupGain;
    }

    void processBlock(float* L, float* R, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; i++)
            processStereo(L[i], R[i]);
    }

private:
    static constexpr double kRatio   = 6.0;
    static constexpr double kKneeDb  = 10.0;
    static constexpr double kAttack  = 0.01;   // seconds
    static constexpr double kRelease = 0.25;   // seconds

    double sr           = 44100.0;
    double threshold    = -30.0;   // dBFS
    float  makeupGain   = 0.0f;
    double envL = 0.0, envR = 0.0;

    BiquadFilter lowpass;

    // Inline feed-forward compressor applied to bassL/bassR
    void compressSample(float& bassL, float& bassR) noexcept
    {
        const double attackCoeff  = std::exp(-1.0 / (sr * kAttack));
        const double releaseCoeff = std::exp(-1.0 / (sr * kRelease));

        auto compress = [&](float& x, double& env) {
            const double levelDb = 20.0 * std::log10(std::max(1e-6, std::abs(static_cast<double>(x))));
            // Envelope tracking
            if (levelDb > env) env = attackCoeff  * env + (1.0 - attackCoeff)  * levelDb;
            else               env = releaseCoeff * env + (1.0 - releaseCoeff) * levelDb;

            // Gain computation with soft knee
            double gainDb = 0.0;
            const double above = env - threshold;
            if (above > kKneeDb / 2.0)
                gainDb = threshold + above / kRatio - env;
            else if (above > -kKneeDb / 2.0)
                gainDb = -std::pow(above + kKneeDb / 2.0, 2.0) / (2.0 * kKneeDb * (1.0 - 1.0 / kRatio));

            x = static_cast<float>(static_cast<double>(x) * std::pow(10.0, gainDb / 20.0));
        };

        compress(bassL, envL);
        compress(bassR, envR);
    }
};
