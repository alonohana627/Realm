#pragma once

// Realm on GameLib's main loop: the title (the demo bot grinding Sprout
// Meadow behind the logo and the menu), character select with three slots
// showing each character standing in its gear, the character creator (name,
// hair, hair color, eyes, skin), the world with its HUD, conversations and
// MapleStory-like windows, portals with soft transitions, the ship's voyage,
// fainting and waking in town, autosaves on every map change, region music
// crossfading by day and night, and the ending (the journey's numbers and
// the credits) after which play continues. The rules live in src/game; this
// is input, presentation, sound and saves.

#include "actions.hpp"
#include "dialogue_view.hpp"
#include "effects.hpp"
#include "hud.hpp"
#include "profile.hpp"
#include "saves.hpp"
#include "shapes.hpp"
#include "sounds.hpp"
#include "view.hpp"
#include "windows.hpp"

#include "demo.hpp"
#include "session.hpp"

#include <gamelib/ai/attract.hpp>
#include <gamelib/app/app.hpp>
#include <gamelib/app/arcade_menus.hpp>
#include <gamelib/core/dialogue.hpp>
#include <gamelib/ui/text_field.hpp>
#include <gamelib/ui/transition.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace realm::app {

inline constexpr float kEndingInputDelay = 5.0f;

class RealmGame final : public gamelib::app::Game {
public:
    RealmGame(gamelib::app::App& app, ProfileFile profile, bool demo);
    ~RealmGame() override;
    RealmGame(const RealmGame&) = delete("owns the menus that point into its profile");
    RealmGame& operator=(const RealmGame&) = delete("owns the menus that point into its profile");

    void onFocus(gamelib::app::App& app, bool focused) override;
    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event) override;
    void onMouse(gamelib::app::App& app, const gamelib::platform::MouseEvent& event) override;
    void onText(gamelib::app::App& app, const gamelib::platform::TextEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    enum class Mode { Title, Select, Create, Play, Ending, Sheet };

    struct DemoRun {
        game::Session session;
        game::DemoBot bot;
    };
    // A character slot as the select screen shows it.
    struct SlotCard {
        bool exists = false;
        bool readable = false;
        std::optional<game::SaveData> data;
    };
    // The creator's state.
    struct Creator {
        game::Appearance look;
        int row = 0;        // name, hair, color, eyes, skin, start
        // The name, typed (GameLib's text field); a suggestion stays selected
        // until the first key replaces it.
        gamelib::ui::TextField name{10, gamelib::ui::TextFilter::Name, gamelib::ui::TextCase::Capitalized};
        float age = 0.0f;
    };

    // Input.
    void handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result);
    void selectInput(gamelib::app::App& app, gamelib::ui::MenuInput input);
    void createInput(gamelib::app::App& app, gamelib::ui::MenuInput input);
    bool createKey(gamelib::app::App& app, gamelib::platform::Key key);
    bool playKey(gamelib::app::App& app, gamelib::platform::ActionId action);
    void windowKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event);
    void dialogueInput(gamelib::app::App& app, gamelib::ui::MenuInput input);
    void windowFeedback(gamelib::app::App& app, WindowFeedback feedback);
    void openWindow(gamelib::app::App& app, WindowId id);

    // Flow.
    void toSelect(gamelib::app::App& app);
    void refreshCards();
    void startSlot(gamelib::app::App& app, int slot);
    void startNew(gamelib::app::App& app);
    void enterPlay(gamelib::app::App& app);
    void toTitle(gamelib::app::App& app);
    void saveNow();
    void playTick(gamelib::app::App& app);
    void react(gamelib::app::App& app, const game::Event& e);
    void openTalk(gamelib::app::App& app, game::NpcId npc);
    void showNode(gamelib::app::App& app);
    void afterTravel();
    void startEnding(gamelib::app::App& app);
    void updateMusic(gamelib::app::App& app);
    void applyDevScene(gamelib::app::App& app, const std::string& scene);
    [[nodiscard]] Prefs& prefs() { return m_profile.value().game; }
    [[nodiscard]] const Prefs& prefs() const { return m_profile.value().game; }

    // Screens (screens.cpp).
    void drawTitle(gamelib::draw::DrawList& list, float time) const;
    void drawMenu(gamelib::draw::DrawList& list, float time) const;
    void drawSelect(gamelib::draw::DrawList& list, float time) const;
    void drawCreate(gamelib::draw::DrawList& list, float time) const;
    void drawVoyage(gamelib::draw::DrawList& list, float time) const;
    void drawEnding(gamelib::draw::DrawList& list, float time) const;
    void drawFainted(gamelib::draw::DrawList& list, float time) const;
    void drawStage(gamelib::draw::DrawList& list, float time) const;

    ProfileFile m_profile; // the menus edit its settings in place: declared first
    gamelib::app::ArcadeMenus m_menus;
    Sounds m_sfx;
    Music m_music;
    Shapes m_shapes;
    Saves m_saves;
    gamelib::core::Size m_canvas;
    Mode m_mode = Mode::Title;
    std::array<SlotCard, kSlotCount> m_cards;
    int m_slot = 0;
    bool m_eraseAsked = false;
    float m_selectAge = 0.0f;
    Creator m_creator;

    std::optional<game::Session> m_session;
    gamelib::platform::ActionMap m_actions;
    WorldView m_view;
    Effects m_effects;
    Hud m_hud;
    Windows m_windows;
    DialogueView m_dialogue{m_shapes};
    std::unique_ptr<gamelib::core::DialogueRunner> m_runner;
    std::optional<std::string> m_pendingService;
    gamelib::ui::Transition m_transition;
    float m_deadTime = 0.0f;
    float m_voyageTime = -1.0f; // >= 0 while the ship sails
    float m_endingDue = -1.0f;  // >= 0: seconds until the ending starts
    float m_endingTime = 0.0f;
    game::RunStats m_endStats;
    int m_endLevel = 1;
    bool m_newBest = false;
    int m_sheet = 0;

    gamelib::ai::AttractMode<DemoRun> m_attract;
    WorldView m_demoView;
    Effects m_demoEffects;
    bool m_demoMode = false;
    float m_time = 0.0f;
    float m_alpha = 1.0f;
    int m_typed = 0;
};

} // namespace realm::app
