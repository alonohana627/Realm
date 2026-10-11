#pragma once

// One character's playthrough from a save: the hero (GameLib's platformer
// controller tuned by Speed and Jump, the basic attack, the hyper jump,
// prone, ropes and ladders, eight job skills), the current map with its
// monsters, spawns, drops and shots, an area boss in its arena, the
// townsfolk, the clock, the quests and the story. Deterministic: the same
// save and per-tick input give the same run. No window, audio or clock.
//
// The app drives it: tick() with the input, events() for sounds, numbers
// and effects, and commands from its windows (spend a point, learn a skill,
// equip, use, buy, sell) and from conversations (conditions and effects).

#include "bosses.hpp"
#include "character.hpp"
#include "maps.hpp"
#include "monsters.hpp"
#include "rules.hpp"
#include "story.hpp"

#include <gamelib/core/combat.hpp>
#include <gamelib/core/drops.hpp>
#include <gamelib/core/event_queue.hpp>
#include <gamelib/core/game_clock.hpp>
#include <gamelib/core/items.hpp>
#include <gamelib/core/map_reveal.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/platformer.hpp>
#include <gamelib/core/projectiles.hpp>
#include <gamelib/core/quest_talk.hpp>
#include <gamelib/core/quests.hpp>
#include <gamelib/core/rng.hpp>
#include <gamelib/core/spawner.hpp>
#include <gamelib/core/telegraphs.hpp>
#include <gamelib/core/world_flags.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace realm::game {

inline constexpr std::int64_t kTicksPerDay = 16LL * 60 * kTickRate; // a day is sixteen minutes

enum class EventKind : std::uint8_t {
    Jump, HyperJump, Land, Climb, Swing, Hit, MonsterDown, Hurt, Heal, Pickup, Coins, LevelUp, SkillCast, SkillHit, Chain,
    Star, SkillLearned, NoMana, Cooldown, Potion, QuestUpdate, QuestAccepted, QuestComplete, QuestDone, Travel, PortalClosed,
    Death, BossWake, BossPhase, BossRoar, BossDown, BossLand, EnemyShot, HazardWarn, HazardStrike, Saved, Rest, JobAdvance,
    Spawned, Equip, Bought, Sold, Full, Buff, BuffEnd, ShieldHit, Prone, Sail, Ending, Point, Drop
};

struct Event {
    EventKind kind = EventKind::Jump;
    int value = 0;  // damage, amount, level, skill, quest, item, map...
    int extra = 0;  // rarity, combo step, progress, the target's key...
    gamelib::core::Vec2 at;
    gamelib::core::Vec2 to; // a chain's end, a shot's direction
    bool crit = false;
    bool onHero = false;
    int facing = 1;
};

enum class Phase : std::uint8_t { Playing, Dead };

// A shot: the hero's (a pebble, an arcane bolt) or a boss's.
struct ShotData {
    bool hero = false;
    ShotLook look = ShotLook::Pebble;
    int targetsLeft = 1;
    int percent = 100;     // hero shots: damage percent
    bool magic = false;
    bool groundWave = false;
    bool spent = false;    // landed: removed this tick
};
using Shots = gamelib::core::Projectiles<ShotData>;

// What lies on the ground.
struct Loot {
    gamelib::core::ItemStack stack; // empty for coins
    std::int64_t coins = 0;
};
using Drops = gamelib::core::Drops<Loot>;

struct Hero {
    gamelib::core::PlatformerBody body;
    gamelib::core::Rect previous;
    gamelib::core::Attacker attack{1, 1'000'000};
    gamelib::core::Attacker skill{1, 2'000'000};
    gamelib::core::Vulnerable vulnerable;
    int castTicks = 0;       // > 0 while a skill plays out (no walking)
    int castLength = 1;
    SkillId casting = SkillId::PebbleToss;
    int rushTicks = 0;       // > 0 while rushing forward
    int hurtTicks = 0;       // knocked back, no control
    int hyperTicks = 0;      // > 0 just after a hyper jump (the trail)
    bool hyperUp = false;
    bool prone = false;
    int landTicks = 0;
    int airTicks = 0;
    int idleTicks = 0;
    int pickupCooldown = 0;
    bool alive = true;
};

struct NpcHere {
    NpcId npc = NpcId::NanaRue;
    gamelib::core::Vec2 feet;
    int facing = 1;
};

struct RunStats {
    std::int64_t ticks = 0;
    int defeated = 0;
    int elites = 0;
    int bosses = 0;
    int deaths = 0;
    std::int64_t coinsEarned = 0;
    int potions = 0;
    int bestHit = 0;
    int questsDone = 0;
};

// Everything a save keeps.
struct SaveData {
    Character character;
    std::vector<gamelib::core::QuestRecord> quests;
    gamelib::core::WorldFlags flags;
    MapId map = MapId::Petalbrook;
    gamelib::core::Vec2 feet;
    std::int64_t clock = 0;
    RunStats stats;
    std::array<int, kBossCount> bossRespawn{}; // ticks until each boss returns
    std::array<bool, kMapCount> visited{};
    bool finished = false;
    std::string bindings; // the character's key layout (app::readActionBindings text), kept as is
};

class Session {
public:
    // A new character, or a loaded one.
    explicit Session(const Appearance& look, std::uint64_t seed = 1);
    Session(const SaveData& save, std::uint64_t seed);

    void tick(const Input& input);

    // ---- state
    [[nodiscard]] Phase phase() const { return m_phase; }
    [[nodiscard]] const Hero& hero() const { return m_hero; }
    [[nodiscard]] const Character& character() const { return m_char; }
    [[nodiscard]] const Derived& derived() const { return m_derived; }
    [[nodiscard]] MapId mapId() const { return m_map; }
    [[nodiscard]] const MapDef& map() const { return mapDef(m_map); }
    [[nodiscard]] const std::vector<Monster>& monsters() const { return m_monsters; }
    [[nodiscard]] const std::optional<Boss>& boss() const { return m_boss; }
    [[nodiscard]] const Drops& drops() const { return m_drops; }
    [[nodiscard]] const Shots& shots() const { return m_shots; }
    [[nodiscard]] const gamelib::core::Telegraphs<float>& hazards() const { return m_hazards; }
    [[nodiscard]] const std::vector<NpcHere>& npcs() const { return m_npcs; }
    [[nodiscard]] const gamelib::core::GameClock& clock() const { return m_clock; }
    [[nodiscard]] const gamelib::core::QuestBook& quests() const { return m_quests; }
    [[nodiscard]] const RunStats& stats() const { return m_stats; }
    [[nodiscard]] std::int64_t ticks() const { return m_ticks; }
    [[nodiscard]] gamelib::core::EventQueue<Event>& events() { return m_events; }
    [[nodiscard]] float nightAmount() const;
    [[nodiscard]] bool night() const;
    [[nodiscard]] bool finished() const { return m_finished; }
    [[nodiscard]] bool visited(MapId map) const { return m_visited[static_cast<std::size_t>(map)]; }
    [[nodiscard]] int chapter() const { return chapterOf(m_quests); }
    [[nodiscard]] bool portalOpen(int index) const;
    [[nodiscard]] int bossRespawn(BossId id) const { return m_bossRespawn[static_cast<std::size_t>(id)]; }
    [[nodiscard]] const std::string& bindings() const { return m_bindings; }
    void setBindings(std::string text) { m_bindings = std::move(text); }

    // What the hero could interact with now (for prompts).
    [[nodiscard]] std::optional<NpcId> npcInReach() const;
    [[nodiscard]] int portalInReach() const; // -1: none
    // Set by a tick when the hero pressed up at an NPC (the app opens the conversation).
    [[nodiscard]] std::optional<NpcId> takeTalkRequest();
    // A map change waits for the app's transition: travel() does it.
    [[nodiscard]] bool travelPending() const { return m_travel.has_value(); }
    [[nodiscard]] MapId travelTarget() const { return m_travel ? m_travel->map : m_map; }
    [[nodiscard]] bool travelIsVoyage() const { return m_travel && m_travel->voyage; }
    void travel();
    [[nodiscard]] bool wantsSave();
    // After fainting: back in the nearest town.
    void respawn();
    // The story's end was reached this tick (the last boss fell).
    [[nodiscard]] bool takeEnding();

    // ---- conversations
    void startTalk(NpcId npc);
    [[nodiscard]] bool condition(std::string_view condition) const;
    void effect(std::string_view effect);
    [[nodiscard]] std::optional<std::string> takeServiceRequest(); // "shop", "gearshop"
    [[nodiscard]] std::optional<NpcId> talkingTo() const { return m_talking; }

    // ---- windows
    bool spendPoint(Core stat);
    int autoAssign();
    gamelib::core::LearnResult learnSkill(SkillId skill);
    [[nodiscard]] bool canLearn(SkillId skill) const;
    bool equipFrom(int bagSlot);           // from the equip tab
    bool unequip(Slot slot);
    bool useAt(Tab tab, int bagSlot);      // equip, drink or read
    bool useItem(int item);                // a hotkey: the first stack of that item
    bool moveInBag(Tab tab, int from, int to);
    bool dropFromBag(Tab tab, int bagSlot);
    [[nodiscard]] std::vector<gamelib::core::ShopEntry> shopEntries() const;
    gamelib::core::TradeResult buy(int entry, int count);
    bool sell(Tab tab, int bagSlot);

    // ---- saves
    [[nodiscard]] SaveData save() const;

    // ---- developer aids and the demo
    void debugTeleport(MapId map, int portal);
    void debugLevel(int level);
    void debugJob(Job job);
    void debugQuestsTo(int quest);
    void debugBossLow();
    void debugTime(int hour);
    void debugGear();
    void debugMove(float x);
    void debugLearnAll();

private:
    struct Travel {
        MapId map = MapId::Petalbrook;
        int portal = 0;
        bool voyage = false;
        std::optional<gamelib::core::Vec2> feet;
    };
    void init();
    void enterMap(MapId map, gamelib::core::Vec2 feet, int facing);
    void spawnInto(int point, bool immediate);
    void tickHero(const Input& input);
    void tickAttacks();
    void tickMonsters();
    void tickBoss();
    void tickShots();
    void tickDrops(const Input& input);
    void tickSpawns();
    void tickHazards();
    void placeNpcs();
    void useSkill(SkillId skill);
    // Strikes up to `targets` monsters (and the boss) in `area`, nearest
    // first; returns how many were struck.
    int strike(const gamelib::core::Rect& area, int percent, bool magic, int targets, int attackId, int stunTicks = 0,
               float push = 1.0f, int hitIndex = 0);
    void hitMonster(Monster& m, int percent, bool magic, int stunTicks, float push, int hitIndex, int direction);
    void hitBoss(int percent, bool magic, int hitIndex);
    void damageHero(int damage, float fromX);
    void defeatMonster(Monster& m);
    void defeatBoss();
    void dropLoot(const Monster& m);
    void spawnLoot(std::vector<Loot> loot, gamelib::core::Vec2 at);
    void addItem(const gamelib::core::ItemStack& stack);
    void refreshCollectQuests();
    void questEvent(const gamelib::core::QuestEvent& e);
    void refreshQuests();
    void applyReward(const gamelib::core::QuestReward& reward);
    void questAction(gamelib::core::QuestAction action, int quest);
    void gainXp(std::int64_t xp, gamelib::core::Vec2 at);
    void refreshDerived();
    [[nodiscard]] int damageRoll(int percent, bool magic, int defense, bool& crit);
    [[nodiscard]] gamelib::core::TileWorld world() const;
    [[nodiscard]] int weaponStyle() const;
    void push(const Event& e) { m_events.push(e); }

    gamelib::core::Rng m_rng;
    Phase m_phase = Phase::Playing;
    Character m_char;
    Derived m_derived;
    Hero m_hero;
    MapId m_map = MapId::Petalbrook;
    std::vector<Monster> m_monsters;
    gamelib::core::Spawner m_spawner;
    std::optional<Boss> m_boss;
    Drops m_drops;
    Shots m_shots{128};
    gamelib::core::Telegraphs<float> m_hazards;
    std::vector<NpcHere> m_npcs;
    gamelib::core::GameClock m_clock{kTicksPerDay};
    gamelib::core::QuestBook m_quests;
    gamelib::core::WorldFlags m_flags;
    gamelib::core::EventQueue<Event> m_events;
    RunStats m_stats;
    std::array<int, kBossCount> m_bossRespawn{};
    std::array<bool, kMapCount> m_visited{};
    std::int64_t m_ticks = 0;
    std::optional<Travel> m_travel;
    std::optional<NpcId> m_talk;
    std::optional<NpcId> m_talking;
    std::optional<std::string> m_service;
    std::string m_bindings;
    bool m_saveWanted = false;
    bool m_finished = false;
    bool m_endingNow = false;
    int m_deadTicks = 0;
    int m_mapTicks = 0;
    int m_regenTicks = 0;
    int m_attackId = 3'000'000;
    // Starfall's second wave and Quake's shockwave land a little later.
    struct Delayed {
        int ticks = 0;
        gamelib::core::Rect area;
        int percent = 100;
        bool magic = false;
        int targets = 1;
        int stun = 0;
        int hitIndex = 1;
        SkillId skill = SkillId::Starfall;
    };
    std::vector<Delayed> m_delayed;
};

} // namespace realm::game
