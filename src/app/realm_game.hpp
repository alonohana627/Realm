#pragma once

// Realm on GameLib's main loop.

#include "actions.hpp"
#include "effects.hpp"
#include "shapes.hpp"
#include "view.hpp"

#include "session.hpp"

#include <gamelib/app/app.hpp>

#include <optional>
#include <string>

namespace realm::app {

class RealmGame final : public gamelib::app::Game {
public:
    explicit RealmGame(gamelib::app::App& app);

    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    enum class Mode { Play, Sheet };
    void applyDevScene(const std::string& scene);

    Shapes m_shapes;
    gamelib::core::Size m_canvas;
    Mode m_mode = Mode::Play;
    int m_page = 0;
    std::optional<game::Session> m_session;
    gamelib::platform::ActionMap m_actions;
    WorldView m_view;
    Effects m_effects;
    float m_time = 0.0f;
    float m_alpha = 1.0f;
};

} // namespace realm::app
