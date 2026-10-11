#pragma once

// Monsters: the fourteen species (a drawn archetype, a behaviour, a level,
// a size and a drop), their numbers by level and elite status, and the
// runtime monster with its small brain: wander on its own platform without
// ever falling off, notice the hero (aggressive kinds) or only answer a hit
// (passive kinds), wind up and lunge, hop, charge or swoop, flinch when hit,
// stagger when stunned, pop when defeated.

#include "rules.hpp"

#include <gamelib/core/combat.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/tile_collision.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace realm::game {

// The drawn creature (monster.frag's kinds).
enum class Archetype : std::uint8_t { Bellcap, Mochi, Turbanshell, Crab, Gull, Pudgy, Stumpling, Wisp, Pebblet, Flitter, Count };

enum class Behavior : std::uint8_t {
    Walker,  // wanders, walks at the hero when angry, a short lunge
    Hopper,  // moves in hops, a big hop at the hero
    Charger, // wanders, then a fast charge along its platform
    Flyer,   // floats over its home, drifts at the hero and swoops
};

enum class SpeciesId : std::uint8_t {
    Sproutcap, Dewdrop, Swirlsnail,        // Petal Isle
    Pinchy, Puffgull, BarrelHog,           // Saltwhistle coast
    Stumpling, Glowcap, Leafwisp,          // Fernwood
    Snowhog, Pebblet, Iceling,             // Windcrest Pass
    CaveFlitter, Runeshell,                // the Undercroft
    Count
};
inline constexpr int kSpeciesCount = static_cast<int>(SpeciesId::Count);

struct Species {
    std::string_view name;
    Archetype look = Archetype::Bellcap;
    Behavior behavior = Behavior::Walker;
    Region region = Region::Island;
    int level = 1;
    float speed = 1.0f;       // walk speed, units per tick
    gamelib::core::Vec2 size; // collision box
    bool aggressive = false;  // notices the hero; passive kinds only answer a hit
};

[[nodiscard]] const Species& species(SpeciesId id);
[[nodiscard]] constexpr int dropItem(const SpeciesId id) { return 100 + static_cast<int>(id); }

[[nodiscard]] int monsterHp(int level, bool elite);
[[nodiscard]] int monsterAttack(int level, bool elite);
[[nodiscard]] int monsterDefense(int level);
[[nodiscard]] int monsterXp(int level, bool elite);

enum class MonsterState : std::uint8_t { Spawn, Idle, Move, Windup, Attack, Hit, Stun, Defeat };

struct Monster {
    SpeciesId species = SpeciesId::Sproutcap;
    bool elite = false;
    int level = 1;
    int spawn = -1;               // the spawner point it came from (-1: summoned)
    gamelib::core::Rect box;
    gamelib::core::Rect previous; // last tick's box, for drawing
    gamelib::core::Vec2 velocity;
    gamelib::core::Vec2 home;     // flyers float around it
    float patrolLeft = 0.0f;      // the run of floor it keeps to
    float patrolRight = 0.0f;
    int facing = -1;
    MonsterState state = MonsterState::Spawn;
    int stateTicks = 0;
    int stateLength = 1;
    int hp = 1;
    int maxHp = 1;
    int attackCooldown = 0;
    int aggro = 0;                // ticks left angry at the hero
    int hopTimer = 0;
    int wanderTimer = 0;
    bool wandering = true;
    bool grounded = false;
    int lastHitTick = -1000;      // the health bar shows for a while after a hit
    std::uint8_t seed = 0;
    gamelib::core::Vulnerable vulnerable;

    [[nodiscard]] const Species& kind() const { return realm::game::species(species); }
    [[nodiscard]] bool alive() const { return state != MonsterState::Defeat; }
    [[nodiscard]] bool active() const { return alive() && state != MonsterState::Spawn; }
    [[nodiscard]] float progress() const
    {
        return stateLength > 0 ? std::min(1.0f, static_cast<float>(stateTicks) / static_cast<float>(stateLength)) : 0.0f;
    }
};

struct HeroView {
    gamelib::core::Rect box;
    bool alive = true;
};

struct MonsterStep {
    bool lunged = false;     // an attack started (a sound)
    bool landed = false;
    bool finished = false;   // the defeat animation ended: remove it
};

[[nodiscard]] Monster makeMonster(SpeciesId id, int level, bool elite, gamelib::core::Vec2 feet, float patrolLeft,
                                  float patrolRight, std::uint8_t seed);

MonsterStep stepMonster(Monster& m, const HeroView& hero, const gamelib::core::TileWorld& world, int tick);

// A hit landed: knockback, a flinch (or a stun), and the defeat when hp runs out.
void hurtMonster(Monster& m, int damage, int direction, int tick, int stunTicks = 0, float push = 1.0f);

// The body hurts on touch (not while spawning or popping).
[[nodiscard]] bool monsterTouches(const Monster& m);

} // namespace realm::game
