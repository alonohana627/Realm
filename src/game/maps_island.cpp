// Petal Isle, the starting island: the hamlet of Petalbrook with its
// windmill, Sprout Meadow (the first field), Puddle Path, Breezy Bluff and
// Petal Pier, where Captain Marlo's ship sails for Saltwhistle.

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

void petalbrook(MapDef& def)
{
    def.name = "Petalbrook";
    def.kind = MapKind::Town;
    constexpr int g = 21;
    TileSketch s{100, 26};
    s.ground(0, 99, g, kGround);
    // The windmill hill at the east end.
    s.rampUp(78, g, 2, kRampUp, kGround).ground(80, 99, g - 2, kGround);
    // A lookout over the square, up a ladder.
    s.row(30, 36, g - 4, kFoothold).column(29, g - 4, g - 1, kLadder);
    // Pip's tree swing platform.
    s.row(56, 60, g - 3, kFoothold).column(61, g - 3, g - 1, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::SproutMeadow, .toPortal = 0},
        {.at = feetOn(97.0f, g - 2), .to = MapId::PuddlePath, .toPortal = 0},
    };
    const float y = g * kTile;
    const float hill = (g - 2) * kTile;
    def.decos = {
        {K::Sign, {tx(5.5f), y}, 0, 1.0f, false},
        {K::Bush, {tx(7.0f), y}, 0, 0.9f, true},
        // The bakery by the west gate, bread on the sill.
        {K::House, {tx(10.5f), y}, house(H::Bakery), 0.85f, false, 8},
        {K::Barrel, {tx(13.6f), y}, 0, 0.9f, false},
        {K::Tree, {tx(14.8f), y}, 0, 1.15f, false},
        {K::Flowers, {tx(12.0f), y}, 0, 0.9f, true},
        // Nana Rue's cottage with its garden.
        {K::House, {tx(18.0f), y}, house(H::Cottage), 1.0f, false, 1},
        {K::FlowerPot, {tx(23.5f), y}, 0, 1.0f, false},
        {K::Fence, {tx(13.5f), y}, 0, 1.0f, true},
        {K::Lamp, {tx(25.5f), y}, 0, 1.0f, false},
        // The square: a well, the lookout, bunting, a bench.
        {K::Well, {tx(33.0f), y}, 0, 1.0f, false},
        {K::Tree, {tx(36.5f), (g - 4) * kTile + 4.0f}, 1, 0.75f, false},
        {K::Bunting, {tx(39.0f), y - 230.0f}, 0, 1.4f, false},
        {K::Bench, {tx(40.5f), y}, 0, 1.0f, false},
        {K::FlowerPot, {tx(42.3f), y}, 1, 0.9f, false},
        {K::Grass, {tx(37.5f), y}, 0, 1.0f, true},
        // Hettie's shop.
        {K::House, {tx(47.0f), y}, house(H::Shop), 1.0f, false, 2},
        {K::Barrel, {tx(52.4f), y}, 0, 1.0f, false},
        {K::Crate, {tx(53.3f), y}, 0, 0.9f, false},
        // Pip's family home and the swing tree.
        {K::House, {tx(65.5f), y}, house(H::Townhouse), 0.95f, false, 3},
        {K::Tree, {tx(58.0f), y}, 0, 1.35f, false},
        {K::Flowers, {tx(70.5f), y}, 1, 0.9f, true},
        {K::Lamp, {tx(72.5f), y}, 0, 1.0f, false},
        {K::Stall, {tx(75.8f), y}, 1, 0.9f, false},
        {K::Bush, {tx(73.8f), y}, 1, 0.9f, true},
        {K::Crate, {tx(77.4f), y}, 0, 0.8f, false},
        // The windmill on the hill, the miller's cottage beside it.
        {K::House, {tx(83.6f), hill}, house(H::Cottage), 0.72f, false, 10},
        {K::Windmill, {tx(91.0f), hill}, 0, 1.0f, false},
        {K::Fence, {tx(86.5f), hill}, 0, 0.9f, true},
        {K::Flowers, {tx(94.5f), hill}, 0, 0.9f, true},
        {K::Flowers, {tx(62.0f), y}, 0, 0.8f, true},
        {K::Grass, {tx(54.5f), y}, 0, 1.0f, true},
        {K::Lamp, {tx(8.0f), y}, 0, 1.0f, false},
        {K::Grass, {tx(4.0f), y}, 0, 1.0f, true},
        {K::Grass, {tx(27.0f), y}, 0, 1.0f, true},
        {K::Grass, {tx(44.0f), y}, 0, 1.0f, true},
        {K::Grass, {tx(81.0f), hill}, 0, 1.0f, true},
    };
    // The hazy back row: more of the village and the orchards behind.
    const float back = y - 50.0f;
    def.skyline = {
        {K::Tree, {tx(1.0f), back}, 0, 0.85f, false},     {K::House, {tx(7.0f), back}, house(H::Cottage), 0.62f, false, 4},
        {K::Pine, {tx(13.0f), back}, 0, 0.8f, false},     {K::House, {tx(20.0f), back}, house(H::Townhouse), 0.6f, false, 9},
        {K::Tree, {tx(26.0f), back}, 1, 0.85f, false},    {K::House, {tx(32.0f), back}, house(H::Bakery), 0.6f, false, 5},
        {K::Pine, {tx(38.0f), back}, 0, 0.8f, false},     {K::House, {tx(44.0f), back}, house(H::Inn), 0.62f, false, 11},
        {K::Tree, {tx(51.0f), back}, 0, 0.9f, false},     {K::House, {tx(57.0f), back}, house(H::Cottage), 0.6f, false, 6},
        {K::Tree, {tx(63.0f), back}, 1, 0.85f, false},    {K::House, {tx(70.0f), back}, house(H::Townhouse), 0.58f, false, 7},
        {K::Pine, {tx(76.0f), back}, 0, 0.8f, false},     {K::House, {tx(82.0f), back - 60.0f}, house(H::Library), 0.58f, false, 2},
        {K::Tree, {tx(89.0f), back - 80.0f}, 0, 0.85f, false}, {K::Pine, {tx(96.0f), back - 90.0f}, 0, 0.8f, false},
    };
    def.npcs = {
        {NpcId::NanaRue, feetOn(24.0f, g), -1},
        {NpcId::Hettie, feetOn(43.0f, g), 1},
        {NpcId::Pip, feetOn(58.0f, g - 3), 1},
    };
}

void sproutMeadow(MapDef& def)
{
    def.name = "Sprout Meadow";
    constexpr int g = 23;
    TileSketch s{110, 28};
    s.ground(0, 109, g, kGround);
    s.rampUp(14, g, 1, kRampUp, kGround).ground(15, 30, g - 1, kGround).rampDown(31, g, 1, kRampDown, kGround);
    s.row(18, 26, g - 4, kFoothold).column(22, g - 4, g - 2, kRope);
    s.row(36, 46, g - 3, kFoothold).row(42, 52, g - 6, kFoothold).column(48, g - 6, g - 4, kLadder).column(39, g - 3, g - 1, kRope);
    s.row(38, 46, g - 9, kFoothold).column(44, g - 9, g - 7, kRope);
    s.rampUp(58, g, 2, kRampUp, kGround).ground(60, 74, g - 2, kGround).rampDown(75, g, 2, kRampDown, kGround);
    s.row(62, 71, g - 5, kFoothold).column(66, g - 5, g - 3, kRope).row(64, 69, g - 8, kFoothold).column(68, g - 8, g - 6, kLadder);
    s.row(82, 92, g - 3, kFoothold).column(84, g - 3, g - 1, kRope).row(88, 98, g - 6, kFoothold).column(96, g - 6, g - 4, kRope);
    s.row(100, 106, g - 3, kFoothold);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(106.5f, g), .to = MapId::Petalbrook, .toPortal = 0},
        {.at = feetOn(41.0f, g - 9), .to = MapId::Petalbrook, .toPortal = 0}, // a little shortcut home from the top
    };
    def.spawns = {
        {S::Sproutcap, runOn(1, 13, g), 2},       {S::Sproutcap, runOn(33, 57, g), 3},  {S::Dewdrop, runOn(15, 30, g - 1), 2},
        {S::Sproutcap, runOn(18, 26, g - 4), 1},  {S::Dewdrop, runOn(36, 46, g - 3), 1}, {S::Sproutcap, runOn(42, 52, g - 6), 1},
        {S::Sproutcap, runOn(60, 74, g - 2), 2},  {S::Dewdrop, runOn(62, 71, g - 5), 1}, {S::Sproutcap, runOn(77, 104, g), 3},
        {S::Dewdrop, runOn(82, 92, g - 3), 1},    {S::Sproutcap, runOn(88, 98, g - 6), 1},
    };
    def.populationCap = 16;
    const float y = g * kTile;
    def.decos = {
        {K::Sign, {tx(103.0f), y}, 0, 1.0f, false},       {K::Tree, {tx(8.0f), y}, 0, 1.25f, false},
        {K::Flowers, {tx(11.0f), y}, 0, 1.0f, true},      {K::Tree, {tx(25.0f), (g - 1) * kTile}, 1, 1.0f, false},
        {K::Bush, {tx(19.0f), (g - 1) * kTile}, 0, 1.0f, true}, {K::Mushroom, {tx(34.0f), y}, 0, 1.0f, false},
        {K::Tree, {tx(51.0f), y}, 0, 1.3f, false},        {K::Grass, {tx(45.0f), y}, 0, 1.0f, true},
        {K::Rock, {tx(55.0f), y}, 0, 1.0f, false},        {K::Tree, {tx(70.0f), (g - 2) * kTile}, 1, 1.15f, false},
        {K::Flowers, {tx(63.0f), (g - 2) * kTile}, 1, 1.0f, true}, {K::Mushroom, {tx(79.0f), y}, 1, 0.8f, false},
        {K::Tree, {tx(86.0f), y}, 0, 1.1f, false},        {K::Bush, {tx(93.0f), y}, 1, 1.0f, true},
        {K::Grass, {tx(99.0f), y}, 0, 1.0f, true},        {K::Flowers, {tx(60.0f), y}, 0, 1.0f, true},
        {K::Fence, {tx(2.5f), y}, 0, 1.0f, false},
    };
}

void puddlePath(MapDef& def)
{
    def.name = "Puddle Path";
    constexpr int g = 24;
    TileSketch s{124, 30};
    s.ground(0, 123, g, kGround);
    s.row(6, 14, g - 3, kFoothold).row(10, 18, g - 6, kFoothold).column(16, g - 6, g - 4, kRope).column(8, g - 3, g - 1, kRope);
    s.ground(22, 40, g - 4, kGround).column(21, g - 4, g - 1, kLadder).rampDown(41, g, 4, kRampDown, kGround);
    s.row(25, 33, g - 8, kFoothold).column(30, g - 8, g - 5, kRope).row(28, 37, g - 11, kFoothold).column(35, g - 11, g - 9, kRope);
    s.row(50, 58, g - 3, kFoothold).row(55, 64, g - 6, kFoothold).column(62, g - 6, g - 1, kRope).row(60, 70, g - 9, kFoothold)
        .column(66, g - 9, g - 7, kLadder);
    s.row(76, 84, g - 3, kFoothold).column(78, g - 3, g - 1, kRope).row(82, 92, g - 6, kFoothold).column(90, g - 6, g - 4, kRope);
    s.rampUp(98, g, 2, kRampUp, kGround).ground(100, 123, g - 2, kGround);
    s.row(104, 112, g - 6, kFoothold).column(110, g - 6, g - 3, kLadder);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::Petalbrook, .toPortal = 1},
        {.at = feetOn(120.5f, g - 2), .to = MapId::BreezyBluff, .toPortal = 0},
    };
    def.spawns = {
        {S::Dewdrop, runOn(1, 20, g), 2},         {S::Dewdrop, runOn(6, 14, g - 3), 1},   {S::Swirlsnail, runOn(22, 40, g - 4), 2},
        {S::Dewdrop, runOn(25, 33, g - 8), 1},    {S::Swirlsnail, runOn(28, 37, g - 11), 1}, {S::Dewdrop, runOn(45, 74, g), 3},
        {S::Swirlsnail, runOn(55, 64, g - 6), 1}, {S::Dewdrop, runOn(60, 70, g - 9), 1},  {S::Swirlsnail, runOn(75, 96, g), 2},
        {S::Dewdrop, runOn(82, 92, g - 6), 1},    {S::Swirlsnail, runOn(100, 119, g - 2), 2}, {S::Dewdrop, runOn(104, 112, g - 6), 1},
    };
    def.populationCap = 17;
    const float y = g * kTile;
    def.decos = {
        {K::Reeds, {tx(5.0f), y}, 0, 1.0f, true},         {K::Tree, {tx(13.0f), y}, 1, 1.2f, false},
        {K::Rock, {tx(19.0f), y}, 1, 0.9f, false},        {K::Tree, {tx(26.0f), (g - 4) * kTile}, 0, 1.2f, false},
        {K::Mushroom, {tx(38.0f), (g - 4) * kTile}, 0, 0.9f, false}, {K::Reeds, {tx(47.0f), y}, 0, 1.0f, true},
        {K::Tree, {tx(53.0f), y}, 0, 1.3f, false},        {K::Bush, {tx(68.0f), y}, 0, 1.0f, true},
        {K::Reeds, {tx(73.0f), y}, 0, 1.0f, true},        {K::Tree, {tx(87.0f), y}, 1, 1.2f, false},
        {K::Rock, {tx(95.0f), y}, 0, 1.0f, false},        {K::Tree, {tx(106.0f), (g - 2) * kTile}, 0, 1.2f, false},
        {K::Flowers, {tx(114.0f), (g - 2) * kTile}, 0, 1.0f, true}, {K::Sign, {tx(117.0f), (g - 2) * kTile}, 0, 1.0f, false},
    };
}

void breezyBluff(MapDef& def)
{
    def.name = "Breezy Bluff";
    constexpr int g = 26;
    TileSketch s{104, 32};
    s.ground(0, 103, g, kGround);
    // A tall bluff climbed in tiers.
    s.ground(30, 64, g - 5, kGround).column(29, g - 5, g - 1, kLadder).rampDown(65, g, 5, kRampDown, kGround);
    s.ground(38, 56, g - 10, kGround).column(37, g - 10, g - 6, kLadder);
    s.ground(43, 52, g - 14, kGround).column(53, g - 14, g - 11, kRope).column(42, g - 14, g - 11, kRope);
    s.column(57, g - 10, g - 6, kLadder); // back up from the east side
    s.row(8, 16, g - 3, kFoothold).row(14, 22, g - 6, kFoothold).column(20, g - 6, g - 1, kRope).row(18, 26, g - 9, kFoothold)
        .column(24, g - 9, g - 7, kRope);
    s.row(72, 80, g - 3, kFoothold).row(78, 88, g - 6, kFoothold).column(86, g - 6, g - 1, kLadder).row(84, 94, g - 9, kFoothold)
        .column(90, g - 9, g - 7, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::PuddlePath, .toPortal = 1},
        {.at = feetOn(100.5f, g), .to = MapId::PetalPier, .toPortal = 0},
    };
    def.spawns = {
        {S::Swirlsnail, runOn(1, 28, g), 3},       {S::Sproutcap, runOn(8, 16, g - 3), 1},  {S::Dewdrop, runOn(14, 22, g - 6), 1},
        {S::Swirlsnail, runOn(18, 26, g - 9), 1},  {S::Swirlsnail, runOn(30, 37, g - 5), 1}, {S::Dewdrop, runOn(57, 64, g - 5), 1},
        {S::Swirlsnail, runOn(38, 42, g - 10), 1}, {S::Swirlsnail, runOn(53, 56, g - 10), 1}, {S::Sproutcap, runOn(43, 52, g - 14), 1}, {S::Swirlsnail, runOn(70, 99, g), 3},
        {S::Dewdrop, runOn(78, 88, g - 6), 1},     {S::Swirlsnail, runOn(84, 94, g - 9), 1},
    };
    def.populationCap = 15;
    def.eliteChance = 6;
    const float y = g * kTile;
    def.decos = {
        {K::Tree, {tx(6.0f), y}, 0, 1.2f, false},          {K::Bush, {tx(12.0f), y}, 1, 1.0f, true},
        {K::Rock, {tx(26.0f), y}, 1, 1.1f, false},         {K::Tree, {tx(33.0f), (g - 5) * kTile}, 1, 1.0f, false},
        {K::Grass, {tx(60.0f), (g - 5) * kTile}, 0, 1.0f, true}, {K::Pine, {tx(41.0f), (g - 10) * kTile}, 0, 0.9f, false},
        {K::Flowers, {tx(48.0f), (g - 14) * kTile}, 1, 1.0f, true}, {K::Sign, {tx(47.0f), (g - 14) * kTile}, 1, 1.0f, false},
        {K::Tree, {tx(76.0f), y}, 0, 1.3f, false},         {K::Rock, {tx(94.0f), y}, 0, 1.0f, false},
        {K::Bush, {tx(97.0f), y}, 0, 1.0f, true},
    };
}

void petalPier(MapDef& def)
{
    def.name = "Petal Pier";
    def.kind = MapKind::Town;
    constexpr int g = 20;
    TileSketch s{72, 26};
    // The beach, then the pier out over the water.
    s.ground(0, 40, g, kGround);
    s.row(41, 71, g, kFoothold);
    s.row(18, 26, g - 4, kFoothold).column(17, g - 4, g - 1, kLadder);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::BreezyBluff, .toPortal = 1},
    };
    def.dock = feetOn(60.0f, g);
    const float y = g * kTile;
    def.decos = {
        {K::Sign, {tx(8.0f), y}, 1, 1.0f, false},
        {K::House, {tx(22.0f), y}, house(H::Dockhouse), 0.95f, false, 1},
        {K::Barrel, {tx(28.0f), y}, 0, 1.0f, false},
        {K::Crate, {tx(29.0f), y}, 0, 1.0f, false},
        {K::Crate, {tx(29.4f), y - 74.0f}, 0, 0.8f, false},
        {K::Shell, {tx(12.0f), y}, 0, 1.0f, true},
        {K::Shell, {tx(35.0f), y}, 1, 1.0f, true},
        {K::Lamp, {tx(39.5f), y}, 0, 1.0f, false},
        {K::Post, {tx(44.0f), y}, 0, 1.0f, true},
        {K::Post, {tx(52.0f), y}, 0, 1.0f, true},
        {K::Post, {tx(68.0f), y}, 0, 1.0f, true},
        {K::Lantern, {tx(55.0f), y}, 0, 1.0f, false},
    };
    def.npcs = {{NpcId::Marlo, feetOn(48.0f, g), 1}};
}

} // namespace

void buildIsland(MapDef& def)
{
    def.region = Region::Island;
    switch (def.id) {
    case MapId::Petalbrook: petalbrook(def); break;
    case MapId::SproutMeadow: sproutMeadow(def); break;
    case MapId::PuddlePath: puddlePath(def); break;
    case MapId::BreezyBluff: breezyBluff(def); break;
    case MapId::PetalPier: petalPier(def); break;
    default: break;
    }
}

} // namespace realm::game
