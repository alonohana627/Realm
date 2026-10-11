#include "shapes.hpp"

#include "avatar_frag.spv.hpp"

namespace realm::app {

Shapes Shapes::registerWith(gamelib::gfx::Renderer& renderer)
{
    return {
        .avatar = renderer.addShape(shaders::avatar_frag),
    };
}

} // namespace realm::app
