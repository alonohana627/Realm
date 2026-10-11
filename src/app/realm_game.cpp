#include "realm_game.hpp"

#include "look.hpp"
#include "sheet.hpp"

#include <gamelib/app/dev_start.hpp>
#include <gamelib/app/settings.hpp>
#include <gamelib/core/rng.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <format>
#include <sstream>
#include <string_view>
#include <utility>

namespace realm::app {

namespace {

// helpers

using gamelib::app::ArcadeAction;
using gamelib::app::ArcadeScreen;
using gamelib::core::Vec2;
using gamelib::platform::ActionId;
using gamelib::platform::Key;
using gamelib::ui::MenuInput;

constexpr int kKeysCommand = 1;
constexpr int kCharactersCommand = 2;

constexpr float kFaintSeconds = 2.6f;
constexpr float kVoyageSeconds = 5.5f;

constexpr std::array<std::string_view, 16> kNames{"Rin", "Mochi", "Kaze", "Tavi", "Nell", "Yuzu", "Bram", "Suki",
                                                  "Lio", "Wynn", "Hana", "Taro", "Mika", "Ollie", "Zuri", "Fen"};

std::optional<game::NpcId> npcBySpeaker(const std::string_view speaker)
{
    for (int i = 0; i < game::kNpcCount; ++i) {
        if (game::npcName(static_cast<game::NpcId>(i)) == speaker) {
            return static_cast<game::NpcId>(i);
        }
    }
    return std::nullopt;
}

int toInt(const std::string_view text)
{
    int v = 0;
    (void)std::from_chars(text.data(), text.data() + text.size(), v);
    return v;
}

// The menu keys that never move: arrows, Enter and Escape (the keybinding
// window places every other key).
std::optional<MenuInput> fixedMenuKey(const Key key)
{
    switch (key) {
    case Key::Up: return MenuInput::Up;
    case Key::Down: return MenuInput::Down;
    case Key::Left: return MenuInput::Left;
    case Key::Right: return MenuInput::Right;
    case Key::Enter: return MenuInput::Confirm;
    case Key::Escape: return MenuInput::Back;
    default: return std::nullopt;
    }
}

std::optional<WindowId> windowFor(const ActionId a)
{
    if (a == actionId(Action::Stats)) {
        return WindowId::Stats;
    }
    if (a == actionId(Action::Skills)) {
        return WindowId::Skills;
    }
    if (a == actionId(Action::Inventory)) {
        return WindowId::Inventory;
    }
    if (a == actionId(Action::Equipment)) {
        return WindowId::Equipment;
    }
    if (a == actionId(Action::Quests)) {
        return WindowId::Quests;
    }
    if (a == actionId(Action::WorldMap)) {
        return WindowId::WorldMap;
    }
    if (a == actionId(Action::Keys)) {
        return WindowId::Keys;
    }
    return std::nullopt;
}

Sfx skillSound(const game::SkillId s)
{
    using game::SkillId;
    switch (s) {
    case SkillId::PebbleToss: return Sfx::Pebble;
    case SkillId::CatchBreath: return Sfx::Bloom;
    case SkillId::HyperJump: return Sfx::HyperJump;
    case SkillId::PowerStrike:
    case SkillId::Cleave: return Sfx::Strike;
    case SkillId::Quake: return Sfx::Quake;
    case SkillId::Rush: return Sfx::Rush;
    case SkillId::IronBody:
    case SkillId::WarCry:
    case SkillId::ManaShield:
    case SkillId::Swiftwind: return Sfx::Buff;
    case SkillId::ArcaneBolt: return Sfx::Bolt;
    case SkillId::ChainSpark: return Sfx::Spark;
    case SkillId::Starfall: return Sfx::Starfall;
    case SkillId::Bloom: return Sfx::Bloom;
    default: return Sfx::Swing;
    }
}

} // namespace

RealmGame::RealmGame(gamelib::app::App& app, ProfileFile profile, const bool demo)
    : m_profile{std::move(profile)}
    , m_menus{"REALM", m_profile.value().settings, {}, {.music = true, .vsync = true},
              [](gamelib::ui::Menu& paused) {
                  paused.action("KEY BINDINGS", kKeysCommand);
                  paused.action("CHARACTERS", kCharactersCommand);
              }}
    , m_sfx{app.audio(), sfxRecipe}
    , m_music{app.audio(), 2, [](const Tune t, const int variant) { return tune(t, variant); }}
    , m_shapes{Shapes::registerWith(app.renderer())}
    , m_canvas{app.canvas()}
    , m_actions{makeActions(game::Job::Beginner)}
    , m_view{app.canvas()}
    , m_attract{[](const std::uint64_t seed) { return DemoRun{.session = game::demoSession(seed), .bot = {}}; },
                [](DemoRun& run, float) {
                    run.session.tick(game::demoInput(run.session, run.bot));
                    (void)run.session.events().take();
                },
                [](const DemoRun& run) { return game::demoDone(run.session); }, gamelib::core::randomSeed()}
    , m_demoView{app.canvas()}
    , m_demoMode{demo}
{
    m_menus.setSounds({.moved = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuMove)},
                       .changed = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuMove)},
                       .selected = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuSelect)},
                       .back = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuBack)}});
    m_profile.save();
    gamelib::app::applySettings(app, m_profile.value().settings);
    m_demoView.snap(m_attract.sim().session);
    m_demoEffects.setPlace(m_attract.sim().session.map());
    m_music.play(app.audio(), Tune::Title, 0.6f);
    const gamelib::app::DevStart dev = gamelib::app::readDevStart("REALM");
    if (dev.scene) {
        applyDevScene(app, *dev.scene);
    }
}

RealmGame::~RealmGame()
{
    saveNow();
    m_profile.save();
}

void RealmGame::saveNow()
{
    if (m_session && (m_mode == Mode::Play || m_mode == Mode::Ending) && !m_demoMode) {
        m_saves.save(m_slot, m_session->save(), m_actions);
    }
}

// ---------------------------------------------------------------------------
// Developer scenes: REALM_SCENE=token,token,... (see README)

void RealmGame::applyDevScene(gamelib::app::App& app, const std::string& scene)
{
    if (scene.starts_with("sheet")) {
        m_mode = Mode::Sheet;
        m_sheet = scene.size() > 5 ? toInt(std::string_view{scene}.substr(5)) : 0;
        return;
    }
    if (scene == "select") {
        toSelect(app);
        return;
    }
    if (scene == "create") {
        toSelect(app);
        startNew(app);
        return;
    }
    if (scene == "title") {
        return;
    }
    // Anything else plays a fresh character in slot 3 (never saved over a
    // real one: the scene's slot is the dev data folder's).
    m_slot = 2;
    m_session.emplace(game::Appearance{.name = "Rin", .hair = 0, .hairColor = 0, .eyes = 0, .skin = 0}, gamelib::core::randomSeed());
    m_actions = makeActions(game::Job::Beginner);
    enterPlay(app);
    game::Session& s = *m_session;
    std::istringstream tokens{scene};
    std::string token;
    while (std::getline(tokens, token, ',')) {
        const std::string_view t{token};
        if (t.starts_with("map")) {
            s.debugTeleport(static_cast<game::MapId>(std::clamp(toInt(t.substr(3)), 0, game::kMapCount - 1)), 0);
            afterTravel();
        } else if (t.starts_with("level")) {
            s.debugLevel(toInt(t.substr(5)));
        } else if (t == "warrior" || t == "mage") {
            s.debugJob(t == "warrior" ? game::Job::Warrior : game::Job::Mage);
            addJobActions(m_actions, s.character().job);
        } else if (t == "night") {
            s.debugTime(22);
        } else if (t == "dusk") {
            s.debugTime(18);
        } else if (t == "gear") {
            s.debugGear();
        } else if (t == "learn") {
            s.debugLearnAll();
        } else if (t.starts_with("quest")) {
            s.debugQuestsTo(toInt(t.substr(5)));
        } else if (t.starts_with("x")) {
            s.debugMove(static_cast<float>(toInt(t.substr(1))) * game::kTile);
            m_view.snap(s);
        } else if (t == "bosslow") {
            s.debugBossLow();
        } else if (t == "stats") {
            openWindow(app, WindowId::Stats);
        } else if (t == "skills") {
            openWindow(app, WindowId::Skills);
        } else if (t == "inventory") {
            openWindow(app, WindowId::Inventory);
        } else if (t == "equipment") {
            openWindow(app, WindowId::Equipment);
        } else if (t == "quests") {
            openWindow(app, WindowId::Quests);
        } else if (t == "worldmap") {
            openWindow(app, WindowId::WorldMap);
        } else if (t == "keys") {
            openWindow(app, WindowId::Keys);
        } else if (t.starts_with("talk")) {
            openTalk(app, static_cast<game::NpcId>(std::clamp(toInt(t.substr(4)), 0, game::kNpcCount - 1)));
        } else if (t == "voyage") {
            m_voyageTime = 0.0f;
        } else if (t == "ending") {
            m_endingDue = 0.0f;
        }
    }
    for (const game::Event& e : s.events().take()) {
        (void)e;
    }
    m_hud.reset(s);
    m_view.snap(s);
}

// ---------------------------------------------------------------------------
// Input

void RealmGame::onFocus(gamelib::app::App& app, const bool focused)
{
    if (!m_demoMode && m_mode == Mode::Play && !m_windows.isOpen() && !m_dialogue.isOpen() && m_voyageTime < 0.0f) {
        (void)m_menus.onFocus(app, focused, m_time);
    }
}

void RealmGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (m_demoMode) {
        if (event.pressed && event.key != Key::F11) {
            m_demoMode = false;
            app.input().releaseAll();
        }
        return;
    }
    const auto input = gamelib::ui::menuInput(m_actions, event, menuBindings());
    const ActionId action = m_actions.actionOn(event.key);
    switch (m_mode) {
    case Mode::Sheet:
        if (event.pressed && input) {
            if (*input == MenuInput::Back) {
                app.requestQuit();
            } else if (*input == MenuInput::Right || *input == MenuInput::Confirm) {
                m_sheet = (m_sheet + 1) % 3;
            } else if (*input == MenuInput::Left) {
                m_sheet = (m_sheet + 2) % 3;
            }
        }
        return;
    case Mode::Ending:
        if (event.pressed && m_endingTime > kEndingInputDelay && input && (*input == MenuInput::Confirm || *input == MenuInput::Back)) {
            // The story is done; the world stays open.
            m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xFFF4E0), .coverSeconds = 0.8f,
                                .holdSeconds = 0.2f, .uncoverSeconds = 0.8f},
                               [this] {
                                   m_mode = Mode::Play;
                                   m_menus.go(ArcadeScreen::Playing, m_time);
                                   m_hud.banner("the journey goes on", "the realm is yours to roam", srgb(0xFFE08A), 4.0f);
                               });
        }
        return;
    case Mode::Select:
        if (event.pressed) {
            if (event.key == Key::Delete || event.key == Key::Backspace) {
                selectInput(app, MenuInput::Up); // Up stands for "erase" on this screen
            } else if (input) {
                selectInput(app, *input);
            }
        }
        return;
    case Mode::Create:
        if (event.pressed) {
            if (!createKey(app, event.key) && input) {
                createInput(app, *input);
            }
        }
        return;
    case Mode::Title:
    case Mode::Play: break;
    }
    if (m_mode == Mode::Play) {
        if (m_voyageTime >= 0.0f) {
            if (event.pressed && input && *input == MenuInput::Confirm && m_voyageTime > 1.0f) {
                m_voyageTime = std::max(m_voyageTime, kVoyageSeconds); // skip the rest of the voyage
            }
            return;
        }
        if (m_dialogue.isOpen()) {
            if (event.pressed && input) {
                dialogueInput(app, *input);
            }
            return;
        }
        if (m_windows.isOpen()) {
            if (event.pressed) {
                windowKey(app, event);
            }
            return;
        }
        if (m_session && (m_session->phase() != game::Phase::Playing || m_transition.active() || m_endingDue >= 0.0f)) {
            return;
        }
    }
    const gamelib::app::ArcadeResult result = m_menus.onKey(app, event, m_time);
    handleResult(app, result);
    if (event.pressed && m_mode == Mode::Play && result.from == ArcadeScreen::Playing && m_menus.is(ArcadeScreen::Playing)) {
        (void)playKey(app, action);
    }
}

void RealmGame::onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event)
{
    if (m_demoMode) {
        if (event.pressed) {
            m_demoMode = false;
            app.input().releaseAll();
        }
        return;
    }
    const auto input = gamelib::ui::menuInput(m_actions, event, menuBindings());
    const ActionId action = m_actions.actionOn(event.button);
    if (!event.pressed) {
        if (m_mode == Mode::Title || m_mode == Mode::Play) {
            handleResult(app, m_menus.onGamepad(app, event, m_time));
        }
        return;
    }
    switch (m_mode) {
    case Mode::Sheet:
        if (input && *input == MenuInput::Back) {
            app.requestQuit();
        }
        return;
    case Mode::Ending:
        if (m_endingTime > kEndingInputDelay && input) {
            m_mode = Mode::Play;
            m_menus.go(ArcadeScreen::Playing, m_time);
        }
        return;
    case Mode::Select:
        if (input) {
            selectInput(app, *input);
        }
        return;
    case Mode::Create:
        if (input) {
            createInput(app, *input);
        }
        return;
    case Mode::Title:
    case Mode::Play: break;
    }
    if (m_mode == Mode::Play) {
        if (m_voyageTime >= 0.0f) {
            return;
        }
        if (m_dialogue.isOpen()) {
            if (input) {
                dialogueInput(app, *input);
            }
            return;
        }
        if (m_windows.isOpen() && m_session) {
            if (event.button == gamelib::platform::GamepadButton::Start || event.button == gamelib::platform::GamepadButton::Back) {
                m_windows.close();
                windowFeedback(app, WindowFeedback::Closed);
            } else if (event.button == gamelib::platform::GamepadButton::LeftBumper) {
                windowFeedback(app, m_windows.handleKey(Key::Tab, *m_session, m_actions));
            } else if (input) {
                windowFeedback(app, m_windows.handle(*input, *m_session, m_actions));
            }
            if (!m_windows.isOpen()) {
                app.input().releaseAll();
            }
            return;
        }
        if (m_session && (m_session->phase() != game::Phase::Playing || m_transition.active())) {
            return;
        }
    }
    const gamelib::app::ArcadeResult result = m_menus.onGamepad(app, event, m_time);
    handleResult(app, result);
    if (m_mode == Mode::Play && result.from == ArcadeScreen::Playing && m_menus.is(ArcadeScreen::Playing)) {
        if (event.button == gamelib::platform::GamepadButton::Back) {
            openWindow(app, WindowId::Inventory);
        } else {
            (void)playKey(app, action);
        }
    }
}

void RealmGame::onMouse(gamelib::app::App& app, const gamelib::platform::MouseEvent& event)
{
    if (m_mode == Mode::Play && m_session && m_windows.isOpen() && !m_dialogue.isOpen()) {
        const WindowFeedback f = m_windows.handleMouse(event, *m_session, m_actions);
        if (f != WindowFeedback::None) {
            windowFeedback(app, f);
        }
    }
}

void RealmGame::windowKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (!m_session) {
        return;
    }
    game::Session& s = *m_session;
    const std::optional<WindowId> current = m_windows.current();
    if (m_windows.wantsAllKeys()) {
        // The keybinding window takes every key (GameLib's KeyPalette: arrows,
        // Enter, Escape, Tab and Delete drive it; other keys are placed on).
        windowFeedback(app, m_windows.handleKey(event.key, s, m_actions));
    } else {
        const ActionId action = m_actions.actionOn(event.key);
        const std::optional<WindowId> target = windowFor(action);
        const std::optional<MenuInput> fixed = fixedMenuKey(event.key);
        if (target && !fixed) {
            if (target == current) {
                m_windows.close();
                windowFeedback(app, WindowFeedback::Closed);
            } else {
                openWindow(app, *target);
            }
        } else if (fixed) {
            windowFeedback(app, m_windows.handle(*fixed, s, m_actions));
        } else if (const auto input = gamelib::ui::menuInput(m_actions, event, menuBindings());
                   input && (*input == MenuInput::Confirm || *input == MenuInput::Back) && current != WindowId::Skills
                   && current != WindowId::Inventory) {
            windowFeedback(app, m_windows.handle(*input, s, m_actions));
        } else {
            windowFeedback(app, m_windows.handleKey(event.key, s, m_actions));
        }
    }
    if (!m_windows.isOpen()) {
        app.input().releaseAll();
        m_pendingService.reset();
    }
}

bool RealmGame::playKey(gamelib::app::App& app, const ActionId action)
{
    if (!m_session || m_session->phase() != game::Phase::Playing) {
        return false;
    }
    if (const std::optional<WindowId> w = windowFor(action)) {
        openWindow(app, *w);
        return true;
    }
    if (action == actionId(Action::Minimap)) {
        prefs().minimap = (prefs().minimap + 2) % 3; // small -> hidden -> large -> small
        m_profile.save();
        m_sfx.queue(app.sounds(), Sfx::MenuMove);
        return true;
    }
    return false;
}

void RealmGame::openWindow(gamelib::app::App& app, const WindowId id)
{
    if (!m_session) {
        return;
    }
    m_windows.open(id, *m_session, m_actions);
    m_sfx.queue(app.sounds(), Sfx::Open);
    app.input().releaseAll();
}

void RealmGame::windowFeedback(gamelib::app::App& app, const WindowFeedback feedback)
{
    auto& q = app.sounds();
    switch (feedback) {
    case WindowFeedback::Moved:
    case WindowFeedback::Tab: m_sfx.queue(q, Sfx::MenuMove); break;
    case WindowFeedback::Done: m_sfx.queue(q, m_windows.current() == WindowId::Stats ? Sfx::Point : Sfx::MenuSelect); break;
    case WindowFeedback::Failed: m_sfx.queue(q, Sfx::Error); break;
    case WindowFeedback::Closed: m_sfx.queue(q, Sfx::Close); break;
    case WindowFeedback::Big: m_sfx.queue(q, Sfx::Learn); break;
    case WindowFeedback::Equip: m_sfx.queue(q, Sfx::Equip); break;
    case WindowFeedback::Bound:
        m_sfx.queue(q, Sfx::Bind);
        saveNow();
        break;
    case WindowFeedback::Unbound:
        m_sfx.queue(q, Sfx::MenuBack);
        saveNow();
        break;
    case WindowFeedback::None: break;
    }
    // Purchases, equips and points push events: play their sounds now.
    if (m_session) {
        for (const game::Event& e : m_session->events().take()) {
            react(app, e);
        }
    }
}

void RealmGame::handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result)
{
    if (result.changed) {
        m_profile.save();
        gamelib::app::applySettings(app, m_profile.value().settings);
        m_effects.setShakeScale(m_profile.value().settings.screenShake ? 1.0f : 0.0f);
    }
    switch (result.action) {
    case ArcadeAction::Start:
        if (result.from == ArcadeScreen::Paused) {
            // RESTART: back to the last save.
            startSlot(app, m_slot);
            break;
        }
        toSelect(app);
        break;
    case ArcadeAction::ToTitle: toTitle(app); break;
    case ArcadeAction::Quit:
        saveNow();
        m_profile.save();
        app.requestQuit();
        break;
    case ArcadeAction::Game:
        if (result.command == kKeysCommand) {
            openWindow(app, WindowId::Keys);
        } else if (result.command == kCharactersCommand) {
            saveNow();
            toSelect(app);
        }
        break;
    case ArcadeAction::Resume:
    case ArcadeAction::Controls:
    case ArcadeAction::None: break;
    }
}

void RealmGame::selectInput(gamelib::app::App& app, const MenuInput input)
{
    auto& q = app.sounds();
    switch (input) {
    case MenuInput::Left:
    case MenuInput::Right: {
        const int step = input == MenuInput::Left ? -1 : 1;
        m_slot = (m_slot + step + kSlotCount) % kSlotCount;
        m_eraseAsked = false;
        m_sfx.queue(q, Sfx::MenuMove);
        break;
    }
    case MenuInput::Up: // erase (Delete), asked twice
    case MenuInput::Down:
        if (input == MenuInput::Down) {
            break;
        }
        if (!m_cards[static_cast<std::size_t>(m_slot)].exists) {
            m_sfx.queue(q, Sfx::Error);
        } else if (!m_eraseAsked) {
            m_eraseAsked = true;
            m_sfx.queue(q, Sfx::Error);
        } else {
            m_saves.erase(m_slot);
            m_eraseAsked = false;
            refreshCards();
            m_sfx.queue(q, Sfx::Sell);
        }
        break;
    case MenuInput::Confirm: {
        m_eraseAsked = false;
        const SlotCard& card = m_cards[static_cast<std::size_t>(m_slot)];
        m_sfx.queue(q, Sfx::MenuSelect);
        if (card.exists && card.readable) {
            startSlot(app, m_slot);
        } else {
            startNew(app);
        }
        break;
    }
    case MenuInput::Back:
        if (m_eraseAsked) {
            m_eraseAsked = false;
            m_sfx.queue(q, Sfx::MenuBack);
            break;
        }
        m_sfx.queue(q, Sfx::MenuBack);
        m_mode = Mode::Title;
        m_menus.go(ArcadeScreen::Title, m_time);
        break;
    }
}

void RealmGame::createInput(gamelib::app::App& app, const MenuInput input)
{
    auto& q = app.sounds();
    Creator& c = m_creator;
    constexpr int kRows = 6;
    switch (input) {
    case MenuInput::Up:
        c.row = (c.row + kRows - 1) % kRows;
        m_sfx.queue(q, Sfx::MenuMove);
        break;
    case MenuInput::Down:
        c.row = (c.row + 1) % kRows;
        m_sfx.queue(q, Sfx::MenuMove);
        break;
    case MenuInput::Left:
    case MenuInput::Right: {
        const int step = input == MenuInput::Left ? -1 : 1;
        const auto cycle = [step](int& v, const int n) { v = (v + step + n) % n; };
        switch (c.row) {
        case 0: {
            // A suggested name.
            const auto it = std::ranges::find(kNames, std::string_view{c.name.text()});
            int i = it == kNames.end() ? 0 : static_cast<int>(it - kNames.begin());
            cycle(i, static_cast<int>(kNames.size()));
            c.name.setText(std::string{kNames[static_cast<std::size_t>(i)]}, true);
            c.look.name = c.name.text();
            break;
        }
        case 1: cycle(c.look.hair, 8); break;
        case 2: cycle(c.look.hairColor, kHairColors); break;
        case 3: cycle(c.look.eyes, 8); break;
        case 4: cycle(c.look.skin, 4); break;
        default: break;
        }
        c.age = 0.0f;
        m_sfx.queue(q, Sfx::MenuMove);
        break;
    }
    case MenuInput::Confirm:
        if (c.row < 5) {
            c.row = 5;
            m_sfx.queue(q, Sfx::MenuMove);
            break;
        }
        c.look.name = c.name.trimmed();
        if (c.look.name.empty()) {
            c.row = 0;
            m_sfx.queue(q, Sfx::Error);
            break;
        }
        m_sfx.queue(q, Sfx::JobAdvance);
        m_session.emplace(c.look, gamelib::core::randomSeed());
        m_actions = makeActions(game::Job::Beginner);
        m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xFFF4E0), .coverSeconds = 0.6f, .holdSeconds = 0.2f,
                            .uncoverSeconds = 0.8f},
                           [this, &app] {
                               enterPlay(app);
                               saveNow();
                               m_hud.banner("welcome to petal isle", "talk to nana rue - press up beside her", srgb(0xFFE08A), 5.0f);
                           });
        break;
    case MenuInput::Back:
        m_sfx.queue(q, Sfx::MenuBack);
        m_mode = Mode::Select;
        break;
    }
}

bool RealmGame::createKey(gamelib::app::App& app, const Key key)
{
    // On the name row, typing keys belong to the field (their characters
    // arrive in onText); Backspace and Delete erase.
    Creator& c = m_creator;
    if (c.row != 0 || !c.name.consumes(key)) {
        return false;
    }
    const gamelib::ui::TextEdit edit = c.name.handleKey(key);
    if (edit == gamelib::ui::TextEdit::Erased) {
        m_sfx.queue(app.sounds(), Sfx::Type);
    }
    c.look.name = c.name.text();
    return true;
}

void RealmGame::onText(gamelib::app::App& app, const gamelib::platform::TextEvent& event)
{
    if (m_mode != Mode::Create || m_creator.row != 0 || m_transition.active()) {
        return;
    }
    Creator& c = m_creator;
    const gamelib::ui::TextEdit edit = c.name.type(event.codepoint);
    m_sfx.queue(app.sounds(), edit == gamelib::ui::TextEdit::Typed ? Sfx::Type : Sfx::Error);
    c.look.name = c.name.text();
}

void RealmGame::dialogueInput(gamelib::app::App& app, const MenuInput input)
{
    if (!m_runner) {
        m_dialogue.close();
        return;
    }
    switch (m_dialogue.handle(input)) {
    case DialogueStep::Completed: m_sfx.queue(app.sounds(), Sfx::MenuMove, 0.5f); break;
    case DialogueStep::Next:
        m_sfx.queue(app.sounds(), Sfx::MenuMove, 0.6f);
        m_runner->advance();
        showNode(app);
        break;
    case DialogueStep::Chose:
        m_sfx.queue(app.sounds(), Sfx::MenuSelect);
        m_runner->choose(m_dialogue.choice());
        showNode(app);
        break;
    case DialogueStep::None:
        if (input == MenuInput::Up || input == MenuInput::Down) {
            m_sfx.queue(app.sounds(), Sfx::MenuMove, 0.6f);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// Flow

void RealmGame::refreshCards()
{
    for (int i = 0; i < kSlotCount; ++i) {
        SlotCard& card = m_cards[static_cast<std::size_t>(i)];
        card.exists = m_saves.exists(i);
        card.readable = m_saves.readable(i);
        card.data.reset();
        if (card.exists && card.readable) {
            if (std::optional<LoadedSlot> loaded = m_saves.load(i)) {
                card.data = std::move(loaded->data);
            }
        }
    }
}

void RealmGame::toSelect(gamelib::app::App& app)
{
    m_session.reset();
    m_windows.close();
    m_dialogue.close();
    m_runner.reset();
    m_transition.cancel();
    m_voyageTime = -1.0f;
    m_endingDue = -1.0f;
    refreshCards();
    m_slot = std::clamp(prefs().lastSlot, 0, kSlotCount - 1);
    m_eraseAsked = false;
    m_selectAge = 0.0f;
    m_mode = Mode::Select;
    m_menus.go(ArcadeScreen::Title, m_time);
    app.input().releaseAll();
}

void RealmGame::startNew(gamelib::app::App& app)
{
    gamelib::core::Rng rng{gamelib::core::randomSeed()};
    m_creator = {};
    m_creator.name.setText(std::string{kNames[static_cast<std::size_t>(rng.below(static_cast<std::uint32_t>(kNames.size())))]}, true);
    m_creator.look.name = m_creator.name.text();
    m_creator.look.hair = static_cast<int>(rng.below(8));
    m_creator.look.hairColor = static_cast<int>(rng.below(kHairColors));
    m_creator.look.eyes = static_cast<int>(rng.below(8));
    m_mode = Mode::Create;
    app.input().releaseAll();
}

void RealmGame::startSlot(gamelib::app::App& app, const int slot)
{
    std::optional<LoadedSlot> loaded = m_saves.load(slot);
    if (!loaded) {
        startNew(app);
        return;
    }
    m_slot = slot;
    m_session.emplace(loaded->data, gamelib::core::randomSeed());
    m_actions = actionsFor(m_session->character().job, &loaded->ini);
    m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0x1E1830), .coverSeconds = 0.4f, .holdSeconds = 0.1f,
                        .uncoverSeconds = 0.6f},
                       [this, &app] { enterPlay(app); });
}

void RealmGame::enterPlay(gamelib::app::App& app)
{
    prefs().lastSlot = m_slot;
    m_profile.save();
    (void)m_session->events().take();
    m_view.snap(*m_session);
    m_effects.clear();
    m_effects.setPlace(m_session->map());
    m_effects.setShakeScale(m_profile.value().settings.screenShake ? 1.0f : 0.0f);
    m_hud.reset(*m_session);
    m_hud.mapTitle(*m_session);
    m_windows.close();
    m_dialogue.close();
    m_runner.reset();
    m_pendingService.reset();
    m_mode = Mode::Play;
    m_menus.go(ArcadeScreen::Playing, m_time);
    app.input().releaseAll();
    m_deadTime = 0.0f;
    m_voyageTime = -1.0f;
    m_endingDue = -1.0f;
    updateMusic(app);
}

void RealmGame::toTitle(gamelib::app::App& app)
{
    saveNow();
    m_session.reset();
    m_transition.cancel();
    m_windows.close();
    m_dialogue.close();
    m_runner.reset();
    m_voyageTime = -1.0f;
    m_endingDue = -1.0f;
    m_mode = Mode::Title;
    m_menus.go(ArcadeScreen::Title, m_time);
    m_profile.save();
    m_music.play(app.audio(), Tune::Title, 0.8f);
    app.input().releaseAll();
}

void RealmGame::openTalk(gamelib::app::App& app, const game::NpcId npc)
{
    game::Session& s = *m_session;
    s.startTalk(npc);
    m_runner = std::make_unique<gamelib::core::DialogueRunner>(
        game::dialogueTree(), [&s](const std::string_view c) { return s.condition(c); }, [&s](const std::string_view e) { s.effect(e); });
    m_runner->start(game::npcRoot(npc));
    m_sfx.queue(app.sounds(), Sfx::Open);
    app.input().releaseAll();
    showNode(app);
}

void RealmGame::showNode(gamelib::app::App& app)
{
    game::Session& s = *m_session;
    for (const game::Event& e : s.events().take()) {
        react(app, e);
    }
    if (std::optional<std::string> service = s.takeServiceRequest()) {
        m_pendingService = std::move(service);
    }
    if (!m_runner || m_runner->done() || m_runner->node() == nullptr) {
        m_dialogue.close();
        m_runner.reset();
        app.input().releaseAll();
        if (m_pendingService) {
            if (*m_pendingService == "shop" || *m_pendingService == "gearshop") {
                openWindow(app, WindowId::Shop);
            }
            m_pendingService.reset();
        }
        return;
    }
    const gamelib::core::DialogueNode* node = m_runner->node();
    const std::optional<game::NpcId> npc = npcBySpeaker(node->speaker);
    std::vector<std::string> choices = m_runner->choiceLabels();
    for (std::string& c : choices) {
        c = gamelib::platform::withKeys(c, m_actions);
    }
    m_dialogue.open(node->speaker, npc ? std::string{game::npcRole(*npc)} : std::string{}, gamelib::platform::withKeys(node->text, m_actions),
                    std::move(choices), npc);
}

void RealmGame::afterTravel()
{
    game::Session& s = *m_session;
    for (const game::Event& e : s.events().take()) {
        (void)e;
    }
    m_view.snap(s);
    m_effects.clear();
    m_effects.setPlace(s.map());
    m_hud.mapTitle(s);
    if (s.boss()) {
        m_hud.reset(s);
    }
}

void RealmGame::startEnding(gamelib::app::App& app)
{
    if (!m_session) {
        return;
    }
    m_endStats = m_session->stats();
    m_endLevel = m_session->character().level;
    saveNow();
    m_newBest = m_profile.value().bests.offer(kBestTime, m_endStats.ticks, gamelib::app::Bests::Better::Lower);
    m_profile.save();
    m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xFFF4E0), .coverSeconds = 1.6f, .holdSeconds = 0.6f,
                        .uncoverSeconds = 1.6f},
                       [this, &app] {
                           m_mode = Mode::Ending;
                           m_endingTime = 0.0f;
                           m_windows.close();
                           m_dialogue.close();
                           m_runner.reset();
                           m_music.play(app.audio(), Tune::Ending, 1.5f);
                       });
}

void RealmGame::tick(gamelib::app::App& app)
{
    if (m_demoMode || m_mode == Mode::Title || m_mode == Mode::Select || m_mode == Mode::Create) {
        m_attract.tick(app.tickSeconds());
        return;
    }
    if (m_mode != Mode::Play || !m_session || !m_menus.is(ArcadeScreen::Playing) || m_windows.isOpen() || m_dialogue.isOpen()
        || m_voyageTime >= 0.0f) {
        return;
    }
    if (m_transition.active() && !m_session->travelPending()) {
        return;
    }
    playTick(app);
}

void RealmGame::playTick(gamelib::app::App& app)
{
    game::Session& s = *m_session;
    if (s.travelPending()) {
        return; // the transition is running
    }
    s.tick(readInput(m_actions, app.input()));
    for (const game::Event& e : s.events().take()) {
        react(app, e);
    }
    if (const std::optional<game::NpcId> talk = s.takeTalkRequest()) {
        openTalk(app, *talk);
        return;
    }
    if (s.travelPending() && !m_transition.active()) {
        if (s.travelIsVoyage()) {
            m_sfx.queue(app.sounds(), Sfx::Horn);
            m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xEAF6FF), .coverSeconds = 0.7f,
                                .holdSeconds = 0.2f, .uncoverSeconds = 0.7f},
                               [this] { m_voyageTime = 0.0f; });
        } else {
            m_sfx.queue(app.sounds(), Sfx::Portal);
            m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0x1E1830), .coverSeconds = 0.3f,
                                .holdSeconds = 0.1f, .uncoverSeconds = 0.45f},
                               [this] {
                                   m_session->travel();
                                   afterTravel();
                               });
        }
    }
    if (s.wantsSave()) {
        saveNow();
    }
    if (s.takeEnding()) {
        m_endingDue = 3.0f; // let the last boss fall first
    }
}

void RealmGame::react(gamelib::app::App& app, const game::Event& e)
{
    using game::EventKind;
    if (!m_session) {
        return;
    }
    m_effects.react(e, *m_session);
    m_hud.react(e, *m_session);
    auto& q = app.sounds();
    switch (e.kind) {
    case EventKind::Jump: m_sfx.queue(q, Sfx::Jump); break;
    case EventKind::HyperJump: m_sfx.queue(q, Sfx::HyperJump); break;
    case EventKind::Land:
        if (e.value > 14) {
            m_sfx.queue(q, Sfx::Land);
        }
        break;
    case EventKind::Climb: m_sfx.queue(q, Sfx::Climb, 0.6f); break;
    case EventKind::Swing: m_sfx.queue(q, Sfx::Swing, 1.0f, e.extra == 0 ? 1.0f : 1.1f); break;
    case EventKind::Hit:
    case EventKind::SkillHit:
        m_sfx.queue(q, e.crit ? Sfx::Crit : Sfx::Hit, e.kind == EventKind::SkillHit ? 0.8f : 1.0f);
        if (e.crit) {
            app.hitStop(0.04f);
        }
        break;
    case EventKind::Chain: m_sfx.queue(q, Sfx::Spark, 0.5f, 1.2f); break;
    case EventKind::Star: m_sfx.queue(q, Sfx::Starfall, 0.5f, 1.15f); break;
    case EventKind::MonsterDown: m_sfx.queue(q, Sfx::MonsterDown, e.extra != 0 ? 1.0f : 0.8f); break;
    case EventKind::Hurt:
        m_sfx.queue(q, Sfx::Hurt);
        app.hitStop(0.05f);
        break;
    case EventKind::ShieldHit: m_sfx.queue(q, Sfx::ShieldHit); break;
    case EventKind::Death:
        m_sfx.queue(q, Sfx::Death);
        m_deadTime = 0.0f;
        break;
    case EventKind::Heal: m_sfx.queue(q, Sfx::Bloom, 0.7f); break;
    case EventKind::Pickup: {
        const auto r = static_cast<game::Rarity>(std::clamp(e.extra, 0, 3));
        m_sfx.queue(q, r == game::Rarity::Unique ? Sfx::PickupUnique
                       : r == game::Rarity::Epic ? Sfx::PickupEpic
                       : r == game::Rarity::Rare ? Sfx::PickupRare
                                                 : Sfx::Pickup);
        break;
    }
    case EventKind::Coins: m_sfx.queue(q, Sfx::Coin, 0.8f); break;
    case EventKind::LevelUp: m_sfx.queue(q, Sfx::LevelUp); break;
    case EventKind::SkillCast: m_sfx.queue(q, skillSound(static_cast<game::SkillId>(e.value))); break;
    case EventKind::SkillLearned: m_sfx.queue(q, Sfx::Learn); break;
    case EventKind::NoMana:
    case EventKind::Cooldown: m_sfx.queue(q, Sfx::NoMana, 0.7f); break;
    case EventKind::Potion: m_sfx.queue(q, Sfx::Potion); break;
    case EventKind::QuestAccepted: m_sfx.queue(q, Sfx::QuestAccept); break;
    case EventKind::QuestComplete: m_sfx.queue(q, Sfx::QuestComplete); break;
    case EventKind::QuestDone: m_sfx.queue(q, Sfx::QuestDone); break;
    case EventKind::PortalClosed: m_sfx.queue(q, Sfx::Sealed); break;
    case EventKind::BossWake: m_sfx.queue(q, Sfx::BossRoar); break;
    case EventKind::BossRoar: m_sfx.queue(q, Sfx::BossRoar, 0.6f); break;
    case EventKind::BossPhase: m_sfx.queue(q, Sfx::BossRoar, 1.0f, 1.2f); break;
    case EventKind::BossLand: m_sfx.queue(q, Sfx::BossLand); break;
    case EventKind::BossDown:
        m_sfx.queue(q, Sfx::BossDown);
        app.hitStop(0.25f);
        m_music.stop(app.audio(), 1.0f);
        break;
    case EventKind::EnemyShot: m_sfx.queue(q, Sfx::Shot, 0.7f); break;
    case EventKind::HazardWarn: m_sfx.queue(q, Sfx::HazardWarn); break;
    case EventKind::HazardStrike: m_sfx.queue(q, Sfx::BossLand, 0.6f); break;
    case EventKind::Rest:
        m_sfx.queue(q, Sfx::Rest);
        saveNow();
        break;
    case EventKind::JobAdvance:
        m_sfx.queue(q, Sfx::JobAdvance);
        addJobActions(m_actions, m_session->character().job);
        saveNow();
        break;
    case EventKind::Spawned: m_sfx.queue(q, Sfx::Spawn, 0.35f); break;
    case EventKind::Equip: m_sfx.queue(q, Sfx::Equip); break;
    case EventKind::Bought: m_sfx.queue(q, Sfx::Buy); break;
    case EventKind::Sold: m_sfx.queue(q, Sfx::Sell); break;
    case EventKind::Full: m_sfx.queue(q, Sfx::Error); break;
    case EventKind::Buff: m_sfx.queue(q, Sfx::Buff, 0.6f); break;
    case EventKind::Point: m_sfx.queue(q, Sfx::Point); break;
    case EventKind::Drop: m_sfx.queue(q, Sfx::Coin, 0.4f, 0.8f); break;
    case EventKind::Sail:
    case EventKind::BuffEnd:
    case EventKind::Prone:
    case EventKind::QuestUpdate:
    case EventKind::Travel:
    case EventKind::Ending:
    case EventKind::Saved: break;
    }
}

void RealmGame::updateMusic(gamelib::app::App& app)
{
    if (m_mode == Mode::Title || m_mode == Mode::Select || m_mode == Mode::Create || m_mode == Mode::Sheet) {
        m_music.play(app.audio(), Tune::Title, 0.8f);
        return;
    }
    if (m_mode == Mode::Ending) {
        m_music.play(app.audio(), Tune::Ending, 1.5f);
        return;
    }
    if (!m_session) {
        return;
    }
    if (m_voyageTime >= 0.0f) {
        m_music.play(app.audio(), Tune::Voyage, 1.0f);
        return;
    }
    const game::Session& s = *m_session;
    if (s.boss()) {
        if (s.boss()->fighting()) {
            m_music.play(app.audio(), s.boss()->id == game::BossId::Murkmaw ? Tune::FinalBoss : Tune::Boss, 0.6f);
            return;
        }
        if (!s.boss()->alive()) {
            return; // the boss-down silence, then the region again on the way out
        }
    }
    if (s.phase() == game::Phase::Dead) {
        return;
    }
    const game::MapDef& def = s.map();
    const bool nightTake = def.region != game::Region::Depths && s.nightAmount() > 0.6f;
    m_music.play(app.audio(), regionTune(def.region), 2.5f, nightTake ? 1 : 0);
}

void RealmGame::frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info)
{
    m_time = info.time;
    m_transition.update(info.seconds);
    m_windows.update(info.seconds);
    if (m_mode == Mode::Ending) {
        m_endingTime += info.seconds;
    }
    if (m_mode == Mode::Select) {
        m_selectAge += info.seconds;
    }
    if (m_mode == Mode::Create) {
        m_creator.age += info.seconds;
    }
    if (m_mode == Mode::Play && m_session) {
        game::Session& s = *m_session;
        const bool frozen = !m_menus.is(ArcadeScreen::Playing) || m_windows.isOpen() || m_dialogue.isOpen() || s.travelPending();
        m_alpha = frozen ? 1.0f : info.alpha;
        m_view.follow(s, m_alpha, info.seconds, m_time);
        m_effects.update(info.seconds, s, m_view.visible(), m_view.heroFeet(s, m_alpha));
        m_hud.update(info.seconds, s);
        m_dialogue.update(info.seconds);
        m_typed += m_dialogue.typedThisFrame();
        if (m_typed >= 3) {
            m_typed = 0;
            m_sfx.queue(app.sounds(), Sfx::Type, 0.45f);
        }
        // The voyage: a few seconds at sea, then the far dock.
        if (m_voyageTime >= 0.0f) {
            m_voyageTime += info.seconds;
            if (m_voyageTime > kVoyageSeconds && !m_transition.active()) {
                m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xEAF6FF), .coverSeconds = 0.6f,
                                    .holdSeconds = 0.2f, .uncoverSeconds = 0.6f},
                                   [this] {
                                       if (m_session->travelPending()) {
                                           m_session->travel();
                                       }
                                       afterTravel();
                                       m_voyageTime = -1.0f;
                                   });
            }
        }
        // The last boss fell: a moment, then the ending.
        if (m_endingDue >= 0.0f && !m_transition.active()) {
            m_endingDue -= info.seconds;
            if (m_endingDue < 0.0f) {
                startEnding(app);
            }
        }
        // Fainted: a moment, then wake up in town.
        if (s.phase() == game::Phase::Dead) {
            m_deadTime += info.seconds;
            if (m_deadTime > kFaintSeconds && !m_transition.active()) {
                m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xFFF4E0), .coverSeconds = 0.8f,
                                    .holdSeconds = 0.4f, .uncoverSeconds = 0.8f},
                                   [this] {
                                       m_session->respawn();
                                       afterTravel();
                                       m_hud.reset(*m_session);
                                       m_hud.banner("you wake up in town", "a little experience was lost", srgb(0xFFE08A), 3.5f);
                                       m_deadTime = 0.0f;
                                   });
            }
        }
    } else if (m_mode != Mode::Ending) {
        const game::Session& demo = m_attract.sim().session;
        if (demo.ticks() <= 1) {
            m_demoView.snap(demo);
            m_demoEffects.clear();
            m_demoEffects.setPlace(demo.map());
        }
        m_demoView.follow(demo, info.alpha, info.seconds, m_time);
        m_demoEffects.update(info.seconds, demo, m_demoView.visible(), m_demoView.heroFeet(demo, info.alpha));
    }
    updateMusic(app);
}

void RealmGame::draw(gamelib::app::App&, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    const float time = info.time;
    switch (m_mode) {
    case Mode::Sheet: drawSheet(list, m_shapes, m_canvas, m_sheet, time); break;
    case Mode::Title:
        drawTitle(list, time);
        drawMenu(list, time);
        break;
    case Mode::Select: drawSelect(list, time); break;
    case Mode::Create: drawCreate(list, time); break;
    case Mode::Ending: drawEnding(list, time); break;
    case Mode::Play:
        if (!m_session) {
            break;
        }
        if (m_voyageTime >= 0.0f) {
            drawVoyage(list, time);
            break;
        }
        {
            const Vec2 shake = m_effects.shake(time);
            m_view.draw(list, m_shapes, *m_session, m_effects, m_alpha, time, shake);
            list.setLayer(gamelib::draw::layers::Hud);
            m_effects.drawScreen(list, m_canvas);
            const bool quiet = m_dialogue.isOpen() || m_endingDue >= 0.0f;
            if (!quiet) {
                m_hud.draw(list, m_shapes, *m_session, m_actions, m_view.view(shake), prefs().minimap, time);
            }
            if (m_session->phase() == game::Phase::Dead) {
                drawFainted(list, time);
            }
            list.setLayer(gamelib::draw::layers::Hud + 20);
            m_windows.draw(list, m_shapes, *m_session, m_actions, time);
            m_dialogue.draw(list, m_shapes, time);
            drawMenu(list, time);
        }
        break;
    }
    list.setLayer(gamelib::draw::layers::Hud + 90);
    m_transition.draw(list, m_canvas);
}

} // namespace realm::app
