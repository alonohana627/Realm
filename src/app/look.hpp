#pragma once

// How Realm looks, as data: the interface palette, each place's sky,
// silhouettes, ground and lighting by day and by night, the hero's avatar
// from the creator's choices and the equipment worn, the sixteen
// townsfolk, the monsters' colors (elites deeper and decorated), item and
// skill icons and rarity colors. Presentation only.

#include "anime_look.hpp"

#include "character.hpp"
#include "items.hpp"
#include "maps.hpp"
#include "monsters.hpp"
#include "skills.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/draw/effects.hpp>

#include <cstdint>

namespace realm::app {

using gamelib::core::Color;

[[nodiscard]] inline Color srgb(const std::uint32_t hex, const float alpha = 1.0f)
{
    return gamelib::core::srgb(hex).withAlpha(alpha);
}

// The interface: cream windows, a sky-blue title bar, warm brown ink.
namespace ui_color {
inline const Color kWindow = srgb(0xFFF8EA);
inline const Color kTitle = srgb(0x8ACBE8);
inline const Color kInk = srgb(0x4E3D33);
inline const Color kSoftInk = srgb(0x8A7460);
inline const Color kGold = srgb(0xF2B33D);
inline const Color kHp = srgb(0xF25F6E);
inline const Color kMp = srgb(0x5E9CF2);
inline const Color kXp = srgb(0xF6C845);
inline const Color kGood = srgb(0x3FAE62);
inline const Color kBad = srgb(0xE0525E);
inline const Color kWhite = srgb(0xFFFFFF);
inline const Color kButton = srgb(0xFFE7B8);
inline const Color kDark = srgb(0x2E2A44);
} // namespace ui_color

struct RegionLook {
    Color skyTop, skyHorizon;
    Color cloud, cloudShade;
    Color far, farRim;
    Color near, nearRim;
    Color light;
    Color cap, body;         // ground: the top and the earth
    Color footCap, footBody; // floating footholds
    int terrain = 0;         // the terrain shader's material: 0 grass, 1 sand, 2 wood, 3 snow, 4 stone, 5 moss
    int backdrop = 0;        // the backdrop shader's scene
    Color ambient;
    gamelib::draw::ColorGrade grade;
    float bloom = 0.35f;
};

// A map's look at a time of day (0 day .. 1 night).
[[nodiscard]] RegionLook regionLook(const game::MapDef& map, float night);

// Avatars.
[[nodiscard]] Color hairColor(int index);
inline constexpr int kHairColors = 8;
[[nodiscard]] anime::Avatar heroAvatar(const game::Character& character);
[[nodiscard]] anime::Avatar baseAvatar(const game::Appearance& look);
[[nodiscard]] anime::Avatar npcAvatar(game::NpcId npc);

struct MonsterColors {
    Color body;
    Color accent;
};
[[nodiscard]] MonsterColors monsterColors(game::SpeciesId species, bool elite);
[[nodiscard]] MonsterColors bossColors(game::BossId boss);
// The variant of its archetype a species is drawn as (a cap's pattern, a
// mochi's crystal), and the decoration its elites wear.
[[nodiscard]] int speciesVariant(game::SpeciesId species);

// How big a creature is drawn for its box.
struct MonsterScale {
    float quad = 0.0f; // the quad height
    float top = 0.0f;  // how far above the feet the drawing's top is
};
[[nodiscard]] MonsterScale monsterScale(game::Archetype archetype, float boxHeight);

// Icons (ui.frag's icon families).
struct IconLook {
    float family = 0.0f;
    float style = 0.0f;
    Color a;
    Color b;
};
[[nodiscard]] IconLook itemIcon(int item);
[[nodiscard]] IconLook skillIcon(game::SkillId skill);
[[nodiscard]] Color rarityColor(game::Rarity rarity);
[[nodiscard]] Color paletteColor(int index); // kAnimePalette
[[nodiscard]] Color regionAccent(game::Region region);
[[nodiscard]] Color skillColor(game::SkillId skill);

} // namespace realm::app
