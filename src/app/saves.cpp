#include "saves.hpp"

#include "actions.hpp"
#include "profile.hpp"

#include <gamelib/app/bindings.hpp>
#include <gamelib/app/rpg_file.hpp>
#include <gamelib/app/world_file.hpp>
#include <gamelib/core/timers.hpp>
#include <gamelib/storage/files.hpp>

#include <algorithm>
#include <format>
#include <string>

namespace realm::app {

namespace {

// helpers

using gamelib::core::ItemStack;
using gamelib::storage::IniFile;

constexpr int kSaveVersion = 1;
constexpr std::array<std::string_view, game::kTabCount> kBagSections{"bag_equip", "bag_use", "bag_etc"};

std::string slotName(const int slot)
{
    return std::to_string(slot + 1);
}

IniFile write(const game::SaveData& d, const gamelib::platform::ActionMap& actions)
{
    IniFile ini;
    const game::Character& c = d.character;
    ini.set("look", "name", c.look.name);
    ini.setInt("look", "hair", c.look.hair);
    ini.setInt("look", "hair_color", c.look.hairColor);
    ini.setInt("look", "eyes", c.look.eyes);
    ini.setInt("look", "skin", c.look.skin);
    ini.setInt("hero", "job", static_cast<int>(c.job));
    ini.setInt("hero", "level", c.level);
    ini.setInt("hero", "xp", c.xp);
    for (int i = 0; i < game::kCoreCount; ++i) {
        const auto stat = static_cast<game::Core>(i);
        ini.setInt("hero", game::coreShort(stat), c.points.base()[stat]);
    }
    ini.setInt("hero", "ability_points", c.points.available());
    ini.setInt("hero", "hp", c.hp.current);
    ini.setInt("hero", "mp", c.mp.current);
    ini.setInt("hero", "coins", c.coins);
    gamelib::app::writeSkillBook(c.skills, ini, "skills");
    for (int t = 0; t < game::kTabCount; ++t) {
        gamelib::app::writeInventory(c.bags[static_cast<std::size_t>(t)], ini, kBagSections[static_cast<std::size_t>(t)]);
    }
    gamelib::app::writeEquipment(c.gear, ini, "gear");
    gamelib::app::writeQuestRecords(d.quests, ini, "quests");
    gamelib::app::writeWorldFlags(d.flags, ini, "flags");
    ini.setInt("world", "map", static_cast<int>(d.map));
    ini.setInt("world", "x", static_cast<long long>(d.feet.x));
    ini.setInt("world", "y", static_cast<long long>(d.feet.y));
    ini.setInt("world", "clock", d.clock);
    ini.setBool("world", "finished", d.finished);
    std::string visited;
    for (const bool v : d.visited) {
        visited += v ? '1' : '0';
    }
    ini.set("world", "visited", visited);
    for (int b = 0; b < game::kBossCount; ++b) {
        ini.setInt("bosses", std::to_string(b), d.bossRespawn[static_cast<std::size_t>(b)]);
    }
    ini.setInt("stats", "ticks", d.stats.ticks);
    ini.setInt("stats", "defeated", d.stats.defeated);
    ini.setInt("stats", "elites", d.stats.elites);
    ini.setInt("stats", "bosses", d.stats.bosses);
    ini.setInt("stats", "deaths", d.stats.deaths);
    ini.setInt("stats", "coins", d.stats.coinsEarned);
    ini.setInt("stats", "potions", d.stats.potions);
    ini.setInt("stats", "best_hit", d.stats.bestHit);
    ini.setInt("stats", "quests", d.stats.questsDone);
    gamelib::app::writeActionBindings(actions, ini, "keys");
    return ini;
}

game::SaveData read(const IniFile& ini)
{
    game::SaveData d;
    game::Character& c = d.character;
    c.look.name = std::string{ini.get("look", "name").value_or("Hero")};
    c.look.hair = ini.getInt("look", "hair", 0, 7).value_or(0);
    c.look.hairColor = ini.getInt("look", "hair_color", 0, 7).value_or(0);
    c.look.eyes = ini.getInt("look", "eyes", 0, 7).value_or(0);
    c.look.skin = ini.getInt("look", "skin", 0, 3).value_or(0);
    c.job = static_cast<game::Job>(ini.getInt("hero", "job", 0, 2).value_or(0));
    c.level = ini.getInt("hero", "level", 1, game::kMaxLevel).value_or(1);
    c.xp = std::max<long long>(0, ini.getInt("hero", "xp").value_or(0));
    game::CoreStats base;
    for (int i = 0; i < game::kCoreCount; ++i) {
        const auto stat = static_cast<game::Core>(i);
        base[stat] = ini.getInt("hero", game::coreShort(stat), 1, 999).value_or(game::kStartCore);
    }
    c.points.restore(base, ini.getInt("hero", "ability_points", 0, 999).value_or(0));
    gamelib::app::readSkillBook(ini, "skills", c.skills);
    c.coins = std::max<long long>(0, ini.getInt("hero", "coins").value_or(0));
    for (int t = 0; t < game::kTabCount; ++t) {
        (void)gamelib::app::readInventory(ini, kBagSections[static_cast<std::size_t>(t)], c.bags[static_cast<std::size_t>(t)]);
    }
    for (const ItemStack& misfit : gamelib::app::readEquipment(ini, "gear", c.gear)) {
        (void)c.add(misfit); // a piece that no longer fits its slot goes to the bag
    }
    c.refreshResources();
    c.hp.current = std::clamp(ini.getInt("hero", "hp", 1, 999'999).value_or(c.hp.max), 1, c.hp.max);
    c.mp.current = std::clamp(ini.getInt("hero", "mp", 0, 999'999).value_or(c.mp.max), 0, c.mp.max);
    d.quests = gamelib::app::readQuestRecords(ini, "quests");
    d.flags = gamelib::app::readWorldFlags(ini, "flags");
    d.map = static_cast<game::MapId>(ini.getInt("world", "map", 0, game::kMapCount - 1).value_or(0));
    d.feet = {static_cast<float>(ini.getInt("world", "x").value_or(600)), static_cast<float>(ini.getInt("world", "y").value_or(600))};
    d.clock = std::max<long long>(0, ini.getInt("world", "clock").value_or(0));
    d.finished = ini.getBool("world", "finished").value_or(false);
    const std::string visited{ini.get("world", "visited").value_or("")};
    for (std::size_t i = 0; i < d.visited.size() && i < visited.size(); ++i) {
        d.visited[i] = visited[i] == '1';
    }
    for (int b = 0; b < game::kBossCount; ++b) {
        d.bossRespawn[static_cast<std::size_t>(b)] = ini.getInt("bosses", std::to_string(b), 0, 1'000'000).value_or(0);
    }
    d.stats.ticks = std::max<long long>(0, ini.getInt("stats", "ticks").value_or(0));
    d.stats.defeated = ini.getInt("stats", "defeated", 0, 99'999'999).value_or(0);
    d.stats.elites = ini.getInt("stats", "elites", 0, 99'999'999).value_or(0);
    d.stats.bosses = ini.getInt("stats", "bosses", 0, 99'999).value_or(0);
    d.stats.deaths = ini.getInt("stats", "deaths", 0, 99'999'999).value_or(0);
    d.stats.coinsEarned = std::max<long long>(0, ini.getInt("stats", "coins").value_or(0));
    d.stats.potions = ini.getInt("stats", "potions", 0, 99'999'999).value_or(0);
    d.stats.bestHit = ini.getInt("stats", "best_hit", 0, 99'999'999).value_or(0);
    d.stats.questsDone = ini.getInt("stats", "quests", 0, 9'999).value_or(0);
    return d;
}

std::string summary(const game::SaveData& d)
{
    return std::format("{}  lv {}  {}  {}", d.character.look.name, d.character.level, game::jobName(d.character.job), game::mapDef(d.map).name);
}

} // namespace

gamelib::platform::ActionMap actionsFor(const game::Job job, const gamelib::storage::IniFile* saved)
{
    gamelib::platform::ActionMap actions = makeActions(job);
    if (saved != nullptr) {
        gamelib::app::readActionBindings(*saved, actions, "keys");
    }
    return actions;
}

Saves::Saves()
    : m_slots{gamelib::storage::userDataDir(kAppName, kDataDirVariable) / "saves", kSaveVersion, "realm"}
{
}

std::optional<LoadedSlot> Saves::load(const int slot)
{
    auto loaded = m_slots.load(slotName(slot));
    if (!loaded) {
        return std::nullopt;
    }
    return LoadedSlot{.data = read(loaded->data), .ini = loaded->data};
}

bool Saves::exists(const int slot) const
{
    return m_slots.info(slotName(slot)).exists;
}

bool Saves::readable(const int slot) const
{
    return m_slots.info(slotName(slot)).readable;
}

void Saves::save(const int slot, const game::SaveData& data, const gamelib::platform::ActionMap& actions)
{
    (void)m_slots.save(slotName(slot), write(data, actions), summary(data));
}

void Saves::erase(const int slot)
{
    (void)m_slots.remove(slotName(slot));
}

} // namespace realm::app
