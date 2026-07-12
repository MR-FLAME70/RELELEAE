#pragma once
#include "BiquadFilter.h"
#include "StereoWidth.h"
#include <cmath>
#include <algorithm>
#include <array>

// ─────────────────────────────────────────────────────────────────────────────
// Acoustic Engine — approximate reimplementation of the SBX Pro Studio /
// Creative Acoustic Engine effects found on the Sound Blaster X-Fi Surround
// 5.1 Pro (SB1095).  Port of createAcousticEngine() in offscreen.js.
//
// Chain (all in-place, all bypassable via the `enabled` flag):
//   Bass (lowshelf at crossoverHz, gain=bassAmt*12dB)
//   -> Crystalizer (highshelf at 7.5kHz + waveshaper saturation)
//   -> Dialog Plus (peaking at 2800Hz)
//   -> Smart Volume (feed-forward compressor)
//   -> Surround (M/S matrix decode to widen the stereo field)
// ─────────────────────────────────────────────────────────────────────────────
class AcousticEngine
{
public:
    struct Params
    {
        double surround    = 0.0;  // 0-100
        double crystalizer = 0.0;  // 0-100
        double bass        = 0.0;  // 0-100
        double crossover   = 100.0;// Hz  (20-500)
        double smartVolume = 0.0;  // 0-100
        double dialogPlus  = 0.0;  // 0-100
    };

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        bassFilter.prepare(sampleRate);
        crystalShelf.prepare(sampleRate);
        dialogFilter.prepare(sampleRate);

        // Compressor state
        compEnvL = compEnvR = 0.0;
        compGainL = compGainR = 1.0;

        update({});
    }

    void reset()
    {
        bassFilter.reset();
        crystalShelf.reset();
        dialogFilter.reset();
        compEnvL = compEnvR = compGainL = compGainR = 0.0;
        satPhaseL = satPhaseR = 0.0;
    }

    void update(const Params& p)
    {
        params = p;

        // Bass shelf
        const double bAmt = std::clamp(p.bass, 0.0, 100.0) / 100.0;
        const double bFreq = std::clamp(p.crossover, 20.0, 500.0);
        bassFilter.setLowShelf(bFreq, bAmt * 12.0);

        // Crystalizer high shelf
        const double cAmt = std::clamp(p.crystalizer, 0.0, 100.0) / 100.0;
        crystalShelf.setHighShelf(7500.0, cAmt * 9.0);
        crystalSatAmount = cAmt * 0.4;

        // Dialog Plus peaking
        const double dAmt = std::clamp(p.dialogPlus, 0.0, 100.0) / 100.0;
        dialogFilter.setPeaking(2800.0, 1.1, dAmt * 10.0);

        // Smart Volume compressor params
        const double sAmt  = std::clamp(p.smartVolume, 0.0, 100.0) / 100.0;
        compThreshDb = -6.0 - sAmt * 40.0;
        compRatio    = 1.0 + sAmt * 11.0;
        compMakeup   = 1.0 + sAmt * 1.2;

        // Surround matrix gains
        const double w    = (std::clamp(p.surround, 0.0, 100.0) / 100.0) * 1.5;
        surroundLL = 1.0 + 0.5 * w;
        surroundRR = 1.0 + 0.5 * w;
        surroundLR = -0.5 * w;   // cross-feed: L gets some -R
        surroundRL = -0.5 * w;   // cross-feed: R gets some -L
    }

    void processBlock(float* L, float* R, int numSamples) noexcept
    {
        const double attackCoeff  = std::exp(-1.0 / (sr * 0.01));
        const double releaseCoeff = std::exp(-1.0 / (sr * 0.3));

        for (int i = 0; i < numSamples; i++)
        {
            float l = L[i], r = R[i];

            // Bass shelf
            bassFilter.processStereo(l, r);

            // Crystalizer: highshelf boost + tanh soft saturation
            l = crystalShelf.processSampleL(l);
            r = crystalShelf.processSampleR(r);
            l = applySaturation(l, crystalSatAmount);
            r = applySaturation(r, crystalSatAmount);

            // Dialog Plus
            dialogFilter.processStereo(l, r);

            // Smart Volume (feed-forward compressor)
            {
                const double lvlL = 20.0 * std::log10(std::max(1e-6, std::abs(static_cast<double>(l))));
                const double lvlR = 20.0 * std::log10(std::max(1e-6, std::abs(static_cast<double>(r))));

                if (lvlL > compEnvL) compEnvL = attackCoeff  * compEnvL + (1.0 - attackCoeff)  * lvlL;
                else                 compEnvL = releaseCoeff * compEnvL + (1.0 - releaseCoeff) * lvlL;
                if (lvlR > compEnvR) compEnvR = attackCoeff  * compEnvR + (1.0 - attackCoeff)  * lvlR;
                else                 compEnvR = releaseCoeff * compEnvR + (1.0 - releaseCoeff) * lvlR;

                auto gainForEnv = [&](double env) -> float {
                    const double above = env - compThreshDb;
                    double gainDb = 0.0;
                    if (above > 3.0)                // hard knee at 6dB (simple threshold)
                        gainDb = compThreshDb + above / compRatio - env;
                    return static_cast<float>(std::pow(10.0, gainDb / 20.0) * compMakeup);
                };

                l *= gainForEnv(compEnvL);
                r *= gainForEnv(compEnvR);
            }

            // Surround matrix
            {
                const float nL = static_cast<float>(surroundLL * l + surroundLR * r);
                const float nR = static_cast<float>(surroundRL * l + surroundRR * r);
                l = nL; r = nR;
            }

            L[i] = l; R[i] = r;
        }
    }

private:
    double sr            = 44100.0;
    Params params;

    BiquadFilter bassFilter;
    BiquadFilter crystalShelf;
    BiquadFilter dialogFilter;
    double crystalSatAmount = 0.0;

    // Compressor state
    double compThreshDb = -6.0;
    double compRatio    = 1.0;
    double compMakeup   = 1.0;
    double compEnvL = 0.0, compEnvR = 0.0;
    double compGainL = 1.0, compGainR = 1.0;

    // Surround matrix gains
    double surroundLL = 1.0, surroundRR = 1.0;
    double surroundLR = 0.0, surroundRL = 0.0;

    // Saturation state (for the waveshaper's nonlinearity)
    double satPhaseL = 0.0, satPhaseR = 0.0;

    // Tanh saturation — makeSaturationCurve() equivalent:
    // amount 0 = straight line; higher = gentle tanh-like coloring
    static float applySaturation(float x, double amount) noexcept
    {
        if (amount < 0.001) return x;
        const double k = amount * 20.0;
        return static_cast<float>((1.0 + k) * x / (1.0 + k * std::abs(static_cast<double>(x))));
    }
};
