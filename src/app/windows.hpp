#pragma once

// The MapleStory-like windows, each a soft rounded panel opened over the
// paused world (GameLib's ui::WindowSwitch pops one in and lets it drop
// away): the stat window (four core stats raised with ability points, a
// preview of every change a point makes, auto-assign, HP, MP, Speed, Jump,
// attack, defense, critical), the skill window (a tab per job tier, icons,
// levels, what the next point does; press a key to place the skill on it),
// the inventory in three tabs with item cards (use, equip, drop; a key on a
// potion places it), the equipment window around the hero's portrait, the
// quest log, the world map, the keybinding window (a keyboard drawn on
// screen; pick a skill, item or action from the palette and place it on a
// key, with the mouse too) and the shops. Keyboard, gamepad and mouse.

#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/transients.hpp>
#include <gamelib/platform/actions.hpp>
#include <gamelib/platform/input.hpp>
#include <gamelib/ui/item_grid.hpp>
#include <gamelib/ui/key_palette.hpp>
#include <gamelib/ui/list_view.hpp>
#include <gamelib/ui/menu.hpp>
#include <gamelib/ui/stat_window.hpp>
#include <gamelib/ui/window_switch.hpp>

#include <optional>
#include <string>
#include <vector>

namespace realm::app {

enum class WindowId { Stats, Skills, Inventory, Equipment, Quests, WorldMap, Keys, Shop };

// What a key press did in a window (for sounds and saving).
enum class WindowFeedback { None, Moved, Done, Failed, Closed, Tab, Big, Bound, Unbound, Equip };

class Windows {
public:
    Windows();

    void open(WindowId id, const game::Session& session, const gamelib::platform::ActionMap& actions);
    void close() { m_switch.close(); }
    [[nodiscard]] std::optional<WindowId> current() const { return m_switch.current(); }
    [[nodiscard]] bool isOpen() const { return m_switch.isOpen(); }
    [[nodiscard]] bool isOpen(WindowId id) const { return m_switch.isOpen(id); }

    // Menu directions, confirm and back.
    WindowFeedback handle(gamelib::ui::MenuInput input, game::Session& session, gamelib::platform::ActionMap& actions);
    // Any other key: a hotkey placed from the skill window, the inventory or
    // the keybinding window; Tab switches tabs; Delete clears or drops.
    WindowFeedback handleKey(gamelib::platform::Key key, game::Session& session, gamelib::platform::ActionMap& actions);
    WindowFeedback handleMouse(const gamelib::platform::MouseEvent& event, game::Session& session, gamelib::platform::ActionMap& actions);

    void update(float seconds);
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
              const gamelib::platform::ActionMap& actions, float time) const;
    // True while the window wants raw keys (the keybinding window places
    // whatever key is pressed).
    [[nodiscard]] bool wantsAllKeys() const;

private:
    void drawStats(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Rect rect,
                   float time) const;
    void drawSkills(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                    const gamelib::platform::ActionMap& actions, gamelib::core::Rect rect, float time) const;
    void drawInventory(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                       const gamelib::platform::ActionMap& actions, gamelib::core::Rect rect, float time) const;
    void drawEquipment(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Rect rect,
                       float time) const;
    void drawQuests(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Rect rect) const;
    void drawWorldMap(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Rect rect,
                      float time) const;
    void drawKeys(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                  const gamelib::platform::ActionMap& actions, gamelib::core::Rect rect, float time) const;
    void drawShop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Rect rect,
                  float time) const;

    [[nodiscard]] gamelib::core::Rect windowRect(WindowId id) const;
    [[nodiscard]] std::vector<game::SkillId> skillRows(const game::Session& session) const;
    [[nodiscard]] std::vector<int> questRows(const game::Session& session) const;

    // The keybinding window's palette entries and its layout.
    struct PaletteEntry {
        gamelib::platform::ActionId action = gamelib::platform::kNoAction;
        std::string label;
        std::optional<game::SkillId> skill;
        int item = 0;
        bool usable = true;
    };
    [[nodiscard]] std::vector<PaletteEntry> palette(const game::Session& session, const gamelib::platform::ActionMap& actions) const;
    [[nodiscard]] gamelib::core::Rect keyboardRect() const;
    [[nodiscard]] gamelib::core::Rect paletteRect() const;
    void refreshPalette(const game::Session& session, const gamelib::platform::ActionMap& actions);
    WindowFeedback keysResult(const gamelib::ui::KeyPaletteResult& result, const gamelib::platform::ActionMap& actions);

    gamelib::ui::WindowSwitch<WindowId> m_switch;
    gamelib::ui::StatWindow m_stats{game::kCoreCount};
    gamelib::ui::ListView m_skillList{8, true};
    gamelib::ui::Tabs m_skillTabs{{"beginner", "job"}};
    gamelib::ui::Tabs m_bagTabs{{"equip", "use", "etc"}};
    gamelib::ui::SlotGrid m_bagGrid;
    gamelib::ui::SlotGrid m_gearGrid;
    gamelib::ui::ListView m_questList{7, true};
    gamelib::ui::Tabs m_questTabs{{"in progress", "done"}};
    gamelib::ui::Tabs m_shopTabs{{"buy", "sell"}};
    gamelib::ui::ListView m_shopList{7, true};
    int m_quantity = 1;
    int m_mapCursor = 0;
    // The keybinding window: GameLib's KeyPalette, six entries by three rows.
    gamelib::ui::KeyPalette m_keys{6, 3};
    std::string m_message;
    float m_messageAge = 0.0f;
    float m_age = 0.0f;
    struct Flash {
        int row = 0;
    };
    gamelib::fx::Transients<Flash> m_flash;
};

} // namespace realm::app
