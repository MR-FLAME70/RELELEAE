cmake_minimum_required(VERSION 3.22)
project(BassNuker VERSION 6.9.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ──────────────────────────────────────────────────────────────────────
# Clone JUCE next to this project OR set JUCE_PATH to your installation:
#   git clone https://github.com/juce-framework/JUCE.git
# Then configure with:  cmake -DJUCE_PATH=/path/to/JUCE ..
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    # Try a sibling JUCE directory first, then CMAKE_PREFIX_PATH
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE JUCE)
    else()
        find_package(JUCE CONFIG REQUIRED)
    endif()
endif()

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(BassNuker
    COMPANY_NAME            "Bass Nuker"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BaNu
    PLUGIN_CODE             BsNk
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Bass Nuker"
    VERSION                 "6.9.0"
    DESCRIPTION             "Sound of death — browser-grade audio enhancement as a native VST3"
    VST3_CATEGORIES         "Fx" "EQ" "Dynamics"
)

# ── Sources ───────────────────────────────────────────────────────────────────
target_sources(BassNuker PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/FDNReverb.h
    Source/DSP/FDNReverb.cpp
    Source/DSP/HybridReverbEngine.h
    Source/DSP/HybridReverbEngine.cpp
    Source/DSP/AcousticEngine.h
    Source/DSP/AcousticEngine.cpp
    Source/DSP/AdvancedAudioChain.h
    Source/DSP/AdvancedAudioChain.cpp
    Source/DSP/SpeakerConfigEngine.h
    Source/DSP/SpeakerConfigEngine.cpp
    Source/DSP/PitchShifter.h
    Source/DSP/SoftClipper.h
    Source/DSP/DynamicBass.h
    Source/DSP/DynamicBass.cpp
    Source/DSP/StereoWidth.h
    Source/DSP/BiquadFilter.h
    Source/Presets/PresetData.h
    Source/Presets/PresetManager.h
    Source/Presets/PresetManager.cpp
    Source/UI/LookAndFeel.h
    Source/UI/LookAndFeel.cpp
    Source/UI/RotaryKnob.h
    Source/UI/RotaryKnob.cpp
    Source/UI/VUMeter.h
    Source/UI/VUMeter.cpp
    Source/UI/EQDisplay.h
    Source/UI/EQDisplay.cpp
)

# ── JUCE modules ──────────────────────────────────────────────────────────────
target_compile_definitions(BassNuker
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(BassNuker
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
