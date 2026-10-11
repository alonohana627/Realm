#pragma once

// Realm on GameLib's main loop.

#include "shapes.hpp"

#include <gamelib/app/app.hpp>

namespace realm::app {

class RealmGame final : public gamelib::app::Game {
public:
    explicit RealmGame(gamelib::app::App& app);

    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    Shapes m_shapes;
    int m_page = 0;
};

} // namespace realm::app
