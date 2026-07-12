#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/BiquadFilter.h"
#include "DSP/HybridReverbEngine.h"
#include "DSP/AcousticEngine.h"
#include "DSP/AdvancedAudioChain.h"
#include "DSP/SpeakerConfigEngine.h"
#include "DSP/SoftClipper.h"
#include "Presets/PresetManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Bass Nuker VST3 — main AudioProcessor
//
// DSP chain order (matches offscreen.js exactly):
//   Input
//   -> Bass Boost lowshelf (bassOn gate, bassFreq, bassGain)
//   -> Volume gain (volume)
//   -> [A/B bypass gate]
//      -> [Reverb wet path]
//         -> Pre-delay
//         -> Early Reflections (synthesized cave IR)
//         -> FDN late tail (8-line Householder)
//         -> HF/LF shelves, high/low-cut, M/S width
//      -> [Dry path] (songVolume)
//   -> Resonance peaking filter (on wet bus, inside reverb engine)
//   -> Acoustic Engine (surround / crystalizer / bass / smart vol / dialog+)
//   -> Advanced Audio (EQ -> DynBass -> Comp -> Lim -> Width -> Pitch)
//   -> Speaker Config Engine (headphones / 2.0 / 2.1 / 4.0 / 4.1 / 5.1 / 7.1)
//   -> Soft tanh clipper (threshold 0.8)
//   -> Output
// ─────────────────────────────────────────────────────────────────────────────

class BassNukerProcessor : public juce::AudioProcessor,
                           public juce::AudioProcessorValueTreeState::Listener
{
public:
    BassNukerProcessor();
    ~BassNukerProcessor() override;

    // ── AudioProcessor interface ─────────────────────────────────────────────
    void prepareToPlay(double sampleRate, int maxBlockSize) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override;

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ── APVTS ────────────────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── Preset access ────────────────────────────────────────────────────────
    PresetManager& getPresetManager() { return *presetManager; }

    // Apply a factory reverb preset (updates all reverb APVTS parameters)
    void applyReverbPreset(const std::string& id);
    void applyEqPreset(const std::string& id);

    // ── VU meter data (read from audio thread, lock-free) ───────────────────
    std::atomic<float> vuLevelL { 0.0f };
    std::atomic<float> vuLevelR { 0.0f };
    std::atomic<float> vuPeakL  { 0.0f };
    std::atomic<float> vuPeakR  { 0.0f };
    std::atomic<bool>  clipping { false };
    static constexpr float kPeakWarnThreshold = 0.85f;

    // ── Spectrum data (read by editor, filled by audio thread) ───────────────
    static constexpr int kSpectrumBins = 128;
    std::atomic<bool> spectrumEnabled { false };
    float spectrumData[kSpectrumBins] {}; // lock-free (single writer / reader)

    // ── Parameter listener ───────────────────────────────────────────────────
    void parameterChanged(const juce::String& paramID, float newValue) override;

private:
    // ── DSP nodes ────────────────────────────────────────────────────────────
    BiquadFilter       bassBoost;        // bass lowshelf
    HybridReverbEngine reverbEngine;     // complete hybrid reverb
    AcousticEngine     acousticEngine;   // SBX-style acoustic effects
    AdvancedAudioChain advancedAudio;    // EQ/Comp/Lim/Width/Pitch/DynBass
    SpeakerConfigEngine speakerConfig;   // virtual surround
    SoftClipper        softClipper;      // final tanh soft limiter

    // Spectrum analyser (optional)
    juce::dsp::FFT          spectrumFft;
    juce::dsp::WindowingFunction<float> spectrumWindow;
    std::vector<float>      spectrumFifo;
    std::vector<float>      spectrumFftData;
    int                     spectrumFifoIndex = 0;
    bool                    nextSpectrumReady  = false;

    // ── Working buffers ──────────────────────────────────────────────────────
    std::vector<float> workL, workR;

    // ── VU meter smoothing ───────────────────────────────────────────────────
    float vuSmL = 0.0f, vuSmR = 0.0f;
    float vuPkL = 0.0f, vuPkR = 0.0f;
    int   vuPkHoldL = 0, vuPkHoldR = 0;
    static constexpr int kPeakHoldSamples = 44100;

    // ── Preset manager ───────────────────────────────────────────────────────
    std::unique_ptr<PresetManager> presetManager;

    // ── Helpers ──────────────────────────────────────────────────────────────
    void syncReverbEngineParams();
    void syncAcousticEngineParams();
    void syncAdvancedAudioParams();
    void syncSpeakerConfigParams();
    void updateVUMeter(const float* L, const float* R, int numSamples);
    void pushSpectrumSample(float sample);

    // Cached parameter values (updated by parameterChanged)
    std::atomic<bool> paramsDirty { true };
    void rebuildParamsIfNeeded();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassNukerProcessor)
};
