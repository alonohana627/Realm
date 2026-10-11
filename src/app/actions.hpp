#pragma once

// Realm's controls as GameLib actions (platform::ActionMap), MapleStory-like:
// moving, jumping, attacking, picking up and talking; a hotkey action per
// skill a character can use and per potion and scroll; the windows (stats,
// skills, items, equipment, quests, the world map, the key bindings). Every
// character keeps its own layout (in its save), placed in the keybinding
// window; a job's skills are registered when the character takes the job
// (they never take a key the player bound). Pause (Escape, Start) is
// locked: GameLib's menus own it.

#include "rules.hpp"
#include "skills.hpp"

#include <gamelib/platform/actions.hpp>
#include <gamelib/platform/input.hpp>
#include <gamelib/ui/menu.hpp>

#include <cstdint>
#include <optional>

namespace realm::app {

// The fixed actions (registered first, in this order).
enum class Action : std::uint8_t {
    Left, Right, Up, Down, Jump, Attack, Pickup,
    Stats, Skills, Inventory, Equipment, Quests, WorldMap, Minimap, Keys,
    Pause, Count
};

[[nodiscard]] constexpr gamelib::platform::ActionId actionId(const Action action)
{
    return static_cast<gamelib::platform::ActionId>(action);
}

// The actions for a job: the fixed ones, the beginner's hotkey skill, the
// job's hotkey skills and the use items.
[[nodiscard]] gamelib::platform::ActionMap makeActions(game::Job job);
// Adds the job's skill actions to a map made for an earlier job (the
// advancement), keeping every binding the player made.
void addJobActions(gamelib::platform::ActionMap& actions, game::Job job);

// What an action stands for beyond the fixed ones.
[[nodiscard]] std::optional<game::SkillId> actionSkill(const gamelib::platform::ActionMap& actions, gamelib::platform::ActionId id);
[[nodiscard]] int actionItem(const gamelib::platform::ActionMap& actions, gamelib::platform::ActionId id); // 0: none
[[nodiscard]] std::optional<gamelib::platform::ActionId> skillAction(const gamelib::platform::ActionMap& actions, game::SkillId skill);
[[nodiscard]] std::optional<gamelib::platform::ActionId> itemAction(const gamelib::platform::ActionMap& actions, int item);

// The rules' input for this tick from the held and pressed actions.
[[nodiscard]] game::Input readInput(const gamelib::platform::ActionMap& actions, const gamelib::platform::InputState& input);

// The player's keys on screens that are not a ui::Menu.
[[nodiscard]] const gamelib::ui::MenuBindings& menuBindings();

} // namespace realm::app
