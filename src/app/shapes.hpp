#pragma once

// Realm's own shader shapes, registered once with the renderer, and small
// builders that pack each shader's fields into a draw::Sprite (see the
// shaders' headers and anime_look.hpp).

#include "anime_look.hpp"

#include <gamelib/draw/draw_list.hpp>
#include <gamelib/gfx/renderer.hpp>

namespace realm::app {

struct Shapes {
    gamelib::draw::ShapeId avatar;

    [[nodiscard]] static Shapes registerWith(gamelib::gfx::Renderer& renderer);
};

} // namespace realm::app
