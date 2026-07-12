#pragma once
#include <algorithm>

// Mid/Side stereo width processor.
// Port of createStereoWidthModule() from offscreen.js.
//
//   M = 0.5*(L+R),  S = 0.5*(L-R)
//   L' = M + width*S = (0.5 + 0.5*factor)*L + (0.5 - 0.5*factor)*R
//   R' = M - width*S = (0.5 - 0.5*factor)*L + (0.5 + 0.5*factor)*R
//
// factor = widthPercent / 100
//   0   => mono
//   100 => exact passthrough
//   200 => doubled side content (exaggerated stereo)
struct StereoWidth
{
    double llGain = 1.0, lrGain = 0.0;
    double rlGain = 0.0, rrGain = 1.0;

    void setWidth(double widthPercent) noexcept
    {
        const double factor = std::clamp(widthPercent, 0.0, 200.0) / 100.0;
        const double mid  = 0.5;
        const double side = 0.5 * factor;
        llGain = mid + side;
        rrGain = mid + side;
        lrGain = mid - side;
        rlGain = mid - side;
    }

    void processStereo(float& L, float& R) const noexcept
    {
        const float newL = static_cast<float>(llGain * L + lrGain * R);
        const float newR = static_cast<float>(rlGain * L + rrGain * R);
        L = newL;
        R = newR;
    }

    void processBlock(float* L, float* R, int numSamples) const noexcept
    {
        for (int i = 0; i < numSamples; i++)
            processStereo(L[i], R[i]);
    }
};
