#pragma once

// The hero, MapleStory-like: a name and a look from the character creator,
// a job (beginner, then warrior or mage at level 10), a level from 1 to 50
// on an experience curve, four core stats raised with five ability points
// per level, skills learned with skill points, HP and MP, timed buffs, an
// inventory in three tabs, six equipment slots and coins. Derived values
// (HP, MP, attack, magic, defense, critical chance, Speed and Jump) come from
// the base stats, gear, passives and buffs; Speed and Jump turn into the
// platformer tuning, so they really change walking and jumping.

#include "items.hpp"
#include "rules.hpp"
#include "skills.hpp"

#include <gamelib/core/items.hpp>
#include <gamelib/core/platformer.hpp>
#include <gamelib/core/progression.hpp>
#include <gamelib/core/skills.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace realm::game {

using CoreStats = gamelib::core::Stats<Core, kCoreCount>;

// The creator's choices (avatar.frag's base looks).
struct Appearance {
    std::string name = "Hero";
    int hair = 0;          // style 0..7
    int hairColor = 0;     // index into the creator's hair colors
    int eyes = 0;          // 0..7
    int skin = 0;          // 0..3
    friend bool operator==(const Appearance&, const Appearance&) = default;
};

// Everything the stat window shows beside the core stats.
struct Derived {
    CoreStats core;  // base points + gear
    int maxHp = 0;
    int maxMp = 0;
    int attack = 0;
    int magic = 0;
    int defense = 0;
    int crit = 0;    // percent
    int speed = 100; // percent, capped at kSpeedCap
    int jump = 100;  // percent, capped at kJumpCap
};
inline constexpr int kSpeedCap = 140;
inline constexpr int kJumpCap = 123;

// What a timed buff adds while it lasts.
struct BuffEffect {
    int defense = 0;
    int attackPercent = 0;
    int speed = 0;
    int jump = 0;
    int shield = 0; // percent of damage taken from MP
};

[[nodiscard]] const gamelib::core::ExperienceCurve& experienceCurve();

class Character {
public:
    Character();

    Appearance look;
    Job job = Job::Beginner;
    int level = 1;
    std::int64_t xp = 0;
    gamelib::core::AbilityPoints<Core, kCoreCount> points;
    gamelib::core::SkillBook skills;
    gamelib::core::Resource hp;
    gamelib::core::Resource mp;
    gamelib::core::Buffs<BuffEffect> buffs;
    std::array<gamelib::core::Inventory, kTabCount> bags;
    gamelib::core::Equipment gear;
    std::int64_t coins = 0;

    [[nodiscard]] gamelib::core::Inventory& bag(const Tab tab) { return bags[static_cast<std::size_t>(tab)]; }
    [[nodiscard]] const gamelib::core::Inventory& bag(const Tab tab) const { return bags[static_cast<std::size_t>(tab)]; }
    [[nodiscard]] int countOf(int item) const;
    // Adds to the item's tab; returns what did not fit.
    int add(const gamelib::core::ItemStack& stack);

    [[nodiscard]] Derived derived() const { return derivedWith(points.base()); }
    // The derived values with other base points (the stat window's preview).
    [[nodiscard]] Derived derivedWith(const CoreStats& base) const;

    bool spendPoint(Core stat);
    // Spends every point by the job's weights; returns how many.
    int autoAssign();
    [[nodiscard]] Core nextAuto() const;

    gamelib::core::LevelUp gainXp(std::int64_t amount);
    [[nodiscard]] float xpProgress() const;
    [[nodiscard]] std::int64_t xpToNext() const;

    // Becomes a warrior or a mage: the job's skills open, a few points to
    // start with, HP and MP grow.
    void advance(Job to);

    void refreshResources();
    [[nodiscard]] gamelib::core::PlatformerTuning tuning() const;

    [[nodiscard]] int skillLevel(SkillId id) const { return skills.levelOf(static_cast<int>(id)); }
    [[nodiscard]] gamelib::core::Wearer wearer() const { return {.level = level, .job = static_cast<int>(job)}; }
    [[nodiscard]] int gearBonus(Bonus bonus) const;
    [[nodiscard]] int buffShield() const;
};

} // namespace realm::game
