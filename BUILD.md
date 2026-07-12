# Bass Nuker VST3 — Build Instructions

## Requirements

| Tool | Version | Notes |
|------|---------|-------|
| CMake | ≥ 3.22 | https://cmake.org/download/ |
| JUCE | ≥ 7.0 | https://github.com/juce-framework/JUCE |
| Visual Studio | 2022 (any edition) | Community edition is free |
| Windows SDK | 10.0.19041+ | Installed with VS |

The plugin targets **Windows x64** as a **VST3** (+ optional Standalone).

---

## Quick start

### 1 — Clone JUCE (once)
Place JUCE as a sibling of this project folder **or** pass `-DJUCE_PATH=` to CMake:

```sh
# from the parent directory of BassNuker-VST3/
git clone https://github.com/juce-framework/JUCE.git
```

Directory layout expected by the default CMake config:
```
parent/
├── JUCE/             ← JUCE repository
└── BassNuker-VST3/   ← this project
```

Alternatively, point CMake at any location:
```sh
cmake .. -DJUCE_PATH=C:/JUCE
```

### 2 — Configure

Open a **Developer Command Prompt for VS 2022** (or any terminal with MSVC on PATH), then:

```sh
cd BassNuker-VST3
cmake -B build -G "Visual Studio 17 2022" -A x64
```

Or with Ninja (faster builds):
```sh
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release
```

### 3 — Build

**Release build (recommended):**
```sh
cmake --build build --config Release
```

**Debug build:**
```sh
cmake --build build --config Debug
```

### 4 — Output files

After a successful Release build, find the plugin artefacts at:

```
build/BassNuker_artefacts/Release/VST3/BassNuker.vst3/
build/BassNuker_artefacts/Release/Standalone/BassNuker.exe
```

### 5 — Install the VST3

Copy `BassNuker.vst3` to your DAW's VST3 scan path:

| Path | Notes |
|------|-------|
| `C:\Program Files\Common Files\VST3\` | System-wide (all users) |
| `%APPDATA%\VST3\` | Current user only |

Restart your DAW, rescan plugins, and search for "Bass Nuker".

---

## Project structure

```
BassNuker-VST3/
├── CMakeLists.txt                 — JUCE plugin definition
├── BUILD.md                       — this file
└── Source/
    ├── PluginProcessor.h/.cpp     — AudioProcessor, APVTS, DSP chain wiring
    ├── PluginEditor.h/.cpp        — GUI (tabbed panel, 860×640, resizable)
    ├── DSP/
    │   ├── BiquadFilter.h         — Direct Form II Transposed biquad (custom)
    │   ├── FDNReverb.h/.cpp       — 8-line FDN late tail (Householder matrix)
    │   ├── HybridReverbEngine.h   — Pre-delay + ER convolution + FDN late tail
    │   ├── AcousticEngine.h       — SBX Pro Studio effects (5 modules)
    │   ├── AdvancedAudioChain.h   — EQ/Comp/Lim/Width/Pitch/DynBass chain
    │   ├── SpeakerConfigEngine.h  — Virtual surround decode (7 modes)
    │   ├── PitchShifter.h         — Granular OLA pitch shifter
    │   ├── DynamicBass.h          — Envelope-driven additive bass enhancer
    │   ├── StereoWidth.h          — M/S stereo width (0–200%)
    │   └── SoftClipper.h          — Tanh soft-knee output limiter (threshold 0.8)
    ├── Presets/
    │   ├── PresetData.h           — All 30+ factory reverb presets + EQ presets
    │   └── PresetManager.h        — Preset load/save/delete (user presets)
    └── UI/
        ├── LookAndFeel.h          — Custom dark-navy/cyan JUCE LookAndFeel V4
        ├── RotaryKnob.h           — Thin Slider wrapper for rotary layout
        ├── VUMeter.h              — Stereo RMS + peak-hold clip indicator
        └── EQDisplay.h            — Live magnitude response curve display
```

---

## DSP signal chain (matches browser extension exactly)

```
Input
  → Bass Boost lowshelf  (on/off, freq 40–1000 Hz, gain 0–24 dB)
  → Volume gain          (0–600%)
  → Hybrid Reverb Engine
      → Pre-delay        (0–1500 ms)
      → Early Reflections (synthesized cave IR, 7 taps/channel)
      → FDN late tail    (8-line Householder, per-line LFO + allpass + RT60)
      → HF/LF damping shelves, high-cut, low-cut, M/S stereo width
      → Resonance peaking filter (Haunted Cavern V1)
      → Outer mix: wet = (mix% × amount%) × reverbOn; dry = songVolume%
  → Acoustic Engine      (on/off)
      → Bass lowshelf (SBX Bass, crossover Hz)
      → Crystalizer highshelf + tanh waveshaper
      → Dialog Plus peaking 2800 Hz
      → Smart Volume feed-forward compressor
      → Surround M/S matrix
  → Advanced Audio Chain (each module independently bypassable)
      → 10-Band EQ   (31/62/125/250/500/1k/2k/4k/8k/16k Hz, Q=1.4, ±12 dB)
      → Dynamic Bass (lowpass 150 Hz + compressor, additive)
      → Compressor   (threshold/ratio/attack/release/makeup)
      → Limiter      (ratio 20:1, threshold/release)
      → Stereo Width (M/S, 0–200%)
      → Pitch Shift  (granular OLA, grain 4096, ±12 semitones)
  → Speaker Config Engine (on/off)
      → Stereo (passthrough) / Headphones / 2.1 / 4.0 / 4.1 / 5.1 / 7.1
      → Virtual Speaker Shifter (front/rear width, distances, per-channel levels)
  → Soft tanh clipper    (threshold 0.8 — matches clamp-worklet.js exactly)
  → Output
```

---

## Factory reverb presets (30 total, default = Haunted Cavern V2)

Haunted Cavern V1 / V2 / V3, Concert Hall, Concert Hall V2, Amphitheater,
Orchestra Pit, Stone Hall, Jazz Club, Recital Hall, Theater, Opera Hall,
Royal Hall, Generic, Padded Cell, Room, Bathroom, Living Room, Stone Room,
Auditorium, Cave, Arena, Hangar, Carpeted Hallway, Hallway, Stone Corridor,
Alley, Forest, City, Mountains, Quarry, Plain, Parking Lot, Sewer Pipe,
Underwater, Drugged, Dizzy, Psychotic.

---

## Notes on browser-extension features not in the VST3

| Feature | Reason omitted |
|---------|---------------|
| **Playback speed** | `HTMLMediaElement.playbackRate` is browser-only; no VST3 equivalent |
| **Tab audio capture** | Chrome Web Audio API feature; not applicable to a DAW plugin |
| **Audio recording** | Recording is the DAW's responsibility in a VST3 context |
| **Live/offscreen mode** | Single-process VST3 model handles this natively |
| **Chrome storage sync** | Replaced by JUCE `AudioProcessorValueTreeState` (DAW preset recall) |

---

## Troubleshooting

**"JUCE not found"** → Run CMake with `-DJUCE_PATH=/absolute/path/to/JUCE`.

**"Cannot open include file: 'juce_audio_processors/...'"** → JUCE is not configured
  correctly. Ensure `add_subdirectory(JUCE)` runs before the plugin target.

**Plugin not appearing in DAW** → Check the VST3 install path; some DAWs only scan
  `Program Files\Common Files\VST3`. Also verify the DAW is 64-bit.

**Clicking/popping on preset change** → This is expected when rapidly switching
  presets during playback. Load presets on stopped transport for a clean switch.

**Long reverb tail on stop** → Intentional. The FDN RT60 at maximum decay (25s) will
  ring briefly after the host stops the transport; this matches the extension behaviour.
