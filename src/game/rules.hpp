#pragma once

// Realm's shared vocabulary and numbers: the tick rate and the tile, the
// regions, the jobs, the four core stats and the item bonuses, equipment
// slots, inventory tabs, rarities, the skills and the per-tick input. Every
// rule runs on whole ticks at kTickRate, so the same per-tick input always
// gives the same run.

#include <gamelib/core/math.hpp>

#include <cstdint>
#include <string_view>

namespace realm::game {

inline constexpr int kTickRate = 60;
inline constexpr float kTile = 48.0f; // world units per tile

inline constexpr int kMaxLevel = 50;
inline constexpr int kPointsPerLevel = 5;  // ability points (core stats)
inline constexpr int kJobLevel = 10;       // the job advancement
inline constexpr int kStartCore = 4;       // every core stat at level 1
inline constexpr int kBagSlots = 24;       // per inventory tab

// The hero's collision box (the chibi drawing is about 120 units tall).
inline constexpr gamelib::core::Vec2 kHeroSize{34.0f, 90.0f};

enum class Region : std::uint8_t { Island, Coast, Forest, Mountain, Depths, Count };
inline constexpr int kRegionCount = static_cast<int>(Region::Count);
[[nodiscard]] std::string_view regionName(Region region);

enum class Job : std::uint8_t { Beginner, Warrior, Mage, Count };
inline constexpr int kJobCount = static_cast<int>(Job::Count);
[[nodiscard]] std::string_view jobName(Job job);

// The four core stats, raised with ability points.
enum class Core : std::uint8_t { Str, Agi, Int, Vit, Count };
inline constexpr int kCoreCount = static_cast<int>(Core::Count);
[[nodiscard]] std::string_view coreName(Core stat);
[[nodiscard]] std::string_view coreShort(Core stat);

// What an item adds (core::ItemStat::stat).
enum class Bonus : std::uint8_t { Str, Agi, Int, Vit, Attack, Magic, Defense, Hp, Mp, Crit, Speed, Jump, Count };
inline constexpr int kBonusCount = static_cast<int>(Bonus::Count);
[[nodiscard]] std::string_view bonusName(Bonus bonus);

enum class Slot : std::uint8_t { Weapon, Hat, Top, Bottom, Shoes, Gloves, Count };
inline constexpr int kSlotCount = static_cast<int>(Slot::Count);
[[nodiscard]] std::string_view slotName(Slot slot);

// The inventory's tabs (an item's core::ItemDef::kind).
enum class Tab : std::uint8_t { Equip, Use, Etc, Count };
inline constexpr int kTabCount = static_cast<int>(Tab::Count);
[[nodiscard]] std::string_view tabName(Tab tab);

enum class Rarity : std::uint8_t { Common, Rare, Epic, Unique, Count };
inline constexpr int kRarityCount = static_cast<int>(Rarity::Count);
[[nodiscard]] std::string_view rarityName(Rarity rarity);

// Every skill: three a beginner learns, eight per job.
enum class SkillId : std::uint8_t {
    // Beginner
    PebbleToss, CatchBreath, HyperJump,
    // Warrior
    PowerStrike, Cleave, IronBody, BladeMastery, WarCry, HpBoost, Quake, Rush,
    // Mage
    ArcaneBolt, ChainSpark, ManaShield, SpellMastery, Swiftwind, MpBoost, Starfall, Bloom,
    Count
};
inline constexpr int kSkillCount = static_cast<int>(SkillId::Count);
static_assert(kSkillCount <= 32, "Input::skills is a 32-bit mask");

// One tick of player input, after key mapping (never raw keys).
struct Input {
    int move = 0;  // -1 left, 0, 1 right
    int climb = 0; // -1 up, 0, 1 down (ropes, ladders, drop through, prone, the hyper jump's direction)
    bool jumpPressed = false;
    bool jumpHeld = false;
    bool attackHeld = false; // held: the basic attack repeats
    bool upPressed = false;  // portals, talking
    bool pickup = false;
    std::uint32_t skills = 0; // bit (1 << SkillId) pressed this tick
    int useItem = 0;          // an item hotkey pressed this tick (item id), 0 none

    [[nodiscard]] bool skill(const SkillId id) const { return (skills >> static_cast<unsigned>(id) & 1u) != 0; }
    friend bool operator==(const Input&, const Input&) = default;
};

} // namespace realm::game
