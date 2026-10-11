#pragma once

// Realm's juice, driven by the rules' events and drawn over the world:
// swing arcs and skill effects that follow the hero (Power Strike's flash,
// Crescent Cleave's wide arc, Earthquake's shockwave, Rush's streak, Arcane
// Bolt sparks, Chain Spark's lightning, Starfall's falling stars, Bloom's
// petals, buff halos), hit sparks, stacked damage numbers in chunky outlined
// digits (criticals bigger and golden, the hero's hurts red, heals green,
// MP blue), the hyper jump's afterimages, the level-up pillar and the job
// advancement burst, loot sparkles, pickup popups with the item's icon,
// dust, ambient motes per place, screen shake and flashes. Presentation
// only, on real frame time.

#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/ambient.hpp>
#include <gamelib/fx/damage_numbers.hpp>
#include <gamelib/fx/juice.hpp>
#include <gamelib/fx/particles.hpp>
#include <gamelib/fx/transients.hpp>

#include <string>

namespace realm::app {

class Effects {
public:
    Effects();

    void react(const game::Event& e, const game::Session& session);
    void update(float seconds, const game::Session& session, gamelib::core::Rect visible, gamelib::core::Vec2 heroFeet);
    void clear();
    void setPlace(const game::MapDef& map);
    void setShakeScale(float scale) { m_shakeScale = scale; }

    // Behind the characters (auras, rings on the floor), in front of them,
    // and the world-space numbers and popups (over everything in the world).
    void drawBehind(gamelib::draw::DrawList& list, const Shapes& shapes, gamelib::core::Vec2 heroFeet, float time) const;
    void drawFront(gamelib::draw::DrawList& list, const Shapes& shapes, gamelib::core::Vec2 heroFeet, int heroFacing,
                   float time) const;
    void drawNumbers(gamelib::draw::DrawList& list, const Shapes& shapes, float time) const;
    // In canvas units: flashes.
    void drawScreen(gamelib::draw::DrawList& list, gamelib::core::Size canvas) const;

    [[nodiscard]] gamelib::core::Vec2 shake(float time) const { return m_shake.offset(time) * m_shakeScale; }

    struct Ghost {
        gamelib::core::Vec2 feet;
        int facing = 1;
        bool up = false;
    };
    [[nodiscard]] const gamelib::fx::Transients<Ghost>& ghosts() const { return m_ghosts; }

private:
    struct Arc {
        gamelib::core::Vec2 offset; // from the hero's feet (attached) or the world point
        int facing = 1;
        bool attached = true;
        gamelib::core::Color core;
        gamelib::core::Color edge;
        float size = 220.0f;
        Fx kind = Fx::Slash;
        float variant = 0.0f;
        float rotation = 0.0f;
        gamelib::core::Vec2 stretch{1.0f, 1.0f};
    };
    struct Popup {
        gamelib::core::Vec2 at;
        std::string text;
        gamelib::core::Color color;
        int item = 0;
        int row = 0;
    };
    struct Falling {
        gamelib::core::Vec2 from;
        gamelib::core::Vec2 to;
        gamelib::core::Color color;
        float delay = 0.0f;
    };

    void addArc(const Arc& arc, float seconds) { m_arcs.add(arc, seconds); }

    gamelib::fx::DamageNumbers m_numbers;
    gamelib::fx::Transients<Arc> m_arcs;
    gamelib::fx::Transients<Popup> m_popups;
    gamelib::fx::Transients<Ghost> m_ghosts;
    gamelib::fx::Transients<Falling> m_falling;
    gamelib::fx::Particles m_sparks;
    gamelib::fx::Particles m_petals;
    gamelib::fx::Ambient m_motes;
    gamelib::fx::AmbientLook m_moteLook;
    gamelib::fx::Shake m_shake{0.4f, 16.0f};
    gamelib::fx::Fade m_flash{0.4f};
    gamelib::core::Color m_flashColor{1.0f, 1.0f, 1.0f, 0.5f};
    float m_shakeScale = 1.0f;
    float m_ghostClock = 0.0f;
    float m_popupClock = 0.0f;
    int m_popupRow = 0;
    bool m_leaves = false;
    bool m_snow = false;
};

} // namespace realm::app
