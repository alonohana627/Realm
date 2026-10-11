#pragma once

// Drawing the world: the camera (GameLib's Camera2D following the hero,
// held inside the map), the place's parallax sky and silhouettes, the back
// row of houses, the ground and its cap, footholds, rope bridges, ropes and
// ladders, houses and decorations, portals, the townsfolk with name plates
// and quest markers, monsters with their levels and health bars, an area
// boss, loot, shots, telegraphed hazards and the hero - every character an
// anime avatar or creature shader - then lights and the color grade of the
// place and the hour. Presentation only.

#include "effects.hpp"
#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/camera.hpp>
#include <gamelib/core/tile_runs.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/sprite_motion.hpp>

#include <vector>

namespace realm::app {

class WorldView {
public:
    explicit WorldView(gamelib::core::Size canvas);

    void snap(const game::Session& session);
    void follow(const game::Session& session, float alpha, float seconds, float time);
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
              float alpha, float time, gamelib::core::Vec2 shake) const;

    [[nodiscard]] gamelib::core::View2D view(gamelib::core::Vec2 shake = {}) const { return m_camera.view(shake); }
    // Scenery only (no one in it, no portals or labels): the ending's stage.
    void setScenery(const bool scenery) { m_scenery = scenery; }
    [[nodiscard]] gamelib::core::Rect visible() const;
    [[nodiscard]] gamelib::core::Vec2 heroFeet(const game::Session& session, float alpha) const;

    // The hero as drawn (also for the windows' portraits).
    [[nodiscard]] static anime::Avatar heroLook(const game::Session& session, float alpha);

private:
    struct Run {
        gamelib::core::Rect rect;
        Terrain kind = Terrain::Foothold;
        bool pier = false; // a harbor pier: always wooden planks
    };
    void cacheMap(const game::MapDef& def) const;
    void drawBackdrop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawTerrain(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawDecos(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawPortals(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawNpcs(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawMonsters(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha) const;
    void drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha) const;
    void drawLoot(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha, float time) const;
    void drawShots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawHero(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects, float alpha,
                  float time) const;
    void drawLights(gamelib::draw::DrawList& list, const game::Session& session, float alpha) const;
    void drawLabels(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha, float time) const;

    gamelib::core::Size m_canvas;
    gamelib::core::Camera2D m_camera;
    bool m_scenery = false;
    game::MapId m_map = game::MapId::Count;
    mutable game::MapId m_cached = game::MapId::Count;
    mutable std::vector<Run> m_runs;
    gamelib::fx::SquashStretch m_squash;
    int m_lastLandTicks = 0;
    float m_expressionBlend = 1.0f;
    int m_expressionFrom = 0;
    int m_expressionTo = 0;
};

} // namespace realm::app
