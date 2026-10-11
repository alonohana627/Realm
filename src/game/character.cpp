#include "character.hpp"

#include <algorithm>

namespace realm::game {

namespace {

// helpers

// Walking and jumping at 100% Speed and Jump (world units and ticks).
constexpr float kWalkSpeed = 5.0f;     // 300 units a second: six tiles
constexpr float kJumpHeight = 152.0f;  // a little over three tiles
constexpr int kJumpApexTicks = 22;
// The hyper jump at 100%: a burst forward with a small lift, the input
// locked for 12 ticks and no air drag for 26 so it carries.
constexpr float kHyperForward = 12.5f;
constexpr float kHyperRise = 5.0f;
constexpr float kHyperUp = 15.5f;
constexpr int kHyperLockTicks = 12;
constexpr int kHyperGlideTicks = 26;

CoreStats autoWeights(const Job job)
{
    switch (job) {
    case Job::Warrior: return CoreStats{{3, 1, 0, 1}};
    case Job::Mage: return CoreStats{{0, 1, 3, 1}};
    default: return CoreStats{{1, 1, 1, 1}};
    }
}

int skillValue(const Character& c, const SkillId id)
{
    const int level = c.skillLevel(id);
    return level > 0 ? skillLevelValues(id, level).value : 0;
}

gamelib::core::Inventory bagFor()
{
    return gamelib::core::Inventory{itemRegistry(), kBagSlots};
}

} // namespace

const gamelib::core::ExperienceCurve& experienceCurve()
{
    // Gentle early levels (an island level in a few minutes), steeper later.
    static const gamelib::core::ExperienceCurve curve = gamelib::core::ExperienceCurve::fromSteps(
        kMaxLevel, [](const int level) { return static_cast<std::int64_t>(15 + 10 * level + 5 * level * level + level * level * level / 6); });
    return curve;
}

Character::Character()
    : points{CoreStats{{kStartCore, kStartCore, kStartCore, kStartCore}}}
    , bags{bagFor(), bagFor(), bagFor()}
    , gear{itemRegistry(), kSlotCount}
{
    for (int i = 0; i < kSkillCount; ++i) {
        skills.define(skillDef(static_cast<SkillId>(i)));
    }
    const Derived d = derived();
    hp = {.current = d.maxHp, .max = d.maxHp};
    mp = {.current = d.maxMp, .max = d.maxMp};
}

int Character::countOf(const int item) const
{
    return bag(tabOf(item)).countOf(item);
}

int Character::add(const gamelib::core::ItemStack& stack)
{
    return bag(tabOf(stack.item)).add(stack);
}

int Character::gearBonus(const Bonus bonus) const
{
    int sum = 0;
    for (int s = 0; s < gear.size(); ++s) {
        sum += stackBonus(gear.at(s), bonus);
    }
    return sum;
}

int Character::buffShield() const
{
    int shield = 0;
    for (const auto& b : buffs.all()) {
        shield = std::max(shield, b.effect.shield);
    }
    return shield;
}

Derived Character::derivedWith(const CoreStats& base) const
{
    Derived d;
    d.core = base;
    d.core[Core::Str] += gearBonus(Bonus::Str);
    d.core[Core::Agi] += gearBonus(Bonus::Agi);
    d.core[Core::Int] += gearBonus(Bonus::Int);
    d.core[Core::Vit] += gearBonus(Bonus::Vit);
    const int str = d.core[Core::Str];
    const int agi = d.core[Core::Agi];
    const int intel = d.core[Core::Int];
    const int vit = d.core[Core::Vit];
    const int past = std::max(level - kJobLevel, 0);
    int hpBase = 50 + 12 * (level - 1) + 10 * vit + 2 * str + gearBonus(Bonus::Hp);
    int mpBase = 20 + 5 * (level - 1) + 6 * intel + gearBonus(Bonus::Mp);
    if (job == Job::Warrior) {
        hpBase += 14 * past + 60;
        mpBase += 1 * past;
    } else if (job == Job::Mage) {
        hpBase += 2 * past;
        mpBase += 11 * past + 60;
    }
    d.maxHp = hpBase * (100 + skillValue(*this, SkillId::HpBoost)) / 100;
    d.maxMp = mpBase * (100 + skillValue(*this, SkillId::MpBoost)) / 100;
    int attackPercent = skillValue(*this, SkillId::BladeMastery);
    int defenseBuff = 0;
    int buffSpeed = 0;
    int buffJump = 0;
    for (const auto& b : buffs.all()) {
        attackPercent += b.effect.attackPercent;
        defenseBuff += b.effect.defense;
        buffSpeed += b.effect.speed;
        buffJump += b.effect.jump;
    }
    d.attack = (gearBonus(Bonus::Attack) + 2 * str + agi / 2 + level) * (100 + attackPercent) / 100;
    d.magic = (gearBonus(Bonus::Magic) + 2 * intel + agi / 4 + level) * (100 + skillValue(*this, SkillId::SpellMastery)) / 100;
    d.defense = gearBonus(Bonus::Defense) + vit + str / 4 + defenseBuff;
    const int mastery = skillLevel(SkillId::BladeMastery) + (skillLevel(SkillId::SpellMastery) + 1) / 2;
    d.crit = std::min(5 + agi / 6 + gearBonus(Bonus::Crit) + mastery, 60);
    d.speed = std::clamp(100 + (agi - kStartCore) / 3 + gearBonus(Bonus::Speed) + buffSpeed, 70, kSpeedCap);
    d.jump = std::clamp(100 + (agi - kStartCore) / 6 + gearBonus(Bonus::Jump) + buffJump, 80, kJumpCap);
    return d;
}

bool Character::spendPoint(const Core stat)
{
    if (!points.spend(stat)) {
        return false;
    }
    refreshResources();
    return true;
}

Core Character::nextAuto() const
{
    return points.nextAuto(autoWeights(job));
}

int Character::autoAssign()
{
    const int spent = points.autoAssign(autoWeights(job));
    refreshResources();
    return spent;
}

gamelib::core::LevelUp Character::gainXp(const std::int64_t amount)
{
    const gamelib::core::LevelUp up = experienceCurve().gain(xp, amount);
    if (up.happened()) {
        for (int l = up.from + 1; l <= up.to; ++l) {
            points.grant(kPointsPerLevel);
            // Beginners earn one skill point a level (2-10), a job three.
            skills.addPoints(job == Job::Beginner ? (l <= kJobLevel ? 1 : 0) : 3);
        }
        level = up.to;
        refreshResources();
        hp.refill();
        mp.refill();
    }
    return up;
}

float Character::xpProgress() const
{
    return level >= kMaxLevel ? 1.0f : experienceCurve().progress(xp);
}

std::int64_t Character::xpToNext() const
{
    return level >= kMaxLevel ? 0 : experienceCurve().toNextLevel(xp);
}

void Character::advance(const Job to)
{
    if (job != Job::Beginner || to == Job::Beginner) {
        return;
    }
    job = to;
    // The levels past 10 a late beginner skipped count too.
    skills.addPoints(1 + 3 * std::max(level - kJobLevel, 0));
    refreshResources();
    hp.refill();
    mp.refill();
}

void Character::refreshResources()
{
    const Derived d = derived();
    hp.max = d.maxHp;
    mp.max = d.maxMp;
    hp.current = std::min(hp.current, hp.max);
    mp.current = std::min(mp.current, mp.max);
}

gamelib::core::PlatformerTuning Character::tuning() const
{
    const Derived d = derived();
    gamelib::core::PlatformerTuning t;
    t.maxSpeed = kWalkSpeed;
    t.groundAccel = 1.1f;
    t.groundDecel = 1.4f;
    // Little air control: a committed MapleStory jump.
    t.airAccel = 0.28f;
    t.airDecel = 0.08f;
    t.setJumpArc(kJumpHeight, kJumpApexTicks);
    t.maxFallSpeed = 14.0f;
    t.fallGravity = 1.1f;
    t.jumpCut = 1.0f; // a fixed arc: releasing early does not cut the jump
    t.coyoteTicks = 5;
    t.jumpBufferTicks = 6;
    t.cornerCorrection = 10.0f;
    t.climbSpeed = 3.4f;
    t.climbJumpSpeed = {4.2f, 7.5f};
    t.dropThroughTicks = 12;
    const int hyper = skillLevel(SkillId::HyperJump);
    if (hyper > 0) {
        const float power = static_cast<float>(skillLevelValues(SkillId::HyperJump, hyper).value) / 100.0f;
        t.hyperForward = {kHyperForward * power, kHyperRise};
        t.hyperUp = kHyperUp * power;
        t.hyperLockTicks = kHyperLockTicks;
        t.hyperGlideTicks = kHyperGlideTicks;
    }
    return gamelib::core::scaledTuning(t, {.speed = static_cast<float>(d.speed) / 100.0f,
                                           .jump = static_cast<float>(d.jump) / 100.0f,
                                           .speedCap = static_cast<float>(kSpeedCap) / 100.0f,
                                           .jumpCap = static_cast<float>(kJumpCap) / 100.0f});
}

} // namespace realm::game
