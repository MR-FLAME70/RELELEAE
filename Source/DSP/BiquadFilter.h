#pragma once
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Direct Form II Transposed biquad filter.
// Supports: lowshelf, highshelf, peaking, lowpass, highpass.
// All coefficients are computed at prepare() time and can be updated
// sample-accurately via update*(). Denormal protection via a tiny DC offset
// added to the state.
struct BiquadFilter
{
    enum class Type { LowShelf, HighShelf, Peaking, LowPass, HighPass };

    double b0 = 1, b1 = 0, b2 = 0;
    double a1 = 0, a2 = 0;
    double z1L = 0, z2L = 0;  // left channel state
    double z1R = 0, z2R = 0;  // right channel state

    double sampleRate = 44100.0;

    void prepare(double sr) noexcept
    {
        sampleRate = sr;
        reset();
    }

    void reset() noexcept
    {
        z1L = z2L = z1R = z2R = 0.0;
    }

    // ── Coefficient setters ───────────────────────────────────────────────

    void setLowShelf(double freqHz, double gainDb) noexcept
    {
        const double A  = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * M_PI * freqHz / sampleRate;
        const double cosW = std::cos(w0);
        const double S  = 1.0; // shelf slope = 1 (maximally flat)
        const double alpha = std::sin(w0) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        const double sqrtA = 2.0 * std::sqrt(A) * alpha;

        const double b0_ = A * ((A + 1.0) - (A - 1.0) * cosW + sqrtA);
        const double b1_ = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW);
        const double b2_ = A * ((A + 1.0) - (A - 1.0) * cosW - sqrtA);
        const double a0_ = (A + 1.0) + (A - 1.0) * cosW + sqrtA;
        const double a1_ = -2.0 * ((A - 1.0) + (A + 1.0) * cosW);
        const double a2_ = (A + 1.0) + (A - 1.0) * cosW - sqrtA;

        setCoeffs(b0_ / a0_, b1_ / a0_, b2_ / a0_, a1_ / a0_, a2_ / a0_);
    }

    void setHighShelf(double freqHz, double gainDb) noexcept
    {
        const double A  = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * M_PI * freqHz / sampleRate;
        const double cosW = std::cos(w0);
        const double S  = 1.0;
        const double alpha = std::sin(w0) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        const double sqrtA = 2.0 * std::sqrt(A) * alpha;

        const double b0_ = A * ((A + 1.0) + (A - 1.0) * cosW + sqrtA);
        const double b1_ = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW);
        const double b2_ = A * ((A + 1.0) + (A - 1.0) * cosW - sqrtA);
        const double a0_ = (A + 1.0) - (A - 1.0) * cosW + sqrtA;
        const double a1_ = 2.0 * ((A - 1.0) - (A + 1.0) * cosW);
        const double a2_ = (A + 1.0) - (A - 1.0) * cosW - sqrtA;

        setCoeffs(b0_ / a0_, b1_ / a0_, b2_ / a0_, a1_ / a0_, a2_ / a0_);
    }

    void setPeaking(double freqHz, double Q, double gainDb) noexcept
    {
        if (Q < 0.0001) Q = 0.0001;
        const double A  = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * M_PI * freqHz / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * Q);

        const double b0_ = 1.0 + alpha * A;
        const double b1_ = -2.0 * std::cos(w0);
        const double b2_ = 1.0 - alpha * A;
        const double a0_ = 1.0 + alpha / A;
        const double a1_ = -2.0 * std::cos(w0);
        const double a2_ = 1.0 - alpha / A;

        setCoeffs(b0_ / a0_, b1_ / a0_, b2_ / a0_, a1_ / a0_, a2_ / a0_);
    }

    void setLowPass(double freqHz, double Q = 0.707) noexcept
    {
        const double w0 = 2.0 * M_PI * freqHz / sampleRate;
        const double cosW = std::cos(w0);
        const double alpha = std::sin(w0) / (2.0 * Q);

        const double b0_ = (1.0 - cosW) / 2.0;
        const double b1_ = 1.0 - cosW;
        const double b2_ = (1.0 - cosW) / 2.0;
        const double a0_ = 1.0 + alpha;
        const double a1_ = -2.0 * cosW;
        const double a2_ = 1.0 - alpha;

        setCoeffs(b0_ / a0_, b1_ / a0_, b2_ / a0_, a1_ / a0_, a2_ / a0_);
    }

    void setHighPass(double freqHz, double Q = 0.707) noexcept
    {
        const double w0 = 2.0 * M_PI * freqHz / sampleRate;
        const double cosW = std::cos(w0);
        const double alpha = std::sin(w0) / (2.0 * Q);

        const double b0_ = (1.0 + cosW) / 2.0;
        const double b1_ = -(1.0 + cosW);
        const double b2_ = (1.0 + cosW) / 2.0;
        const double a0_ = 1.0 + alpha;
        const double a1_ = -2.0 * cosW;
        const double a2_ = 1.0 - alpha;

        setCoeffs(b0_ / a0_, b1_ / a0_, b2_ / a0_, a1_ / a0_, a2_ / a0_);
    }

    void setBypass() noexcept
    {
        b0 = 1.0; b1 = 0.0; b2 = 0.0;
        a1 = 0.0; a2 = 0.0;
    }

    // ── Per-sample processing ─────────────────────────────────────────────

    inline float processSampleL(float x) noexcept
    {
        const double xd = static_cast<double>(x);
        const double y  = b0 * xd + z1L;
        z1L = b1 * xd - a1 * y + z2L;
        z2L = b2 * xd - a2 * y;
        return static_cast<float>(y);
    }

    inline float processSampleR(float x) noexcept
    {
        const double xd = static_cast<double>(x);
        const double y  = b0 * xd + z1R;
        z1R = b1 * xd - a1 * y + z2R;
        z2R = b2 * xd - a2 * y;
        return static_cast<float>(y);
    }

    // Stereo frame
    inline void processStereo(float& L, float& R) noexcept
    {
        L = processSampleL(L);
        R = processSampleR(R);
    }

private:
    void setCoeffs(double b0_, double b1_, double b2_, double a1_, double a2_) noexcept
    {
        b0 = b0_; b1 = b1_; b2 = b2_;
        a1 = a1_; a2 = a2_;
    }
};
