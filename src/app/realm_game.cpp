#include "realm_game.hpp"

#include "sheet.hpp"

#include <cstdlib>
#include <string_view>

namespace realm::app {

RealmGame::RealmGame(gamelib::app::App& app)
    : m_shapes{Shapes::registerWith(app.renderer())}
{
    if (const char* scene = std::getenv("REALM_SCENE")) {
        const std::string_view s{scene};
        if (s.starts_with("sheet") && s.size() > 5) {
            m_page = (s[5] - '0') % kSheetPages;
        }
    }
}

void RealmGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    using gamelib::platform::Key;
    if (!event.pressed) {
        return;
    }
    if (event.key == Key::Escape) {
        app.requestQuit();
    } else if (event.key == Key::Right) {
        m_page = (m_page + 1) % kSheetPages;
    } else if (event.key == Key::Left) {
        m_page = (m_page + kSheetPages - 1) % kSheetPages;
    }
}

void RealmGame::tick(gamelib::app::App&)
{
}

void RealmGame::draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    drawSheet(list, m_shapes, app.canvas(), m_page, info.time);
}

} // namespace realm::app
