#pragma once

// Realm's own shader shapes, registered once with the renderer, and small
// builders that pack each shader's fields into a draw::Sprite (see the
// shaders' headers and anime_look.hpp).

#include "anime_look.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/gfx/renderer.hpp>

namespace realm::app {

struct Shapes {
    gamelib::draw::ShapeId avatar;
    gamelib::draw::ShapeId monster;
    gamelib::draw::ShapeId boss;
    gamelib::draw::ShapeId building;
    gamelib::draw::ShapeId deco;
    gamelib::draw::ShapeId terrain;
    gamelib::draw::ShapeId backdrop;
    gamelib::draw::ShapeId fx;
    gamelib::draw::ShapeId ui;

    [[nodiscard]] static Shapes registerWith(gamelib::gfx::Renderer& renderer);
};

// terrain.frag kinds.
enum class Terrain : std::uint8_t { Ground, Foothold, Rope, Ladder, Bridge, RampUp, RampDown, Water };

// fx.frag kinds.
enum class Fx : std::uint8_t {
    Slash, Crescent, Star, Spark, Petal, Wind, Portal, Shockwave, Warn, Shot, LevelUp, Coin, LootGlow, Puff, Streak, Plus, Bolt,
    Lightning, Shield, Ring, Root, Beam, Glow, Leaf, Snow, Gull
};

// ui.frag kinds.
enum class Ui : std::uint8_t {
    Window, Button, Slot, Gauge, ItemIcon, SkillIcon, StatIcon, Marker, Digit, Coin, KeyCap, Well, Arrow, Tooltip, Sweep, Tab,
    Badge, Plate, Logo, Map, Shadow
};

// A square fx sprite centered on `at`, `size` across.
void addFx(gamelib::draw::DrawList& list, const Shapes& shapes, Fx kind, gamelib::core::Vec2 at, float size,
           gamelib::core::Color core, gamelib::core::Color edge, float progress, float variant = 0.0f, float w = 0.0f,
           float rotation = 0.0f);
// A stretched fx sprite filling `rect`.
void addFxRect(gamelib::draw::DrawList& list, const Shapes& shapes, Fx kind, gamelib::core::Rect rect, gamelib::core::Color core,
               gamelib::core::Color edge, float progress, float variant = 0.0f, float w = 0.0f, float rotation = 0.0f);

// A UI sprite filling `rect`.
void addUi(gamelib::draw::DrawList& list, const Shapes& shapes, Ui kind, gamelib::core::Rect rect, gamelib::core::Color a,
           gamelib::core::Color b, float y = 0.0f, float z = 0.0f, float w = 0.0f);

// A square icon (item, skill, stat, marker) centered on `at`.
void addIcon(gamelib::draw::DrawList& list, const Shapes& shapes, Ui kind, gamelib::core::Vec2 at, float size, float id,
             float style, gamelib::core::Color a, gamelib::core::Color b, float w = 0.0f);

// A decoration standing on `ground` (deco.frag), `height` tall, `width` wide.
[[nodiscard]] gamelib::draw::Sprite decoSprite(const Shapes& shapes, int kind, gamelib::core::Vec2 ground, float width, float height,
                                                gamelib::core::Color a, gamelib::core::Color b, float lit, float variant, float seed);

} // namespace realm::app
