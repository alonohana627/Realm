// The Saltwhistle coast: the port town in two maps (the Docks with the
// warehouses and the harbor office where the ship moors, and the Market with
// the inn, the shops, the training yard, Wren's library and the old door to
// the Undercroft), then Gull Cliffs, Tidepool Shore and Lighthouse Hill.

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

void docks(MapDef& def)
{
    def.name = "Saltwhistle Docks";
    def.kind = MapKind::Town;
    constexpr int g = 21;
    TileSketch s{112, 29};
    // The shore at both ends, wooden piers over the water to the moorings,
    // and the stone quay between them with the sea in front of its wall.
    s.fill(0, 6, g, g + 1, kGround).fill(104, 111, g, g + 1, kGround);
    s.fill(29, 86, g, g + 1, kGround);
    s.row(7, 28, g, kPier).row(87, 103, g, kPier);
    // Walkways over the warehouses, up ladders.
    s.row(30, 44, g - 6, kFoothold).column(29, g - 6, g - 1, kLadder);
    s.row(64, 78, g - 6, kFoothold).column(79, g - 6, g - 1, kLadder);
    s.row(46, 62, g - 9, kFoothold).column(54, g - 9, g - 7, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::GullCliffs, .toPortal = 1},
        {.at = feetOn(109.0f, g), .to = MapId::SaltwhistleMarket, .toPortal = 0},
    };
    def.dock = feetOn(16.0f, g);
    def.water = g * kTile + 44.0f;
    const float y = g * kTile;
    const float sea = y + 112.0f; // the posts' feet, under the water
    def.decos = {
        // The piers stand on posts in the water.
        {K::Post, {tx(7.6f), sea}, 0, 1.25f, false},  {K::Post, {tx(12.0f), sea}, 0, 1.25f, false},
        {K::Post, {tx(20.0f), sea}, 0, 1.25f, false}, {K::Post, {tx(24.5f), sea}, 0, 1.25f, false},
        {K::Post, {tx(28.4f), sea}, 0, 1.25f, false}, {K::Post, {tx(87.6f), sea}, 0, 1.25f, false},
        {K::Post, {tx(92.0f), sea}, 0, 1.25f, false}, {K::Post, {tx(97.0f), sea}, 0, 1.25f, false},
        {K::Post, {tx(102.4f), sea}, 0, 1.25f, false},
        // Mooring posts on the deck edges.
        {K::Post, {tx(9.0f), y}, 0, 0.9f, true},       {K::Post, {tx(26.5f), y}, 1, 0.9f, true},
        {K::Post, {tx(5.0f), y}, 0, 1.0f, true},
        {K::Lantern, {tx(8.0f), y}, 0, 1.0f, false},
        {K::Crate, {tx(11.0f), y}, 0, 1.0f, false},
        {K::Crate, {tx(12.0f), y}, 1, 0.9f, false},
        {K::Crate, {tx(11.5f), y - 74.0f}, 0, 0.8f, false},
        {K::Barrel, {tx(24.0f), y}, 0, 1.0f, false},
        {K::Barrel, {tx(25.0f), y}, 1, 0.9f, false},
        // Warehouse one, the harbor office, warehouse two.
        {K::House, {tx(37.0f), y}, house(H::Warehouse), 1.05f, false, 1},
        {K::Lamp, {tx(45.5f), y}, 0, 1.0f, false},
        {K::House, {tx(54.0f), y}, house(H::Dockhouse), 1.0f, false, 2},
        {K::Banner, {tx(60.0f), y}, 1, 1.0f, false},
        {K::House, {tx(71.0f), y}, house(H::Warehouse), 1.05f, false, 3},
        {K::Crate, {tx(80.5f), y}, 0, 1.0f, false},
        {K::Bunting, {tx(88.0f), y - 230.0f}, 1, 1.4f, false},
        {K::Stall, {tx(86.0f), y}, 2, 1.0f, false},
        {K::Lamp, {tx(92.0f), y}, 0, 1.0f, false},
        {K::Barrel, {tx(92.8f), y}, 0, 1.0f, false},
        {K::Sign, {tx(104.0f), y}, 2, 1.0f, false},
        {K::Post, {tx(100.0f), y}, 1, 1.0f, true},
        // A fishing boat at the east mooring, nets and the day's catch.
        {K::Boat, {tx(96.5f), y + 105.0f}, 1, 0.55f, false},
        {K::Crate, {tx(94.2f), y}, 1, 0.85f, false},
        {K::Shell, {tx(3.5f), y}, 0, 1.0f, true},
        {K::Shell, {tx(107.5f), y}, 1, 1.0f, true},
        {K::Bench, {tx(27.0f), y}, 0, 1.0f, false},
        {K::FlowerPot, {tx(46.6f), y}, 1, 0.9f, false},
        {K::Lantern, {tx(62.5f), y}, 0, 1.0f, false},
        {K::Bunting, {tx(54.0f), y - 330.0f}, 0, 1.3f, false},
        {K::Barrel, {tx(81.6f), y}, 1, 0.9f, false},
        {K::Crate, {tx(81.0f), y - 74.0f}, 1, 0.8f, false},
        {K::Lantern, {tx(36.0f), (g - 6) * kTile}, 0, 0.8f, false},
        {K::FlowerPot, {tx(66.0f), (g - 6) * kTile}, 0, 0.9f, false},
        {K::Banner, {tx(50.0f), (g - 9) * kTile}, 0, 0.8f, false},
    };
    // The hazy harbor behind: the Market's roofs, masts and the lighthouse.
    const float back = y - 40.0f;
    def.skyline = {
        {K::Rock, {tx(6.0f), back + 10.0f}, 0, 2.2f, false},     {K::Lighthouse, {tx(6.0f), back - 50.0f}, 0, 0.7f, false}, {K::House, {tx(18.0f), back}, house(H::Townhouse), 0.55f, false, 8},
        {K::House, {tx(30.0f), back}, house(H::Cottage), 0.5f, false, 9},  {K::Boat, {tx(44.0f), back + 30.0f}, 0, 0.6f, false},
        {K::House, {tx(58.0f), back}, house(H::Inn), 0.55f, false, 10},    {K::House, {tx(72.0f), back}, house(H::Bakery), 0.5f, false, 4},
        {K::Boat, {tx(84.0f), back + 30.0f}, 1, 0.55f, false},               {K::House, {tx(98.0f), back}, house(H::Townhouse), 0.55f, false, 11},
    };
    def.npcs = {
        {NpcId::Marlo, feetOn(19.0f, g), -1},
        {NpcId::Quill, feetOn(50.5f, g), 1},
        {NpcId::Finn, feetOn(83.0f, g), -1},
    };
}

void market(MapDef& def)
{
    def.name = "Saltwhistle Market";
    def.kind = MapKind::Town;
    constexpr int g = 21;
    TileSketch s{132, 26};
    s.ground(0, 131, g, kGround);
    // The upper street on the east hill and a balcony walk.
    s.rampUp(98, g, 2, kRampUp, kGround).ground(100, 131, g - 2, kGround);
    s.row(52, 60, g - 5, kFoothold).column(51, g - 5, g - 1, kLadder);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::SaltwhistleDocks, .toPortal = 1},
        {.at = feetOn(129.0f, g - 2), .to = MapId::FernwoodTrail, .toPortal = 0},
        {.at = feetOn(92.0f, g), .to = MapId::UndercroftStairs, .toPortal = 0, .gate = Gate::Undercroft, .hidden = true},
    };
    const float y = g * kTile;
    const float hill = (g - 2) * kTile;
    def.decos = {
        {K::Lamp, {tx(6.0f), y}, 0, 1.0f, false},
        // The Sleepy Gull inn.
        {K::House, {tx(14.0f), y}, house(H::Inn), 1.05f, false, 0},
        {K::Barrel, {tx(21.0f), y}, 0, 1.0f, false},
        {K::FlowerPot, {tx(22.0f), y}, 1, 1.0f, false},
        // Dottie's general store.
        {K::House, {tx(29.0f), y}, house(H::Shop), 1.0f, false, 1},
        {K::Stall, {tx(36.5f), y}, 0, 1.0f, false},
        // The square: fountain, bunting, benches.
        {K::Bunting, {tx(44.0f), y - 240.0f}, 0, 1.6f, false},
        {K::Fountain, {tx(44.0f), y}, 0, 1.0f, false},
        {K::Bench, {tx(49.0f), y}, 0, 1.0f, false},
        {K::Tree, {tx(57.0f), (g - 5) * kTile + 4.0f}, 1, 0.75f, false},
        // Garrick's armory.
        {K::House, {tx(63.5f), y}, house(H::Armory), 1.0f, false, 2},
        {K::Anvil, {tx(69.5f), y}, 0, 1.0f, false},
        // Kestrel's training yard.
        {K::Fence, {tx(75.0f), y}, 0, 1.0f, false},
        {K::Banner, {tx(73.0f), y}, 0, 1.0f, false},
        {K::Post, {tx(78.0f), y}, 1, 1.0f, false},
        {K::Banner, {tx(82.0f), y}, 0, 1.0f, false},
        // The old door to the Undercroft.
        {K::Shrine, {tx(92.0f), y}, 0, 1.0f, false},
        {K::Lamp, {tx(96.0f), y}, 0, 1.0f, false},
        // Up the hill: Wren's library tower and a bakery.
        {K::House, {tx(108.0f), hill}, house(H::Library), 1.05f, false, 3},
        {K::Flowers, {tx(114.0f), hill}, 1, 0.9f, true},
        {K::House, {tx(121.0f), hill}, house(H::Bakery), 0.95f, false, 4},
        {K::Lamp, {tx(126.5f), hill}, 0, 1.0f, false},
        {K::Grass, {tx(102.0f), hill}, 0, 1.0f, true},
    };
    const float back = y - 70.0f;
    def.skyline = {
        {K::House, {tx(4.0f), back}, house(H::Townhouse), 0.6f, false, 5},  {K::House, {tx(16.0f), back}, house(H::Warehouse), 0.55f, false, 6},
        {K::Lighthouse, {tx(28.0f), back}, 0, 0.45f, false},                {K::House, {tx(40.0f), back}, house(H::Townhouse), 0.6f, false, 7},
        {K::Tree, {tx(52.0f), back}, 0, 0.8f, false},                       {K::House, {tx(60.0f), back}, house(H::Cottage), 0.55f, false, 8},
        {K::House, {tx(74.0f), back}, house(H::Townhouse), 0.6f, false, 9}, {K::House, {tx(88.0f), back}, house(H::Inn), 0.55f, false, 10},
        {K::Pine, {tx(100.0f), back}, 0, 0.75f, false},                     {K::House, {tx(114.0f), back}, house(H::Townhouse), 0.6f, false, 11},
        {K::Tree, {tx(128.0f), back}, 1, 0.8f, false},
    };
    def.npcs = {
        {NpcId::Rosa, feetOn(18.5f, g), 1},
        {NpcId::Dottie, feetOn(34.0f, g), -1},
        {NpcId::Garrick, feetOn(68.0f, g), -1},
        {NpcId::Kestrel, feetOn(79.5f, g), -1},
        {NpcId::Wren, feetOn(103.5f, g - 2), 1},
    };
}

void gullCliffs(MapDef& def)
{
    def.name = "Gull Cliffs";
    constexpr int g = 25;
    TileSketch s{118, 30};
    s.ground(0, 117, g, kGround);
    s.ground(0, 20, g - 6, kGround).column(21, g - 6, g - 1, kLadder);
    s.row(6, 16, g - 10, kFoothold).column(14, g - 10, g - 7, kRope);
    s.row(28, 36, g - 3, kFoothold).row(33, 42, g - 6, kFoothold).column(40, g - 6, g - 1, kRope).row(38, 48, g - 9, kFoothold)
        .column(44, g - 9, g - 7, kLadder);
    s.rampUp(54, g, 3, kRampUp, kGround).ground(57, 76, g - 3, kGround).rampDown(77, g, 3, kRampDown, kGround);
    s.row(60, 70, g - 7, kFoothold).column(62, g - 7, g - 4, kRope).row(64, 74, g - 10, kFoothold).column(72, g - 10, g - 8, kRope);
    s.row(84, 92, g - 3, kFoothold).row(88, 97, g - 6, kFoothold).column(95, g - 6, g - 1, kLadder).row(92, 102, g - 9, kFoothold)
        .column(98, g - 9, g - 7, kRope);
    s.ground(106, 117, g - 2, kGround).rampUp(104, g, 2, kRampUp, kGround);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g - 6), .to = MapId::TidepoolShore, .toPortal = 1},
        {.at = feetOn(115.0f, g - 2), .to = MapId::SaltwhistleDocks, .toPortal = 0},
    };
    def.spawns = {
        {S::Pinchy, runOn(22, 52, g), 3},        {S::Puffgull, runOn(4, 18, g - 6), 2},  {S::Pinchy, runOn(28, 36, g - 3), 1},
        {S::Puffgull, runOn(33, 42, g - 6), 1},  {S::Pinchy, runOn(38, 48, g - 9), 1},   {S::Pinchy, runOn(57, 76, g - 3), 2},
        {S::Puffgull, runOn(60, 70, g - 7), 1},  {S::Pinchy, runOn(64, 74, g - 10), 1},  {S::Pinchy, runOn(79, 103, g), 3},
        {S::Puffgull, runOn(88, 97, g - 6), 1},  {S::Pinchy, runOn(92, 102, g - 9), 1},
    };
    def.populationCap = 15;
    const float y = g * kTile;
    def.decos = {
        {K::Sign, {tx(4.5f), (g - 6) * kTile}, 2, 1.0f, false}, {K::Pine, {tx(10.0f), (g - 6) * kTile}, 0, 0.9f, false},
        {K::Rock, {tx(24.0f), y}, 0, 1.0f, false},               {K::Shell, {tx(31.0f), y}, 0, 1.0f, true},
        {K::Grass, {tx(46.0f), y}, 0, 1.0f, true},               {K::Rock, {tx(52.0f), y}, 1, 1.0f, false},
        {K::Pine, {tx(66.0f), (g - 3) * kTile}, 0, 1.0f, false}, {K::Bush, {tx(73.0f), (g - 3) * kTile}, 0, 1.0f, true},
        {K::Shell, {tx(82.0f), y}, 1, 1.0f, true},               {K::Rock, {tx(90.0f), y}, 0, 1.2f, false},
        {K::Post, {tx(110.0f), (g - 2) * kTile}, 0, 1.0f, true}, {K::Lantern, {tx(112.0f), (g - 2) * kTile}, 0, 1.0f, false},
    };
}

void tidepool(MapDef& def)
{
    def.name = "Tidepool Shore";
    constexpr int g = 24;
    TileSketch s{126, 30};
    s.ground(0, 125, g, kGround);
    s.row(8, 18, g - 3, kFoothold).row(14, 24, g - 6, kFoothold).column(22, g - 6, g - 1, kRope);
    s.ground(30, 46, g - 3, kGround).rampUp(27, g, 3, kRampUp, kGround).rampDown(47, g, 3, kRampDown, kGround);
    s.row(32, 44, g - 8, kFoothold).column(34, g - 8, g - 4, kLadder).row(36, 42, g - 11, kFoothold).column(40, g - 11, g - 9, kRope);
    s.row(56, 64, g - 3, kFoothold).row(60, 70, g - 6, kFoothold).row(66, 74, g - 9, kFoothold).column(72, g - 9, g - 1, kRope);
    s.ground(82, 98, g - 4, kGround).column(81, g - 4, g - 1, kLadder).column(99, g - 4, g - 1, kLadder);
    s.row(86, 96, g - 8, kFoothold).column(90, g - 8, g - 5, kRope);
    s.row(106, 114, g - 3, kFoothold).row(110, 120, g - 6, kFoothold).column(118, g - 6, g - 1, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::LighthouseHill, .toPortal = 0},
        {.at = feetOn(123.0f, g), .to = MapId::GullCliffs, .toPortal = 0},
    };
    def.spawns = {
        {S::Pinchy, runOn(1, 26, g), 3},          {S::BarrelHog, runOn(30, 46, g - 3), 2}, {S::Pinchy, runOn(14, 24, g - 6), 1},
        {S::BarrelHog, runOn(32, 44, g - 8), 1},  {S::Pinchy, runOn(50, 80, g), 3},        {S::BarrelHog, runOn(60, 70, g - 6), 1},
        {S::Pinchy, runOn(66, 74, g - 9), 1},     {S::BarrelHog, runOn(82, 98, g - 4), 2}, {S::Pinchy, runOn(86, 96, g - 8), 1},
        {S::BarrelHog, runOn(100, 124, g), 2},    {S::Pinchy, runOn(110, 120, g - 6), 1},
    };
    def.populationCap = 16;
    const float y = g * kTile;
    def.decos = {
        {K::Shell, {tx(5.0f), y}, 0, 1.0f, true},                 {K::Rock, {tx(12.0f), y}, 1, 1.0f, false},
        {K::Reeds, {tx(26.0f), y}, 0, 1.0f, true},                {K::Bush, {tx(38.0f), (g - 3) * kTile}, 1, 1.0f, true},
        {K::Shell, {tx(52.0f), y}, 1, 1.0f, true},                {K::Rock, {tx(77.0f), y}, 0, 1.1f, false},
        {K::Pine, {tx(92.0f), (g - 4) * kTile}, 0, 1.0f, false},  {K::Grass, {tx(85.0f), (g - 4) * kTile}, 0, 1.0f, true},
        {K::Shell, {tx(103.0f), y}, 0, 1.0f, true},               {K::Post, {tx(121.0f), y}, 0, 1.0f, true},
    };
}

void lighthouse(MapDef& def)
{
    def.name = "Lighthouse Hill";
    constexpr int g = 26;
    TileSketch s{108, 32};
    s.ground(0, 107, g, kGround);
    // The hill climbs west to the lighthouse.
    s.ground(0, 30, g - 9, kGround).column(31, g - 9, g - 5, kLadder);
    s.ground(32, 52, g - 4, kGround).rampDown(53, g, 4, kRampDown, kGround);
    s.row(36, 46, g - 8, kFoothold).column(44, g - 8, g - 5, kRope);
    s.row(10, 20, g - 13, kFoothold).column(18, g - 13, g - 10, kRope);
    s.row(62, 70, g - 3, kFoothold).row(66, 76, g - 6, kFoothold).column(74, g - 6, g - 1, kRope).row(72, 82, g - 9, kFoothold)
        .column(78, g - 9, g - 7, kLadder);
    s.row(88, 96, g - 3, kFoothold).column(94, g - 3, g - 1, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(105.0f, g), .to = MapId::TidepoolShore, .toPortal = 0},
    };
    def.spawns = {
        {S::BarrelHog, runOn(55, 104, g), 4},     {S::Puffgull, runOn(32, 52, g - 4), 2}, {S::BarrelHog, runOn(36, 46, g - 8), 1},
        {S::Puffgull, runOn(4, 28, g - 9), 2},    {S::BarrelHog, runOn(10, 20, g - 13), 1}, {S::Puffgull, runOn(66, 76, g - 6), 1},
        {S::BarrelHog, runOn(72, 82, g - 9), 1},  {S::Puffgull, runOn(88, 96, g - 3), 1},
    };
    def.populationCap = 13;
    def.eliteChance = 7;
    const float y = g * kTile;
    const float top = (g - 9) * kTile;
    def.decos = {
        {K::Lighthouse, {tx(9.0f), top}, 0, 1.0f, false},     {K::Fence, {tx(22.0f), top}, 0, 1.0f, true},
        {K::Bush, {tx(26.0f), top}, 1, 1.0f, true},           {K::Flowers, {tx(28.5f), top}, 0, 1.0f, true},
        {K::Rock, {tx(38.0f), (g - 4) * kTile}, 1, 1.0f, false}, {K::Pine, {tx(49.0f), (g - 4) * kTile}, 0, 1.0f, false},
        {K::Grass, {tx(58.0f), y}, 0, 1.0f, true},            {K::Rock, {tx(85.0f), y}, 0, 1.0f, false},
        {K::Pine, {tx(100.0f), y}, 0, 1.1f, false},           {K::Shell, {tx(65.0f), y}, 0, 1.0f, true},
    };
}

} // namespace

void buildCoast(MapDef& def)
{
    def.region = Region::Coast;
    switch (def.id) {
    case MapId::SaltwhistleDocks: docks(def); break;
    case MapId::SaltwhistleMarket: market(def); break;
    case MapId::GullCliffs: gullCliffs(def); break;
    case MapId::TidepoolShore: tidepool(def); break;
    case MapId::LighthouseHill: lighthouse(def); break;
    default: break;
    }
}

} // namespace realm::game
