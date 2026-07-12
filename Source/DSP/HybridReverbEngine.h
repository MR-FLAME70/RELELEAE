#pragma once
#include "FDNReverb.h"
#include "BiquadFilter.h"
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Hybrid reverb engine — port of reverb-engine.js.
//
// Signal flow:
//   input -> pre-delay -+-> [Early Reflections (synthesized cave IR)] --+-> wet bus
//                       +-> [FDN late tail (FDNReverb)]               --+
//                                                                         |
//   wet bus -> HF-damp shelf -> LF-damp shelf -> high-cut -> low-cut      |
//           -> M/S stereo width -> wetOut                                 |
//   dry path (input) -> dryOut                                            |
//   output = wetOut * wetLevel + dryOut * dryLevel                        |
//
// In addition, an optional resonance peaking filter sits on the wet path
// (used by Haunted Cavern V1 for the "clangy cavern ring").
//
// The Early Reflections are a synthesized cave-style IR: 7 sparse, irregularly-
// timed band-limited taps per channel, decorrelated between L and R, with a
// short fade at the tail edge. Ported from makeCaveEarlyReflectionIR().
// ─────────────────────────────────────────────────────────────────────────────
class HybridReverbEngine
{
public:
    // Parameters applied via setParams()
    struct Params
    {
        double preDelay             = 0.015;  // seconds
        double earlyReflDelay       = 0.0;    // seconds (offsets ER onset)
        double earlyReflLevel       = 0.35;   // 0-1
        double lateReverbLevel      = 1.0;    // 0-1+
        double decayTime            = 1.8;    // seconds
        double diffusion            = 0.7;    // 0-1
        double density              = 0.5;    // 0-1
        double hfDamping            = 0.0;    // 0-1 (extra shelf on wet bus)
        double lfDamping            = 0.0;    // 0-1
        double highCut              = 9000.0; // Hz
        double lowCut               = 80.0;   // Hz
        double stereoWidth          = 1.0;    // 0-2 (1=unchanged)
        double roomSize             = 1.0;    // 0.25-3
        double modulationDepth      = 0.4;
        double modulationRate       = 0.35;
        double wetLevel             = 1.0;    // engine-internal wet/dry
        double dryLevel             = 0.0;
        double reverbMixWet         = 0.0;    // outer dry/wet bus wet gain
        double reverbMixDry         = 1.0;    // outer dry/wet bus dry gain
        double resonanceHz          = 1000.0;
        double resonanceQ           = 0.0;    // 0 = off
    };

    HybridReverbEngine() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        sr           = sampleRate;
        maxBlock     = maxBlockSize;

        // Pre-delay buffer (up to 2s)
        preDelayBuf.assign(static_cast<int>(sampleRate * 2.1) + 4, 0.0f);
        preDelayWriteL = preDelayWriteR = 0;
        preDelaySamples = 0;

        // ER delay buffer (up to 0.5s)
        erDelayBufL.assign(static_cast<int>(sampleRate * 0.55) + 4, 0.0f);
        erDelayBufR.assign(static_cast<int>(sampleRate * 0.55) + 4, 0.0f);
        erDelayWriteL = erDelayWriteR = 0;
        erDelaySamples = 0;

        // ER convolution IR buffers
        erIrL.clear(); erIrR.clear();
        erConvPosL = erConvPosR = 0;
        erBufL.assign(1, 0.0f); erBufR.assign(1, 0.0f);

        // FDN late tail
        fdn.prepare(sampleRate, maxBlockSize);

        // Wet-bus filters
        hfDampShelf.prepare(sampleRate);
        lfDampShelf.prepare(sampleRate);
        highCutFilter.prepare(sampleRate);
        lowCutFilter.prepare(sampleRate);
        resonanceFilter.prepare(sampleRate);

        // M/S width
        stereoWidth.setWidth(100.0);

        applyParams(current);

        // Build initial ER IR
        buildEarlyReflectionIR(current.roomSize, current.diffusion);
    }

    void reset()
    {
        std::fill(preDelayBuf.begin(), preDelayBuf.end(), 0.0f);
        std::fill(erDelayBufL.begin(), erDelayBufL.end(), 0.0f);
        std::fill(erDelayBufR.begin(), erDelayBufR.end(), 0.0f);
        std::fill(erBufL.begin(), erBufL.end(), 0.0f);
        std::fill(erBufR.begin(), erBufR.end(), 0.0f);
        fdn.reset();
        hfDampShelf.reset(); lfDampShelf.reset();
        highCutFilter.reset(); lowCutFilter.reset();
        resonanceFilter.reset();
    }

    void setParams(const Params& p)
    {
        const bool needRebuildIR =
            (std::abs(p.roomSize  - current.roomSize)  > 0.001 ||
             std::abs(p.diffusion - current.diffusion) > 0.001);

        current = p;
        applyParams(p);

        if (needRebuildIR)
            buildEarlyReflectionIR(p.roomSize, p.diffusion);
    }

    // Convenience: set the outer wet/dry mix (Effects Amount + Song Volume)
    void setMix(bool on, double mixPercent, double amountPercent, double songVolumePercent)
    {
        const double amount  = std::clamp(amountPercent, 0.0, 100.0) / 100.0;
        const double wet     = on ? std::clamp(mixPercent, 0.0, 100.0) / 100.0 * amount : 0.0;
        const double dry     = std::max(0.0, songVolumePercent / 100.0);
        current.reverbMixWet = wet;
        current.reverbMixDry = dry;
    }

    // Process one stereo sample
    void processStereo(float xIn, float yIn, float& outL, float& outR) noexcept
    {
        // ── Pre-delay ────────────────────────────────────────────────────────
        const int pdBufLen = static_cast<int>(preDelayBuf.size());
        preDelayBuf[preDelayWriteL] = xIn;
        preDelayBuf[preDelayWriteR] = yIn;   // simple mono pre-delay (same buffer)

        const int pdSamples = static_cast<int>(std::round(current.preDelay * sr));
        const int rdIdxL    = ((preDelayWriteL - pdSamples) % pdBufLen + pdBufLen) % pdBufLen;
        const int rdIdxR    = ((preDelayWriteR - pdSamples) % pdBufLen + pdBufLen) % pdBufLen;
        const float pdOutL  = preDelayBuf[rdIdxL];
        const float pdOutR  = preDelayBuf[rdIdxR];
        preDelayWriteL = (preDelayWriteL + 1) % pdBufLen;
        preDelayWriteR = preDelayWriteL; // share write position

        // ── Early Reflections (ER delay + convolution) ───────────────────────
        const int erBufLen = static_cast<int>(erDelayBufL.size());
        erDelayBufL[erDelayWriteL] = pdOutL;
        erDelayBufR[erDelayWriteR] = pdOutR;

        const int erSmps = static_cast<int>(std::round(current.earlyReflDelay * sr));
        const int erRdL  = ((erDelayWriteL - erSmps) % erBufLen + erBufLen) % erBufLen;
        const int erRdR  = ((erDelayWriteR - erSmps) % erBufLen + erBufLen) % erBufLen;
        const float erInL = erDelayBufL[erRdL];
        const float erInR = erDelayBufR[erRdR];
        erDelayWriteL = (erDelayWriteL + 1) % erBufLen;
        erDelayWriteR = (erDelayWriteR + 1) % erBufLen;

        // Convolve with synthesized cave IR
        float erOutL = 0.0f, erOutR = 0.0f;
        if (!erIrL.empty())
        {
            const int irLen = static_cast<int>(erIrL.size());
            // Push new sample into circular ER input buffer
            erBufL[erConvPosL] = erInL;
            erBufR[erConvPosR] = erInR;
            // Direct convolution (short IR, ≤~14000 samples at 48kHz*0.28s)
            for (int k = 0; k < irLen; k++)
            {
                const int idx = (erConvPosL - k + static_cast<int>(erBufL.size())) % static_cast<int>(erBufL.size());
                erOutL += erIrL[k] * erBufL[idx];
                erOutR += erIrR[k] * erBufR[idx];
            }
            erConvPosL = (erConvPosL + 1) % static_cast<int>(erBufL.size());
            erConvPosR = erConvPosL;
        }

        erOutL *= static_cast<float>(current.earlyReflLevel);
        erOutR *= static_cast<float>(current.earlyReflLevel);

        // ── FDN Late Tail ─────────────────────────────────────────────────────
        float fdnOutL = 0.0f, fdnOutR = 0.0f;
        fdn.processStereo(pdOutL, pdOutR, fdnOutL, fdnOutR);
        fdnOutL *= static_cast<float>(current.lateReverbLevel);
        fdnOutR *= static_cast<float>(current.lateReverbLevel);

        // ── Wet bus: ER + Late ────────────────────────────────────────────────
        float wetL = erOutL + fdnOutL;
        float wetR = erOutR + fdnOutR;

        // HF/LF damping shelves
        hfDampShelf.processStereo(wetL, wetR);
        lfDampShelf.processStereo(wetL, wetR);

        // High-cut / low-cut
        highCutFilter.processStereo(wetL, wetR);
        lowCutFilter.processStereo(wetL, wetR);

        // Resonance peaking filter (Haunted Cavern V1)
        if (current.resonanceQ > 0.001)
            resonanceFilter.processStereo(wetL, wetR);

        // M/S stereo width
        stereoWidth.processStereo(wetL, wetR);

        // Engine-internal wet/dry mix
        const float wetLvl = static_cast<float>(current.wetLevel);
        const float dryLvl = static_cast<float>(current.dryLevel);
        wetL = wetL * wetLvl + pdOutL * dryLvl;
        wetR = wetR * wetLvl + pdOutR * dryLvl;

        // ── Outer mix (Effects Amount / Song Volume) ──────────────────────────
        const float mixWet = static_cast<float>(current.reverbMixWet);
        const float mixDry = static_cast<float>(current.reverbMixDry);
        outL = xIn * mixDry + wetL * mixWet;
        outR = yIn * mixDry + wetR * mixWet;
    }

    void processBlock(const float* inL, const float* inR,
                      float* outL, float* outR, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; i++)
            processStereo(inL[i], inR[i], outL[i], outR[i]);
    }

private:
    // ── Parameters ────────────────────────────────────────────────────────────
    Params current;
    double sr         = 44100.0;
    int    maxBlock   = 512;

    // ── Pre-delay ─────────────────────────────────────────────────────────────
    std::vector<float> preDelayBuf;
    int preDelayWriteL = 0, preDelayWriteR = 0;
    int preDelaySamples = 0;

    // ── ER delay ──────────────────────────────────────────────────────────────
    std::vector<float> erDelayBufL, erDelayBufR;
    int erDelayWriteL = 0, erDelayWriteR = 0;
    int erDelaySamples = 0;

    // ── ER convolution ────────────────────────────────────────────────────────
    std::vector<float> erIrL, erIrR;
    std::vector<float> erBufL, erBufR;
    int erConvPosL = 0, erConvPosR = 0;

    // ── FDN ───────────────────────────────────────────────────────────────────
    FDNReverb fdn;

    // ── Wet-bus filters ───────────────────────────────────────────────────────
    BiquadFilter hfDampShelf, lfDampShelf;
    BiquadFilter highCutFilter, lowCutFilter;
    BiquadFilter resonanceFilter;

    // ── M/S width ─────────────────────────────────────────────────────────────
    struct MSWidth {
        double llGain = 1, lrGain = 0, rlGain = 0, rrGain = 1;
        void setWidth(double widthPercent) noexcept {
            const double f = std::clamp(widthPercent, 0.0, 200.0) / 100.0;
            llGain = 0.5 + 0.5 * f;  rrGain = llGain;
            lrGain = 0.5 - 0.5 * f;  rlGain = lrGain;
        }
        void processStereo(float& L, float& R) const noexcept {
            const float nL = static_cast<float>(llGain * L + lrGain * R);
            const float nR = static_cast<float>(rlGain * L + rrGain * R);
            L = nL; R = nR;
        }
    } stereoWidth;

    // ── Apply params to DSP nodes ─────────────────────────────────────────────
    void applyParams(const Params& p) noexcept
    {
        // FDN parameters
        fdn.setRoomSize(p.roomSize);
        fdn.setDecayTime(p.decayTime);
        fdn.setDiffusion(p.diffusion);
        fdn.setDensity(p.density);
        fdn.setHfDamping(p.hfDamping * 0.5);  // FDN has its own internal scale
        fdn.setLfDamping(p.lfDamping * 0.5);
        fdn.setModDepth(p.modulationDepth);
        fdn.setModRate(p.modulationRate);

        // HF damping on wet bus: 0 → 0dB shelf; 1 → -12dB shelf at 6kHz
        const double hfGainDb = -p.hfDamping * 12.0;
        hfDampShelf.setHighShelf(6000.0, hfGainDb);

        // LF damping: 0 → 0dB; 1 → -6dB at 200Hz
        const double lfGainDb = -p.lfDamping * 6.0;
        lfDampShelf.setLowShelf(200.0, lfGainDb);

        const double hc = std::clamp(p.highCut, 200.0, 20000.0);
        highCutFilter.setLowPass(hc, 0.707);

        const double lc = std::clamp(p.lowCut, 20.0, 2000.0);
        lowCutFilter.setHighPass(lc, 0.707);

        stereoWidth.setWidth(p.stereoWidth * 100.0);

        // Resonance peaking filter
        if (p.resonanceQ > 0.001)
            resonanceFilter.setPeaking(p.resonanceHz, p.resonanceQ, 8.0);
        else
            resonanceFilter.setBypass();
    }

    // ── Synthesize cave early-reflection IR ───────────────────────────────────
    // Port of makeCaveEarlyReflectionIR() from reverb-engine.js.
    // Uses a deterministic PRNG so the IR is identical every time for the
    // same (roomSize, diffusion) pair — matching the offline/live parity of
    // the browser extension.
    void buildEarlyReflectionIR(double roomSize, double diffusion)
    {
        const double durationSec = std::min(0.28, 0.09 + roomSize * 0.06);
        const int irLen = std::max(1, static_cast<int>(std::floor(sr * durationSec)));

        erIrL.assign(irLen, 0.0f);
        erIrR.assign(irLen, 0.0f);

        constexpr int kTaps = 7;
        const double spread = 0.05 + (1.0 - diffusion) * 0.15;

        for (int ch = 0; ch < 2; ch++)
        {
            auto& data   = (ch == 0) ? erIrL : erIrR;
            const double chSeed = (ch == 0) ? 0.37 : 0.71;

            // Deterministic PRNG (mulberry32 variant from reverb-engine.js)
            uint32_t rngState = static_cast<uint32_t>(
                (static_cast<uint32_t>(roomSize * 100003)) ^
                (static_cast<uint32_t>(diffusion * 65537)) ^
                (static_cast<uint32_t>(ch) * 0x9e3779b9u));

            auto rng = [&]() -> double {
                rngState |= 0;
                rngState = (rngState + 0x6d2b79f5u);
                uint32_t t = (rngState ^ (rngState >> 15)) * (1u | rngState);
                t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
                return static_cast<double>((t ^ (t >> 14)) & 0xFFFFFFFFu) / 4294967296.0;
            };

            for (int k = 0; k < kTaps; k++)
            {
                const double frac    = std::fmod((k + 1) * 0.618 + chSeed, 1.0);
                const double tapTime = 0.006 + frac * spread + k * (spread / (kTaps * 2.2));
                const int idx        = static_cast<int>(std::floor(tapTime * sr));
                if (idx >= irLen) continue;

                const double amp     = std::pow(0.72, k) * (0.85 + 0.3 * std::fmod(k * chSeed, 1.0));
                const int burstLen   = std::max(1, static_cast<int>(std::round(sr * 0.001)));
                double lp = 0.0;
                for (int b = 0; b < burstLen && idx + b < irLen; b++)
                {
                    const double white = rng() * 2.0 - 1.0;
                    lp += 0.5 * (white - lp);
                    data[idx + b] += static_cast<float>(lp * amp * (1.0 - static_cast<double>(b) / burstLen));
                }
            }

            // Fade envelope
            for (int i = 0; i < irLen; i++)
            {
                const double t = static_cast<double>(i) / sr;
                data[i] *= static_cast<float>(std::pow(10.0, (-1.2 * t) / durationSec));
            }
        }

        // Normalise peak
        constexpr double kTargetPeak = 0.9;
        float peak = 0.0f;
        for (float v : erIrL) peak = std::max(peak, std::abs(v));
        for (float v : erIrR) peak = std::max(peak, std::abs(v));
        if (peak > kTargetPeak)
        {
            const float scale = static_cast<float>(kTargetPeak) / peak;
            for (float& v : erIrL) v *= scale;
            for (float& v : erIrR) v *= scale;
        }

        // Circular ER input buffer (same length as IR)
        erBufL.assign(irLen, 0.0f);
        erBufR.assign(irLen, 0.0f);
        erConvPosL = erConvPosR = 0;
    }
};
