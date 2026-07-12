#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Parameter IDs (string constants)
// ─────────────────────────────────────────────────────────────────────────────
namespace Params {
    // Live section
    static constexpr char BASS_ON[]          = "bassOn";
    static constexpr char BASS_FREQ[]        = "bassFreq";
    static constexpr char BASS_GAIN[]        = "bassGain";
    static constexpr char VOLUME[]           = "volume";
    static constexpr char BYPASS[]           = "bypass";
    // Reverb
    static constexpr char REVERB_ON[]        = "reverbOn";
    static constexpr char REVERB_MIX[]       = "reverbMix";
    static constexpr char REVERB_AMOUNT[]    = "reverbAmount";
    static constexpr char SONG_VOLUME[]      = "songVolume";
    static constexpr char REVERB_DECAY[]     = "reverbDecay";
    static constexpr char REVERB_PREDELAY[]  = "reverbPredelay";
    static constexpr char REVERB_DIFFUSE[]   = "reverbDiffuse";
    static constexpr char REVERB_TONE[]      = "reverbTone";
    static constexpr char REVERB_RES_HZ[]    = "reverbResHz";
    static constexpr char REVERB_RES_Q[]     = "reverbResQ";
    static constexpr char REVERB_ROOM[]      = "reverbRoom";
    static constexpr char REVERB_DENSITY[]   = "reverbDensity";
    static constexpr char REVERB_ER_LEVEL[]  = "reverbErLevel";
    static constexpr char REVERB_ER_DELAY[]  = "reverbErDelay";
    static constexpr char REVERB_LATE[]      = "reverbLate";
    static constexpr char REVERB_HF_DAMP[]   = "reverbHfDamp";
    static constexpr char REVERB_LF_DAMP[]   = "reverbLfDamp";
    static constexpr char REVERB_HIGH_CUT[]  = "reverbHighCut";
    static constexpr char REVERB_LOW_CUT[]   = "reverbLowCut";
    static constexpr char REVERB_WIDTH[]     = "reverbWidth";
    static constexpr char REVERB_MOD_DEPTH[] = "reverbModDepth";
    static constexpr char REVERB_MOD_RATE[]  = "reverbModRate";
    static constexpr char REVERB_WET_LVL[]   = "reverbWetLvl";
    static constexpr char REVERB_DRY_LVL[]   = "reverbDryLvl";
    // Acoustic Engine
    static constexpr char AE_ON[]            = "aeOn";
    static constexpr char AE_SURROUND[]      = "aeSurround";
    static constexpr char AE_CRYSTAL[]       = "aeCrystal";
    static constexpr char AE_BASS[]          = "aeBass";
    static constexpr char AE_CROSSOVER[]     = "aeCrossover";
    static constexpr char AE_SMARTVOL[]      = "aeSmartVol";
    static constexpr char AE_DIALOG[]        = "aeDialog";
    // EQ
    static constexpr char EQ_ON[]            = "eqOn";
    static constexpr char EQ_BAND[]          = "eqBand";   // + band index suffix 0-9
    // Compressor
    static constexpr char COMP_ON[]          = "compOn";
    static constexpr char COMP_THRESH[]      = "compThresh";
    static constexpr char COMP_RATIO[]       = "compRatio";
    static constexpr char COMP_ATTACK[]      = "compAttack";
    static constexpr char COMP_RELEASE[]     = "compRelease";
    static constexpr char COMP_MAKEUP[]      = "compMakeup";
    // Limiter
    static constexpr char LIM_ON[]           = "limOn";
    static constexpr char LIM_THRESH[]       = "limThresh";
    static constexpr char LIM_RELEASE[]      = "limRelease";
    // Stereo Width
    static constexpr char WIDTH_ON[]         = "widthOn";
    static constexpr char WIDTH[]            = "width";
    // Pitch
    static constexpr char PITCH_ON[]         = "pitchOn";
    static constexpr char PITCH[]            = "pitch";
    // Dynamic Bass
    static constexpr char DYNBASS_ON[]       = "dynBassOn";
    static constexpr char DYNBASS_SENS[]     = "dynBassSens";
    static constexpr char DYNBASS_STR[]      = "dynBassStr";
    // Speaker Config
    static constexpr char SPEAKER_ON[]       = "speakerOn";
    static constexpr char SPEAKER_MODE[]     = "speakerMode";
    static constexpr char SPK_FRONT_W[]      = "spkFrontW";
    static constexpr char SPK_REAR_W[]       = "spkRearW";
    static constexpr char SPK_CTR_DIST[]     = "spkCtrDist";
    static constexpr char SPK_REAR_DIST[]    = "spkRearDist";
    static constexpr char SPK_SUB_DIST[]     = "spkSubDist";
    static constexpr char SPK_LVL_FL[]       = "spkLvlFL";
    static constexpr char SPK_LVL_FR[]       = "spkLvlFR";
    static constexpr char SPK_LVL_C[]        = "spkLvlC";
    static constexpr char SPK_LVL_SUB[]      = "spkLvlSub";
    static constexpr char SPK_LVL_RL[]       = "spkLvlRL";
    static constexpr char SPK_LVL_RR[]       = "spkLvlRR";
    // Spectrum
    static constexpr char SPECTRUM_ON[]      = "spectrumOn";
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
BassNukerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    using FloatParam = juce::AudioParameterFloat;
    using BoolParam  = juce::AudioParameterBool;
    using ChoiceParam= juce::AudioParameterChoice;

    // ── Live section ─────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::BASS_ON,       "Bass Boost", true));
    params.push_back(std::make_unique<FloatParam>  (Params::BASS_FREQ,    "Bass Freq",
        juce::NormalisableRange<float>(40.0f, 1000.0f, 1.0f, 0.3f), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<FloatParam>  (Params::BASS_GAIN,    "Bass Gain",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 12.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<FloatParam>  (Params::VOLUME,       "Volume",
        juce::NormalisableRange<float>(0.0f, 600.0f, 0.5f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<BoolParam>  (Params::BYPASS,        "Bypass", false));

    // ── Reverb ───────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::REVERB_ON,     "Reverb", true));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_MIX,   "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 74.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_AMOUNT,"Effects Amount",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::SONG_VOLUME,  "Song Volume",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.5f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_DECAY, "Reverb Decay",
        juce::NormalisableRange<float>(0.1f, 25.0f, 0.01f, 0.35f), 9.0f,
        juce::AudioParameterFloatAttributes().withLabel("s")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_PREDELAY,"Pre-Delay",
        juce::NormalisableRange<float>(0.0f, 1500.0f, 1.0f), 38.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_DIFFUSE,"Diffuse",
        juce::NormalisableRange<float>(0.0f, 90.0f, 0.5f), 78.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_TONE,  "Tone",
        juce::NormalisableRange<float>(500.0f, 12000.0f, 1.0f, 0.4f), 3600.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_RES_HZ,"Resonance Hz",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_RES_Q, "Resonance Q",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_ROOM,  "Room Size",
        juce::NormalisableRange<float>(0.25f, 3.0f, 0.01f), 2.6f));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_DENSITY,"Density",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 780.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_ER_LEVEL,"Early Refl Level",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 38.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_ER_DELAY,"Early Refl Delay",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_LATE,  "Late Reverb Level",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_HF_DAMP,"HF Damping",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 350.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_LF_DAMP,"LF Damping",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_HIGH_CUT,"High Cut",
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.4f), 9000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_LOW_CUT,"Low Cut",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 90.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_WIDTH, "Reverb Width",
        juce::NormalisableRange<float>(0.0f, 2000.0f, 1.0f), 165.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_MOD_DEPTH,"Mod Depth",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 550.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_MOD_RATE,"Mod Rate",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 300.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_WET_LVL,"Wet Level",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::REVERB_DRY_LVL,"Dry Level",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── Acoustic Engine ───────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::AE_ON,         "Acoustic Engine", false));
    params.push_back(std::make_unique<FloatParam>  (Params::AE_SURROUND,  "Surround",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::AE_CRYSTAL,   "Crystalizer",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::AE_BASS,      "SBX Bass",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::AE_CROSSOVER, "Crossover",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.4f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<FloatParam>  (Params::AE_SMARTVOL,  "Smart Volume",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::AE_DIALOG,    "Dialog+",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── 10-Band EQ ────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::EQ_ON,         "EQ", false));
    for (int b = 0; b < 10; b++)
    {
        const juce::String bandId  = juce::String(Params::EQ_BAND) + juce::String(b);
        const juce::String bandName= juce::String("EQ Band ") + juce::String(b);
        params.push_back(std::make_unique<FloatParam>(bandId, bandName,
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
    }

    // ── Compressor ────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::COMP_ON,       "Compressor", false));
    params.push_back(std::make_unique<FloatParam>  (Params::COMP_THRESH,  "Comp Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.5f), -24.0f,
        juce::AudioParameterFloatAttributes().withLabel("dBFS")));
    params.push_back(std::make_unique<FloatParam>  (Params::COMP_RATIO,   "Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f,
        juce::AudioParameterFloatAttributes().withLabel(":1")));
    params.push_back(std::make_unique<FloatParam>  (Params::COMP_ATTACK,  "Comp Attack",
        juce::NormalisableRange<float>(0.1f, 500.0f, 0.1f, 0.3f), 3.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<FloatParam>  (Params::COMP_RELEASE, "Comp Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f, 0.3f), 250.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<FloatParam>  (Params::COMP_MAKEUP,  "Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // ── Limiter ───────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::LIM_ON,        "Limiter", false));
    params.push_back(std::make_unique<FloatParam>  (Params::LIM_THRESH,   "Lim Threshold",
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.1f), -3.0f,
        juce::AudioParameterFloatAttributes().withLabel("dBFS")));
    params.push_back(std::make_unique<FloatParam>  (Params::LIM_RELEASE,  "Lim Release",
        juce::NormalisableRange<float>(1.0f, 500.0f, 1.0f, 0.3f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // ── Stereo Width ──────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::WIDTH_ON,      "Stereo Width", false));
    params.push_back(std::make_unique<FloatParam>  (Params::WIDTH,        "Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.5f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── Pitch ─────────────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::PITCH_ON,      "Pitch", false));
    params.push_back(std::make_unique<FloatParam>  (Params::PITCH,        "Pitch Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    // ── Dynamic Bass ──────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::DYNBASS_ON,    "Dynamic Bass", false));
    params.push_back(std::make_unique<FloatParam>  (Params::DYNBASS_SENS, "DynBass Sensitivity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::DYNBASS_STR,  "DynBass Strength",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── Speaker Config ────────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>  (Params::SPEAKER_ON,    "Speaker Config", false));
    params.push_back(std::make_unique<ChoiceParam>(Params::SPEAKER_MODE,  "Speaker Mode",
        juce::StringArray { "Stereo", "Headphones", "2.1", "4.0", "4.1", "5.1", "7.1" }, 0));
    params.push_back(std::make_unique<FloatParam>  (Params::SPK_FRONT_W,  "Front Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::SPK_REAR_W,   "Rear Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::SPK_CTR_DIST, "Center Distance",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::SPK_REAR_DIST,"Rear Distance",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<FloatParam>  (Params::SPK_SUB_DIST, "Sub Distance",
        juce::NormalisableRange<float>(0.0f, 30.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("ft")));
    for (const auto* id : { Params::SPK_LVL_FL, Params::SPK_LVL_FR, Params::SPK_LVL_C,
                            Params::SPK_LVL_SUB, Params::SPK_LVL_RL, Params::SPK_LVL_RR })
    {
        params.push_back(std::make_unique<FloatParam>(id, juce::String(id),
            juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));
    }

    // ── Spectrum Analyzer ─────────────────────────────────────────────────────
    params.push_back(std::make_unique<BoolParam>(Params::SPECTRUM_ON, "Spectrum", false));

    return { params.begin(), params.end() };
}

// ─────────────────────────────────────────────────────────────────────────────
BassNukerProcessor::BassNukerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      spectrumFft(7),   // 2^7 = 128 bins
      spectrumWindow(128, juce::dsp::WindowingFunction<float>::hann)
{
    presetManager = std::make_unique<PresetManager>(apvts);

    // Register as listener for all parameters
    for (auto* p : apvts.processor.getParameters())
        apvts.addParameterListener(p->getParameterID(), this);
}

BassNukerProcessor::~BassNukerProcessor()
{
    for (auto* p : apvts.processor.getParameters())
        apvts.removeParameterListener(p->getParameterID(), this);
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerProcessor::prepareToPlay(double sampleRate, int maxBlockSize)
{
    workL.resize(maxBlockSize);
    workR.resize(maxBlockSize);
    spectrumFifo.resize(128, 0.0f);
    spectrumFftData.resize(256, 0.0f);

    bassBoost.prepare(sampleRate);
    reverbEngine.prepare(sampleRate, maxBlockSize);
    acousticEngine.prepare(sampleRate, maxBlockSize);
    advancedAudio.prepare(sampleRate, maxBlockSize);
    speakerConfig.prepare(sampleRate, maxBlockSize);

    paramsDirty.store(true);
}

void BassNukerProcessor::releaseResources() {}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels= buffer.getNumChannels();

    if (numChannels < 2 || numSamples == 0) return;

    rebuildParamsIfNeeded();

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(1);

    // ── A/B Bypass ───────────────────────────────────────────────────────────
    const bool bypassed = static_cast<bool>(*apvts.getRawParameterValue(Params::BYPASS));
    if (bypassed) return;

    // ── Volume ────────────────────────────────────────────────────────────────
    const float volume = *apvts.getRawParameterValue(Params::VOLUME) / 100.0f;
    if (std::abs(volume - 1.0f) > 0.001f)
        buffer.applyGain(volume);

    // ── Bass Boost ────────────────────────────────────────────────────────────
    if (static_cast<bool>(*apvts.getRawParameterValue(Params::BASS_ON)))
        for (int i = 0; i < numSamples; i++) bassBoost.processStereo(L[i], R[i]);

    // ── Reverb Engine (includes dry path & outer mix) ────────────────────────
    reverbEngine.processBlock(L, R, L, R, numSamples);

    // ── Acoustic Engine ───────────────────────────────────────────────────────
    if (static_cast<bool>(*apvts.getRawParameterValue(Params::AE_ON)))
        acousticEngine.processBlock(L, R, numSamples);

    // ── Advanced Audio Chain ─────────────────────────────────────────────────
    advancedAudio.processBlock(L, R, numSamples);

    // ── Speaker Configuration ─────────────────────────────────────────────────
    if (static_cast<bool>(*apvts.getRawParameterValue(Params::SPEAKER_ON)))
        speakerConfig.processBlock(L, R, numSamples);

    // ── Spectrum Analyser ─────────────────────────────────────────────────────
    if (spectrumEnabled.load())
        for (int i = 0; i < numSamples; i++) pushSpectrumSample((L[i] + R[i]) * 0.5f);

    // ── VU Metering ───────────────────────────────────────────────────────────
    updateVUMeter(L, R, numSamples);

    // ── Soft Tanh Clipper ─────────────────────────────────────────────────────
    softClipper.processBlock(L, R, numSamples);
}

void BassNukerProcessor::processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) {}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerProcessor::rebuildParamsIfNeeded()
{
    if (!paramsDirty.exchange(false)) return;

    // ── Bass Boost ────────────────────────────────────────────────────────────
    {
        const float freq = *apvts.getRawParameterValue(Params::BASS_FREQ);
        const float gain = *apvts.getRawParameterValue(Params::BASS_GAIN);
        bassBoost.setLowShelf(freq, gain);
    }

    // ── Reverb ────────────────────────────────────────────────────────────────
    syncReverbEngineParams();

    // ── Acoustic Engine ───────────────────────────────────────────────────────
    syncAcousticEngineParams();

    // ── Advanced Audio ────────────────────────────────────────────────────────
    syncAdvancedAudioParams();

    // ── Speaker Config ────────────────────────────────────────────────────────
    syncSpeakerConfigParams();

    // ── Spectrum ──────────────────────────────────────────────────────────────
    spectrumEnabled.store(static_cast<bool>(*apvts.getRawParameterValue(Params::SPECTRUM_ON)));
}

void BassNukerProcessor::syncReverbEngineParams()
{
    HybridReverbEngine::Params rp;
    rp.preDelay          = *apvts.getRawParameterValue(Params::REVERB_PREDELAY) / 1000.0;
    rp.earlyReflDelay    = *apvts.getRawParameterValue(Params::REVERB_ER_DELAY) / 1000.0;
    rp.earlyReflLevel    = *apvts.getRawParameterValue(Params::REVERB_ER_LEVEL) / 100.0;
    rp.lateReverbLevel   = *apvts.getRawParameterValue(Params::REVERB_LATE) / 100.0;
    rp.decayTime         = *apvts.getRawParameterValue(Params::REVERB_DECAY);
    rp.diffusion         = *apvts.getRawParameterValue(Params::REVERB_DIFFUSE) / 90.0;
    rp.density           = *apvts.getRawParameterValue(Params::REVERB_DENSITY) / 1000.0;
    rp.hfDamping         = *apvts.getRawParameterValue(Params::REVERB_HF_DAMP) / 1000.0;
    rp.lfDamping         = *apvts.getRawParameterValue(Params::REVERB_LF_DAMP) / 1000.0;
    rp.highCut           = *apvts.getRawParameterValue(Params::REVERB_HIGH_CUT);
    rp.lowCut            = *apvts.getRawParameterValue(Params::REVERB_LOW_CUT);
    rp.stereoWidth       = *apvts.getRawParameterValue(Params::REVERB_WIDTH) / 100.0;
    rp.roomSize          = *apvts.getRawParameterValue(Params::REVERB_ROOM);
    rp.modulationDepth   = *apvts.getRawParameterValue(Params::REVERB_MOD_DEPTH) / 1000.0;
    rp.modulationRate    = *apvts.getRawParameterValue(Params::REVERB_MOD_RATE) / 1000.0;
    rp.wetLevel          = *apvts.getRawParameterValue(Params::REVERB_WET_LVL) / 100.0;
    rp.dryLevel          = *apvts.getRawParameterValue(Params::REVERB_DRY_LVL) / 100.0;
    rp.resonanceHz       = *apvts.getRawParameterValue(Params::REVERB_RES_HZ);
    rp.resonanceQ        = *apvts.getRawParameterValue(Params::REVERB_RES_Q);

    const bool on        = static_cast<bool>(*apvts.getRawParameterValue(Params::REVERB_ON));
    const float mix      = *apvts.getRawParameterValue(Params::REVERB_MIX);
    const float amount   = *apvts.getRawParameterValue(Params::REVERB_AMOUNT);
    const float songVol  = *apvts.getRawParameterValue(Params::SONG_VOLUME);
    rp.reverbMixWet = on ? (mix / 100.0f) * (amount / 100.0f) : 0.0;
    rp.reverbMixDry = std::max(0.0, static_cast<double>(songVol) / 100.0);

    reverbEngine.setParams(rp);
}

void BassNukerProcessor::syncAcousticEngineParams()
{
    AcousticEngine::Params ap;
    ap.surround    = *apvts.getRawParameterValue(Params::AE_SURROUND);
    ap.crystalizer = *apvts.getRawParameterValue(Params::AE_CRYSTAL);
    ap.bass        = *apvts.getRawParameterValue(Params::AE_BASS);
    ap.crossover   = *apvts.getRawParameterValue(Params::AE_CROSSOVER);
    ap.smartVolume = *apvts.getRawParameterValue(Params::AE_SMARTVOL);
    ap.dialogPlus  = *apvts.getRawParameterValue(Params::AE_DIALOG);
    acousticEngine.update(ap);
}

void BassNukerProcessor::syncAdvancedAudioParams()
{
    // EQ
    advancedAudio.eq.enabled = static_cast<bool>(*apvts.getRawParameterValue(Params::EQ_ON));
    for (int b = 0; b < 10; b++)
    {
        const auto id  = juce::String(Params::EQ_BAND) + juce::String(b);
        const float dB = *apvts.getRawParameterValue(id);
        advancedAudio.eq.setGain(b, dB);
    }

    // Dynamic Bass
    advancedAudio.dynBassEnabled = static_cast<bool>(*apvts.getRawParameterValue(Params::DYNBASS_ON));
    advancedAudio.dynBass.setSensitivity(*apvts.getRawParameterValue(Params::DYNBASS_SENS));
    advancedAudio.dynBass.setStrength(*apvts.getRawParameterValue(Params::DYNBASS_STR));

    // Compressor
    advancedAudio.comp.enabled   = static_cast<bool>(*apvts.getRawParameterValue(Params::COMP_ON));
    advancedAudio.comp.threshold = *apvts.getRawParameterValue(Params::COMP_THRESH);
    advancedAudio.comp.ratio     = *apvts.getRawParameterValue(Params::COMP_RATIO);
    advancedAudio.comp.attack    = *apvts.getRawParameterValue(Params::COMP_ATTACK) / 1000.0;
    advancedAudio.comp.release   = *apvts.getRawParameterValue(Params::COMP_RELEASE) / 1000.0;
    advancedAudio.comp.makeupDb  = *apvts.getRawParameterValue(Params::COMP_MAKEUP);

    // Limiter
    advancedAudio.lim.enabled   = static_cast<bool>(*apvts.getRawParameterValue(Params::LIM_ON));
    advancedAudio.lim.threshold = *apvts.getRawParameterValue(Params::LIM_THRESH);
    advancedAudio.lim.release   = *apvts.getRawParameterValue(Params::LIM_RELEASE) / 1000.0;

    // Stereo Width
    advancedAudio.stereoWidthEnabled = static_cast<bool>(*apvts.getRawParameterValue(Params::WIDTH_ON));
    advancedAudio.width.setWidth(*apvts.getRawParameterValue(Params::WIDTH));

    // Pitch
    advancedAudio.pitchEnabled = static_cast<bool>(*apvts.getRawParameterValue(Params::PITCH_ON));
    advancedAudio.pitch.setSemitones(*apvts.getRawParameterValue(Params::PITCH));
}

void BassNukerProcessor::syncSpeakerConfigParams()
{
    const int modeIdx = static_cast<int>(*apvts.getRawParameterValue(Params::SPEAKER_MODE));
    SpeakerConfigEngine::Mode modes[] = {
        SpeakerConfigEngine::Mode::Stereo,
        SpeakerConfigEngine::Mode::Headphones,
        SpeakerConfigEngine::Mode::M21,
        SpeakerConfigEngine::Mode::M40,
        SpeakerConfigEngine::Mode::M41,
        SpeakerConfigEngine::Mode::M51,
        SpeakerConfigEngine::Mode::M71,
    };
    speakerConfig.setMode(modes[std::clamp(modeIdx, 0, 6)]);

    SpeakerConfigEngine::Layout layout;
    layout.frontWidth     = *apvts.getRawParameterValue(Params::SPK_FRONT_W)  / 100.0;
    layout.rearWidth      = *apvts.getRawParameterValue(Params::SPK_REAR_W)   / 100.0;
    layout.centerDistance = *apvts.getRawParameterValue(Params::SPK_CTR_DIST) / 100.0;
    layout.rearDistance   = *apvts.getRawParameterValue(Params::SPK_REAR_DIST)/ 100.0;
    layout.subDistanceFt  = *apvts.getRawParameterValue(Params::SPK_SUB_DIST);
    layout.levelFL        = *apvts.getRawParameterValue(Params::SPK_LVL_FL)   / 100.0;
    layout.levelFR        = *apvts.getRawParameterValue(Params::SPK_LVL_FR)   / 100.0;
    layout.levelC         = *apvts.getRawParameterValue(Params::SPK_LVL_C)    / 100.0;
    layout.levelSub       = *apvts.getRawParameterValue(Params::SPK_LVL_SUB)  / 100.0;
    layout.levelRL        = *apvts.getRawParameterValue(Params::SPK_LVL_RL)   / 100.0;
    layout.levelRR        = *apvts.getRawParameterValue(Params::SPK_LVL_RR)   / 100.0;
    speakerConfig.setLayout(layout);
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerProcessor::updateVUMeter(const float* L, const float* R, int numSamples)
{
    float peakL = 0, peakR = 0, sumSqL = 0, sumSqR = 0;
    for (int i = 0; i < numSamples; i++)
    {
        peakL = std::max(peakL, std::abs(L[i]));
        peakR = std::max(peakR, std::abs(R[i]));
        sumSqL += L[i] * L[i];
        sumSqR += R[i] * R[i];
    }
    const float rmsL = std::sqrt(sumSqL / numSamples);
    const float rmsR = std::sqrt(sumSqR / numSamples);

    // Exponential smoothing
    const float smoothed = 0.94f;
    vuSmL = smoothed * vuSmL + (1.0f - smoothed) * rmsL;
    vuSmR = smoothed * vuSmR + (1.0f - smoothed) * rmsR;

    // Peak hold
    if (peakL > vuPkL) { vuPkL = peakL; vuPkHoldL = kPeakHoldSamples; }
    else if (vuPkHoldL > 0) { --vuPkHoldL; }
    else vuPkL = std::max(0.0f, vuPkL - 0.0001f);

    if (peakR > vuPkR) { vuPkR = peakR; vuPkHoldR = kPeakHoldSamples; }
    else if (vuPkHoldR > 0) { --vuPkHoldR; }
    else vuPkR = std::max(0.0f, vuPkR - 0.0001f);

    vuLevelL.store(vuSmL);
    vuLevelR.store(vuSmR);
    vuPeakL.store(vuPkL);
    vuPeakR.store(vuPkR);
    clipping.store(peakL >= kPeakWarnThreshold || peakR >= kPeakWarnThreshold);
}

void BassNukerProcessor::pushSpectrumSample(float sample)
{
    spectrumFifo[spectrumFifoIndex++] = sample;
    if (spectrumFifoIndex >= 128)
    {
        spectrumFifoIndex = 0;
        std::copy(spectrumFifo.begin(), spectrumFifo.end(), spectrumFftData.begin());
        std::fill(spectrumFftData.begin() + 128, spectrumFftData.end(), 0.0f);
        spectrumWindow.multiplyWithWindowingTable(spectrumFftData.data(), 128);
        spectrumFft.performFrequencyOnlyForwardTransform(spectrumFftData.data());
        for (int i = 0; i < kSpectrumBins; i++)
            spectrumData[i] = spectrumFftData[i] / 128.0f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerProcessor::applyReverbPreset(const std::string& id)
{
    const auto* p = presetManager->findFactoryPreset(id);
    if (!p) return;

    auto setf = [&](const char* pid, float val) {
        if (auto* param = apvts.getParameter(pid)) param->setValueNotifyingHost(
            apvts.getParameterRange(pid).convertTo0to1(val));
    };

    setf(Params::REVERB_DECAY,    static_cast<float>(p->decay));
    setf(Params::REVERB_MIX,      static_cast<float>(p->mix));
    setf(Params::REVERB_PREDELAY, static_cast<float>(p->predelay));
    setf(Params::REVERB_DIFFUSE,  static_cast<float>(p->diffuse));
    setf(Params::REVERB_TONE,     static_cast<float>(p->toneHz));
    setf(Params::REVERB_RES_HZ,   static_cast<float>(p->resonanceHz));
    setf(Params::REVERB_RES_Q,    static_cast<float>(p->resonanceQ));
    setf(Params::REVERB_ROOM,     static_cast<float>(p->roomSize));
    setf(Params::REVERB_DENSITY,  static_cast<float>(p->density));
    setf(Params::REVERB_ER_LEVEL, static_cast<float>(p->earlyReflectionLevel));
    setf(Params::REVERB_ER_DELAY, static_cast<float>(p->earlyReflectionDelay));
    setf(Params::REVERB_LATE,     static_cast<float>(p->lateReverbLevel));
    setf(Params::REVERB_HF_DAMP,  static_cast<float>(p->hfDamping * 10.0));
    setf(Params::REVERB_LF_DAMP,  static_cast<float>(p->lfDamping * 10.0));
    setf(Params::REVERB_WIDTH,    static_cast<float>(p->stereoWidth));
    setf(Params::REVERB_MOD_DEPTH,static_cast<float>(p->modulationDepth * 10.0));
    setf(Params::REVERB_MOD_RATE, static_cast<float>(p->modulationRate * 10.0));
    setf(Params::REVERB_LOW_CUT,  static_cast<float>(p->lowCut));
    setf(Params::REVERB_WET_LVL,  static_cast<float>(p->wetLevel));
    setf(Params::REVERB_DRY_LVL,  static_cast<float>(p->dryLevel));
}

void BassNukerProcessor::applyEqPreset(const std::string& id)
{
    const auto* p = presetManager->findEqPreset(id);
    if (!p) return;
    for (int b = 0; b < 10; b++)
    {
        const auto pid = juce::String(Params::EQ_BAND) + juce::String(b);
        if (auto* param = apvts.getParameter(pid))
            param->setValueNotifyingHost(apvts.getParameterRange(pid).convertTo0to1(p->bands[b]));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerProcessor::parameterChanged(const juce::String&, float)
{
    paramsDirty.store(true);
}

double BassNukerProcessor::getTailLengthSeconds() const
{
    const float decay = *apvts.getRawParameterValue(Params::REVERB_DECAY);
    return static_cast<bool>(*apvts.getRawParameterValue(Params::REVERB_ON))
               ? std::min(static_cast<double>(decay) + 1.0, 20.0)
               : 0.0;
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BassNukerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* BassNukerProcessor::createEditor()
{
    return new BassNukerEditor(*this);
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassNukerProcessor();
}
