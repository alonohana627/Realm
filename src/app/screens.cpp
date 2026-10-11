// The screens around the world: the title over the demo bot's meadow, the
// pause and settings menus in Realm's soft windows, character select with
// each character standing in its gear, the character creator, the voyage at
// sea, fainting and the ending with the journey's numbers and the credits.

#include "realm_game.hpp"

#include "look.hpp"
#include "ui_kit.hpp"

#include <gamelib/core/easing.hpp>
#include <gamelib/core/timers.hpp>
#include <gamelib/draw/parallax.hpp>
#include <gamelib/ui/menu.hpp>
#include <gamelib/ui/text_field.hpp>

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
namespace layers = gamelib::draw::layers;

constexpr float kW = 1920.0f;
constexpr float kH = 1080.0f;
constexpr float kAvatarQuad = 158.0f;

constexpr std::array<std::string_view, 8> kHairNames{"sprout", "bob", "ponytail", "long", "buns", "spiky", "braid", "curly"};
constexpr std::array<std::string_view, 8> kEyeNames{"blue", "green", "amber", "violet", "ruby", "teal", "brown", "rose"};
constexpr std::array<std::string_view, 4> kSkinNames{"fair", "warm", "tan", "deep"};
constexpr std::array<std::uint32_t, 8> kEyeHex{0x3A7CE8, 0x2BA86A, 0xD8892C, 0x9058E0, 0xD8384A, 0x18A8B8, 0x7A4A34, 0xC0408A};
constexpr std::array<std::uint32_t, 4> kSkinHex{0xFFE9DC, 0xFAD6BC, 0xE0AE88, 0xA87452};

void veil(gamelib::draw::DrawList& list, const float alpha)
{
    list.box({0.0f, 0.0f, kW, kH}, srgb(0x1E1A38, alpha));
}

std::string lowerCase(std::string_view s)
{
    std::string out;
    for (const char c : s) {
        out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

// A soft round stage under a standing character.
void pedestal(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 feet, const float width)
{
    addUi(list, shapes, Ui::Shadow, Rect{feet.x - width * 0.5f, feet.y - 26.0f, width, 52.0f}.expanded(20.0f), srgb(0xFFFFFF, 0.9f),
          srgb(0xFFFFFF));
}

anime::Avatar standing(const game::Character& c, const Vec2 feet, const float height, const float time, const int seed)
{
    anime::Avatar a = heroAvatar(c);
    a.feet = feet;
    a.height = height;
    a.phase = time;
    a.seed = static_cast<std::uint8_t>(seed);
    a.expression = anime::Expression::Neutral;
    a.nextExpression = anime::Expression::Neutral;
    return a;
}

} // namespace

void RealmGame::drawStage(gamelib::draw::DrawList& list, const float time) const
{
    const game::Session& demo = m_attract.sim().session;
    m_demoView.draw(list, m_shapes, demo, m_demoEffects, 1.0f, time, {});
    list.setLayer(layers::Hud);
}

void RealmGame::drawTitle(gamelib::draw::DrawList& list, const float time) const
{
    drawStage(list, time);
    if (m_demoMode) {
        kit::label(list, "demo - press any key", kW * 0.5f, 60.0f, 30.0f, srgb(0xFFFFFF), Align::Center, 1.6f);
        return;
    }
    // A soft gradient so the logo reads over the sky.
    for (int i = 0; i < 8; ++i) {
        list.box({0.0f, static_cast<float>(i) * 50.0f, kW, 50.0f}, srgb(0x2A2248, 0.32f * (1.0f - static_cast<float>(i) / 8.0f)));
    }
    const float bob = std::sin(time * 1.4f) * 6.0f;
    const Vec2 logo{kW * 0.5f, 250.0f + bob};
    addUi(list, m_shapes, Ui::Logo, Rect{logo.x - 420.0f, logo.y + 92.0f - 76.0f, 840.0f, 152.0f}, srgb(0x7CC2E4), srgb(0xFFFFFF));
    // The name: a cream fill over a soft plum outline and a deeper drop.
    const Font drop = Font::vector(170.0f, 3.4f).withShadow({.color = srgb(0x2A2050, 0.55f), .offset = {0.0f, 12.0f}, .weight = 6.0f});
    gamelib::draw::drawText(list, drop, "Realm", logo.x, drop.topFor(logo.y - 10.0f), srgb(0x000000, 0.0f), Align::Center);
    const Font big = Font::vector(170.0f, 3.4f).withShadow({.color = srgb(0x6A4A9A), .offset = {0.0f, 0.0f}, .weight = 5.2f});
    gamelib::draw::drawText(list, big, "Realm", logo.x, big.topFor(logo.y - 10.0f), srgb(0xFFF4D8), Align::Center);
    // Petals drift across the logo.
    for (int i = 0; i < 6; ++i) {
        const float t = std::fmod(time * 0.25f + static_cast<float>(i) * 0.37f, 1.0f);
        const Vec2 p{logo.x - 420.0f + t * 840.0f, logo.y - 90.0f + std::sin(t * 9.0f + static_cast<float>(i)) * 40.0f + static_cast<float>(i) * 18.0f};
        addFx(list, m_shapes, Fx::Petal, p, 34.0f, srgb(0xFFB8D0), srgb(0xF27AA0), t, static_cast<float>(i), 0.0f, t * 6.0f);
    }
    const Font ribbon = kit::outlined(30.0f, 1.5f);
    gamelib::draw::drawText(list, ribbon, "a cozy adventure across five lands", logo.x, ribbon.topFor(logo.y + 92.0f), srgb(0xFFFFFF),
                            Align::Center);
    kit::label(list, "arrows choose    enter picks    f11 fullscreen", kW * 0.5f, kH - 40.0f, 22.0f, srgb(0xFFFFFF, 0.85f), Align::Center,
               1.4f);
}

void RealmGame::drawMenu(gamelib::draw::DrawList& list, const float time) const
{
    const gamelib::ui::Menu* menu = m_menus.menu();
    if (menu == nullptr || m_demoMode) {
        return;
    }
    (void)time;
    list.setLayer(layers::Hud + 60);
    const bool title = m_mode == Mode::Title;
    gamelib::ui::MenuStyle style;
    style.veil = title ? srgb(0x000000, 0.0f) : srgb(0x1E1A38, 0.45f);
    style.titleFont = kit::titleFont(36.0f);
    style.itemFont = Font::vector(30.0f, 1.35f).lowered();
    style.footerFont = Font::vector(20.0f, 1.2f).lowered();
    style.text = ui_color::kInk;
    style.highlight = ui_color::kInk;
    style.dim = ui_color::kSoftInk;
    style.title = srgb(0xFFFFFF);
    style.valueHighlight = srgb(0xD07A1E);
    style.itemHeight = 60.0f;
    style.padding = 40.0f;
    style.minWidth = 520.0f;
    style.titleHeight = title ? 0.0f : 56.0f;
    style.titleGap = 0.0f;
    style.top = title ? 520.0f : -1.0f;
    style.rowInset = 26.0f;
    style.labelIndent = 50.0f;
    style.panelPainter = [this, title](gamelib::draw::DrawList& l, const Rect& r) {
        addUi(l, m_shapes, Ui::Window, r.expanded(kit::kMargin), ui_color::kWindow.withAlpha(title ? 0.94f : 1.0f), ui_color::kTitle,
              title ? 0.0f : 56.0f, 0.0f, 24.0f);
    };
    style.rowPainter = [this](gamelib::draw::DrawList& l, const Rect& row, std::size_t, const bool selected) {
        if (selected) {
            addUi(l, m_shapes, Ui::Button, row.expanded(kit::kMargin - 4.0f), ui_color::kButton, ui_color::kGold, 1.0f);
        }
    };
    style.titlePainter = [title](gamelib::draw::DrawList& l, const Rect& r, const std::string_view text) {
        if (title) {
            return;
        }
        const Font f = kit::titleFont(32.0f);
        gamelib::draw::drawText(l, f, lowerCase(text), r.center().x, f.topFor(r.y + 28.0f), srgb(0xFFFFFF), Align::Center);
    };
    gamelib::ui::drawMenu(list, *menu, m_canvas, style);
}

void RealmGame::drawSelect(gamelib::draw::DrawList& list, const float time) const
{
    drawStage(list, time);
    veil(list, 0.42f);
    const Font head = kit::titleFont(48.0f);
    gamelib::draw::drawText(list, head, "choose your adventurer", kW * 0.5f, head.topFor(130.0f), srgb(0xFFFFFF), Align::Center);
    constexpr float kCardW = 460.0f;
    constexpr float kCardH = 640.0f;
    for (int i = 0; i < kSlotCount; ++i) {
        const SlotCard& card = m_cards[static_cast<std::size_t>(i)];
        const bool selected = i == m_slot;
        const float pop = gamelib::core::easeOutBack(std::clamp(m_selectAge * 3.0f - static_cast<float>(i) * 0.3f, 0.0f, 1.0f));
        const float lift = selected ? -18.0f + std::sin(time * 2.0f) * 3.0f : 0.0f;
        const Rect r{kW * 0.5f + (static_cast<float>(i) - 1.0f) * (kCardW + 60.0f) - kCardW * 0.5f, 220.0f + lift + (1.0f - pop) * 80.0f, kCardW,
                     kCardH};
        const std::string title = card.data ? card.data->character.look.name : std::format("slot {}", i + 1);
        const Rect inner = kit::window(list, m_shapes, r, title, selected ? srgb(0xF2B33D) : ui_color::kTitle, pop);
        const Rect stage{inner.x, inner.y, inner.width, 360.0f};
        addUi(list, m_shapes, Ui::Well, stage.expanded(kit::kMargin), selected ? srgb(0xFFF4DA) : srgb(0xF2E8D6), ui_color::kTitle, 0.0f, 0.0f,
              18.0f);
        const Vec2 feet{stage.center().x, stage.bottom() - 40.0f};
        if (card.data) {
            const game::Character& c = card.data->character;
            pedestal(list, m_shapes, feet, 200.0f);
            anime::Avatar a = standing(c, feet, 400.0f, time + static_cast<float>(i), 120 + i);
            if (selected) {
                a.pose = std::fmod(time, 4.0f) < 1.0f ? anime::AvatarPose::Cheer : anime::AvatarPose::Idle;
            }
            list.add(anime::avatarSprite(a, m_shapes.avatar));
            float y = stage.bottom() + 34.0f;
            kit::label(list, std::format("level {} {}", c.level, lowerCase(game::jobName(c.job))), inner.center().x, y, 28.0f, ui_color::kInk,
                       Align::Center, 1.6f);
            y += 44.0f;
            kit::label(list, game::mapDef(card.data->map).name, inner.center().x, y, 23.0f, ui_color::kSoftInk, Align::Center);
            y += 38.0f;
            const std::int64_t ticks = std::min<std::int64_t>(card.data->stats.ticks, 359'999LL * game::kTickRate);
            kit::label(list, std::format("played {}", gamelib::core::formatClock(static_cast<int>(ticks), game::kTickRate, 0)), inner.center().x,
                       y, 21.0f, ui_color::kSoftInk, Align::Center);
            if (card.data->finished) {
                y += 36.0f;
                kit::label(list, "the realm is bright again", inner.center().x, y, 21.0f, srgb(0xD08A1E), Align::Center, 1.5f);
            }
        } else if (card.exists) {
            kit::label(list, "this save could not be read", inner.center().x, stage.center().y, 24.0f, ui_color::kBad, Align::Center);
        } else {
            // An empty slot: a soft silhouette and a plus.
            anime::Avatar a = baseAvatar({});
            a.feet = feet;
            a.height = 400.0f;
            a.ghost = true;
            a.topColor = srgb(0xFFFFFF, 0.55f);
            a.phase = time;
            list.add(anime::avatarSprite(a, m_shapes.avatar));
            kit::label(list, "+", stage.center().x, stage.center().y - 20.0f, 90.0f, srgb(0xD08A1E, 0.9f), Align::Center, 2.2f);
            kit::label(list, "new adventurer", inner.center().x, stage.bottom() + 40.0f, 28.0f, ui_color::kInk, Align::Center, 1.6f);
            kit::label(list, "create a character", inner.center().x, stage.bottom() + 84.0f, 22.0f, ui_color::kSoftInk, Align::Center);
        }
        if (selected && m_eraseAsked) {
            kit::button(list, m_shapes, {inner.x + 20.0f, inner.bottom() - 58.0f, inner.width - 40.0f, 50.0f}, "press del again to erase", 1, 22.0f,
                        srgb(0xFFD0D0));
        } else if (selected) {
            kit::button(list, m_shapes, {inner.x + 60.0f, inner.bottom() - 58.0f, inner.width - 120.0f, 50.0f},
                        card.data ? "play" : (card.exists ? "make new" : "create"), 1, 26.0f);
        }
    }
    kit::label(list, "left/right choose    enter play    del erase    esc back", kW * 0.5f, kH - 60.0f, 24.0f, srgb(0xFFFFFF, 0.9f),
               Align::Center, 1.4f);
}

void RealmGame::drawCreate(gamelib::draw::DrawList& list, const float time) const
{
    drawStage(list, time);
    veil(list, 0.45f);
    const Creator& c = m_creator;
    const Font head = kit::titleFont(48.0f);
    gamelib::draw::drawText(list, head, "a new adventurer", kW * 0.5f, head.topFor(110.0f), srgb(0xFFFFFF), Align::Center);
    // The preview: big, on a stage, cycling a few poses.
    const Rect stage{1010.0f, 190.0f, 640.0f, 720.0f};
    addUi(list, m_shapes, Ui::Well, stage.expanded(kit::kMargin), srgb(0xFFF6E2), ui_color::kTitle, 0.0f, 0.0f, 24.0f);
    const Vec2 feet{stage.center().x, stage.bottom() - 80.0f};
    pedestal(list, m_shapes, feet, 320.0f);
    game::Character preview;
    preview.look = c.look;
    anime::Avatar a = standing(preview, feet, 760.0f, time, 77);
    const float cycle = std::fmod(c.age, 9.0f);
    if (cycle > 3.0f && cycle < 4.2f) {
        a.pose = anime::AvatarPose::Cheer;
    } else if (cycle > 6.0f && cycle < 7.6f) {
        a.pose = anime::AvatarPose::Walk;
        a.phase = time * 1.4f;
    }
    list.add(anime::avatarSprite(a, m_shapes.avatar));
    // The rows.
    const Rect panel{270.0f, 190.0f, 680.0f, 720.0f};
    const Rect inner = kit::window(list, m_shapes, panel, "your look");
    const std::array<std::string_view, 5> labels{"name", "hair", "hair color", "eyes", "skin"};
    for (int row = 0; row < 5; ++row) {
        const Rect r{inner.x, inner.y + 14.0f + static_cast<float>(row) * 92.0f, inner.width, 74.0f};
        const bool selected = row == c.row;
        if (selected) {
            addUi(list, m_shapes, Ui::Button, r.expanded(kit::kMargin - 2.0f), ui_color::kButton, ui_color::kGold, 1.0f);
        }
        kit::label(list, labels[static_cast<std::size_t>(row)], r.x + 30.0f, r.center().y, 27.0f, ui_color::kSoftInk, Align::Left,
                   selected ? 1.5f : 1.3f);
        const float vx = r.x + 400.0f;
        if (selected && row > 0) {
            const float nudge = std::sin(time * 6.0f) * 3.0f;
            kit::label(list, "<", vx - 150.0f - nudge, r.center().y, 30.0f, srgb(0xD08A1E), Align::Center, 2.0f);
            kit::label(list, ">", vx + 150.0f + nudge, r.center().y, 30.0f, srgb(0xD08A1E), Align::Center, 2.0f);
        }
        switch (row) {
        case 0: {
            // GameLib's text field in a soft well: the caret blinks while the row
            // is chosen, a suggestion shows selected until a key replaces it.
            const Rect box{vx - 170.0f, r.center().y - 30.0f, 340.0f, 60.0f};
            kit::well(list, m_shapes, box);
            gamelib::ui::TextFieldStyle style;
            style.font = kit::font(34.0f, 1.7f);
            style.fill = srgb(0x000000, 0.0f);
            style.borderWidth = 0.0f;
            style.text = ui_color::kInk;
            style.selection = srgb(0xFFD88A, 0.85f);
            style.caret = selected ? srgb(0xD08A1E) : srgb(0xD08A1E, 0.0f);
            gamelib::ui::drawTextField(list, c.name, box, style, time);
            break;
        }
        case 1: kit::label(list, kHairNames[static_cast<std::size_t>(c.look.hair)], vx, r.center().y, 29.0f, ui_color::kInk, Align::Center, 1.5f); break;
        case 2: {
            list.circle({vx - 70.0f, r.center().y}, 22.0f, srgb(0x3A2E4A, 0.7f));
            list.circle({vx - 70.0f, r.center().y}, 19.0f, hairColor(c.look.hairColor));
            kit::label(list, std::format("{} of {}", c.look.hairColor + 1, kHairColors), vx + 20.0f, r.center().y, 25.0f, ui_color::kInk, Align::Center);
            break;
        }
        case 3: {
            list.circle({vx - 80.0f, r.center().y}, 22.0f, srgb(0x3A2E4A, 0.7f));
            list.circle({vx - 80.0f, r.center().y}, 19.0f, srgb(kEyeHex[static_cast<std::size_t>(c.look.eyes)]));
            kit::label(list, kEyeNames[static_cast<std::size_t>(c.look.eyes)], vx + 20.0f, r.center().y, 27.0f, ui_color::kInk, Align::Center, 1.5f);
            break;
        }
        case 4: {
            list.circle({vx - 80.0f, r.center().y}, 22.0f, srgb(0x3A2E4A, 0.7f));
            list.circle({vx - 80.0f, r.center().y}, 19.0f, srgb(kSkinHex[static_cast<std::size_t>(c.look.skin)]));
            kit::label(list, kSkinNames[static_cast<std::size_t>(c.look.skin)], vx + 20.0f, r.center().y, 27.0f, ui_color::kInk, Align::Center, 1.5f);
            break;
        }
        default: break;
        }
    }
    const Rect start{inner.x + 70.0f, inner.bottom() - 84.0f, inner.width - 140.0f, 66.0f};
    kit::button(list, m_shapes, start, "begin the adventure", c.row == 5 ? 1 : 0, 30.0f, c.row == 5 ? srgb(0xFFE7B8) : srgb(0xF6EEDC));
    kit::label(list, c.row == 0 ? "type a name    left/right: suggestions    backspace: erase" : "up/down choose    left/right change    enter: next",
               kW * 0.5f, kH - 60.0f, 24.0f, srgb(0xFFFFFF, 0.9f), Align::Center, 1.4f);
}

void RealmGame::drawVoyage(gamelib::draw::DrawList& list, const float time) const
{
    // The coast's sky and sea scroll by; the ship rocks in the middle with
    // the hero at the rail and Captain Marlo at the wheel.
    const game::MapDef& coast = game::mapDef(game::MapId::SaltwhistleDocks);
    const float night = m_session ? m_session->nightAmount() : 0.0f;
    const RegionLook look = regionLook(coast, night);
    const auto bits = static_cast<std::uint64_t>(1); // the sea scene
    const gamelib::core::View2D camera{.origin = {m_voyageTime * 160.0f, 0.0f}, .zoom = 1.0f};
    const std::array layersList{
        gamelib::draw::ParallaxLayer{.shape = m_shapes.backdrop, .factor = {0.0f, 0.0f}, .color = look.skyHorizon, .color2 = look.skyTop,
                                     .params = {0.0f, night}, .bits = bits},
        gamelib::draw::ParallaxLayer{.shape = m_shapes.backdrop, .factor = {0.08f, 0.0f}, .color = look.cloud, .color2 = look.cloudShade,
                                     .params = {1.0f, night}, .bits = bits},
        gamelib::draw::ParallaxLayer{.shape = m_shapes.backdrop, .factor = {0.2f, 0.0f}, .color = look.far, .color2 = look.farRim,
                                     .params = {2.0f, night}, .bits = bits},
        gamelib::draw::ParallaxLayer{.shape = m_shapes.backdrop, .factor = {0.5f, 0.0f}, .color = look.near, .color2 = look.nearRim,
                                     .params = {3.0f, night}, .bits = bits},
    };
    list.setLayer(layers::Background);
    gamelib::draw::drawParallax(list, layersList, camera, m_canvas);
    list.setLayer(layers::Hud);
    // The sea in front: soft bands of waves.
    const float scale = 1.5f;
    const Vec2 ground{kW * 0.5f, 900.0f};
    for (int band = 0; band < 3; ++band) {
        const float y = 820.0f + static_cast<float>(band) * 70.0f;
        list.box({0.0f, y, kW, kH - y}, srgb(band == 0 ? 0x5EB8E0 : band == 1 ? 0x4AA4D4 : 0x3A8CC4, 0.75f));
        for (int i = 0; i < 12; ++i) {
            const float x = std::fmod(static_cast<float>(i) * 190.0f - m_voyageTime * (60.0f + static_cast<float>(band) * 50.0f) + 4000.0f, kW + 200.0f) - 100.0f;
            addFx(list, m_shapes, Fx::Spark, {x, y + 8.0f + std::sin(time * 2.0f + static_cast<float>(i)) * 4.0f}, 26.0f, srgb(0xFFFFFF, 0.8f),
                  srgb(0xBFEAFF), std::fmod(time * 0.7f + static_cast<float>(i) * 0.13f, 1.0f));
        }
    }
    const struct {
        float width;
        float height;
    } boat{640.0f * scale, 540.0f * scale};
    const float rock = std::sin(time * 1.2f) * 6.0f;
    list.add(decoSprite(m_shapes, static_cast<int>(game::DecoKind::Boat), ground + Vec2{0.0f, rock * 0.3f}, boat.width, boat.height, srgb(0x3D6CB8),
                        srgb(0xEC5662), night, 0.0f, 1.0f));
    const float deck = ground.y - 34.0f * (boat.height / 128.0f) + 6.0f;
    if (m_session) {
        anime::Avatar hero = standing(m_session->character(), {ground.x + 70.0f, deck + rock * 0.3f}, kAvatarQuad * scale * 1.1f, time, 9);
        list.add(anime::avatarSprite(hero, m_shapes.avatar));
    }
    anime::Avatar captain = npcAvatar(game::NpcId::Marlo);
    captain.feet = {ground.x - 120.0f, deck + rock * 0.3f};
    captain.height = kAvatarQuad * scale * 1.1f;
    captain.phase = time;
    captain.facing = 1.0f;
    list.add(anime::avatarSprite(captain, m_shapes.avatar));
    // Gulls wheel overhead.
    for (int i = 0; i < 3; ++i) {
        const float t = time * 0.5f + static_cast<float>(i) * 2.1f;
        const Vec2 p{kW * 0.5f + std::cos(t) * (420.0f + static_cast<float>(i) * 90.0f), 260.0f + std::sin(t * 1.7f) * 50.0f + static_cast<float>(i) * 40.0f};
        addFx(list, m_shapes, Fx::Leaf, p, 40.0f, srgb(0xFFFFFF), srgb(0x8AA0C0), std::fmod(t, 1.0f), 1.0f, 0.0f, std::sin(t) * 0.4f);
    }
    const std::string where = m_session ? std::string{game::mapDef(m_session->travelTarget()).name} : std::string{};
    const Font f = kit::outlined(40.0f, 1.7f);
    gamelib::draw::drawText(list, f, "sailing to " + lowerCase(where), kW * 0.5f, f.topFor(140.0f), srgb(0xFFFFFF), Align::Center);
    if (m_voyageTime > 1.0f) {
        kit::label(list, "enter: skip ahead", kW * 0.5f, kH - 40.0f, 22.0f, srgb(0xFFFFFF, 0.8f), Align::Center, 1.4f);
    }
}

void RealmGame::drawFainted(gamelib::draw::DrawList& list, const float time) const
{
    const float a = std::min(m_deadTime / 1.2f, 1.0f);
    list.setLayer(layers::Hud + 30);
    list.box({0.0f, 0.0f, kW, kH}, srgb(0x2A2244, 0.45f * a));
    const Font f = kit::outlined(56.0f, 2.0f);
    gamelib::draw::drawText(list, f, "you fainted...", kW * 0.5f, f.topFor(kH * 0.42f + std::sin(time * 2.0f) * 4.0f), srgb(0xFFFFFF, a),
                            Align::Center);
    kit::label(list, "you'll wake up safe in town", kW * 0.5f, kH * 0.42f + 70.0f, 26.0f, srgb(0xFFFFFF, 0.85f * a), Align::Center, 1.4f);
}

void RealmGame::drawEnding(gamelib::draw::DrawList& list, const float time) const
{
    // The world stays behind: Petalbrook in the morning with the friends
    // from the journey gathered, petals and lanterns rising.
    if (m_session) {
        const Vec2 shake{};
        m_view.draw(list, m_shapes, *m_session, m_effects, 1.0f, time, shake);
    }
    list.setLayer(layers::Hud);
    veil(list, 0.35f);
    const float t = m_endingTime;
    for (int i = 0; i < 18; ++i) {
        const float k = std::fmod(t * 0.08f + static_cast<float>(i) * 0.057f, 1.0f);
        const Vec2 p{80.0f + static_cast<float>((i * 211) % 1760), kH + 40.0f - k * (kH + 120.0f)};
        addFx(list, m_shapes, i % 3 == 0 ? Fx::Glow : Fx::Petal, p + Vec2{std::sin(t + static_cast<float>(i)) * 30.0f, 0.0f}, i % 3 == 0 ? 50.0f : 34.0f,
              i % 3 == 0 ? srgb(0xFFE6A0) : srgb(0xFFB8D0), i % 3 == 0 ? srgb(0xFFB040) : srgb(0xF27AA0), k, static_cast<float>(i), 0.0f, k * 8.0f);
    }
    // The cast.
    const std::array<game::NpcId, 6> friends{game::NpcId::NanaRue, game::NpcId::Pip, game::NpcId::Quill, game::NpcId::Kestrel,
                                             game::NpcId::Wren, game::NpcId::Oakhart};
    const float groundY = 860.0f;
    for (std::size_t i = 0; i < friends.size(); ++i) {
        anime::Avatar a = npcAvatar(friends[i]);
        const float side = i < 3 ? -1.0f : 1.0f;
        a.feet = {kW * 0.5f + side * (220.0f + static_cast<float>(i % 3) * 150.0f), groundY};
        a.height = kAvatarQuad * 1.6f;
        a.facing = -side;
        a.phase = time + static_cast<float>(i);
        a.pose = std::fmod(time + static_cast<float>(i) * 0.7f, 3.0f) < 0.8f ? anime::AvatarPose::Cheer : anime::AvatarPose::Idle;
        list.add(anime::avatarSprite(a, m_shapes.avatar));
    }
    if (m_session) {
        anime::Avatar hero = standing(m_session->character(), {kW * 0.5f, groundY}, kAvatarQuad * 1.9f, time, 3);
        hero.pose = std::fmod(time, 2.5f) < 1.0f ? anime::AvatarPose::Cheer : anime::AvatarPose::Idle;
        list.add(anime::avatarSprite(hero, m_shapes.avatar));
    }
    // The heading and the journey's numbers.
    const float in = std::clamp(t / 1.5f, 0.0f, 1.0f);
    const Font head = Font::vector(84.0f, 2.6f).withShadow({.color = srgb(0x3A2E5A, 0.9f), .offset = {0.0f, 4.0f}, .weight = 5.5f});
    gamelib::draw::drawText(list, head, "the realm is bright again", kW * 0.5f, head.topFor(120.0f), srgb(0xFFF4D8, in), Align::Center);
    const float statsIn = std::clamp((t - 1.5f) / 1.0f, 0.0f, 1.0f);
    const Rect panel{kW * 0.5f - 430.0f, 200.0f, 860.0f, 330.0f};
    if (statsIn > 0.0f) {
        const Rect inner = kit::window(list, m_shapes, panel, "your journey", ui_color::kTitle, statsIn);
        const std::int64_t ticks = std::min<std::int64_t>(m_endStats.ticks, 359'999LL * game::kTickRate);
        const std::array<std::pair<std::string, std::string>, 8> rows{{
            {"time", gamelib::core::formatClock(static_cast<int>(ticks), game::kTickRate, 0) + (m_newBest ? "  best!" : "")},
            {"level", std::to_string(m_endLevel)},
            {"monsters", kit::groupDigits(m_endStats.defeated)},
            {"area bosses", std::to_string(m_endStats.bosses)},
            {"quests done", std::to_string(m_endStats.questsDone)},
            {"best hit", kit::groupDigits(m_endStats.bestHit)},
            {"coins earned", kit::groupDigits(m_endStats.coinsEarned)},
            {"fainted", std::to_string(m_endStats.deaths)},
        }};
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const float x = inner.x + 30.0f + static_cast<float>(i % 2) * inner.width * 0.5f;
            const float y = inner.y + 30.0f + static_cast<float>(i / 2) * 58.0f;
            kit::label(list, rows[i].first, x, y, 25.0f, ui_color::kSoftInk.withAlpha(statsIn));
            kit::label(list, rows[i].second, x + inner.width * 0.5f - 70.0f, y, 28.0f, ui_color::kInk.withAlpha(statsIn), Align::Right, 1.6f);
        }
    }
    // The credits roll along the bottom.
    const std::array<std::string_view, 7> credits{
        "Realm", "a game made with GameLib", "everything drawn by shaders, every sound synthesized",
        "the townsfolk of Petal Isle, Saltwhistle, Mossbough and Scout's Camp", "thank you for playing", "the world is still yours to roam",
        "press enter to keep adventuring"};
    const float creditsT = std::max(0.0f, t - 3.0f);
    const auto line = static_cast<std::size_t>(std::min(creditsT / 3.2f, static_cast<float>(credits.size() - 1)));
    const float fade = line + 1 < credits.size() ? std::clamp(1.0f - std::abs(std::fmod(creditsT, 3.2f) - 1.6f) / 1.6f, 0.0f, 1.0f) * 1.6f : 1.0f;
    if (t > 3.0f) {
        const Font f = kit::outlined(34.0f, 1.6f);
        gamelib::draw::drawText(list, f, credits[line], kW * 0.5f, f.topFor(kH - 70.0f), srgb(0xFFFFFF, std::min(fade, 1.0f)), Align::Center);
    }
}

} // namespace realm::app
