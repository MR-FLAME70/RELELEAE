#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/VUMeter.h"
#include "UI/EQDisplay.h"

// ─────────────────────────────────────────────────────────────────────────────
// Tab identifiers — maps 1:1 to the popup.js tab layout
// ─────────────────────────────────────────────────────────────────────────────
enum class Tab { Live, Reverb, AcousticEngine, AdvancedAudio, SpeakerConfig, Presets };

// ─────────────────────────────────────────────────────────────────────────────
// Compact attachment helper — holds one Slider + SliderAttachment
// ─────────────────────────────────────────────────────────────────────────────
struct SliderWithAttachment
{
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void attach(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                juce::Slider::SliderStyle style = juce::Slider::RotaryVerticalDrag)
    {
        slider.setSliderStyle(style);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, slider);
    }
};

struct ButtonWithAttachment
{
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    void attach(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                const juce::String& text = {})
    {
        if (!text.isEmpty()) button.setButtonText(text);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, id, button);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Main plugin editor
// ─────────────────────────────────────────────────────────────────────────────
class BassNukerEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit BassNukerEditor(BassNukerProcessor&);
    ~BassNukerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BassNukerProcessor& proc;
    BassNukerLookAndFeel laf;

    // ── Tab bar ───────────────────────────────────────────────────────────────
    juce::TextButton tabLive      { "Live" };
    juce::TextButton tabReverb    { "Reverb" };
    juce::TextButton tabAcoustic  { "Acoustic" };
    juce::TextButton tabAdvanced  { "Advanced" };
    juce::TextButton tabSpeaker   { "Speaker" };
    juce::TextButton tabPresets   { "Presets" };
    Tab              activeTab    = Tab::Live;

    void switchTab(Tab t);

    // ── VU Meter ──────────────────────────────────────────────────────────────
    VUMeter vuMeter;

    // ── Title bar ─────────────────────────────────────────────────────────────
    juce::Label titleLabel;

    // ── Bypass ────────────────────────────────────────────────────────────────
    ButtonWithAttachment bypassBtn;

    // ── Live Tab ──────────────────────────────────────────────────────────────
    juce::Component livePage;
    ButtonWithAttachment bassOnBtn;
    SliderWithAttachment bassFreqSlider, bassGainSlider, volumeSlider;
    juce::Label bassFreqLabel { {}, "Freq" };
    juce::Label bassGainLabel { {}, "Gain" };
    juce::Label volumeLabel   { {}, "Volume" };

    // ── Reverb Tab ────────────────────────────────────────────────────────────
    juce::Component reverbPage;
    ButtonWithAttachment reverbOnBtn;
    juce::ComboBox   reverbPresetBox;
    SliderWithAttachment reverbMixSlider, reverbAmountSlider, songVolumeSlider;
    SliderWithAttachment reverbDecaySlider, reverbPredelaySlider, reverbDiffuseSlider;
    SliderWithAttachment reverbToneSlider, reverbResHzSlider, reverbResQSlider;
    SliderWithAttachment reverbRoomSlider, reverbDensitySlider;
    SliderWithAttachment reverbErLevelSlider, reverbErDelaySlider;
    SliderWithAttachment reverbLateSlider, reverbHfDampSlider, reverbLfDampSlider;
    SliderWithAttachment reverbHighCutSlider, reverbLowCutSlider;
    SliderWithAttachment reverbWidthSlider, reverbModDepthSlider, reverbModRateSlider;
    SliderWithAttachment reverbWetLvlSlider, reverbDryLvlSlider;

    // ── Acoustic Engine Tab ───────────────────────────────────────────────────
    juce::Component acousticPage;
    ButtonWithAttachment aeOnBtn;
    SliderWithAttachment aeSurroundSlider, aeCrystalSlider, aeBassSlider;
    SliderWithAttachment aeCrossoverSlider, aeSmartVolSlider, aeDialogSlider;

    // ── Advanced Audio Tab ────────────────────────────────────────────────────
    juce::Component advancedPage;
    // EQ
    ButtonWithAttachment eqOnBtn;
    juce::ComboBox eqPresetBox;
    EQDisplay      eqDisplay;
    SliderWithAttachment eqBandSliders[10];
    // Compressor
    ButtonWithAttachment compOnBtn;
    SliderWithAttachment compThreshSlider, compRatioSlider, compAttackSlider;
    SliderWithAttachment compReleaseSlider, compMakeupSlider;
    // Limiter
    ButtonWithAttachment limOnBtn;
    SliderWithAttachment limThreshSlider, limReleaseSlider;
    // Stereo Width
    ButtonWithAttachment widthOnBtn;
    SliderWithAttachment widthSlider;
    // Pitch
    ButtonWithAttachment pitchOnBtn;
    SliderWithAttachment pitchSlider;
    // Dynamic Bass
    ButtonWithAttachment dynBassOnBtn;
    SliderWithAttachment dynBassSensSlider, dynBassStrSlider;
    // Spectrum
    ButtonWithAttachment spectrumOnBtn;

    // ── Speaker Config Tab ────────────────────────────────────────────────────
    juce::Component speakerPage;
    ButtonWithAttachment speakerOnBtn;
    juce::ComboBox speakerModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> speakerModeAttachment;
    SliderWithAttachment spkFrontWSlider, spkRearWSlider;
    SliderWithAttachment spkCtrDistSlider, spkRearDistSlider, spkSubDistSlider;
    SliderWithAttachment spkLvlFLSlider, spkLvlFRSlider, spkLvlCSlider;
    SliderWithAttachment spkLvlSubSlider, spkLvlRLSlider, spkLvlRRSlider;

    // ── Presets Tab ───────────────────────────────────────────────────────────
    juce::Component presetsPage;
    juce::ListBox    userPresetList;
    juce::TextEditor presetNameEditor;
    juce::TextButton savePresetBtn   { "Save" };
    juce::TextButton loadPresetBtn   { "Load" };
    juce::TextButton deletePresetBtn { "Delete" };

    // ── Helpers ───────────────────────────────────────────────────────────────
    void buildLivePage();
    void buildReverbPage();
    void buildAcousticPage();
    void buildAdvancedPage();
    void buildSpeakerPage();
    void buildPresetsPage();

    void layoutLivePage(juce::Rectangle<int> area);
    void layoutReverbPage(juce::Rectangle<int> area);
    void layoutAcousticPage(juce::Rectangle<int> area);
    void layoutAdvancedPage(juce::Rectangle<int> area);
    void layoutSpeakerPage(juce::Rectangle<int> area);
    void layoutPresetsPage(juce::Rectangle<int> area);

    void populateReverbPresetBox();
    void populateEqPresetBox();
    void syncEqDisplayFromSliders();

    void timerCallback() override;

    // Draws a section divider with a label
    static void paintSectionHeader(juce::Graphics& g, juce::Rectangle<int> area,
                                   const juce::String& title);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassNukerEditor)
};
