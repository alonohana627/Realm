#pragma once

// Realm's sound effects and music, all synthesized with GameLib's synth at
// startup (no audio files): a bright island tune, a sea waltz for the coast,
// a curious forest walk, airy mountain bells, the hushed Undercroft, each
// outdoor region with a softer night take (MusicBank variants), the area
// bosses, the last boss, the voyage, the title and the ending. Effects for
// every swing, skill, pickup by rarity, level up, quest step and UI click.

#include "rules.hpp"

#include <gamelib/audio/music.hpp>
#include <gamelib/audio/player.hpp>
#include <gamelib/audio/sound_bank.hpp>
#include <gamelib/audio/synth.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace realm::app {

enum class Sfx : std::uint8_t {
    Jump, HyperJump, Land, Swing, Hit, Crit, MonsterDown, Hurt, Death, LevelUp,
    Pickup, PickupRare, PickupEpic, PickupUnique, Coin, Potion,
    Pebble, Strike, Quake, Rush, Buff, Bolt, Spark, Starfall, Bloom, ShieldHit, NoMana,
    Portal, Sealed, QuestAccept, QuestComplete, QuestDone, Learn, Point, Equip, Buy, Sell, Bind,
    MenuMove, MenuSelect, MenuBack, Error, Type, Open, Close,
    BossRoar, BossLand, BossDown, Shot, HazardWarn, Rest, JobAdvance, Spawn, Horn, Climb,
    Count
};
inline constexpr std::size_t kSfxCount = static_cast<std::size_t>(Sfx::Count);

[[nodiscard]] gamelib::audio::SoundRecipe sfxRecipe(Sfx sfx);

enum class Tune : std::uint8_t { Title, Island, Coast, Forest, Mountain, Depths, Boss, FinalBoss, Voyage, Ending, Count };
inline constexpr std::size_t kTuneCount = static_cast<std::size_t>(Tune::Count);

// Variant 0 by day, 1 at night (nullopt where a tune has no night take).
[[nodiscard]] std::optional<gamelib::audio::Song> tune(Tune tune, int variant);
[[nodiscard]] Tune regionTune(game::Region region);

using Music = gamelib::audio::MusicBank<Tune, kTuneCount>;
using Sounds = gamelib::audio::SoundBank<Sfx, kSfxCount>;

} // namespace realm::app
