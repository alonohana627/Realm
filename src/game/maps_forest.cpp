// The Fernwood: Fernwood Trail from the port, Mossbough - the tree-top
// village of huts on the branches of four giant trees, joined by rope
// bridges - then Canopy Walk through the branches, Glowcap Hollow and
// Thornbark's Grove, where the old tree woke up angry.

#include "maps.hpp"

#include <gamelib/core/tile_sketch.hpp>

namespace realm::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;
using H = HouseStyle;
using gamelib::core::TileSketch;

constexpr int house(const H style)
{
    return static_cast<int>(style);
}

void trail(MapDef& def)
{
    def.name = "Fernwood Trail";
    constexpr int g = 24;
    TileSketch s{124, 30};
    s.ground(0, 123, g, kGround);
    s.row(8, 16, g - 3, kFoothold).row(12, 22, g - 6, kFoothold).column(20, g - 6, g - 1, kRope).row(16, 26, g - 9, kFoothold)
        .column(24, g - 9, g - 7, kLadder);
    s.rampUp(32, g, 2, kRampUp, kGround).ground(34, 54, g - 2, kGround).rampDown(55, g, 2, kRampDown, kGround);
    s.row(36, 46, g - 6, kFoothold).column(38, g - 6, g - 3, kRope).row(42, 52, g - 9, kFoothold).column(50, g - 9, g - 7, kRope);
    s.row(62, 70, g - 3, kFoothold).row(66, 76, g - 6, kFoothold).column(74, g - 6, g - 1, kLadder).row(72, 82, g - 9, kFoothold)
        .column(78, g - 9, g - 7, kRope).row(78, 86, g - 12, kFoothold).column(84, g - 12, g - 10, kRope);
    s.ground(92, 108, g - 4, kGround).column(91, g - 4, g - 1, kLadder).rampDown(109, g, 4, kRampDown, kGround);
    s.row(96, 104, g - 8, kFoothold).column(98, g - 8, g - 5, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::SaltwhistleMarket, .toPortal = 1},
        {.at = feetOn(121.0f, g), .to = MapId::Mossbough, .toPortal = 0},
    };
    def.spawns = {
        {S::BarrelHog, runOn(1, 30, g), 3},      {S::Stumpling, runOn(12, 22, g - 6), 1}, {S::BarrelHog, runOn(16, 26, g - 9), 1},
        {S::Stumpling, runOn(34, 54, g - 2), 3}, {S::BarrelHog, runOn(36, 46, g - 6), 1}, {S::Stumpling, runOn(42, 52, g - 9), 1},
        {S::Stumpling, runOn(58, 90, g), 3},     {S::BarrelHog, runOn(66, 76, g - 6), 1}, {S::Stumpling, runOn(72, 82, g - 9), 1},
        {S::Stumpling, runOn(92, 108, g - 4), 2}, {S::BarrelHog, runOn(96, 104, g - 8), 1},
    };
    def.populationCap = 16;
    const float y = g * kTile;
    def.decos = {
        {K::Sign, {tx(5.0f), y}, 0, 1.0f, false},                 {K::Tree, {tx(10.0f), y}, 0, 1.35f, false},
        {K::Mushroom, {tx(28.0f), y}, 1, 0.9f, false},            {K::Tree, {tx(44.0f), (g - 2) * kTile}, 1, 1.3f, false},
        {K::Bush, {tx(52.0f), (g - 2) * kTile}, 0, 1.0f, true},   {K::Pine, {tx(60.0f), y}, 0, 1.2f, false},
        {K::Grass, {tx(66.0f), y}, 0, 1.0f, true},                {K::Tree, {tx(80.0f), y}, 0, 1.4f, false},
        {K::Mushroom, {tx(88.0f), y}, 0, 1.0f, false},            {K::Pine, {tx(100.0f), (g - 4) * kTile}, 0, 1.1f, false},
        {K::Flowers, {tx(105.0f), (g - 4) * kTile}, 1, 1.0f, true}, {K::Tree, {tx(116.0f), y}, 1, 1.3f, false},
    };
}

void mossbough(MapDef& def)
{
    def.name = "Mossbough";
    def.kind = MapKind::Town;
    constexpr int g = 30;
    TileSketch s{104, 36};
    s.ground(0, 103, g, kGround);
    // Four giant trees (x 14, 40, 66, 90): branch platforms in three tiers,
    // ladders up the trunks, rope bridges between the branches.
    const int trunks[4] = {14, 40, 66, 90};
    for (const int t : trunks) {
        s.column(t, g - 7, g - 1, kLadder);
    }
    // Tier one (g - 7).
    s.row(8, 20, g - 7, kFoothold).row(21, 33, g - 7, kBridge).row(34, 46, g - 7, kFoothold);
    s.row(60, 72, g - 7, kFoothold).row(73, 83, g - 7, kBridge).row(84, 96, g - 7, kFoothold);
    // Tier two (g - 14).
    s.row(9, 19, g - 14, kFoothold).column(15, g - 14, g - 8, kLadder);
    s.row(35, 45, g - 14, kFoothold).column(41, g - 14, g - 8, kLadder).row(46, 59, g - 14, kBridge);
    s.row(60, 71, g - 14, kFoothold).column(65, g - 14, g - 8, kLadder);
    s.row(85, 95, g - 14, kFoothold).column(89, g - 14, g - 8, kLadder);
    s.row(20, 34, g - 14, kBridge);
    // Tier three (g - 21): the chief's lookout and the path up the mountain.
    s.row(36, 46, g - 21, kFoothold).column(39, g - 21, g - 15, kRope);
    s.row(47, 61, g - 21, kBridge).row(62, 72, g - 21, kFoothold).column(68, g - 21, g - 15, kRope);
    s.row(84, 100, g - 21, kFoothold).column(92, g - 21, g - 15, kRope);
    s.row(73, 83, g - 21, kBridge);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::FernwoodTrail, .toPortal = 1},
        {.at = feetOn(101.0f, g), .to = MapId::CanopyWalk, .toPortal = 0},
        {.at = feetOn(98.0f, g - 21), .to = MapId::WindcrestFoothills, .toPortal = 0},
    };
    const float y = g * kTile;
    const float t1 = (g - 7) * kTile;
    const float t2 = (g - 14) * kTile;
    const float t3 = (g - 21) * kTile;
    def.decos = {
        // The giant trees behind everything.
        {K::GiantTree, {tx(14.5f), y}, 0, 1.0f, false},
        {K::GiantTree, {tx(40.5f), y}, 1, 1.0f, false},
        {K::GiantTree, {tx(66.5f), y}, 0, 1.0f, false},
        {K::GiantTree, {tx(90.5f), y}, 1, 1.0f, false},
        // The forest floor.
        {K::Mushroom, {tx(5.0f), y}, 1, 1.0f, false},
        {K::Flowers, {tx(25.0f), y}, 0, 1.0f, true},
        {K::Campfire, {tx(28.0f), y}, 0, 1.0f, false},
        {K::Bench, {tx(31.5f), y}, 0, 1.0f, false},
        {K::Mushroom, {tx(53.0f), y}, 0, 1.2f, false},
        {K::Bush, {tx(78.0f), y}, 1, 1.0f, true},
        {K::Grass, {tx(98.0f), y}, 0, 1.0f, true},
        {K::Lantern, {tx(20.0f), y}, 0, 1.0f, false},
        {K::Lantern, {tx(84.0f), y}, 0, 1.0f, false},
        // Tier one: Lila's herb shop and Bramble's lodge.
        {K::House, {tx(10.5f), t1}, house(H::TreeHut), 0.9f, false, 1},
        {K::House, {tx(44.0f), t1}, house(H::TreeHut), 0.85f, false, 2},
        {K::House, {tx(62.5f), t1}, house(H::Shop), 0.85f, false, 6},
        {K::House, {tx(93.5f), t1}, house(H::TreeHut), 0.85f, false, 3},
        {K::Lantern, {tx(17.0f), t1}, 0, 0.9f, false},
        {K::FlowerPot, {tx(70.0f), t1}, 1, 1.0f, false},
        // Tier two.
        {K::House, {tx(12.5f), t2}, house(H::TreeHut), 0.8f, false, 4},
        {K::House, {tx(68.5f), t2}, house(H::TreeHut), 0.8f, false, 5},
        {K::Lantern, {tx(37.0f), t2}, 0, 0.9f, false},
        {K::Lantern, {tx(87.0f), t2}, 0, 0.9f, false},
        // Tier three: the chief's lookout.
        {K::House, {tx(42.5f), t3}, house(H::Library), 0.75f, false, 7},
        {K::Banner, {tx(64.0f), t3}, 2, 0.9f, false},
        {K::Sign, {tx(95.0f), t3}, 3, 1.0f, false},
        {K::Lantern, {tx(70.0f), t3}, 0, 0.9f, false},
    };
    def.npcs = {
        {NpcId::Lila, feetOn(67.5f, g - 7), -1},
        {NpcId::Bramble, feetOn(37.0f, g - 7), 1},
        {NpcId::Oakhart, feetOn(38.0f, g - 21), 1},
    };
}

void canopy(MapDef& def)
{
    def.name = "Canopy Walk";
    constexpr int g = 28;
    TileSketch s{120, 34};
    s.ground(0, 9, g, kGround).ground(110, 119, g, kGround);
    // A long walk on branches high above the floor, with a safety net of
    // low branches - fall and climb back up the ropes.
    s.row(10, 109, g + 2, kFoothold); // the low floor of branches
    s.row(4, 18, g - 5, kFoothold).column(8, g - 5, g - 1, kLadder).row(19, 30, g - 5, kBridge).row(31, 44, g - 5, kFoothold)
        .column(38, g - 5, g + 1, kRope);
    s.row(45, 56, g - 5, kBridge).row(57, 70, g - 5, kFoothold).column(64, g - 5, g + 1, kRope).row(71, 82, g - 5, kBridge)
        .row(83, 96, g - 5, kFoothold).column(90, g - 5, g + 1, kRope).row(97, 108, g - 5, kBridge).row(109, 116, g - 5, kFoothold)
        .column(112, g - 5, g - 1, kLadder);
    s.row(22, 34, g - 11, kFoothold).column(28, g - 11, g - 6, kRope).row(48, 60, g - 11, kFoothold).column(54, g - 11, g - 6, kRope)
        .row(74, 86, g - 11, kFoothold).column(80, g - 11, g - 6, kRope).row(98, 108, g - 11, kFoothold).column(104, g - 11, g - 6, kRope);
    s.row(35, 47, g - 11, kBridge).row(61, 73, g - 11, kBridge);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::Mossbough, .toPortal = 1},
        {.at = feetOn(117.0f, g), .to = MapId::GlowcapHollow, .toPortal = 0},
    };
    def.spawns = {
        {S::Leafwisp, runOn(12, 40, g + 2), 2},  {S::Glowcap, runOn(31, 44, g - 5), 2}, {S::Glowcap, runOn(57, 70, g - 5), 2},
        {S::Leafwisp, runOn(60, 100, g + 2), 2}, {S::Glowcap, runOn(83, 96, g - 5), 2}, {S::Glowcap, runOn(22, 34, g - 11), 1},
        {S::Leafwisp, runOn(48, 60, g - 11), 1}, {S::Glowcap, runOn(74, 86, g - 11), 1}, {S::Stumpling, runOn(4, 18, g - 5), 1},
        {S::Stumpling, runOn(98, 108, g - 11), 1},
    };
    def.populationCap = 14;
    def.decos = {
        {K::GiantTree, {tx(8.0f), (g + 3) * kTile}, 1, 1.0f, false},  {K::GiantTree, {tx(38.0f), (g + 3) * kTile}, 0, 1.0f, false},
        {K::GiantTree, {tx(64.0f), (g + 3) * kTile}, 1, 1.0f, false}, {K::GiantTree, {tx(90.0f), (g + 3) * kTile}, 0, 1.0f, false},
        {K::GiantTree, {tx(113.0f), (g + 3) * kTile}, 1, 1.0f, false}, {K::Lantern, {tx(16.0f), (g - 5) * kTile}, 0, 0.9f, false},
        {K::Lantern, {tx(68.0f), (g - 5) * kTile}, 0, 0.9f, false},   {K::Mushroom, {tx(42.0f), (g - 5) * kTile}, 1, 0.6f, false},
        {K::Mushroom, {tx(94.0f), (g - 5) * kTile}, 0, 0.6f, false},
    };
}

void hollow(MapDef& def)
{
    def.name = "Glowcap Hollow";
    def.kind = MapKind::Dungeon;
    constexpr int g = 25;
    TileSketch s{120, 30};
    s.ground(0, 119, g, kGround);
    s.row(6, 14, g - 3, kFoothold).row(10, 20, g - 6, kFoothold).column(18, g - 6, g - 1, kRope);
    s.ground(26, 40, g - 3, kGround).rampUp(23, g, 3, kRampUp, kGround).rampDown(41, g, 3, kRampDown, kGround);
    s.row(28, 38, g - 8, kFoothold).column(30, g - 8, g - 4, kLadder).row(32, 40, g - 11, kFoothold).column(37, g - 11, g - 9, kRope);
    s.row(50, 58, g - 3, kFoothold).row(54, 64, g - 6, kFoothold).column(62, g - 6, g - 1, kRope).row(58, 68, g - 9, kFoothold)
        .column(66, g - 9, g - 7, kRope);
    s.ground(76, 92, g - 4, kGround).column(75, g - 4, g - 1, kLadder).rampDown(93, g, 4, kRampDown, kGround);
    s.row(78, 88, g - 8, kFoothold).column(84, g - 8, g - 5, kRope);
    s.row(100, 110, g - 3, kFoothold).column(108, g - 3, g - 1, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::CanopyWalk, .toPortal = 1},
        {.at = feetOn(117.0f, g), .to = MapId::ThornbarkGrove, .toPortal = 0},
    };
    def.spawns = {
        {S::Glowcap, runOn(1, 22, g), 3},        {S::Leafwisp, runOn(10, 20, g - 6), 1}, {S::Stumpling, runOn(26, 40, g - 3), 2},
        {S::Glowcap, runOn(28, 38, g - 8), 1},   {S::Leafwisp, runOn(32, 40, g - 11), 1}, {S::Glowcap, runOn(44, 74, g), 3},
        {S::Stumpling, runOn(54, 64, g - 6), 1}, {S::Leafwisp, runOn(58, 68, g - 9), 1}, {S::Glowcap, runOn(76, 92, g - 4), 2},
        {S::Stumpling, runOn(78, 88, g - 8), 1}, {S::Leafwisp, runOn(96, 114, g), 2},
    };
    def.populationCap = 16;
    def.eliteChance = 8;
    const float y = g * kTile;
    def.decos = {
        {K::Mushroom, {tx(5.0f), y}, 2, 1.3f, false},              {K::Crystal, {tx(16.0f), y}, 1, 0.8f, false},
        {K::Mushroom, {tx(34.0f), (g - 3) * kTile}, 2, 1.0f, false}, {K::Torch, {tx(44.0f), y}, 0, 1.0f, false},
        {K::Mushroom, {tx(52.0f), y}, 2, 1.5f, false},             {K::Grass, {tx(70.0f), y}, 0, 1.0f, true},
        {K::Mushroom, {tx(86.0f), (g - 4) * kTile}, 2, 1.1f, false}, {K::Crystal, {tx(98.0f), y}, 1, 1.0f, false},
        {K::Torch, {tx(112.0f), y}, 0, 1.0f, false},               {K::Mushroom, {tx(104.0f), y}, 2, 0.9f, false},
    };
}

void grove(MapDef& def)
{
    def.name = "Thornbark's Grove";
    def.kind = MapKind::Arena;
    constexpr int g = 19;
    TileSketch s{46, 24};
    s.ground(0, 45, g, kGround);
    s.row(5, 11, g - 4, kFoothold).row(34, 40, g - 4, kFoothold);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::GlowcapHollow, .toPortal = 1, .gate = Gate::AfterBoss},
    };
    def.arena = Arena{.boss = BossId::Thornbark, .bossFeet = feetOn(32.0f, g), .bounds = {tx(1.0f), 0.0f, tx(44.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::GiantTree, {tx(-2.0f), y}, 1, 1.0f, false}, {K::GiantTree, {tx(48.0f), y}, 0, 1.0f, false},
        {K::Torch, {tx(6.0f), y}, 0, 1.0f, false},      {K::Torch, {tx(40.0f), y}, 0, 1.0f, false},
        {K::Mushroom, {tx(15.0f), y}, 2, 0.8f, true},   {K::Grass, {tx(28.0f), y}, 0, 1.0f, true},
    };
}

} // namespace

void buildForest(MapDef& def)
{
    def.region = Region::Forest;
    switch (def.id) {
    case MapId::FernwoodTrail: trail(def); break;
    case MapId::Mossbough: mossbough(def); break;
    case MapId::CanopyWalk: canopy(def); break;
    case MapId::GlowcapHollow: hollow(def); break;
    case MapId::ThornbarkGrove: grove(def); break;
    default: break;
    }
}

} // namespace realm::game
