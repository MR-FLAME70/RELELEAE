#pragma once
#include <cmath>
#include <algorithm>

// Soft-knee limiter ported from clamp-worklet.js.
// Below threshold (0.8) signal passes through unmodified.
// Above threshold: tanh-shaped soft knee so peaks compress
// smoothly rather than hard-clipping (which introduces waveform
// discontinuities that sound like cracking).
//
// Transfer: y = sign(x) * (threshold + headroom * tanh(excess / headroom))
//   where excess   = |x| - threshold
//         headroom = 1.0 - threshold
struct SoftClipper
{
    static constexpr float kThreshold = 0.8f;
    static constexpr float kHeadroom  = 1.0f - kThreshold;

    inline float processSample(float x) const noexcept
    {
        const float av = std::abs(x);
        if (av <= kThreshold) return x;

        const float sign   = (x < 0.0f) ? -1.0f : 1.0f;
        const float excess = av - kThreshold;
        return sign * (kThreshold + kHeadroom * std::tanh(excess / kHeadroom));
    }

    void processBlock(float* L, float* R, int numSamples) const noexcept
    {
        for (int i = 0; i < numSamples; i++)
        {
            L[i] = processSample(L[i]);
            R[i] = processSample(R[i]);
        }
    }
};
