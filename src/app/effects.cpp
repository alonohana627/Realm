#include "effects.hpp"

#include "look.hpp"

#include <gamelib/core/easing.hpp>
#include <gamelib/draw/font.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

namespace realm::app {

namespace {

// helpers

using gamelib::core::Color;
using gamelib::core::Rect;
using gamelib::core::Vec2;
using game::EventKind;
using game::SkillId;

constexpr float kPi = std::numbers::pi_v<float>;

enum NumberKind : int { kHitNumber = 0, kCritNumber = 1, kHurtNumber = 2, kHealNumber = 3, kManaNumber = 4 };

gamelib::fx::DamageNumberStyle numberStyle()
{
    gamelib::fx::DamageNumberStyle s;
    s.criticalScale = 1.35f;
    s.pop = 0.55f;
    s.popSeconds = 0.14f;
    s.life = 1.05f;
    s.rise = 46.0f;
    s.fadeFrom = 0.62f;
    s.stackSeconds = 0.6f;
    s.digitHeight = 50.0f;
    s.digitAdvance = 0.62f;
    s.digitBob = 3.0f;
    s.stackSpacing = 46.0f;
    return s;
}

gamelib::draw::Font popupFont(const float size)
{
    return gamelib::draw::Font::vector(size, 1.5f).withShadow({.color = srgb(0x2A1E3A, 0.85f), .offset = {0.0f, 0.0f}, .weight = 3.2f});
}

} // namespace

Effects::Effects()
    : m_numbers{numberStyle()}
    , m_sparks{700, {0.0f, 900.0f}, 0.25f, 7}
    , m_petals{300, {0.0f, 120.0f}, 0.4f, 11}
{
}

void Effects::clear()
{
    m_numbers.clear();
    m_arcs.clear();
    m_popups.clear();
    m_ghosts.clear();
    m_falling.clear();
    m_sparks.clear();
    m_petals.clear();
    m_motes.clear();
    m_shake.clear();
    m_flash.clear();
}

void Effects::setPlace(const game::MapDef& map)
{
    using game::Region;
    gamelib::fx::AmbientStyle style;
    m_moteLook = {};
    m_leaves = false;
    m_snow = false;
    switch (map.region) {
    case Region::Island:
        // Drifting petals and pollen.
        style = {.rate = 3.0f, .maxMotes = 40, .speedX = {8.0f, 24.0f}, .speedY = {-6.0f, 10.0f}, .life = {5.0f, 9.0f}, .size = {2.0f, 3.5f}};
        m_moteLook = {.color = srgb(0xFFF2C0, 0.8f), .glow = 2.0f, .glowAlpha = 0.25f};
        m_leaves = true;
        break;
    case Region::Coast:
        style = {.rate = 2.0f, .maxMotes = 30, .speedX = {14.0f, 30.0f}, .speedY = {-8.0f, 4.0f}, .life = {4.0f, 8.0f}, .size = {1.5f, 2.5f}};
        m_moteLook = {.color = srgb(0xFFFFFF, 0.7f), .glow = 2.5f, .glowAlpha = 0.25f};
        break;
    case Region::Forest:
        // Fireflies and falling leaves.
        style = {.rate = 5.0f, .maxMotes = 60, .speedX = {-10.0f, 10.0f}, .speedY = {-12.0f, 6.0f}, .life = {3.0f, 6.0f}, .size = {1.6f, 3.0f}};
        m_moteLook = {.color = srgb(0xD8FF8A, 0.9f), .glow = 3.0f, .glowAlpha = 0.45f};
        m_leaves = true;
        break;
    case Region::Mountain:
        style = {.rate = 10.0f, .maxMotes = 120, .speedX = {-30.0f, -8.0f}, .speedY = {30.0f, 60.0f}, .life = {5.0f, 9.0f}, .size = {1.6f, 3.2f},
                 .band = {0.0f, 0.4f}};
        m_moteLook = {.color = srgb(0xFFFFFF, 0.9f), .glow = 1.5f, .glowAlpha = 0.3f};
        m_snow = true;
        break;
    case Region::Depths:
        style = {.rate = 5.0f, .maxMotes = 60, .speedX = {-6.0f, 6.0f}, .speedY = {-14.0f, -4.0f}, .life = {3.0f, 7.0f}, .size = {1.4f, 2.8f}};
        m_moteLook = {.color = srgb(0xC8A8FF, 0.9f), .glow = 3.5f, .glowAlpha = 0.5f};
        break;
    case Region::Count: break;
    }
    if (map.id == game::MapId::GlowcapHollow) {
        m_moteLook = {.color = srgb(0x8CF0E0, 0.9f), .glow = 3.5f, .glowAlpha = 0.5f};
    }
    m_motes.setStyle(style);
    m_motes.clear();
}

void Effects::react(const game::Event& e, const game::Session& session)
{
    const Vec2 heroCenter = session.hero().body.box.center();
    const Vec2 heroFeet = session.hero().body.box.bottomCenter();
    switch (e.kind) {
    case EventKind::Hit: {
        const int kind = e.crit ? kCritNumber : kHitNumber;
        const std::uint64_t target = static_cast<std::uint64_t>(static_cast<std::uint32_t>(e.target));
        m_numbers.add(e.value, e.at + Vec2{0.0f, -10.0f}, srgb(0xFFFFFF), e.crit, target, kind);
        addArc({.offset = e.at + Vec2{0.0f, 18.0f}, .attached = false, .core = e.crit ? srgb(0xFFE070) : srgb(0xFFFFFF),
                .edge = e.crit ? srgb(0xFF8A3A) : srgb(0xFFD0A0), .size = e.crit ? 170.0f : 120.0f, .kind = Fx::Spark,
                .variant = static_cast<float>(e.value % 7)},
               0.32f);
        m_sparks.burst(e.at + Vec2{0.0f, 18.0f}, e.crit ? 12 : 6,
                       {.speed = {200.0f, 520.0f}, .size = {4.0f, 8.0f}, .life = {0.18f, 0.4f},
                        .color = e.crit ? srgb(0xFFD060).scaled(1.4f) : srgb(0xFFF2D0).scaled(1.2f)});
        m_shake.trigger(e.crit ? 0.35f : 0.12f);
        break;
    }
    case EventKind::Hurt:
        m_numbers.add(e.value, e.at + Vec2{0.0f, -12.0f}, srgb(0xFFFFFF), false, 0xFFFF'FFFF'0001ull, kHurtNumber);
        m_shake.trigger(0.5f);
        m_flash.trigger(0.4f);
        m_flashColor = srgb(0xFF4060, 0.35f);
        break;
    case EventKind::ShieldHit:
        m_numbers.add(e.value, e.at + Vec2{30.0f, -40.0f}, srgb(0xFFFFFF), false, 0xFFFF'FFFF'0003ull, kManaNumber);
        addArc({.offset = {0.0f, -60.0f}, .attached = true, .core = srgb(0xC8C0FF), .edge = srgb(0x9A8CFF), .size = 200.0f, .kind = Fx::Shield},
               0.4f);
        break;
    case EventKind::Heal:
        m_numbers.add(e.value, e.at + Vec2{0.0f, -40.0f}, srgb(0xFFFFFF), false, 0xFFFF'FFFF'0002ull, kHealNumber);
        addArc({.offset = {0.0f, -60.0f}, .attached = true, .core = srgb(0xC8FFD8), .edge = srgb(0x7CE8A0), .size = 260.0f, .kind = Fx::Ring},
               0.6f);
        m_petals.burst(heroCenter, 22, {.direction = {kPi * 1.1f, kPi * 1.9f}, .speed = {120.0f, 320.0f}, .size = {8.0f, 13.0f},
                                        .life = {0.8f, 1.5f}, .spin = {-6.0f, 6.0f}, .color = srgb(0xFFA8D0)});
        break;
    case EventKind::Potion:
        if (e.onHero) {
            m_numbers.add(e.value, e.at + Vec2{0.0f, -40.0f}, srgb(0xFFFFFF), false, 0xFFFF'FFFF'0002ull, e.extra == 1 ? kManaNumber : kHealNumber);
            m_sparks.burst(heroCenter, 10, {.direction = {kPi * 1.2f, kPi * 1.8f}, .speed = {80.0f, 200.0f}, .size = {4.0f, 7.0f},
                                            .life = {0.4f, 0.8f}, .color = (e.extra == 1 ? srgb(0x9AC8FF) : srgb(0xFF9AAA)).scaled(1.3f)});
        }
        break;
    case EventKind::Swing: {
        const bool heavy = e.value == 3 || e.value == 6;
        const bool magic = e.value == 4 || e.value == 5;
        addArc({.offset = {static_cast<float>(e.facing) * 46.0f, -60.0f}, .facing = e.facing, .attached = true,
                .core = magic ? srgb(0xD8E8FF) : srgb(0xFFFFFF), .edge = magic ? srgb(0x8AB8FF) : srgb(0xFFE0A8),
                .size = heavy ? 230.0f : 190.0f, .kind = Fx::Slash, .variant = heavy ? 1.0f : 0.0f},
               0.26f);
        break;
    }
    case EventKind::SkillCast: {
        const auto skill = static_cast<SkillId>(e.value);
        const Color c = skillColor(skill);
        const float f = static_cast<float>(e.facing);
        switch (skill) {
        case SkillId::PowerStrike:
            addArc({.offset = {f * 70.0f, -64.0f}, .facing = e.facing, .attached = true, .core = srgb(0xFFF6C0), .edge = srgb(0xFFA040),
                    .size = 300.0f, .kind = Fx::Slash, .variant = 1.0f},
                   0.36f);
            addArc({.offset = {f * 110.0f, -60.0f}, .facing = e.facing, .attached = true, .core = srgb(0xFFF0B0), .edge = srgb(0xFF8A3A),
                    .size = 180.0f, .kind = Fx::Star},
                   0.3f);
            m_flash.trigger(0.25f);
            m_flashColor = srgb(0xFFF0C0, 0.25f);
            break;
        case SkillId::Cleave:
            addArc({.offset = {0.0f, -66.0f}, .facing = e.facing, .attached = true, .core = srgb(0xFFF0D8), .edge = srgb(0xFF7A4A),
                    .size = 420.0f, .kind = Fx::Crescent},
                   0.42f);
            addArc({.offset = {0.0f, -66.0f}, .facing = -e.facing, .attached = true, .core = srgb(0xFFE0C8), .edge = srgb(0xFF9A6A),
                    .size = 340.0f, .kind = Fx::Crescent},
                   0.38f);
            m_shake.trigger(0.3f);
            break;
        case SkillId::Quake:
            addArc({.offset = {f * 40.0f, -110.0f}, .facing = e.facing, .attached = true, .core = srgb(0xFFE8B0), .edge = srgb(0xD8904A),
                    .size = 220.0f, .kind = Fx::Slash, .variant = 1.0f},
                   0.3f);
            break;
        case SkillId::Rush:
            addArc({.offset = {-f * 80.0f, -50.0f}, .facing = e.facing, .attached = true, .core = srgb(0xFFF2B0), .edge = srgb(0xFFC040),
                    .size = 260.0f, .kind = Fx::Streak, .stretch = {1.6f, 0.5f}},
                   0.45f);
            break;
        case SkillId::ArcaneBolt:
        case SkillId::PebbleToss:
            addArc({.offset = {f * 50.0f, -60.0f}, .facing = e.facing, .attached = true, .core = c.scaled(1.3f), .edge = c,
                    .size = 110.0f, .kind = Fx::Ring},
                   0.3f);
            break;
        case SkillId::ChainSpark:
            addArc({.offset = {f * 40.0f, -60.0f}, .facing = e.facing, .attached = true, .core = srgb(0xE8FCFF), .edge = srgb(0x8AD8FF),
                    .size = 140.0f, .kind = Fx::Spark},
                   0.3f);
            break;
        case SkillId::IronBody:
        case SkillId::WarCry:
        case SkillId::ManaShield:
        case SkillId::Swiftwind:
            addArc({.offset = {0.0f, -60.0f}, .attached = true, .core = c.scaled(1.4f), .edge = c, .size = 320.0f, .kind = Fx::Ring}, 0.6f);
            addArc({.offset = {0.0f, -120.0f}, .attached = true, .core = c.scaled(1.2f), .edge = c, .size = 300.0f, .kind = Fx::LevelUp}, 0.8f);
            m_sparks.burst(heroCenter, 16, {.direction = {kPi * 1.1f, kPi * 1.9f}, .speed = {150.0f, 380.0f}, .size = {5.0f, 9.0f},
                                            .life = {0.4f, 0.8f}, .color = c.scaled(1.4f)});
            break;
        case SkillId::Bloom:
            addArc({.offset = {0.0f, -10.0f}, .attached = true, .core = srgb(0xFFD8EC), .edge = srgb(0xFF8AC0), .size = 380.0f, .kind = Fx::Ring},
                   0.7f);
            break;
        default: break;
        }
        break;
    }
    case EventKind::SkillHit: {
        const auto skill = static_cast<SkillId>(e.value);
        if (skill == SkillId::Quake) {
            for (const float side : {-1.0f, 1.0f}) {
                for (int i = 0; i < 4; ++i) {
                    addArc({.offset = {e.at.x + side * (60.0f + static_cast<float>(i) * 95.0f), e.at.y + e.to.y * 0.5f - 40.0f}, .attached = false,
                            .core = srgb(0xFFE8B8), .edge = srgb(0xD8904A), .size = 180.0f - static_cast<float>(i) * 20.0f, .kind = Fx::Shockwave},
                           0.3f + static_cast<float>(i) * 0.08f);
                }
            }
            m_sparks.burst({e.at.x, e.at.y + e.to.y * 0.5f - 10.0f}, 24, {.direction = {kPi * 1.05f, kPi * 1.95f}, .speed = {200.0f, 520.0f},
                                                                         .size = {6.0f, 12.0f}, .life = {0.4f, 0.8f}, .color = srgb(0xC89A6A)});
            m_shake.trigger(1.0f);
        } else if (skill == SkillId::Starfall) {
            m_sparks.burst({e.at.x, e.at.y + e.to.y * 0.35f}, 14, {.speed = {120.0f, 360.0f}, .size = {5.0f, 9.0f}, .life = {0.3f, 0.6f},
                                                                    .color = srgb(0xFFE68A).scaled(1.4f)});
            m_shake.trigger(0.4f);
        }
        break;
    }
    case EventKind::Star: {
        // Starfall: stars falling across the area, staggered.
        const float left = e.at.x - e.to.x * 0.5f;
        for (int i = 0; i < 14; ++i) {
            const float fx = left + e.to.x * (static_cast<float>((i * 7) % 14) + 0.5f) / 14.0f;
            const float land = e.at.y + e.to.y * (0.55f + 0.35f * static_cast<float>((i * 5) % 7) / 7.0f);
            m_falling.add({.from = {fx - 160.0f, e.at.y - 300.0f}, .to = {fx, land}, .color = srgb(0xFFE68A),
                           .delay = static_cast<float>(i % 7) * 0.06f + (i >= 7 ? 0.24f : 0.0f)},
                          0.8f);
        }
        break;
    }
    case EventKind::Chain: {
        const Vec2 d = e.to - e.at;
        const float len = std::max(std::sqrt(d.x * d.x + d.y * d.y), 1.0f);
        addArc({.offset = (e.at + e.to) * 0.5f, .attached = false, .core = srgb(0xF0FCFF), .edge = srgb(0x7AD0FF), .size = len,
                .kind = Fx::Lightning, .variant = static_cast<float>(e.value), .rotation = std::atan2(d.y, d.x), .stretch = {1.0f, 0.32f}},
               0.3f + static_cast<float>(e.value) * 0.03f);
        addArc({.offset = e.to, .attached = false, .core = srgb(0xF0FCFF), .edge = srgb(0x7AD0FF), .size = 120.0f, .kind = Fx::Spark}, 0.3f);
        break;
    }
    case EventKind::HyperJump:
        addArc({.offset = {-static_cast<float>(e.facing) * (e.value == 1 ? 0.0f : 100.0f), e.value == 1 ? 10.0f : -50.0f}, .facing = e.facing,
                .attached = false, .core = srgb(0xE0F8FF), .edge = srgb(0x7AD0FF), .size = 260.0f, .kind = Fx::Streak,
                .rotation = e.value == 1 ? -kPi * 0.5f : (e.facing > 0 ? 0.0f : kPi), .stretch = {1.4f, 0.45f}},
               0.4f);
        addArc({.offset = heroFeet + Vec2{0.0f, -30.0f}, .attached = false, .core = srgb(0xE8FCFF), .edge = srgb(0x8CDCFF), .size = 200.0f,
                .kind = Fx::Ring},
               0.35f);
        m_ghostClock = 0.0f;
        break;
    case EventKind::Jump:
        m_sparks.burst(e.at, 4, {.direction = {kPi * 1.1f, kPi * 1.9f}, .speed = {40.0f, 110.0f}, .size = {5.0f, 9.0f}, .life = {0.2f, 0.4f},
                                 .color = srgb(0xFFFFFF, 0.7f)});
        break;
    case EventKind::Land:
        if (e.value > 18) {
            m_sparks.burst(e.at, 8, {.direction = {kPi * 1.0f, kPi * 2.0f}, .speed = {60.0f, 180.0f}, .size = {6.0f, 10.0f},
                                     .life = {0.25f, 0.45f}, .color = srgb(0xF6EEDC, 0.8f)});
        }
        break;
    case EventKind::MonsterDown: {
        const MonsterColors mc = monsterColors(static_cast<game::SpeciesId>(e.value), e.extra != 0);
        m_sparks.burst(e.at, e.extra != 0 ? 22 : 12, {.speed = {120.0f, 340.0f}, .size = {6.0f, 11.0f}, .life = {0.35f, 0.7f}, .color = mc.accent});
        break;
    }
    case EventKind::Pickup: {
        const std::string name{game::itemName(e.value)};
        m_popups.add({.at = heroFeet + Vec2{0.0f, -150.0f}, .text = name, .color = rarityColor(static_cast<game::Rarity>(std::clamp(e.extra, 0, 3))),
                      .item = e.value, .row = m_popupRow++},
                     1.6f);
        m_popupClock = 0.0f;
        break;
    }
    case EventKind::Coins:
        m_popups.add({.at = heroFeet + Vec2{0.0f, -150.0f}, .text = std::format("+{} coins", e.value), .color = srgb(0xFFD860), .item = -1,
                      .row = m_popupRow++},
                     1.3f);
        m_popupClock = 0.0f;
        break;
    case EventKind::LevelUp:
        addArc({.offset = {0.0f, -150.0f}, .attached = true, .core = srgb(0xFFF0A0), .edge = srgb(0xFFB040), .size = 420.0f, .kind = Fx::LevelUp},
               1.6f);
        m_popups.add({.at = heroFeet + Vec2{0.0f, -210.0f}, .text = "level up!", .color = srgb(0xFFE070), .item = -2, .row = 0}, 2.0f);
        m_flash.trigger(0.6f);
        m_flashColor = srgb(0xFFF4C8, 0.4f);
        break;
    case EventKind::JobAdvance:
        addArc({.offset = {0.0f, -150.0f}, .attached = true, .core = srgb(0xE8F0FF), .edge = srgb(0x8AB8FF), .size = 520.0f, .kind = Fx::LevelUp},
               2.2f);
        addArc({.offset = {0.0f, -60.0f}, .attached = true, .core = srgb(0xFFFFFF), .edge = srgb(0xFFE070), .size = 420.0f, .kind = Fx::Ring}, 0.8f);
        m_sparks.burst(heroCenter, 40, {.speed = {200.0f, 600.0f}, .size = {6.0f, 12.0f}, .life = {0.5f, 1.2f}, .color = srgb(0xFFF0B0).scaled(1.4f)});
        m_flash.trigger(1.0f);
        m_flashColor = srgb(0xFFFFFF, 0.6f);
        break;
    case EventKind::Spawned:
        if (e.extra < 100) {
            addArc({.offset = e.at + Vec2{0.0f, -20.0f}, .attached = false, .core = srgb(0xFFFFFF, 0.8f), .edge = srgb(0xD8E8FF), .size = 120.0f,
                    .kind = Fx::Ring},
                   0.4f);
        }
        break;
    case EventKind::BossLand:
        m_shake.trigger(1.0f);
        m_sparks.burst(e.at, 18, {.direction = {kPi * 1.0f, kPi * 2.0f}, .speed = {150.0f, 420.0f}, .size = {7.0f, 13.0f}, .life = {0.3f, 0.6f},
                                  .color = srgb(0xE8DCC8)});
        break;
    case EventKind::BossPhase:
        m_flash.trigger(0.6f);
        m_flashColor = srgb(0xFF6050, 0.3f);
        m_shake.trigger(0.6f);
        break;
    case EventKind::BossDown:
        m_flash.trigger(1.0f);
        m_flashColor = srgb(0xFFFFFF, 0.7f);
        m_shake.trigger(1.0f);
        m_sparks.burst(e.at, 50, {.speed = {200.0f, 700.0f}, .size = {7.0f, 14.0f}, .life = {0.6f, 1.4f}, .color = srgb(0xFFE8A0).scaled(1.4f)});
        break;
    case EventKind::HazardStrike:
        if (e.value == 1) {
            addArc({.offset = e.at + Vec2{0.0f, -80.0f}, .attached = false, .core = srgb(0xA8744A), .edge = srgb(0x6CC05A), .size = 170.0f,
                    .kind = Fx::Root, .stretch = {0.6f, 1.0f}},
                   0.7f);
            m_shake.trigger(0.35f);
        }
        break;
    case EventKind::Death:
        m_flash.trigger(1.0f);
        m_flashColor = srgb(0xFFFFFF, 0.5f);
        break;
    default: break;
    }
}

void Effects::update(const float seconds, const game::Session& session, const Rect visible, const Vec2 heroFeet)
{
    m_numbers.update(seconds);
    m_arcs.update(seconds);
    m_popups.update(seconds);
    m_ghosts.update(seconds);
    m_falling.update(seconds);
    m_sparks.update(seconds);
    m_petals.update(seconds);
    m_motes.update(seconds, visible);
    m_shake.update(seconds);
    m_flash.update(seconds);
    m_popupClock += seconds;
    if (m_popupClock > 1.2f) {
        m_popupRow = 0;
    }
    // The hyper jump's afterimages while it carries.
    const game::Hero& h = session.hero();
    if (h.hyperTicks > 4) {
        m_ghostClock -= seconds;
        if (m_ghostClock <= 0.0f) {
            m_ghostClock = 0.035f;
            m_ghosts.add({.feet = heroFeet, .facing = h.body.facing, .up = h.hyperUp}, 0.32f);
        }
    }
}

void Effects::drawBehind(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 heroFeet, const float time) const
{
    (void)time;
    for (const auto& item : m_arcs.items()) {
        const Arc& a = item.value;
        if (a.kind != Fx::Ring || !a.attached) {
            continue;
        }
        const Vec2 at = heroFeet + a.offset;
        addFx(list, shapes, a.kind, at, a.size, a.core, a.edge, item.progress(), a.variant);
    }
}

void Effects::drawFront(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 heroFeet, const int heroFacing, const float time) const
{
    (void)heroFacing;
    for (const auto& item : m_arcs.items()) {
        const Arc& a = item.value;
        if (a.kind == Fx::Ring && a.attached) {
            continue;
        }
        const Vec2 at = a.attached ? heroFeet + a.offset : a.offset;
        const float t = item.progress();
        if (a.stretch.x != 1.0f || a.stretch.y != 1.0f) {
            const float w = a.size * a.stretch.x;
            const float h = a.size * a.stretch.y;
            addFxRect(list, shapes, a.kind, {at.x - w * 0.5f, at.y - h * 0.5f, w, h}, a.core, a.edge, t, a.variant, time, a.rotation);
            continue;
        }
        // Arcs mirror for the left by turning half a circle.
        const float rot = a.rotation + (a.facing < 0 && (a.kind == Fx::Slash || a.kind == Fx::Crescent) ? kPi : 0.0f);
        float size = a.size;
        if (a.kind == Fx::Ring || a.kind == Fx::Spark) {
            size *= 0.6f + 0.6f * t;
        }
        list.add({.rect = {at.x - size * 0.5f, at.y - size * 0.5f, size, size},
                  .color = a.core,
                  .color2 = a.edge,
                  .shape = shapes.fx,
                  .rotation = rot,
                  .params = {static_cast<float>(a.kind), t, a.variant, a.facing < 0 && a.kind == Fx::Slash ? 1.0f : 0.0f}});
    }
    // Falling stars.
    for (const auto& item : m_falling.items()) {
        const Falling& f = item.value;
        const float t = std::clamp((item.age - f.delay) / 0.3f, 0.0f, 1.0f);
        if (item.age < f.delay) {
            continue;
        }
        const Vec2 at = gamelib::core::lerp(f.from, f.to, t);
        const float fade = 1.0f - std::clamp((item.age - f.delay - 0.3f) / 0.3f, 0.0f, 1.0f);
        if (t < 1.0f) {
            addFxRect(list, shapes, Fx::Streak, {at.x - 90.0f, at.y - 22.0f, 180.0f, 44.0f}, f.color.scaled(1.3f).withAlpha(0.9f), f.color,
                      0.0f, 0.0f, 0.0f, std::atan2(f.to.y - f.from.y, f.to.x - f.from.x));
            addFx(list, shapes, Fx::Star, at, 90.0f, srgb(0xFFF6C8).scaled(1.5f), f.color.scaled(1.2f), 0.0f, 0.0f, time * 3.0f);
        } else if (fade > 0.0f) {
            addFx(list, shapes, Fx::Spark, at, 160.0f, srgb(0xFFF6C8).withAlpha(fade), f.color, 1.0f - fade);
        }
    }
    m_sparks.draw(list);
    m_petals.draw(list, {}, {.aspect = 0.6f, .cornerRadius = 6.0f, .endScale = 0.7f});
    m_motes.draw(list, m_moteLook);
}

void Effects::drawNumbers(gamelib::draw::DrawList& list, const Shapes& shapes, const float time) const
{
    (void)time;
    // Damage numbers in chunky two-tone outlined digits.
    m_numbers.draw(list, [&shapes](gamelib::draw::DrawList& l, const gamelib::fx::DamageNumbers::Number& n, const char digit,
                                   const Rect& cell, const float alpha) {
        Color top = srgb(0xFFE070);
        Color bottom = srgb(0xFF8A2A);
        switch (n.kind) {
        case kCritNumber: top = srgb(0xFFD0F0); bottom = srgb(0xF0508A); break;
        case kHurtNumber: top = srgb(0xD8B0FF); bottom = srgb(0x8A3AD0); break;
        case kHealNumber: top = srgb(0xC8FFB0); bottom = srgb(0x3AB050); break;
        case kManaNumber: top = srgb(0xC8E4FF); bottom = srgb(0x3A7AE0); break;
        default: break;
        }
        const int d = digit == '+' ? 10 : digit - '0';
        if (d < 0 || d > 10) {
            return;
        }
        addUi(l, shapes, Ui::Digit, cell, top.withAlpha(alpha), bottom, static_cast<float>(d));
    });
    // Pickup popups: the item's icon and its name, rising and fading.
    const gamelib::draw::Font font = popupFont(26.0f);
    for (const auto& item : m_popups.items()) {
        const Popup& p = item.value;
        const float t = item.progress();
        const float rise = gamelib::core::easeOutCubic(std::min(t * 2.0f, 1.0f)) * 40.0f + t * 20.0f;
        const float alpha = 1.0f - gamelib::core::smoothstep(0.65f, 1.0f, t);
        const Vec2 at = p.at + Vec2{0.0f, -rise - static_cast<float>(p.row) * 40.0f};
        if (p.item == -2) {
            const gamelib::draw::Font big = popupFont(52.0f);
            gamelib::draw::drawText(list, big, p.text, at.x, at.y, p.color.withAlpha(alpha), gamelib::draw::Align::Center);
            continue;
        }
        const float w = font.width(p.text);
        float x = at.x - w * 0.5f;
        if (p.item > 0) {
            const IconLook icon = itemIcon(p.item);
            addIcon(list, shapes, Ui::ItemIcon, {x - 4.0f, at.y + 12.0f}, 40.0f, icon.family, icon.style, icon.a.withAlpha(alpha), icon.b);
            x += 20.0f;
        }
        gamelib::draw::drawText(list, font, p.text, x, at.y, p.color.withAlpha(alpha));
    }
}

void Effects::drawScreen(gamelib::draw::DrawList& list, const gamelib::core::Size canvas) const
{
    gamelib::fx::drawFlash(list, canvas, m_flash, m_flashColor);
}

} // namespace realm::app
