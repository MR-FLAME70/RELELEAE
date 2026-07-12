#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Granular overlap-add pitch shifter.
// Port of pitch-shift-worklet.js.
//
// Two read grains, half a grain apart, each advancing at `pitchRatio` instead
// of 1.  Cross-faded with complementary Hann windows whose sum is always 1,
// so there's never a volume dip at grain boundaries.
//
// Range: ±22 semitones (ratio 0.4..2.2)
// Grain size: ~93ms @ 44.1kHz (4096 samples)
// Ring buffer: 16384 samples per channel
// Latency: one grain = 4096 samples
class PitchShifter
{
public:
    static constexpr int kGrain    = 4096;
    static constexpr int kRingSize = 16384;

    PitchShifter() = default;

    void prepare(double /*sampleRate*/, int maxChannels)
    {
        buffers.assign(maxChannels, std::vector<float>(kRingSize, 0.0f));
        writePos = 0;
        posA     = 0.0;
        posB     = static_cast<double>(kGrain) / 2.0;
    }

    void reset()
    {
        for (auto& ch : buffers) std::fill(ch.begin(), ch.end(), 0.0f);
        writePos = 0;
        posA = 0.0;
        posB = static_cast<double>(kGrain) / 2.0;
    }

    // semitones ∈ [-12, +12] (or wider, clamped to ratio 0.4–2.2)
    void setSemitones(double semitones) noexcept
    {
        ratio = std::pow(2.0, semitones / 12.0);
        ratio = std::clamp(ratio, 0.4, 2.2);
    }

    void processBlock(const float* const* inChannels,
                      float* const*       outChannels,
                      int numChannels, int numSamples) noexcept
    {
        const int chCount = std::min(numChannels, static_cast<int>(buffers.size()));

        for (int i = 0; i < numSamples; i++)
        {
            // Write input into all channel rings
            for (int ch = 0; ch < chCount; ch++)
                buffers[ch][writePos] = inChannels[ch][i];

            // Hann windows — sum = 1 at every sample
            const double winA = 0.5 - 0.5 * std::cos((2.0 * M_PI * posA) / kGrain);
            const double winB = 1.0 - winA;

            for (int ch = 0; ch < chCount; ch++)
            {
                const double rPA = writePos - kGrain + posA;
                const double rPB = writePos - kGrain + posB;
                const float sA   = readInterpolated(buffers[ch], rPA);
                const float sB   = readInterpolated(buffers[ch], rPB);
                outChannels[ch][i] = static_cast<float>(winA * sA + winB * sB);
            }

            writePos = (writePos + 1) % kRingSize;

            posA += ratio;
            if (posA >= kGrain) posA -= kGrain;
            posB += ratio;
            if (posB >= kGrain) posB -= kGrain;
        }
    }

private:
    std::vector<std::vector<float>> buffers;
    int    writePos = 0;
    double posA = 0.0;
    double posB = static_cast<double>(kGrain) / 2.0;
    double ratio = 1.0;

    static float readInterpolated(const std::vector<float>& buf, double pos) noexcept
    {
        const int n    = static_cast<int>(buf.size());
        int base       = static_cast<int>(std::floor(pos));
        const double f = pos - base;
        const int i0   = ((base % n) + n) % n;
        const int i1   = (i0 + 1) % n;
        return static_cast<float>(buf[i0] * (1.0 - f) + buf[i1] * f);
    }
};
