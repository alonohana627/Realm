#pragma once

// Realm's interface pieces, in canvas units: soft rounded windows with a
// sky-blue title bar, smooth lowercase-friendly text (GameLib's vector font
// through draw::Font), pill buttons, item slots with rarity glows and
// counts, gauges, key caps, tabs, the cooldown sweep, and the item card
// (name in its rarity color, requirements, stats and what it changes).

#include "look.hpp"
#include "shapes.hpp"

#include "character.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/draw/font.hpp>
#include <gamelib/ui/item_grid.hpp>
#include <gamelib/ui/panel.hpp>

#include <string>
#include <string_view>

namespace realm::app::kit {

using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::draw::Align;
using gamelib::draw::Font;

inline constexpr float kMargin = 12.0f; // ui.frag panels' room for shadows and glows

// Fonts.
[[nodiscard]] Font font(float size, float weight = 1.3f);
[[nodiscard]] Font titleFont(float size = 34.0f);
[[nodiscard]] Font outlined(float size, float weight = 1.4f); // light text over the world

void text(gamelib::draw::DrawList& list, std::string_view text, float x, float y, float size, Color color = ui_color::kInk,
          Align align = Align::Left, float weight = 1.3f);
// Text vertically centered on `centerY`.
void label(gamelib::draw::DrawList& list, std::string_view text, float x, float centerY, float size, Color color = ui_color::kInk,
           Align align = Align::Left, float weight = 1.3f);
[[nodiscard]] float width(std::string_view text, float size, float weight = 1.3f);

// A window (a soft panel with a title bar); returns the content rect.
Rect window(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, std::string_view title, Color accent = ui_color::kTitle,
            float alpha = 1.0f);
void well(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, float alpha = 1.0f);
void tooltip(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, float alpha = 1.0f);
// state: 0 normal, 1 selected, 2 pressed, 3 disabled.
void button(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, std::string_view label, int state, float size = 26.0f,
            Color fill = ui_color::kButton);
void slot(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, const gamelib::core::ItemStack& stack, bool selected, float time,
          float alpha = 1.0f);
void slotIcon(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, int item, int count, float alpha = 1.0f);
void keyCap(gamelib::draw::DrawList& list, const Shapes& shapes, Vec2 center, std::string_view label, float height = 34.0f);
void gauge(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, float fill, float trail, Color color, Color trailColor);
void tab(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, std::string_view label, bool selected);
void sweep(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, float remaining);
void skillIconAt(gamelib::draw::DrawList& list, const Shapes& shapes, Vec2 center, float size, game::SkillId skill, float alpha = 1.0f);

// The item card for a stack (equipment compared with what is worn).
float itemCard(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, const gamelib::core::ItemStack& stack,
               const game::Character& character, bool compare);

[[nodiscard]] std::string groupDigits(long long value);

} // namespace realm::app::kit
