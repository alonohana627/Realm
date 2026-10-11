#include "hud.hpp"

#include "actions.hpp"
#include "ui_kit.hpp"

#include <gamelib/core/easing.hpp>
#include <gamelib/core/grid_layout.hpp>
#include <gamelib/draw/font.hpp>
#include <gamelib/ui/key_binding_screen.hpp>
#include <gamelib/ui/world_map.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace realm::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::draw::Align;
using gamelib::draw::Font;

constexpr float kW = 1920.0f;
constexpr float kH = 1080.0f;
constexpr float kBarTop = 968.0f;

struct QuickKey {
    gamelib::platform::Key key;
    std::optional<game::SkillId> skill;
    int item = 0;
};

std::vector<QuickKey> quickKeys(const gamelib::platform::ActionMap& actions, const game::Character& c)
{
    std::vector<QuickKey> keys;
    for (const gamelib::ui::KeyCap& cap : gamelib::ui::keyboardLayout()) {
        const gamelib::platform::ActionId id = actions.actionOn(cap.key);
        if (id == gamelib::platform::kNoAction) {
            continue;
        }
        if (const std::optional<game::SkillId> skill = actionSkill(actions, id)) {
            if (c.skillLevel(*skill) > 0) {
                keys.push_back({.key = cap.key, .skill = skill});
            }
        } else if (const int item = actionItem(actions, id); item != 0) {
            keys.push_back({.key = cap.key, .item = item});
        }
        if (keys.size() >= 14) {
            break;
        }
    }
    return keys;
}

std::string timeText(const gamelib::core::GameClock& clock)
{
    const int h = clock.hour();
    const int m = clock.minute();
    return std::format("{}:{:02} {}", h % 12 == 0 ? 12 : h % 12, m / 10 * 10, h < 12 ? "am" : "pm");
}

} // namespace

Hud::Hud()
{
}

void Hud::reset(const game::Session& session)
{
    const game::Character& c = session.character();
    m_hp.reset(c.hp.fraction());
    m_mp.reset(c.mp.fraction());
    m_xpShown = c.xpProgress();
    m_bossShown = 0.0f;
    m_log.clear();
    m_banners.clear();
}

void Hud::log(std::string text, const Color color)
{
    m_log.add(std::move(text), color);
}

void Hud::banner(std::string title, std::string line, const Color color, const float seconds)
{
    m_banners.show(std::move(title), std::move(line), color, seconds);
}

void Hud::mapTitle(const game::Session& session)
{
    const game::MapDef& m = session.map();
    m_title.show(std::string{m.name}, std::string{game::regionName(m.region)}, srgb(0xFFFFFF), 3.2f);
}

void Hud::react(const game::Event& e, const game::Session& session)
{
    using game::EventKind;
    switch (e.kind) {
    case EventKind::Pickup:
        log(std::format("you got {}{}", game::itemName(e.value), session.character().countOf(e.value) > 1 ? "" : ""), rarityColor(game::rarityOf({.item = e.value, .count = 1, .rarity = e.extra})));
        break;
    case EventKind::Coins:
        if (e.value > 0) {
            log(std::format("+{} coins", kit::groupDigits(e.value)), srgb(0xFFE08A));
        }
        break;
    case EventKind::QuestAccepted: log(std::format("new quest: {}", game::questText(e.value).title), srgb(0xFFD86A)); break;
    case EventKind::QuestUpdate: {
        const gamelib::core::QuestDef* def = session.quests().find(e.value);
        if (def != nullptr && !def->objectives.empty()) {
            log(std::format("{}  {}/{}", game::questText(e.value).title, e.extra, def->objectives.front().required), srgb(0xD8F0FF));
        }
        break;
    }
    case EventKind::QuestComplete:
        log(std::format("{} - ready to hand in!", game::questText(e.value).title), srgb(0x9AF0A0));
        banner("quest complete!", std::string{game::questText(e.value).title}, srgb(0xFFE070), 2.6f);
        break;
    case EventKind::QuestDone: log(std::format("quest done: {}", game::questText(e.value).title), srgb(0x9AF0A0)); break;
    case EventKind::LevelUp: log(std::format("you reached level {}!", e.value), srgb(0xFFE070)); break;
    case EventKind::SkillLearned:
        log(std::format("{} is now level {}", game::skillInfo(static_cast<game::SkillId>(e.value)).name,
                        session.character().skillLevel(static_cast<game::SkillId>(e.value))),
            srgb(0xC8E0FF));
        break;
    case EventKind::JobAdvance:
        banner(std::format("you are now a {}!", game::jobName(static_cast<game::Job>(e.value))), "new skills await in the skill window",
               srgb(0xFFE070), 4.0f);
        break;
    case EventKind::Full: log(e.value == 1 ? "not enough coins" : "your bag is full", srgb(0xFF9A9A)); break;
    case EventKind::NoMana:
        if (e.value < -1) {
            log(std::format("no {} left", game::itemName(-1 - e.value)), srgb(0xFF9A9A));
        } else {
            log("not enough mp", srgb(0x9AC8FF));
        }
        break;
    case EventKind::PortalClosed: log("the way is sealed", srgb(0xFFB0B0)); break;
    case EventKind::BossWake:
        banner(std::string{game::bossInfo(static_cast<game::BossId>(e.value)).name},
               std::string{game::bossInfo(static_cast<game::BossId>(e.value)).title}, srgb(0xFF9A8A), 3.4f);
        break;
    case EventKind::BossDown:
        banner(std::format("{} is calm again", game::bossInfo(static_cast<game::BossId>(e.value)).name), "", srgb(0xFFE070), 3.0f);
        break;
    case EventKind::Rest: log(e.value == 1 ? "you slept until morning" : "you feel rested", srgb(0xFFE08A)); break;
    case EventKind::Sold: log(std::format("sold for {} coins", kit::groupDigits(e.value)), srgb(0xFFE08A)); break;
    default: break;
    }
}

void Hud::update(const float seconds, const game::Session& session)
{
    const game::Character& c = session.character();
    m_hp.set(c.hp.fraction());
    m_mp.set(c.mp.fraction());
    m_hp.update(seconds);
    m_mp.update(seconds);
    // The experience bar fills smoothly (and wraps on a level up).
    const float xp = c.xpProgress();
    if (xp < m_xpShown - 0.02f) {
        m_xpShown = 0.0f;
    }
    m_xpShown += (xp - m_xpShown) * (1.0f - std::exp(-8.0f * seconds));
    m_log.update(seconds);
    m_banners.update(seconds);
    m_title.update(seconds);
    const bool boss = session.boss() && session.boss()->fighting();
    m_bossShown = gamelib::core::approach(m_bossShown, boss ? 1.0f : 0.0f, seconds * 3.0f);
    if (session.boss()) {
        const game::Boss& b = *session.boss();
        m_boss.set(static_cast<float>(b.hp) / static_cast<float>(std::max(b.maxHp, 1)));
    }
    m_boss.update(seconds);
}

void Hud::drawStatus(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    const game::Character& c = session.character();
    // The bar's panel.
    addUi(list, shapes, Ui::Window, Rect{8.0f, kBarTop, kW - 16.0f, kH - kBarTop - 26.0f}.expanded(kit::kMargin), srgb(0xFFF6E6, 0.96f),
          ui_color::kTitle, 0.0f, 0.0f, 20.0f);
    // The level badge.
    const Vec2 badge{70.0f, kBarTop + 41.0f};
    addIcon(list, shapes, Ui::Badge, badge, 92.0f, 0.0f, 0.0f, srgb(0xFFF6E0), srgb(0xFFFFFF));
    kit::label(list, "lv", badge.x, badge.y - 22.0f, 16.0f, srgb(0xB07A2A), Align::Center, 1.4f);
    kit::label(list, std::to_string(c.level), badge.x, badge.y + 6.0f, c.level >= 10 ? 34.0f : 38.0f, ui_color::kInk, Align::Center, 2.0f);
    // Name and job.
    kit::label(list, c.look.name, 132.0f, kBarTop + 20.0f, 26.0f, ui_color::kInk, Align::Left, 1.6f);
    kit::label(list, game::jobName(c.job), 132.0f, kBarTop + 52.0f, 20.0f, ui_color::kSoftInk, Align::Left, 1.3f);
    // HP and MP.
    const float gx = 300.0f;
    const float gw = 420.0f;
    const Rect hp{gx, kBarTop + 8.0f, gw, 26.0f};
    const Rect mp{gx, kBarTop + 44.0f, gw, 26.0f};
    kit::gauge(list, shapes, hp, c.hp.fraction(), m_hp.value(), ui_color::kHp, srgb(0xFFE0A0));
    kit::gauge(list, shapes, mp, c.mp.fraction(), m_mp.value(), ui_color::kMp, srgb(0xC8E0FF));
    const Font f = kit::outlined(19.0f, 1.3f);
    gamelib::draw::drawText(list, f, "hp", hp.left() + 14.0f, f.topFor(hp.center().y), srgb(0xFFFFFF));
    gamelib::draw::drawText(list, f, "mp", mp.left() + 14.0f, f.topFor(mp.center().y), srgb(0xFFFFFF));
    gamelib::draw::drawText(list, f, std::format("{} / {}", c.hp.current, c.hp.max), hp.center().x + 16.0f, f.topFor(hp.center().y),
                            srgb(0xFFFFFF), Align::Center);
    gamelib::draw::drawText(list, f, std::format("{} / {}", c.mp.current, c.mp.max), mp.center().x + 16.0f, f.topFor(mp.center().y),
                            srgb(0xFFFFFF), Align::Center);
    // Points to spend: little reminders.
    float px = gx + gw + 30.0f;
    if (c.points.available() > 0) {
        addUi(list, shapes, Ui::Plate, Rect{px, kBarTop + 10.0f, 150.0f, 28.0f}.expanded(kit::kMargin), srgb(0xF2B33D, 0.95f), srgb(0xFFFFFF));
        kit::label(list, std::format("{} ap", c.points.available()), px + 75.0f, kBarTop + 24.0f, 18.0f, srgb(0xFFFFFF), Align::Center, 1.5f);
        px += 0.0f;
    }
    if (c.skills.points() > 0) {
        addUi(list, shapes, Ui::Plate, Rect{px, kBarTop + 46.0f, 150.0f, 28.0f}.expanded(kit::kMargin), srgb(0x6A9AE8, 0.95f), srgb(0xFFFFFF));
        kit::label(list, std::format("{} sp", c.skills.points()), px + 75.0f, kBarTop + 60.0f, 18.0f, srgb(0xFFFFFF), Align::Center, 1.5f);
    }
    // Experience along the bottom.
    const Rect xp{16.0f, kH - 24.0f, kW - 32.0f, 16.0f};
    kit::gauge(list, shapes, xp, m_xpShown, m_xpShown, ui_color::kXp, srgb(0xFFF2B0));
    const Font xf = kit::outlined(15.0f, 1.2f);
    const std::string xpText = c.level >= game::kMaxLevel ? std::string{"exp max"}
                                                           : std::format("exp {:.2f}%", static_cast<double>(c.xpProgress() * 100.0f));
    gamelib::draw::drawText(list, xf, xpText, xp.center().x, xf.topFor(xp.center().y), srgb(0xFFFFFF), Align::Center);
}

void Hud::drawQuickSlots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                         const gamelib::platform::ActionMap& actions, const float time) const
{
    (void)time;
    const game::Character& c = session.character();
    const std::vector<QuickKey> keys = quickKeys(actions, c);
    constexpr float kSlot = 50.0f;
    constexpr float kGap = 8.0f;
    constexpr int kColumns = 7;
    const float left = kW - 24.0f - static_cast<float>(kColumns) * (kSlot + kGap);
    for (int i = 0; i < kColumns * 2; ++i) {
        const int col = i % kColumns;
        const int row = i / kColumns;
        const Rect r{left + static_cast<float>(col) * (kSlot + kGap), kBarTop + 2.0f + static_cast<float>(row) * (kSlot + 6.0f), kSlot, kSlot};
        addUi(list, shapes, Ui::Slot, r.expanded(kit::kMargin), srgb(0xEFE2CA), ui_color::kTitle, 0.0f, 0.0f, 10.0f);
        if (i >= static_cast<int>(keys.size())) {
            continue;
        }
        const QuickKey& k = keys[static_cast<std::size_t>(i)];
        if (k.skill) {
            kit::skillIconAt(list, shapes, r.center(), kSlot * 0.9f, *k.skill);
            const float remaining = c.skills.cooldownProgress(static_cast<int>(*k.skill));
            if (c.skills.cooldownRemaining(static_cast<int>(*k.skill)) > 0) {
                kit::sweep(list, shapes, r, 1.0f - remaining);
            }
            const int cost = game::skillLevelValues(*k.skill, std::max(1, c.skillLevel(*k.skill))).cost;
            if (cost > c.mp.current) {
                addUi(list, shapes, Ui::Sweep, r.expanded(kit::kMargin), srgb(0x000000), srgb(0x6A9AE8), 1.0f, 0.0f, 10.0f);
            }
        } else {
            const int count = c.countOf(k.item);
            kit::slotIcon(list, shapes, r, k.item, 0, count > 0 ? 1.0f : 0.35f);
            const Font f = kit::outlined(17.0f, 1.3f);
            gamelib::draw::drawText(list, f, std::to_string(count), r.right() - 3.0f, r.bottom() - f.height() - 3.0f, srgb(0xFFFFFF),
                                    Align::Right);
        }
        // The key's label in the corner.
        const std::string_view label = gamelib::platform::keyLabel(k.key);
        const Font kf = kit::outlined(14.0f, 1.3f);
        gamelib::draw::drawText(list, kf, label, r.left() + 4.0f, r.top() + 3.0f, srgb(0xFFF2C0));
    }
}

void Hud::drawMinimap(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const int size,
                      const float time) const
{
    if (size == 0) {
        return;
    }
    const game::MapDef& def = session.map();
    if (m_cached != def.id) {
        m_cached = def.id;
        const gamelib::core::TileWorld world{def.tiles, 0, game::tileShapes()};
        m_floors = gamelib::core::floorRuns(world);
        m_climbs = gamelib::core::climbRuns(world);
    }
    const float w = size == 2 ? 600.0f : 380.0f;
    const float h = size == 2 ? 300.0f : 190.0f;
    const Rect frame{16.0f, 16.0f, w, h};
    const Rect content = kit::window(list, shapes, frame, def.name);
    const Rect area = content.expanded(-4.0f);
    addUi(list, shapes, Ui::Well, area.expanded(kit::kMargin + 4.0f), srgb(0x3A4A6A, 0.9f), ui_color::kTitle, 0.0f, 0.0f, 10.0f);
    const gamelib::core::GridLayout layout = gamelib::core::GridLayout::fit(area, def.columns, def.rows, 1e9f, false);
    gamelib::ui::drawTileRuns(list, area, layout, m_floors, srgb(0xB8E890), 3.0f);
    gamelib::ui::drawTileRuns(list, area, layout, m_climbs, srgb(0xE8C890), 2.0f);
    std::vector<gamelib::ui::MapMarker> markers;
    const auto cell = [&](const Vec2 world) { return Vec2{world.x / game::kTile, world.y / game::kTile - 0.6f}; };
    for (const game::Portal& p : def.portals) {
        markers.push_back({.at = cell(p.at), .color = srgb(0x8CD8FF), .shape = gamelib::ui::MarkerShape::Ring, .size = 1.6f});
    }
    for (const game::NpcHere& n : session.npcs()) {
        markers.push_back({.at = cell(n.feet), .color = srgb(0xFFD86A), .shape = gamelib::ui::MarkerShape::Diamond, .size = 1.4f});
    }
    if (session.boss() && session.boss()->alive()) {
        markers.push_back({.at = cell(session.boss()->box.bottomCenter()), .color = srgb(0xFF6A6A), .shape = gamelib::ui::MarkerShape::Square,
                           .size = 2.0f});
    }
    const float pulse = 0.85f + 0.15f * std::sin(time * 6.0f);
    markers.push_back({.at = cell(session.hero().body.box.bottomCenter()), .color = srgb(0xFFF6A0), .shape = gamelib::ui::MarkerShape::Dot,
                       .size = 1.8f * pulse});
    gamelib::ui::drawMapMarkers(list, area, layout, markers);
}

void Hud::drawBuffs(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    // The clock and the day's light, top right.
    const Rect clock{kW - 220.0f, 16.0f, 204.0f, 56.0f};
    addUi(list, shapes, Ui::Plate, clock.expanded(kit::kMargin), srgb(0x2E3A5A, 0.82f), srgb(0xFFFFFF, 0.6f));
    const bool night = session.night();
    addFx(list, shapes, night ? Fx::Glow : Fx::Glow, {clock.left() + 34.0f, clock.center().y}, 44.0f,
          night ? srgb(0xD8E0FF) : srgb(0xFFE070), srgb(0xFFFFFF), 0.0f);
    addIcon(list, shapes, Ui::StatIcon, {clock.left() + 34.0f, clock.center().y}, 34.0f, night ? 2.0f : 10.0f, 0.0f, srgb(0xFFFFFF),
            srgb(0xFFFFFF));
    kit::label(list, timeText(session.clock()), clock.left() + 64.0f, clock.center().y, 24.0f, srgb(0xFFFFFF), Align::Left, 1.5f);
    // Buffs, newest at the right, under the clock.
    const auto& all = session.character().buffs.all();
    float x = kW - 60.0f;
    for (auto it = all.rbegin(); it != all.rend(); ++it) {
        game::SkillId skill = game::SkillId::IronBody;
        switch (it->id) {
        case game::kBuffWarCry: skill = game::SkillId::WarCry; break;
        case game::kBuffManaShield: skill = game::SkillId::ManaShield; break;
        case game::kBuffSwiftwind: skill = game::SkillId::Swiftwind; break;
        default: break;
        }
        const Rect r{x - 22.0f, 86.0f, 44.0f, 44.0f};
        addUi(list, shapes, Ui::Slot, r.expanded(kit::kMargin), srgb(0xEFE2CA), skillColor(skill), 0.5f, 0.0f, 10.0f);
        kit::skillIconAt(list, shapes, r.center(), 40.0f, skill);
        const float left = it->total > 0 ? static_cast<float>(it->remaining) / static_cast<float>(it->total) : 1.0f;
        kit::sweep(list, shapes, r, 1.0f - left);
        const Font f = kit::outlined(15.0f, 1.2f);
        gamelib::draw::drawText(list, f, std::format("{}s", it->remaining / game::kTickRate), r.center().x, r.bottom() + 2.0f, srgb(0xFFFFFF),
                                Align::Center);
        x -= 56.0f;
    }
}

void Hud::drawTracker(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    const std::vector<int> log = session.quests().log();
    std::vector<int> shown;
    for (const int q : log) {
        const auto s = session.quests().stateOf(q);
        if (s == gamelib::core::QuestState::Active || s == gamelib::core::QuestState::Complete) {
            shown.push_back(q);
        }
    }
    if (shown.empty()) {
        return;
    }
    if (shown.size() > 3) {
        shown.erase(shown.begin(), shown.end() - 3);
    }
    const float w = 420.0f;
    float y = 170.0f;
    const Font title = kit::outlined(22.0f, 1.5f);
    const Font line = kit::outlined(19.0f, 1.25f);
    for (const int q : shown) {
        const game::QuestText& t = game::questText(q);
        const bool done = session.quests().stateOf(q) == gamelib::core::QuestState::Complete;
        const float x = kW - 24.0f;
        gamelib::draw::drawText(list, title, t.title, x, y, t.main ? srgb(0xFFE08A) : srgb(0xFFFFFF), Align::Right);
        y += 30.0f;
        const gamelib::core::QuestDef* def = session.quests().find(q);
        std::string progress;
        if (done) {
            progress = "ready to hand in";
        } else if (def != nullptr && !def->objectives.empty()) {
            const auto& o = def->objectives.front();
            if (o.kind == gamelib::core::ObjectiveKind::Talk || o.kind == gamelib::core::ObjectiveKind::Deliver) {
                progress = std::format("see {}", game::npcName(static_cast<game::NpcId>(o.subject)));
            } else {
                std::string what;
                if (o.kind == gamelib::core::ObjectiveKind::Collect) {
                    what = std::string{game::itemName(o.subject)};
                } else if (o.subject >= game::kBossSubject) {
                    what = std::string{game::bossInfo(static_cast<game::BossId>(o.subject - game::kBossSubject)).name};
                } else {
                    what = std::string{game::species(static_cast<game::SpeciesId>(o.subject)).name};
                }
                progress = std::format("{}  {}/{}", what, session.quests().progressOf(q, 0), o.required);
            }
        }
        gamelib::draw::drawText(list, line, progress, x, y, done ? srgb(0x9AF0A0) : srgb(0xE8F0FF), Align::Right);
        y += 36.0f;
    }
    (void)shapes;
    (void)w;
}

void Hud::drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    if (!session.boss() || m_bossShown <= 0.01f) {
        return;
    }
    const game::Boss& b = *session.boss();
    const float slide = gamelib::core::easeOutCubic(m_bossShown);
    const float y = -80.0f + slide * 110.0f;
    const Rect bar{kW * 0.5f - 420.0f, y + 34.0f, 840.0f, 26.0f};
    kit::gauge(list, shapes, bar, static_cast<float>(b.hp) / static_cast<float>(std::max(b.maxHp, 1)), m_boss.value(), srgb(0xE8506A),
               srgb(0xFFD0A0));
    const Font f = kit::outlined(28.0f, 1.6f);
    gamelib::draw::drawText(list, f, std::format("{} - lv {}", b.info().name, b.info().level), kW * 0.5f, y - 2.0f, srgb(0xFFF2E0), Align::Center);
}

void Hud::drawPrompt(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                     const gamelib::platform::ActionMap& actions, const gamelib::core::View2D& view, const float time) const
{
    std::optional<Vec2> at;
    std::string word;
    if (const std::optional<game::NpcId> npc = session.npcInReach()) {
        for (const game::NpcHere& n : session.npcs()) {
            if (n.npc == *npc) {
                at = n.feet + Vec2{0.0f, -205.0f};
            }
        }
        word = "talk";
    } else if (const int portal = session.portalInReach(); portal >= 0) {
        at = session.map().portals[static_cast<std::size_t>(portal)].at + Vec2{0.0f, -280.0f};
        word = session.portalOpen(portal) ? "enter" : "sealed";
    }
    if (!at) {
        return;
    }
    const Vec2 c = view.toCanvas(*at) + Vec2{0.0f, std::sin(time * 4.0f) * 4.0f};
    const std::string key{actions.keyLabel(actionId(Action::Up))};
    kit::keyCap(list, shapes, {c.x - 30.0f, c.y}, key == "UP" ? "^" : key, 36.0f);
    const Font f = kit::outlined(22.0f, 1.4f);
    gamelib::draw::drawText(list, f, word, c.x - 6.0f, f.topFor(c.y), srgb(0xFFFFFF));
}

void Hud::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
               const gamelib::platform::ActionMap& actions, const gamelib::core::View2D& view, const int minimap, const float time) const
{
    drawPrompt(list, shapes, session, actions, view, time);
    drawMinimap(list, shapes, session, minimap, time);
    drawBuffs(list, shapes, session);
    drawTracker(list, shapes, session);
    drawBoss(list, shapes, session);
    gamelib::ui::MessageLogStyle logStyle;
    logStyle.font = kit::outlined(21.0f, 1.3f);
    logStyle.align = Align::Left;
    logStyle.lineHeight = 34.0f;
    logStyle.gap = 6.0f;
    logStyle.padding = 16.0f;
    logStyle.maxShown = 6;
    logStyle.background = srgb(0x1E1A30, 0.45f);
    logStyle.cornerRadius = 14.0f;
    gamelib::ui::drawMessageLog(list, m_log, {20.0f, kBarTop - 24.0f}, logStyle);
    drawStatus(list, shapes, session);
    drawQuickSlots(list, shapes, session, actions, time);
    // The map's title and the banners.
    gamelib::fx::BannerStyle title;
    title.top = 150.0f;
    title.titleFont = kit::outlined(64.0f, 1.8f);
    title.lineFont = kit::outlined(28.0f, 1.4f);
    title.lineAbove = true;
    title.lineGap = 46.0f;
    title.lineColor = srgb(0xFFE8B0);
    title.growFrom = 0.7f;
    title.fadeIn = true;
    title.slide = 20.0f;
    m_title.draw(list, kW * 0.5f, title);
    gamelib::fx::BannerStyle big;
    big.top = 330.0f;
    big.titleFont = kit::outlined(56.0f, 1.9f);
    big.lineFont = kit::outlined(26.0f, 1.4f);
    big.lineGap = 72.0f;
    big.lineColor = srgb(0xFFFFFF);
    big.growFrom = 0.5f;
    m_banners.draw(list, kW * 0.5f, big);
}

} // namespace realm::app
