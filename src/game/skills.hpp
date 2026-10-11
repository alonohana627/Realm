#pragma once

// Realm's skills: three a beginner learns with the few points of levels 2-10
// and eight per job (warrior or mage) - attack skills, buffs and passives -
// with their per-level numbers (GameLib's core::SkillDef: MP cost,
// cooldown, duration, value), the character level that opens each, what
// they look and sound like (their family), and the skill window's words.

#include "rules.hpp"

#include <gamelib/core/skills.hpp>

#include <span>
#include <string>
#include <string_view>

namespace realm::game {

enum class SkillKind : std::uint8_t { Attack, Buff, Passive, Movement, Heal };

// The look and sound family of a skill (effects and sounds per family).
enum class SkillFamily : std::uint8_t { Throw, Rest, Hyper, Slash, Guard, Roar, Quake, Rush, Bolt, Spark, Shield, Wind, Star, Bloom };

struct SkillInfo {
    SkillId id = SkillId::PebbleToss;
    std::string_view name;
    Job job = Job::Beginner;
    SkillKind kind = SkillKind::Attack;
    SkillFamily family = SkillFamily::Throw;
    int unlockLevel = 1;
    int maxTargets = 1;        // attack skills: monsters struck per use (level 1)
    std::string_view description;
};

[[nodiscard]] const SkillInfo& skillInfo(SkillId id);
[[nodiscard]] const gamelib::core::SkillDef& skillDef(SkillId id);
// The skills of a job in the skill window's order.
[[nodiscard]] std::span<const SkillId> jobSkills(Job job);
// What a skill does at a level ("deals 240% damage to up to 6 monsters").
[[nodiscard]] std::string skillEffectText(SkillId id, int level);
// A level's numbers (level 1..max; out of range: the nearest level).
[[nodiscard]] const gamelib::core::SkillLevel& skillLevelValues(SkillId id, int level);
// Monsters struck per use at a level.
[[nodiscard]] int skillTargets(SkillId id, int level);

// Buff ids for core::Buffs.
inline constexpr int kBuffIronBody = 1;
inline constexpr int kBuffWarCry = 2;
inline constexpr int kBuffManaShield = 3;
inline constexpr int kBuffSwiftwind = 4;

} // namespace realm::game
