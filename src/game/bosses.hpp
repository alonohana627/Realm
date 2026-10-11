#pragma once

// The three area bosses: big multi-part versions of their region's
// creatures (Thornbark the old tree, Frostmane the snow beast, Murkmaw the
// gloom), each with three or four telegraphed moves picked so the same move
// never comes twice in a row, faster below 60% and 30% health. They come
// back on a long timer after they fall, like MapleStory's area bosses. The
// boss step only asks for things (shots, hazards to warn about and drop or
// raise, a shockwave, minions); the session makes them.

#include "maps.hpp"
#include "monsters.hpp"
#include "rules.hpp"

#include <gamelib/core/combat.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/rng.hpp>
#include <gamelib/core/tile_collision.hpp>

#include <string_view>
#include <vector>

namespace realm::game {

enum class BossAction : std::uint8_t { None, Stomp, Hazards, Summon, Throw, Charge, Volley, Teleport, Dive };

enum class BossState : std::uint8_t { Intro, Idle, Windup, Act, Recover, Defeat };

// The look of a hostile shot (the app draws each its own way).
enum class ShotLook : std::uint8_t { Acorn, Snowball, Gloom, Wave, Bolt, Star, Pebble };

struct BossInfo {
    std::string_view name;
    std::string_view title;   // "the old tree of the grove"
    int level = 1;
    bool flying = false;
    gamelib::core::Vec2 size;
    float speed = 1.0f;
    std::vector<BossAction> actions;
    SpeciesId minion = SpeciesId::Stumpling;
    ShotLook shot = ShotLook::Acorn;
    bool hazardsFromBelow = false; // roots rise from the floor instead of falling
    int respawnTicks = 0;
};
[[nodiscard]] const BossInfo& bossInfo(BossId id);
[[nodiscard]] int bossHp(BossId id);
[[nodiscard]] int bossAttack(BossId id);
[[nodiscard]] int bossXp(BossId id);

struct BossShotRequest {
    gamelib::core::Vec2 at;
    gamelib::core::Vec2 velocity;
    float gravity = 0.0f;
    gamelib::core::Vec2 size{30.0f, 30.0f};
    int lifetime = 240;
    ShotLook look = ShotLook::Acorn;
    bool groundWave = false;
};

struct BossStep {
    std::vector<BossShotRequest> shots;
    std::vector<float> hazards; // x positions to warn about, then strike
    int summon = 0;
    bool landed = false;
    bool roared = false;        // a move starts (a sound and a face)
    bool phaseChanged = false;
    bool finished = false;      // the defeat is over
    bool contact = false;       // the body hurts on touch
};

struct Boss {
    BossId id = BossId::Thornbark;
    gamelib::core::Rect box;
    gamelib::core::Rect previous;
    gamelib::core::Vec2 velocity;
    gamelib::core::Rect bounds;
    int facing = -1;
    int hp = 1;
    int maxHp = 1;
    int phase = 0;
    BossState state = BossState::Intro;
    BossAction action = BossAction::None;
    BossAction lastAction = BossAction::None;
    int stateTicks = 0;
    int stateLength = 1;
    int repeats = 0;
    bool grounded = false;
    float fade = 1.0f;          // teleport: 0 gone .. 1 there
    int lastHitTick = -1000;
    gamelib::core::Vec2 diveTarget;
    gamelib::core::Vulnerable vulnerable;

    [[nodiscard]] const BossInfo& info() const { return bossInfo(id); }
    [[nodiscard]] bool alive() const { return state != BossState::Defeat; }
    [[nodiscard]] bool fighting() const { return alive() && state != BossState::Intro; }
    [[nodiscard]] float progress() const
    {
        return stateLength > 0 ? std::min(1.0f, static_cast<float>(stateTicks) / static_cast<float>(stateLength)) : 0.0f;
    }
};

[[nodiscard]] Boss makeBoss(BossId id, gamelib::core::Vec2 feet, gamelib::core::Rect bounds);
BossStep stepBoss(Boss& boss, gamelib::core::Rect hero, const gamelib::core::TileWorld& world, gamelib::core::Rng& rng,
                  int tick);
// A hit landed: returns true when it was the last one.
bool hurtBoss(Boss& boss, int damage, int tick);

} // namespace realm::game
