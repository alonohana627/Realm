#pragma once

// The C++ side of Realm's anime shaders: packs each shader's fields into a
// draw::Sprite exactly as the shader unpacks them (see the shaders' headers).
// Grown from GameLib's shaders/samples/anime/anime_samples.hpp.

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/draw/sprite.hpp>

#include <algorithm>
#include <cstdint>

namespace realm::app::anime {

// Indices into kAnimePalette (gamelib/anime.glsl).
enum class Swatch : std::uint8_t {
    Cream, Plum, Red, Orange, Yellow, Leaf, Forest, Sky, Blue, Navy, Purple, Pink, Brown, Tan, Slate, Gold
};

// The face kit's named expressions (kAnime* in gamelib/anime_face.glsl).
enum class Expression : std::uint8_t { Neutral, Happy, Angry, Sad, Surprised, Hurt, Blink, Determined, Knocked };

// ---------------------------------------------------------------------------
// avatar.frag

enum class AvatarPose : std::uint8_t { Idle, Walk, Jump, Swing, Hurt, Cheer, Climb, Cast, Prone, Throw, Sit };
enum class Hair : std::uint8_t { Sprout, Bob, Ponytail, Long, Buns, Spiky, Braid, Curly };
enum class Hat : std::uint8_t { None, Beret, Wizard, Bandana, Helmet, Feather, Flower, Hood };
enum class Top : std::uint8_t { Tee, Tunic, Robe, Plate, Coat, Apron, Vest, Sailor };
enum class Bottom : std::uint8_t { Pants, Shorts, Skirt, Greaves };
enum class Shoes : std::uint8_t { Boots, Shoes, Sabatons, Sandals };
enum class Gloves : std::uint8_t { None, Cloth, Gauntlets };
enum class Weapon : std::uint8_t { None, Branch, Sword, Axe, Staff, Wand, Broadsword, Tool };
enum class Cape : std::uint8_t { None, Cape, Scarf, Mantle };

inline constexpr int kHairStyles = 8;
inline constexpr int kEyeColors = 8;
inline constexpr int kSkinTones = 4;

struct Avatar {
    gamelib::core::Vec2 feet;
    float height = 128.0f; // the quad height; the avatar is about 0.75 of it
    float facing = 1.0f;
    AvatarPose pose = AvatarPose::Idle;
    float phase = 0.0f;
    gamelib::core::Color hairColor = gamelib::core::srgb(0x7A4E36);
    gamelib::core::Color topColor = gamelib::core::srgb(0xF6F0E0);
    Hair hair = Hair::Sprout;
    Hat hat = Hat::None;
    Top top = Top::Tee;
    Bottom bottom = Bottom::Shorts;
    Shoes shoes = Shoes::Shoes;
    Gloves gloves = Gloves::None;
    Weapon weapon = Weapon::None;
    Cape cape = Cape::None;
    Swatch hatColor = Swatch::Red;
    Swatch bottomColor = Swatch::Blue;
    Swatch shoeColor = Swatch::Brown;
    Swatch capeColor = Swatch::Red;
    Swatch accent = Swatch::Gold;
    std::uint8_t eyes = 0;
    std::uint8_t skin = 0;
    Expression expression = Expression::Neutral;
    Expression nextExpression = Expression::Neutral;
    float blend = 0.0f;
    std::uint8_t seed = 0;
    bool skeleton = false;
    bool glow = false;  // a buff's halo
    bool ghost = false; // an afterimage in topColor
};

[[nodiscard]] inline std::uint64_t packAvatarBits(const Avatar& a)
{
    const auto u = [](auto v) { return static_cast<std::uint64_t>(v); };
    const std::uint64_t lo = u(a.hair) | u(a.hat) << 3 | u(a.top) << 6 | u(a.bottom) << 9 | u(a.shoes) << 11
                             | u(a.gloves) << 13 | u(a.weapon) << 15 | u(a.cape) << 18 | u(a.eyes & 7u) << 20
                             | u(a.skin & 3u) << 23 | u(a.expression) << 25 | u(a.skeleton) << 29 | u(a.glow) << 30
                             | u(a.ghost) << 31;
    const std::uint64_t hi = u(a.nextExpression) | u(a.hatColor) << 4 | u(a.bottomColor) << 8 | u(a.shoeColor) << 12
                             | u(a.capeColor) << 16 | u(a.accent) << 20 | u(a.seed) << 24;
    return lo | hi << 32;
}

// The quad is square, its bottom 8/128 below the feet.
[[nodiscard]] inline gamelib::draw::Sprite avatarSprite(const Avatar& a, const gamelib::draw::ShapeId shape)
{
    const float size = a.height;
    return {.rect = {a.feet.x - size * 0.5f, a.feet.y - size * (120.0f / 128.0f), size, size},
            .color = a.hairColor,
            .color2 = a.topColor,
            .shape = shape,
            .params = {a.facing < 0.0f ? -1.0f : 1.0f, static_cast<float>(a.pose), a.phase, std::clamp(a.blend, 0.0f, 1.0f)},
            .bits = packAvatarBits(a)};
}

} // namespace realm::app::anime
