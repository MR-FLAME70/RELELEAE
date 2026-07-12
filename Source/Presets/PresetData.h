#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────────────────────
// All reverb preset data — ported from EAX_PRESETS in popup.js.
// Fields map 1:1 to the hybrid reverb engine parameters.
// ─────────────────────────────────────────────────────────────────────────────
struct ReverbPreset
{
    std::string id;
    std::string name;

    // Basic reverb
    double decay      = 1.8;   // seconds
    double mix        = 35.0;  // %  (wet level relative to dry)
    double predelay   = 7.0;   // ms
    double diffuse    = 70.0;  // %
    double toneHz     = 9000.0;// high-cut Hz on wet bus
    double amount     = 100.0; // Effects Amount %

    // Resonance (peaking filter on wet path)
    double resonanceHz= 1000.0;
    double resonanceQ = 0.0;

    // Hybrid engine parameters
    double roomSize            = 1.0;
    double earlyReflectionDelay= 0.0;  // ms
    double earlyReflectionLevel= 35.0; // %
    double lateReverbLevel     = 100.0;// %
    double hfDamping           = 0.0;  // %
    double lfDamping           = 0.0;  // %
    double stereoWidth         = 100.0;// %
    double modulationDepth     = 40.0; // %
    double modulationRate      = 35.0; // %
    double lowCut              = 80.0; // Hz
    double density             = 70.0; // %
    double wetLevel            = 100.0;// %
    double dryLevel            = 0.0;  // %
};

// Helper: fill in defaults for presets that don't specify every field
static void applyPresetDefaults(ReverbPreset& p)
{
    if (p.predelay  == 0.0) p.predelay = std::min(45.0, std::round(p.decay * 8.0));
    if (p.diffuse   == 0.0) p.diffuse  = std::min(85.0, std::round(35.0 + p.decay * 6.0));
    if (p.density   == 0.0) p.density  = p.diffuse;
}

// Factory presets — exact values from popup.js EAX_PRESETS
// The presets are listed in UI display order.
static std::vector<ReverbPreset> createFactoryPresets()
{
    std::vector<ReverbPreset> presets;

    // ── Haunted Cavern V1 ──────────────────────────────────────────────────
    presets.push_back({
        "hauntedcavernv1", "Haunted Cavern V1",
        5.6, 68.0, 22.0, 88.0, 2600.0, 100.0,
        750.0, 5.0,
        1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 88.0, 100.0, 0.0
    });

    // ── Haunted Cavern V2 ─────────────────────────────────────────────────
    // Factory default preset — fully tuned for SB0490 character
    presets.push_back({
        "hauntedcavernv2", "Haunted Cavern V2",
        9.0, 74.0, 38.0, 78.0, 3600.0, 100.0,
        1000.0, 0.0,
        2.6, 0.0, 38.0, 100.0, 35.0, 15.0, 165.0, 55.0, 30.0, 90.0, 78.0, 100.0, 0.0
    });

    // ── Haunted Cavern V3 ──────────────────────────────────────────────────
    // Lighter, vocal-forward sibling; designed for ~15% Effects Amount
    presets.push_back({
        "hauntedcavernv3", "Haunted Cavern V3",
        5.5, 15.0, 28.0, 90.0, 3600.0, 100.0,
        1000.0, 0.0,
        2.7, 0.0, 280.0, 667.0, 30.0, 12.0, 150.0, 50.0, 25.0, 85.0, 90.0, 100.0, 0.0
    });

    // ── Concert Hall ──────────────────────────────────────────────────────
    presets.push_back({
        "concerthall", "Concert Hall",
        3.92, 45.0, 20.0, 90.0, 8000.0, 100.0,
        1000.0, 0.0,
        1.70, 0.0, 24.0, 100.0, 18.0, 0.0, 130.0, 45.0, 30.0, 80.0, 90.0, 100.0, 0.0
    });

    // ── Concert Hall V2 ───────────────────────────────────────────────────
    presets.push_back({
        "concerthallv2", "Concert Hall V2",
        5.6, 58.0, 32.0, 88.0, 6200.0, 100.0,
        1000.0, 0.0,
        2.15, 4.0, 30.0, 100.0, 32.0, 8.0, 155.0, 40.0, 28.0, 90.0, 92.0, 100.0, 0.0
    });

    // ── Amphitheater ─────────────────────────────────────────────────────
    presets.push_back({
        "amphitheater", "Amphitheater",
        4.6, 42.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── Orchestra Pit ────────────────────────────────────────────────────
    presets.push_back({
        "orchestrapit", "Orchestra Pit",
        1.8, 34.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── Stone Hall ───────────────────────────────────────────────────────
    presets.push_back({
        "stonehall", "Stone Hall",
        3.3, 50.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── Jazz Club ────────────────────────────────────────────────────────
    presets.push_back({
        "jazzclub", "Jazz Club",
        1.1, 28.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── Recital Hall ─────────────────────────────────────────────────────
    presets.push_back({
        "recitalhall", "Recital Hall",
        2.6, 38.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── Theater ──────────────────────────────────────────────────────────
    presets.push_back({
        "theater", "Theater",
        1.7, 33.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── Opera Hall ───────────────────────────────────────────────────────
    presets.push_back({
        "operahall", "Opera Hall",
        3.6, 45.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── Royal Hall ───────────────────────────────────────────────────────
    presets.push_back({
        "royalhall", "Royal Hall",
        4.2, 48.0, 0.0, 0.0, 0.0, 100.0, 1000.0, 0.0, 1.0, 0.0, 35.0, 100.0, 0.0, 0.0, 100.0, 40.0, 35.0, 80.0, 0.0, 100.0, 0.0
    });

    // ── EAX Standard Rooms ───────────────────────────────────────────────
    presets.push_back({
        "generic", "Generic",
        1.49, 35.0, 7.0, 90.0, 9000.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 5.0, 100.0, 10.0, 0.0, 100.0, 35.0, 35.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "paddedcell", "Padded Cell",
        0.17, 20.0, 1.0, 90.0, 1500.0, 100.0, 1000.0, 0.0,
        0.35, 0.0, 25.0, 100.0, 54.0, 15.0, 80.0, 15.0, 20.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "room", "Room",
        0.40, 25.0, 2.0, 90.0, 8200.0, 100.0, 1000.0, 0.0,
        0.40, 0.0, 15.0, 100.0, 10.0, 0.0, 90.0, 20.0, 25.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "bathroom", "Bathroom",
        1.49, 55.0, 7.0, 90.0, 6000.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 65.0, 100.0, 28.0, 0.0, 85.0, 30.0, 30.0, 80.0, 60.0, 100.0, 0.0
    });
    presets.push_back({
        "livingroom", "Living Room",
        0.50, 20.0, 3.0, 90.0, 1500.0, 100.0, 1000.0, 0.0,
        0.44, 0.0, 21.0, 100.0, 54.0, 10.0, 85.0, 20.0, 20.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "stoneroom", "Stone Room",
        2.31, 40.0, 12.0, 90.0, 8600.0, 100.0, 1000.0, 0.0,
        1.10, 0.0, 44.0, 100.0, 22.0, 0.0, 105.0, 40.0, 35.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "auditorium", "Auditorium",
        4.32, 40.0, 20.0, 90.0, 8100.0, 100.0, 1000.0, 0.0,
        1.85, 0.0, 40.0, 100.0, 25.0, 0.0, 120.0, 45.0, 30.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "cave", "Cave",
        2.91, 45.0, 15.0, 90.0, 9000.0, 100.0, 1000.0, 0.0,
        1.35, 0.0, 50.0, 100.0, 0.0, 0.0, 110.0, 45.0, 30.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "arena", "Arena",
        7.24, 45.0, 20.0, 90.0, 7200.0, 100.0, 1000.0, 0.0,
        2.90, 0.0, 26.0, 100.0, 40.0, 0.0, 145.0, 50.0, 30.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "hangar", "Hangar",
        10.05, 45.0, 20.0, 90.0, 6400.0, 100.0, 1000.0, 0.0,
        3.0, 0.0, 50.0, 100.0, 46.0, 10.0, 160.0, 55.0, 30.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "carpetedhallway", "Carpeted Hallway",
        0.30, 15.0, 2.0, 90.0, 2500.0, 100.0, 1000.0, 0.0,
        0.36, 0.0, 12.0, 100.0, 54.0, 15.0, 80.0, 15.0, 20.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "hallway", "Hallway",
        1.49, 25.0, 7.0, 90.0, 8600.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 25.0, 100.0, 25.0, 0.0, 95.0, 35.0, 35.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "stonecorridor", "Stone Corridor",
        2.70, 30.0, 13.0, 90.0, 8800.0, 100.0, 1000.0, 0.0,
        1.25, 0.0, 25.0, 100.0, 13.0, 0.0, 100.0, 40.0, 35.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "alley", "Alley",
        1.49, 25.0, 7.0, 90.0, 8700.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 25.0, 100.0, 8.0, 0.0, 105.0, 35.0, 35.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "forest", "Forest",
        1.49, 20.0, 45.0, 79.0, 3000.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 5.0, 80.0, 28.0, 0.0, 130.0, 50.0, 20.0, 60.0, 79.0, 100.0, 0.0
    });
    presets.push_back({
        "city", "City",
        1.49, 15.0, 7.0, 50.0, 6900.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 7.0, 70.0, 20.0, 0.0, 120.0, 35.0, 25.0, 75.0, 50.0, 100.0, 0.0
    });
    presets.push_back({
        "mountains", "Mountains",
        1.49, 12.0, 45.0, 27.0, 4000.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 4.0, 60.0, 47.0, 0.0, 160.0, 55.0, 15.0, 50.0, 27.0, 100.0, 0.0
    });
    presets.push_back({
        "quarry", "Quarry",
        1.49, 30.0, 45.0, 90.0, 6400.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 0.0, 100.0, 10.0, 0.0, 120.0, 40.0, 30.0, 70.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "plain", "Plain",
        1.49, 10.0, 45.0, 21.0, 4800.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 6.0, 55.0, 30.0, 0.0, 150.0, 50.0, 15.0, 50.0, 21.0, 100.0, 0.0
    });
    presets.push_back({
        "parkinglot", "Parking Lot",
        1.65, 20.0, 8.0, 90.0, 9000.0, 100.0, 1000.0, 0.0,
        0.86, 0.0, 21.0, 85.0, 0.0, 0.0, 115.0, 35.0, 30.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "sewerpipe", "Sewer Pipe",
        2.81, 60.0, 14.0, 80.0, 6400.0, 100.0, 1000.0, 0.0,
        1.29, 0.0, 100.0, 100.0, 51.0, 5.0, 75.0, 30.0, 40.0, 100.0, 60.0, 100.0, 0.0
    });
    presets.push_back({
        "underwater", "Underwater",
        1.49, 65.0, 7.0, 90.0, 2500.0, 100.0, 1000.0, 0.0,
        0.80, 0.0, 60.0, 100.0, 54.0, 20.0, 85.0, 60.0, 20.0, 100.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "drugged", "Drugged",
        8.39, 50.0, 2.0, 90.0, 9000.0, 100.0, 1000.0, 0.0,
        3.0, 0.0, 88.0, 100.0, 0.0, 0.0, 130.0, 75.0, 15.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "dizzy", "Dizzy",
        17.23, 50.0, 20.0, 90.0, 8400.0, 100.0, 1000.0, 0.0,
        3.0, 0.0, 14.0, 100.0, 26.0, 0.0, 145.0, 70.0, 20.0, 80.0, 90.0, 100.0, 0.0
    });
    presets.push_back({
        "psychotic", "Psychotic",
        7.56, 55.0, 20.0, 90.0, 9000.0, 100.0, 1000.0, 0.0,
        3.0, 0.0, 49.0, 100.0, 5.0, 0.0, 140.0, 65.0, 40.0, 80.0, 90.0, 100.0, 0.0
    });

    // Apply defaults for presets with missing fields
    for (auto& p : presets)
    {
        if (p.predelay == 0.0 && p.id.find("haunted") == std::string::npos)
            p.predelay = std::min(45.0, std::round(p.decay * 8.0));
        if (p.diffuse == 0.0 && p.id.find("haunted") == std::string::npos)
            p.diffuse  = std::min(85.0, std::round(35.0 + p.decay * 6.0));
        if (p.density == 0.0)
            p.density = p.diffuse;
    }

    return presets;
}

// EQ genre presets (10 bands: 31,62,125,250,500,1k,2k,4k,8k,16k Hz)
struct EqPreset { std::string id, name; float bands[10]; };

static std::vector<EqPreset> createEqPresets()
{
    return {
        { "flat",      "Flat",       { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0 } },
        { "bassboost", "Bass Boost", { 6,  5,  4,  2,  0,  0,  0,  0,  0,  0 } },
        { "vocal",     "Vocal",      { -2, -1,  0,  2,  4,  4,  2,  1,  0, -1 } },
        { "rock",      "Rock",       { 4,  3,  2,  0, -1, -1,  0,  2,  3,  4 } },
        { "pop",       "Pop",        { -1,  0,  2,  4,  3,  0, -1,  0,  1,  2 } },
        { "edm",       "EDM",        { 5,  4,  1, -2,  0,  1,  2,  3,  4,  5 } },
        { "cinema",    "Cinema",     { 2,  1,  0,  0,  0,  2,  4,  4,  3,  1 } },
        { "gaming",    "Gaming",     { 3,  2,  1,  0,  0,  1,  2,  3,  3,  2 } },
    };
}
