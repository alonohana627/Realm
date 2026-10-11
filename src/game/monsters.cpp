#include "monsters.hpp"

#include <gamelib/core/enemy_motion.hpp>

#include <array>
#include <cmath>

namespace realm::game {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr float kGravity = 0.6f;
constexpr float kMaxFall = 13.0f;
constexpr int kSpawnTicks = 30;
constexpr int kHitTicks = 16;
constexpr int kDefeatTicks = 42;

const std::array<Species, kSpeciesCount>& table()
{
    using A = Archetype;
    using B = Behavior;
    using R = Region;
    static const std::array<Species, kSpeciesCount> t{{
        {"Sproutcap", A::Bellcap, B::Hopper, R::Island, 1, 0.9f, {52, 64}, false},
        {"Dewdrop", A::Mochi, B::Hopper, R::Island, 2, 1.0f, {62, 46}, false},
        {"Swirlsnail", A::Turbanshell, B::Walker, R::Island, 4, 0.55f, {72, 58}, false},
        {"Pinchy", A::Crab, B::Walker, R::Coast, 7, 1.35f, {72, 50}, true},
        {"Puffgull", A::Gull, B::Flyer, R::Coast, 9, 1.5f, {62, 50}, true},
        {"Barrel Hog", A::Pudgy, B::Charger, R::Coast, 11, 1.15f, {74, 60}, false},
        {"Stumpling", A::Stumpling, B::Walker, R::Forest, 13, 0.9f, {56, 78}, true},
        {"Glowcap", A::Bellcap, B::Hopper, R::Forest, 15, 1.1f, {58, 72}, false},
        {"Leafwisp", A::Wisp, B::Flyer, R::Forest, 17, 1.4f, {56, 64}, true},
        {"Snowhog", A::Pudgy, B::Charger, R::Mountain, 20, 1.35f, {78, 64}, true},
        {"Pebblet", A::Pebblet, B::Walker, R::Mountain, 22, 0.9f, {64, 58}, true},
        {"Iceling", A::Mochi, B::Hopper, R::Mountain, 24, 1.2f, {66, 50}, true},
        {"Cave Flitter", A::Flitter, B::Flyer, R::Depths, 26, 1.8f, {60, 48}, true},
        {"Runeshell", A::Turbanshell, B::Charger, R::Depths, 29, 0.9f, {82, 64}, true},
    }};
    return t;
}

int sign(const float v)
{
    return v < 0.0f ? -1 : 1;
}

void enter(Monster& m, const MonsterState state, const int length)
{
    m.state = state;
    m.stateTicks = 0;
    m.stateLength = std::max(length, 1);
}

void fall(Monster& m, const gamelib::core::TileWorld& world, MonsterStep& step)
{
    m.velocity.y = std::min(m.velocity.y + kGravity, kMaxFall);
    const bool was = m.grounded;
    const gamelib::core::BoxMove move = gamelib::core::moveBox(m.box, m.velocity, world);
    m.box = move.box;
    m.grounded = move.contacts.ground;
    if (move.contacts.blockedX) {
        m.velocity.x = 0.0f;
    }
    if (move.contacts.blockedY) {
        m.velocity.y = 0.0f;
    }
    step.landed = m.grounded && !was;
}

} // namespace

const Species& species(const SpeciesId id)
{
    return table()[static_cast<std::size_t>(id)];
}

int monsterHp(const int level, const bool elite)
{
    const int hp = 20 + 10 * level + level * level * 6 / 5;
    return elite ? hp * 4 : hp;
}

int monsterAttack(const int level, const bool elite)
{
    const int atk = 7 + level * 16 / 5;
    return elite ? atk * 3 / 2 : atk;
}

int monsterDefense(const int level)
{
    return level * 6 / 5;
}

int monsterXp(const int level, const bool elite)
{
    const int xp = 4 + 3 * level + level * level * 7 / 20;
    return elite ? xp * 4 : xp;
}

Monster makeMonster(const SpeciesId id, const int level, const bool elite, const Vec2 feet, const float patrolLeft,
                    const float patrolRight, const std::uint8_t seed)
{
    const Species& s = species(id);
    Monster m;
    m.species = id;
    m.elite = elite;
    m.level = level;
    const Vec2 size = elite ? s.size * 1.3f : s.size;
    m.box = Rect::fromBottomCenter(feet, size);
    m.previous = m.box;
    m.home = m.box.center();
    m.patrolLeft = patrolLeft;
    m.patrolRight = patrolRight;
    m.facing = (seed & 1) != 0 ? 1 : -1;
    m.hp = monsterHp(level, elite);
    m.maxHp = m.hp;
    m.seed = seed;
    m.hopTimer = 20 + seed % 40;
    m.wanderTimer = 60 + seed % 90;
    m.attackCooldown = 60;
    m.vulnerable = gamelib::core::Vulnerable{m.box, 2};
    enter(m, MonsterState::Spawn, kSpawnTicks);
    return m;
}

bool monsterTouches(const Monster& m)
{
    return m.active();
}

void hurtMonster(Monster& m, const int damage, const int direction, const int tick, const int stunTicks, const float push)
{
    if (!m.alive()) {
        return;
    }
    m.hp -= damage;
    m.lastHitTick = tick;
    m.aggro = 600;
    const bool flyer = m.kind().behavior == Behavior::Flyer;
    if (m.hp <= 0) {
        m.hp = 0;
        enter(m, MonsterState::Defeat, kDefeatTicks);
        m.velocity = flyer ? Vec2{static_cast<float>(direction) * 2.0f, -1.0f} : Vec2{static_cast<float>(direction) * 2.5f, -5.0f};
        return;
    }
    if (stunTicks > 0) {
        enter(m, MonsterState::Stun, stunTicks);
    } else if (m.elite && m.state == MonsterState::Attack) {
        return; // elites shrug off flinches mid-lunge
    } else {
        enter(m, MonsterState::Hit, kHitTicks);
    }
    m.facing = -direction;
    const float k = (m.elite ? 0.5f : 1.0f) * push;
    m.velocity = flyer ? Vec2{static_cast<float>(direction) * 3.0f * k, -1.0f}
                       : Vec2{static_cast<float>(direction) * 3.2f * k, m.grounded ? -2.2f * k : m.velocity.y};
}

MonsterStep stepMonster(Monster& m, const HeroView& hero, const gamelib::core::TileWorld& world, const int tick)
{
    MonsterStep step;
    m.previous = m.box;
    m.vulnerable.tick();
    ++m.stateTicks;
    m.attackCooldown = std::max(0, m.attackCooldown - 1);
    m.aggro = std::max(0, m.aggro - 1);
    const Species& s = m.kind();
    const bool flyer = s.behavior == Behavior::Flyer;
    const Vec2 center = m.box.center();
    const Vec2 heroCenter = hero.box.center();
    const float dx = heroCenter.x - center.x;
    const float dy = heroCenter.y - center.y;

    if (m.state == MonsterState::Defeat) {
        if (!flyer) {
            m.velocity.x *= 0.9f;
            fall(m, world, step);
            if (m.grounded) {
                m.velocity.x = 0.0f;
            }
        } else {
            m.box.x += m.velocity.x;
            m.box.y += m.velocity.y;
            m.velocity = m.velocity * 0.9f;
        }
        step.finished = m.stateTicks >= m.stateLength;
        return step;
    }
    if (m.state == MonsterState::Spawn) {
        if (m.stateTicks >= m.stateLength) {
            enter(m, MonsterState::Idle, 50);
        }
        if (!flyer) {
            m.velocity.x = 0.0f;
            fall(m, world, step);
        }
        m.vulnerable.setHurtbox(m.box);
        return step;
    }

    // Noticing the hero: aggressive kinds on about the same level.
    const float noticeX = flyer ? 380.0f : 320.0f;
    if (hero.alive && s.aggressive && std::abs(dx) < noticeX && std::abs(dy) < (flyer ? 260.0f : 100.0f)) {
        m.aggro = std::max(m.aggro, 90);
    }
    if (!hero.alive) {
        m.aggro = 0;
    }
    const bool chasing = m.aggro > 0;

    if ((m.state == MonsterState::Hit || m.state == MonsterState::Stun) && m.stateTicks >= m.stateLength) {
        enter(m, MonsterState::Move, 40);
        m.attackCooldown = std::max(m.attackCooldown, 30);
    }

    if (flyer) {
        const float t = static_cast<float>(tick) + static_cast<float>(m.seed) * 13.0f;
        Vec2 target = m.home + Vec2{std::sin(t * 0.014f) * 100.0f, std::cos(t * 0.029f) * 28.0f};
        if (chasing) {
            target = heroCenter + Vec2{static_cast<float>(-sign(dx)) * 80.0f, -80.0f};
            const Vec2 fromHome = target - m.home;
            const float far = std::sqrt(fromHome.x * fromHome.x + fromHome.y * fromHome.y);
            if (far > 520.0f) {
                target = m.home + fromHome * (520.0f / far);
            }
        }
        switch (m.state) {
        case MonsterState::Idle:
        case MonsterState::Move: {
            const Vec2 d = target - center;
            const float len = std::max(std::sqrt(d.x * d.x + d.y * d.y), 1e-3f);
            const float speed = s.speed * (chasing ? 1.3f : 0.7f);
            const Vec2 want = len > 4.0f ? d * (std::min(speed, len) / len) : Vec2{};
            m.velocity = m.velocity + (want - m.velocity) * 0.12f;
            if (std::abs(m.velocity.x) > 0.15f) {
                m.facing = sign(m.velocity.x);
            }
            if (chasing && m.attackCooldown == 0 && std::abs(dx) < 240.0f && std::abs(dy) < 200.0f) {
                enter(m, MonsterState::Windup, m.elite ? 26 : 34);
                m.facing = sign(dx);
            } else if (m.state == MonsterState::Idle && m.stateTicks > 40) {
                enter(m, MonsterState::Move, 60);
            }
            break;
        }
        case MonsterState::Windup:
            m.velocity = m.velocity * 0.85f + Vec2{0.0f, -0.08f};
            m.facing = sign(dx);
            if (m.stateTicks >= m.stateLength) {
                enter(m, MonsterState::Attack, 34);
                const float len = std::max(std::sqrt(dx * dx + dy * dy), 1.0f);
                m.velocity = Vec2{dx, dy} * ((m.elite ? 8.0f : 7.0f) / len);
                step.lunged = true;
            }
            break;
        case MonsterState::Attack:
            m.velocity = m.velocity * 0.97f;
            if (m.stateTicks >= m.stateLength) {
                enter(m, MonsterState::Move, 60);
                m.attackCooldown = 120 + m.seed % 40;
            }
            break;
        case MonsterState::Hit:
        case MonsterState::Stun: m.velocity = m.velocity * 0.88f; break;
        default: break;
        }
        m.box.x += m.velocity.x;
        m.box.y += m.velocity.y;
        m.box.y = std::clamp(m.box.y, m.home.y - 360.0f, m.home.y + 200.0f);
        m.box.x = std::clamp(m.box.x, 0.0f,
                             static_cast<float>(world.layer().width()) * world.tileSize() - m.box.width);
        m.vulnerable.setHurtbox(m.box);
        return step;
    }

    // Ground monsters.
    const float speed = s.speed * (m.elite ? 1.15f : 1.0f);
    float vx = m.velocity.x;
    switch (m.state) {
    case MonsterState::Idle:
    case MonsterState::Move: {
        // MapleStory-like wandering: walk a while, stand a while, turn now and then.
        if (--m.wanderTimer <= 0) {
            m.wandering = !m.wandering;
            m.wanderTimer = (m.wandering ? 70 : 50) + static_cast<int>((m.seed * 37 + tick) % 80);
            if (m.wandering && ((tick + m.seed) % 3) == 0) {
                m.facing = -m.facing;
            }
        }
        int dir = m.facing;
        bool moving = m.wandering;
        if (chasing && std::abs(dx) > 10.0f) {
            dir = sign(dx);
            moving = true;
        }
        m.state = moving ? MonsterState::Move : MonsterState::Idle;
        const bool edge = (dir < 0 && m.box.left() <= m.patrolLeft + 2.0f) || (dir > 0 && m.box.right() >= m.patrolRight - 2.0f)
                          || (m.grounded && gamelib::core::ledgeAhead(m.box, dir, world, 6.0f))
                          || gamelib::core::wallAhead(m.box, dir, world, 2.0f);
        if (edge) {
            if (chasing) {
                moving = false;
            } else {
                dir = -dir;
            }
        }
        m.facing = dir;
        if (s.behavior == Behavior::Hopper) {
            if (m.grounded) {
                vx *= 0.55f;
                if (moving && --m.hopTimer <= 0 && !edge) {
                    m.velocity.y = -6.2f;
                    vx = static_cast<float>(dir) * speed * 2.3f;
                    m.hopTimer = 34 + m.seed % 22;
                    m.grounded = false;
                }
            }
        } else {
            const float target = moving ? static_cast<float>(dir) * speed * (chasing ? 1.35f : 1.0f) : 0.0f;
            vx += (target - vx) * 0.25f;
        }
        const float reach = s.behavior == Behavior::Charger ? 340.0f : s.behavior == Behavior::Hopper ? 170.0f : 100.0f;
        if (chasing && m.attackCooldown == 0 && m.grounded && std::abs(dx) < reach && std::abs(dy) < 80.0f) {
            enter(m, MonsterState::Windup, m.elite ? 24 : 32);
            m.facing = sign(dx);
            vx = 0.0f;
        }
        break;
    }
    case MonsterState::Windup:
        vx *= 0.7f;
        if (m.stateTicks >= m.stateLength) {
            step.lunged = true;
            if (s.behavior == Behavior::Hopper) {
                enter(m, MonsterState::Attack, 40);
                vx = static_cast<float>(m.facing) * std::min(std::abs(dx) / 22.0f + 2.0f, 7.0f);
                m.velocity.y = -8.5f;
                m.grounded = false;
            } else if (s.behavior == Behavior::Charger) {
                enter(m, MonsterState::Attack, 46);
                vx = static_cast<float>(m.facing) * (m.elite ? 9.5f : 8.5f);
            } else {
                enter(m, MonsterState::Attack, 18);
                vx = static_cast<float>(m.facing) * 6.5f;
            }
        }
        break;
    case MonsterState::Attack: {
        const bool edge = (m.facing < 0 && m.box.left() <= m.patrolLeft) || (m.facing > 0 && m.box.right() >= m.patrolRight)
                          || (m.grounded && gamelib::core::ledgeAhead(m.box, m.facing, world, 4.0f));
        if (s.behavior == Behavior::Charger) {
            if (edge) {
                vx = 0.0f;
            }
            vx *= m.stateTicks > 30 ? 0.85f : 1.0f;
        } else if (s.behavior == Behavior::Walker) {
            vx *= 0.88f;
            if (edge) {
                vx = 0.0f;
            }
        }
        const bool done = m.stateTicks >= m.stateLength || (s.behavior == Behavior::Hopper && m.grounded && m.stateTicks > 6);
        if (done) {
            enter(m, MonsterState::Move, 40);
            m.attackCooldown = 100 + m.seed % 50;
            vx = 0.0f;
        }
        break;
    }
    case MonsterState::Hit:
    case MonsterState::Stun: {
        vx *= 0.82f;
        // Knockback never pushes a monster off its platform.
        const int dir = vx < 0.0f ? -1 : 1;
        if (std::abs(vx) > 0.1f && m.grounded && gamelib::core::ledgeAhead(m.box, dir, world, 2.0f)) {
            vx = 0.0f;
        }
        break;
    }
    default: break;
    }
    m.velocity.x = vx;
    fall(m, world, step);
    // A monster that fell off its run anyway adopts the floor it lands on.
    if (step.landed && (m.box.right() < m.patrolLeft || m.box.left() > m.patrolRight)) {
        m.patrolLeft = m.box.left() - 140.0f;
        m.patrolRight = m.box.right() + 140.0f;
    }
    m.vulnerable.setHurtbox(m.box);
    return step;
}

} // namespace realm::game
