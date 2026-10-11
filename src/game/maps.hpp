#pragma once

// The world: twenty-five maps in five places - Petal Isle (the starting
// island), the port town of Saltwhistle and its coast, the Fernwood with
// the tree-top village of Mossbough, Windcrest Pass, and the Undercroft
// below the port. Each map is its own space (MapleStory's maps all start at
// (0, 0)), drawn as level text with core::TileSketch: ground, floating
// one-way footholds in tiers, ropes and ladders, ramps, walls only at the
// edges. On it: portals (stand in one and press up; core::WorldGraph links
// them), monster spawns on runs of floor, decorations and houses (drawn by
// the app), townsfolk, and an area boss's arena.

#include "monsters.hpp"
#include "rules.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/core/tile_collision.hpp>
#include <gamelib/core/tile_map.hpp>
#include <gamelib/core/world_graph.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace realm::game {

enum class MapId : std::uint8_t {
    Petalbrook, SproutMeadow, PuddlePath, BreezyBluff, PetalPier,
    SaltwhistleDocks, SaltwhistleMarket, GullCliffs, TidepoolShore, LighthouseHill,
    FernwoodTrail, Mossbough, CanopyWalk, GlowcapHollow, ThornbarkGrove,
    WindcrestFoothills, ScoutCamp, CliffsideClimb, FrostbiteLedge, SummitShrine,
    UndercroftStairs, GlimmerTunnels, SunkenArchive, DeepHollow, HeartOfGloom,
    Count
};
inline constexpr int kMapCount = static_cast<int>(MapId::Count);

// Tiles (the level text's characters).
inline constexpr char kEmpty = ' ';
inline constexpr char kGround = '#';
inline constexpr char kFoothold = '=';
inline constexpr char kRope = '|';
inline constexpr char kLadder = 'H';
inline constexpr char kRampUp = '/';    // rises to the right
inline constexpr char kRampDown = '\\'; // rises to the left
inline constexpr char kBridge = '~';    // a rope bridge (one-way, drawn sagging)
inline constexpr char kPier = 'P';      // a wooden pier over the water (solid, drawn as planks)

[[nodiscard]] const gamelib::core::TileShapes& tileShapes();

enum class MapKind : std::uint8_t { Field, Town, Dungeon, Arena };

// When a portal opens.
enum class Gate : std::uint8_t {
    Open,
    Undercroft, // the door below Saltwhistle: opened by the story (chapter 4)
    AfterBoss,  // an arena's way out while its boss fights is shut
};

struct Portal {
    gamelib::core::Vec2 at; // feet point: where the hero arrives and where it is entered
    MapId to = MapId::Petalbrook;
    int toPortal = 0;       // index of the arrival portal in the target map
    Gate gate = Gate::Open;
    bool hidden = false;    // a house door: drawn as a door, not a swirl
};

struct SpawnDef {
    SpeciesId species = SpeciesId::Sproutcap;
    gamelib::core::Rect run; // floor run in world units: x range, bottom = the floor's top
    int count = 1;
};

enum class DecoKind : std::uint8_t {
    House, Tree, Pine, Bush, Flowers, Grass, Rock, Lamp, Fence, Sign, Barrel, Crate, FlowerPot, Well, Bench, Stall,
    Bunting, Fountain, Lighthouse, Boat, Post, Crystal, Torch, Lantern, Mushroom, Snowpile, Banner, Anvil, GiantTree,
    Tent, Campfire, Stalagmite, Column, Shrine, Shell, Reeds, Windmill, Count
};

// House styles (Deco::variant for DecoKind::House) - building.frag's kinds.
enum class HouseStyle : std::uint8_t {
    Cottage, Shop, Inn, Warehouse, Townhouse, TreeHut, Library, Armory, Dockhouse, Bakery, Count
};

struct Deco {
    DecoKind kind = DecoKind::Tree;
    gamelib::core::Vec2 at; // ground point (bottom middle)
    int variant = 0;        // style or kind
    float scale = 1.0f;
    bool front = false;     // drawn in front of the characters
    int palette = 0;        // a color set (houses: walls and roofs)
};

enum class NpcId : std::uint8_t {
    NanaRue, Pip, Hettie, Marlo, Quill, Finn, Rosa, Dottie, Garrick, Kestrel, Wren, Oakhart, Lila, Bramble, Tamsin, Nix,
    Count
};
inline constexpr int kNpcCount = static_cast<int>(NpcId::Count);

struct NpcPlace {
    NpcId npc = NpcId::NanaRue;
    gamelib::core::Vec2 feet;
    int facing = 1;
};

enum class BossId : std::uint8_t { Thornbark, Frostmane, Murkmaw, Count };
inline constexpr int kBossCount = static_cast<int>(BossId::Count);

struct Arena {
    BossId boss = BossId::Thornbark;
    gamelib::core::Vec2 bossFeet;
    gamelib::core::Rect bounds; // where the boss moves
};

struct MapDef {
    MapId id = MapId::Petalbrook;
    std::string_view name;
    Region region = Region::Island;
    MapKind kind = MapKind::Field;
    int columns = 0;
    int rows = 0;
    std::vector<std::string> text;
    std::vector<Portal> portals;
    std::vector<SpawnDef> spawns;
    std::vector<Deco> decos;
    std::vector<Deco> skyline; // a back row on a parallax layer
    std::vector<NpcPlace> npcs;
    std::optional<Arena> arena;
    int eliteChance = 4;      // percent per spawn
    int populationCap = 0;    // at most this many monsters at once (0: one per spawn slot)
    gamelib::core::Vec2 dock; // the ship's mooring (Petal Pier and the docks), x < 0: none
    float water = -1.0f;      // the sea's surface (world y) under the piers; < 0: no water
    gamelib::core::TileMap tiles{1, 1, kTile};

    [[nodiscard]] gamelib::core::Rect bounds() const
    {
        return {0.0f, 0.0f, static_cast<float>(columns) * kTile, static_cast<float>(rows) * kTile};
    }
    [[nodiscard]] bool town() const { return kind == MapKind::Town; }
};

[[nodiscard]] const MapDef& mapDef(MapId id);
// The world graph: one room per map (its own space), one portal exit per
// map portal (exit id = portalExit(map, index)), linked in pairs.
[[nodiscard]] const gamelib::core::WorldGraph& worldGraph();
[[nodiscard]] int portalExit(MapId map, int portal);
// The map and portal of an exit id.
[[nodiscard]] std::pair<MapId, int> exitPortal(int exit);
// Where a return scroll (and fainting) takes the hero from a map.
[[nodiscard]] MapId nearestTown(MapId map);

// World points for map authoring: a tile's left edge, a feet point on top of a row.
[[nodiscard]] constexpr float tx(const float column) { return column * kTile; }
[[nodiscard]] constexpr gamelib::core::Vec2 feetOn(const float column, const int row)
{
    return {column * kTile, static_cast<float>(row) * kTile};
}
// A floor run between columns (inclusive) whose top is row `row`.
[[nodiscard]] constexpr gamelib::core::Rect runOn(const int x0, const int x1, const int row)
{
    return {static_cast<float>(x0) * kTile, static_cast<float>(row) * kTile - 4.0f, static_cast<float>(x1 - x0 + 1) * kTile, 4.0f};
}

// Region map makers (maps_*.cpp).
void buildIsland(MapDef& def);
void buildCoast(MapDef& def);
void buildForest(MapDef& def);
void buildMountain(MapDef& def);
void buildDepths(MapDef& def);

} // namespace realm::game
