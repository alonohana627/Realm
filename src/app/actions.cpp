#include "actions.hpp"

#include "items.hpp"

#include <array>
#include <string>

namespace realm::app {

namespace {

// helpers

using gamelib::platform::ActionId;
using gamelib::platform::GamepadButton;
using gamelib::platform::Key;
using game::SkillId;

struct SkillKey {
    SkillId skill;
    Key key;
    GamepadButton button;
};

constexpr std::array<SkillKey, 6> kWarriorKeys{{{SkillId::PowerStrike, Key::A, GamepadButton::Y},
                                                {SkillId::Cleave, Key::S, GamepadButton::RightBumper},
                                                {SkillId::Quake, Key::D, GamepadButton::RightTrigger},
                                                {SkillId::Rush, Key::F, GamepadButton::LeftBumper},
                                                {SkillId::IronBody, Key::G, GamepadButton::Count},
                                                {SkillId::WarCry, Key::H, GamepadButton::Count}}};
constexpr std::array<SkillKey, 6> kMageKeys{{{SkillId::ArcaneBolt, Key::A, GamepadButton::Y},
                                             {SkillId::ChainSpark, Key::S, GamepadButton::RightBumper},
                                             {SkillId::Starfall, Key::D, GamepadButton::RightTrigger},
                                             {SkillId::Bloom, Key::F, GamepadButton::LeftBumper},
                                             {SkillId::ManaShield, Key::G, GamepadButton::Count},
                                             {SkillId::Swiftwind, Key::H, GamepadButton::Count}}};

std::string skillActionName(const SkillId skill)
{
    std::string name = "skill_";
    for (const char c : game::skillInfo(skill).name) {
        name += c == ' ' ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return name;
}

std::string itemActionName(const int item)
{
    return "item_" + std::to_string(item);
}

std::string upper(const std::string_view s)
{
    std::string out;
    for (const char c : s) {
        out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

void addSkill(gamelib::platform::ActionMap& map, const SkillKey& k)
{
    gamelib::platform::ActionInfo info{.name = skillActionName(k.skill), .label = upper(game::skillInfo(k.skill).name),
                                       .category = "SKILLS", .keys = {k.key}};
    if (k.button != GamepadButton::Count) {
        info.buttons = {k.button};
    }
    if (!map.find(info.name)) {
        (void)map.add(info);
    }
}

} // namespace

gamelib::platform::ActionMap makeActions(const game::Job job)
{
    gamelib::platform::ActionMap map;
    map.add({.name = "left", .label = "LEFT", .category = "MOVE", .keys = {Key::Left},
             .buttons = {GamepadButton::DpadLeft, GamepadButton::LeftStickLeft}});
    map.add({.name = "right", .label = "RIGHT", .category = "MOVE", .keys = {Key::Right},
             .buttons = {GamepadButton::DpadRight, GamepadButton::LeftStickRight}});
    map.add({.name = "up", .label = "UP / TALK", .category = "MOVE", .keys = {Key::Up},
             .buttons = {GamepadButton::DpadUp, GamepadButton::LeftStickUp}});
    map.add({.name = "down", .label = "DOWN / PRONE", .category = "MOVE", .keys = {Key::Down},
             .buttons = {GamepadButton::DpadDown, GamepadButton::LeftStickDown}});
    map.add({.name = "jump", .label = "JUMP", .category = "ACTION", .keys = {Key::Space, Key::LeftAlt}, .buttons = {GamepadButton::A}});
    map.add({.name = "attack", .label = "ATTACK", .category = "ACTION", .keys = {Key::LeftControl, Key::X}, .buttons = {GamepadButton::X}});
    map.add({.name = "pickup", .label = "PICK UP", .category = "ACTION", .keys = {Key::Z}, .buttons = {GamepadButton::B}});
    map.add({.name = "stats", .label = "STATS", .category = "MENUS", .keys = {Key::C}});
    map.add({.name = "skills", .label = "SKILLS", .category = "MENUS", .keys = {Key::K}});
    map.add({.name = "inventory", .label = "ITEMS", .category = "MENUS", .keys = {Key::I}, .buttons = {GamepadButton::Back}});
    map.add({.name = "equipment", .label = "EQUIPMENT", .category = "MENUS", .keys = {Key::E}});
    map.add({.name = "quests", .label = "QUESTS", .category = "MENUS", .keys = {Key::Q}});
    map.add({.name = "worldmap", .label = "WORLD MAP", .category = "MENUS", .keys = {Key::W}});
    map.add({.name = "minimap", .label = "MINIMAP", .category = "MENUS", .keys = {Key::M}});
    map.add({.name = "keys", .label = "KEY BINDINGS", .category = "MENUS", .keys = {Key::B}});
    map.add({.name = "pause", .label = "MENU", .category = "SYSTEM", .keys = {Key::Escape}, .buttons = {GamepadButton::Start},
             .locked = true});
    addSkill(map, {SkillId::PebbleToss, Key::LeftShift, GamepadButton::Count});
    const std::array<std::pair<int, Key>, 7> items{{{game::kRedPotion, Key::Num1},
                                                    {game::kOrangePotion, Key::Num2},
                                                    {game::kWhitePotion, Key::Num3},
                                                    {game::kBluePotion, Key::Num4},
                                                    {game::kVioletPotion, Key::Num5},
                                                    {game::kHoneyElixir, Key::Num6},
                                                    {game::kReturnScroll, Key::Num0}}};
    for (const auto& [item, key] : items) {
        gamelib::platform::ActionInfo info{.name = itemActionName(item), .label = upper(game::itemName(item)), .category = "ITEMS",
                                           .keys = {key}};
        if (item == game::kRedPotion) {
            info.buttons = {GamepadButton::LeftTrigger};
        }
        (void)map.add(info);
    }
    addJobActions(map, job);
    return map;
}

void addJobActions(gamelib::platform::ActionMap& actions, const game::Job job)
{
    if (job == game::Job::Warrior) {
        for (const SkillKey& k : kWarriorKeys) {
            addSkill(actions, k);
        }
    } else if (job == game::Job::Mage) {
        for (const SkillKey& k : kMageKeys) {
            addSkill(actions, k);
        }
    }
}

std::optional<SkillId> actionSkill(const gamelib::platform::ActionMap& actions, const ActionId id)
{
    if (static_cast<std::size_t>(id) >= actions.size()) {
        return std::nullopt;
    }
    const std::string& name = actions.info(id).name;
    for (int i = 0; i < game::kSkillCount; ++i) {
        if (name == skillActionName(static_cast<SkillId>(i))) {
            return static_cast<SkillId>(i);
        }
    }
    return std::nullopt;
}

int actionItem(const gamelib::platform::ActionMap& actions, const ActionId id)
{
    if (static_cast<std::size_t>(id) >= actions.size()) {
        return 0;
    }
    const std::string& name = actions.info(id).name;
    if (!name.starts_with("item_")) {
        return 0;
    }
    return std::stoi(name.substr(5));
}

std::optional<ActionId> skillAction(const gamelib::platform::ActionMap& actions, const SkillId skill)
{
    return actions.find(skillActionName(skill));
}

std::optional<ActionId> itemAction(const gamelib::platform::ActionMap& actions, const int item)
{
    return actions.find(itemActionName(item));
}

game::Input readInput(const gamelib::platform::ActionMap& actions, const gamelib::platform::InputState& input)
{
    game::Input in;
    const auto down = [&](const Action a) { return actions.isDown(input, actionId(a)); };
    const auto pressed = [&](const Action a) { return actions.wasPressed(input, actionId(a)); };
    in.move = (down(Action::Right) ? 1 : 0) - (down(Action::Left) ? 1 : 0);
    in.climb = (down(Action::Down) ? 1 : 0) - (down(Action::Up) ? 1 : 0);
    in.jumpPressed = pressed(Action::Jump);
    in.jumpHeld = in.jumpPressed || down(Action::Jump);
    in.attackHeld = pressed(Action::Attack) || down(Action::Attack);
    in.pickup = pressed(Action::Pickup) || down(Action::Pickup);
    in.upPressed = pressed(Action::Up);
    for (std::size_t i = static_cast<std::size_t>(Action::Count); i < actions.size(); ++i) {
        const auto id = static_cast<ActionId>(i);
        if (!actions.wasPressed(input, id)) {
            continue;
        }
        if (const std::optional<SkillId> skill = actionSkill(actions, id)) {
            in.skills |= 1u << static_cast<unsigned>(*skill);
        } else if (const int item = actionItem(actions, id); item != 0) {
            in.useItem = item;
        }
    }
    return in;
}

const gamelib::ui::MenuBindings& menuBindings()
{
    static const gamelib::ui::MenuBindings bindings{
        .up = {actionId(Action::Up)},
        .down = {actionId(Action::Down)},
        .left = {actionId(Action::Left)},
        .right = {actionId(Action::Right)},
        .confirm = {actionId(Action::Jump), actionId(Action::Attack)},
        .back = {actionId(Action::Pause)},
    };
    return bindings;
}

} // namespace realm::app
