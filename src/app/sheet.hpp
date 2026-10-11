#pragma once

// The construction sheet (REALM_SCENE=sheet): the cast and the job outfits
// on one page - large, at game size, in every pose and expression - for
// reviewing the art.

#include "shapes.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>

namespace realm::app {

inline constexpr int kSheetPages = 3;

void drawSheet(gamelib::draw::DrawList& list, const Shapes& shapes, gamelib::core::Size canvas, int page, float time);

} // namespace realm::app
