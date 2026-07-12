#pragma once
#include "PresetData.h"
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <string>

// Manages factory reverb presets and user-saved presets.
// User presets are stored in the JUCE appdata directory.
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts)
        : state(apvts)
    {
        factoryPresets = createFactoryPresets();
        eqPresets      = createEqPresets();
    }

    // ── Factory Reverb Presets ──────────────────────────────────────────────
    const std::vector<ReverbPreset>& getFactoryPresets() const { return factoryPresets; }

    const ReverbPreset* findFactoryPreset(const std::string& id) const
    {
        for (const auto& p : factoryPresets)
            if (p.id == id) return &p;
        return nullptr;
    }

    // Returns the factory default preset (Haunted Cavern V2)
    const ReverbPreset& getDefault() const
    {
        const auto* p = findFactoryPreset("hauntedcavernv2");
        return p ? *p : factoryPresets.front();
    }

    // ── EQ Presets ──────────────────────────────────────────────────────────
    const std::vector<EqPreset>& getEqPresets() const { return eqPresets; }

    const EqPreset* findEqPreset(const std::string& id) const
    {
        for (const auto& p : eqPresets)
            if (p.id == id) return &p;
        return nullptr;
    }

    // ── User Preset Persistence ─────────────────────────────────────────────
    // Saves the current APVTS state as a named user preset
    void saveUserPreset(const juce::String& name)
    {
        juce::MemoryBlock block;
        {
            juce::MemoryOutputStream stream(block, false);
            auto xml = state.copyState().createXml();
            xml->setAttribute("presetName", name);
            xml->writeToStream(stream, juce::String());
        }
        const juce::File presetFile = getUserPresetDir().getChildFile(name + ".bnpreset");
        presetFile.replaceWithData(block.getData(), block.getSize());
        refreshUserPresets();
    }

    // Loads a named user preset into the APVTS
    bool loadUserPreset(const juce::String& name)
    {
        const juce::File presetFile = getUserPresetDir().getChildFile(name + ".bnpreset");
        if (!presetFile.existsAsFile()) return false;

        juce::MemoryBlock block;
        presetFile.loadFileAsData(block);
        if (auto xml = juce::parseXML(juce::String::fromUTF8(static_cast<const char*>(block.getData()), static_cast<int>(block.getSize()))))
        {
            const auto tree = juce::ValueTree::fromXml(*xml);
            if (tree.isValid())
            {
                state.replaceState(tree);
                return true;
            }
        }
        return false;
    }

    void deleteUserPreset(const juce::String& name)
    {
        getUserPresetDir().getChildFile(name + ".bnpreset").deleteFile();
        refreshUserPresets();
    }

    const juce::StringArray& getUserPresetNames() const { return userPresetNames; }

    void refreshUserPresets()
    {
        userPresetNames.clear();
        for (const auto& f : getUserPresetDir().findChildFiles(juce::File::findFiles, false, "*.bnpreset"))
            userPresetNames.add(f.getFileNameWithoutExtension());
        userPresetNames.sort(false);
    }

    juce::File getUserPresetDir() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("BassNuker").getChildFile("Presets");
    }

private:
    juce::AudioProcessorValueTreeState& state;
    std::vector<ReverbPreset> factoryPresets;
    std::vector<EqPreset>     eqPresets;
    juce::StringArray         userPresetNames;
};
