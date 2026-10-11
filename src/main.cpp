// Realm - a small MapleStory-inspired open-world platformer RPG on GameLib.
//
// Command line: --demo (the demo bot plays) plus GameLib's self-test switches
// (--frames N --hidden --require-validation --no-vsync --allow-software
// --resize-at-frame N --minimize-at-frame N --focus-loss-at-frame N
// --keep-running).
#include "realm_game.hpp"

#include "rules.hpp"

#include <gamelib/app/app.hpp>

#include <memory>

int main(int argc, char** argv)
{
    gamelib::app::AppConfig config{
        .title = "Realm",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / realm::game::kTickRate},
        .pauseOnFocusLoss = true,
        .gameFlags = {"--demo"},
    };
    return gamelib::app::runMain(argc, argv, config,
                                 [&](gamelib::app::App& app) { return std::make_unique<realm::app::RealmGame>(app); });
}
