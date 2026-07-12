#include "PluginEditor.h"
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
static void labelFor(juce::Component& parent, juce::Label& lbl,
                     const juce::String& text, float size = 11.0f)
{
    lbl.setText(text, juce::dontSendNotification);
    lbl.setFont(juce::Font("Segoe UI", size, juce::Font::plain));
    lbl.setJustificationType(juce::Justification::centred);
    lbl.setColour(juce::Label::textColourId, juce::Colour(0xff8899aa));
    parent.addAndMakeVisible(lbl);
}

static void addSlider(juce::Component& parent, SliderWithAttachment& sa,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& paramId,
                      juce::Slider::SliderStyle style = juce::Slider::RotaryVerticalDrag)
{
    sa.attach(apvts, paramId, style);
    parent.addAndMakeVisible(sa.slider);
}

static void addButton(juce::Component& parent, ButtonWithAttachment& ba,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& paramId, const juce::String& text)
{
    ba.button.setButtonText(text);
    ba.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, ba.button);
    parent.addAndMakeVisible(ba.button);
}

// ─────────────────────────────────────────────────────────────────────────────
BassNukerEditor::BassNukerEditor(BassNukerProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p), vuMeter(p)
{
    setLookAndFeel(&laf);
    setSize(860, 640);
    setResizable(true, true);
    setResizeLimits(700, 520, 1400, 900);

    // ── Title ────────────────────────────────────────────────────────────────
    titleLabel.setText("BASS NUKER  v6.9", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("Segoe UI", 18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d4ff));
    addAndMakeVisible(titleLabel);

    // ── Bypass ───────────────────────────────────────────────────────────────
    bypassBtn.button.setButtonText("BYPASS");
    bypassBtn.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "bypass", bypassBtn.button);
    addAndMakeVisible(bypassBtn.button);

    // ── VU Meter ─────────────────────────────────────────────────────────────
    addAndMakeVisible(vuMeter);

    // ── Tab buttons ───────────────────────────────────────────────────────────
    for (auto* btn : { &tabLive, &tabReverb, &tabAcoustic, &tabAdvanced, &tabSpeaker, &tabPresets })
    {
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(1);
        addAndMakeVisible(*btn);
    }
    tabLive.setToggleState(true, juce::dontSendNotification);

    tabLive.onClick     = [this] { switchTab(Tab::Live); };
    tabReverb.onClick   = [this] { switchTab(Tab::Reverb); };
    tabAcoustic.onClick = [this] { switchTab(Tab::AcousticEngine); };
    tabAdvanced.onClick = [this] { switchTab(Tab::AdvancedAudio); };
    tabSpeaker.onClick  = [this] { switchTab(Tab::SpeakerConfig); };
    tabPresets.onClick  = [this] { switchTab(Tab::Presets); };

    // ── Pages ─────────────────────────────────────────────────────────────────
    buildLivePage();
    buildReverbPage();
    buildAcousticPage();
    buildAdvancedPage();
    buildSpeakerPage();
    buildPresetsPage();

    for (auto* pg : { &livePage, &reverbPage, &acousticPage, &advancedPage, &speakerPage, &presetsPage })
        addAndMakeVisible(*pg);

    switchTab(Tab::Live);

    startTimerHz(24);
}

BassNukerEditor::~BassNukerEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerEditor::paint(juce::Graphics& g)
{
    // Background gradient
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1a1a2e), b.getTopLeft(),
        juce::Colour(0xff0d1217), b.getBottomRight(), false));
    g.fillAll();

    // Tab underline
    g.setColour(juce::Colour(0xff2a3a5a));
    g.fillRect(0, 44, getWidth(), 2);
}

void BassNukerEditor::resized()
{
    auto area = getLocalBounds();

    // ── Top bar ───────────────────────────────────────────────────────────────
    auto topBar = area.removeFromTop(46);
    titleLabel.setBounds(topBar.removeFromLeft(200));
    bypassBtn.button.setBounds(topBar.removeFromRight(80).reduced(6, 8));
    vuMeter.setBounds(topBar.removeFromRight(90).reduced(4, 8));

    // ── Tab strip ─────────────────────────────────────────────────────────────
    area.removeFromTop(2); // separator gap
    auto tabRow = area.removeFromTop(36);
    const int tabW = tabRow.getWidth() / 6;
    for (auto* btn : { &tabLive, &tabReverb, &tabAcoustic, &tabAdvanced, &tabSpeaker, &tabPresets })
        btn->setBounds(tabRow.removeFromLeft(tabW).reduced(2, 4));

    // ── Content area ──────────────────────────────────────────────────────────
    area.reduce(8, 6);
    livePage.setBounds(area);
    reverbPage.setBounds(area);
    acousticPage.setBounds(area);
    advancedPage.setBounds(area);
    speakerPage.setBounds(area);
    presetsPage.setBounds(area);

    layoutLivePage(livePage.getBounds());
    layoutReverbPage(reverbPage.getBounds());
    layoutAcousticPage(acousticPage.getBounds());
    layoutAdvancedPage(advancedPage.getBounds());
    layoutSpeakerPage(speakerPage.getBounds());
    layoutPresetsPage(presetsPage.getBounds());
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerEditor::switchTab(Tab t)
{
    activeTab = t;
    livePage.setVisible     (t == Tab::Live);
    reverbPage.setVisible   (t == Tab::Reverb);
    acousticPage.setVisible (t == Tab::AcousticEngine);
    advancedPage.setVisible (t == Tab::AdvancedAudio);
    speakerPage.setVisible  (t == Tab::SpeakerConfig);
    presetsPage.setVisible  (t == Tab::Presets);
    repaint();
}

void BassNukerEditor::timerCallback()
{
    // Sync EQ display from sliders when advanced tab is visible
    if (activeTab == Tab::AdvancedAudio)
        syncEqDisplayFromSliders();
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Live Tab ─────────────────────────────────────────────────────────────────
void BassNukerEditor::buildLivePage()
{
    addButton(livePage, bassOnBtn, proc.apvts, "bassOn", "Bass Boost ON");
    addSlider(livePage, bassFreqSlider, proc.apvts, "bassFreq");
    addSlider(livePage, bassGainSlider, proc.apvts, "bassGain");
    addSlider(livePage, volumeSlider,   proc.apvts, "volume");
    labelFor(livePage, bassFreqLabel, "Bass Freq");
    labelFor(livePage, bassGainLabel, "Bass Gain");
    labelFor(livePage, volumeLabel,   "Volume");
}

void BassNukerEditor::layoutLivePage(juce::Rectangle<int> area)
{
    bassOnBtn.button.setBounds(area.removeFromTop(30).reduced(0, 2));
    area.removeFromTop(10);

    auto row = area.removeFromTop(110);
    const int kW = row.getWidth() / 3;
    auto setBandBlock = [&](SliderWithAttachment& sa, juce::Label& lbl) {
        auto col = row.removeFromLeft(kW);
        sa.slider.setBounds(col.removeFromTop(80).reduced(8, 2));
        lbl.setBounds(col);
    };
    setBandBlock(bassFreqSlider, bassFreqLabel);
    setBandBlock(bassGainSlider, bassGainLabel);
    setBandBlock(volumeSlider,   volumeLabel);
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Reverb Tab ────────────────────────────────────────────────────────────────
void BassNukerEditor::buildReverbPage()
{
    addButton(reverbPage, reverbOnBtn, proc.apvts, "reverbOn", "Reverb ON");
    reverbPage.addAndMakeVisible(reverbPresetBox);
    populateReverbPresetBox();
    reverbPresetBox.onChange = [this] {
        const int idx = reverbPresetBox.getSelectedItemIndex();
        const auto& presets = proc.getPresetManager().getFactoryPresets();
        if (idx >= 0 && idx < static_cast<int>(presets.size()))
            proc.applyReverbPreset(presets[idx].id);
    };

    // Core
    addSlider(reverbPage, reverbMixSlider,      proc.apvts, "reverbMix");
    addSlider(reverbPage, reverbAmountSlider,    proc.apvts, "reverbAmount");
    addSlider(reverbPage, songVolumeSlider,      proc.apvts, "songVolume");
    addSlider(reverbPage, reverbDecaySlider,     proc.apvts, "reverbDecay");
    addSlider(reverbPage, reverbPredelaySlider,  proc.apvts, "reverbPredelay");
    addSlider(reverbPage, reverbDiffuseSlider,   proc.apvts, "reverbDiffuse");
    addSlider(reverbPage, reverbToneSlider,      proc.apvts, "reverbTone");
    // Advanced
    addSlider(reverbPage, reverbResHzSlider,     proc.apvts, "reverbResHz");
    addSlider(reverbPage, reverbResQSlider,      proc.apvts, "reverbResQ");
    addSlider(reverbPage, reverbRoomSlider,      proc.apvts, "reverbRoom");
    addSlider(reverbPage, reverbDensitySlider,   proc.apvts, "reverbDensity");
    addSlider(reverbPage, reverbErLevelSlider,   proc.apvts, "reverbErLevel");
    addSlider(reverbPage, reverbErDelaySlider,   proc.apvts, "reverbErDelay");
    addSlider(reverbPage, reverbLateSlider,      proc.apvts, "reverbLate");
    addSlider(reverbPage, reverbHfDampSlider,    proc.apvts, "reverbHfDamp");
    addSlider(reverbPage, reverbLfDampSlider,    proc.apvts, "reverbLfDamp");
    addSlider(reverbPage, reverbHighCutSlider,   proc.apvts, "reverbHighCut");
    addSlider(reverbPage, reverbLowCutSlider,    proc.apvts, "reverbLowCut");
    addSlider(reverbPage, reverbWidthSlider,     proc.apvts, "reverbWidth");
    addSlider(reverbPage, reverbModDepthSlider,  proc.apvts, "reverbModDepth");
    addSlider(reverbPage, reverbModRateSlider,   proc.apvts, "reverbModRate");
    addSlider(reverbPage, reverbWetLvlSlider,    proc.apvts, "reverbWetLvl");
    addSlider(reverbPage, reverbDryLvlSlider,    proc.apvts, "reverbDryLvl");
}

void BassNukerEditor::layoutReverbPage(juce::Rectangle<int> area)
{
    reverbOnBtn.button.setBounds(area.removeFromTop(28).reduced(0, 2));
    area.removeFromTop(4);
    reverbPresetBox.setBounds(area.removeFromTop(26));
    area.removeFromTop(6);

    // Row 1: core controls
    auto placeRow = [&](std::vector<SliderWithAttachment*> sliders) {
        auto row = area.removeFromTop(90);
        const int w = row.getWidth() / static_cast<int>(sliders.size());
        for (auto* s : sliders)
            s->slider.setBounds(row.removeFromLeft(w).reduced(4, 2));
    };

    placeRow({ &reverbMixSlider, &reverbAmountSlider, &songVolumeSlider,
               &reverbDecaySlider, &reverbPredelaySlider, &reverbDiffuseSlider, &reverbToneSlider });
    area.removeFromTop(4);
    placeRow({ &reverbResHzSlider, &reverbResQSlider, &reverbRoomSlider, &reverbDensitySlider,
               &reverbErLevelSlider, &reverbErDelaySlider });
    area.removeFromTop(4);
    placeRow({ &reverbLateSlider, &reverbHfDampSlider, &reverbLfDampSlider,
               &reverbHighCutSlider, &reverbLowCutSlider });
    area.removeFromTop(4);
    placeRow({ &reverbWidthSlider, &reverbModDepthSlider, &reverbModRateSlider,
               &reverbWetLvlSlider, &reverbDryLvlSlider });
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Acoustic Engine Tab ───────────────────────────────────────────────────────
void BassNukerEditor::buildAcousticPage()
{
    addButton(acousticPage, aeOnBtn, proc.apvts, "aeOn", "Acoustic Engine ON");
    addSlider(acousticPage, aeSurroundSlider,  proc.apvts, "aeSurround");
    addSlider(acousticPage, aeCrystalSlider,   proc.apvts, "aeCrystal");
    addSlider(acousticPage, aeBassSlider,      proc.apvts, "aeBass");
    addSlider(acousticPage, aeCrossoverSlider, proc.apvts, "aeCrossover");
    addSlider(acousticPage, aeSmartVolSlider,  proc.apvts, "aeSmartVol");
    addSlider(acousticPage, aeDialogSlider,    proc.apvts, "aeDialog");
}

void BassNukerEditor::layoutAcousticPage(juce::Rectangle<int> area)
{
    aeOnBtn.button.setBounds(area.removeFromTop(28).reduced(0, 2));
    area.removeFromTop(16);
    auto row = area.removeFromTop(110);
    const int kW = row.getWidth() / 6;
    for (auto* s : { &aeSurroundSlider, &aeCrystalSlider, &aeBassSlider,
                     &aeSmartVolSlider, &aeDialogSlider, &aeCrossoverSlider })
        s->slider.setBounds(row.removeFromLeft(kW).reduced(6, 2));
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Advanced Audio Tab ────────────────────────────────────────────────────────
void BassNukerEditor::buildAdvancedPage()
{
    // EQ
    addButton(advancedPage, eqOnBtn, proc.apvts, "eqOn", "EQ ON");
    advancedPage.addAndMakeVisible(eqPresetBox);
    populateEqPresetBox();
    eqPresetBox.onChange = [this] {
        const auto& presets = proc.getPresetManager().getEqPresets();
        const int idx = eqPresetBox.getSelectedItemIndex();
        if (idx >= 0 && idx < static_cast<int>(presets.size()))
            proc.applyEqPreset(presets[idx].id);
    };
    advancedPage.addAndMakeVisible(eqDisplay);
    for (int b = 0; b < 10; b++)
    {
        addSlider(advancedPage, eqBandSliders[b], proc.apvts,
                  juce::String("eqBand") + juce::String(b),
                  juce::Slider::LinearVertical);
    }

    // Compressor
    addButton(advancedPage, compOnBtn, proc.apvts, "compOn", "Comp");
    addSlider(advancedPage, compThreshSlider,  proc.apvts, "compThresh");
    addSlider(advancedPage, compRatioSlider,   proc.apvts, "compRatio");
    addSlider(advancedPage, compAttackSlider,  proc.apvts, "compAttack");
    addSlider(advancedPage, compReleaseSlider, proc.apvts, "compRelease");
    addSlider(advancedPage, compMakeupSlider,  proc.apvts, "compMakeup");

    // Limiter
    addButton(advancedPage, limOnBtn, proc.apvts, "limOn", "Lim");
    addSlider(advancedPage, limThreshSlider,  proc.apvts, "limThresh");
    addSlider(advancedPage, limReleaseSlider, proc.apvts, "limRelease");

    // Width
    addButton(advancedPage, widthOnBtn, proc.apvts, "widthOn", "Width");
    addSlider(advancedPage, widthSlider, proc.apvts, "width");

    // Pitch
    addButton(advancedPage, pitchOnBtn, proc.apvts, "pitchOn", "Pitch");
    addSlider(advancedPage, pitchSlider, proc.apvts, "pitch");

    // Dynamic Bass
    addButton(advancedPage, dynBassOnBtn, proc.apvts, "dynBassOn", "DynBass");
    addSlider(advancedPage, dynBassSensSlider, proc.apvts, "dynBassSens");
    addSlider(advancedPage, dynBassStrSlider,  proc.apvts, "dynBassStr");

    // Spectrum
    addButton(advancedPage, spectrumOnBtn, proc.apvts, "spectrumOn", "Spectrum");
}

void BassNukerEditor::layoutAdvancedPage(juce::Rectangle<int> area)
{
    // EQ section at top
    auto eqToggleRow = area.removeFromTop(24);
    eqOnBtn.button.setBounds(eqToggleRow.removeFromLeft(80));
    eqPresetBox.setBounds(eqToggleRow.removeFromLeft(160).reduced(4, 2));

    // EQ bands + display
    auto eqArea = area.removeFromTop(160);
    eqDisplay.setBounds(eqArea.removeFromRight(eqArea.getWidth() / 2).reduced(2));
    const int bandW = eqArea.getWidth() / 10;
    for (int b = 0; b < 10; b++)
        eqBandSliders[b].slider.setBounds(eqArea.removeFromLeft(bandW).reduced(1, 4));

    area.removeFromTop(8);

    // Bottom row: Comp | Lim | Width | Pitch | DynBass
    auto bottomRow = area;
    auto placeSection = [&](juce::Rectangle<int>& row, ButtonWithAttachment& btn,
                             std::vector<SliderWithAttachment*> sls) {
        const int secW = row.getWidth() / 5;
        auto sec = row.removeFromLeft(secW);
        btn.button.setBounds(sec.removeFromTop(22));
        const int sw = sec.getWidth() / static_cast<int>(std::max<int>(1, sls.size()));
        for (auto* sl : sls)
            sl->slider.setBounds(sec.removeFromLeft(sw).reduced(2, 2));
    };

    placeSection(bottomRow, compOnBtn,  { &compThreshSlider, &compRatioSlider, &compAttackSlider, &compReleaseSlider, &compMakeupSlider });
    placeSection(bottomRow, limOnBtn,   { &limThreshSlider, &limReleaseSlider });
    placeSection(bottomRow, widthOnBtn, { &widthSlider });
    placeSection(bottomRow, pitchOnBtn, { &pitchSlider });
    placeSection(bottomRow, dynBassOnBtn, { &dynBassSensSlider, &dynBassStrSlider });
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Speaker Config Tab ────────────────────────────────────────────────────────
void BassNukerEditor::buildSpeakerPage()
{
    addButton(speakerPage, speakerOnBtn, proc.apvts, "speakerOn", "Speaker Config ON");
    speakerPage.addAndMakeVisible(speakerModeBox);
    speakerModeBox.addItem("Stereo",     1);
    speakerModeBox.addItem("Headphones", 2);
    speakerModeBox.addItem("2.1",        3);
    speakerModeBox.addItem("4.0",        4);
    speakerModeBox.addItem("4.1",        5);
    speakerModeBox.addItem("5.1",        6);
    speakerModeBox.addItem("7.1",        7);
    speakerModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "speakerMode", speakerModeBox);

    addSlider(speakerPage, spkFrontWSlider,   proc.apvts, "spkFrontW");
    addSlider(speakerPage, spkRearWSlider,    proc.apvts, "spkRearW");
    addSlider(speakerPage, spkCtrDistSlider,  proc.apvts, "spkCtrDist");
    addSlider(speakerPage, spkRearDistSlider, proc.apvts, "spkRearDist");
    addSlider(speakerPage, spkSubDistSlider,  proc.apvts, "spkSubDist");
    addSlider(speakerPage, spkLvlFLSlider,    proc.apvts, "spkLvlFL");
    addSlider(speakerPage, spkLvlFRSlider,    proc.apvts, "spkLvlFR");
    addSlider(speakerPage, spkLvlCSlider,     proc.apvts, "spkLvlC");
    addSlider(speakerPage, spkLvlSubSlider,   proc.apvts, "spkLvlSub");
    addSlider(speakerPage, spkLvlRLSlider,    proc.apvts, "spkLvlRL");
    addSlider(speakerPage, spkLvlRRSlider,    proc.apvts, "spkLvlRR");
}

void BassNukerEditor::layoutSpeakerPage(juce::Rectangle<int> area)
{
    speakerOnBtn.button.setBounds(area.removeFromTop(26));
    area.removeFromTop(4);
    speakerModeBox.setBounds(area.removeFromTop(24));
    area.removeFromTop(10);

    auto row1 = area.removeFromTop(100);
    const int kW5 = row1.getWidth() / 5;
    spkFrontWSlider.slider.setBounds(  row1.removeFromLeft(kW5).reduced(4, 2));
    spkRearWSlider.slider.setBounds(   row1.removeFromLeft(kW5).reduced(4, 2));
    spkCtrDistSlider.slider.setBounds( row1.removeFromLeft(kW5).reduced(4, 2));
    spkRearDistSlider.slider.setBounds(row1.removeFromLeft(kW5).reduced(4, 2));
    spkSubDistSlider.slider.setBounds( row1.reduced(4, 2));

    area.removeFromTop(8);

    auto row2 = area.removeFromTop(100);
    const int kW6 = row2.getWidth() / 6;
    spkLvlFLSlider.slider.setBounds( row2.removeFromLeft(kW6).reduced(4, 2));
    spkLvlFRSlider.slider.setBounds( row2.removeFromLeft(kW6).reduced(4, 2));
    spkLvlCSlider.slider.setBounds(  row2.removeFromLeft(kW6).reduced(4, 2));
    spkLvlSubSlider.slider.setBounds(row2.removeFromLeft(kW6).reduced(4, 2));
    spkLvlRLSlider.slider.setBounds( row2.removeFromLeft(kW6).reduced(4, 2));
    spkLvlRRSlider.slider.setBounds( row2.reduced(4, 2));
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Presets Tab ───────────────────────────────────────────────────────────────
void BassNukerEditor::buildPresetsPage()
{
    presetsPage.addAndMakeVisible(userPresetList);
    presetsPage.addAndMakeVisible(presetNameEditor);
    presetsPage.addAndMakeVisible(savePresetBtn);
    presetsPage.addAndMakeVisible(loadPresetBtn);
    presetsPage.addAndMakeVisible(deletePresetBtn);

    presetNameEditor.setTextToShowWhenEmpty("Preset name...", juce::Colour(0xff445566));
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId,    juce::Colour(0xff0d1520));
    presetNameEditor.setColour(juce::TextEditor::outlineColourId,       juce::Colour(0xff2a3a5a));
    presetNameEditor.setColour(juce::TextEditor::focusedOutlineColourId,juce::Colour(0xff00d4ff));
    presetNameEditor.setColour(juce::TextEditor::textColourId,          juce::Colour(0xfff0f0f0));

    savePresetBtn.onClick = [this] {
        const auto name = presetNameEditor.getText().trim();
        if (name.isNotEmpty())
        {
            proc.getPresetManager().saveUserPreset(name);
            userPresetList.updateContent();
        }
    };

    loadPresetBtn.onClick = [this] {
        const int row = userPresetList.getSelectedRow();
        const auto& names = proc.getPresetManager().getUserPresetNames();
        if (row >= 0 && row < names.size())
            proc.getPresetManager().loadUserPreset(names[row]);
    };

    deletePresetBtn.onClick = [this] {
        const int row = userPresetList.getSelectedRow();
        const auto& names = proc.getPresetManager().getUserPresetNames();
        if (row >= 0 && row < names.size())
        {
            proc.getPresetManager().deleteUserPreset(names[row]);
            userPresetList.updateContent();
        }
    };
}

void BassNukerEditor::layoutPresetsPage(juce::Rectangle<int> area)
{
    auto controls = area.removeFromBottom(80);
    userPresetList.setBounds(area.reduced(0, 4));

    controls.reduce(8, 8);
    presetNameEditor.setBounds(controls.removeFromTop(26));
    controls.removeFromTop(6);
    auto btnRow = controls;
    const int bw = btnRow.getWidth() / 3;
    savePresetBtn.setBounds(  btnRow.removeFromLeft(bw).reduced(4, 0));
    loadPresetBtn.setBounds(  btnRow.removeFromLeft(bw).reduced(4, 0));
    deletePresetBtn.setBounds(btnRow.reduced(4, 0));
}

// ─────────────────────────────────────────────────────────────────────────────
void BassNukerEditor::populateReverbPresetBox()
{
    reverbPresetBox.clear(juce::dontSendNotification);
    int i = 1;
    for (const auto& p : proc.getPresetManager().getFactoryPresets())
        reverbPresetBox.addItem(p.name, i++);
    // Select Haunted Cavern V2 as default (index 1 in our list = index 2 = the 2nd item)
    reverbPresetBox.setSelectedItemIndex(1, juce::dontSendNotification);
}

void BassNukerEditor::populateEqPresetBox()
{
    eqPresetBox.clear(juce::dontSendNotification);
    int i = 1;
    for (const auto& p : proc.getPresetManager().getEqPresets())
        eqPresetBox.addItem(p.name, i++);
    eqPresetBox.setSelectedItemIndex(0, juce::dontSendNotification);
}

void BassNukerEditor::syncEqDisplayFromSliders()
{
    for (int b = 0; b < 10; b++)
        eqDisplay.setBandGain(b, static_cast<float>(eqBandSliders[b].slider.getValue()));
}

void BassNukerEditor::paintSectionHeader(juce::Graphics& g, juce::Rectangle<int> area,
                                          const juce::String& title)
{
    g.setColour(juce::Colour(0xff2a3a5a));
    g.fillRect(area.removeFromBottom(1));
    g.setColour(juce::Colour(0xff00d4ff));
    g.setFont(juce::Font("Segoe UI", 11.0f, juce::Font::bold));
    g.drawText(title, area, juce::Justification::centredLeft, false);
}
