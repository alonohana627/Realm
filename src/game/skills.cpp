#include "skills.hpp"

#include <algorithm>
#include <array>
#include <format>

namespace realm::game {

namespace {

// helpers

using gamelib::core::SkillDef;
using gamelib::core::SkillLevel;
using F = SkillFamily;
using K = SkillKind;

constexpr int seconds(const int s)
{
    return s * kTickRate;
}

const std::array<SkillInfo, kSkillCount>& infos()
{
    static const std::array<SkillInfo, kSkillCount> t{{
        {SkillId::PebbleToss, "Pebble Toss", Job::Beginner, K::Attack, F::Throw, 2, 1,
         "Throw a lucky pebble at the nearest monster ahead."},
        {SkillId::CatchBreath, "Catch Breath", Job::Beginner, K::Passive, F::Rest, 2, 0,
         "Rest easy: HP and MP come back faster, more so standing still."},
        {SkillId::HyperJump, "Hyper Jump", Job::Beginner, K::Movement, F::Hyper, 1, 0,
         "Jump again in the air to burst forward. Hold up to shoot straight up."},
        {SkillId::PowerStrike, "Power Strike", Job::Warrior, K::Attack, F::Slash, 10, 1,
         "A heavy, glowing slash at the monster in front of you."},
        {SkillId::Cleave, "Crescent Cleave", Job::Warrior, K::Attack, F::Slash, 10, 4,
         "A wide crescent that strikes every monster around you."},
        {SkillId::IronBody, "Iron Body", Job::Warrior, K::Buff, F::Guard, 12, 0,
         "Harden like iron: more defense and no knockback."},
        {SkillId::BladeMastery, "Blade Mastery", Job::Warrior, K::Passive, F::Slash, 13, 0,
         "Practice pays: more attack and more critical hits."},
        {SkillId::WarCry, "War Cry", Job::Warrior, K::Buff, F::Roar, 15, 0, "A mighty shout that raises your attack."},
        {SkillId::HpBoost, "Hearty", Job::Warrior, K::Passive, F::Guard, 16, 0, "Raises your max HP."},
        {SkillId::Quake, "Earthquake", Job::Warrior, K::Attack, F::Quake, 20, 6,
         "Slam the ground: a shockwave runs both ways along the floor and stuns."},
        {SkillId::Rush, "Rush", Job::Warrior, K::Attack, F::Rush, 22, 8,
         "Charge forward, carrying every monster in your way."},
        {SkillId::ArcaneBolt, "Arcane Bolt", Job::Mage, K::Attack, F::Bolt, 10, 1,
         "A homing bolt of starlight. Pierces from level 8."},
        {SkillId::ChainSpark, "Chain Spark", Job::Mage, K::Attack, F::Spark, 10, 3,
         "Lightning that leaps from monster to monster."},
        {SkillId::ManaShield, "Mana Shield", Job::Mage, K::Buff, F::Shield, 12, 0,
         "A shimmering shield: part of every hit is paid with MP instead."},
        {SkillId::SpellMastery, "Spell Mastery", Job::Mage, K::Passive, F::Bolt, 13, 0,
         "Study pays: more magic and more critical hits."},
        {SkillId::Swiftwind, "Swiftwind", Job::Mage, K::Buff, F::Wind, 15, 0,
         "A friendly wind lifts your Speed and Jump."},
        {SkillId::MpBoost, "Deep Well", Job::Mage, K::Passive, F::Shield, 16, 0, "Raises your max MP."},
        {SkillId::Starfall, "Starfall", Job::Mage, K::Attack, F::Star, 20, 8,
         "Stars rain down on the monsters ahead, twice."},
        {SkillId::Bloom, "Bloom", Job::Mage, K::Heal, F::Bloom, 22, 0, "Flowers bloom around you and mend your wounds."},
    }};
    return t;
}

std::array<SkillDef, kSkillCount> buildDefs()
{
    std::array<SkillDef, kSkillCount> defs;
    const auto make = [&](const SkillId id, const int levels, auto&& level) {
        SkillDef& d = defs[static_cast<std::size_t>(id)];
        const SkillInfo& info = skillInfo(id);
        d.id = static_cast<int>(id);
        d.passive = info.kind == K::Passive;
        d.characterLevel = info.unlockLevel;
        for (int l = 1; l <= levels; ++l) {
            d.levels.push_back(level(l));
        }
    };
    // Beginner (max 3 each; nine points from levels 2-10).
    make(SkillId::PebbleToss, 3, [](const int l) { return SkillLevel{.cost = 2, .cooldownTicks = 24, .value = 110 + 25 * l}; });
    make(SkillId::CatchBreath, 3, [](const int l) { return SkillLevel{.value = 35 * l}; });
    make(SkillId::HyperJump, 5, [](const int l) { return SkillLevel{.cost = std::max(1, 5 - l), .value = 100 + 8 * (l - 1)}; });
    // Warrior.
    make(SkillId::PowerStrike, 15, [](const int l) { return SkillLevel{.cost = 5 + l / 3, .cooldownTicks = 10, .value = 175 + 13 * l}; });
    make(SkillId::Cleave, 15, [](const int l) { return SkillLevel{.cost = 9 + l / 2, .cooldownTicks = 30, .value = 120 + 9 * l}; });
    make(SkillId::IronBody, 10, [](const int l) {
        return SkillLevel{.cost = 14 + l, .cooldownTicks = seconds(2), .durationTicks = seconds(60 + 12 * l), .value = 8 + 3 * l};
    });
    make(SkillId::BladeMastery, 10, [](const int l) { return SkillLevel{.value = 2 * l}; });
    make(SkillId::WarCry, 10, [](const int l) {
        return SkillLevel{.cost = 18 + l, .cooldownTicks = seconds(2), .durationTicks = seconds(60 + 12 * l), .value = 6 + 2 * l};
    });
    make(SkillId::HpBoost, 10, [](const int l) { return SkillLevel{.value = 4 * l}; });
    make(SkillId::Quake, 15, [](const int l) {
        return SkillLevel{.cost = 20 + l, .cooldownTicks = seconds(6) - 12 * l, .value = 150 + 14 * l};
    });
    make(SkillId::Rush, 10, [](const int l) { return SkillLevel{.cost = 16 + l, .cooldownTicks = seconds(4), .value = 160 + 16 * l}; });
    // Mage.
    make(SkillId::ArcaneBolt, 15, [](const int l) { return SkillLevel{.cost = 5 + l / 3, .cooldownTicks = 12, .value = 185 + 14 * l}; });
    make(SkillId::ChainSpark, 15, [](const int l) { return SkillLevel{.cost = 10 + l / 2, .cooldownTicks = 30, .value = 115 + 10 * l}; });
    make(SkillId::ManaShield, 10, [](const int l) {
        return SkillLevel{.cost = 20 + l, .cooldownTicks = seconds(2), .durationTicks = seconds(60 + 20 * l), .value = 30 + 4 * l};
    });
    make(SkillId::SpellMastery, 10, [](const int l) { return SkillLevel{.value = 3 * l}; });
    make(SkillId::Swiftwind, 10, [](const int l) {
        return SkillLevel{.cost = 18 + l, .cooldownTicks = seconds(2), .durationTicks = seconds(60 + 12 * l), .value = 5 + l};
    });
    make(SkillId::MpBoost, 10, [](const int l) { return SkillLevel{.value = 5 * l}; });
    make(SkillId::Starfall, 15, [](const int l) {
        return SkillLevel{.cost = 26 + l, .cooldownTicks = seconds(5) - 8 * l, .value = 125 + 11 * l};
    });
    make(SkillId::Bloom, 10, [](const int l) {
        return SkillLevel{.cost = 26 + 2 * l, .cooldownTicks = seconds(14) - 30 * l, .value = 18 + 3 * l};
    });
    return defs;
}

const std::array<SkillDef, kSkillCount>& defs()
{
    static const std::array<SkillDef, kSkillCount> d = buildDefs();
    return d;
}

std::string secondsText(const int ticks)
{
    if (ticks % kTickRate == 0) {
        return std::format("{}s", ticks / kTickRate);
    }
    return std::format("{:.1f}s", static_cast<float>(ticks) / kTickRate);
}

} // namespace

const SkillInfo& skillInfo(const SkillId id)
{
    return infos()[static_cast<std::size_t>(id)];
}

const SkillDef& skillDef(const SkillId id)
{
    return defs()[static_cast<std::size_t>(id)];
}

std::span<const SkillId> jobSkills(const Job job)
{
    static constexpr std::array<SkillId, 3> kBeginner{SkillId::PebbleToss, SkillId::CatchBreath, SkillId::HyperJump};
    static constexpr std::array<SkillId, 8> kWarrior{SkillId::PowerStrike, SkillId::Cleave, SkillId::IronBody, SkillId::BladeMastery,
                                                     SkillId::WarCry, SkillId::HpBoost, SkillId::Quake, SkillId::Rush};
    static constexpr std::array<SkillId, 8> kMage{SkillId::ArcaneBolt, SkillId::ChainSpark, SkillId::ManaShield, SkillId::SpellMastery,
                                                  SkillId::Swiftwind, SkillId::MpBoost, SkillId::Starfall, SkillId::Bloom};
    switch (job) {
    case Job::Warrior: return kWarrior;
    case Job::Mage: return kMage;
    default: return kBeginner;
    }
}

const SkillLevel& skillLevelValues(const SkillId id, const int level)
{
    const SkillDef& d = skillDef(id);
    return d.levels[static_cast<std::size_t>(std::clamp(level, 1, static_cast<int>(d.levels.size())) - 1)];
}

int skillTargets(const SkillId id, const int level)
{
    switch (id) {
    case SkillId::Cleave: return 4 + level / 5;
    case SkillId::Quake: return 6 + level / 5;
    case SkillId::Rush: return 8;
    case SkillId::ArcaneBolt: return level >= 8 ? 2 + (level - 8) / 4 : 1;
    case SkillId::ChainSpark: return 3 + level / 5;
    case SkillId::Starfall: return 8 + level / 5;
    default: return 1;
    }
}

std::string skillEffectText(const SkillId id, const int level)
{
    const SkillDef& d = skillDef(id);
    if (level <= 0 || level > static_cast<int>(d.levels.size())) {
        return "";
    }
    const SkillLevel& l = skillLevelValues(id, level);
    const int t = skillTargets(id, level);
    const std::string targets = t > 1 ? std::format(" to up to {} monsters", t) : "";
    switch (id) {
    case SkillId::PebbleToss: return std::format("{}% damage, {} mp", l.value, l.cost);
    case SkillId::CatchBreath: return std::format("recovery +{}%", l.value);
    case SkillId::HyperJump: return std::format("burst {}%, {} mp", l.value, l.cost);
    case SkillId::PowerStrike: return std::format("{}% damage, {} mp", l.value, l.cost);
    case SkillId::Cleave: return std::format("{}% damage{}, {} mp", l.value, targets, l.cost);
    case SkillId::IronBody: return std::format("defense +{} for {}, {} mp", l.value, secondsText(l.durationTicks), l.cost);
    case SkillId::BladeMastery: return std::format("attack +{}%, critical +{}%", l.value, level);
    case SkillId::WarCry: return std::format("attack +{}% for {}, {} mp", l.value, secondsText(l.durationTicks), l.cost);
    case SkillId::HpBoost: return std::format("max hp +{}%", l.value);
    case SkillId::Quake: return std::format("{}% damage{}, stun, {} mp, cooldown {}", l.value, targets, l.cost, secondsText(l.cooldownTicks));
    case SkillId::Rush: return std::format("{}% damage{}, {} mp, cooldown {}", l.value, targets, l.cost, secondsText(l.cooldownTicks));
    case SkillId::ArcaneBolt: return std::format("{}% magic{}, {} mp", l.value, targets, l.cost);
    case SkillId::ChainSpark: return std::format("{}% magic{}, {} mp", l.value, targets, l.cost);
    case SkillId::ManaShield: return std::format("{}% of damage from mp for {}, {} mp", l.value, secondsText(l.durationTicks), l.cost);
    case SkillId::SpellMastery: return std::format("magic +{}%, critical +{}%", l.value, level / 2 + 1);
    case SkillId::Swiftwind:
        return std::format("speed +{}%, jump +{}% for {}, {} mp", l.value, l.value / 2, secondsText(l.durationTicks), l.cost);
    case SkillId::MpBoost: return std::format("max mp +{}%", l.value);
    case SkillId::Starfall: return std::format("2 x {}% magic{}, {} mp, cooldown {}", l.value, targets, l.cost, secondsText(l.cooldownTicks));
    case SkillId::Bloom: return std::format("heals {}% hp + magic, {} mp, cooldown {}", l.value, l.cost, secondsText(l.cooldownTicks));
    case SkillId::Count: break;
    }
    return "";
}

} // namespace realm::game
