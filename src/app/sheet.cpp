#include "sheet.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/draw/vector_text.hpp>

#include <array>
#include <cmath>
#include <string_view>

namespace realm::app {

namespace {

// helpers

using anime::Avatar;
using gamelib::core::srgb;
using gamelib::core::Vec2;

Avatar beginner()
{
    Avatar a;
    a.hair = anime::Hair::Sprout;
    a.hairColor = srgb(0x8A5A3C);
    a.top = anime::Top::Tee;
    a.topColor = srgb(0xF7F3E8);
    a.bottom = anime::Bottom::Shorts;
    a.bottomColor = anime::Swatch::Blue;
    a.shoes = anime::Shoes::Shoes;
    a.shoeColor = anime::Swatch::Red;
    a.weapon = anime::Weapon::Branch;
    a.accent = anime::Swatch::Orange;
    a.eyes = 0;
    return a;
}

Avatar warrior()
{
    Avatar a;
    a.hair = anime::Hair::Spiky;
    a.hairColor = srgb(0x3E3A5C);
    a.hat = anime::Hat::Bandana;
    a.hatColor = anime::Swatch::Red;
    a.top = anime::Top::Plate;
    a.topColor = srgb(0xC8D4E8);
    a.bottom = anime::Bottom::Pants;
    a.bottomColor = anime::Swatch::Navy;
    a.shoes = anime::Shoes::Boots;
    a.shoeColor = anime::Swatch::Brown;
    a.gloves = anime::Gloves::Cloth;
    a.weapon = anime::Weapon::Sword;
    a.cape = anime::Cape::Cape;
    a.capeColor = anime::Swatch::Red;
    a.accent = anime::Swatch::Gold;
    a.eyes = 2;
    a.skin = 1;
    return a;
}

Avatar mage()
{
    Avatar a;
    a.hair = anime::Hair::Long;
    a.hairColor = srgb(0xF2D27A);
    a.hat = anime::Hat::Wizard;
    a.hatColor = anime::Swatch::Purple;
    a.top = anime::Top::Robe;
    a.topColor = srgb(0x7E62D8);
    a.bottom = anime::Bottom::Skirt;
    a.bottomColor = anime::Swatch::Navy;
    a.shoes = anime::Shoes::Shoes;
    a.shoeColor = anime::Swatch::Plum;
    a.weapon = anime::Weapon::Staff;
    a.accent = anime::Swatch::Sky;
    a.eyes = 3;
    return a;
}

void label(gamelib::draw::DrawList& list, const std::string_view text, const float x, const float y, const float size = 30.0f)
{
    gamelib::draw::drawVectorText(list, text, x, y,
                                  {.size = size, .color = srgb(0x4E3D33), .weight = 1.3f, .align = gamelib::draw::Align::Center});
}

void put(gamelib::draw::DrawList& list, const Shapes& shapes, Avatar a, const Vec2 feet, const float height)
{
    a.feet = feet;
    a.height = height;
    list.add(anime::avatarSprite(a, shapes.avatar));
}

} // namespace

void drawSheet(gamelib::draw::DrawList& list, const Shapes& shapes, const gamelib::core::Size canvas, const int page, const float time)
{
    const float w = static_cast<float>(canvas.width);
    const float h = static_cast<float>(canvas.height);
    list.vignette({0.0f, 0.0f, w, h}, srgb(0xCDEBFA), 1.15f);
    using P = anime::AvatarPose;
    if (page == 0) {
        label(list, "realm - the cast and the jobs", w * 0.5f, 30.0f, 40.0f);
        const std::array<Avatar, 3> jobs{beginner(), warrior(), mage()};
        const std::array<std::string_view, 3> names{"beginner", "warrior", "mage"};
        for (int i = 0; i < 3; ++i) {
            const float x = w * (0.18f + 0.32f * static_cast<float>(i));
            Avatar a = jobs[static_cast<std::size_t>(i)];
            a.seed = static_cast<std::uint8_t>(i * 70);
            put(list, shapes, a, {x - 90.0f, 560.0f}, 460.0f);
            Avatar b = a;
            b.facing = -1.0f;
            b.pose = P::Walk;
            b.phase = std::fmod(time * 1.3f, 1.0f);
            put(list, shapes, b, {x + 150.0f, 560.0f}, 230.0f);
            label(list, names[static_cast<std::size_t>(i)], x, 590.0f);
        }
        // Hair styles at game size.
        for (int i = 0; i < anime::kHairStyles; ++i) {
            Avatar a = beginner();
            a.hair = static_cast<anime::Hair>(i);
            static constexpr std::array<std::uint32_t, 8> kHair{0x8A5A3C, 0xE07A9A, 0x3E3A5C, 0xF2D27A, 0x6A9AE8, 0xE8763A, 0x5AAE7A, 0xB58AE0};
            a.hairColor = srgb(kHair[static_cast<std::size_t>(i)]);
            a.eyes = static_cast<std::uint8_t>(i);
            a.skin = static_cast<std::uint8_t>(i % 4);
            a.seed = static_cast<std::uint8_t>(i * 31);
            a.weapon = anime::Weapon::None;
            a.expression = i % 3 == 1 ? anime::Expression::Happy : anime::Expression::Neutral;
            put(list, shapes, a, {w * (0.08f + 0.12f * static_cast<float>(i)), 1000.0f}, 300.0f);
        }
        put(list, shapes, beginner(), {w - 60.0f, 700.0f}, 158.0f);
    } else if (page == 1) {
        label(list, "poses", w * 0.5f, 30.0f, 40.0f);
        const std::array<Avatar, 3> jobs{beginner(), warrior(), mage()};
        const std::array<P, 11> poses{P::Idle, P::Walk, P::Jump, P::Swing, P::Hurt, P::Cheer, P::Climb, P::Cast, P::Prone, P::Throw, P::Sit};
        const std::array<std::string_view, 11> names{"idle", "walk", "jump", "swing", "hurt", "cheer", "climb", "cast", "prone", "throw", "sit"};
        for (int j = 0; j < 3; ++j) {
            for (int i = 0; i < 11; ++i) {
                Avatar a = jobs[static_cast<std::size_t>(j)];
                a.pose = poses[static_cast<std::size_t>(i)];
                a.phase = std::fmod(time * 0.9f, 1.0f);
                if (a.pose == P::Jump) {
                    a.phase = 0.5f + 0.5f * std::sin(time * 2.0f);
                }
                put(list, shapes, a, {90.0f + 170.0f * static_cast<float>(i), 330.0f + 330.0f * static_cast<float>(j)}, 250.0f);
                if (j == 0) {
                    label(list, names[static_cast<std::size_t>(i)], 90.0f + 170.0f * static_cast<float>(i), 80.0f, 26.0f);
                }
            }
        }
    } else {
        label(list, "expressions", w * 0.5f, 30.0f, 40.0f);
        const std::array<std::string_view, 9> names{"neutral", "happy", "angry", "sad", "surprised", "hurt", "blink", "determined", "knocked"};
        for (int i = 0; i < 9; ++i) {
            Avatar a = i % 3 == 0 ? beginner() : i % 3 == 1 ? warrior() : mage();
            a.expression = static_cast<anime::Expression>(i);
            a.nextExpression = a.expression;
            a.seed = 200; // no blink in the shot
            put(list, shapes, a, {110.0f + 212.0f * static_cast<float>(i), 640.0f}, 420.0f);
            label(list, names[static_cast<std::size_t>(i)], 110.0f + 212.0f * static_cast<float>(i), 680.0f, 28.0f);
        }
    }
}

} // namespace realm::app
