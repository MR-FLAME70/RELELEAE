#pragma once
#include "BiquadFilter.h"
#include <cmath>
#include <algorithm>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Speaker Configuration Engine — port of createSpeakerConfigEngine() +
// SPEAKER_MODES from offscreen.js.
//
// Headphones: full HRTF-style simulation via simple ICTD/IIDC convolution
//   approximation (left channel: slight +delay; right channel: slight -delay).
// Stereo (2.0): straight passthrough, no processing.
// 2.1: passthrough + low-cut mono LFE summed back in.
// 4.0: passthrough + L-R derived rear channels.
// 4.1: 4.0 + LFE.
// 5.1: passthrough + center + rear + LFE.
// 7.1: 5.1 + side channels.
//
// All derived channels are band-limited (highpass 100 Hz / lowpass 7 kHz),
// delayed (rear: 18-21ms; side: 9-12ms), and folded back into the stereo
// output via equal-power panning — a virtual decode, not upmix.
// ─────────────────────────────────────────────────────────────────────────────
class SpeakerConfigEngine
{
public:
    enum class Mode { Headphones, Stereo, M21, M40, M41, M51, M71 };

    struct Layout
    {
        double frontWidth     = 1.0;
        double rearWidth      = 1.0;
        double centerDistance = 1.0;
        double rearDistance   = 1.0;
        double subDistanceFt  = 0.0;
        double levelFL = 1.0, levelFR = 1.0;
        double levelC  = 1.0, levelSub = 1.0;
        double levelRL = 1.0, levelRR  = 1.0;
    };

    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;

        // Delay buffers: rear 21ms max, LFE 250ms, side 12ms, HP 0.5ms
        const int maxDelaySmp = static_cast<int>(std::ceil(sr * 0.26));
        for (auto& b : delayBufL) b.assign(maxDelaySmp, 0.0f);
        for (auto& b : delayBufR) b.assign(maxDelaySmp, 0.0f);
        for (auto& w : writePos) w = 0;

        // Derived channel filters: HP 100Hz, LP 7kHz (band-limit the diff signal)
        diffHP.prepare(sampleRate); diffHP.setHighPass(100.0, 0.707);
        diffLP.prepare(sampleRate); diffLP.setLowPass(7000.0, 0.707);
        // LFE lowpass 120Hz
        lfeLP.prepare(sampleRate);  lfeLP.setLowPass(120.0, 0.707);
        // Headphones ILD filter: slight high-boost to simulate pinna coloration
        hpFilterL.prepare(sampleRate); hpFilterL.setHighShelf(3000.0, 2.0);
        hpFilterR.prepare(sampleRate); hpFilterR.setHighShelf(3000.0, 2.0);

        setMode(Mode::Stereo);
    }

    void reset()
    {
        for (auto& b : delayBufL) std::fill(b.begin(), b.end(), 0.0f);
        for (auto& b : delayBufR) std::fill(b.begin(), b.end(), 0.0f);
        diffHP.reset(); diffLP.reset(); lfeLP.reset();
        hpFilterL.reset(); hpFilterR.reset();
    }

    void setMode(Mode m) noexcept    { mode = m; }
    void setLayout(const Layout& l) noexcept { layout = l; }

    static Mode modeFromString(const std::string& s)
    {
        if (s == "headphones") return Mode::Headphones;
        if (s == "2.1")        return Mode::M21;
        if (s == "4.0")        return Mode::M40;
        if (s == "4.1")        return Mode::M41;
        if (s == "5.1")        return Mode::M51;
        if (s == "7.1")        return Mode::M71;
        return Mode::Stereo;
    }

    void processBlock(float* L, float* R, int numSamples) noexcept
    {
        if (mode == Mode::Stereo)
        {
            // Straight passthrough with output-level trim
            for (int i = 0; i < numSamples; i++)
            {
                L[i] *= static_cast<float>(layout.levelFL);
                R[i] *= static_cast<float>(layout.levelFR);
            }
            return;
        }

        for (int i = 0; i < numSamples; i++)
        {
            float l = L[i] * static_cast<float>(layout.levelFL);
            float r = R[i] * static_cast<float>(layout.levelFR);

            // ── Headphones: ILD/ITD simulation ────────────────────────────
            if (mode == Mode::Headphones)
            {
                // Tiny ITD (~0.3ms) for front staging
                float dl = readDelay(0, 14);   // ~0.3ms @ 44.1kHz
                float dr = readDelay(1, 0);
                writeDelay(0, l); writeDelay(1, r);

                // Pinna high-boost for front image
                dl = hpFilterL.processSampleL(dl);
                dr = hpFilterR.processSampleR(dr);

                // Center phantom (mono sum, reduced level)
                const float ctr = (dl + dr) * 0.35f * static_cast<float>(layout.levelC);

                // Rear: L-R diff → delayed, folded to opposite side
                float diff = dl - dr;
                diff = diffHP.processSampleL(diff);
                diff = diffLP.processSampleL(diff);
                const float rearDelay = readDelay(2, static_cast<int>(sr * 0.018));
                writeDelay(2, diff);
                // LFE
                float lfe = lfeLP.processSampleL((dl + dr) * 0.5f);
                const float lfeDelay = readDelay(4, lfeDelaySamples());
                writeDelay(4, lfe);
                lfe = lfeDelay * 1.3f * static_cast<float>(layout.levelSub);

                L[i] = dl + ctr - rearDelay * 0.4f * static_cast<float>(layout.levelRL) + lfe;
                R[i] = dr + ctr + rearDelay * 0.4f * static_cast<float>(layout.levelRR) + lfe;
                continue;
            }

            // ── Derived channel computation ────────────────────────────────
            float outL = l, outR = r;

            // Derive: L-R difference, band-limited
            float diffIn = l - r;
            diffIn = diffHP.processSampleL(diffIn);
            diffIn = diffLP.processSampleL(diffIn);

            // Center: mono sum (5.1 / 7.1)
            if (mode == Mode::M51 || mode == Mode::M71)
            {
                const float ctr = (l + r) * 0.5f * static_cast<float>(layout.levelC);
                outL += ctr; outR += ctr;
            }

            // Rear channels (4.0 / 4.1 / 5.1 / 7.1)
            if (mode == Mode::M40 || mode == Mode::M41 || mode == Mode::M51 || mode == Mode::M71)
            {
                const int dlyL = static_cast<int>(sr * 0.018 * layout.rearDistance);
                const int dlyR = static_cast<int>(sr * 0.021 * layout.rearDistance);
                float rearL = readDelay(2, std::max(1, dlyL));
                float rearR = readDelay(3, std::max(1, dlyR));
                writeDelay(2, diffIn); writeDelay(3, diffIn);
                const float rwL = static_cast<float>(layout.rearWidth);
                const float rlL = static_cast<float>(layout.levelRL);
                const float rlR = static_cast<float>(layout.levelRR);
                // Equal-power fold: rear-left adds to left, rear-right to right
                outL += rearL * 0.7f * rwL * rlL;
                outR += rearR * 0.7f * rwL * rlR;
            }

            // Side channels (7.1 only)
            if (mode == Mode::M71)
            {
                const int dlyLS = static_cast<int>(sr * 0.009 * layout.rearDistance);
                const int dlyRS = static_cast<int>(sr * 0.012 * layout.rearDistance);
                float sideL = readDelay(5, std::max(1, dlyLS));
                float sideR = readDelay(6, std::max(1, dlyRS));
                writeDelay(5, diffIn); writeDelay(6, diffIn);
                const float rwS = static_cast<float>(layout.rearWidth * 1.45);
                outL += sideL * 0.5f * rwS * static_cast<float>(layout.levelRL);
                outR += sideR * 0.5f * rwS * static_cast<float>(layout.levelRR);
            }

            // LFE (.1 layouts)
            if (mode == Mode::M21 || mode == Mode::M41 || mode == Mode::M51 || mode == Mode::M71)
            {
                float lfeIn = (l + r) * 0.5f;
                lfeIn = lfeLP.processSampleL(lfeIn);
                const int lfeD = lfeDelaySamples();
                float lfe = readDelay(4, lfeD);
                writeDelay(4, lfeIn);
                float lfeGain = 1.0f;
                if (mode == Mode::M21)        lfeGain = 1.00f;
                else if (mode == Mode::M41)   lfeGain = 1.00f;
                else if (mode == Mode::M51)   lfeGain = 1.15f;
                else if (mode == Mode::M71)   lfeGain = 1.20f;
                lfe *= lfeGain * static_cast<float>(layout.levelSub);
                outL += lfe; outR += lfe;
            }

            L[i] = outL; R[i] = outR;
        }
    }

private:
    double sr    = 44100.0;
    Mode   mode  = Mode::Stereo;
    Layout layout;

    // Delay buffers: indices 0-1 HP, 2-3 rear, 4 LFE, 5-6 side
    static constexpr int kNumDelays = 7;
    std::vector<float> delayBufL[kNumDelays], delayBufR[kNumDelays];
    int writePos[kNumDelays] = {};

    BiquadFilter diffHP, diffLP, lfeLP;
    BiquadFilter hpFilterL, hpFilterR;

    int lfeDelaySamples() const noexcept
    {
        const double distMeters = std::max(0.0, layout.subDistanceFt) * 0.3048;
        return static_cast<int>(std::min(sr * 0.24, distMeters / 343.0 * sr));
    }

    float readDelay(int idx, int delaySamples) noexcept
    {
        const int len = static_cast<int>(delayBufL[idx].size());
        if (len == 0) return 0.0f;
        delaySamples = std::min(delaySamples, len - 1);
        const int rp = ((writePos[idx] - delaySamples) % len + len) % len;
        return delayBufL[idx][rp];
    }

    void writeDelay(int idx, float val) noexcept
    {
        const int len = static_cast<int>(delayBufL[idx].size());
        if (len == 0) return;
        delayBufL[idx][writePos[idx]] = val;
        writePos[idx] = (writePos[idx] + 1) % len;
    }
};
