#include "windows.hpp"

#include "actions.hpp"
#include "look.hpp"
#include "ui_kit.hpp"
#include "view.hpp"

#include <gamelib/core/easing.hpp>
#include <gamelib/draw/font.hpp>
#include <gamelib/ui/focus.hpp>
#include <gamelib/ui/key_palette.hpp>
#include <gamelib/ui/keyboard.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace realm::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::draw::Align;
using gamelib::draw::Font;
using gamelib::ui::MenuInput;
using gamelib::platform::ActionId;
using gamelib::platform::Key;

constexpr float kW = 1920.0f;
constexpr int kBagColumns = 6;
// The keybinding window: between key caps, between palette cells.
constexpr float kKeyGap = 6.0f;
constexpr Vec2 kEntryGap{20.0f, 14.0f};

Rect lifted(const Rect& r, const float amount)
{
    const float a = std::clamp(amount, 0.0f, 1.2f);
    return {r.x, r.y + (1.0f - std::min(a, 1.0f)) * 40.0f - (a > 1.0f ? (a - 1.0f) * 30.0f : 0.0f), r.width, r.height};
}

bool hotkeyCandidate(const Key key)
{
    // Keys a hotkey may be placed on from the skill window or the bag: not
    // the arrows, Escape, Enter or Tab (the windows' own keys).
    return key != Key::Unknown && key != Key::Up && key != Key::Down && key != Key::Left && key != Key::Right && key != Key::Escape
           && key != Key::Enter && key != Key::Tab && key != Key::Backspace && key != Key::Delete;
}

std::string lower(std::string_view s)
{
    std::string out;
    for (const char c : s) {
        out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

// `text` centered in `box` on one or two lines, the font shrinking from
// maxSize until it fits; a word is never cut (below minSize the last
// resort is an ellipsis).
void fitLabel(gamelib::draw::DrawList& list, const std::string& text, const Rect box, const float maxSize, const float minSize,
              const Color color)
{
    for (float size = maxSize; size >= minSize; size -= 1.0f) {
        const Font f = kit::font(size, 1.35f);
        const float line = f.height() * 1.15f;
        if (f.width(text) <= box.width && f.height() <= box.height) {
            gamelib::draw::drawText(list, f, text, box.center().x, f.topFor(box.center().y), color, Align::Center);
            return;
        }
        // The two-line split with the narrowest wider line.
        std::size_t best = std::string::npos;
        float bestWidth = 1e9f;
        for (std::size_t i = text.find(' '); i != std::string::npos; i = text.find(' ', i + 1)) {
            const float w = std::max(f.width(std::string_view{text}.substr(0, i)), f.width(std::string_view{text}.substr(i + 1)));
            if (w < bestWidth) {
                bestWidth = w;
                best = i;
            }
        }
        if (best != std::string::npos && bestWidth <= box.width && line * 2.0f <= box.height + 2.0f) {
            const float top = box.center().y - line;
            gamelib::draw::drawText(list, f, std::string_view{text}.substr(0, best), box.center().x, top, color, Align::Center);
            gamelib::draw::drawText(list, f, std::string_view{text}.substr(best + 1), box.center().x, top + line, color, Align::Center);
            return;
        }
    }
    const Font f = kit::font(minSize, 1.35f);
    std::string cut = text;
    while (cut.size() > 1 && f.width(cut + "..") > box.width) {
        cut.pop_back();
    }
    gamelib::draw::drawText(list, f, cut + "..", box.center().x, f.topFor(box.center().y), color, Align::Center);
}

// World map pins: a position on the painted map (-1..1 over its rect).
Vec2 mapPin(const game::MapId id)
{
    using M = game::MapId;
    switch (id) {
    case M::Petalbrook: return {-0.62f, 0.3f};
    case M::SproutMeadow: return {-0.8f, 0.38f};
    case M::PuddlePath: return {-0.56f, 0.12f};
    case M::BreezyBluff: return {-0.46f, 0.3f};
    case M::PetalPier: return {-0.42f, 0.5f};
    case M::SaltwhistleDocks: return {-0.06f, 0.54f};
    case M::SaltwhistleMarket: return {0.08f, 0.46f};
    case M::GullCliffs: return {-0.2f, 0.4f};
    case M::TidepoolShore: return {-0.3f, 0.55f};
    case M::LighthouseHill: return {-0.3f, 0.7f};
    case M::FernwoodTrail: return {0.26f, 0.36f};
    case M::Mossbough: return {0.44f, 0.24f};
    case M::CanopyWalk: return {0.6f, 0.3f};
    case M::GlowcapHollow: return {0.72f, 0.16f};
    case M::ThornbarkGrove: return {0.84f, 0.04f};
    case M::WindcrestFoothills: return {0.42f, 0.02f};
    case M::ScoutCamp: return {0.38f, -0.2f};
    case M::CliffsideClimb: return {0.48f, -0.4f};
    case M::FrostbiteLedge: return {0.62f, -0.54f};
    case M::SummitShrine: return {0.76f, -0.66f};
    case M::UndercroftStairs: return {0.1f, 0.66f};
    case M::GlimmerTunnels: return {0.2f, 0.7f};
    case M::SunkenArchive: return {0.3f, 0.66f};
    case M::DeepHollow: return {0.38f, 0.72f};
    case M::HeartOfGloom: return {0.46f, 0.66f};
    case M::Count: break;
    }
    return {};
}

const char* statHelp(const game::Core c)
{
    switch (c) {
    case game::Core::Str: return "Strength makes every swing hit harder and adds a little HP.";
    case game::Core::Agi: return "Agility raises your critical chance, Speed and Jump.";
    case game::Core::Int: return "Intellect powers your magic and deepens your MP.";
    case game::Core::Vit: return "Vitality adds HP and defense.";
    case game::Core::Count: break;
    }
    return "";
}

} // namespace

Windows::Windows()
{
}

bool Windows::wantsAllKeys() const
{
    return m_switch.isOpen(WindowId::Keys);
}

Rect Windows::windowRect(const WindowId id) const
{
    switch (id) {
    case WindowId::Stats: return {440.0f, 150.0f, 1040.0f, 680.0f};
    case WindowId::Skills: return {480.0f, 110.0f, 960.0f, 760.0f};
    case WindowId::Inventory: return {1140.0f, 170.0f, 660.0f, 640.0f};
    case WindowId::Equipment: return {140.0f, 120.0f, 600.0f, 720.0f};
    case WindowId::Quests: return {400.0f, 130.0f, 1120.0f, 720.0f};
    case WindowId::WorldMap: return {170.0f, 70.0f, 1580.0f, 860.0f};
    case WindowId::Keys: return {100.0f, 50.0f, 1720.0f, 900.0f};
    case WindowId::Shop: return {300.0f, 120.0f, 1320.0f, 760.0f};
    }
    return {};
}

void Windows::open(const WindowId id, const game::Session& session, const gamelib::platform::ActionMap& actions)
{
    m_switch.open(id);
    m_age = 0.0f;
    m_message.clear();
    if (id == WindowId::Skills) {
        m_skillTabs.select(session.character().job == game::Job::Beginner ? 0 : 1);
        m_skillList.setCount(static_cast<int>(skillRows(session).size()));
        m_skillList.select(0);
    } else if (id == WindowId::Quests) {
        m_questList.setCount(static_cast<int>(questRows(session).size()));
        m_questList.select(0);
    } else if (id == WindowId::Shop) {
        m_shopTabs.select(0);
        m_shopList.setCount(static_cast<int>(session.shopEntries().size()));
        m_shopList.select(0);
        m_quantity = 1;
    } else if (id == WindowId::Keys) {
        m_keys.reset();
        refreshPalette(session, actions);
    } else if (id == WindowId::WorldMap) {
        m_mapCursor = static_cast<int>(session.mapId());
    }
}

void Windows::update(const float seconds)
{
    m_switch.update(seconds);
    m_flash.update(seconds);
    m_age += seconds;
    m_messageAge += seconds;
}

std::vector<game::SkillId> Windows::skillRows(const game::Session& session) const
{
    const game::Job job = m_skillTabs.current() == 0 ? game::Job::Beginner : session.character().job;
    if (m_skillTabs.current() == 1 && job == game::Job::Beginner) {
        return {};
    }
    const auto skills = game::jobSkills(job);
    return {skills.begin(), skills.end()};
}

std::vector<int> Windows::questRows(const game::Session& session) const
{
    std::vector<int> rows;
    for (const int q : session.quests().log()) {
        const auto s = session.quests().stateOf(q);
        const bool done = s == gamelib::core::QuestState::TurnedIn;
        if ((m_questTabs.current() == 1) == done) {
            rows.push_back(q);
        }
    }
    if (m_questTabs.current() == 0) {
        std::ranges::reverse(rows); // the newest first
    }
    return rows;
}

// ---------------------------------------------------------------------------
// Input

WindowFeedback Windows::handle(const MenuInput input, game::Session& session, gamelib::platform::ActionMap& actions)
{
    const std::optional<WindowId> id = m_switch.current();
    if (!id) {
        return WindowFeedback::None;
    }
    if (input == MenuInput::Back && *id != WindowId::Keys) {
        m_switch.close();
        return WindowFeedback::Closed;
    }
    game::Character const& c = session.character();
    switch (*id) {
    case WindowId::Stats: {
        const gamelib::ui::StatResult r = m_stats.handle(input);
        if (r.action == gamelib::ui::StatAction::Spend) {
            if (!session.spendPoint(static_cast<game::Core>(r.stat))) {
                return WindowFeedback::Failed;
            }
            m_flash.add({.row = static_cast<int>(r.stat)}, 0.4f);
            return WindowFeedback::Done;
        }
        if (r.action == gamelib::ui::StatAction::Auto) {
            return session.autoAssign() > 0 ? WindowFeedback::Big : WindowFeedback::Failed;
        }
        return r.action == gamelib::ui::StatAction::Moved ? WindowFeedback::Moved : WindowFeedback::None;
    }
    case WindowId::Skills: {
        if (input == MenuInput::Left || input == MenuInput::Right) {
            if (m_skillTabs.handle(input)) {
                m_skillList.setCount(static_cast<int>(skillRows(session).size()));
                m_skillList.select(0);
                return WindowFeedback::Tab;
            }
            return WindowFeedback::None;
        }
        const gamelib::ui::ListEvent e = m_skillList.handle(input);
        if (e == gamelib::ui::ListEvent::Confirmed) {
            const std::vector<game::SkillId> rows = skillRows(session);
            if (m_skillList.cursor() < static_cast<int>(rows.size())) {
                const game::SkillId s = rows[static_cast<std::size_t>(m_skillList.cursor())];
                if (session.learnSkill(s) == gamelib::core::LearnResult::Learned) {
                    m_flash.add({.row = m_skillList.cursor()}, 0.5f);
                    return WindowFeedback::Big;
                }
                return WindowFeedback::Failed;
            }
        }
        return e == gamelib::ui::ListEvent::Moved ? WindowFeedback::Moved : WindowFeedback::None;
    }
    case WindowId::Inventory: {
        const auto tab = static_cast<game::Tab>(m_bagTabs.current());
        if ((input == MenuInput::Left && m_bagGrid.cursor() % kBagColumns == 0) || (input == MenuInput::Right && m_bagGrid.cursor() % kBagColumns == kBagColumns - 1)) {
            // At the grid's side, left and right switch tabs.
            if (m_bagTabs.handle(input)) {
                m_bagGrid.select(input == MenuInput::Left ? m_bagGrid.cursor() + kBagColumns - 1 : m_bagGrid.cursor() - kBagColumns + 1);
                return WindowFeedback::Tab;
            }
        }
        const gamelib::ui::SlotEvent e = m_bagGrid.handle(input);
        if (e == gamelib::ui::SlotEvent::Confirmed) {
            const bool equip = tab == game::Tab::Equip;
            if (session.useAt(tab, m_bagGrid.cursor())) {
                return equip ? WindowFeedback::Equip : WindowFeedback::Done;
            }
            return WindowFeedback::Failed;
        }
        return e == gamelib::ui::SlotEvent::Moved ? WindowFeedback::Moved : WindowFeedback::None;
    }
    case WindowId::Equipment: {
        const gamelib::ui::SlotEvent e = m_gearGrid.handle(input);
        if (e == gamelib::ui::SlotEvent::Confirmed) {
            return session.unequip(static_cast<game::Slot>(m_gearGrid.cursor())) ? WindowFeedback::Equip : WindowFeedback::Failed;
        }
        return e == gamelib::ui::SlotEvent::Moved ? WindowFeedback::Moved : WindowFeedback::None;
    }
    case WindowId::Quests: {
        if (input == MenuInput::Left || input == MenuInput::Right) {
            if (m_questTabs.handle(input)) {
                m_questList.setCount(static_cast<int>(questRows(session).size()));
                m_questList.select(0);
                return WindowFeedback::Tab;
            }
            return WindowFeedback::None;
        }
        return m_questList.handle(input) == gamelib::ui::ListEvent::Moved ? WindowFeedback::Moved : WindowFeedback::None;
    }
    case WindowId::WorldMap: {
        // Move between the pins of the maps already seen.
        std::vector<Rect> pins;
        std::vector<int> ids;
        for (int i = 0; i < game::kMapCount; ++i) {
            if (session.visited(static_cast<game::MapId>(i))) {
                const Vec2 at = mapPin(static_cast<game::MapId>(i));
                pins.push_back({at.x * 100.0f, at.y * 100.0f, 1.0f, 1.0f});
                ids.push_back(i);
            }
        }
        const auto here = std::ranges::find(ids, m_mapCursor);
        const int current = here == ids.end() ? 0 : static_cast<int>(here - ids.begin());
        int dx = input == MenuInput::Left ? -1 : input == MenuInput::Right ? 1 : 0;
        int dy = input == MenuInput::Up ? -1 : input == MenuInput::Down ? 1 : 0;
        if ((dx != 0 || dy != 0) && !pins.empty()) {
            const int next = gamelib::ui::stepFocus(pins, current, dx, dy, false);
            if (next != current) {
                m_mapCursor = ids[static_cast<std::size_t>(next)];
                return WindowFeedback::Moved;
            }
        }
        return WindowFeedback::None;
    }
    case WindowId::Shop: {
        if (input == MenuInput::Left || input == MenuInput::Right) {
            const std::vector<gamelib::core::ShopEntry> entries = session.shopEntries();
            const bool buying = m_shopTabs.current() == 0;
            const int entry = m_shopList.cursor();
            const bool stackable = buying && entry < static_cast<int>(entries.size())
                                   && game::itemRegistry().at(entries[static_cast<std::size_t>(entry)].item).stackLimit > 1;
            if (stackable && input == MenuInput::Right && m_quantity < 100) {
                m_quantity = m_quantity == 1 ? 10 : std::min(m_quantity + 10, 100);
                return WindowFeedback::Moved;
            }
            if (stackable && input == MenuInput::Left && m_quantity > 1) {
                m_quantity = m_quantity <= 10 ? 1 : m_quantity - 10;
                return WindowFeedback::Moved;
            }
            if (m_shopTabs.handle(input)) {
                m_shopList.setCount(m_shopTabs.current() == 0 ? static_cast<int>(entries.size()) : game::kBagSlots * game::kTabCount);
                m_shopList.select(0);
                m_quantity = 1;
                return WindowFeedback::Tab;
            }
            return WindowFeedback::None;
        }
        if (m_shopTabs.current() == 0) {
            const gamelib::ui::ListEvent e = m_shopList.handle(input);
            if (e == gamelib::ui::ListEvent::Confirmed) {
                const auto r = session.buy(m_shopList.cursor(), m_quantity);
                return r == gamelib::core::TradeResult::Done ? WindowFeedback::Done : WindowFeedback::Failed;
            }
            if (e == gamelib::ui::ListEvent::Moved) {
                m_quantity = 1;
                return WindowFeedback::Moved;
            }
            return WindowFeedback::None;
        }
        // Selling: the bag's items in one list (equip, use, etc.).
        std::vector<std::pair<game::Tab, int>> sellable;
        for (int t = 0; t < game::kTabCount; ++t) {
            for (int s = 0; s < game::kBagSlots; ++s) {
                if (!c.bags[static_cast<std::size_t>(t)].at(s).empty()) {
                    sellable.emplace_back(static_cast<game::Tab>(t), s);
                }
            }
        }
        m_shopList.setCount(static_cast<int>(sellable.size()));
        const gamelib::ui::ListEvent e = m_shopList.handle(input);
        if (e == gamelib::ui::ListEvent::Confirmed && m_shopList.cursor() < static_cast<int>(sellable.size())) {
            const auto [tab, slot] = sellable[static_cast<std::size_t>(m_shopList.cursor())];
            return session.sell(tab, slot) ? WindowFeedback::Done : WindowFeedback::Failed;
        }
        return e == gamelib::ui::ListEvent::Moved ? WindowFeedback::Moved : WindowFeedback::None;
    }
    case WindowId::Keys:
        refreshPalette(session, actions);
        return keysResult(m_keys.handle(input, actions), actions);
    }
    return WindowFeedback::None;
}

WindowFeedback Windows::handleKey(const Key key, game::Session& session, gamelib::platform::ActionMap& actions)
{
    const std::optional<WindowId> id = m_switch.current();
    if (!id) {
        return WindowFeedback::None;
    }
    if (*id == WindowId::Keys) {
        // Every key goes to the palette: arrows, Enter, Escape, Tab and Delete
        // drive it, any other key takes what is in hand.
        refreshPalette(session, actions);
        return keysResult(m_keys.handleKey(key, actions), actions);
    }
    if (key == Key::Tab) {
        if (*id == WindowId::Inventory) {
            (void)m_bagTabs.next();
            return WindowFeedback::Tab;
        }
        if (*id == WindowId::Skills) {
            (void)m_skillTabs.next();
            m_skillList.setCount(static_cast<int>(skillRows(session).size()));
            m_skillList.select(0);
            return WindowFeedback::Tab;
        }
        return WindowFeedback::None;
    }
    if (*id == WindowId::Inventory && (key == Key::Delete || key == Key::Backspace)) {
        return session.dropFromBag(static_cast<game::Tab>(m_bagTabs.current()), m_bagGrid.cursor()) ? WindowFeedback::Done
                                                                                                   : WindowFeedback::Failed;
    }
    if (!hotkeyCandidate(key)) {
        return WindowFeedback::None;
    }
    // A key pressed in the skill window or on a potion places it there.
    if (*id == WindowId::Skills) {
        const std::vector<game::SkillId> rows = skillRows(session);
        if (m_skillList.cursor() < static_cast<int>(rows.size())) {
            const game::SkillId s = rows[static_cast<std::size_t>(m_skillList.cursor())];
            const std::optional<ActionId> a = skillAction(actions, s);
            if (a && session.character().skillLevel(s) > 0) {
                const ActionId on = actions.actionOn(key);
                if (on != gamelib::platform::kNoAction && actions.isLocked(on)) {
                    return WindowFeedback::Failed;
                }
                (void)actions.bind(key, *a, gamelib::platform::ActionMap::Conflict::Replace);
                return WindowFeedback::Bound;
            }
        }
        return WindowFeedback::None;
    }
    if (*id == WindowId::Inventory && m_bagTabs.current() == static_cast<int>(game::Tab::Use)) {
        const gamelib::core::ItemStack& s = session.character().bag(game::Tab::Use).at(m_bagGrid.cursor());
        if (const std::optional<ActionId> a = s.empty() ? std::nullopt : itemAction(actions, s.item)) {
            const ActionId on = actions.actionOn(key);
            if (on != gamelib::platform::kNoAction && actions.isLocked(on)) {
                return WindowFeedback::Failed;
            }
            (void)actions.bind(key, *a, gamelib::platform::ActionMap::Conflict::Replace);
            return WindowFeedback::Bound;
        }
    }
    return WindowFeedback::None;
}

WindowFeedback Windows::handleMouse(const gamelib::platform::MouseEvent& event, game::Session& session,
                                    gamelib::platform::ActionMap& actions)
{
    using gamelib::platform::MouseButton;
    using Kind = gamelib::platform::MouseEvent::Kind;
    const std::optional<WindowId> id = m_switch.current();
    if (!id) {
        return WindowFeedback::None;
    }
    if (*id != WindowId::Keys) {
        // Clicks select and use slots in the bag and the gear.
        if (event.kind == Kind::Button && event.pressed && event.button == MouseButton::Left) {
            if (*id == WindowId::Inventory) {
                if (const std::optional<int> slot = m_bagGrid.slotAt(event.position)) {
                    if (*slot == m_bagGrid.cursor()) {
                        return handle(MenuInput::Confirm, session, actions);
                    }
                    m_bagGrid.select(*slot);
                    return WindowFeedback::Moved;
                }
            } else if (*id == WindowId::Equipment) {
                if (const std::optional<int> slot = m_gearGrid.slotAt(event.position)) {
                    if (*slot == m_gearGrid.cursor()) {
                        return handle(MenuInput::Confirm, session, actions);
                    }
                    m_gearGrid.select(*slot);
                    return WindowFeedback::Moved;
                }
            }
        }
        return WindowFeedback::None;
    }
    // The keybinding window: GameLib's KeyPalette (drag and drop, the wheel).
    refreshPalette(session, actions);
    const gamelib::ui::KeyPalette::Layout layout = m_keys.layout(keyboardRect(), paletteRect(), kKeyGap, kEntryGap);
    const gamelib::ui::KeyPaletteResult r = m_keys.handleMouse(event, layout, actions);
    return keysResult(r, actions);
}

// ---------------------------------------------------------------------------
// The keybinding window's layout

Rect Windows::keyboardRect() const
{
    const Rect w = windowRect(WindowId::Keys);
    constexpr float kUnit = 76.0f;
    const float width = gamelib::ui::kKeyboardUnitsWide * kUnit;
    return {w.center().x - width * 0.5f, w.y + 90.0f, width, gamelib::ui::kKeyboardUnitsHigh * kUnit};
}

Rect Windows::paletteRect() const
{
    const Rect w = windowRect(WindowId::Keys);
    const Rect k = keyboardRect();
    return {w.x + 40.0f, k.bottom() + 70.0f, w.width - 80.0f, w.bottom() - k.bottom() - 120.0f};
}

std::vector<Windows::PaletteEntry> Windows::palette(const game::Session& session, const gamelib::platform::ActionMap& actions) const
{
    std::vector<PaletteEntry> entries;
    const game::Character& c = session.character();
    // Skills the character knows, then items, then the actions and menus.
    for (std::size_t i = 0; i < actions.size(); ++i) {
        const auto id = static_cast<ActionId>(i);
        if (const std::optional<game::SkillId> s = actionSkill(actions, id); s && c.skillLevel(*s) > 0) {
            entries.push_back({.action = id, .label = std::string{game::skillInfo(*s).name}, .skill = s});
        }
    }
    for (std::size_t i = 0; i < actions.size(); ++i) {
        const auto id = static_cast<ActionId>(i);
        if (const int item = actionItem(actions, id); item != 0) {
            entries.push_back({.action = id, .label = std::string{game::itemName(item)}, .item = item, .usable = c.countOf(item) > 0});
        }
    }
    for (std::size_t i = 0; i < static_cast<std::size_t>(Action::Count); ++i) {
        const auto id = static_cast<ActionId>(i);
        if (actions.isLocked(id)) {
            continue;
        }
        entries.push_back({.action = id, .label = lower(actions.info(id).label)});
    }
    return entries;
}

void Windows::refreshPalette(const game::Session& session, const gamelib::platform::ActionMap& actions)
{
    std::vector<gamelib::ui::KeyPaletteEntry> list;
    for (const PaletteEntry& e : palette(session, actions)) {
        list.push_back({.action = e.action, .label = e.label});
    }
    m_keys.setEntries(std::move(list));
}

WindowFeedback Windows::keysResult(const gamelib::ui::KeyPaletteResult& r, const gamelib::platform::ActionMap& actions)
{
    using gamelib::ui::KeyPaletteEvent;
    const auto name = [&](const ActionId a) { return lower(actions.info(a).label); };
    switch (r.event) {
    case KeyPaletteEvent::Cursor:
    case KeyPaletteEvent::Dropped: return WindowFeedback::Moved;
    case KeyPaletteEvent::Picked:
        m_message = std::format("{} in hand - press its new key, or choose a key and press enter", name(r.action));
        m_messageAge = 0.0f;
        return WindowFeedback::Done;
    case KeyPaletteEvent::Bound:
    case KeyPaletteEvent::Moved:
        m_message = std::format("{} is on {}", name(r.action), lower(gamelib::platform::keyLabel(r.key)));
        if (r.displacedTo != Key::Unknown && r.displaced != gamelib::platform::kNoAction) {
            m_message += std::format(" - {} moved to {}", name(r.displaced), lower(gamelib::platform::keyLabel(r.displacedTo)));
        }
        if (r.displacedUnbound) {
            m_message += std::format(" - {} has no key now", name(r.displaced));
        }
        m_messageAge = 0.0f;
        return WindowFeedback::Bound;
    case KeyPaletteEvent::Unbound: return WindowFeedback::Unbound;
    case KeyPaletteEvent::Hover: return WindowFeedback::None;
    case KeyPaletteEvent::Refused:
        m_message = "that key is locked";
        m_messageAge = 0.0f;
        return WindowFeedback::Failed;
    case KeyPaletteEvent::Back:
        m_switch.close();
        return WindowFeedback::Closed;
    case KeyPaletteEvent::None: break;
    }
    return WindowFeedback::None;
}

// ---------------------------------------------------------------------------
// Drawing

void Windows::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                   const gamelib::platform::ActionMap& actions, const float time) const
{
    const std::optional<WindowId> id = m_switch.shown();
    if (!id) {
        return;
    }
    const float veil = m_switch.veil();
    list.box({0.0f, 0.0f, kW, 1080.0f}, srgb(0x1A1430, 0.35f * veil));
    const Rect rect = lifted(windowRect(*id), m_switch.amount());
    switch (*id) {
    case WindowId::Stats: drawStats(list, shapes, session, rect, time); break;
    case WindowId::Skills: drawSkills(list, shapes, session, actions, rect, time); break;
    case WindowId::Inventory: drawInventory(list, shapes, session, actions, rect, time); break;
    case WindowId::Equipment: drawEquipment(list, shapes, session, rect, time); break;
    case WindowId::Quests: drawQuests(list, shapes, session, rect); break;
    case WindowId::WorldMap: drawWorldMap(list, shapes, session, rect, time); break;
    case WindowId::Keys: drawKeys(list, shapes, session, actions, rect, time); break;
    case WindowId::Shop: drawShop(list, shapes, session, rect, time); break;
    }
}

void Windows::drawStats(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Rect rect,
                        const float time) const
{
    const game::Character& c = session.character();
    const game::Derived now = session.derived();
    std::vector<gamelib::ui::StatLine> lines;
    for (int i = 0; i < game::kCoreCount; ++i) {
        const auto stat = static_cast<game::Core>(i);
        const int base = c.points.base()[stat];
        lines.push_back({.name = std::string{game::coreName(stat)}, .value = base, .bonus = now.core[stat] - base});
    }
    game::Derived next = now;
    if (const std::optional<std::size_t> sel = m_stats.selectedStat(); sel && c.points.available() > 0) {
        next = c.derivedWith(c.points.preview(static_cast<game::Core>(*sel)));
    } else if (m_stats.onAuto() && c.points.available() > 0) {
        next = c.derivedWith(c.points.preview(c.nextAuto()));
    }
    const std::vector<gamelib::ui::DerivedLine> derived{
        {.name = "max hp", .now = now.maxHp, .next = next.maxHp},
        {.name = "max mp", .now = now.maxMp, .next = next.maxMp},
        {.name = "attack", .now = now.attack, .next = next.attack},
        {.name = "magic", .now = now.magic, .next = next.magic},
        {.name = "defense", .now = now.defense, .next = next.defense},
        {.name = "critical", .now = now.crit, .next = next.crit, .percent = true},
        {.name = "speed", .now = now.speed, .next = next.speed, .percent = true},
        {.name = "jump", .now = now.jump, .next = next.jump, .percent = true},
    };
    gamelib::ui::StatWindowStyle style;
    style.panel.painter = [&shapes](gamelib::draw::DrawList& l, const Rect& r) {
        addUi(l, shapes, Ui::Window, r.expanded(kit::kMargin), ui_color::kWindow, ui_color::kTitle, 52.0f, 0.0f, 22.0f);
    };
    style.titleHeight = 52.0f;
    style.titlePainter = [rect](gamelib::draw::DrawList& l, const Rect&, const std::string_view title) {
        // In the shader title bar along the window's top.
        const Font f = kit::titleFont(32.0f);
        gamelib::draw::drawText(l, f, title, rect.center().x, f.topFor(rect.y + 26.0f), srgb(0xFFFFFF), Align::Center);
    };
    style.titleFont = kit::titleFont(32.0f);
    style.font = kit::font(28.0f, 1.45f);
    style.smallFont = kit::font(23.0f, 1.3f);
    style.title = srgb(0xFFFFFF);
    style.text = ui_color::kInk;
    style.dim = ui_color::kSoftInk;
    style.highlight = ui_color::kInk;
    style.pointsColor = srgb(0xE8901E);
    style.plusFont = kit::font(30.0f, 2.0f);
    style.better = srgb(0x2E9E52);
    style.worse = srgb(0xD8404E);
    style.padding = 30.0f;
    style.rowHeight = 62.0f;
    style.lineHeight = 36.0f;
    style.portraitWidth = 300.0f;
    style.iconSize = 40.0f;
    style.pointsScale = c.points.available() > 0 ? 1.0f + 0.06f * std::sin(time * 5.0f) : 1.0f;
    style.icon = [&shapes](gamelib::draw::DrawList& l, const Rect& cell, const std::size_t stat, bool) {
        addIcon(l, shapes, Ui::StatIcon, cell.center(), cell.height, static_cast<float>(stat), 0.0f, srgb(0xFFFFFF), srgb(0xFFFFFF));
    };
    style.rowPainter = [&shapes, this](gamelib::draw::DrawList& l, const Rect& row, const std::size_t index, const bool selected) {
        float flash = 0.0f;
        for (const auto& f : m_flash.items()) {
            if (static_cast<std::size_t>(f.value.row) == index) {
                flash = 1.0f - f.progress();
            }
        }
        if (selected || flash > 0.0f) {
            addUi(l, shapes, Ui::Button, row.expanded(kit::kMargin - 2.0f), gamelib::core::lerp(srgb(0xFFE7B8), srgb(0xFFFFFF), flash),
                  ui_color::kGold, 1.0f);
        }
    };
    style.plusPainter = [&shapes](gamelib::draw::DrawList& l, const Rect& button, const bool enabled, const bool selected) {
        addUi(l, shapes, Ui::Button, button.expanded(kit::kMargin), enabled ? srgb(0xFFF0D0) : srgb(0xE8E0D0), ui_color::kGold,
              enabled ? (selected ? 1.0f : 0.0f) : 3.0f);
    };
    gamelib::ui::StatWindowLabels labels{.points = "ability points", .autoAssign = "auto-assign"};
    const float below = gamelib::ui::drawStatWindow(list, m_stats, rect, "character", c.points.available(), lines, derived, style, labels);
    // A hint for the selected stat.
    std::string help;
    if (const std::optional<std::size_t> sel = m_stats.selectedStat()) {
        help = statHelp(static_cast<game::Core>(*sel));
    } else {
        help = "Spends every point the way your job likes best.";
    }
    kit::text(list, help, rect.x + 30.0f + 300.0f + 24.0f, below + 6.0f, 21.0f, ui_color::kSoftInk);
    // The portrait column: the hero, name, job, level, experience.
    const Rect portrait = gamelib::ui::statWindowPortraitRect(rect, style);
    addUi(list, shapes, Ui::Well, Rect{portrait.x, portrait.y, portrait.width, portrait.width}.expanded(kit::kMargin), srgb(0xF6EDDA),
          ui_color::kTitle, 0.0f, 0.0f, 16.0f);
    anime::Avatar a = heroAvatar(c);
    a.feet = {portrait.center().x, portrait.y + portrait.width - 26.0f};
    a.height = portrait.width * 1.05f;
    a.expression = anime::Expression::Neutral;
    a.nextExpression = anime::Expression::Neutral;
    a.seed = 200;
    list.add(anime::avatarSprite(a, shapes.avatar));
    float y = portrait.y + portrait.width + 18.0f;
    kit::text(list, c.look.name, portrait.center().x, y, 30.0f, ui_color::kInk, Align::Center, 1.7f);
    y += 40.0f;
    kit::text(list, std::format("level {} {}", c.level, lower(game::jobName(c.job))), portrait.center().x, y, 23.0f, ui_color::kSoftInk,
              Align::Center);
    y += 38.0f;
    kit::gauge(list, shapes, {portrait.x + 10.0f, y, portrait.width - 20.0f, 18.0f}, c.xpProgress(), c.xpProgress(), ui_color::kXp,
               srgb(0xFFF2B0));
    y += 28.0f;
    kit::text(list, c.level >= game::kMaxLevel ? std::string{"max level"} : std::format("{} exp to go", kit::groupDigits(c.xpToNext())),
              portrait.center().x, y, 19.0f, ui_color::kSoftInk, Align::Center);
}

void Windows::drawSkills(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                         const gamelib::platform::ActionMap& actions, const Rect rect, const float time) const
{
    (void)time;
    const game::Character& c = session.character();
    const Rect inner = kit::window(list, shapes, rect, "skills");
    // Tabs and the points.
    const std::array<std::string, 2> tabNames{"beginner", c.job == game::Job::Beginner ? std::string{"job (level 10)"} : lower(game::jobName(c.job))};
    for (int i = 0; i < 2; ++i) {
        kit::tab(list, shapes, {inner.x + static_cast<float>(i) * 230.0f, inner.y, 220.0f, 46.0f}, tabNames[static_cast<std::size_t>(i)],
                 m_skillTabs.current() == i);
    }
    kit::label(list, "skill points", inner.right() - 70.0f, inner.y + 23.0f, 24.0f, ui_color::kSoftInk, Align::Right);
    kit::label(list, std::to_string(c.skills.points()), inner.right() - 10.0f, inner.y + 23.0f, 32.0f,
               c.skills.points() > 0 ? srgb(0xE8901E) : ui_color::kSoftInk, Align::Right, 1.8f);
    const std::vector<game::SkillId> rows = skillRows(session);
    const Rect area{inner.x, inner.y + 62.0f, inner.width, 8.0f * 60.0f};
    kit::well(list, shapes, area);
    if (rows.empty()) {
        kit::label(list, "become a warrior or a mage at level 10 to learn job skills", area.center().x, area.center().y, 24.0f,
                   ui_color::kSoftInk, Align::Center);
        return;
    }
    for (int i = m_skillList.first(); i <= m_skillList.last(); ++i) {
        const game::SkillId s = rows[static_cast<std::size_t>(i)];
        const game::SkillInfo& info = game::skillInfo(s);
        const Rect row = m_skillList.rowRect(area, i, 60.0f).expanded(-4.0f);
        const bool selected = i == m_skillList.cursor();
        float flash = 0.0f;
        for (const auto& f : m_flash.items()) {
            if (f.value.row == i) {
                flash = 1.0f - f.progress();
            }
        }
        if (selected || flash > 0.0f) {
            addUi(list, shapes, Ui::Button, row.expanded(kit::kMargin - 2.0f), gamelib::core::lerp(srgb(0xFFE7B8), srgb(0xFFFFFF), flash),
                  ui_color::kGold, 1.0f);
        }
        const int level = c.skillLevel(s);
        const bool known = level > 0;
        kit::skillIconAt(list, shapes, {row.x + 30.0f, row.center().y}, 48.0f, s, known ? 1.0f : 0.45f);
        kit::label(list, info.name, row.x + 66.0f, row.center().y - 9.0f, 25.0f, known ? ui_color::kInk : ui_color::kSoftInk, Align::Left,
                   selected ? 1.6f : 1.4f);
        const std::string sub = info.kind == game::SkillKind::Passive ? std::format("lv {} / {}  passive", level, c.skills.maxLevelOf(static_cast<int>(s)))
                                                                       : std::format("lv {} / {}", level, c.skills.maxLevelOf(static_cast<int>(s)));
        kit::label(list, sub, row.x + 66.0f, row.center().y + 15.0f, 19.0f, ui_color::kSoftInk);
        // The key it is on.
        if (const std::optional<ActionId> a = skillAction(actions, s)) {
            const std::vector<Key> keys = actions.keysOf(*a);
            if (!keys.empty() && known) {
                kit::keyCap(list, shapes, {row.right() - 120.0f, row.center().y}, gamelib::platform::keyLabel(keys.front()), 34.0f);
            }
        } else if (s == game::SkillId::HyperJump && known) {
            kit::label(list, "jump x2", row.right() - 120.0f, row.center().y, 20.0f, ui_color::kSoftInk, Align::Center);
        }
        const bool can = session.canLearn(s);
        addUi(list, shapes, Ui::Button, Rect{row.right() - 52.0f, row.center().y - 20.0f, 44.0f, 40.0f}.expanded(kit::kMargin),
              can ? srgb(0xFFF0D0) : srgb(0xE8E0D0), ui_color::kGold, can ? (selected ? 1.0f : 0.0f) : 3.0f);
        kit::label(list, "+", row.right() - 30.0f, row.center().y - 1.0f, 30.0f, can ? ui_color::kInk : ui_color::kSoftInk, Align::Center, 2.0f);
    }
    if (m_skillList.scrollable()) {
        gamelib::ui::drawScrollBar(list, m_skillList, {area.right() - 10.0f, area.y + 6.0f, 6.0f, area.height - 12.0f});
    }
    // The selected skill's details.
    const game::SkillId s = rows[static_cast<std::size_t>(std::min(m_skillList.cursor(), static_cast<int>(rows.size()) - 1))];
    const game::SkillInfo& info = game::skillInfo(s);
    const int level = c.skillLevel(s);
    const Rect detail{inner.x, area.bottom() + 18.0f, inner.width, inner.bottom() - area.bottom() - 18.0f};
    kit::well(list, shapes, detail);
    float y = detail.y + 12.0f;
    y += gamelib::draw::drawParagraph(list, kit::font(23.0f, 1.35f), info.description, detail.x + 16.0f, y, detail.width - 32.0f,
                                      ui_color::kInk) + 8.0f;
    if (level > 0) {
        kit::text(list, "now:  " + game::skillEffectText(s, level), detail.x + 16.0f, y, 21.0f, ui_color::kSoftInk);
        y += 30.0f;
    }
    if (level < c.skills.maxLevelOf(static_cast<int>(s))) {
        if (c.level < info.unlockLevel) {
            kit::text(list, std::format("opens at level {}", info.unlockLevel), detail.x + 16.0f, y, 21.0f, srgb(0xD8404E));
        } else if (s == game::SkillId::HyperJump && level == 0) {
            kit::text(list, "Pip in Petalbrook knows the trick.", detail.x + 16.0f, y, 21.0f, srgb(0xD8404E));
        } else {
            kit::text(list, "next: " + game::skillEffectText(s, level + 1), detail.x + 16.0f, y, 21.0f, srgb(0x2E8E4E));
        }
    }
    kit::text(list, "enter: learn    press a key: place it on that key", detail.x + 16.0f, detail.bottom() - 30.0f, 19.0f, ui_color::kSoftInk);
}

void Windows::drawInventory(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                            const gamelib::platform::ActionMap& actions, const Rect rect, const float time) const
{
    const game::Character& c = session.character();
    const Rect inner = kit::window(list, shapes, rect, "items");
    for (int i = 0; i < game::kTabCount; ++i) {
        kit::tab(list, shapes, {inner.x + static_cast<float>(i) * 168.0f, inner.y, 160.0f, 46.0f}, game::tabName(static_cast<game::Tab>(i)),
                 m_bagTabs.current() == i);
    }
    constexpr float kSlot = 88.0f;
    constexpr float kGap = 14.0f;
    const Vec2 origin{inner.x + (inner.width - kBagColumns * kSlot - (kBagColumns - 1) * kGap) * 0.5f, inner.y + 70.0f};
    const std::vector<Rect> slots = gamelib::ui::gridRects(origin, game::kBagSlots, kBagColumns, kSlot, kGap);
    auto& grid = const_cast<gamelib::ui::SlotGrid&>(m_bagGrid);
    if (grid.size() != static_cast<int>(slots.size()) || grid.rect(0) != slots.front()) {
        const int cursor = grid.cursor();
        grid.setSlots(slots);
        grid.select(cursor);
    }
    const auto tab = static_cast<game::Tab>(m_bagTabs.current());
    const gamelib::core::Inventory& bag = c.bag(tab);
    // Tall enough for six rows of 4.
    for (int i = 0; i < game::kBagSlots; ++i) {
        const Rect r = slots[static_cast<std::size_t>(i)];
        kit::slot(list, shapes, r, bag.at(i), i == grid.cursor(), time);
        // A potion on a key shows its key.
        if (tab == game::Tab::Use && !bag.at(i).empty()) {
            if (const std::optional<ActionId> a = itemAction(actions, bag.at(i).item)) {
                const std::vector<Key> keys = actions.keysOf(*a);
                if (!keys.empty()) {
                    const Font f = kit::outlined(16.0f, 1.3f);
                    gamelib::draw::drawText(list, f, gamelib::platform::keyLabel(keys.front()), r.x + 6.0f, r.y + 4.0f, srgb(0xFFF2C0));
                }
            }
        }
    }
    // Coins.
    const float coinsY = inner.bottom() - 30.0f;
    addIcon(list, shapes, Ui::Coin, {inner.x + 30.0f, coinsY}, 40.0f, 0.0f, 0.0f, srgb(0xFFFFFF), srgb(0xFFFFFF));
    kit::label(list, kit::groupDigits(c.coins), inner.x + 60.0f, coinsY, 28.0f, ui_color::kInk, Align::Left, 1.6f);
    kit::label(list, tab == game::Tab::Equip ? "enter: equip    del: drop" : tab == game::Tab::Use ? "enter: use    key: place on key" : "del: drop",
               inner.right(), coinsY, 19.0f, ui_color::kSoftInk, Align::Right);
    // The card of the selected item, beside the window.
    const gamelib::core::ItemStack& sel = bag.at(grid.cursor());
    if (!sel.empty()) {
        kit::itemCard(list, shapes, {rect.x - 470.0f, rect.y + 60.0f, 440.0f, 420.0f}, sel, c, tab == game::Tab::Equip);
    }
}

void Windows::drawEquipment(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Rect rect,
                            const float time) const
{
    const game::Character& c = session.character();
    const Rect inner = kit::window(list, shapes, rect, "equipment");
    // The hero in the middle, the six slots around.
    const Rect stage{inner.center().x - 150.0f, inner.y + 20.0f, 300.0f, 380.0f};
    addUi(list, shapes, Ui::Well, stage.expanded(kit::kMargin), srgb(0xF6EDDA), ui_color::kTitle, 0.0f, 0.0f, 18.0f);
    anime::Avatar a = heroAvatar(c);
    a.feet = {stage.center().x, stage.bottom() - 30.0f};
    a.height = 380.0f;
    a.seed = 200;
    a.expression = anime::Expression::Neutral;
    a.nextExpression = anime::Expression::Neutral;
    list.add(anime::avatarSprite(a, shapes.avatar));
    constexpr float kSlot = 96.0f;
    const std::array<Vec2, game::kSlotCount> places{{
        {stage.x - kSlot - 24.0f, stage.y + 150.0f},  // weapon
        {stage.x - kSlot - 24.0f, stage.y + 20.0f},   // hat
        {stage.right() + 24.0f, stage.y + 20.0f},     // top
        {stage.right() + 24.0f, stage.y + 150.0f},    // bottom
        {stage.right() + 24.0f, stage.y + 280.0f},    // shoes
        {stage.x - kSlot - 24.0f, stage.y + 280.0f},  // gloves
    }};
    std::vector<Rect> slots;
    for (const Vec2 p : places) {
        slots.push_back({p.x, p.y, kSlot, kSlot});
    }
    auto& grid = const_cast<gamelib::ui::SlotGrid&>(m_gearGrid);
    if (grid.size() != static_cast<int>(slots.size()) || grid.rect(0) != slots.front()) {
        const int cursor = grid.cursor();
        grid.setSlots(slots);
        grid.select(cursor);
    }
    for (int i = 0; i < game::kSlotCount; ++i) {
        const Rect r = slots[static_cast<std::size_t>(i)];
        kit::slot(list, shapes, r, c.gear.at(i), i == grid.cursor(), time);
        if (c.gear.at(i).empty()) {
            addIcon(list, shapes, Ui::ItemIcon, r.center(), r.height * 0.7f, 60.0f + static_cast<float>(i), 0.0f, srgb(0xD8CCB8, 0.6f),
                    srgb(0xD8CCB8, 0.6f));
        }
        kit::label(list, game::slotName(static_cast<game::Slot>(i)), r.center().x, r.bottom() + 16.0f, 18.0f, ui_color::kSoftInk, Align::Center);
    }
    const gamelib::core::ItemStack& sel = c.gear.at(grid.cursor());
    if (!sel.empty()) {
        kit::itemCard(list, shapes, {rect.right() + 30.0f, rect.y + 60.0f, 440.0f, 400.0f}, sel, c, false);
    }
    // The totals of the gear.
    const game::Derived d = session.derived();
    const Rect sum{inner.x, stage.bottom() + 60.0f, inner.width, inner.bottom() - stage.bottom() - 60.0f};
    kit::well(list, shapes, sum);
    const std::array<std::pair<std::string_view, int>, 6> totals{{{"attack", d.attack}, {"magic", d.magic}, {"defense", d.defense},
                                                                  {"critical", d.crit}, {"speed", d.speed}, {"jump", d.jump}}};
    for (std::size_t i = 0; i < totals.size(); ++i) {
        const float x = sum.x + 24.0f + static_cast<float>(i % 3) * (sum.width - 48.0f) / 3.0f;
        const float y = sum.y + 30.0f + static_cast<float>(i / 3) * 40.0f;
        kit::label(list, totals[i].first, x, y, 21.0f, ui_color::kSoftInk);
        kit::label(list, std::format("{}{}", totals[i].second, i >= 3 ? "%" : ""), x + 150.0f, y, 23.0f, ui_color::kInk, Align::Right, 1.5f);
    }
    kit::label(list, "enter: take off", inner.center().x, inner.bottom() - 14.0f, 19.0f, ui_color::kSoftInk, Align::Center);
}

void Windows::drawQuests(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Rect rect) const
{
    const Rect inner = kit::window(list, shapes, rect, "quests");
    for (int i = 0; i < 2; ++i) {
        kit::tab(list, shapes, {inner.x + static_cast<float>(i) * 230.0f, inner.y, 220.0f, 46.0f}, m_questTabs.labels()[static_cast<std::size_t>(i)],
                 m_questTabs.current() == i);
    }
    const std::vector<int> rows = questRows(session);
    const Rect area{inner.x, inner.y + 62.0f, 440.0f, inner.bottom() - inner.y - 62.0f};
    kit::well(list, shapes, area);
    if (rows.empty()) {
        kit::label(list, m_questTabs.current() == 0 ? "no quests yet - look for a !" : "nothing done yet", area.center().x, area.y + 40.0f, 22.0f,
                   ui_color::kSoftInk, Align::Center);
        return;
    }
    const_cast<gamelib::ui::ListView&>(m_questList).setVisibleRows(static_cast<int>(area.height / 64.0f));
    for (int i = m_questList.first(); i <= m_questList.last() && i < static_cast<int>(rows.size()); ++i) {
        const int q = rows[static_cast<std::size_t>(i)];
        const game::QuestText& t = game::questText(q);
        const Rect row = m_questList.rowRect(area, i, 64.0f).expanded(-4.0f);
        if (i == m_questList.cursor()) {
            addUi(list, shapes, Ui::Button, row.expanded(kit::kMargin - 2.0f), srgb(0xFFE7B8), ui_color::kGold, 1.0f);
        }
        const bool complete = session.quests().stateOf(q) == gamelib::core::QuestState::Complete;
        addIcon(list, shapes, Ui::Marker, {row.x + 26.0f, row.center().y}, 44.0f, complete ? 2.0f : (m_questTabs.current() == 1 ? 2.0f : 1.0f),
                0.0f, complete ? srgb(0x4CC86A) : (t.main ? srgb(0xFFC23A) : srgb(0xB8B0C8)), srgb(0xFFFFFF));
        kit::label(list, t.title, row.x + 56.0f, row.center().y, 23.0f, ui_color::kInk, Align::Left, i == m_questList.cursor() ? 1.6f : 1.35f);
    }
    // The selected quest.
    const int q = rows[static_cast<std::size_t>(std::min(m_questList.cursor(), static_cast<int>(rows.size()) - 1))];
    const game::QuestText& t = game::questText(q);
    const gamelib::core::QuestDef* def = session.quests().find(q);
    const Rect detail{area.right() + 24.0f, area.y, inner.right() - area.right() - 24.0f, area.height};
    kit::well(list, shapes, detail);
    float y = detail.y + 16.0f;
    kit::text(list, t.title, detail.x + 20.0f, y, 32.0f, ui_color::kInk, Align::Left, 1.7f);
    y += 46.0f;
    kit::text(list, t.main ? "main story" : "side quest", detail.x + 20.0f, y, 20.0f, t.main ? srgb(0xD08A1E) : ui_color::kSoftInk);
    y += 36.0f;
    y += gamelib::draw::drawParagraph(list, kit::font(23.0f, 1.35f), t.summary, detail.x + 20.0f, y, detail.width - 40.0f, ui_color::kInk) + 18.0f;
    if (def != nullptr) {
        const game::NpcId to = static_cast<game::NpcId>(def->turnInTo != 0 ? def->turnInTo : def->giver);
        kit::text(list, std::format("from {}  -  hand in to {}", game::npcName(static_cast<game::NpcId>(def->giver)), game::npcName(to)),
                  detail.x + 20.0f, y, 20.0f, ui_color::kSoftInk);
        y += 40.0f;
        for (std::size_t i = 0; i < def->objectives.size(); ++i) {
            const auto& o = def->objectives[i];
            const int have = session.quests().progressOf(q, static_cast<int>(i));
            const float frac = static_cast<float>(have) / static_cast<float>(std::max(o.required, 1));
            kit::gauge(list, shapes, {detail.x + 20.0f, y, detail.width - 160.0f, 20.0f}, frac, frac, srgb(0x7CCB58), srgb(0xD8F0B0));
            kit::label(list, std::format("{}/{}", have, o.required), detail.right() - 20.0f, y + 10.0f, 21.0f, ui_color::kInk, Align::Right, 1.5f);
            y += 40.0f;
        }
        y += 10.0f;
        kit::text(list, "rewards", detail.x + 20.0f, y, 22.0f, ui_color::kSoftInk, Align::Left, 1.5f);
        y += 34.0f;
        std::string rewards = std::format("{} exp   {} coins", kit::groupDigits(def->reward.experience), kit::groupDigits(def->reward.coins));
        kit::text(list, rewards, detail.x + 20.0f, y, 22.0f, ui_color::kInk);
        y += 34.0f;
        float x = detail.x + 20.0f;
        for (const auto& item : def->reward.items) {
            const Rect r{x, y, 64.0f, 64.0f};
            kit::slot(list, shapes, r, {.item = item.item, .count = item.count}, false, 0.0f);
            x += 76.0f;
        }
    }
}

void Windows::drawWorldMap(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Rect rect,
                           const float time) const
{
    const Rect inner = kit::window(list, shapes, rect, "world map");
    const Rect map{inner.x, inner.y, inner.width, inner.height - 70.0f};
    addUi(list, shapes, Ui::Map, map, srgb(0xFFFFFF), srgb(0xFFFFFF));
    const auto at = [&](const game::MapId id) {
        const Vec2 pin = mapPin(id);
        return Vec2{map.center().x + pin.x * map.width * 0.5f, map.center().y + pin.y * map.height * 0.5f};
    };
    // Region names.
    const Font regionFont = kit::outlined(30.0f, 1.6f);
    // Each region's name in an open spot beside its paths (map fractions).
    const std::array<std::pair<Vec2, std::string_view>, 5> regions{{{{-0.62f, 0.0f}, "Petal Isle"},
                                                                   {{-0.02f, 0.26f}, "Saltwhistle"},
                                                                   {{0.62f, 0.44f}, "Fernwood"},
                                                                   {{0.2f, -0.55f}, "Windcrest Pass"},
                                                                   {{0.3f, 0.86f}, "The Undercroft"}}};
    for (const auto& [pin, name] : regions) {
        const Vec2 p{map.center().x + pin.x * map.width * 0.5f, map.center().y + pin.y * map.height * 0.5f};
        gamelib::draw::drawText(list, regionFont, name, p.x, regionFont.topFor(p.y), srgb(0xFFF8E8), Align::Center);
    }
    // The paths between maps, and the ship's route.
    const gamelib::core::WorldGraph& graph = game::worldGraph();
    for (const gamelib::core::Exit& e : graph.exits()) {
        if (e.target < 0) {
            continue;
        }
        const auto [fromMap, fromPortal] = game::exitPortal(static_cast<int>(&e - graph.exits().data()));
        const auto [toMap, toPortal] = game::exitPortal(e.target);
        (void)fromPortal;
        (void)toPortal;
        if (static_cast<int>(fromMap) > static_cast<int>(toMap)) {
            continue;
        }
        const bool seen = session.visited(fromMap) || session.visited(toMap);
        const Vec2 a = at(fromMap);
        const Vec2 b = at(toMap);
        const Vec2 d = b - a;
        const float len = std::sqrt(d.x * d.x + d.y * d.y);
        list.add({.rect = {(a.x + b.x) * 0.5f - len * 0.5f, (a.y + b.y) * 0.5f - 4.0f, len, 8.0f},
                  .color = seen ? srgb(0xFFF4D8, 0.9f) : srgb(0xFFF4D8, 0.3f),
                  .shape = gamelib::draw::shapes::Box,
                  .rotation = std::atan2(d.y, d.x),
                  .params = {4.0f, 0.0f, 0.0f, 0.0f}});
    }
    {
        const Vec2 a = at(game::MapId::PetalPier);
        const Vec2 b = at(game::MapId::SaltwhistleDocks);
        for (int i = 0; i <= 10; ++i) {
            const float t = static_cast<float>(i) / 10.0f;
            const Vec2 p = gamelib::core::lerp(a, b, t) + Vec2{0.0f, std::sin(t * 3.14159f) * 30.0f};
            list.circle(p, 4.0f, srgb(0xFFFFFF, 0.8f));
        }
    }
    // The pins: visited maps bright, others faint; towns as houses.
    for (int i = 0; i < game::kMapCount; ++i) {
        const auto id = static_cast<game::MapId>(i);
        const game::MapDef& def = game::mapDef(id);
        const bool seen = session.visited(id);
        const Vec2 p = at(id);
        const Color c = def.town() ? srgb(0xFFD86A) : def.kind == game::MapKind::Arena ? srgb(0xFF7A6A)
                       : def.region == game::Region::Depths ? srgb(0xB89AFF) : srgb(0xFFFFFF);
        const float size = def.town() ? 30.0f : 22.0f;
        list.circle(p, size * 0.5f + 3.0f, srgb(0x3A2E4A, seen ? 0.9f : 0.35f));
        list.circle(p, size * 0.5f, c.withAlpha(seen ? 1.0f : 0.35f));
    }
    // You are here, and the cursor's map.
    const Vec2 here = at(session.mapId());
    const float pulse = 0.5f + 0.5f * std::sin(time * 5.0f);
    addFx(list, shapes, Fx::Ring, here, 80.0f + pulse * 20.0f, srgb(0xFFF6A0), srgb(0xFFB040), pulse * 0.5f);
    addIcon(list, shapes, Ui::Marker, here + Vec2{0.0f, -48.0f + pulse * -6.0f}, 52.0f, 0.0f, 0.0f, srgb(0xFF7A9A), srgb(0xFFFFFF));
    const auto cursor = static_cast<game::MapId>(m_mapCursor);
    const Vec2 cp = at(cursor);
    addFx(list, shapes, Fx::Ring, cp, 64.0f, srgb(0xFFFFFF), srgb(0x8CD8FF), 0.2f);
    const game::MapDef& cd = game::mapDef(cursor);
    int lo = 99;
    int hi = 0;
    for (const game::SpawnDef& sd : cd.spawns) {
        lo = std::min(lo, game::species(sd.species).level);
        hi = std::max(hi, game::species(sd.species).level + 1);
    }
    std::string info = std::string{cd.name};
    if (hi > 0) {
        info += std::format("  -  monsters lv {}-{}", lo, hi);
    } else if (cd.town()) {
        info += "  -  town";
    } else if (cd.arena) {
        info += std::format("  -  {}", game::bossInfo(cd.arena->boss).name);
    }
    kit::label(list, info, inner.center().x, inner.bottom() - 30.0f, 28.0f, ui_color::kInk, Align::Center, 1.6f);
}

void Windows::drawKeys(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                       const gamelib::platform::ActionMap& actions, const Rect rect, const float time) const
{
    (void)kit::window(list, shapes, rect, "key bindings");
    const Rect keyboard = keyboardRect();
    addUi(list, shapes, Ui::Well, keyboard.expanded(14.0f + kit::kMargin), srgb(0xE8DCC6), ui_color::kTitle, 0.0f, 0.0f, 18.0f);
    const std::vector<PaletteEntry> entries = palette(session, actions);
    const gamelib::ui::KeyPalette::Layout layout = m_keys.layout(keyboard, paletteRect(), kKeyGap, kEntryGap);
    const game::Character& c = session.character();
    // GameLib's KeyPalette lays out and runs the keyboard and the palette;
    // Realm paints the caps, the entries and what is in hand its own way.
    gamelib::ui::KeyPaletteStyle style;
    style.capPainter = [&](gamelib::draw::DrawList& l, const gamelib::ui::KeyCapView& cap) {
        const ActionId a = cap.lifted ? gamelib::platform::kNoAction : cap.action;
        Color face = srgb(0xF6F0E6);
        std::optional<game::SkillId> skill;
        int item = 0;
        if (a != gamelib::platform::kNoAction) {
            skill = actionSkill(actions, a);
            item = actionItem(actions, a);
            const std::string& cat = actions.info(a).category;
            face = skill ? srgb(0xD8E8FF) : item != 0 ? srgb(0xFFE0E4) : cat == "MENUS" ? srgb(0xE6DCF8) : cat == "SYSTEM" ? srgb(0xD8D4CC)
                                                                                                                            : srgb(0xFFF0C8);
        }
        const Rect r = cap.rect;
        addUi(l, shapes, Ui::KeyCap, r.expanded(kit::kMargin), face, srgb(0xFFFFFF));
        if (cap.focused) {
            addUi(l, shapes, Ui::Slot, r.expanded(kit::kMargin + 2.0f), srgb(0x000000, 0.0f), ui_color::kGold, 0.0f,
                  0.75f + 0.25f * std::sin(time * 6.0f), 10.0f);
        }
        gamelib::draw::drawText(l, kit::font(16.0f, 1.3f), gamelib::platform::keyLabel(cap.key), r.x + 7.0f, r.y + 5.0f, ui_color::kSoftInk);
        if (skill) {
            // A learned skill's icon; one not learned yet, faded.
            kit::skillIconAt(l, shapes, {r.center().x, r.center().y + 6.0f}, std::min(r.height, r.width) * 0.62f, *skill,
                             c.skillLevel(*skill) > 0 ? 1.0f : 0.35f);
        } else if (item != 0) {
            kit::slotIcon(l, shapes, Rect{r.center().x - 22.0f, r.center().y - 16.0f, 44.0f, 44.0f}, item, 0, c.countOf(item) > 0 ? 1.0f : 0.4f);
        } else if (a != gamelib::platform::kNoAction) {
            fitLabel(l, lower(actions.info(a).label), {r.x + 4.0f, r.y + 22.0f, r.width - 8.0f, r.height - 24.0f}, 19.0f, 12.0f, ui_color::kInk);
        }
    };
    style.entryPainter = [&](gamelib::draw::DrawList& l, const gamelib::ui::KeyEntryView& view) {
        const PaletteEntry& e = entries[view.index];
        const Rect r = view.rect;
        addUi(l, shapes, Ui::Button, r.expanded(kit::kMargin - 2.0f), view.held ? srgb(0xD8F0D0) : srgb(0xFFF4DE), ui_color::kGold,
              view.selected ? 1.0f : 0.0f);
        if (e.skill) {
            kit::skillIconAt(l, shapes, {r.x + 34.0f, r.center().y}, 46.0f, *e.skill);
        } else if (e.item != 0) {
            kit::slotIcon(l, shapes, Rect{r.x + 12.0f, r.center().y - 22.0f, 44.0f, 44.0f}, e.item, 0, e.usable ? 1.0f : 0.4f);
        } else {
            // An action: a small key cap in its category's color (as on the keyboard).
            const Color tint = actions.info(e.action).category == "MENUS" ? srgb(0xE6DCF8) : srgb(0xFFF0C8);
            addUi(l, shapes, Ui::KeyCap, Rect{r.x + 14.0f, r.center().y - 18.0f, 40.0f, 36.0f}.expanded(kit::kMargin), tint, srgb(0xFFFFFF));
        }
        kit::label(l, e.label, r.x + 66.0f, r.center().y - 8.0f, 20.0f, ui_color::kInk, Align::Left, view.selected ? 1.5f : 1.3f);
        const bool none = view.key == Key::Unknown;
        kit::label(l, none ? std::string{"no key"} : "on " + lower(gamelib::platform::keyLabel(view.key)), r.x + 66.0f, r.center().y + 16.0f,
                   16.0f, none ? srgb(0xD8404E) : ui_color::kSoftInk);
    };
    style.heldPainter = [&](gamelib::draw::DrawList& l, const ActionId held, const Vec2 at) {
        if (const std::optional<game::SkillId> s = actionSkill(actions, held)) {
            kit::skillIconAt(l, shapes, at, 56.0f, *s);
        } else if (const int item = actionItem(actions, held); item != 0) {
            kit::slotIcon(l, shapes, Rect{at.x - 28.0f, at.y - 28.0f, 56.0f, 56.0f}, item, 0);
        } else {
            kit::keyCap(l, shapes, at, lower(actions.info(held).label), 34.0f);
        }
    };
    gamelib::ui::drawKeyPalette(list, m_keys, layout, actions, style);
    // The palette's heading and where it is scrolled to.
    const Rect pal = paletteRect();
    kit::label(list, "pick a skill, item or action, then press its new key - or drag it onto a key", pal.x, pal.y - 34.0f, 21.0f,
               ui_color::kSoftInk);
    const int rows = m_keys.rowCount();
    if (rows > m_keys.rows()) {
        const int firstRow = m_keys.firstRow();
        const std::string more = std::format("rows {}-{} of {}{}{}", firstRow + 1, std::min(rows, firstRow + m_keys.rows()), rows,
                                             firstRow > 0 ? "   ^ more above" : "", firstRow + m_keys.rows() < rows ? "   v more below" : "");
        kit::label(list, more, pal.right(), pal.y - 34.0f, 20.0f, srgb(0xD08A1E), Align::Right, 1.4f);
    }
    const std::string hint = !m_message.empty() && m_messageAge < 4.0f ? m_message
                                                                       : "arrows move   enter: pick or place   del: clear a key   tab: keyboard / palette   esc: close";
    kit::label(list, hint, rect.center().x, rect.bottom() - 34.0f, 21.0f, m_message.empty() ? ui_color::kSoftInk : srgb(0x2E7E4E),
               Align::Center, 1.4f);
}

void Windows::drawShop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Rect rect,
                       const float time) const
{
    (void)time;
    const game::Character& c = session.character();
    const std::string title = session.talkingTo() ? std::string{game::npcName(*session.talkingTo())} + "'s shop" : std::string{"shop"};
    const Rect inner = kit::window(list, shapes, rect, title);
    for (int i = 0; i < 2; ++i) {
        kit::tab(list, shapes, {inner.x + static_cast<float>(i) * 200.0f, inner.y, 190.0f, 46.0f}, m_shopTabs.labels()[static_cast<std::size_t>(i)],
                 m_shopTabs.current() == i);
    }
    addIcon(list, shapes, Ui::Coin, {inner.right() - 240.0f, inner.y + 23.0f}, 40.0f, 0.0f, 0.0f, srgb(0xFFFFFF), srgb(0xFFFFFF));
    kit::label(list, kit::groupDigits(c.coins), inner.right() - 210.0f, inner.y + 23.0f, 28.0f, ui_color::kInk, Align::Left, 1.6f);
    const Rect area{inner.x, inner.y + 62.0f, 720.0f, inner.bottom() - inner.y - 62.0f};
    kit::well(list, shapes, area);
    const_cast<gamelib::ui::ListView&>(m_shopList).setVisibleRows(static_cast<int>(area.height / 74.0f));
    gamelib::core::ItemStack shown{};
    if (m_shopTabs.current() == 0) {
        const std::vector<gamelib::core::ShopEntry> entries = session.shopEntries();
        for (int i = m_shopList.first(); i <= m_shopList.last() && i < static_cast<int>(entries.size()); ++i) {
            const int item = entries[static_cast<std::size_t>(i)].item;
            const gamelib::core::ItemDef& def = game::itemRegistry().at(item);
            const Rect row = m_shopList.rowRect(area, i, 74.0f).expanded(-4.0f);
            const bool selected = i == m_shopList.cursor();
            if (selected) {
                addUi(list, shapes, Ui::Button, row.expanded(kit::kMargin - 2.0f), srgb(0xFFE7B8), ui_color::kGold, 1.0f);
                shown = {.item = item, .count = 1};
            }
            kit::slot(list, shapes, {row.x + 8.0f, row.y + 6.0f, row.height - 12.0f, row.height - 12.0f}, {.item = item, .count = 1}, false, 0.0f);
            kit::label(list, def.name, row.x + 86.0f, row.center().y - 10.0f, 24.0f, ui_color::kInk, Align::Left, selected ? 1.6f : 1.35f);
            const bool wearable = !game::isEquipment(item) || ((def.jobs & game::jobBit(c.job)) != 0 && def.levelRequirement <= c.level);
            const std::string sub = game::isEquipment(item) ? std::format("lv {} {}", def.levelRequirement, game::slotName(game::slotOf(item)))
                                                            : std::string{game::itemInfo(item).text};
            kit::label(list, sub.size() > 46 ? sub.substr(0, 44) + "..." : sub, row.x + 86.0f, row.center().y + 16.0f, 18.0f,
                       wearable ? ui_color::kSoftInk : srgb(0xD8404E));
            const int qty = selected && def.stackLimit > 1 ? m_quantity : 1;
            const bool afford = c.coins >= static_cast<long long>(def.price) * qty;
            kit::label(list, std::format("{}{}", kit::groupDigits(static_cast<long long>(def.price) * qty), qty > 1 ? std::format(" (x{})", qty) : ""),
                       row.right() - 16.0f, row.center().y, 24.0f, afford ? ui_color::kInk : srgb(0xD8404E), Align::Right, 1.5f);
        }
        kit::label(list, "enter: buy    left/right: how many", area.center().x, rect.bottom() - 34.0f, 19.0f, ui_color::kSoftInk, Align::Center);
    } else {
        std::vector<std::pair<game::Tab, int>> sellable;
        for (int t = 0; t < game::kTabCount; ++t) {
            for (int s = 0; s < game::kBagSlots; ++s) {
                if (!c.bags[static_cast<std::size_t>(t)].at(s).empty()) {
                    sellable.emplace_back(static_cast<game::Tab>(t), s);
                }
            }
        }
        if (sellable.empty()) {
            kit::label(list, "nothing to sell", area.center().x, area.y + 40.0f, 22.0f, ui_color::kSoftInk, Align::Center);
        }
        for (int i = m_shopList.first(); i <= m_shopList.last() && i < static_cast<int>(sellable.size()); ++i) {
            const auto [tab, slot] = sellable[static_cast<std::size_t>(i)];
            const gamelib::core::ItemStack& s = c.bag(tab).at(slot);
            const Rect row = m_shopList.rowRect(area, i, 74.0f).expanded(-4.0f);
            const bool selected = i == m_shopList.cursor();
            if (selected) {
                addUi(list, shapes, Ui::Button, row.expanded(kit::kMargin - 2.0f), srgb(0xFFE7B8), ui_color::kGold, 1.0f);
                shown = s;
            }
            kit::slot(list, shapes, {row.x + 8.0f, row.y + 6.0f, row.height - 12.0f, row.height - 12.0f}, s, false, 0.0f);
            kit::label(list, game::itemName(s.item), row.x + 86.0f, row.center().y, 24.0f,
                       s.rarity > 0 ? gamelib::core::lerp(rarityColor(game::rarityOf(s)), ui_color::kInk, 0.35f) : ui_color::kInk, Align::Left, selected ? 1.6f : 1.35f);
            const bool questItem = s.item >= game::kNanaLetter && s.item < 300;
            kit::label(list, questItem ? std::string{"keep"} : std::format("+{}", kit::groupDigits(static_cast<long long>(game::sellValue(s)) * s.count)),
                       row.right() - 16.0f, row.center().y, 24.0f, questItem ? ui_color::kSoftInk : srgb(0x2E8E4E), Align::Right, 1.5f);
        }
        kit::label(list, "enter: sell the whole stack", area.center().x, rect.bottom() - 34.0f, 19.0f, ui_color::kSoftInk, Align::Center);
    }
    if (m_shopList.scrollable()) {
        gamelib::ui::drawScrollBar(list, m_shopList, {area.right() - 10.0f, area.y + 6.0f, 6.0f, area.height - 12.0f});
    }
    if (!shown.empty()) {
        kit::itemCard(list, shapes, {area.right() + 30.0f, area.y, inner.right() - area.right() - 30.0f, 440.0f}, shown, c, true);
    }
}

} // namespace realm::app
