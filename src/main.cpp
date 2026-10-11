// Realm - a small MapleStory-inspired open-world platformer RPG on GameLib.
//
// Command line: --demo (the demo bot plays full screen) plus GameLib's
// self-test switches (--frames N --hidden --require-validation --no-vsync
// --allow-software --resize-at-frame N --minimize-at-frame N
// --focus-loss-at-frame N --keep-running).
//
// Developer aids (environment): REALM_SCENE=sheet0..sheet2 shows the
// character construction sheets, "title", "select" or "create" a screen;
// otherwise a comma list starts a fresh character in slot 3 there, e.g.
// "map11,level15,warrior,gear,learn,night,stats" (RealmGame::applyDevScene).
#include "profile.hpp"
#include "realm_game.hpp"

#include "rules.hpp"

#include <gamelib/app/app.hpp>

#include <memory>
#include <utility>

int main(int argc, char** argv)
{
    realm::app::ProfileFile profile = realm::app::openProfile();
    gamelib::app::AppConfig config{
        .title = "Realm",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / realm::game::kTickRate},
        .fullscreen = profile.value().settings.fullscreen,
        .f11TogglesFullscreen = false, // F11 goes through ArcadeMenus, so the setting is saved
        .pauseOnFocusLoss = true,
        .gameFlags = {"--demo"},
    };
    config.renderer.vsync = profile.value().settings.vsync;
    return gamelib::app::runMain(argc, argv, config, [&](gamelib::app::App& app) {
        return std::make_unique<realm::app::RealmGame>(app, std::move(profile), app.hasFlag("--demo"));
    });
}
