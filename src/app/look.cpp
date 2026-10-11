#include "look.hpp"

#include <gamelib/core/game_clock.hpp>

#include <algorithm>
#include <array>

namespace realm::app {

namespace {

// helpers

using game::Region;
using Sw = anime::Swatch;

Color mix(const Color a, const Color b, const float t)
{
    return gamelib::core::lerp(a, b, t);
}

RegionLook dayLook(const game::MapDef& m)
{
    RegionLook l;
    l.cloud = srgb(0xFFFFFF);
    l.cloudShade = srgb(0xCBDDF2);
    l.light = srgb(0xFFF4CC);
    l.ambient = srgb(0xFFFFFF);
    switch (m.region) {
    case Region::Island:
        l.skyTop = srgb(0x7CC6F4);
        l.skyHorizon = srgb(0xE0F4FF);
        l.far = srgb(0xA6D9B8);
        l.farRim = srgb(0xD2F0D0);
        l.near = srgb(0x7FCB7A);
        l.nearRim = srgb(0xB0E89A);
        l.cap = srgb(0x86D45E);
        l.body = srgb(0xC98B56);
        l.footCap = srgb(0x8EDB66);
        l.footBody = srgb(0xB57E4C);
        l.terrain = 0;
        l.backdrop = 0;
        l.bloom = 0.28f;
        break;
    case Region::Coast:
        l.skyTop = srgb(0x62BEF2);
        l.skyHorizon = srgb(0xE4F7FF);
        l.far = srgb(0x8CCFE0);
        l.farRim = srgb(0xC8EEF6);
        l.near = srgb(0x7FC2A0);
        l.nearRim = srgb(0xB2E2C2);
        if (m.town()) {
            l.cap = srgb(0xE2D2B4);
            l.body = srgb(0xA89A88);
            l.footCap = srgb(0xD49A5C);
            l.footBody = srgb(0x9A6A42);
            l.terrain = 4;
        } else {
            l.cap = srgb(0x8CD36A);
            l.body = srgb(0xD6B47E);
            l.footCap = srgb(0xF0DCA8);
            l.footBody = srgb(0xC49A66);
            l.terrain = 1;
        }
        l.backdrop = 1;
        l.bloom = 0.3f;
        break;
    case Region::Forest:
        l.skyTop = srgb(0x8CD2C4);
        l.skyHorizon = srgb(0xF2F4D2);
        l.far = srgb(0x74AE7E);
        l.farRim = srgb(0xA8D49A);
        l.near = srgb(0x55966A);
        l.nearRim = srgb(0x86C27E);
        l.cap = srgb(0x72C65A);
        l.body = srgb(0x8E6240);
        l.footCap = srgb(0xC8925A);
        l.footBody = srgb(0x8E5E3A);
        l.terrain = 5;
        l.backdrop = 2;
        l.light = srgb(0xFFF0B0);
        l.bloom = 0.32f;
        break;
    case Region::Mountain:
        l.skyTop = srgb(0x86BEF0);
        l.skyHorizon = srgb(0xF2F8FF);
        l.far = srgb(0xBBCBEA);
        l.farRim = srgb(0xF4F8FF);
        l.near = srgb(0x86A8C0);
        l.nearRim = srgb(0xC8DAEA);
        l.cap = srgb(0xF4F8FF);
        l.body = srgb(0x9AA4C2);
        l.footCap = srgb(0xF6FAFF);
        l.footBody = srgb(0x8E98B8);
        l.terrain = 3;
        l.backdrop = 3;
        l.cloudShade = srgb(0xD0DCF0);
        l.bloom = 0.34f;
        break;
    case Region::Depths:
        l.skyTop = srgb(0x221E40);
        l.skyHorizon = srgb(0x48387A);
        l.cloud = srgb(0x6A5AA8);
        l.cloudShade = srgb(0x40366E);
        l.far = srgb(0x3C3468);
        l.farRim = srgb(0x6A5EA8);
        l.near = srgb(0x2C2650);
        l.nearRim = srgb(0x5A4E92);
        l.cap = srgb(0x8A80B8);
        l.body = srgb(0x4E4672);
        l.footCap = srgb(0x9A8CC8);
        l.footBody = srgb(0x5A4E80);
        l.terrain = 4;
        l.backdrop = 4;
        l.light = srgb(0xA890FF);
        l.ambient = srgb(0x8C86C0);
        l.grade = {.tint = srgb(0xE6E2FF), .brightness = 1.0f, .contrast = 1.05f, .saturation = 1.1f};
        l.bloom = 0.5f;
        break;
    case Region::Count: break;
    }
    if (m.id == game::MapId::GlowcapHollow) {
        // A dim hollow lit by the glowing caps.
        l.skyTop = srgb(0x2A4048);
        l.skyHorizon = srgb(0x4E7068);
        l.far = srgb(0x2E5048);
        l.near = srgb(0x23403A);
        l.farRim = srgb(0x5A8E7A);
        l.nearRim = srgb(0x4A7A66);
        l.cloud = srgb(0x7AAE9A);
        l.ambient = srgb(0x9AB0B4);
        l.light = srgb(0x8CF0D8);
        l.bloom = 0.5f;
    }
    return l;
}

} // namespace

RegionLook regionLook(const game::MapDef& m, const float night)
{
    RegionLook l = dayLook(m);
    const bool underground = m.region == Region::Depths || m.id == game::MapId::GlowcapHollow;
    if (underground || night <= 0.0f) {
        return l;
    }
    // Night: a deep blue pastel sky, the land in moonlight, lamps doing the work.
    const float t = std::clamp(night, 0.0f, 1.0f);
    const float dusk = gamelib::core::duskAmount(t);
    l.skyTop = mix(mix(l.skyTop, srgb(0x6A5AA8), dusk * 0.7f), srgb(0x1C2858), t);
    l.skyHorizon = mix(mix(l.skyHorizon, srgb(0xFFB48A), dusk), srgb(0x44508C), t);
    l.cloud = mix(mix(l.cloud, srgb(0xFFD2B8), dusk), srgb(0x5A62A0), t);
    l.cloudShade = mix(l.cloudShade, srgb(0x343C78), t);
    l.far = mix(l.far, srgb(0x3A4A80), t * 0.8f);
    l.farRim = mix(l.farRim, srgb(0x6A78B8), t * 0.8f);
    l.near = mix(l.near, srgb(0x2C3A6A), t * 0.75f);
    l.nearRim = mix(l.nearRim, srgb(0x5A6AA8), t * 0.75f);
    l.light = mix(l.light, srgb(0xB8C8FF), t);
    l.ambient = mix(mix(srgb(0xFFFFFF), srgb(0xFFD6C0), dusk), srgb(0x7280C8), t);
    l.grade = gamelib::draw::lerp(l.grade, {.tint = srgb(0xC8D0FF), .brightness = 1.0f, .contrast = 1.02f, .saturation = 1.05f}, t);
    l.bloom = l.bloom + 0.25f * t;
    return l;
}

Color hairColor(const int index)
{
    static constexpr std::array<std::uint32_t, kHairColors> kHair{0x8A5A3C, 0x3E3A5C, 0xF2D27A, 0xE8763A, 0xE07A9A, 0x6A9AE8, 0x5AAE7A, 0xF2F0F6};
    return srgb(kHair[static_cast<std::size_t>(std::clamp(index, 0, kHairColors - 1))]);
}

anime::Avatar baseAvatar(const game::Appearance& look)
{
    anime::Avatar a;
    a.hair = static_cast<anime::Hair>(std::clamp(look.hair, 0, anime::kHairStyles - 1));
    a.hairColor = hairColor(look.hairColor);
    a.eyes = static_cast<std::uint8_t>(std::clamp(look.eyes, 0, 7));
    a.skin = static_cast<std::uint8_t>(std::clamp(look.skin, 0, 3));
    a.seed = static_cast<std::uint8_t>(look.name.size() * 37 + static_cast<std::size_t>(look.hair) * 11);
    a.top = anime::Top::Tee;
    a.topColor = srgb(0xF7F3E8);
    a.bottom = anime::Bottom::Shorts;
    a.bottomColor = Sw::Blue;
    a.shoes = anime::Shoes::Shoes;
    a.shoeColor = Sw::Red;
    a.accent = Sw::Orange;
    return a;
}

anime::Avatar heroAvatar(const game::Character& c)
{
    anime::Avatar a = baseAvatar(c.look);
    a.top = anime::Top::Tee;
    a.bottom = anime::Bottom::Shorts;
    a.shoes = anime::Shoes::Shoes;
    a.weapon = anime::Weapon::None;
    a.hat = anime::Hat::None;
    a.gloves = anime::Gloves::None;
    const auto look = [&](const game::Slot slot) -> const game::ItemLook* {
        const gamelib::core::ItemStack& s = c.gear.at(static_cast<int>(slot));
        return s.empty() ? nullptr : &game::itemInfo(s.item).look;
    };
    if (const game::ItemLook* l = look(game::Slot::Weapon)) {
        a.weapon = static_cast<anime::Weapon>(l->style);
        a.accent = static_cast<Sw>(l->swatch);
    }
    if (const game::ItemLook* l = look(game::Slot::Hat)) {
        a.hat = static_cast<anime::Hat>(l->style);
        a.hatColor = static_cast<Sw>(l->swatch);
    }
    if (const game::ItemLook* l = look(game::Slot::Top)) {
        a.top = static_cast<anime::Top>(l->style);
        a.topColor = srgb(l->color);
        if (l->style != 0 || a.weapon == anime::Weapon::None) {
            a.accent = static_cast<Sw>(l->swatch);
        }
    } else {
        a.topColor = srgb(0xE8DCC8);
    }
    if (const game::ItemLook* l = look(game::Slot::Bottom)) {
        a.bottom = static_cast<anime::Bottom>(l->style);
        a.bottomColor = static_cast<Sw>(l->swatch);
    }
    if (const game::ItemLook* l = look(game::Slot::Shoes)) {
        a.shoes = static_cast<anime::Shoes>(l->style);
        a.shoeColor = static_cast<Sw>(l->swatch);
    }
    if (const game::ItemLook* l = look(game::Slot::Gloves)) {
        a.gloves = static_cast<anime::Gloves>(l->style);
    }
    // A cape for the job: a warrior's red, a mage's star-blue scarf.
    if (c.job == game::Job::Warrior && c.level >= 15) {
        a.cape = anime::Cape::Cape;
        a.capeColor = Sw::Red;
    } else if (c.job == game::Job::Mage && c.level >= 15) {
        a.cape = anime::Cape::Mantle;
        a.capeColor = Sw::Navy;
    }
    return a;
}

anime::Avatar npcAvatar(const game::NpcId npc)
{
    using game::NpcId;
    using H = anime::Hair;
    using T = anime::Top;
    anime::Avatar a;
    a.seed = static_cast<std::uint8_t>(static_cast<int>(npc) * 29 + 7);
    switch (npc) {
    case NpcId::NanaRue:
        a.hair = H::Buns; a.hairColor = srgb(0xEDEAF2); a.top = T::Robe; a.topColor = srgb(0x9ACB86); a.accent = Sw::Gold;
        a.bottom = anime::Bottom::Skirt; a.shoes = anime::Shoes::Shoes; a.shoeColor = Sw::Brown; a.eyes = 6; a.weapon = anime::Weapon::Tool;
        a.cape = anime::Cape::Scarf; a.capeColor = Sw::Pink;
        break;
    case NpcId::Pip:
        a.hair = H::Sprout; a.hairColor = srgb(0xE8763A); a.top = T::Tee; a.topColor = srgb(0x86C8F0); a.accent = Sw::Yellow;
        a.bottom = anime::Bottom::Shorts; a.bottomColor = Sw::Brown; a.shoeColor = Sw::Leaf; a.eyes = 1; a.hat = anime::Hat::None;
        break;
    case NpcId::Hettie:
        a.hair = H::Ponytail; a.hairColor = srgb(0x9A5A3A); a.top = T::Apron; a.topColor = srgb(0xF2A0B0); a.accent = Sw::Pink;
        a.bottom = anime::Bottom::Skirt; a.bottomColor = Sw::Plum; a.eyes = 2; a.skin = 1; a.hat = anime::Hat::Bandana; a.hatColor = Sw::Pink;
        break;
    case NpcId::Marlo:
        a.hair = H::Spiky; a.hairColor = srgb(0x3A3A4A); a.top = T::Coat; a.topColor = srgb(0x3D4C92); a.accent = Sw::Gold;
        a.bottom = anime::Bottom::Pants; a.bottomColor = Sw::Cream; a.shoes = anime::Shoes::Boots; a.shoeColor = Sw::Brown; a.eyes = 0;
        a.skin = 2; a.hat = anime::Hat::Feather; a.hatColor = Sw::Navy;
        break;
    case NpcId::Quill:
        a.hair = H::Bob; a.hairColor = srgb(0xC8C8D8); a.top = T::Coat; a.topColor = srgb(0x2E5E8A); a.accent = Sw::Gold;
        a.bottom = anime::Bottom::Pants; a.bottomColor = Sw::Navy; a.shoes = anime::Shoes::Boots; a.eyes = 5; a.skin = 1;
        a.hat = anime::Hat::Beret; a.hatColor = Sw::Navy;
        break;
    case NpcId::Finn:
        a.hair = H::Spiky; a.hairColor = srgb(0xF2D27A); a.top = T::Sailor; a.topColor = srgb(0xF8F8FF); a.accent = Sw::Red;
        a.bottom = anime::Bottom::Shorts; a.bottomColor = Sw::Navy; a.shoes = anime::Shoes::Sandals; a.shoeColor = Sw::Brown; a.eyes = 0;
        a.skin = 2; a.cape = anime::Cape::Scarf; a.capeColor = Sw::Red;
        break;
    case NpcId::Rosa:
        a.hair = H::Curly; a.hairColor = srgb(0xC0402E); a.top = T::Apron; a.topColor = srgb(0xF2E2B8); a.accent = Sw::Orange;
        a.bottom = anime::Bottom::Skirt; a.bottomColor = Sw::Red; a.eyes = 1; a.skin = 1;
        break;
    case NpcId::Dottie:
        a.hair = H::Bob; a.hairColor = srgb(0x6A4A3A); a.top = T::Apron; a.topColor = srgb(0xB8E0A8); a.accent = Sw::Leaf;
        a.bottom = anime::Bottom::Skirt; a.bottomColor = Sw::Forest; a.eyes = 6; a.skin = 0; a.hat = anime::Hat::Flower; a.hatColor = Sw::Yellow;
        break;
    case NpcId::Garrick:
        a.hair = H::Spiky; a.hairColor = srgb(0x5A3A2A); a.top = T::Apron; a.topColor = srgb(0xA87A58); a.accent = Sw::Brown;
        a.bottom = anime::Bottom::Pants; a.bottomColor = Sw::Slate; a.shoes = anime::Shoes::Boots; a.gloves = anime::Gloves::Cloth;
        a.eyes = 2; a.skin = 3; a.weapon = anime::Weapon::Axe; a.hat = anime::Hat::Bandana; a.hatColor = Sw::Orange;
        break;
    case NpcId::Kestrel:
        a.hair = H::Ponytail; a.hairColor = srgb(0xB02E3A); a.top = T::Plate; a.topColor = srgb(0xD8E0F0); a.accent = Sw::Red;
        a.bottom = anime::Bottom::Greaves; a.bottomColor = Sw::Slate; a.shoes = anime::Shoes::Sabatons; a.shoeColor = Sw::Slate;
        a.gloves = anime::Gloves::Gauntlets; a.weapon = anime::Weapon::Broadsword; a.cape = anime::Cape::Cape; a.capeColor = Sw::Red;
        a.eyes = 4; a.skin = 1;
        break;
    case NpcId::Wren:
        a.hair = H::Long; a.hairColor = srgb(0x7A62D8); a.top = T::Robe; a.topColor = srgb(0x3A3E7E); a.accent = Sw::Sky;
        a.hat = anime::Hat::Wizard; a.hatColor = Sw::Navy; a.weapon = anime::Weapon::Staff; a.eyes = 3; a.skin = 0;
        a.cape = anime::Cape::Mantle; a.capeColor = Sw::Purple;
        break;
    case NpcId::Oakhart:
        a.hair = H::Buns; a.hairColor = srgb(0xD8D2C0); a.top = T::Robe; a.topColor = srgb(0x6A8A3A); a.accent = Sw::Gold;
        a.hat = anime::Hat::Hood; a.hatColor = Sw::Forest; a.weapon = anime::Weapon::Staff; a.eyes = 1; a.skin = 2;
        break;
    case NpcId::Lila:
        a.hair = H::Braid; a.hairColor = srgb(0x5AAE7A); a.top = T::Vest; a.topColor = srgb(0xE8B8D8); a.accent = Sw::Leaf;
        a.bottom = anime::Bottom::Skirt; a.bottomColor = Sw::Forest; a.hat = anime::Hat::Flower; a.hatColor = Sw::Pink; a.eyes = 1;
        break;
    case NpcId::Bramble:
        a.hair = H::Spiky; a.hairColor = srgb(0x6A8A3A); a.top = T::Tunic; a.topColor = srgb(0x5E8A4A); a.accent = Sw::Brown;
        a.bottom = anime::Bottom::Pants; a.bottomColor = Sw::Brown; a.shoes = anime::Shoes::Boots; a.hat = anime::Hat::Feather;
        a.hatColor = Sw::Forest; a.eyes = 2; a.skin = 2; a.cape = anime::Cape::Scarf; a.capeColor = Sw::Leaf;
        break;
    case NpcId::Tamsin:
        a.hair = H::Bob; a.hairColor = srgb(0x3A4A7A); a.top = T::Coat; a.topColor = srgb(0xE86A5A); a.accent = Sw::Cream;
        a.bottom = anime::Bottom::Pants; a.bottomColor = Sw::Navy; a.shoes = anime::Shoes::Boots; a.hat = anime::Hat::Hood;
        a.hatColor = Sw::Cream; a.eyes = 5; a.skin = 1; a.cape = anime::Cape::Scarf; a.capeColor = Sw::Yellow;
        break;
    case NpcId::Nix:
        a.hair = H::Ponytail; a.hairColor = srgb(0x2E2A3A); a.top = T::Vest; a.topColor = srgb(0xC8A060); a.accent = Sw::Orange;
        a.bottom = anime::Bottom::Shorts; a.bottomColor = Sw::Brown; a.shoes = anime::Shoes::Boots; a.gloves = anime::Gloves::Cloth;
        a.hat = anime::Hat::Beret; a.hatColor = Sw::Brown; a.eyes = 2; a.skin = 3;
        break;
    case NpcId::Count: break;
    }
    return a;
}

MonsterColors monsterColors(const game::SpeciesId s, const bool elite)
{
    using S = game::SpeciesId;
    MonsterColors c;
    switch (s) {
    case S::Sproutcap: c = {srgb(0xFFEEDA), srgb(0xF2788A)}; break;
    case S::Dewdrop: c = {srgb(0x8EDCF6), srgb(0xFFFFFF)}; break;
    case S::Swirlsnail: c = {srgb(0xF6DEB2), srgb(0xF0A050)}; break;
    case S::Pinchy: c = {srgb(0xF2705A), srgb(0xFFD8B0)}; break;
    case S::Puffgull: c = {srgb(0xF8F8FF), srgb(0xF6B84A)}; break;
    case S::BarrelHog: c = {srgb(0xF6C2A8), srgb(0xA8703E)}; break;
    case S::Stumpling: c = {srgb(0xB07A48), srgb(0x7CCB58)}; break;
    case S::Glowcap: c = {srgb(0xEAF6F0), srgb(0x5AD8E8)}; break;
    case S::Leafwisp: c = {srgb(0xDDF6C8), srgb(0x6CC05A)}; break;
    case S::Snowhog: c = {srgb(0xF6F8FF), srgb(0x8EB8E8)}; break;
    case S::Pebblet: c = {srgb(0xB8B0C8), srgb(0x8CD8F0)}; break;
    case S::Iceling: c = {srgb(0xBCE8FF), srgb(0xFFFFFF)}; break;
    case S::CaveFlitter: c = {srgb(0x8A72C8), srgb(0xF0C8FF)}; break;
    case S::Runeshell: c = {srgb(0x9C9AB8), srgb(0x8CF0E0)}; break;
    case S::Count: break;
    }
    if (elite) {
        // Elites: deeper, richer colors (the shader adds the crown and aura).
        c.body = mix(c.body, srgb(0xC09AF0), 0.25f);
        c.accent = mix(c.accent, srgb(0xF4C24E), 0.45f);
    }
    return c;
}

MonsterColors bossColors(const game::BossId b)
{
    switch (b) {
    case game::BossId::Thornbark: return {srgb(0xA8744A), srgb(0x6CC05A)};
    case game::BossId::Frostmane: return {srgb(0xF4F6FF), srgb(0x7AB8F0)};
    case game::BossId::Murkmaw: return {srgb(0x5A4890), srgb(0xF07AD0)};
    case game::BossId::Count: break;
    }
    return {srgb(0xFFFFFF), srgb(0xFFFFFF)};
}

int speciesVariant(const game::SpeciesId s)
{
    using S = game::SpeciesId;
    switch (s) {
    case S::Glowcap: return 1;    // glowing spotted cap
    case S::Iceling: return 1;    // crystals on the jelly
    case S::Snowhog: return 1;    // a woolly hog with little horns
    case S::Runeshell: return 1;  // a rune-carved stone shell
    case S::Leafwisp: return 1;   // a leafy sprite
    default: return 0;
    }
}

MonsterScale monsterScale(const game::Archetype a, const float boxHeight)
{
    using A = game::Archetype;
    // The quad is square with the feet 14/128 above its bottom; each
    // creature fills a different share of it.
    float fill = 0.62f; // the drawing's height as a share of the quad
    switch (a) {
    case A::Bellcap: fill = 0.62f; break;
    case A::Mochi: fill = 0.42f; break;
    case A::Turbanshell: fill = 0.55f; break;
    case A::Crab: fill = 0.48f; break;
    case A::Gull: fill = 0.5f; break;
    case A::Pudgy: fill = 0.52f; break;
    case A::Stumpling: fill = 0.68f; break;
    case A::Wisp: fill = 0.6f; break;
    case A::Pebblet: fill = 0.5f; break;
    case A::Flitter: fill = 0.46f; break;
    case A::Count: break;
    }
    const float quad = boxHeight * 1.08f / fill;
    return {.quad = quad, .top = quad * fill};
}

IconLook itemIcon(const int item)
{
    const game::ItemInfo& info = game::itemInfo(item);
    IconLook l;
    l.family = static_cast<float>(info.icon);
    if (game::isEquipment(item)) {
        l.style = static_cast<float>(info.look.style);
        l.a = info.look.color != 0 ? srgb(info.look.color) : paletteColor(info.look.swatch);
        l.b = paletteColor(info.look.swatch);
    } else {
        l.a = srgb(0xFFFFFF);
        l.b = srgb(0xFFFFFF);
    }
    return l;
}

IconLook skillIcon(const game::SkillId skill)
{
    const Color c = skillColor(skill);
    return {.family = 80.0f + static_cast<float>(skill), .style = 0.0f, .a = c, .b = mix(c, srgb(0xFFFFFF), 0.6f)};
}

Color skillColor(const game::SkillId s)
{
    using S = game::SkillId;
    switch (s) {
    case S::PebbleToss: return srgb(0xC8B898);
    case S::CatchBreath: return srgb(0x9AE0B8);
    case S::HyperJump: return srgb(0x8CDCFF);
    case S::PowerStrike: return srgb(0xFFB84A);
    case S::Cleave: return srgb(0xFF8A5A);
    case S::IronBody: return srgb(0xB8C8E0);
    case S::BladeMastery: return srgb(0xE0C8A0);
    case S::WarCry: return srgb(0xFF6A5A);
    case S::HpBoost: return srgb(0xFF7A8A);
    case S::Quake: return srgb(0xD89A5A);
    case S::Rush: return srgb(0xFFD05A);
    case S::ArcaneBolt: return srgb(0x8AB8FF);
    case S::ChainSpark: return srgb(0xB8F0FF);
    case S::ManaShield: return srgb(0x9A8CFF);
    case S::SpellMastery: return srgb(0xC8A8FF);
    case S::Swiftwind: return srgb(0x8CF0C8);
    case S::MpBoost: return srgb(0x6A9AF2);
    case S::Starfall: return srgb(0xFFE68A);
    case S::Bloom: return srgb(0xFF9AC8);
    case S::Count: break;
    }
    return srgb(0xFFFFFF);
}

Color rarityColor(const game::Rarity r)
{
    switch (r) {
    case game::Rarity::Common: return srgb(0xF2EEE4);
    case game::Rarity::Rare: return srgb(0x5EB0FF);
    case game::Rarity::Epic: return srgb(0xB070FF);
    case game::Rarity::Unique: return srgb(0xFFC040);
    case game::Rarity::Count: break;
    }
    return srgb(0xFFFFFF);
}

Color paletteColor(const int index)
{
    static constexpr std::array<std::uint32_t, 16> kPalette{0xF7F1E3, 0x4A3F66, 0xEC5662, 0xF89A48, 0xFAD65E, 0x86D065, 0x3FA067, 0x72C8F2,
                                                            0x4E80E6, 0x3D4C92, 0xA06CE4, 0xF790B6, 0x9C6438, 0xDDB584, 0x8E9DBC, 0xF4C24E};
    return srgb(kPalette[static_cast<std::size_t>(index & 15)]);
}

Color regionAccent(const game::Region r)
{
    switch (r) {
    case Region::Island: return srgb(0x8EDB66);
    case Region::Coast: return srgb(0x62BEF2);
    case Region::Forest: return srgb(0x6CC05A);
    case Region::Mountain: return srgb(0xB8D8FF);
    case Region::Depths: return srgb(0xA890FF);
    case Region::Count: break;
    }
    return srgb(0xFFFFFF);
}

} // namespace realm::app
