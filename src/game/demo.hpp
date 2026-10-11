#pragma once

// The demo bot: a young adventurer grinding monsters on Sprout Meadow, as
// the title screen's attract mode and the --demo run. It reads the session
// like a player reads the screen (the nearest monster, its own health, a
// rope or a ladder to the tier above) and answers with one tick of input:
// it walks, jumps, climbs, swings, throws pebbles, hyper-jumps across gaps
// and drinks potions. Deterministic for a seed.

#include "session.hpp"

#include <gamelib/ai/planner.hpp>

namespace realm::game {

struct DemoBot {
    int ticks = 0;
    int climbTicks = 0;    // > 0 while heading up a rope or a ladder
    int wanderDir = 1;
    int hyperIn = -1;
    gamelib::ai::StuckWatch stuck{120};
    float lastX = 0.0f;
};

[[nodiscard]] Session demoSession(std::uint64_t seed);
[[nodiscard]] Input demoInput(const Session& session, DemoBot& bot);
[[nodiscard]] bool demoDone(const Session& session);

} // namespace realm::game
