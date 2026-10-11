// Windcrest Pass: the foothills above Mossbough, the scouts' camp, a long
// climb up the cliffs, the snowy Frostbite Ledge and the Summit Shrine where
// Frostmane guards the second shard.

#include "maps.hpp"

#include <gamelib/core/tile_sketch.hpp>

namespace realm::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;
using gamelib::core::TileSketch;

void foothills(MapDef& def)
{
    def.name = "Windcrest Foothills";
    constexpr int g = 25;
    TileSketch s{126, 30};
    s.ground(0, 126, g, kGround);
    s.rampUp(12, g, 3, kRampUp, kGround).ground(15, 34, g - 3, kGround).rampUp(35, g - 3, 3, kRampUp, kGround)
        .ground(38, 52, g - 6, kGround).rampDown(53, g, 6, kRampDown, kGround);
    s.row(18, 28, g - 7, kFoothold).column(24, g - 7, g - 4, kRope).row(40, 50, g - 10, kFoothold).column(46, g - 10, g - 7, kLadder);
    s.row(64, 72, g - 3, kFoothold).row(68, 78, g - 6, kFoothold).column(76, g - 6, g - 1, kRope).row(74, 84, g - 9, kFoothold)
        .column(80, g - 9, g - 7, kRope);
    s.ground(92, 110, g - 4, kGround).column(91, g - 4, g - 1, kLadder).rampDown(111, g, 4, kRampDown, kGround);
    s.row(96, 106, g - 8, kFoothold).column(100, g - 8, g - 5, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::Mossbough, .toPortal = 2},
        {.at = feetOn(123.0f, g), .to = MapId::ScoutCamp, .toPortal = 0},
    };
    def.spawns = {
        {S::Snowhog, runOn(15, 34, g - 3), 2},   {S::Pebblet, runOn(18, 28, g - 7), 1},  {S::Snowhog, runOn(38, 52, g - 6), 2},
        {S::Pebblet, runOn(40, 50, g - 10), 1},  {S::Snowhog, runOn(58, 90, g), 3},      {S::Pebblet, runOn(68, 78, g - 6), 1},
        {S::Snowhog, runOn(74, 84, g - 9), 1},   {S::Pebblet, runOn(92, 110, g - 4), 2}, {S::Snowhog, runOn(96, 106, g - 8), 1},
        {S::Pebblet, runOn(113, 122, g), 1},
    };
    def.populationCap = 14;
    const float y = g * kTile;
    def.decos = {
        {K::Pine, {tx(6.0f), y}, 0, 1.2f, false},                 {K::Rock, {tx(10.0f), y}, 1, 1.0f, false},
        {K::Pine, {tx(22.0f), (g - 3) * kTile}, 1, 1.0f, false},  {K::Snowpile, {tx(31.0f), (g - 3) * kTile}, 0, 1.0f, true},
        {K::Pine, {tx(45.0f), (g - 6) * kTile}, 1, 1.1f, false},  {K::Rock, {tx(62.0f), y}, 1, 1.2f, false},
        {K::Pine, {tx(86.0f), y}, 1, 1.3f, false},                {K::Snowpile, {tx(98.0f), (g - 4) * kTile}, 0, 1.0f, true},
        {K::Pine, {tx(104.0f), (g - 4) * kTile}, 1, 1.0f, false}, {K::Sign, {tx(119.0f), y}, 3, 1.0f, false},
    };
}

void camp(MapDef& def)
{
    def.name = "Scout's Camp";
    def.kind = MapKind::Town;
    constexpr int g = 20;
    TileSketch s{70, 25};
    s.ground(0, 69, g, kGround);
    s.row(28, 38, g - 5, kFoothold).column(27, g - 5, g - 1, kLadder);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::WindcrestFoothills, .toPortal = 1},
        {.at = feetOn(67.0f, g), .to = MapId::CliffsideClimb, .toPortal = 0},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Pine, {tx(7.0f), y}, 1, 1.2f, false},
        {K::Tent, {tx(15.0f), y}, 0, 1.0f, false},
        {K::Campfire, {tx(22.0f), y}, 0, 1.0f, false},
        {K::Bench, {tx(25.5f), y}, 0, 1.0f, false},
        {K::Tent, {tx(43.0f), y}, 1, 1.1f, false},
        {K::Banner, {tx(35.0f), (g - 5) * kTile}, 2, 0.9f, false},
        {K::Crate, {tx(50.0f), y}, 0, 1.0f, false},
        {K::Barrel, {tx(51.2f), y}, 0, 1.0f, false},
        {K::Lantern, {tx(55.0f), y}, 0, 1.0f, false},
        {K::Pine, {tx(61.0f), y}, 1, 1.3f, false},
        {K::Snowpile, {tx(31.0f), y}, 0, 1.0f, true},
    };
    def.npcs = {{NpcId::Tamsin, feetOn(30.0f, g), 1}};
}

void cliffside(MapDef& def)
{
    def.name = "Cliffside Climb";
    constexpr int g = 52;
    TileSketch s{64, 56};
    s.ground(0, 63, g, kGround);
    // Switchbacks up the cliff: long ledges left and right joined by
    // ladders and ropes, the summit path at the top.
    s.ground(0, 6, 0, kGround).ground(57, 63, 0, kGround); // the cliff walls
    int y = g - 6;
    bool left = true;
    for (int tier = 0; tier < 7; ++tier) {
        const int x0 = left ? 7 : 22;
        const int x1 = left ? 41 : 56;
        s.row(x0, x1, y, kFoothold);
        const int climb = left ? x1 - 2 : x0 + 2;
        s.column(climb, y, y + 5, tier % 2 == 0 ? kLadder : kRope);
        y -= 6;
        left = !left;
    }
    s.row(7, 56, y, kFoothold).column(30, y, y + 5, kLadder);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(9.0f, g), .to = MapId::ScoutCamp, .toPortal = 1},
        {.at = feetOn(53.0f, y), .to = MapId::FrostbiteLedge, .toPortal = 0},
    };
    def.spawns = {
        {S::Pebblet, runOn(7, 56, g), 3},
        {S::Iceling, runOn(7, 41, g - 6), 2},
        {S::Pebblet, runOn(22, 56, g - 12), 2},
        {S::Iceling, runOn(7, 41, g - 18), 2},
        {S::Pebblet, runOn(22, 56, g - 24), 2},
        {S::Iceling, runOn(7, 41, g - 30), 2},
        {S::Pebblet, runOn(22, 56, g - 36), 2},
        {S::Iceling, runOn(7, 41, g - 42), 2},
    };
    def.populationCap = 15;
    def.decos = {
        {K::Pine, {tx(12.0f), g * kTile}, 1, 1.0f, false},   {K::Rock, {tx(48.0f), g * kTile}, 1, 1.2f, false},
        {K::Snowpile, {tx(30.0f), (g - 12) * kTile}, 0, 1.0f, true}, {K::Pine, {tx(52.0f), (g - 24) * kTile}, 1, 0.9f, false},
        {K::Snowpile, {tx(15.0f), (g - 30) * kTile}, 0, 1.0f, true}, {K::Rock, {tx(36.0f), (g - 36) * kTile}, 0, 0.9f, false},
        {K::Sign, {tx(50.0f), static_cast<float>(y) * kTile}, 3, 1.0f, false},
    };
}

void ledge(MapDef& def)
{
    def.name = "Frostbite Ledge";
    constexpr int g = 25;
    TileSketch s{124, 30};
    s.ground(0, 123, g, kGround);
    s.row(6, 14, g - 3, kFoothold).row(10, 20, g - 6, kFoothold).column(18, g - 6, g - 1, kRope).row(14, 24, g - 9, kFoothold)
        .column(22, g - 9, g - 7, kLadder);
    s.ground(30, 50, g - 4, kGround).column(29, g - 4, g - 1, kLadder).rampDown(51, g, 4, kRampDown, kGround);
    s.row(32, 44, g - 8, kFoothold).column(36, g - 8, g - 5, kRope).row(38, 48, g - 12, kFoothold).column(44, g - 12, g - 9, kRope);
    s.row(60, 68, g - 3, kFoothold).row(64, 74, g - 6, kFoothold).column(72, g - 6, g - 1, kRope).row(70, 80, g - 9, kFoothold)
        .column(76, g - 9, g - 7, kRope);
    s.rampUp(86, g, 3, kRampUp, kGround).ground(89, 123, g - 3, kGround);
    s.row(94, 104, g - 7, kFoothold).column(98, g - 7, g - 4, kLadder).row(106, 116, g - 10, kFoothold).column(108, g - 10, g - 8, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::CliffsideClimb, .toPortal = 1},
        {.at = feetOn(121.0f, g - 3), .to = MapId::SummitShrine, .toPortal = 0},
    };
    def.spawns = {
        {S::Iceling, runOn(1, 28, g), 3},       {S::Snowhog, runOn(10, 20, g - 6), 1}, {S::Iceling, runOn(14, 24, g - 9), 1},
        {S::Snowhog, runOn(30, 50, g - 4), 2},  {S::Iceling, runOn(32, 44, g - 8), 1}, {S::Pebblet, runOn(38, 48, g - 12), 1},
        {S::Iceling, runOn(54, 84, g), 3},      {S::Snowhog, runOn(64, 74, g - 6), 1}, {S::Iceling, runOn(70, 80, g - 9), 1},
        {S::Snowhog, runOn(89, 118, g - 3), 2}, {S::Iceling, runOn(94, 104, g - 7), 1}, {S::Pebblet, runOn(106, 116, g - 10), 1},
    };
    def.populationCap = 16;
    def.eliteChance = 8;
    const float y = g * kTile;
    def.decos = {
        {K::Pine, {tx(5.0f), y}, 1, 1.2f, false},                {K::Snowpile, {tx(26.0f), y}, 0, 1.0f, true},
        {K::Pine, {tx(40.0f), (g - 4) * kTile}, 1, 1.0f, false}, {K::Crystal, {tx(47.0f), (g - 4) * kTile}, 2, 0.8f, false},
        {K::Rock, {tx(58.0f), y}, 0, 1.0f, false},               {K::Snowpile, {tx(83.0f), y}, 0, 1.0f, true},
        {K::Pine, {tx(92.0f), (g - 3) * kTile}, 1, 1.3f, false}, {K::Crystal, {tx(112.0f), (g - 3) * kTile}, 2, 1.0f, false},
        {K::Pine, {tx(118.0f), (g - 3) * kTile}, 1, 1.0f, false},
    };
}

void summit(MapDef& def)
{
    def.name = "Summit Shrine";
    def.kind = MapKind::Arena;
    constexpr int g = 19;
    TileSketch s{46, 24};
    s.ground(0, 45, g, kGround);
    s.row(6, 12, g - 5, kFoothold).row(33, 39, g - 5, kFoothold).row(18, 27, g - 9, kFoothold);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::FrostbiteLedge, .toPortal = 1, .gate = Gate::AfterBoss},
    };
    def.arena = Arena{.boss = BossId::Frostmane, .bossFeet = feetOn(32.0f, g), .bounds = {tx(1.0f), 0.0f, tx(44.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::Shrine, {tx(23.0f), y}, 1, 1.4f, false}, {K::Pine, {tx(-1.0f), y}, 1, 1.4f, false},
        {K::Pine, {tx(47.0f), y}, 1, 1.4f, false},   {K::Torch, {tx(7.0f), y}, 0, 1.0f, false},
        {K::Torch, {tx(39.0f), y}, 0, 1.0f, false},  {K::Snowpile, {tx(14.0f), y}, 0, 1.0f, true},
    };
}

} // namespace

void buildMountain(MapDef& def)
{
    def.region = Region::Mountain;
    switch (def.id) {
    case MapId::WindcrestFoothills: foothills(def); break;
    case MapId::ScoutCamp: camp(def); break;
    case MapId::CliffsideClimb: cliffside(def); break;
    case MapId::FrostbiteLedge: ledge(def); break;
    case MapId::SummitShrine: summit(def); break;
    default: break;
    }
}

} // namespace realm::game
