// The Undercroft below Saltwhistle: the old stairs where Nix the explorer
// camps, the Glimmer Tunnels, the Sunken Archive of a forgotten library,
// the Deep Hollow and the Heart of the Gloom, Murkmaw's lair.

#include "maps.hpp"

#include <gamelib/core/tile_sketch.hpp>

namespace realm::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;
using gamelib::core::TileSketch;

void stairs(MapDef& def)
{
    def.name = "Undercroft Stairs";
    def.kind = MapKind::Dungeon;
    constexpr int g = 22;
    TileSketch s{80, 26};
    s.ground(0, 79, g, kGround);
    // Steps down from the door into the dark.
    s.ground(0, 14, g - 6, kGround).rampDown(15, g - 2, 4, kRampDown, kGround).ground(19, 30, g - 2, kGround)
        .rampDown(31, g, 2, kRampDown, kGround);
    s.row(40, 50, g - 4, kFoothold).column(48, g - 4, g - 1, kLadder);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(3.0f, g - 6), .to = MapId::SaltwhistleMarket, .toPortal = 2, .hidden = true},
        {.at = feetOn(77.0f, g), .to = MapId::GlimmerTunnels, .toPortal = 0},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Torch, {tx(8.0f), (g - 6) * kTile}, 0, 1.0f, false}, {K::Torch, {tx(26.0f), (g - 2) * kTile}, 0, 1.0f, false},
        {K::Tent, {tx(56.0f), y}, 2, 1.0f, false},                {K::Campfire, {tx(62.0f), y}, 0, 1.0f, false},
        {K::Crate, {tx(66.0f), y}, 0, 1.0f, false},               {K::Lantern, {tx(45.0f), (g - 4) * kTile}, 0, 0.9f, false},
        {K::Column, {tx(36.0f), y}, 0, 1.0f, false},              {K::Column, {tx(72.0f), y}, 1, 1.0f, false},
        {K::Crystal, {tx(52.0f), y}, 0, 0.7f, false},
    };
    def.npcs = {{NpcId::Nix, feetOn(60.0f, g), -1}};
}

void tunnels(MapDef& def)
{
    def.name = "Glimmer Tunnels";
    def.kind = MapKind::Dungeon;
    constexpr int g = 24;
    TileSketch s{120, 28};
    s.ground(0, 119, g, kGround);
    s.fill(0, 119, 0, 1, kGround); // the ceiling
    s.row(8, 18, g - 4, kFoothold).column(10, g - 4, g - 1, kRope).row(14, 24, g - 8, kFoothold).column(22, g - 8, g - 5, kLadder);
    s.ground(30, 44, g - 3, kGround).rampUp(27, g, 3, kRampUp, kGround).rampDown(45, g, 3, kRampDown, kGround);
    s.row(32, 42, g - 8, kFoothold).column(38, g - 8, g - 4, kRope).row(36, 46, g - 12, kFoothold).column(44, g - 12, g - 9, kRope);
    s.row(54, 62, g - 3, kFoothold).row(58, 68, g - 6, kFoothold).column(66, g - 6, g - 1, kLadder).row(62, 72, g - 10, kFoothold)
        .column(67, g - 10, g - 7, kRope);
    s.ground(80, 96, g - 4, kGround).column(79, g - 4, g - 1, kLadder).rampDown(97, g, 4, kRampDown, kGround);
    s.row(84, 94, g - 9, kFoothold).column(90, g - 9, g - 5, kRope);
    s.row(104, 114, g - 4, kFoothold).column(112, g - 4, g - 1, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::UndercroftStairs, .toPortal = 1},
        {.at = feetOn(117.0f, g), .to = MapId::SunkenArchive, .toPortal = 0},
    };
    def.spawns = {
        {S::CaveFlitter, runOn(1, 26, g), 2},     {S::Runeshell, runOn(8, 18, g - 4), 1},   {S::CaveFlitter, runOn(14, 24, g - 8), 1},
        {S::Runeshell, runOn(30, 44, g - 3), 2},  {S::CaveFlitter, runOn(32, 42, g - 8), 1}, {S::Runeshell, runOn(48, 78, g), 3},
        {S::CaveFlitter, runOn(58, 68, g - 6), 1}, {S::Runeshell, runOn(62, 72, g - 10), 1}, {S::Runeshell, runOn(80, 96, g - 4), 2},
        {S::CaveFlitter, runOn(84, 94, g - 9), 1}, {S::CaveFlitter, runOn(100, 116, g), 2},
    };
    def.populationCap = 16;
    const float y = g * kTile;
    def.decos = {
        {K::Crystal, {tx(5.0f), y}, 0, 1.0f, false},               {K::Stalagmite, {tx(20.0f), y}, 0, 1.0f, false},
        {K::Torch, {tx(26.0f), y}, 0, 1.0f, false},                {K::Crystal, {tx(40.0f), (g - 3) * kTile}, 1, 0.9f, false},
        {K::Stalagmite, {tx(50.0f), y}, 1, 1.2f, false},           {K::Crystal, {tx(74.0f), y}, 0, 1.2f, false},
        {K::Torch, {tx(86.0f), (g - 4) * kTile}, 0, 1.0f, false},  {K::Stalagmite, {tx(101.0f), y}, 0, 0.9f, false},
        {K::Crystal, {tx(110.0f), y}, 1, 1.0f, false},
    };
}

void archive(MapDef& def)
{
    def.name = "Sunken Archive";
    def.kind = MapKind::Dungeon;
    constexpr int g = 26;
    TileSketch s{116, 30};
    s.ground(0, 115, g, kGround);
    s.fill(0, 115, 0, 1, kGround);
    // Bookshelf galleries in three floors, ladders between them.
    s.row(4, 50, g - 6, kFoothold).column(6, g - 6, g - 1, kLadder).column(46, g - 6, g - 1, kLadder);
    s.row(12, 42, g - 12, kFoothold).column(14, g - 12, g - 7, kRope).column(40, g - 12, g - 7, kRope);
    s.row(20, 34, g - 18, kFoothold).column(27, g - 18, g - 13, kLadder);
    s.row(62, 108, g - 6, kFoothold).column(64, g - 6, g - 1, kLadder).column(106, g - 6, g - 1, kLadder);
    s.row(70, 100, g - 12, kFoothold).column(72, g - 12, g - 7, kRope).column(98, g - 12, g - 7, kRope);
    s.row(78, 92, g - 18, kFoothold).column(85, g - 18, g - 13, kLadder);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::GlimmerTunnels, .toPortal = 1},
        {.at = feetOn(113.0f, g), .to = MapId::DeepHollow, .toPortal = 0},
    };
    def.spawns = {
        {S::Runeshell, runOn(8, 44, g), 2},       {S::CaveFlitter, runOn(4, 50, g - 6), 2}, {S::Runeshell, runOn(12, 42, g - 12), 2},
        {S::CaveFlitter, runOn(20, 34, g - 18), 1}, {S::Runeshell, runOn(52, 60, g), 1},  {S::Runeshell, runOn(66, 104, g), 2},
        {S::CaveFlitter, runOn(62, 108, g - 6), 2}, {S::Runeshell, runOn(70, 100, g - 12), 2}, {S::CaveFlitter, runOn(78, 92, g - 18), 1},
    };
    def.populationCap = 15;
    def.eliteChance = 8;
    const float y = g * kTile;
    def.decos = {
        {K::Column, {tx(10.0f), y}, 1, 1.0f, false},              {K::Column, {tx(30.0f), y}, 1, 1.0f, false},
        {K::Torch, {tx(20.0f), (g - 6) * kTile}, 0, 1.0f, false}, {K::Crystal, {tx(27.0f), (g - 18) * kTile}, 1, 0.8f, false},
        {K::Column, {tx(56.0f), y}, 0, 1.2f, false},              {K::Column, {tx(84.0f), y}, 1, 1.0f, false},
        {K::Torch, {tx(90.0f), (g - 6) * kTile}, 0, 1.0f, false}, {K::Crystal, {tx(85.0f), (g - 18) * kTile}, 0, 0.8f, false},
        {K::Lantern, {tx(36.0f), (g - 12) * kTile}, 0, 0.9f, false}, {K::Lantern, {tx(76.0f), (g - 12) * kTile}, 0, 0.9f, false},
    };
}

void deepHollow(MapDef& def)
{
    def.name = "The Deep Hollow";
    def.kind = MapKind::Dungeon;
    constexpr int g = 26;
    TileSketch s{124, 30};
    s.ground(0, 123, g, kGround);
    s.fill(0, 123, 0, 1, kGround);
    s.ground(10, 28, g - 4, kGround).column(9, g - 4, g - 1, kLadder).rampDown(29, g, 4, kRampDown, kGround);
    s.row(12, 24, g - 9, kFoothold).column(18, g - 9, g - 5, kRope).row(16, 26, g - 13, kFoothold).column(24, g - 13, g - 10, kRope);
    s.row(40, 48, g - 3, kFoothold).row(44, 54, g - 7, kFoothold).column(52, g - 7, g - 1, kRope).row(50, 60, g - 11, kFoothold)
        .column(53, g - 11, g - 8, kLadder);
    s.rampUp(66, g, 4, kRampUp, kGround).ground(70, 86, g - 4, kGround).rampDown(87, g, 4, kRampDown, kGround);
    s.row(72, 84, g - 9, kFoothold).column(78, g - 9, g - 5, kRope);
    s.row(96, 104, g - 3, kFoothold).row(100, 110, g - 7, kFoothold).column(108, g - 7, g - 1, kLadder).row(106, 116, g - 11, kFoothold)
        .column(109, g - 11, g - 8, kRope);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::SunkenArchive, .toPortal = 1},
        {.at = feetOn(121.0f, g), .to = MapId::HeartOfGloom, .toPortal = 0},
    };
    def.spawns = {
        {S::Runeshell, runOn(10, 28, g - 4), 2},   {S::CaveFlitter, runOn(12, 24, g - 9), 1}, {S::Runeshell, runOn(16, 26, g - 13), 1},
        {S::CaveFlitter, runOn(32, 64, g), 3},     {S::Runeshell, runOn(44, 54, g - 7), 1},   {S::CaveFlitter, runOn(50, 60, g - 11), 1},
        {S::Runeshell, runOn(70, 86, g - 4), 2},   {S::CaveFlitter, runOn(72, 84, g - 9), 1}, {S::Runeshell, runOn(90, 118, g), 3},
        {S::CaveFlitter, runOn(100, 110, g - 7), 1}, {S::Runeshell, runOn(106, 116, g - 11), 1},
    };
    def.populationCap = 16;
    def.eliteChance = 10;
    const float y = g * kTile;
    def.decos = {
        {K::Stalagmite, {tx(5.0f), y}, 1, 1.2f, false},            {K::Crystal, {tx(20.0f), (g - 4) * kTile}, 2, 1.0f, false},
        {K::Torch, {tx(34.0f), y}, 0, 1.0f, false},                {K::Stalagmite, {tx(62.0f), y}, 0, 1.1f, false},
        {K::Crystal, {tx(78.0f), (g - 4) * kTile}, 2, 1.2f, false}, {K::Torch, {tx(92.0f), y}, 0, 1.0f, false},
        {K::Stalagmite, {tx(118.0f), y}, 1, 1.0f, false},
    };
}

void heart(MapDef& def)
{
    def.name = "Heart of the Gloom";
    def.kind = MapKind::Arena;
    constexpr int g = 19;
    TileSketch s{48, 24};
    s.ground(0, 47, g, kGround);
    s.fill(0, 47, 0, 0, kGround);
    s.row(5, 11, g - 4, kFoothold).row(36, 42, g - 4, kFoothold).row(19, 28, g - 8, kFoothold);
    def.text = s.text();
    def.portals = {
        {.at = feetOn(2.5f, g), .to = MapId::DeepHollow, .toPortal = 1, .gate = Gate::AfterBoss},
    };
    def.arena = Arena{.boss = BossId::Murkmaw, .bossFeet = feetOn(33.0f, g), .bounds = {tx(1.0f), 0.0f, tx(46.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::Crystal, {tx(4.0f), y}, 2, 1.3f, false},    {K::Crystal, {tx(44.0f), y}, 2, 1.3f, false},
        {K::Column, {tx(14.0f), y}, 1, 1.2f, false},    {K::Column, {tx(34.0f), y}, 1, 1.2f, false},
        {K::Torch, {tx(9.0f), y}, 0, 1.0f, false},      {K::Torch, {tx(39.0f), y}, 0, 1.0f, false},
    };
}

} // namespace

void buildDepths(MapDef& def)
{
    def.region = Region::Depths;
    switch (def.id) {
    case MapId::UndercroftStairs: stairs(def); break;
    case MapId::GlimmerTunnels: tunnels(def); break;
    case MapId::SunkenArchive: archive(def); break;
    case MapId::DeepHollow: deepHollow(def); break;
    case MapId::HeartOfGloom: heart(def); break;
    default: break;
    }
}

} // namespace realm::game
