#include "sounds.hpp"

#include <string>
#include <vector>

namespace realm::app {

namespace {

// helpers

using gamelib::audio::chord;
using gamelib::audio::Chord;
using gamelib::audio::chordPattern;
using gamelib::audio::noise;
using gamelib::audio::note;
using gamelib::audio::Note;
using gamelib::audio::Song;
using gamelib::audio::SoundRecipe;
using gamelib::audio::tone;
using gamelib::audio::Track;
using gamelib::audio::Wave;

Note rest(const float seconds)
{
    return tone(Wave::Pulse50, note("C4"), seconds, 0.0f, 0.0f);
}

std::string times(const std::string& pattern, const int n)
{
    std::string out;
    for (int i = 0; i < n; ++i) {
        out += pattern + " | ";
    }
    return out;
}

// A cozy arrangement: a lead, an arpeggio of the chords, a bass, light drums.
struct Arrangement {
    float bpm = 100.0f;
    std::string lead;
    std::vector<Chord> chords;
    std::string arp;
    std::string bass;
    std::string drums;
    Wave leadWave = Wave::Pulse25;
    float leadVolume = 0.6f;
    float peakDb = -17.0f;
};

Song song(const Arrangement& a)
{
    Song s{.bpm = a.bpm, .stepsPerBeat = 2, .tracks = {}, .peakDb = a.peakDb};
    s.tracks.push_back(Track{.wave = a.leadWave, .volume = a.leadVolume, .decay = 2.2f, .intro = {}, .loop = a.lead});
    s.tracks.push_back(Track{.wave = Wave::Pulse12, .volume = 0.2f, .decay = 4.5f, .intro = {}, .loop = chordPattern(a.chords, a.arp, 1)});
    s.tracks.push_back(Track{.wave = Wave::Triangle, .volume = 0.85f, .decay = 1.0f, .intro = {}, .loop = chordPattern(a.chords, a.bass)});
    if (!a.drums.empty()) {
        s.tracks.push_back(Track{.wave = Wave::Noise, .volume = 0.13f, .decay = 7.0f, .intro = {}, .loop = a.drums});
    }
    return s;
}

// Night: slower and softer, the lead a music box, no drums.
Song nightOf(Song s)
{
    s.bpm *= 0.8f;
    s.tracks[0].wave = Wave::Triangle;
    s.tracks[0].volume = 1.0f;
    s.tracks[0].decay = 3.5f;
    if (s.tracks.size() > 3) {
        s.tracks.pop_back();
    }
    s.peakDb -= 1.5f;
    return s;
}

// ---- Petal Isle: a skipping tune in D.
Arrangement island()
{
    return {.bpm = 112.0f,
            .lead = "F#5 . A5 . D6 - A5 . | E5 . F#5 . A5 - . . | F#5 . D5 . B4 - D5 . | G5 - - . F#5 . E5 . | "
                    "D5 . F#5 . A5 - F#5 . | G5 . B5 . D6 - B5 . | A5 . G5 . F#5 . E5 . | D5 - - - - - . . |",
            .chords = {chord("D"), chord("A"), chord("Bm"), chord("G"), chord("D"), chord("G"), chord("A"), chord("D")},
            .arp = "1 5 8 5 3' 5 8 5",
            .bass = "1 - 5, - 1 - 5, -",
            .drums = times("C3 . C7 . C3 C7 C7 .", 8)};
}

// ---- Saltwhistle: a sea waltz in C (six eighths a bar).
Arrangement coast()
{
    return {.bpm = 120.0f,
            .lead = "E5 - G5 - C6 - | A5 - - - F5 - | G5 - E5 - C5 - | D5 - - - - - | "
                    "E5 - C5 - A4 - | F5 - A5 - C6 - | B5 - G5 - D5 - | C5 - - - . . |",
            .chords = {chord("C"), chord("F"), chord("C"), chord("G"), chord("Am"), chord("F"), chord("G"), chord("C")},
            .arp = "1 5 8 5 3' 5",
            .bass = "1 - - 5, - -",
            .drums = times("C3 . C7 . C7 .", 8),
            .leadWave = Wave::Pulse25,
            .leadVolume = 0.62f};
}

// ---- Fernwood: a curious walk in E minor.
Arrangement forest()
{
    return {.bpm = 96.0f,
            .lead = "B4 . E5 . G5 - F#5 . | E5 - C5 . G4 - . . | D5 . G5 . B5 - A5 . | F#5 - - . D5 . A4 . | "
                    "B4 . E5 . G5 . B5 . | A5 - E5 . C5 - . . | D5 . F#5 . A5 - F#5 . | E5 - - - - - . . |",
            .chords = {chord("Em"), chord("C"), chord("G"), chord("D"), chord("Em"), chord("Am"), chord("D"), chord("Em")},
            .arp = "1 5 8 5 1 5 8 5",
            .bass = "1 - - 5, 1 - - -",
            .drums = times("C3 . . C7 . . C6 .", 8),
            .leadWave = Wave::Pulse12,
            .leadVolume = 0.7f};
}

// ---- Windcrest Pass: airy bells in F.
Arrangement mountain()
{
    return {.bpm = 84.0f,
            .lead = "C5 - - - F5 - A5 - | G5 - - - E5 - C5 - | D5 - - - A5 - F5 - | D5 - - - - - Bb4 - | "
                    "A4 - - - C5 - F5 - | E5 - - - A5 - C6 - | Bb5 - A5 - G5 - F5 - | E5 - - - - - . . |",
            .chords = {chord("F"), chord("C"), chord("Dm"), chord("Bb"), chord("F"), chord("Am"), chord("Bb"), chord("C")},
            .arp = "1 5 8 3' 5' 3' 8 5",
            .bass = "1 - - - 5, - - -",
            .drums = {},
            .leadWave = Wave::Pulse12,
            .leadVolume = 0.72f,
            .peakDb = -17.5f};
}

// ---- The Undercroft: hushed, C minor.
Arrangement depths()
{
    return {.bpm = 70.0f,
            .lead = "C5 - - - . . Eb5 - | G5 - Ab5 - G5 - - - | F5 - - - . . Ab5 - | G5 - - - - - . . | "
                    "Eb5 - - - . . C5 - | Ab4 - C5 - Eb5 - - - | D5 - Bb4 - F5 - - - | B4 - - - - - . . |",
            .chords = {chord("Cm"), chord("Ab"), chord("Fm"), chord("G"), chord("Cm"), chord("Ab"), chord("Bb"), chord("G")},
            .arp = "1 . 5 . 8 . 5 .",
            .bass = "1 - - - - - - -",
            .drums = times("C2 . . . . . . .", 8),
            .leadWave = Wave::Pulse12,
            .leadVolume = 0.6f,
            .peakDb = -18.0f};
}

Arrangement boss()
{
    return {.bpm = 150.0f,
            .lead = "D5 . D5 . F5 . A5 . | Bb5 - A5 . F5 . D5 . | C5 . E5 . G5 - E5 . | C#5 - - . E5 . A5 . | "
                    "D6 . A5 . F5 . D5 . | Bb4 . D5 . F5 - Bb5 . | G5 . Bb5 . D6 - Bb5 . | A5 - C#6 - E6 - - . |",
            .chords = {chord("Dm"), chord("Bb"), chord("C"), chord("A"), chord("Dm"), chord("Bb"), chord("Gm"), chord("A")},
            .arp = "1 5 8 5 1 5 8 5",
            .bass = "1 1 8, 1 1 1 8, 5,",
            .drums = times("C3 . C8 . C6 . C8 C8", 8),
            .leadWave = Wave::Pulse25,
            .leadVolume = 0.75f,
            .peakDb = -15.5f};
}

Arrangement finalBoss()
{
    return {.bpm = 158.0f,
            .lead = "E5 . G5 . B5 . E6 . | C6 - B5 . G5 . E5 . | A5 . C6 . E6 - C6 . | D#6 - - . B5 . F#5 . | "
                    "E6 . B5 . G5 . E5 . | C5 . E5 . G5 - C6 . | D6 . A5 . F#5 - D5 . | D#5 - F#5 - B5 - - . |",
            .chords = {chord("Em"), chord("C"), chord("Am"), chord("B"), chord("Em"), chord("C"), chord("D"), chord("B")},
            .arp = "1 5 8 5 3 5 8 5",
            .bass = "1 1 8, 1 1 1 8, 5,",
            .drums = times("C3 . C8 . C6 . C8 C8", 8),
            .leadWave = Wave::Pulse25,
            .leadVolume = 0.78f,
            .peakDb = -15.0f};
}

Arrangement title()
{
    return {.bpm = 88.0f,
            .lead = "B4 - - - D5 - G5 - | F#5 - - - A5 - D5 - | E5 - - - G5 - B5 - | A5 - - - - - G5 - | "
                    "B5 - - - A5 - G5 - | F#5 - - - D5 - B4 - | C5 - E5 - G5 - E5 - | D5 - - - - - . . |",
            .chords = {chord("G"), chord("D"), chord("Em"), chord("C"), chord("G"), chord("Bm"), chord("C"), chord("D")},
            .arp = "1 3 5 8 5 3 1 3",
            .bass = "1 - - - 5 - - -",
            .drums = {},
            .leadWave = Wave::Pulse25,
            .leadVolume = 0.6f};
}

// ---- The voyage: a rocking waltz in A.
Arrangement voyage()
{
    return {.bpm = 100.0f,
            .lead = "C#5 - E5 - A5 - | G#5 - - - E5 - | F#5 - A5 - C#6 - | B5 - - - A5 - | "
                    "E5 - A5 - C#6 - | D6 - B5 - F#5 - | E5 - G#5 - B5 - | A5 - - - . . |",
            .chords = {chord("A"), chord("E"), chord("F#m"), chord("D"), chord("A"), chord("D"), chord("E"), chord("A")},
            .arp = "1 5 8 5 3' 5",
            .bass = "1 - - 5, - -",
            .drums = times("C3 . . C7 . .", 8),
            .leadWave = Wave::Pulse25,
            .leadVolume = 0.6f};
}

} // namespace

Tune regionTune(const game::Region region)
{
    switch (region) {
    case game::Region::Island: return Tune::Island;
    case game::Region::Coast: return Tune::Coast;
    case game::Region::Forest: return Tune::Forest;
    case game::Region::Mountain: return Tune::Mountain;
    case game::Region::Depths: return Tune::Depths;
    case game::Region::Count: break;
    }
    return Tune::Island;
}

std::optional<Song> tune(const Tune t, const int variant)
{
    const bool hasNight = t == Tune::Island || t == Tune::Coast || t == Tune::Forest || t == Tune::Mountain;
    if (variant == 1 && !hasNight) {
        return std::nullopt;
    }
    Song s;
    switch (t) {
    case Tune::Title: s = song(title()); break;
    case Tune::Island: s = song(island()); break;
    case Tune::Coast: s = song(coast()); break;
    case Tune::Forest: s = song(forest()); break;
    case Tune::Mountain: s = song(mountain()); break;
    case Tune::Depths: s = song(depths()); break;
    case Tune::Boss: s = song(boss()); break;
    case Tune::FinalBoss: s = song(finalBoss()); break;
    case Tune::Voyage: s = song(voyage()); break;
    case Tune::Ending: {
        Arrangement a = island();
        a.bpm = 92.0f;
        a.arp = "1 3 5 8 5 3 1 3";
        a.drums = times("C3 . . . C7 . . .", 8);
        a.leadVolume = 0.66f;
        s = song(a);
        break;
    }
    case Tune::Count: break;
    }
    if (variant == 1) {
        return nightOf(s);
    }
    return s;
}

SoundRecipe sfxRecipe(const Sfx sfx)
{
    switch (sfx) {
    case Sfx::Jump: return {{{tone(Wave::Pulse25, note("E5"), 0.08f, note("B5"), 0.8f)}}, -24.0f};
    case Sfx::HyperJump:
        return {{{tone(Wave::Pulse12, note("B5"), 0.05f, note("F#6")), tone(Wave::Pulse12, note("F#6"), 0.12f, note("C#7"))},
                 {noise(9000.0f, 0.22f, 3000.0f, 0.6f)}},
                -17.0f};
    case Sfx::Land: return {{{tone(Wave::Triangle, note("E3"), 0.05f, note("B2"))}, {noise(1600.0f, 0.04f, 600.0f, 0.35f)}}, -27.0f};
    case Sfx::Climb: return {{{noise(2400.0f, 0.03f, 1200.0f, 0.4f)}}, -30.0f};
    case Sfx::Swing: return {{{noise(12000.0f, 0.1f, 3500.0f, 0.9f)}, {tone(Wave::Pulse12, note("D6"), 0.05f, note("A5"), 0.3f)}}, -21.0f};
    case Sfx::Hit: return {{{tone(Wave::Pulse25, note("A5"), 0.04f, note("D5"))}, {noise(6000.0f, 0.06f, 1800.0f, 0.8f)}}, -18.0f};
    case Sfx::Crit:
        return {{{tone(Wave::Pulse25, note("D6"), 0.04f, note("A5")), tone(Wave::Pulse12, note("A6"), 0.09f, note("D6"))},
                 {noise(8000.0f, 0.1f, 1500.0f, 0.9f)}},
                -15.5f};
    case Sfx::MonsterDown:
        return {{{tone(Wave::Pulse25, note("G5"), 0.05f), tone(Wave::Pulse25, note("D5"), 0.05f),
                  tone(Wave::Pulse25, note("A4"), 0.12f, note("E4"))},
                 {noise(3000.0f, 0.18f, 300.0f, 0.5f)}},
                -18.0f};
    case Sfx::Hurt:
        return {{{tone(Wave::Pulse25, note("B5"), 0.05f, note("F#5")), tone(Wave::Pulse25, note("E5"), 0.12f, note("B4"))},
                 {noise(2600.0f, 0.12f, 600.0f, 0.5f)}},
                -16.0f};
    case Sfx::Death:
        return {{{tone(Wave::Pulse25, note("G5"), 0.14f), tone(Wave::Pulse25, note("E5"), 0.14f), tone(Wave::Pulse25, note("C5"), 0.14f),
                  tone(Wave::Triangle, note("A4"), 0.7f, note("D3"))}},
                -15.0f};
    case Sfx::LevelUp:
        return {{{tone(Wave::Pulse25, note("D5"), 0.08f), tone(Wave::Pulse25, note("F#5"), 0.08f), tone(Wave::Pulse25, note("A5"), 0.08f),
                  tone(Wave::Pulse25, note("D6"), 0.16f), tone(Wave::Pulse25, note("A5"), 0.08f), tone(Wave::Pulse25, note("D6"), 0.5f)},
                 {tone(Wave::Pulse12, note("F#4"), 0.24f), tone(Wave::Pulse12, note("A4"), 0.24f), tone(Wave::Pulse12, note("F#5"), 0.6f)},
                 {tone(Wave::Triangle, note("D3"), 0.32f), tone(Wave::Triangle, note("A3"), 0.32f), tone(Wave::Triangle, note("D4"), 0.5f)},
                 {rest(0.4f), noise(10000.0f, 0.5f, 14000.0f, 0.25f)}},
                -12.0f};
    case Sfx::Pickup: return {{{tone(Wave::Pulse25, note("B5"), 0.05f), tone(Wave::Pulse25, note("F#6"), 0.08f)}}, -22.0f};
    case Sfx::PickupRare:
        return {{{tone(Wave::Pulse25, note("D6"), 0.06f), tone(Wave::Pulse25, note("F#6"), 0.06f), tone(Wave::Pulse25, note("A6"), 0.12f)}},
                -19.0f};
    case Sfx::PickupEpic:
        return {{{tone(Wave::Pulse25, note("D6"), 0.06f), tone(Wave::Pulse25, note("F#6"), 0.06f), tone(Wave::Pulse25, note("A6"), 0.06f),
                  tone(Wave::Pulse25, note("C#7"), 0.22f)},
                 {rest(0.06f), tone(Wave::Pulse12, note("A5"), 0.34f)}},
                -17.0f};
    case Sfx::PickupUnique:
        return {{{tone(Wave::Pulse25, note("A5"), 0.07f), tone(Wave::Pulse25, note("D6"), 0.07f), tone(Wave::Pulse25, note("F#6"), 0.07f),
                  tone(Wave::Pulse25, note("A6"), 0.07f), tone(Wave::Pulse25, note("D7"), 0.36f)},
                 {tone(Wave::Pulse12, note("D5"), 0.28f), tone(Wave::Pulse12, note("F#5"), 0.36f)},
                 {tone(Wave::Triangle, note("D4"), 0.64f)}},
                -14.0f};
    case Sfx::Coin: return {{{tone(Wave::Pulse12, note("C#6"), 0.04f), tone(Wave::Pulse12, note("F#6"), 0.1f)}}, -23.0f};
    case Sfx::Potion:
        return {{{tone(Wave::Triangle, note("D5"), 0.08f, note("A5")), tone(Wave::Pulse12, note("D6"), 0.12f, note("F#6"))}}, -19.0f};
    case Sfx::Pebble:
        return {{{tone(Wave::Pulse12, note("E6"), 0.06f, note("A5"), 0.5f)}, {noise(7000.0f, 0.05f, 3000.0f, 0.4f)}}, -22.0f};
    case Sfx::Strike:
        return {{{noise(11000.0f, 0.2f, 2500.0f, 1.0f)}, {tone(Wave::Pulse25, note("A4"), 0.18f, note("A5"), 0.6f)}}, -15.5f};
    case Sfx::Quake:
        return {{{noise(1400.0f, 0.5f, 90.0f, 1.0f)},
                 {tone(Wave::Triangle, note("E2"), 0.45f, note("A1"))},
                 {rest(0.1f), noise(3000.0f, 0.3f, 500.0f, 0.5f)}},
                -12.5f};
    case Sfx::Rush:
        return {{{noise(5000.0f, 0.35f, 12000.0f, 0.8f)}, {tone(Wave::Pulse25, note("D4"), 0.3f, note("D5"), 0.5f)}}, -16.0f};
    case Sfx::Buff:
        return {{{tone(Wave::Pulse12, note("A5"), 0.07f), tone(Wave::Pulse12, note("C#6"), 0.07f), tone(Wave::Pulse12, note("E6"), 0.07f),
                  tone(Wave::Pulse12, note("A6"), 0.25f)},
                 {tone(Wave::Triangle, note("A3"), 0.46f, note("A4"))}},
                -16.0f};
    case Sfx::Bolt:
        return {{{tone(Wave::Pulse12, note("A6"), 0.12f, note("A5"), 0.6f)}, {noise(9000.0f, 0.12f, 4000.0f, 0.35f)}}, -18.0f};
    case Sfx::Spark:
        return {{{noise(14000.0f, 0.25f, 6000.0f, 0.9f)},
                 {tone(Wave::Pulse12, note("E7"), 0.04f), tone(Wave::Pulse12, note("B6"), 0.04f), tone(Wave::Pulse12, note("G7"), 0.08f, note("E6"))}},
                -16.0f};
    case Sfx::Starfall:
        return {{{tone(Wave::Pulse12, note("E6"), 0.05f), tone(Wave::Pulse12, note("B6"), 0.05f), tone(Wave::Pulse12, note("E7"), 0.14f, note("A6"))},
                 {noise(7000.0f, 0.45f, 300.0f, 0.6f)},
                 {tone(Wave::Triangle, note("E3"), 0.4f, note("B2"))}},
                -14.0f};
    case Sfx::Bloom:
        return {{{tone(Wave::Pulse25, note("D5"), 0.08f), tone(Wave::Pulse25, note("F#5"), 0.08f), tone(Wave::Pulse25, note("A5"), 0.08f),
                  tone(Wave::Pulse25, note("D6"), 0.3f)},
                 {tone(Wave::Triangle, note("D4"), 0.54f)}},
                -16.0f};
    case Sfx::ShieldHit: return {{{tone(Wave::Pulse12, note("F#7"), 0.07f, note("C#7"))}, {noise(9000.0f, 0.05f)}}, -20.0f};
    case Sfx::NoMana: return {{{tone(Wave::Pulse12, note("F4"), 0.06f), rest(0.03f), tone(Wave::Pulse12, note("D4"), 0.08f)}}, -22.0f};
    case Sfx::Portal:
        return {{{tone(Wave::Pulse25, note("D5"), 0.4f, note("D6"), 0.6f)},
                 {noise(5000.0f, 0.4f, 12000.0f, 0.4f)},
                 {tone(Wave::Triangle, note("A4"), 0.4f, note("A5"))}},
                -16.0f};
    case Sfx::Sealed: return {{{tone(Wave::Pulse12, note("D4"), 0.07f), rest(0.03f), tone(Wave::Pulse12, note("D4"), 0.09f)}}, -21.0f};
    case Sfx::QuestAccept: return {{{tone(Wave::Pulse25, note("A5"), 0.07f), tone(Wave::Pulse25, note("D6"), 0.14f)}}, -18.0f};
    case Sfx::QuestComplete:
        return {{{tone(Wave::Pulse25, note("F#5"), 0.07f), tone(Wave::Pulse25, note("A5"), 0.07f), tone(Wave::Pulse25, note("F#6"), 0.2f)}},
                -17.0f};
    case Sfx::QuestDone:
        return {{{tone(Wave::Pulse25, note("D5"), 0.08f), tone(Wave::Pulse25, note("F#5"), 0.08f), tone(Wave::Pulse25, note("A5"), 0.08f),
                  tone(Wave::Pulse25, note("F#6"), 0.08f), tone(Wave::Pulse25, note("D6"), 0.32f)},
                 {tone(Wave::Triangle, note("D4"), 0.32f), tone(Wave::Triangle, note("A3"), 0.32f)}},
                -15.0f};
    case Sfx::Learn:
        return {{{tone(Wave::Pulse12, note("E6"), 0.06f), tone(Wave::Pulse12, note("B6"), 0.06f), tone(Wave::Pulse12, note("E7"), 0.16f)}},
                -17.0f};
    case Sfx::Point: return {{{tone(Wave::Pulse25, note("F#6"), 0.05f, note("B6"))}}, -19.0f};
    case Sfx::Equip:
        return {{{noise(5000.0f, 0.05f, 2000.0f, 0.6f)}, {tone(Wave::Pulse12, note("B5"), 0.06f), tone(Wave::Pulse12, note("E6"), 0.08f)}},
                -19.0f};
    case Sfx::Buy: return {{{tone(Wave::Pulse12, note("F#6"), 0.04f), tone(Wave::Pulse12, note("C#7"), 0.09f)}}, -20.0f};
    case Sfx::Sell: return {{{tone(Wave::Pulse12, note("C#7"), 0.04f), tone(Wave::Pulse12, note("F#6"), 0.09f)}}, -20.0f};
    case Sfx::Bind:
        return {{{tone(Wave::Pulse25, note("A5"), 0.04f), tone(Wave::Pulse25, note("E6"), 0.06f)}, {noise(6000.0f, 0.03f, 3000.0f, 0.4f)}},
                -20.0f};
    case Sfx::MenuMove: return {{{tone(Wave::Pulse12, note("B5"), 0.03f)}}, -26.0f};
    case Sfx::MenuSelect: return {{{tone(Wave::Pulse25, note("F#5"), 0.04f), tone(Wave::Pulse25, note("B5"), 0.07f)}}, -21.0f};
    case Sfx::MenuBack: return {{{tone(Wave::Pulse25, note("B5"), 0.04f), tone(Wave::Pulse25, note("F#5"), 0.06f)}}, -22.0f};
    case Sfx::Error: return {{{tone(Wave::Pulse12, note("D4"), 0.08f, note("B3"))}}, -21.0f};
    case Sfx::Type: return {{{tone(Wave::Triangle, note("B5"), 0.025f)}}, -27.0f};
    case Sfx::Open:
        return {{{tone(Wave::Pulse25, note("D5"), 0.04f, note("A5"), 0.7f), tone(Wave::Pulse12, note("D6"), 0.05f)}}, -21.0f};
    case Sfx::Close: return {{{tone(Wave::Pulse25, note("A5"), 0.04f, note("D5"), 0.7f)}}, -22.0f};
    case Sfx::BossRoar:
        return {{{tone(Wave::Pulse25, note("B3"), 0.32f, note("F#3"))}, {noise(800.0f, 0.42f, 200.0f, 0.7f)}}, -15.0f};
    case Sfx::BossLand:
        return {{{noise(1200.0f, 0.35f, 100.0f, 1.0f)}, {tone(Wave::Triangle, note("F#2"), 0.3f, note("B1"))}}, -13.0f};
    case Sfx::BossDown:
        return {{{noise(2000.0f, 0.9f, 80.0f, 1.0f)},
                 {tone(Wave::Triangle, note("B3"), 0.9f, note("B1"))},
                 {tone(Wave::Pulse25, note("F#5"), 0.15f), tone(Wave::Pulse25, note("D5"), 0.15f), tone(Wave::Pulse25, note("B4"), 0.5f)}},
                -12.0f};
    case Sfx::Shot:
        return {{{tone(Wave::Pulse12, note("A5"), 0.1f, note("D5"), 0.6f)}, {noise(4000.0f, 0.08f, 1500.0f, 0.3f)}}, -21.0f};
    case Sfx::HazardWarn:
        return {{{tone(Wave::Pulse12, note("F#6"), 0.06f), rest(0.05f), tone(Wave::Pulse12, note("F#6"), 0.06f)}}, -22.0f};
    case Sfx::Rest:
        return {{{tone(Wave::Triangle, note("F#5"), 0.25f), tone(Wave::Triangle, note("D5"), 0.25f), tone(Wave::Triangle, note("E5"), 0.25f),
                  tone(Wave::Triangle, note("A4"), 0.6f)},
                 {tone(Wave::Triangle, note("D4"), 1.35f)}},
                -16.0f};
    case Sfx::JobAdvance:
        return {{{tone(Wave::Pulse25, note("A4"), 0.12f), tone(Wave::Pulse25, note("D5"), 0.12f), tone(Wave::Pulse25, note("F#5"), 0.12f),
                  tone(Wave::Pulse25, note("A5"), 0.24f), tone(Wave::Pulse25, note("F#5"), 0.12f), tone(Wave::Pulse25, note("D6"), 0.7f)},
                 {tone(Wave::Pulse12, note("F#4"), 0.36f), tone(Wave::Pulse12, note("A4"), 0.36f), tone(Wave::Pulse12, note("D5"), 0.82f)},
                 {tone(Wave::Triangle, note("D3"), 0.72f), tone(Wave::Triangle, note("D3"), 0.82f)},
                 {rest(0.6f), noise(9000.0f, 0.8f, 14000.0f, 0.3f)}},
                -12.0f};
    case Sfx::Spawn: return {{{tone(Wave::Pulse12, note("D5"), 0.06f, note("A5"), 0.5f)}}, -27.0f};
    case Sfx::Horn:
        return {{{tone(Wave::Pulse50, note("A3"), 0.5f, 0.0f, 0.9f), tone(Wave::Pulse50, note("D4"), 0.8f, 0.0f, 0.9f)},
                 {tone(Wave::Triangle, note("D3"), 1.3f)}},
                -15.0f};
    case Sfx::Count: break;
    }
    return {{{tone(Wave::Pulse12, note("D5"), 0.05f)}}, -24.0f};
}

} // namespace realm::app
