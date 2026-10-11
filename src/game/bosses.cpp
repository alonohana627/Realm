#include "bosses.hpp"

#include <gamelib/ai/opponent.hpp>
#include <gamelib/core/state_machine.hpp>

#include <array>
#include <cmath>

namespace realm::game {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr float kGravity = 0.55f;
constexpr int kIntroTicks = 100;
constexpr int kDefeatTicks = 110;

const std::array<BossInfo, kBossCount>& table()
{
    using A = BossAction;
    static const std::array<BossInfo, kBossCount> t{{
        {"Thornbark", "the old tree of the grove", 20, false, {150, 210}, 0.9f, {A::Hazards, A::Throw, A::Stomp, A::Summon},
         SpeciesId::Stumpling, ShotLook::Acorn, true, 4 * 60 * kTickRate},
        {"Frostmane", "the beast of the summit", 28, false, {190, 170}, 1.3f, {A::Charge, A::Hazards, A::Volley, A::Stomp},
         SpeciesId::Iceling, ShotLook::Snowball, false, 5 * 60 * kTickRate},
        {"Murkmaw", "the gloom under the harbor", 35, true, {190, 190}, 1.6f,
         {A::Teleport, A::Volley, A::Dive, A::Hazards, A::Summon}, SpeciesId::CaveFlitter, ShotLook::Gloom, false, 6 * 60 * kTickRate},
    }};
    return t;
}

int sign(const float v)
{
    return v < 0.0f ? -1 : 1;
}

void enter(Boss& b, const BossState state, const int length)
{
    b.state = state;
    b.stateTicks = 0;
    b.stateLength = std::max(length, 1);
}

int windupFor(const Boss& b)
{
    return 44 - b.phase * 9;
}

void moveGround(Boss& b, const gamelib::core::TileWorld& world, BossStep& step)
{
    b.velocity.y = std::min(b.velocity.y + kGravity, 16.0f);
    const bool was = b.grounded;
    const gamelib::core::BoxMove move = gamelib::core::moveBox(b.box, b.velocity, world);
    b.box = move.box;
    b.grounded = move.contacts.ground;
    if (move.contacts.blockedX) {
        b.velocity.x = 0.0f;
    }
    if (b.grounded) {
        b.velocity.y = 0.0f;
    }
    // Never out of the arena.
    b.box.x = std::clamp(b.box.x, b.bounds.left(), b.bounds.right() - b.box.width);
    if (b.grounded && !was) {
        step.landed = true;
    }
}

} // namespace

const BossInfo& bossInfo(const BossId id)
{
    return table()[static_cast<std::size_t>(id)];
}

int bossHp(const BossId id)
{
    switch (id) {
    case BossId::Thornbark: return 12000;
    case BossId::Frostmane: return 24000;
    case BossId::Murkmaw: return 45000;
    case BossId::Count: break;
    }
    return 1000;
}

int bossAttack(const BossId id)
{
    return 18 + bossInfo(id).level * 4;
}

int bossXp(const BossId id)
{
    const int l = bossInfo(id).level;
    return 300 + l * l * 6;
}

Boss makeBoss(const BossId id, const Vec2 feet, const Rect bounds)
{
    Boss b;
    b.id = id;
    const BossInfo& info = bossInfo(id);
    b.box = Rect::fromBottomCenter(feet, info.size);
    if (info.flying) {
        b.box.y -= 140.0f;
    }
    b.previous = b.box;
    b.bounds = bounds;
    b.hp = bossHp(id);
    b.maxHp = b.hp;
    b.vulnerable = gamelib::core::Vulnerable{b.box, 2};
    enter(b, BossState::Intro, kIntroTicks);
    return b;
}

bool hurtBoss(Boss& b, const int damage, const int tick)
{
    if (!b.fighting()) {
        return false;
    }
    b.hp -= damage;
    b.lastHitTick = tick;
    if (b.hp <= 0) {
        b.hp = 0;
        enter(b, BossState::Defeat, kDefeatTicks);
        b.velocity = {};
        return true;
    }
    return false;
}

BossStep stepBoss(Boss& b, const Rect hero, const gamelib::core::TileWorld& world, gamelib::core::Rng& rng, const int tick)
{
    (void)tick;
    BossStep step;
    b.previous = b.box;
    b.vulnerable.tick();
    ++b.stateTicks;
    const BossInfo& info = b.info();
    const Vec2 c = b.box.center();
    const Vec2 h = hero.center();
    const float dx = h.x - c.x;

    // Phases at 60% and 30% health, only ever advancing.
    const int phase = b.hp * 10 < b.maxHp * 3 ? 2 : b.hp * 10 < b.maxHp * 6 ? 1 : 0;
    if (phase > b.phase) {
        b.phase = phase;
        step.phaseChanged = true;
    }

    if (b.state == BossState::Defeat) {
        b.fade = 1.0f;
        if (!info.flying) {
            b.velocity.x = 0.0f;
            moveGround(b, world, step);
        } else {
            b.box.y += 1.5f;
        }
        step.finished = b.stateTicks >= b.stateLength;
        b.vulnerable.setHurtbox(b.box);
        return step;
    }
    if (b.state == BossState::Intro) {
        if (b.stateTicks == 30) {
            step.roared = true;
        }
        if (!info.flying) {
            moveGround(b, world, step);
        }
        if (b.stateTicks >= b.stateLength) {
            enter(b, BossState::Idle, 60);
        }
        b.vulnerable.setHurtbox(b.box);
        return step;
    }

    step.contact = b.fade > 0.6f;
    switch (b.state) {
    case BossState::Idle: {
        b.facing = sign(dx);
        if (info.flying) {
            const Vec2 target{h.x - static_cast<float>(sign(dx)) * 260.0f, b.bounds.bottom() - 330.0f + std::sin(static_cast<float>(b.stateTicks) * 0.05f) * 30.0f};
            const Vec2 d = target - c;
            b.velocity = b.velocity + (d * 0.02f - b.velocity) * 0.08f;
            b.box.x += b.velocity.x;
            b.box.y += b.velocity.y;
        } else {
            const float want = std::abs(dx) > 220.0f ? static_cast<float>(sign(dx)) * info.speed * (1.0f + 0.25f * static_cast<float>(b.phase)) : 0.0f;
            b.velocity.x += (want - b.velocity.x) * 0.15f;
            moveGround(b, world, step);
        }
        if (b.stateTicks >= b.stateLength) {
            std::vector<BossAction> options;
            for (const BossAction a : info.actions) {
                if (a != b.lastAction) {
                    options.push_back(a);
                }
            }
            b.action = options[static_cast<std::size_t>(gamelib::core::nextPattern(rng, static_cast<int>(options.size())))];
            b.lastAction = b.action;
            enter(b, BossState::Windup, windupFor(b));
            step.roared = true;
            b.velocity.x = 0.0f;
        }
        break;
    }
    case BossState::Windup:
        b.facing = sign(dx);
        if (!info.flying) {
            b.velocity.x *= 0.8f;
            moveGround(b, world, step);
        } else {
            b.velocity = b.velocity * 0.85f;
            b.box.x += b.velocity.x;
            b.box.y += b.velocity.y;
        }
        if (b.stateTicks >= b.stateLength) {
            int length = 30;
            switch (b.action) {
            case BossAction::Stomp:
                length = 90;
                b.velocity = {std::clamp(dx / 38.0f, -9.0f, 9.0f), -15.0f};
                b.grounded = false;
                break;
            case BossAction::Charge:
                length = 80;
                b.velocity.x = static_cast<float>(b.facing) * (9.0f + 1.5f * static_cast<float>(b.phase));
                break;
            case BossAction::Teleport: length = 50; break;
            case BossAction::Dive:
                length = 70;
                b.diveTarget = {h.x, b.bounds.bottom() - b.box.height * 0.5f};
                break;
            default: break;
            }
            enter(b, BossState::Act, length);
        }
        break;
    case BossState::Act: {
        bool done = b.stateTicks >= b.stateLength;
        switch (b.action) {
        case BossAction::Stomp:
            moveGround(b, world, step);
            if (step.landed && b.stateTicks > 4) {
                // Shockwaves both ways along the floor.
                for (const float d : {-1.0f, 1.0f}) {
                    step.shots.push_back({.at = {c.x + d * b.box.width * 0.5f, b.box.bottom() - 26.0f},
                                          .velocity = {d * (7.0f + 1.5f * static_cast<float>(b.phase)), 0.0f}, .size = {46.0f, 46.0f},
                                          .lifetime = 120, .look = ShotLook::Wave, .groundWave = true});
                }
                done = true;
            }
            break;
        case BossAction::Hazards:
            if (b.stateTicks == 1) {
                const int count = 4 + b.phase * 2;
                step.hazards.push_back(h.x);
                for (int i = 1; i < count; ++i) {
                    step.hazards.push_back(rng.uniform(b.bounds.left() + 80.0f, b.bounds.right() - 80.0f));
                }
            }
            if (!info.flying) {
                moveGround(b, world, step);
            }
            break;
        case BossAction::Summon:
            if (b.stateTicks == 1) {
                step.summon = 2 + b.phase;
            }
            if (!info.flying) {
                moveGround(b, world, step);
            }
            break;
        case BossAction::Throw:
            if (b.stateTicks == 1 || b.stateTicks == 12 || (b.phase > 0 && b.stateTicks == 24)) {
                const Vec2 from{c.x + static_cast<float>(b.facing) * b.box.width * 0.3f, b.box.top() + 40.0f};
                const Vec2 to{h.x + rng.uniform(-60.0f, 60.0f), hero.bottom() - 20.0f};
                const Vec2 v = gamelib::ai::lobVelocity(from, to, 0.35f, 50, {12.0f, 16.0f});
                step.shots.push_back({.at = from, .velocity = v, .gravity = 0.35f, .size = {40.0f, 40.0f}, .lifetime = 200, .look = info.shot});
            }
            moveGround(b, world, step);
            break;
        case BossAction::Charge:
            b.velocity.x *= b.stateTicks > 60 ? 0.9f : 1.0f;
            moveGround(b, world, step);
            if (b.box.left() <= b.bounds.left() + 1.0f || b.box.right() >= b.bounds.right() - 1.0f) {
                step.landed = true; // a crash into the wall
                done = true;
            }
            break;
        case BossAction::Volley:
            if (b.stateTicks == 1 || (b.phase > 0 && b.stateTicks == 18) || (b.phase > 1 && b.stateTicks == 30)) {
                const int n = 5 + b.phase;
                const Vec2 from{c.x + static_cast<float>(b.facing) * b.box.width * 0.35f, c.y - 10.0f};
                const float aim = std::atan2(h.y - from.y, h.x - from.x);
                for (int i = 0; i < n; ++i) {
                    const float a = aim + (static_cast<float>(i) - static_cast<float>(n - 1) * 0.5f) * 0.2f;
                    step.shots.push_back({.at = from, .velocity = {std::cos(a) * 6.5f, std::sin(a) * 6.5f}, .size = {34.0f, 34.0f},
                                          .lifetime = 220, .look = info.shot});
                }
            }
            if (!info.flying) {
                moveGround(b, world, step);
            }
            break;
        case BossAction::Teleport:
            if (b.stateTicks < 20) {
                b.fade = 1.0f - static_cast<float>(b.stateTicks) / 20.0f;
            } else if (b.stateTicks == 20) {
                const float side = rng.chance(0.5f) ? -1.0f : 1.0f;
                const float x = std::clamp(h.x + side * 300.0f, b.bounds.left() + b.box.width * 0.5f, b.bounds.right() - b.box.width * 0.5f);
                b.box.x = x - b.box.width * 0.5f;
                b.box.y = b.bounds.bottom() - 360.0f;
                b.previous = b.box;
            } else {
                b.fade = std::min(1.0f, static_cast<float>(b.stateTicks - 20) / 20.0f);
            }
            break;
        case BossAction::Dive: {
            const float t = b.progress();
            if (t < 0.35f) {
                b.box.y -= 3.0f; // rise
            } else {
                const Vec2 d = b.diveTarget - b.box.center();
                const float len = std::max(std::sqrt(d.x * d.x + d.y * d.y), 1.0f);
                const float speed = 14.0f + 2.0f * static_cast<float>(b.phase);
                if (len > speed) {
                    b.box.x += d.x / len * speed;
                    b.box.y += d.y / len * speed;
                } else {
                    step.landed = true;
                    for (const float dd : {-1.0f, 1.0f}) {
                        step.shots.push_back({.at = {b.box.center().x + dd * 60.0f, b.bounds.bottom() - 26.0f}, .velocity = {dd * 7.0f, 0.0f},
                                              .size = {46.0f, 46.0f}, .lifetime = 110, .look = ShotLook::Wave, .groundWave = true});
                    }
                    done = true;
                }
            }
            break;
        }
        case BossAction::None: done = true; break;
        }
        if (done) {
            enter(b, BossState::Recover, 50 - b.phase * 10);
            b.fade = 1.0f;
        }
        break;
    }
    case BossState::Recover:
        if (!info.flying) {
            b.velocity.x *= 0.8f;
            moveGround(b, world, step);
        } else {
            const float floorGap = b.bounds.bottom() - b.box.bottom();
            if (floorGap < 160.0f) {
                b.box.y -= 4.0f; // lift off after a dive
            }
        }
        if (b.stateTicks >= b.stateLength) {
            enter(b, BossState::Idle, 70 - b.phase * 18);
        }
        break;
    default: break;
    }
    if (info.flying) {
        b.box.x = std::clamp(b.box.x, b.bounds.left(), b.bounds.right() - b.box.width);
        b.box.y = std::clamp(b.box.y, b.bounds.top() + 40.0f, b.bounds.bottom() - b.box.height);
    }
    b.vulnerable.setHurtbox(b.box.expanded(-10.0f));
    return step;
}

} // namespace realm::game
