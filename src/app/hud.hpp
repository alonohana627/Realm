#pragma once

// The play screen's HUD, MapleStory-like in Realm's own soft style: the
// status bar along the bottom (the level badge, name and job, HP and MP
// gauges with a "damage taken" trail, the experience bar), the quick slots
// - every key holding a skill or an item, its icon, the key cap, the
// clockwise cooldown sweep and the item count - the minimap with portals,
// townsfolk and the hero, active buffs with their time, the day clock, the
// quest tracker, the message log of drops and quest updates, the map's
// title, an area boss's health bar and the "up" prompt near people and
// portals.

#include "look.hpp"
#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/camera.hpp>
#include <gamelib/core/tile_runs.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/banners.hpp>
#include <gamelib/platform/actions.hpp>
#include <gamelib/ui/hud.hpp>
#include <gamelib/ui/message_log.hpp>

#include <string>
#include <vector>

namespace realm::app {

class Hud {
public:
    Hud();

    void reset(const game::Session& session);
    void react(const game::Event& e, const game::Session& session);
    void update(float seconds, const game::Session& session);
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
              const gamelib::platform::ActionMap& actions, const gamelib::core::View2D& view, int minimap, float time) const;
    void log(std::string text, Color color);
    void banner(std::string title, std::string line, Color color, float seconds);
    void mapTitle(const game::Session& session);

private:
    void drawStatus(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawQuickSlots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                        const gamelib::platform::ActionMap& actions, float time) const;
    void drawMinimap(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, int size, float time) const;
    void drawBuffs(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawTracker(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawPrompt(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                    const gamelib::platform::ActionMap& actions, const gamelib::core::View2D& view, float time) const;

    gamelib::ui::BarTrail m_hp{0.5f, 0.6f};
    gamelib::ui::BarTrail m_mp{0.5f, 0.6f};
    gamelib::ui::BarTrail m_boss{0.6f, 0.4f};
    gamelib::ui::MessageLog m_log{5.0f, 7};
    gamelib::fx::Banners m_banners;
    gamelib::fx::Banners m_title;
    float m_xpShown = 0.0f;
    float m_bossShown = 0.0f; // 0..1 how far the boss bar has slid in
    mutable game::MapId m_cached = game::MapId::Count;
    mutable std::vector<gamelib::core::TileRun> m_floors;
    mutable std::vector<gamelib::core::TileRun> m_climbs;
};

} // namespace realm::app
