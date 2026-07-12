#pragma once
#include "BiquadFilter.h"
#include "DynamicBass.h"
#include "StereoWidth.h"
#include "PitchShifter.h"
#include <array>
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Advanced Audio Chain — port of createAdvancedAudioChain() in offscreen.js.
//
// Chain order (identical to the browser extension):
//   10-Band EQ -> Dynamic Bass -> Compressor -> Limiter -> Stereo Width -> Pitch
//
// Each module has its own enable flag. When disabled a module is bypassed.
// ─────────────────────────────────────────────────────────────────────────────

// ── 10-Band EQ ────────────────────────────────────────────────────────────────
class EqModule
{
public:
    static constexpr int kBands = 10;
    static constexpr int kBandHz[kBands] = { 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
    static constexpr float kQ = 1.4f;

    bool enabled = false;
    float bandGain[kBands] = {};  // dB, -12..+12

    void prepare(double sampleRate)
    {
        for (int b = 0; b < kBands; b++)
        {
            filters[b].prepare(sampleRate);
            filters[b].setPeaking(kBandHz[b], kQ, 0.0);
        }
    }

    void reset() { for (auto& f : filters) f.reset(); }

    void setGain(int band, float dB) noexcept
    {
        if (band < 0 || band >= kBands) return;
        bandGain[band] = std::clamp(dB, -12.0f, 12.0f);
        // Update filter coefficient (needs prepared sampleRate — filters cache it)
        filters[band].setPeaking(kBandHz[band], kQ, bandGain[band]);
    }

    void setBands(const float* gains, int count) noexcept
    {
        for (int b = 0; b < std::min(count, kBands); b++) setGain(b, gains[b]);
    }

    void processBlock(float* L, float* R, int numSamples) noexcept
    {
        if (!enabled) return;
        for (int b = 0; b < kBands; b++)
            for (int i = 0; i < numSamples; i++)
                filters[b].processStereo(L[i], R[i]);
    }

private:
    std::array<BiquadFilter, kBands> filters;
};

// ── Compressor ────────────────────────────────────────────────────────────────
class CompressorModule
{
public:
    bool   enabled   = false;
    double threshold = -24.0;  // dBFS
    double ratio     = 4.0;
    double attack    = 0.003;  // seconds
    double release   = 0.250;  // seconds
    double makeupDb  = 0.0;    // dB

    void prepare(double sampleRate) { sr = sampleRate; envL = envR = 0.0; }
    void reset()                    { envL = envR = 0.0; }

    void processBlock(float* L, float* R, int numSamples) noexcept
    {
        if (!enabled) return;
        const double attC = std::exp(-1.0 / (sr * attack));
        const double relC = std::exp(-1.0 / (sr * release));
        const double mkup = std::pow(10.0, makeupDb / 20.0);
        constexpr double kKnee = 6.0;

        for (int i = 0; i < numSamples; i++)
        {
            auto compress = [&](float& x, double& env) {
                const double lvl = 20.0 * std::log10(std::max(1e-9, std::abs(static_cast<double>(x))));
                if (lvl > env) env = attC * env + (1.0 - attC) * lvl;
                else           env = relC * env + (1.0 - relC) * lvl;

                double gainDb = 0.0;
                const double above = env - threshold;
                if (above > kKnee / 2.0)
                    gainDb = threshold + above / ratio - env;
                else if (above > -kKnee / 2.0)
                    gainDb = -std::pow(above + kKnee / 2.0, 2.0) / (2.0 * kKnee * (1.0 - 1.0 / ratio));

                x = static_cast<float>(static_cast<double>(x) * std::pow(10.0, gainDb / 20.0) * mkup);
            };
            compress(L[i], envL);
            compress(R[i], envR);
        }
    }

private:
    double sr = 44100.0;
    double envL = 0.0, envR = 0.0;
};

// ── Limiter ───────────────────────────────────────────────────────────────────
// High-ratio compressor (ratio=20, near-zero knee, fast attack).
class LimiterModule
{
public:
    bool   enabled   = false;
    double threshold = -3.0;   // dBFS
    double release   = 0.050;  // seconds

    void prepare(double sampleRate) { sr = sampleRate; envL = envR = 0.0; }
    void reset()                    { envL = envR = 0.0; }

    void processBlock(float* L, float* R, int numSamples) noexcept
    {
        if (!enabled) return;
        constexpr double kRatio  = 20.0;
        constexpr double kAttack = 0.001;
        const double attC = std::exp(-1.0 / (sr * kAttack));
        const double relC = std::exp(-1.0 / (sr * release));

        for (int i = 0; i < numSamples; i++)
        {
            auto limit = [&](float& x, double& env) {
                const double lvl = 20.0 * std::log10(std::max(1e-9, std::abs(static_cast<double>(x))));
                if (lvl > env) env = attC * env + (1.0 - attC) * lvl;
                else           env = relC * env + (1.0 - relC) * lvl;

                double gainDb = 0.0;
                const double above = env - threshold;
                if (above > 0.0) gainDb = threshold + above / kRatio - env;
                x = static_cast<float>(static_cast<double>(x) * std::pow(10.0, gainDb / 20.0));
            };
            limit(L[i], envL);
            limit(R[i], envR);
        }
    }

private:
    double sr = 44100.0;
    double envL = 0.0, envR = 0.0;
};

// ── Full Advanced Audio Chain ─────────────────────────────────────────────────
class AdvancedAudioChain
{
public:
    EqModule       eq;
    DynamicBass    dynBass;
    CompressorModule comp;
    LimiterModule  lim;
    StereoWidth    width;
    PitchShifter   pitch;

    bool dynBassEnabled   = false;
    bool stereoWidthEnabled = false;
    bool pitchEnabled     = false;

    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        maxBlock = maxBlockSize;

        eq.prepare(sampleRate);
        dynBass.prepare(sampleRate, maxBlockSize);
        comp.prepare(sampleRate);
        lim.prepare(sampleRate);
        pitch.prepare(sampleRate, 2);

        tmpL.resize(maxBlockSize);
        tmpR.resize(maxBlockSize);
        pitchOutL.resize(maxBlockSize);
        pitchOutR.resize(maxBlockSize);
    }

    void reset()
    {
        eq.reset(); dynBass.reset(); comp.reset(); lim.reset(); pitch.reset();
    }

    void processBlock(float* L, float* R, int numSamples) noexcept
    {
        // EQ
        eq.processBlock(L, R, numSamples);

        // Dynamic Bass (always processes if enabled — adds to signal)
        if (dynBassEnabled)
            dynBass.processBlock(L, R, numSamples);

        // Compressor
        comp.processBlock(L, R, numSamples);

        // Limiter
        lim.processBlock(L, R, numSamples);

        // Stereo Width
        if (stereoWidthEnabled)
            for (int i = 0; i < numSamples; i++)
                width.processStereo(L[i], R[i]);

        // Pitch Shifter
        if (pitchEnabled)
        {
            const float* inChs[2]  = { L, R };
            float*       outChs[2] = { pitchOutL.data(), pitchOutR.data() };
            pitch.processBlock(inChs, outChs, 2, numSamples);
            for (int i = 0; i < numSamples; i++) { L[i] = pitchOutL[i]; R[i] = pitchOutR[i]; }
        }
    }

private:
    double sr      = 44100.0;
    int    maxBlock = 512;

    std::vector<float> tmpL, tmpR, pitchOutL, pitchOutR;
};
