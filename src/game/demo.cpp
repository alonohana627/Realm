#include "demo.hpp"

#include <gamelib/core/tile_runs.hpp>

#include <cmath>

namespace realm::game {

namespace {

// helpers

using gamelib::core::Vec2;

constexpr int kDemoTicks = 100 * kTickRate;

int sign(const float v)
{
    return v < 0.0f ? -1 : 1;
}

// The x of the nearest rope or ladder that starts at about the hero's floor
// and leads up.
std::optional<float> climbNear(const Session& s, const Vec2 feet)
{
    const MapDef& m = s.map();
    gamelib::core::TileWorld w{m.tiles, 0, tileShapes()};
    std::optional<float> best;
    float bestDist = 1e9f;
    for (const gamelib::core::TileRun& run : gamelib::core::climbRuns(w)) {
        const gamelib::core::Rect r = run.rect(kTile);
        if (r.bottom() < feet.y - kTile * 1.5f || r.bottom() > feet.y + 4.0f || r.top() > feet.y - kTile * 2.5f) {
            continue;
        }
        const float x = r.center().x;
        const float d = std::abs(x - feet.x);
        if (d < bestDist) {
            bestDist = d;
            best = x;
        }
    }
    return best;
}

} // namespace

Session demoSession(const std::uint64_t seed)
{
    Appearance look{.name = "Demo", .hair = static_cast<int>(seed % 8), .hairColor = static_cast<int>(seed / 8 % 8),
                    .eyes = static_cast<int>(seed / 3 % 8), .skin = static_cast<int>(seed % 3)};
    Session s{look, seed};
    s.debugQuestsTo(kQuestShellShock);
    s.debugLevel(6);
    s.debugLearnAll();
    s.debugTeleport(MapId::SproutMeadow, 0);
    s.debugMove(tx(80.0f));
    (void)s.events().take();
    return s;
}

bool demoDone(const Session& session)
{
    return session.ticks() > kDemoTicks || session.phase() == Phase::Dead;
}

Input demoInput(const Session& s, DemoBot& bot)
{
    Input in;
    ++bot.ticks;
    const Hero& h = s.hero();
    const auto& b = h.body;
    const Vec2 feet = b.box.bottomCenter();
    const Character& c = s.character();
    if (c.hp.current * 10 < c.hp.max * 4 && bot.ticks % 60 == 0) {
        in.useItem = kRedPotion;
        return in;
    }
    if (bot.stuck.update(static_cast<float>(s.stats().defeated) * 100.0f + std::abs(feet.x - bot.lastX) * 0.0f)) {
        bot.wanderDir = -bot.wanderDir;
        bot.climbTicks = 0;
        in.jumpPressed = true;
    }
    bot.lastX = feet.x;
    in.jumpHeld = true;
    // Picking up what lies around.
    if (bot.ticks % 4 == 0) {
        in.pickup = true;
    }
    if (b.climbing) {
        in.climb = -1;
        if (bot.ticks % 90 == 0) {
            in.move = bot.wanderDir; // leap off sideways now and then
            in.jumpPressed = true;
        }
        return in;
    }
    // The nearest monster.
    const Monster* target = nullptr;
    float best = 1e9f;
    for (const Monster& m : s.monsters()) {
        if (!m.active()) {
            continue;
        }
        const Vec2 d = m.box.bottomCenter() - feet;
        const float cost = std::abs(d.x) + std::abs(d.y) * 3.0f;
        if (cost < best) {
            best = cost;
            target = &m;
        }
    }
    if (target == nullptr) {
        in.move = bot.wanderDir;
        if (feet.x < tx(4.0f)) {
            bot.wanderDir = 1;
        } else if (feet.x > s.map().bounds().right() - tx(4.0f)) {
            bot.wanderDir = -1;
        }
        return in;
    }
    const Vec2 d = target->box.bottomCenter() - feet;
    if (d.y < -kTile * 2.5f) {
        // Above: head for a rope or a ladder going up.
        if (bot.climbTicks <= 0) {
            if (const std::optional<float> climbX = climbNear(s, feet)) {
                const float dx = *climbX - feet.x;
                if (std::abs(dx) < 10.0f) {
                    in.climb = -1;
                    bot.climbTicks = 30;
                } else {
                    in.move = sign(dx);
                }
                return in;
            }
            // No way up nearby: a hyper jump straight up from below.
            if (std::abs(d.x) < 60.0f && b.grounded) {
                in.jumpPressed = true;
                bot.hyperIn = 14;
            }
        } else {
            --bot.climbTicks;
            in.climb = -1;
        }
    } else if (d.y > kTile * 2.0f && b.grounded) {
        // Below: drop through the platform.
        in.climb = 1;
        in.jumpPressed = bot.ticks % 20 == 0;
        return in;
    }
    if (bot.hyperIn > 0 && --bot.hyperIn == 0) {
        in.jumpPressed = true;
        in.climb = -1;
        return in;
    }
    const float ax = std::abs(d.x);
    const int toward = sign(d.x);
    if (ax > 110.0f) {
        in.move = toward;
        // A pebble at a monster ahead on the same floor.
        if (ax < 420.0f && std::abs(d.y) < 40.0f && b.facing == toward && bot.ticks % 50 == 0 && c.skillLevel(SkillId::PebbleToss) > 0) {
            in.skills |= 1u << static_cast<unsigned>(SkillId::PebbleToss);
        }
        // A hyper jump across a long walk.
        if (ax > 500.0f && b.grounded && bot.ticks % 150 == 0) {
            in.jumpPressed = true;
            bot.hyperIn = 10;
        }
    } else {
        if (b.facing != toward) {
            in.move = toward;
        }
        in.attackHeld = true;
    }
    return in;
}

} // namespace realm::game
