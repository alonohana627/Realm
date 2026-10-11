#include "view.hpp"

#include "look.hpp"

#include <gamelib/core/collision.hpp>
#include <gamelib/core/easing.hpp>
#include <gamelib/draw/font.hpp>
#include <gamelib/draw/parallax.hpp>
#include <gamelib/draw/tile_map.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>

namespace realm::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
namespace layers = gamelib::draw::layers;
using game::DecoKind;

constexpr float kAvatarQuad = 158.0f; // the hero is about 120 units tall
// 1.25 canvas units per world unit: the hero stands about 150 of the 1080
// canvas rows tall (MapleStory-like) and a field shows 32 x 18 tiles.
constexpr float kZoom = 1.25f;

constexpr int kLayerSkyline = -262;
constexpr int kLayerLight = -258;
constexpr int kLayerDecoBack = -250;
constexpr int kLayerHouses = -240;
constexpr int kLayerClimb = -160;
constexpr int kLayerTerrain = -150;
constexpr int kLayerFoothold = -140;
constexpr int kLayerPortal = -60;
constexpr int kLayerBehindFx = -10;
constexpr int kLayerNpc = 10;
constexpr int kLayerLoot = 20;
constexpr int kLayerBoss = 25;
constexpr int kLayerMonster = 30;
constexpr int kLayerHero = 50;
constexpr int kLayerFront = 200;
constexpr int kLayerEffects = 300;
constexpr int kLayerWorldUi = layers::Hud;

float hashf(const float x)
{
    const float s = std::sin(x * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

Rect lerpR(const Rect& a, const Rect& b, const float t)
{
    return gamelib::core::lerp(a, b, t);
}

struct DecoSize {
    float width;
    float height;
};

DecoSize decoSize(const DecoKind k)
{
    using K = DecoKind;
    switch (k) {
    case K::House: return {520.0f, 520.0f};
    case K::Tree: return {330.0f, 330.0f};
    case K::Pine: return {280.0f, 300.0f};
    case K::Bush: return {200.0f, 128.0f};
    case K::Flowers: return {130.0f, 96.0f};
    case K::Grass: return {96.0f, 72.0f};
    case K::Rock: return {160.0f, 120.0f};
    case K::Lamp: return {130.0f, 250.0f};
    case K::Fence: return {260.0f, 100.0f};
    case K::Sign: return {140.0f, 170.0f};
    case K::Barrel: return {80.0f, 90.0f};
    case K::Crate: return {80.0f, 80.0f};
    case K::FlowerPot: return {80.0f, 80.0f};
    case K::Well: return {220.0f, 230.0f};
    case K::Bench: return {150.0f, 96.0f};
    case K::Stall: return {270.0f, 250.0f};
    case K::Bunting: return {520.0f, 130.0f}; // the width scales, the height doesn't
    case K::Fountain: return {340.0f, 280.0f};
    case K::Lighthouse: return {340.0f, 540.0f};
    case K::Boat: return {640.0f, 540.0f};
    case K::Post: return {70.0f, 120.0f};
    case K::Crystal: return {150.0f, 150.0f};
    case K::Torch: return {80.0f, 150.0f};
    case K::Lantern: return {110.0f, 200.0f};
    case K::Mushroom: return {300.0f, 300.0f};
    case K::Snowpile: return {170.0f, 90.0f};
    case K::Banner: return {120.0f, 240.0f};
    case K::Anvil: return {130.0f, 120.0f};
    case K::GiantTree: return {760.0f, 1640.0f};
    case K::Tent: return {260.0f, 200.0f};
    case K::Campfire: return {140.0f, 120.0f};
    case K::Stalagmite: return {140.0f, 200.0f};
    case K::Column: return {150.0f, 300.0f};
    case K::Shrine: return {300.0f, 330.0f};
    case K::Shell: return {80.0f, 60.0f};
    case K::Reeds: return {130.0f, 160.0f};
    case K::Windmill: return {440.0f, 540.0f};
    case K::Count: break;
    }
    return {100.0f, 100.0f};
}

void decoColors(const DecoKind k, const game::MapDef& map, const int variant, Color& a, Color& b)
{
    using K = DecoKind;
    using R = game::Region;
    const R r = map.region;
    a = srgb(0xFFFFFF);
    b = srgb(0xFFFFFF);
    switch (k) {
    case K::Tree:
        a = r == R::Forest ? srgb(0x5EBC5A) : (variant == 1 ? srgb(0x7ACB62) : srgb(0x68C25C));
        b = variant == 1 ? srgb(0xF790B6) : srgb(0xEC5662);
        break;
    case K::Pine: a = r == R::Mountain ? srgb(0x4A9A84) : srgb(0x3F9A6A); break;
    case K::Bush:
        a = srgb(0x6CC062);
        b = variant == 1 ? srgb(0xFFD84E) : srgb(0xF790B6);
        break;
    case K::Flowers:
        a = variant == 1 ? srgb(0xFFD84E) : srgb(0xF790B6);
        b = variant == 1 ? srgb(0xFFFFFF) : srgb(0xA0B4FF);
        break;
    case K::Grass: a = r == R::Mountain ? srgb(0xA8C8B0) : srgb(0x7CCB58); break;
    case K::Rock: a = r == R::Mountain ? srgb(0xB8C4DC) : r == R::Coast ? srgb(0xC8BCA8) : srgb(0xA8A4BC); break;
    case K::Fence: a = map.town() ? srgb(0xFFFFFF) : srgb(0xC8955A); break;
    case K::FlowerPot: a = variant == 1 ? srgb(0xFFD84E) : srgb(0xEC5662); break;
    case K::Well: a = srgb(0xEC5662); break;
    case K::Stall:
        a = variant == 1 ? srgb(0x7CC8F0) : variant == 2 ? srgb(0xF2A060) : srgb(0xF790B6);
        b = srgb(0xFFF6EA);
        break;
    case K::Boat:
        a = srgb(0x3D6CB8);
        b = srgb(0xEC5662);
        break;
    case K::Mushroom: a = variant == 2 ? srgb(0x5AD8E8) : variant == 1 ? srgb(0xF4C24E) : srgb(0xEC5662); break;
    case K::Stalagmite: a = r == R::Depths ? srgb(0x7A70A8) : srgb(0xA8A0B8); break;
    default: break;
    }
}

// House walls and roofs: each style its own colors, a palette index picks
// one of the storybook pairs instead so a street is varied.
void houseColors(const int style, const int palette, Color& wall, Color& roof)
{
    if (palette > 0) {
        static constexpr std::array<std::uint32_t, 12> kWalls{0xFFF1D6, 0xE2F4E4, 0xFFE2D2, 0xEAE4F0, 0xDDEEFA, 0xFFF6C8,
                                                              0xF8E0EC, 0xE8E0D0, 0xF4EAD8, 0xE0ECF8, 0xFCE6D0, 0xEEF2E0};
        static constexpr std::array<std::uint32_t, 12> kRoofs{0xEC5662, 0x4E80E6, 0x6DBE57, 0xF2C460, 0x9A6AD8, 0xF0924E,
                                                              0x4FB8B0, 0xD86A8E, 0x5E8AD8, 0xE06A4E, 0x7AB84A, 0xC86ABE};
        wall = srgb(kWalls[static_cast<std::size_t>(palette - 1) % kWalls.size()]);
        roof = srgb(kRoofs[static_cast<std::size_t>(palette * 5 + style) % kRoofs.size()]);
        if (style == static_cast<int>(game::HouseStyle::Warehouse)) {
            wall = srgb(0xC89A6A);
        } else if (style == static_cast<int>(game::HouseStyle::Dockhouse)) {
            wall = srgb(0x9CC8E8);
        } else if (style == static_cast<int>(game::HouseStyle::TreeHut)) {
            wall = srgb(0xC8955A);
            roof = srgb(0x6CC462);
        }
        return;
    }
    using H = game::HouseStyle;
    switch (static_cast<H>(style)) {
    case H::Cottage: wall = srgb(0xFFF1D6); roof = srgb(0xF2C460); break;
    case H::Shop: wall = srgb(0xFDE6EE); roof = srgb(0x4E80E6); break;
    case H::Inn: wall = srgb(0xFFF1D6); roof = srgb(0xEC5662); break;
    case H::Warehouse: wall = srgb(0xC89A6A); roof = srgb(0x5E6680); break;
    case H::Townhouse: wall = srgb(0xEAE4F0); roof = srgb(0x9A6AD8); break;
    case H::TreeHut: wall = srgb(0xC8955A); roof = srgb(0x6CC462); break;
    case H::Library: wall = srgb(0xD8D4E8); roof = srgb(0x5A5EC8); break;
    case H::Armory: wall = srgb(0xC8C0B0); roof = srgb(0x8A4A3A); break;
    case H::Dockhouse: wall = srgb(0x9CC8E8); roof = srgb(0xEC5662); break;
    case H::Bakery: wall = srgb(0xFFF4E2); roof = srgb(0xF0924E); break;
    case H::Count: break;
    }
}

int expressionFor(const game::Session& s)
{
    const game::Hero& h = s.hero();
    if (!h.alive) {
        return static_cast<int>(anime::Expression::Knocked);
    }
    if (h.hurtTicks > 0) {
        return static_cast<int>(anime::Expression::Hurt);
    }
    if (h.attack.busy() || h.castTicks > 0) {
        return static_cast<int>(anime::Expression::Determined);
    }
    if (s.character().hp.current * 4 < s.character().hp.max) {
        return static_cast<int>(anime::Expression::Sad);
    }
    return static_cast<int>(anime::Expression::Neutral);
}

bool lightsUp(const game::MapDef& def)
{
    return def.region == game::Region::Depths || def.kind == game::MapKind::Dungeon;
}

} // namespace

WorldView::WorldView(const gamelib::core::Size canvas)
    : m_canvas{canvas}
    , m_camera{canvas, kZoom}
    , m_squash{4.5f, 0.32f}
{
}

Rect WorldView::visible() const
{
    return m_camera.view().visibleWorld(m_canvas);
}

Vec2 WorldView::heroFeet(const game::Session& session, const float alpha) const
{
    const game::Hero& h = session.hero();
    return lerpR(h.previous, h.body.box, alpha).bottomCenter();
}

void WorldView::snap(const game::Session& session)
{
    m_map = session.mapId();
    m_camera.setZoom(session.map().kind == game::MapKind::Arena ? 1.0f : kZoom);
    // The camera stops a little below the lowest floor, so the earth under
    // the ground shows as a band, not half the screen.
    const gamelib::core::TileWorld world{session.map().tiles, 0, game::tileShapes()};
    m_camera.setBounds(gamelib::core::boundsAboveLowestFloor(session.map().bounds(), world, 3.3f * game::kTile));
    m_camera.centerOn(session.hero().body.box.bottomCenter() + Vec2{0.0f, -140.0f});
    m_squash.reset();
}

void WorldView::follow(const game::Session& session, const float alpha, const float seconds, const float time)
{
    (void)time;
    if (session.mapId() != m_map) {
        snap(session);
    }
    const game::Hero& h = session.hero();
    m_camera.follow({.position = heroFeet(session, alpha) + Vec2{0.0f, -150.0f},
                     .facing = static_cast<float>(h.body.facing),
                     .grounded = h.body.grounded || h.body.climbing},
                    seconds,
                    {.deadZone = {50.0f, 40.0f}, .lookahead = 110.0f, .lookaheadRate = 1.8f, .rate = 6.0f, .verticalOnLanding = true,
                     .maxRise = 300.0f});
    if (h.landTicks < m_lastLandTicks && h.body.grounded) {
        m_squash.kick(-0.15f);
    }
    m_lastLandTicks = h.landTicks;
    m_squash.update(seconds);
    const int wanted = expressionFor(session);
    if (wanted != m_expressionTo) {
        m_expressionFrom = m_expressionBlend > 0.5f ? m_expressionTo : m_expressionFrom;
        m_expressionTo = wanted;
        m_expressionBlend = 0.0f;
    }
    m_expressionBlend = std::min(1.0f, m_expressionBlend + seconds / 0.09f);
}

anime::Avatar WorldView::heroLook(const game::Session& s, const float alpha)
{
    const game::Hero& h = s.hero();
    const auto& b = h.body;
    anime::Avatar a = heroAvatar(s.character());
    a.feet = lerpR(h.previous, b.box, alpha).bottomCenter();
    a.height = kAvatarQuad;
    a.facing = static_cast<float>(b.facing);
    using P = anime::AvatarPose;
    const int style = s.character().gear.at(0).empty() ? 0 : game::itemInfo(s.character().gear.at(0).item).look.style;
    const bool caster = style == 4 || style == 5;
    if (!h.alive) {
        a.pose = P::Hurt;
        a.phase = 0.0f;
    } else if (h.hurtTicks > 0) {
        a.pose = P::Hurt;
        a.phase = 1.0f - static_cast<float>(h.hurtTicks) / 16.0f;
    } else if (b.climbing) {
        a.pose = P::Climb;
        a.phase = std::fmod(b.box.top() / 64.0f, 1.0f);
        if (a.phase < 0.0f) {
            a.phase += 1.0f;
        }
    } else if (h.castTicks > 0) {
        const float p = 1.0f - static_cast<float>(h.castTicks) / static_cast<float>(std::max(h.castLength, 1));
        using S = game::SkillId;
        switch (h.casting) {
        case S::PowerStrike:
        case S::Cleave:
        case S::Quake:
            a.pose = P::Swing;
            a.phase = 0.2f + p * 0.8f;
            break;
        case S::PebbleToss:
            a.pose = P::Throw;
            a.phase = p;
            break;
        case S::Rush:
            a.pose = P::Swing;
            a.phase = 0.55f;
            break;
        case S::IronBody:
        case S::WarCry:
            a.pose = P::Cheer;
            a.phase = p;
            break;
        default:
            a.pose = P::Cast;
            a.phase = p;
            break;
        }
    } else if (h.attack.busy()) {
        const auto& clip = h.attack.clip();
        const float total = static_cast<float>(std::max(clip.totalTicks(), 1));
        const float swingAt = static_cast<float>(clip.startup) / total;
        const float p = static_cast<float>(h.attack.ticks()) / total;
        if (caster) {
            a.pose = P::Cast;
            a.phase = p;
        } else {
            a.pose = P::Swing;
            a.phase = p < swingAt ? p / std::max(swingAt, 1e-3f) * 0.38f : 0.38f + (p - swingAt) / std::max(1.0f - swingAt, 1e-3f) * 0.62f;
        }
    } else if (h.prone) {
        a.pose = P::Prone;
    } else if (!b.grounded) {
        a.pose = P::Jump;
        a.phase = std::clamp((b.velocity.y + 6.0f) / 12.0f, 0.0f, 1.0f);
    } else if (std::abs(b.velocity.x) > 0.4f) {
        a.pose = P::Walk;
        a.phase = gamelib::fx::walkPhase(b.walked, 132.0f);
    } else {
        a.pose = P::Idle;
    }
    // A glow while a buff lasts.
    a.glow = !s.character().buffs.all().empty();
    return a;
}

void WorldView::cacheMap(const game::MapDef& def) const
{
    if (m_cached == def.id) {
        return;
    }
    m_cached = def.id;
    m_runs.clear();
    const auto at = [&](const int x, const int y) -> char {
        if (x < 0 || y < 0 || x >= def.columns || y >= def.rows) {
            return game::kEmpty;
        }
        return def.text[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
    };
    const auto climbable = [](const char c) { return c == game::kRope || c == game::kLadder; };
    const float t = game::kTile;
    // Footholds (with the top cell of a rope or ladder between their cells)
    // and bridges, row by row.
    const std::vector<gamelib::core::TileRun> rows =
        gamelib::core::tileRuns(def.columns, def.rows, gamelib::core::Axis::X, [&](const gamelib::core::Cell c) {
            const char ch = at(c.x, c.y);
            if (ch == game::kFoothold) {
                return 1;
            }
            if (ch == game::kBridge) {
                return 2;
            }
            if (climbable(ch) && at(c.x, c.y - 1) != ch
                && (at(c.x - 1, c.y) == game::kFoothold || at(c.x + 1, c.y) == game::kFoothold)) {
                return 1;
            }
            return 0;
        });
    for (const gamelib::core::TileRun& run : rows) {
        const float left = static_cast<float>(run.first) * t;
        const float width = static_cast<float>(run.length()) * t;
        const float top = static_cast<float>(run.line) * t;
        if (run.kind == 1) {
            m_runs.push_back({.rect = {left - 12.0f, top - 14.0f, width + 24.0f, 72.0f}, .kind = Terrain::Foothold});
        } else {
            m_runs.push_back({.rect = {left - 12.0f, top - 16.0f, width + 24.0f, 64.0f}, .kind = Terrain::Bridge});
        }
    }
    // Ropes and ladders, column by column.
    const std::vector<gamelib::core::TileRun> columns =
        gamelib::core::tileRuns(def.columns, def.rows, gamelib::core::Axis::Y, [&](const gamelib::core::Cell c) {
            const char ch = at(c.x, c.y);
            return ch == game::kRope ? 3 : ch == game::kLadder ? 4 : 0;
        });
    for (const gamelib::core::TileRun& run : columns) {
        const bool rope = run.kind == 3;
        const float w = rope ? 30.0f : 46.0f;
        const float x = static_cast<float>(run.line) * t + t * 0.5f;
        const float top = static_cast<float>(run.first) * t + (rope ? 16.0f : 6.0f);
        const float bottom = static_cast<float>(run.last + 1) * t - 4.0f;
        m_runs.push_back({.rect = {x - w * 0.5f, top, w, bottom - top}, .kind = rope ? Terrain::Rope : Terrain::Ladder});
    }
}

void WorldView::drawBackdrop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    const game::MapDef& def = session.map();
    const float night = session.nightAmount();
    const RegionLook look = regionLook(def, night);
    int scene = look.backdrop;
    if (def.id == game::MapId::GlowcapHollow) {
        scene = 5;
    }
    const auto bits = static_cast<std::uint64_t>(scene);
    const Rect bounds = def.bounds();
    const Vec2 anchor{0.0f, bounds.bottom() - static_cast<float>(m_canvas.height) / m_camera.zoom()};
    const std::array layersList{
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.0f, 0.0f}, .color = look.skyHorizon, .color2 = look.skyTop,
                                     .params = {0.0f, night}, .bits = bits},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.05f, 0.03f}, .anchor = anchor, .color = look.cloud,
                                     .color2 = look.cloudShade, .params = {1.0f, night}, .bits = bits},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.16f, 0.1f}, .anchor = anchor, .color = look.far,
                                     .color2 = look.farRim, .params = {2.0f, night}, .bits = bits},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.36f, 0.22f}, .anchor = anchor, .color = look.near,
                                     .color2 = look.nearRim, .params = {3.0f, night}, .bits = bits},
    };
    list.setLayer(layers::Background);
    gamelib::draw::drawParallax(list, std::span{layersList}.first(2), m_camera.view(), m_canvas);
    list.setLayer(layers::Parallax);
    gamelib::draw::drawParallax(list, std::span{layersList}.subspan(2), m_camera.view(), m_canvas);
    const std::array lightLayer{gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.3f, 0.0f}, .color = look.light,
                                                             .params = {4.0f, night}, .bits = bits}};
    list.setLayer(kLayerLight);
    gamelib::draw::drawParallax(list, lightLayer, m_camera.view(), m_canvas);
}

void WorldView::drawTerrain(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    const game::MapDef& def = session.map();
    cacheMap(def);
    const RegionLook look = regionLook(def, 0.0f);
    const auto material = static_cast<float>(look.terrain);
    gamelib::draw::TileSet tiles;
    tiles.set(gamelib::core::charTile(game::kGround),
              {.shape = shapes.terrain, .color = look.cap, .color2 = look.body, .params = {0.0f, material, 0.0f, 0.0f}});
    tiles.set(gamelib::core::charTile(game::kRampUp),
              {.shape = shapes.terrain, .color = look.cap, .color2 = look.body, .params = {5.0f, material, 0.0f, 0.0f}});
    tiles.set(gamelib::core::charTile(game::kRampDown),
              {.shape = shapes.terrain, .color = look.cap, .color2 = look.body, .params = {6.0f, material, 0.0f, 0.0f}});
    list.setLayer(kLayerTerrain);
    const Rect vis = visible().expanded(game::kTile);
    gamelib::draw::drawTiles(list, def.tiles, 0, tiles, vis, {.outsideConnects = true});
    // Footholds: wood in Mossbough and on the docks' walkways, the place's own elsewhere.
    float footMaterial = material;
    if (def.region == game::Region::Forest || def.id == game::MapId::SaltwhistleDocks || def.id == game::MapId::SunkenArchive) {
        footMaterial = 2.0f;
    } else if (def.region == game::Region::Island || def.region == game::Region::Coast) {
        footMaterial = 0.0f;
    }
    const Color footCap = footMaterial == 2.0f ? srgb(0xD49A5C) : look.footCap;
    const Color footBody = footMaterial == 2.0f ? srgb(0x9A6A42) : look.footBody;
    for (const Run& r : m_runs) {
        if (!gamelib::core::overlaps(r.rect, vis)) {
            continue;
        }
        list.setLayer(r.kind == Terrain::Foothold || r.kind == Terrain::Bridge ? kLayerFoothold : kLayerClimb);
        list.add({.rect = r.rect,
                  .color = r.kind == Terrain::Foothold ? footCap : srgb(0xFFFFFF),
                  .color2 = footBody,
                  .shape = shapes.terrain,
                  .params = {static_cast<float>(r.kind), footMaterial, r.rect.x, 0.0f}});
    }
}

void WorldView::drawDecos(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    const game::MapDef& def = session.map();
    const float night = session.nightAmount();
    const float lit = lightsUp(def) ? 1.0f : gamelib::core::smoothstep(0.2f, 0.6f, night);
    const auto spriteOf = [&](const game::Deco& d, const int index) {
        const DecoSize size = decoSize(d.kind);
        const float w = size.width * d.scale;
        const float h = d.kind == DecoKind::Bunting ? size.height : size.height * d.scale;
        if (d.kind == DecoKind::House) {
            Color wall;
            Color roof;
            houseColors(d.variant, d.palette, wall, roof);
            return gamelib::draw::Sprite{.rect = {d.at.x - w * 0.5f, d.at.y - h * (122.0f / 128.0f), w, h},
                                         .color = wall,
                                         .color2 = roof,
                                         .shape = shapes.building,
                                         .params = {static_cast<float>(d.variant), lit, 0.35f, 0.0f},
                                         .bits = static_cast<std::uint64_t>((index * 37 + d.palette * 11) % 255)};
        }
        Color a;
        Color b;
        decoColors(d.kind, def, d.variant, a, b);
        float decoLit = lit;
        if (d.kind == DecoKind::Shrine && d.variant == 0) {
            decoLit = session.portalOpen(2) ? 1.0f : 0.0f; // the Undercroft door's gem
        }
        if (d.kind == DecoKind::Torch || d.kind == DecoKind::Campfire) {
            decoLit = 1.0f;
        }
        return decoSprite(shapes, static_cast<int>(d.kind), d.at, w, h, a, b, decoLit, static_cast<float>(d.variant),
                          hashf(static_cast<float>(index)) * 10.0f);
    };
    if (!def.skyline.empty()) {
        std::vector<gamelib::draw::Sprite> row;
        row.reserve(def.skyline.size());
        int index = 500;
        for (const game::Deco& d : def.skyline) {
            row.push_back(spriteOf(d, ++index));
        }
        list.setLayer(kLayerSkyline);
        gamelib::draw::drawParallaxSprites(list, row, m_camera.view(), m_canvas,
                                           {.haze = regionLook(def, night).far, .hazeAmount = 0.5f});
    }
    const Rect vis = visible().expanded(400.0f);
    int index = 0;
    // The ship at its mooring, behind everything on the docks.
    if (def.dock.x >= 0.0f) {
        const DecoSize size = decoSize(DecoKind::Boat);
        Color a;
        Color b;
        decoColors(DecoKind::Boat, def, 0, a, b);
        list.setLayer(kLayerDecoBack - 2);
        list.add(decoSprite(shapes, static_cast<int>(DecoKind::Boat), def.dock + Vec2{0.0f, 70.0f}, size.width, size.height, a, b, lit, 0.0f, 1.0f));
    }
    for (const game::Deco& d : def.decos) {
        ++index;
        const gamelib::draw::Sprite s = spriteOf(d, index);
        if (s.rect.right() < vis.left() || s.rect.left() > vis.right()) {
            continue;
        }
        list.setLayer(d.kind == DecoKind::House ? kLayerHouses
                      : d.kind == DecoKind::GiantTree ? kLayerHouses - 2
                      : d.front ? kLayerFront : kLayerDecoBack);
        list.add(s);
    }
}

void WorldView::drawPortals(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    (void)time;
    const game::MapDef& def = session.map();
    list.setLayer(kLayerPortal);
    for (int i = 0; i < static_cast<int>(def.portals.size()); ++i) {
        const game::Portal& p = def.portals[static_cast<std::size_t>(i)];
        if (p.hidden) {
            continue;
        }
        const bool open = session.portalOpen(i);
        const Color edge = open ? srgb(0x7AC8FF) : srgb(0xB0A8C0);
        list.add({.rect = {p.at.x - 80.0f, p.at.y - 200.0f, 160.0f, 210.0f},
                  .color = srgb(0xE8F8FF).scaled(1.2f),
                  .color2 = edge,
                  .shape = shapes.fx,
                  .params = {static_cast<float>(Fx::Portal), 0.0f, open ? 0.0f : 1.0f, static_cast<float>(i)}});
    }
}

void WorldView::drawNpcs(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    const Vec2 heroFeet = session.hero().body.box.bottomCenter();
    list.setLayer(kLayerNpc);
    for (const game::NpcHere& n : session.npcs()) {
        anime::Avatar a = npcAvatar(n.npc);
        a.feet = n.feet;
        a.height = kAvatarQuad;
        const float dx = heroFeet.x - n.feet.x;
        const bool near = std::abs(dx) < 260.0f && std::abs(heroFeet.y - n.feet.y) < 160.0f;
        a.facing = near ? (dx < 0.0f ? -1.0f : 1.0f) : static_cast<float>(n.facing);
        a.pose = anime::AvatarPose::Idle;
        a.expression = anime::Expression::Neutral; // eyes open: closed eyes are for reactions
        a.nextExpression = a.expression;
        if (n.npc == game::NpcId::Garrick && !near) {
            const float cycle = std::fmod(time, 2.6f);
            if (cycle < 0.7f) {
                a.pose = anime::AvatarPose::Swing;
                a.phase = cycle / 0.7f;
            }
        } else if (n.npc == game::NpcId::Pip) {
            // Pip swings his legs on the branch: a little bob, eyes open.
            a.phase = time * 1.6f;
        } else if (n.npc == game::NpcId::Kestrel && !near) {
            const float cycle = std::fmod(time, 3.2f);
            if (cycle < 0.8f) {
                a.pose = anime::AvatarPose::Swing;
                a.phase = cycle / 0.8f;
            }
        }
        list.add(anime::avatarSprite(a, shapes.avatar));
    }
}

void WorldView::drawMonsters(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha) const
{
    list.setLayer(kLayerMonster);
    for (const game::Monster& m : session.monsters()) {
        const Rect box = lerpR(m.previous, m.box, alpha);
        const game::Species& sp = m.kind();
        const MonsterColors colors = monsterColors(m.species, m.elite);
        const MonsterScale scale = monsterScale(sp.look, box.height);
        using MS = game::MonsterState;
        float progress = m.progress();
        int state = 0;
        float fade = 1.0f;
        switch (m.state) {
        case MS::Spawn:
            state = 0;
            fade = progress;
            break;
        case MS::Idle: state = 0; break;
        case MS::Move:
            state = 1;
            if (sp.behavior == game::Behavior::Hopper) {
                progress = m.grounded ? 0.05f : 0.2f + 0.6f * std::clamp((m.velocity.y + 7.0f) / 14.0f, 0.0f, 1.0f);
            } else {
                progress = std::fmod(box.center().x / 60.0f + 10.0f, 1.0f);
            }
            break;
        case MS::Windup: state = 2; break;
        case MS::Attack:
            state = 1;
            progress = sp.behavior == game::Behavior::Hopper ? 0.5f : std::fmod(box.center().x / 40.0f + 10.0f, 1.0f);
            break;
        case MS::Hit:
        case MS::Stun: state = 3; break;
        case MS::Defeat: state = 4; break;
        }
        if (m.state == MS::Stun) {
            progress = 0.5f;
        }
        const float size = scale.quad;
        const Vec2 feet = box.bottomCenter();
        list.add({.rect = {feet.x - size * 0.5f, feet.y - size * (114.0f / 128.0f), size, size},
                  .color = colors.body.withAlpha(fade),
                  .color2 = colors.accent,
                  .shape = shapes.monster,
                  .params = {static_cast<float>(sp.look), static_cast<float>(state), progress, static_cast<float>(m.facing)},
                  .bits = (m.elite ? 1u : 0u) | static_cast<std::uint64_t>(speciesVariant(m.species) & 7) << 1
                          | static_cast<std::uint64_t>(m.seed) << 8});
        if (m.state == MS::Stun) {
            // Stars circling over a stunned monster.
            for (int i = 0; i < 3; ++i) {
                const float a = static_cast<float>(session.ticks()) * 0.12f + static_cast<float>(i) * 2.09f;
                addFx(list, shapes, Fx::Star, {feet.x + std::cos(a) * 30.0f, feet.y - scale.top - 10.0f + std::sin(a) * 8.0f}, 28.0f,
                      srgb(0xFFF2A0).scaled(1.3f), srgb(0xFFB040), 0.0f);
            }
        }
    }
}

void WorldView::drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha) const
{
    if (!session.boss()) {
        return;
    }
    const game::Boss& b = *session.boss();
    const Rect box = lerpR(b.previous, b.box, alpha);
    const MonsterColors bc = bossColors(b.id);
    int state = 0;
    float progress = b.progress();
    switch (b.state) {
    case game::BossState::Intro:
    case game::BossState::Idle: state = std::abs(b.velocity.x) > 0.3f ? 1 : 0; break;
    case game::BossState::Windup: state = 2; break;
    case game::BossState::Act: state = 1; break;
    case game::BossState::Recover: state = 0; break;
    case game::BossState::Defeat: state = 4; break;
    }
    if (state == 1) {
        progress = std::fmod(box.center().x / 120.0f + 10.0f, 1.0f);
    }
    if (session.ticks() - b.lastHitTick < 10 && b.alive()) {
        state = 3;
        progress = static_cast<float>(session.ticks() - b.lastHitTick) / 10.0f;
    }
    const float size = b.info().size.y * 2.25f;
    const Vec2 feet = box.bottomCenter();
    list.setLayer(kLayerBoss);
    list.add({.rect = {feet.x - size * 0.5f, feet.y - size * (114.0f / 128.0f), size, size},
              .color = bc.body.withAlpha(b.fade),
              .color2 = bc.accent,
              .shape = shapes.boss,
              .params = {static_cast<float>(b.phase), static_cast<float>(state), progress, static_cast<float>(b.facing)},
              .bits = static_cast<std::uint64_t>(b.id)});
}

void WorldView::drawLoot(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                         const float time) const
{
    list.setLayer(kLayerLoot);
    for (const auto& d : session.drops().items()) {
        const Rect box = lerpR(d.previous, d.box, alpha);
        const float bob = d.settled ? std::sin(time * 3.0f + static_cast<float>(d.id)) * 4.0f - 4.0f : 0.0f;
        const Vec2 at = box.center() + Vec2{0.0f, bob};
        // A blink before it goes.
        const float life = session.drops().lifeLeft(d);
        const float blink = life < 0.1f ? (std::fmod(time * 6.0f, 1.0f) < 0.5f ? 0.3f : 1.0f) : 1.0f;
        if (d.value.coins > 0) {
            addFx(list, shapes, Fx::Coin, at, 42.0f, srgb(0xF6C845).withAlpha(blink), srgb(0xFFFFFF),
                  std::fmod(time * 0.8f + static_cast<float>(d.id) * 0.3f, 1.0f));
            continue;
        }
        const game::Rarity rarity = game::rarityOf(d.value.stack);
        const Color glow = rarityColor(rarity);
        addFx(list, shapes, Fx::LootGlow, at, 110.0f, glow.withAlpha((rarity == game::Rarity::Common ? 0.3f : 0.8f) * blink), glow.scaled(1.3f),
              0.0f, rarity >= game::Rarity::Epic ? 1.0f : 0.0f, static_cast<float>(d.id));
        const IconLook icon = itemIcon(d.value.stack.item);
        addIcon(list, shapes, Ui::ItemIcon, at, 54.0f, icon.family, icon.style, icon.a.withAlpha(blink), icon.b);
    }
}

void WorldView::drawShots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    list.setLayer(kLayerEffects - 10);
    for (const auto& s : session.shots().items()) {
        const float ang = std::atan2(s.velocity.y, s.velocity.x);
        if (s.extra.hero) {
            Vec2 back = gamelib::core::normalized(s.velocity) * -1.0f;
            if (back.x == 0.0f && back.y == 0.0f) {
                back = {-1.0f, 0.0f};
            }
            if (s.extra.look == game::ShotLook::Bolt) {
                const Vec2 tail = s.position + back * 90.0f;
                addFxRect(list, shapes, Fx::Streak, {tail.x - 100.0f, tail.y - 34.0f, 200.0f, 68.0f}, srgb(0xC8E0FF).withAlpha(0.9f),
                          srgb(0x6A9AFF), 0.0f, 0.0f, 0.0f, ang);
                addFx(list, shapes, Fx::Bolt, s.position, 110.0f, srgb(0xE0F0FF).scaled(1.3f), srgb(0x6A9AFF), 0.0f, 0.0f,
                      static_cast<float>(s.id));
            } else {
                addFx(list, shapes, Fx::Shot, s.position, 40.0f, srgb(0xFFFFFF), srgb(0xFFFFFF), 0.0f,
                      static_cast<float>(game::ShotLook::Pebble), static_cast<float>(s.id) + time);
            }
            continue;
        }
        Color core = srgb(0xFFFFFF);
        Color edge = srgb(0xFFFFFF);
        switch (s.extra.look) {
        case game::ShotLook::Acorn: core = srgb(0xC8884A); edge = srgb(0x8A5A3A); break;
        case game::ShotLook::Snowball: core = srgb(0xF6FAFF); edge = srgb(0x8AB8F0); break;
        case game::ShotLook::Gloom: core = srgb(0x8A6AD8); edge = srgb(0xF07AD0); break;
        case game::ShotLook::Wave: core = srgb(0xFFF2D0); edge = srgb(0xFFB060); break;
        default: core = srgb(0xFFF2B0); edge = srgb(0xFFC84A); break;
        }
        const float size = std::max(s.size.x, s.size.y) * (s.extra.groundWave ? 2.4f : 1.8f);
        addFx(list, shapes, Fx::Shot, s.position, size, core, edge, 0.0f, static_cast<float>(s.extra.look), static_cast<float>(s.id));
    }
    // Hazards announced before they strike.
    if (session.map().arena) {
        const float floorY = session.map().arena->bounds.bottom();
        const bool below = session.boss() && session.boss()->info().hazardsFromBelow;
        for (const auto& item : session.hazards().items()) {
            const float h = below ? 200.0f : 420.0f;
            addFxRect(list, shapes, Fx::Warn, {item.value - 50.0f, floorY - h, 100.0f, h}, srgb(0xFF6A7A), srgb(0xFFB0B8), item.progress());
        }
    }
}

void WorldView::drawHero(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
                         const float alpha, const float time) const
{
    const game::Hero& h = session.hero();
    // The hyper jump's afterimages.
    list.setLayer(kLayerHero - 1);
    for (const auto& g : effects.ghosts().items()) {
        anime::Avatar a = heroLook(session, alpha);
        a.feet = g.value.feet;
        a.facing = static_cast<float>(g.value.facing);
        a.pose = anime::AvatarPose::Jump;
        a.ghost = true;
        a.glow = false;
        gamelib::draw::Sprite s = anime::avatarSprite(a, shapes.avatar);
        s.color = s.color.withAlpha(0.5f * (1.0f - g.progress()));
        s.color2 = g.value.up ? srgb(0xB8F0FF) : srgb(0x9FD8FF);
        list.add(s);
    }
    anime::Avatar a = heroLook(session, alpha);
    a.expression = static_cast<anime::Expression>(m_expressionFrom);
    a.nextExpression = static_cast<anime::Expression>(m_expressionTo);
    a.blend = m_expressionBlend;
    gamelib::draw::Sprite sprite = anime::avatarSprite(a, shapes.avatar);
    const Vec2 sq = m_squash.scale();
    sprite.rect = gamelib::fx::scaledFromBottom(sprite.rect, {sq.x, sq.y});
    if (!h.body.grounded && !h.body.climbing) {
        const float stretch = std::clamp(-h.body.velocity.y * 0.007f, -0.05f, 0.07f);
        sprite.rect = gamelib::fx::scaledFromBottom(sprite.rect, {1.0f - stretch * 0.6f, 1.0f + stretch});
    }
    if (h.rushTicks > 0) {
        sprite.rotation = static_cast<float>(h.body.facing) * 0.12f;
    }
    if (h.vulnerable.invincible() && h.alive && h.rushTicks == 0) {
        sprite.color.a = 0.55f + 0.45f * (0.5f + 0.5f * std::sin(time * 30.0f));
    }
    list.setLayer(kLayerHero);
    list.add(sprite);
    // Buff auras.
    const Vec2 feet = heroFeet(session, alpha);
    list.setLayer(kLayerHero + 1);
    if (session.character().buffs.active(game::kBuffSwiftwind)) {
        addFx(list, shapes, Fx::Wind, feet + Vec2{0.0f, -62.0f}, 240.0f, srgb(0xE8FFF4).withAlpha(0.7f), srgb(0x9AF0C8), 0.0f);
    }
    if (session.character().buffs.active(game::kBuffManaShield)) {
        addFx(list, shapes, Fx::Shield, feet + Vec2{0.0f, -60.0f}, 190.0f, srgb(0xD8D0FF), srgb(0x9A8CFF), 0.0f);
    }
}

void WorldView::drawLights(gamelib::draw::DrawList& list, const game::Session& session, const float alpha) const
{
    const game::MapDef& def = session.map();
    const float night = session.nightAmount();
    const bool dark = lightsUp(def);
    const float amount = dark ? 1.0f : gamelib::core::smoothstep(0.15f, 0.7f, night);
    const RegionLook look = regionLook(def, night);
    list.setPostProcess({.ambient = look.ambient,
                         .bloom = {.intensity = look.bloom, .threshold = 0.85f, .radius = 1.2f},
                         .grade = look.grade,
                         .overlayLayer = layers::Hud});
    if (amount <= 0.01f) {
        return;
    }
    const Rect vis = visible().expanded(400.0f);
    int seed = 0;
    for (const game::Deco& d : def.decos) {
        ++seed;
        if (!vis.contains(d.at)) {
            continue;
        }
        using K = DecoKind;
        const DecoSize size = decoSize(d.kind);
        const auto light = [&](const Vec2 offset, const float radius, const std::uint32_t color, const float intensity, const float flicker) {
            list.light({.position = d.at + offset, .radius = radius, .color = srgb(color).withAlpha(amount), .intensity = intensity,
                        .flicker = flicker, .seed = static_cast<std::uint32_t>(seed)});
        };
        switch (d.kind) {
        case K::Lamp: light({size.width * 0.11f, -size.height * 0.78f}, 320.0f, 0xFFC27A, 1.1f, 0.08f); break;
        case K::Torch: light({0.0f, -size.height * 0.62f}, 300.0f, 0xFF9E4A, 1.2f, 0.3f); break;
        case K::Lantern: light({size.width * 0.15f, -size.height * 0.62f}, 260.0f, 0xFFC27A, 1.0f, 0.12f); break;
        case K::Campfire: light({0.0f, -40.0f}, 380.0f, 0xFF9E4A, 1.3f, 0.35f); break;
        case K::Crystal: light({0.0f, -50.0f}, 260.0f, 0x9FE6FF, 0.95f, 0.1f); break;
        case K::Mushroom:
            if (d.variant == 2) {
                light({0.0f, -size.height * 0.55f * d.scale}, 320.0f, 0x8CF0E0, 0.9f, 0.05f);
            }
            break;
        case K::Lighthouse: light({0.0f, -size.height * 0.88f}, 520.0f, 0xFFF0B0, 1.4f, 0.05f); break;
        case K::House: light({0.0f, -size.height * 0.4f * d.scale}, 380.0f * d.scale, 0xFFB868, 0.9f, 0.05f); break;
        case K::Windmill: light({0.0f, -size.height * 0.45f}, 260.0f, 0xFFB868, 0.8f, 0.05f); break;
        default: break;
        }
    }
    for (int i = 0; i < static_cast<int>(def.portals.size()); ++i) {
        const game::Portal& p = def.portals[static_cast<std::size_t>(i)];
        if (p.hidden || !vis.contains(p.at)) {
            continue;
        }
        list.light({.position = p.at + Vec2{0.0f, -90.0f}, .radius = 260.0f, .color = srgb(0x9FD8FF).withAlpha(amount), .intensity = 0.9f,
                    .flicker = 0.1f, .seed = static_cast<std::uint32_t>(i * 11)});
    }
    const Vec2 feet = heroFeet(session, alpha);
    list.light({.position = feet + Vec2{0.0f, -80.0f}, .radius = dark ? 520.0f : 300.0f,
                .color = srgb(0xFFE2B0).withAlpha(dark ? 1.0f : amount * 0.7f), .intensity = dark ? 1.1f : 0.8f,
                .flicker = dark ? 0.05f : 0.0f, .seed = 5});
    for (const auto& s : session.shots().items()) {
        if (s.extra.hero && s.extra.look == game::ShotLook::Bolt) {
            list.light({.position = s.position, .radius = 220.0f, .color = srgb(0x8AB8FF).withAlpha(amount), .intensity = 1.0f});
        }
    }
    if (session.boss()) {
        const game::Boss& b = *session.boss();
        list.light({.position = b.box.center(), .radius = 460.0f, .color = srgb(0xFFE0C0).withAlpha(amount * 0.6f), .intensity = 0.8f});
    }
}

void WorldView::drawLabels(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                           const float time) const
{
    using gamelib::draw::Align;
    list.setLayer(kLayerWorldUi);
    const Rect view = visible();
    const gamelib::draw::Font nameFont = gamelib::draw::Font::vector(22.0f, 1.3f);
    const auto plate = [&](const std::string_view text, const Vec2 where, const Color fill, const Color ink, const gamelib::draw::Font& font) {
        if (where.x < view.left() - 100.0f || where.x > view.right() + 100.0f) {
            return;
        }
        const float w = font.width(text) + font.size * 1.0f;
        const float h = font.size * 1.25f;
        const Vec2 at{std::clamp(where.x, view.left() + w * 0.5f + 12.0f, view.right() - w * 0.5f - 12.0f), where.y};
        addUi(list, shapes, Ui::Plate, Rect{at.x - w * 0.5f, at.y, w, h}.expanded(12.0f), fill, ink.withAlpha(0.5f));
        gamelib::draw::drawText(list, font, text, at.x, font.topFor(at.y + h * 0.5f), ink, Align::Center);
    };
    for (const game::NpcHere& n : session.npcs()) {
        plate(game::npcName(n.npc), n.feet + Vec2{0.0f, 10.0f}, srgb(0x3A2E4A, 0.85f), srgb(0xFFF2D8), nameFont);
        const gamelib::core::QuestMarker marker = session.quests().markerFor(static_cast<int>(n.npc));
        if (marker != gamelib::core::QuestMarker::None) {
            const float bob = std::sin(time * 3.0f + static_cast<float>(n.npc)) * 5.0f;
            const int kind = marker == gamelib::core::QuestMarker::Available ? 0 : marker == gamelib::core::QuestMarker::ReadyToTurnIn ? 2 : 1;
            const Color c = kind == 0 ? srgb(0xFFC23A) : kind == 2 ? srgb(0x4CC86A) : srgb(0xB8B0C8);
            addIcon(list, shapes, Ui::Marker, n.feet + Vec2{0.0f, -kAvatarQuad * 0.98f + bob}, 66.0f, static_cast<float>(kind), 0.0f, c, c);
        }
    }
    // Monsters: a health bar after a hit, the name of elites.
    const gamelib::draw::Font monsterFont = gamelib::draw::Font::vector(18.0f, 1.25f);
    for (const game::Monster& m : session.monsters()) {
        if (!m.active()) {
            continue;
        }
        const Rect box = lerpR(m.previous, m.box, alpha);
        const bool recent = session.ticks() - m.lastHitTick < 240;
        if (recent || m.elite) {
            const float w = std::max(box.width * 1.15f, 70.0f);
            const float top = monsterScale(m.kind().look, box.height).top;
            const Rect bar{box.center().x - w * 0.5f, box.bottom() - top - 24.0f, w, 12.0f};
            addUi(list, shapes, Ui::Gauge, bar.expanded(12.0f), srgb(0xF25F6E), srgb(0xFFC8C8),
                  static_cast<float>(m.hp) / static_cast<float>(std::max(m.maxHp, 1)), 0.0f);
            const std::string name = std::format("Lv.{} {}{}", m.level, m.elite ? "elite " : "", m.kind().name);
            plate(name, {box.center().x, box.bottom() + 8.0f}, m.elite ? srgb(0x5A2E6A, 0.85f) : srgb(0x2E2A3A, 0.72f),
                  m.elite ? srgb(0xFFD86A) : srgb(0xFFFFFF), monsterFont);
        }
    }
    // Portal names near the hero.
    const game::MapDef& def = session.map();
    const Vec2 heroFeet = session.hero().body.box.bottomCenter();
    for (int i = 0; i < static_cast<int>(def.portals.size()); ++i) {
        const game::Portal& p = def.portals[static_cast<std::size_t>(i)];
        if (std::abs(p.at.x - heroFeet.x) > 380.0f || std::abs(p.at.y - heroFeet.y) > 300.0f) {
            continue;
        }
        const bool open = session.portalOpen(i);
        const std::string_view name = game::mapDef(p.to).name;
        plate(open ? name : std::string_view{"sealed"}, p.at + Vec2{0.0f, -236.0f}, open ? srgb(0x2E4A6A, 0.82f) : srgb(0x4A3A3A, 0.72f),
              open ? srgb(0xD8F2FF) : srgb(0xC8B8B8), nameFont);
    }
}

void WorldView::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
                     const float alpha, const float time, const Vec2 shake) const
{
    list.setView(m_camera.view(shake));
    drawBackdrop(list, shapes, session);
    drawDecos(list, shapes, session);
    drawTerrain(list, shapes, session);
    drawPortals(list, shapes, session, time);
    const Vec2 feet = heroFeet(session, alpha);
    list.setLayer(kLayerBehindFx);
    effects.drawBehind(list, shapes, feet, time);
    drawNpcs(list, shapes, session, time);
    drawLoot(list, shapes, session, alpha, time);
    drawBoss(list, shapes, session, alpha);
    drawMonsters(list, shapes, session, alpha);
    drawHero(list, shapes, session, effects, alpha, time);
    drawShots(list, shapes, session, time);
    list.setLayer(kLayerEffects);
    effects.drawFront(list, shapes, feet, session.hero().body.facing, time);
    drawLabels(list, shapes, session, alpha, time);
    list.setLayer(kLayerWorldUi + 1);
    effects.drawNumbers(list, shapes, time);
    drawLights(list, session, alpha);
    list.setView({});
}

} // namespace realm::app
