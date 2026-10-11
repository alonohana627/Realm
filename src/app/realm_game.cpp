#include "realm_game.hpp"

#include "sheet.hpp"

#include <gamelib/core/rng.hpp>

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <string_view>

namespace realm::app {

namespace {

// helpers

int toInt(const std::string_view text)
{
    int v = 0;
    (void)std::from_chars(text.data(), text.data() + text.size(), v);
    return v;
}

} // namespace

RealmGame::RealmGame(gamelib::app::App& app)
    : m_shapes{Shapes::registerWith(app.renderer())}
    , m_canvas{app.canvas()}
    , m_actions{makeActions(game::Job::Beginner)}
    , m_view{app.canvas()}
{
    std::string scene;
    if (const char* s = std::getenv("REALM_SCENE")) {
        scene = s;
    }
    if (scene.starts_with("sheet")) {
        m_mode = Mode::Sheet;
        m_page = scene.size() > 5 ? (scene[5] - '0') % kSheetPages : 0;
        return;
    }
    m_session.emplace(game::Appearance{.name = "Rin", .hair = 0, .hairColor = 0, .eyes = 0, .skin = 0}, gamelib::core::randomSeed());
    applyDevScene(scene);
    (void)m_session->events().take();
    m_view.snap(*m_session);
    m_effects.setPlace(m_session->map());
}

void RealmGame::applyDevScene(const std::string& scene)
{
    game::Session& s = *m_session;
    std::istringstream tokens{scene};
    std::string token;
    while (std::getline(tokens, token, ',')) {
        if (token.starts_with("map")) {
            s.debugTeleport(static_cast<game::MapId>(std::clamp(toInt(std::string_view{token}.substr(3)), 0, game::kMapCount - 1)), 0);
        } else if (token.starts_with("level")) {
            s.debugLevel(toInt(std::string_view{token}.substr(5)));
        } else if (token == "warrior" || token == "mage") {
            s.debugJob(token == "warrior" ? game::Job::Warrior : game::Job::Mage);
            addJobActions(m_actions, s.character().job);
        } else if (token == "night") {
            s.debugTime(22);
        } else if (token == "dusk") {
            s.debugTime(18);
        } else if (token == "gear") {
            s.debugGear();
        } else if (token == "learn") {
            s.debugLearnAll();
        } else if (token.starts_with("quest")) {
            s.debugQuestsTo(toInt(std::string_view{token}.substr(5)));
        } else if (token.starts_with("x")) {
            s.debugMove(static_cast<float>(toInt(std::string_view{token}.substr(1))) * game::kTile);
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
    } else if (m_mode == Mode::Sheet && event.key == Key::Right) {
        m_page = (m_page + 1) % kSheetPages;
    } else if (m_mode == Mode::Sheet && event.key == Key::Left) {
        m_page = (m_page + kSheetPages - 1) % kSheetPages;
    }
}

void RealmGame::tick(gamelib::app::App& app)
{
    if (m_mode != Mode::Play || !m_session) {
        return;
    }
    game::Session& s = *m_session;
    s.tick(readInput(m_actions, app.input()));
    for (const game::Event& e : s.events().take()) {
        m_effects.react(e, s);
    }
    if (s.travelPending()) {
        s.travel();
        (void)s.events().take();
        m_view.snap(s);
        m_effects.clear();
        m_effects.setPlace(s.map());
    }
    (void)s.takeTalkRequest();
}

void RealmGame::frame(gamelib::app::App&, const gamelib::app::FrameInfo& info)
{
    m_time = info.time;
    if (m_mode == Mode::Play && m_session) {
        m_alpha = info.alpha;
        m_view.follow(*m_session, m_alpha, info.seconds, m_time);
        m_effects.update(info.seconds, *m_session, m_view.visible(), m_view.heroFeet(*m_session, m_alpha));
    }
}

void RealmGame::draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    if (m_mode == Mode::Sheet) {
        drawSheet(list, m_shapes, app.canvas(), m_page, info.time);
        return;
    }
    if (m_session) {
        m_view.draw(list, m_shapes, *m_session, m_effects, m_alpha, m_time, m_effects.shake(m_time));
        list.setLayer(gamelib::draw::layers::Hud);
        m_effects.drawScreen(list, m_canvas);
    }
}

} // namespace realm::app
