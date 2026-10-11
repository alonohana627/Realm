#include "maps.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace realm::game {

namespace {

// helpers

std::array<MapDef, kMapCount> buildAll()
{
    std::array<MapDef, kMapCount> maps;
    for (int i = 0; i < kMapCount; ++i) {
        MapDef& m = maps[static_cast<std::size_t>(i)];
        m.id = static_cast<MapId>(i);
        m.dock = {-1.0f, 0.0f};
        if (i <= static_cast<int>(MapId::PetalPier)) {
            buildIsland(m);
        } else if (i <= static_cast<int>(MapId::LighthouseHill)) {
            buildCoast(m);
        } else if (i <= static_cast<int>(MapId::ThornbarkGrove)) {
            buildForest(m);
        } else if (i <= static_cast<int>(MapId::SummitShrine)) {
            buildMountain(m);
        } else {
            buildDepths(m);
        }
    }
    for (MapDef& m : maps) {
        if (m.text.empty()) {
            throw std::logic_error("map without tiles: " + std::to_string(static_cast<int>(m.id)));
        }
        m.rows = static_cast<int>(m.text.size());
        m.columns = static_cast<int>(m.text.front().size());
        std::vector<std::string_view> views(m.text.begin(), m.text.end());
        const std::array<gamelib::core::LayerMapping, 1> layers{gamelib::core::charTile};
        m.tiles = gamelib::core::TileMap::fromText(views, kTile, layers);
    }
    // Every portal must lead somewhere real and back.
    for (const MapDef& m : maps) {
        for (const Portal& p : m.portals) {
            const MapDef& target = maps[static_cast<std::size_t>(p.to)];
            if (p.toPortal < 0 || p.toPortal >= static_cast<int>(target.portals.size())) {
                throw std::logic_error(std::string{m.name} + ": portal to " + std::string{target.name} + " has no arrival portal");
            }
        }
    }
    return maps;
}

const std::array<MapDef, kMapCount>& maps()
{
    static const std::array<MapDef, kMapCount> all = buildAll();
    return all;
}

struct Graph {
    gamelib::core::WorldGraph graph;
    std::vector<int> firstExit; // per map: the exit id of its portal 0
    std::vector<std::pair<MapId, int>> portals; // per exit
};

Graph buildGraph()
{
    Graph g;
    for (const MapDef& m : maps()) {
        (void)g.graph.addRoom(m.bounds(), gamelib::core::RoomSpace::Own);
    }
    for (const MapDef& m : maps()) {
        g.firstExit.push_back(static_cast<int>(g.portals.size()));
        for (int i = 0; i < static_cast<int>(m.portals.size()); ++i) {
            const Portal& p = m.portals[static_cast<std::size_t>(i)];
            // The portal's area: a column a tile and a half wide standing on its feet point.
            const gamelib::core::Rect area{p.at.x - kTile * 0.75f, p.at.y - kTile * 2.0f, kTile * 1.5f, kTile * 2.0f};
            (void)g.graph.addExit(static_cast<int>(m.id), area, gamelib::core::ExitKind::Portal);
            g.portals.emplace_back(m.id, i);
        }
    }
    for (const MapDef& m : maps()) {
        for (int i = 0; i < static_cast<int>(m.portals.size()); ++i) {
            const Portal& p = m.portals[static_cast<std::size_t>(i)];
            const int from = g.firstExit[static_cast<std::size_t>(m.id)] + i;
            const int to = g.firstExit[static_cast<std::size_t>(p.to)] + p.toPortal;
            g.graph.linkOneWay(from, to);
        }
    }
    return g;
}

const Graph& graphData()
{
    static const Graph g = buildGraph();
    return g;
}

} // namespace

const gamelib::core::TileShapes& tileShapes()
{
    using gamelib::core::TileShape;
    // Ropes are climbed like ladders: their top cell is a floor, so climbing
    // up a rope ends standing on the foothold it hangs from.
    static const gamelib::core::TileShapes shapes{{kGround, TileShape::Solid},       {kFoothold, TileShape::OneWay},       {kBridge, TileShape::OneWay},
                                                  {kRope, TileShape::Ladder},        {kLadder, TileShape::Ladder},
                                                  {kRampUp, TileShape::RampUpRight}, {kRampDown, TileShape::RampUpLeft}};
    return shapes;
}

const MapDef& mapDef(const MapId id)
{
    return maps()[static_cast<std::size_t>(id)];
}

const gamelib::core::WorldGraph& worldGraph()
{
    return graphData().graph;
}

int portalExit(const MapId map, const int portal)
{
    return graphData().firstExit[static_cast<std::size_t>(map)] + portal;
}

std::pair<MapId, int> exitPortal(const int exit)
{
    return graphData().portals.at(static_cast<std::size_t>(exit));
}

MapId nearestTown(const MapId map)
{
    switch (mapDef(map).region) {
    case Region::Island: return MapId::Petalbrook;
    case Region::Coast: return MapId::SaltwhistleMarket;
    case Region::Forest: return MapId::Mossbough;
    case Region::Mountain: return MapId::ScoutCamp;
    case Region::Depths: return MapId::SaltwhistleMarket;
    case Region::Count: break;
    }
    return MapId::Petalbrook;
}

} // namespace realm::game
