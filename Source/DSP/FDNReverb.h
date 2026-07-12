#pragma once
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <array>
#include <cmath>
#include <vector>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─────────────────────────────────────────────────────────────────────────────
// 8-line Feedback Delay Network (FDN) late-reverb tail
//
// Direct port of fdn-reverb-worklet.js.  Architecture:
//   1. Per-line LFO-modulated, linearly-interpolated read
//   2. 2-stage per-line allpass diffusion (decorrelated coefficients)
//   3. Per-line HF (one-pole lowpass) + LF damping
//   4. RT60-correct per-line feedback gain  g = 10^(-3*delay/RT60)
//   5. Householder feedback matrix  Hv = v - (2/N)*sum(v)*1
//   6. Stereo output via decorrelated +/-1 tap patterns
//
// Input diffusion: 4-stage allpass driven by the Density parameter,
// applied separately to L and R before injection into even/odd lines.
// ─────────────────────────────────────────────────────────────────────────────
class FDNReverb
{
public:
    static constexpr int N = 8;

    FDNReverb() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Maximum buffer needed: largest base delay * maxRoomSize + mod excursion + margin
        const double maxRoomSize = 3.0;
        const double maxBase     = *std::max_element(baseDelaySec.begin(), baseDelaySec.end());
        const double maxModSec   = 0.008;
        const double maxDelaySec = maxBase * maxRoomSize + maxModSec + 0.02;
        bufLen = static_cast<int>(std::ceil(sampleRate * maxDelaySec)) + 4;

        for (int i = 0; i < N; i++)
        {
            buf[i].assign(bufLen, 0.0f);
            writeIdx[i] = 0;
            modPhase[i] = static_cast<double>(i) / N * (2.0 * M_PI);
        }

        ap1StateY.fill(0); ap1StateX.fill(0);
        ap2StateY.fill(0); ap2StateX.fill(0);
        hfState.fill(0);   lfState.fill(0);

        inDiffStL.fill(0); inDiffXL.fill(0);
        inDiffStR.fill(0); inDiffXR.fill(0);

        // Smooth coeff ~30ms time constant
        smoothCoeff = std::exp(-1.0 / (sampleRate * 0.03));

        // Initialise smoothed params to current targets
        sm = target;
    }

    void reset()
    {
        for (int i = 0; i < N; i++)
        {
            std::fill(buf[i].begin(), buf[i].end(), 0.0f);
            writeIdx[i] = 0;
        }
        ap1StateY.fill(0); ap1StateX.fill(0);
        ap2StateY.fill(0); ap2StateX.fill(0);
        hfState.fill(0);   lfState.fill(0);
        inDiffStL.fill(0); inDiffXL.fill(0);
        inDiffStR.fill(0); inDiffXR.fill(0);
    }

    // ── Parameter setters (thread-safe: called from message thread) ──────────
    void setRoomSize(double v)           noexcept { target.roomSize     = std::clamp(v, 0.25, 3.0); }
    void setDecayTime(double v)          noexcept { target.decayTime    = std::max(0.1, v); }
    void setDiffusion(double v)          noexcept { target.diffusion    = std::clamp(v, 0.0, 1.0); }
    void setDensity(double v)            noexcept { target.density      = std::clamp(v, 0.0, 1.0); }
    void setHfDamping(double v)          noexcept { target.hfDamping    = std::clamp(v, 0.0, 1.0); }
    void setLfDamping(double v)          noexcept { target.lfDamping    = std::clamp(v, 0.0, 1.0); }
    void setModDepth(double v)           noexcept { target.modDepth     = std::clamp(v, 0.0, 1.0); }
    void setModRate(double v)            noexcept { target.modRate      = std::clamp(v, 0.0, 1.0); }
    void setInputGain(double v)          noexcept { target.inputGain    = std::max(0.0, v); }
    void setOutputGain(double v)         noexcept { target.outputGain   = std::max(0.0, v); }

    // ── Process one stereo frame ─────────────────────────────────────────────
    void processStereo(float xIn, float yIn, float& outL, float& outR) noexcept
    {
        const double c = smoothCoeff;
        // Smooth all params
        sm.roomSize  += (1.0 - c) * (target.roomSize  - sm.roomSize);
        sm.decayTime += (1.0 - c) * (target.decayTime - sm.decayTime);
        sm.diffusion += (1.0 - c) * (target.diffusion - sm.diffusion);
        sm.density   += (1.0 - c) * (target.density   - sm.density);
        sm.hfDamping += (1.0 - c) * (target.hfDamping - sm.hfDamping);
        sm.lfDamping += (1.0 - c) * (target.lfDamping - sm.lfDamping);
        sm.modDepth  += (1.0 - c) * (target.modDepth  - sm.modDepth);
        sm.modRate   += (1.0 - c) * (target.modRate   - sm.modRate);
        const double inputGain  = target.inputGain;
        const double outputGain = target.outputGain;

        const double decayTime = std::max(0.1, sm.decayTime);
        const double diffK     = std::min(0.7, std::max(0.0, sm.diffusion * 0.7));

        // HF damping -> one-pole cutoff
        const double hfCutoff = 600.0 + (18000.0 - 600.0) * std::pow(1.0 - sm.hfDamping, 2.0);
        const double hfA      = std::exp(-(2.0 * M_PI * hfCutoff) / sr);
        // LF damping
        const double lfCutoff = 80.0 + 400.0 * sm.lfDamping;
        const double lfA      = std::exp(-(2.0 * M_PI * lfCutoff) / sr);
        const double lfAmount = sm.lfDamping * 0.9;

        const double modExcursion = sm.modDepth * 0.004;
        const double modRateHz    = 0.03 + sm.modRate * 1.47;

        // Input diffusion (4-stage allpass), L and R separately
        double dL = xIn * inputGain;
        double dR = yIn * inputGain;
        const double densityMix = 0.2 + sm.density * 0.5;
        for (int st = 0; st < 4; st++)
        {
            const double k = inDiffCoeffs[st] * densityMix;
            const double yL = -k * dL + inDiffXL[st] + k * inDiffStL[st];
            inDiffXL[st] = dL; inDiffStL[st] = yL; dL = yL;

            const double yR = -k * dR + inDiffXR[st] + k * inDiffStR[st];
            inDiffXR[st] = dR; inDiffStR[st] = yR; dR = yR;
        }

        // Per-line processing
        std::array<double, N> damped;
        for (int ln = 0; ln < N; ln++)
        {
            // LFO modulate read position
            modPhase[ln] += (2.0 * M_PI * modRateHz * modRateMul[ln]) / sr;
            if (modPhase[ln] > 2.0 * M_PI) modPhase[ln] -= 2.0 * M_PI;

            const double baseSec    = baseDelaySec[ln] * sm.roomSize;
            const double modSec     = std::sin(modPhase[ln]) * modExcursion;
            const double delaySec   = std::max(0.001, baseSec + modSec);
            const double delaySmps  = delaySec * sr;

            // Linearly-interpolated read
            double readPos = writeIdx[ln] - delaySmps;
            double rp = std::fmod(readPos, bufLen);
            if (rp < 0.0) rp += bufLen;
            const int i0  = static_cast<int>(rp);
            const double frac = rp - i0;
            const int i1  = (i0 + 1) % bufLen;
            const double rawSample = buf[ln][i0] * (1.0 - frac) + buf[ln][i1] * frac;

            // Per-line diffusion (2 cascaded allpasses)
            const double lineDiffK = std::min(0.7, diffK * diffMul[ln]);
            double d = allpass(rawSample, lineDiffK,          ap1StateX, ap1StateY, ln);
            d        = allpass(d,         lineDiffK * 0.8,    ap2StateX, ap2StateY, ln);

            // HF damping
            hfState[ln] = (1.0 - hfA) * d + hfA * hfState[ln];
            d = hfState[ln];

            // LF damping (subtract lowpassed component)
            lfState[ln] = (1.0 - lfA) * d + lfA * lfState[ln];
            d = d - lfAmount * lfState[ln];

            // RT60-correct per-line gain
            double g = std::pow(10.0, (-3.0 * delaySec) / decayTime);
            g = std::clamp(g, 0.0, 0.995);

            damped[ln] = d * g;
        }

        // Householder matrix: Hv = v - (2/N)*sum(v)*1
        double sum = 0.0;
        for (int ln = 0; ln < N; ln++) sum += damped[ln];
        const double sub = (2.0 / N) * sum;

        double lOut = 0.0, rOut = 0.0;
        for (int ln = 0; ln < N; ln++)
        {
            const double mixed  = damped[ln] - sub;
            const double inject = (ln % 2 == 0) ? dL : dR;
            const double wv     = mixed + inject * 0.6;
            buf[ln][writeIdx[ln]] = static_cast<float>(wv);
            writeIdx[ln] = (writeIdx[ln] + 1) % bufLen;
            lOut += mixed * tapL[ln];
            rOut += mixed * tapR[ln];
        }

        const double norm = outputGain / std::sqrt(static_cast<double>(N));
        outL = static_cast<float>(lOut * norm);
        outR = static_cast<float>(rOut * norm);
    }

    // Process a block (no internal dry path — reverb is always 100% wet out)
    void processBlock(const float* inL, const float* inR,
                      float* outL, float* outR, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; i++)
            processStereo(inL[i], inR[i], outL[i], outR[i]);
    }

private:
    // Irregular, incommensurate base delay lengths (seconds) from the original
    const std::array<double, N> baseDelaySec { 0.0233, 0.0297, 0.0331, 0.0389,
                                               0.0431, 0.0479, 0.0523, 0.0577 };
    // Per-line LFO rate detune
    const std::array<double, N> modRateMul   { 0.83, 1.0, 1.14, 0.96, 1.27, 0.9, 1.08, 1.2 };
    // Per-line diffusion coefficient detune (~10% jitter, mean=1)
    const std::array<double, N> diffMul      { 0.92, 1.05, 0.97, 1.1, 0.9, 1.06, 0.95, 1.08 };
    // Decorrelated stereo output tap patterns
    const std::array<double, N> tapL  {  1,  1, -1, -1,  1, -1, -1,  1 };
    const std::array<double, N> tapR  {  1, -1,  1, -1, -1,  1, -1,  1 };
    // Input diffusion allpass coefficients (4 stages)
    const std::array<double, 4> inDiffCoeffs { 0.68, -0.59, 0.51, -0.45 };

    // Delay buffers and write heads
    std::array<std::vector<float>, N> buf;
    std::array<int, N> writeIdx {};
    int bufLen = 0;

    // Modulation phase per line
    std::array<double, N> modPhase {};

    // Per-line allpass states (2 cascaded per line, both channels)
    std::array<double, N> ap1StateY {}, ap1StateX {};
    std::array<double, N> ap2StateY {}, ap2StateX {};

    // Per-line HF / LF one-pole filter states
    std::array<double, N> hfState {}, lfState {};

    // Input diffusion allpass states (4 stages, L and R)
    std::array<double, 4> inDiffStL {}, inDiffXL {};
    std::array<double, 4> inDiffStR {}, inDiffXR {};

    double sr = 44100.0;
    double smoothCoeff = 0.0;

    struct Params {
        double roomSize  = 1.0;
        double decayTime = 1.8;
        double diffusion = 0.7;
        double density   = 0.5;
        double hfDamping = 0.5;
        double lfDamping = 0.2;
        double modDepth  = 0.4;
        double modRate   = 0.35;
        double inputGain = 1.0;
        double outputGain= 1.0;
    } target, sm;

    // First-order allpass: y[n] = -k*x[n] + x[n-1] + k*y[n-1]
    static double allpass(double x, double k,
                          std::array<double,N>& xState,
                          std::array<double,N>& yState,
                          int idx) noexcept
    {
        const double y    = -k * x + xState[idx] + k * yState[idx];
        xState[idx] = x;
        yState[idx] = y;
        return y;
    }
};
