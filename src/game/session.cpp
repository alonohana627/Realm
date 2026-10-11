#include "session.hpp"

#include <gamelib/core/collision.hpp>
#include <gamelib/core/quest_talk.hpp>
#include <gamelib/core/tile_collision.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace realm::game {

namespace {

// helpers

using gamelib::core::AttackClip;
using gamelib::core::ItemStack;
using gamelib::core::ObjectiveKind;
using gamelib::core::QuestState;
using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr int kRespawnDelay = 7 * kTickRate;  // about 7 s, like MapleStory's fields
constexpr int kHurtStun = 16;
constexpr int kHurtInvincible = 90;
constexpr Vec2 kTalkReach{90.0f, 110.0f};
constexpr int kTeamHero = 1;
constexpr int kTeamMonsters = 2;

// The basic attack per weapon: a quick overhead swing, faster with a wand,
// heavier with a two-handed weapon. Damage is a percent of attack.
AttackClip basicClip(const int weaponStyle)
{
    const bool heavy = weaponStyle == 3 || weaponStyle == 6; // axe, broadsword
    const bool light = weaponStyle == 5 || weaponStyle == 0; // wand, bare hands
    const int startup = heavy ? 9 : light ? 5 : 7;
    const float reach = heavy ? 1.15f : light ? 0.8f : 1.0f;
    return {.startup = startup, .active = 5, .recovery = heavy ? 18 : light ? 12 : 14,
            .hitbox = {-10.0f, -70.0f, 110.0f * reach, 110.0f}, .damage = 100, .knockback = {3.5f, -2.5f}, .stunTicks = 14,
            .hitStopTicks = 3, .cancelFrom = -1, .id = 1};
}

bool overlapsRect(const Rect& a, const Rect& b)
{
    return gamelib::core::overlaps(a, b);
}

} // namespace

// ---------------------------------------------------------------------------
// Construction and saves

Session::Session(const Appearance& look, const std::uint64_t seed)
    : m_rng{seed}
{
    init();
    m_char.look = look;
    m_char.gear.setAt(static_cast<int>(Slot::Weapon), {.item = kWillowBranch, .count = 1});
    m_char.gear.setAt(static_cast<int>(Slot::Top), {.item = kPlainTee, .count = 1});
    m_char.gear.setAt(static_cast<int>(Slot::Bottom), {.item = kCottonShorts, .count = 1});
    m_char.gear.setAt(static_cast<int>(Slot::Shoes), {.item = kRedSneakers, .count = 1});
    m_char.add({.item = kRedPotion, .count = 15});
    m_char.add({.item = kBluePotion, .count = 5});
    m_char.coins = 80;
    m_char.refreshResources();
    m_char.hp.refill();
    m_char.mp.refill();
    m_clock.setTime(8, 30);
    refreshQuests();
    refreshDerived();
    m_hero.body.facing = 1;
    enterMap(MapId::Petalbrook, feetOn(8.0f, 21), 1);
    m_saveWanted = false;
}

Session::Session(const SaveData& save, const std::uint64_t seed)
    : m_rng{seed}
{
    init();
    m_char = save.character;
    m_quests.load(save.quests);
    m_flags = save.flags;
    m_clock.setElapsed(save.clock);
    m_stats = save.stats;
    m_bossRespawn = save.bossRespawn;
    m_visited = save.visited;
    m_finished = save.finished;
    m_bindings = save.bindings;
    refreshQuests();
    m_char.refreshResources();
    refreshDerived();
    MapId map = save.map;
    Vec2 feet = save.feet;
    // Never wake inside an arena: start at its way out's far side.
    if (mapDef(map).kind == MapKind::Arena) {
        const Portal& out = mapDef(map).portals.front();
        map = out.to;
        feet = mapDef(map).portals[static_cast<std::size_t>(out.toPortal)].at;
    }
    enterMap(map, feet, 1);
    m_saveWanted = false;
}

void Session::init()
{
    for (const auto& q : questDefs()) {
        m_quests.define(q);
    }
    m_hero.vulnerable = gamelib::core::Vulnerable{m_hero.body.box, kTeamHero};
    gamelib::core::DropTuning tuning;
    tuning.lifetimeTicks = 120 * kTickRate;
    tuning.size = {34.0f, 34.0f};
    m_drops = Drops{tuning};
}

SaveData Session::save() const
{
    SaveData s;
    s.character = m_char;
    s.quests = m_quests.save();
    s.flags = m_flags;
    s.map = m_map;
    s.feet = m_hero.body.box.bottomCenter();
    s.clock = m_clock.elapsed();
    s.stats = m_stats;
    s.bossRespawn = m_bossRespawn;
    s.visited = m_visited;
    s.finished = m_finished;
    s.bindings = m_bindings;
    return s;
}

gamelib::core::TileWorld Session::world() const
{
    gamelib::core::TileWorld w{map().tiles, 0, tileShapes()};
    w.setSideWalls(true);
    return w;
}

void Session::refreshDerived()
{
    m_derived = m_char.derived();
}

int Session::weaponStyle() const
{
    const ItemStack& w = m_char.gear.at(static_cast<int>(Slot::Weapon));
    return w.empty() ? 0 : itemInfo(w.item).look.style;
}

// ---------------------------------------------------------------------------
// State

bool Session::night() const
{
    const int h = m_clock.hour();
    return h >= 19 || h < 6;
}

float Session::nightAmount() const
{
    return m_clock.nightAmount();
}

bool Session::portalOpen(const int index) const
{
    const MapDef& m = map();
    if (index < 0 || index >= static_cast<int>(m.portals.size())) {
        return false;
    }
    switch (m.portals[static_cast<std::size_t>(index)].gate) {
    case Gate::Open: return true;
    case Gate::Undercroft: return m_quests.stateOf(kQuestBeneath) == QuestState::TurnedIn;
    case Gate::AfterBoss: return !(m_boss && m_boss->fighting());
    }
    return true;
}

// ---------------------------------------------------------------------------
// Maps

void Session::enterMap(const MapId id, const Vec2 feet, const int facing)
{
    m_map = id;
    m_mapTicks = 0;
    m_visited[static_cast<std::size_t>(id)] = true;
    m_monsters.clear();
    m_drops.clear();
    m_shots.clear();
    m_hazards.clear();
    m_delayed.clear();
    m_boss.reset();
    const MapDef& def = map();
    m_spawner = gamelib::core::Spawner{def.populationCap};
    for (int s = 0; s < static_cast<int>(def.spawns.size()); ++s) {
        const SpawnDef& sd = def.spawns[static_cast<std::size_t>(s)];
        for (int i = 0; i < sd.count; ++i) {
            (void)m_spawner.add({.kind = s, .position = {sd.run.left(), sd.run.bottom()},
                                 .respawnTicks = kRespawnDelay + m_rng.range(0, 2 * kTickRate), .width = sd.run.width});
        }
    }
    const std::span<const int> opening = m_spawner.reset();
    for (const int point : std::vector<int>(opening.begin(), opening.end())) {
        spawnInto(point, true);
    }
    if (def.arena && m_bossRespawn[static_cast<std::size_t>(def.arena->boss)] <= 0) {
        m_boss = makeBoss(def.arena->boss, def.arena->bossFeet, def.arena->bounds);
        push({.kind = EventKind::BossWake, .value = static_cast<int>(def.arena->boss)});
    }
    Hero& h = m_hero;
    h.body.box = Rect::fromBottomCenter(feet, kHeroSize);
    h.body.velocity = {};
    h.body.facing = facing;
    h.body.climbing = false;
    h.body.grounded = false;
    h.previous = h.body.box;
    h.attack.cancel();
    h.skill.cancel();
    h.castTicks = 0;
    h.rushTicks = 0;
    h.hurtTicks = 0;
    h.prone = false;
    placeNpcs();
    push({.kind = EventKind::Travel, .value = static_cast<int>(id)});
    m_saveWanted = true; // autosave on every map change
}

void Session::spawnInto(const int point, const bool immediate)
{
    const MapDef& def = map();
    const SpawnDef& sd = def.spawns[static_cast<std::size_t>(m_spawner.point(point).kind)];
    const Species& s = species(sd.species);
    const Vec2 placed = m_spawner.place(point, m_rng, s.size.x, m_hero.body.box.expanded(80.0f));
    const bool elite = m_rng.range(0, 99) < def.eliteChance;
    const int level = s.level + m_rng.range(0, 1);
    Vec2 feet{placed.x, sd.run.bottom()};
    if (s.behavior == Behavior::Flyer) {
        feet.y -= 80.0f + m_rng.unit() * 80.0f;
    }
    Monster m = makeMonster(sd.species, level, elite, feet, sd.run.left(), sd.run.right(), static_cast<std::uint8_t>(m_rng.range(0, 255)));
    m.spawn = point;
    if (immediate) {
        m.stateTicks = m.stateLength - 1;
    }
    m_monsters.push_back(m);
    if (!immediate) {
        push({.kind = EventKind::Spawned, .value = static_cast<int>(sd.species), .at = m.box.bottomCenter()});
    }
}

void Session::placeNpcs()
{
    m_npcs.clear();
    for (const NpcPlace& n : map().npcs) {
        m_npcs.push_back({.npc = n.npc, .feet = n.feet, .facing = n.facing});
    }
}

void Session::travel()
{
    if (!m_travel) {
        return;
    }
    const Travel t = *m_travel;
    m_travel.reset();
    const MapDef& target = mapDef(t.map);
    Vec2 feet = t.feet.value_or(target.portals.empty() ? feetOn(4.0f, target.rows - 3)
                                                       : target.portals[static_cast<std::size_t>(std::clamp(t.portal, 0, static_cast<int>(target.portals.size()) - 1))].at);
    const int facing = feet.x < target.bounds().width * 0.5f ? 1 : -1;
    enterMap(t.map, feet, facing);
    if (t.voyage) {
        push({.kind = EventKind::Sail, .value = static_cast<int>(t.map)});
    }
}

bool Session::wantsSave()
{
    return std::exchange(m_saveWanted, false);
}

bool Session::takeEnding()
{
    return std::exchange(m_endingNow, false);
}

void Session::respawn()
{
    m_phase = Phase::Playing;
    m_hero.alive = true;
    m_char.refreshResources();
    m_char.hp.current = std::max(1, m_char.hp.max / 2);
    m_char.mp.current = std::max(m_char.mp.current, m_char.mp.max / 3);
    // Fainting costs a little experience, never a level.
    const std::int64_t floor = experienceCurve().xpFor(m_char.level);
    m_char.xp = std::max(floor, m_char.xp - experienceCurve().toNextLevel(m_char.xp) / 20);
    m_hero.vulnerable.makeInvincible(120);
    const MapId town = nearestTown(m_map);
    enterMap(town, mapDef(town).portals.front().at + Vec2{kTile * 2.0f, 0.0f}, 1);
}

// ---------------------------------------------------------------------------
// Interaction

std::optional<NpcId> Session::npcInReach() const
{
    const Vec2 feet = m_hero.body.box.bottomCenter();
    std::optional<NpcId> best;
    float bestDist = 1e9f;
    for (const NpcHere& n : m_npcs) {
        const float dx = std::abs(n.feet.x - feet.x);
        const float dy = std::abs(n.feet.y - feet.y);
        if (dx < kTalkReach.x && dy < kTalkReach.y && dx < bestDist) {
            best = n.npc;
            bestDist = dx;
        }
    }
    return best;
}

int Session::portalInReach() const
{
    const int exit = worldGraph().exitIn(static_cast<int>(m_map), m_hero.body.box);
    return exit < 0 ? -1 : exitPortal(exit).second;
}

std::optional<NpcId> Session::takeTalkRequest()
{
    return std::exchange(m_talk, std::nullopt);
}

std::optional<std::string> Session::takeServiceRequest()
{
    return std::exchange(m_service, std::nullopt);
}

void Session::questEvent(const gamelib::core::QuestEvent& e)
{
    for (const auto& u : m_quests.onEvent(e)) {
        push({.kind = u.questComplete ? EventKind::QuestComplete : EventKind::QuestUpdate, .value = u.quest, .extra = u.progress});
    }
}

void Session::startTalk(const NpcId npc)
{
    m_talking = npc;
    questEvent({.kind = ObjectiveKind::Talk, .subject = static_cast<int>(npc)});
    // Deliveries.
    const auto deliver = [&](const int quest, const int item, const NpcId to) {
        if (npc == to && m_quests.stateOf(quest) == QuestState::Active && m_char.countOf(item) > 0) {
            m_char.bag(Tab::Etc).remove(item, 1);
            questEvent({.kind = ObjectiveKind::Deliver, .subject = static_cast<int>(to)});
        }
    };
    deliver(kQuestAcrossWater, kNanaLetter, NpcId::Quill);
    deliver(kQuestDelivery, kDottieParcel, NpcId::Lila);
}

bool Session::condition(const std::string_view c) const
{
    if (c.empty()) {
        return true;
    }
    if (const std::optional<bool> q = gamelib::core::questCondition(m_quests, c)) {
        return *q;
    }
    const gamelib::core::DialogueCommand cmd = gamelib::core::parseCommand(c);
    if (cmd.verb == "job") {
        return jobName(m_char.job) == cmd.argument || (cmd.argument == "beginner" && m_char.job == Job::Beginner)
               || (cmd.argument == "warrior" && m_char.job == Job::Warrior) || (cmd.argument == "mage" && m_char.job == Job::Mage);
    }
    if (cmd.verb == "night") {
        return night();
    }
    if (cmd.verb == "day") {
        return !night();
    }
    if (cmd.verb == "has" && cmd.number) {
        return m_char.countOf(*cmd.number) > 0;
    }
    if (cmd.verb == "level" && cmd.number) {
        return m_char.level >= *cmd.number;
    }
    return false;
}

void Session::effect(const std::string_view e)
{
    if (const std::optional<gamelib::core::QuestEffect> q = gamelib::core::questEffect(e)) {
        questAction(q->action, q->quest);
        return;
    }
    const gamelib::core::DialogueCommand cmd = gamelib::core::parseCommand(e);
    if (cmd.verb == "shop" || cmd.verb == "gearshop") {
        m_service = std::string{cmd.verb};
    } else if (cmd.verb == "sail") {
        if (m_char.coins < 10) {
            push({.kind = EventKind::Full, .value = 1});
            return;
        }
        m_char.coins -= 10;
        const bool fromIsland = m_map == MapId::PetalPier;
        const MapId to = fromIsland ? MapId::SaltwhistleDocks : MapId::PetalPier;
        const MapDef& target = mapDef(to);
        m_travel = Travel{.map = to, .portal = 0, .voyage = true, .feet = target.dock + Vec2{fromIsland ? kTile * 3.0f : -kTile * 6.0f, 0.0f}};
    } else if (cmd.verb == "rest") {
        if (m_char.coins < 20) {
            push({.kind = EventKind::Full, .value = 1});
            return;
        }
        m_char.coins -= 20;
        m_char.hp.refill();
        m_char.mp.refill();
        const bool sleepTillMorning = night();
        if (sleepTillMorning) {
            m_clock.advanceTo(m_clock.timeOf(7, 0));
        }
        m_saveWanted = true;
        push({.kind = EventKind::Rest, .value = sleepTillMorning ? 1 : 0});
    }
}

void Session::questAction(const gamelib::core::QuestAction action, const int quest)
{
    using gamelib::core::QuestAction;
    if (action == QuestAction::Accept) {
        if (m_quests.accept(quest)) {
            push({.kind = EventKind::QuestAccepted, .value = quest});
            if (quest == kQuestAcrossWater) {
                addItem({.item = kNanaLetter, .count = 1});
            } else if (quest == kQuestDelivery) {
                addItem({.item = kDottieParcel, .count = 1});
            }
            refreshCollectQuests();
        }
    } else if (action == QuestAction::TurnIn) {
        if (m_quests.stateOf(quest) != QuestState::Complete) {
            return;
        }
        for (const auto& item : m_quests.handInItems(quest)) {
            m_char.bag(tabOf(item.item)).remove(item.item, item.count);
        }
        if (const std::optional<gamelib::core::QuestReward> reward = m_quests.turnIn(quest)) {
            ++m_stats.questsDone;
            push({.kind = EventKind::QuestDone, .value = quest});
            if (quest == kQuestHopToIt && m_char.skillLevel(SkillId::HyperJump) == 0) {
                m_char.skills.setLevel(static_cast<int>(SkillId::HyperJump), 1);
                push({.kind = EventKind::SkillLearned, .value = static_cast<int>(SkillId::HyperJump)});
            } else if (quest == kQuestWarrior || quest == kQuestMage) {
                m_char.advance(quest == kQuestWarrior ? Job::Warrior : Job::Mage);
                refreshDerived();
                push({.kind = EventKind::JobAdvance, .value = static_cast<int>(m_char.job), .at = m_hero.body.box.center(), .onHero = true});
            } else if (quest == kQuestBeneath) {
                // The shards go into the beacon's lens.
                m_char.bag(Tab::Etc).remove(kDawnShard, 1);
                m_char.bag(Tab::Etc).remove(kFrostShard, 1);
            } else if (quest == kQuestHeart) {
                m_char.bag(Tab::Etc).remove(kHeartShard, 1);
            }
            applyReward(*reward);
            refreshQuests();
            refreshCollectQuests();
        }
    } else {
        (void)m_quests.abandon(quest);
        refreshQuests();
    }
}

void Session::refreshQuests()
{
    const Job job = m_char.job;
    m_quests.refresh(m_char.level, [job](const int flag) {
        return (flag == kFlagBeginner && job == Job::Beginner) || (flag == kFlagHasJob && job != Job::Beginner);
    });
}

void Session::applyReward(const gamelib::core::QuestReward& reward)
{
    m_char.coins += reward.coins;
    m_stats.coinsEarned += reward.coins;
    if (reward.coins > 0) {
        push({.kind = EventKind::Coins, .value = static_cast<int>(reward.coins), .at = m_hero.body.box.center(), .onHero = true});
    }
    for (const auto& item : reward.items) {
        ItemStack stack{.item = item.item, .count = item.count};
        if (isEquipment(item.item)) {
            stack = rollEquipment(item.item, m_rng, false);
            stack.rarity = std::max(stack.rarity, 1); // quest gear is at least rare
            if (stack.bonuses.empty()) {
                stack.bonuses.push_back({.stat = static_cast<int>(Bonus::Defense), .value = 2});
            }
        }
        addItem(stack);
    }
    gainXp(reward.experience, m_hero.body.box.center());
}

void Session::addItem(const ItemStack& stack)
{
    const int left = m_char.add(stack);
    if (left < stack.count) {
        push({.kind = EventKind::Pickup, .value = stack.item, .extra = stack.rarity, .at = m_hero.body.box.center(), .onHero = true});
    }
    if (left > 0) {
        ItemStack rest = stack;
        rest.count = left;
        spawnLoot({{.stack = rest}}, m_hero.body.box.center());
        push({.kind = EventKind::Full});
    }
    refreshCollectQuests();
}

void Session::refreshCollectQuests()
{
    for (const auto& u : m_quests.countCollected([this](const int item) { return m_char.countOf(item); })) {
        push({.kind = u.questComplete ? EventKind::QuestComplete : EventKind::QuestUpdate, .value = u.quest, .extra = u.progress});
    }
}

void Session::gainXp(const std::int64_t xp, const Vec2 at)
{
    if (xp <= 0) {
        return;
    }
    const gamelib::core::LevelUp up = m_char.gainXp(xp);
    if (up.happened()) {
        push({.kind = EventKind::LevelUp, .value = up.to, .at = at, .onHero = true});
        refreshQuests();
        refreshDerived();
    }
}

// ---------------------------------------------------------------------------
// Windows

bool Session::spendPoint(const Core stat)
{
    const bool ok = m_char.spendPoint(stat);
    refreshDerived();
    if (ok) {
        push({.kind = EventKind::Point, .value = static_cast<int>(stat)});
    }
    return ok;
}

int Session::autoAssign()
{
    const int spent = m_char.autoAssign();
    refreshDerived();
    if (spent > 0) {
        push({.kind = EventKind::Point, .value = -1});
    }
    return spent;
}

bool Session::canLearn(const SkillId skill) const
{
    const int id = static_cast<int>(skill);
    const SkillInfo& info = skillInfo(skill);
    if (skill == SkillId::HyperJump && m_char.skills.levelOf(id) == 0) {
        return false; // Pip teaches it
    }
    if (info.job != Job::Beginner && info.job != m_char.job) {
        return false;
    }
    return m_char.skills.points() > 0 && m_char.skills.levelOf(id) < m_char.skills.maxLevelOf(id) && m_char.level >= info.unlockLevel;
}

gamelib::core::LearnResult Session::learnSkill(const SkillId skill)
{
    if (!canLearn(skill)) {
        return gamelib::core::LearnResult::MissingPrerequisite;
    }
    const gamelib::core::LearnResult r = m_char.skills.learn(static_cast<int>(skill), m_char.level);
    if (r == gamelib::core::LearnResult::Learned) {
        m_char.refreshResources();
        refreshDerived();
        push({.kind = EventKind::SkillLearned, .value = static_cast<int>(skill)});
    }
    return r;
}

bool Session::equipFrom(const int bagSlot)
{
    const gamelib::core::EquipResult r = m_char.gear.equipFrom(m_char.bag(Tab::Equip), bagSlot, m_char.wearer());
    if (r == gamelib::core::EquipResult::Done) {
        m_char.refreshResources();
        refreshDerived();
        push({.kind = EventKind::Equip});
        return true;
    }
    return false;
}

bool Session::unequip(const Slot slot)
{
    const gamelib::core::EquipResult r = m_char.gear.unequipTo(m_char.bag(Tab::Equip), static_cast<int>(slot));
    if (r == gamelib::core::EquipResult::Done) {
        m_char.refreshResources();
        refreshDerived();
        push({.kind = EventKind::Equip});
        return true;
    }
    if (r == gamelib::core::EquipResult::InventoryFull) {
        push({.kind = EventKind::Full});
    }
    return false;
}

bool Session::useAt(const Tab tab, const int bagSlot)
{
    const ItemStack stack = m_char.bag(tab).at(bagSlot);
    if (stack.empty()) {
        return false;
    }
    if (tab == Tab::Equip) {
        return equipFrom(bagSlot);
    }
    if (tab != Tab::Use) {
        return false;
    }
    const ItemInfo& info = itemInfo(stack.item);
    if (stack.item == kReturnScroll) {
        if (m_map == nearestTown(m_map) || (m_boss && m_boss->fighting())) {
            return false;
        }
        (void)m_char.bag(tab).takeAt(bagSlot, 1);
        const MapId town = nearestTown(m_map);
        m_travel = Travel{.map = town, .portal = 0, .feet = mapDef(town).portals.front().at + Vec2{kTile * 2.0f, 0.0f}};
        push({.kind = EventKind::Potion, .value = stack.item});
        return true;
    }
    if ((info.heal > 0 && info.mana == 0 && m_char.hp.full()) || (info.mana > 0 && info.heal == 0 && m_char.mp.full())
        || (info.heal > 0 && info.mana > 0 && m_char.hp.full() && m_char.mp.full())) {
        return false;
    }
    m_char.hp.restore(info.heal);
    m_char.mp.restore(info.mana);
    (void)m_char.bag(tab).takeAt(bagSlot, 1);
    ++m_stats.potions;
    push({.kind = EventKind::Potion, .value = info.heal > 0 ? info.heal : info.mana, .extra = info.heal > 0 ? 0 : 1,
          .at = m_hero.body.box.center(), .onHero = true});
    refreshCollectQuests();
    return true;
}

bool Session::useItem(const int item)
{
    const Tab tab = tabOf(item);
    if (const std::optional<int> slot = m_char.bag(tab).firstSlotOf(item)) {
        if (useAt(tab, *slot)) {
            return true;
        }
        push({.kind = EventKind::Cooldown, .value = -1});
        return false;
    }
    push({.kind = EventKind::NoMana, .value = -1 - item});
    return false;
}

bool Session::moveInBag(const Tab tab, const int from, const int to)
{
    return m_char.bag(tab).move(from, to);
}

bool Session::dropFromBag(const Tab tab, const int bagSlot)
{
    const ItemStack stack = m_char.bag(tab).at(bagSlot);
    if (stack.empty() || (stack.item >= kNanaLetter && stack.item < 300)) {
        return false; // quest items stay
    }
    (void)m_char.bag(tab).takeAt(bagSlot, stack.count);
    spawnLoot({{.stack = stack}}, m_hero.body.box.center());
    push({.kind = EventKind::Drop});
    refreshCollectQuests();
    return true;
}

std::vector<gamelib::core::ShopEntry> Session::shopEntries() const
{
    std::vector<gamelib::core::ShopEntry> e;
    const auto add = [&](std::initializer_list<int> ids) {
        for (const int id : ids) {
            e.push_back({id});
        }
    };
    const NpcId who = m_talking.value_or(NpcId::Hettie);
    switch (who) {
    case NpcId::Hettie: add({kRedPotion, kBluePotion, kReturnScroll, 1001, 1002, 1100, 1209, 1302, 1301, 1401, 1500}); break;
    case NpcId::Dottie: add({kRedPotion, kOrangePotion, kBluePotion, kVioletPotion, kReturnScroll, 1201, 1402, 1101}); break;
    case NpcId::Garrick:
        add({1010, 1011, 1020, 1021, 1102, 1105, 1202, 1205, 1304, 1012, 1022, 1103, 1106, 1203, 1206, 1303, 1403, 1406, 1501, 1502});
        break;
    case NpcId::Lila: add({kOrangePotion, kWhitePotion, kVioletPotion, kHoneyElixir, kReturnScroll, 1013, 1023, 1108, 1208, 1404}); break;
    case NpcId::Tamsin:
        add({kOrangePotion, kWhitePotion, kVioletPotion, kHoneyElixir, kReturnScroll, 1014, 1024, 1104, 1107, 1204, 1207, 1305, 1306, 1405,
             1503});
        break;
    default: add({kRedPotion, kBluePotion, kReturnScroll}); break;
    }
    return e;
}

gamelib::core::TradeResult Session::buy(const int entry, const int count)
{
    const std::vector<gamelib::core::ShopEntry> entries = shopEntries();
    if (entry < 0 || entry >= static_cast<int>(entries.size())) {
        return gamelib::core::TradeResult::UnknownItem;
    }
    const int item = entries[static_cast<std::size_t>(entry)].item;
    gamelib::core::Shop shop{itemRegistry(), entries};
    const gamelib::core::TradeResult r = shop.buy(entry, count, m_char.bag(tabOf(item)), m_char.coins);
    if (r == gamelib::core::TradeResult::Done) {
        push({.kind = EventKind::Bought, .value = item});
        refreshCollectQuests();
    } else if (r == gamelib::core::TradeResult::InventoryFull) {
        push({.kind = EventKind::Full});
    }
    return r;
}

bool Session::sell(const Tab tab, const int bagSlot)
{
    const ItemStack stack = m_char.bag(tab).at(bagSlot);
    if (stack.empty() || (stack.item >= kNanaLetter && stack.item < 300)) {
        return false;
    }
    const std::int64_t before = m_char.coins;
    gamelib::core::Shop shop{itemRegistry(), shopEntries()};
    const auto price = [](const ItemStack& s) { return sellValue(s); };
    if (shop.sell(m_char.bag(tab), bagSlot, stack.count, m_char.coins, price) != gamelib::core::TradeResult::Done) {
        return false;
    }
    push({.kind = EventKind::Sold, .value = static_cast<int>(m_char.coins - before)});
    refreshCollectQuests();
    return true;
}

// ---------------------------------------------------------------------------
// The tick

void Session::tick(const Input& input)
{
    ++m_ticks;
    ++m_stats.ticks;
    ++m_mapTicks;
    for (int& t : m_bossRespawn) {
        t = std::max(0, t - 1);
    }
    if (m_travel) {
        return; // the app runs the transition, then calls travel()
    }
    m_clock.tick();
    for (const int expired : m_char.buffs.tick()) {
        push({.kind = EventKind::BuffEnd, .value = expired});
        m_char.refreshResources();
    }
    (void)m_char.skills.tick();
    refreshDerived();
    if (m_phase == Phase::Dead) {
        ++m_deadTicks;
        m_hero.previous = m_hero.body.box;
        tickMonsters();
        return;
    }
    tickHero(input);
    tickAttacks();
    tickMonsters();
    tickBoss();
    tickShots();
    tickHazards();
    tickDrops(input);
    tickSpawns();
    // Recovery, MapleStory-style: a little every few seconds, more standing still.
    ++m_regenTicks;
    if (m_regenTicks >= 5 * kTickRate) {
        m_regenTicks = 0;
        const int rest = 100 + skillLevelValues(SkillId::CatchBreath, std::max(1, m_char.skillLevel(SkillId::CatchBreath))).value
                                   * (m_char.skillLevel(SkillId::CatchBreath) > 0 ? 1 : 0);
        const int still = m_hero.idleTicks > 2 * kTickRate ? 2 : 1;
        m_char.hp.restore(std::max(1, m_char.hp.max / 50) * rest * still / 100);
        m_char.mp.restore(std::max(1, m_char.mp.max / 40) * rest * still / 100);
    }
}

void Session::useSkill(const SkillId skill)
{
    const int id = static_cast<int>(skill);
    const SkillInfo& info = skillInfo(skill);
    if (m_char.skills.levelOf(id) == 0 || info.kind == SkillKind::Passive || skill == SkillId::HyperJump) {
        return;
    }
    const gamelib::core::UseCheck check = m_char.skills.check(id, m_char.mp);
    if (check == gamelib::core::UseCheck::NotEnoughResource) {
        push({.kind = EventKind::NoMana, .value = id, .at = m_hero.body.box.center()});
        return;
    }
    if (check != gamelib::core::UseCheck::Ready) {
        push({.kind = EventKind::Cooldown, .value = id});
        return;
    }
    const std::optional<gamelib::core::SkillUse> use = m_char.skills.use(id, m_char.mp);
    if (!use) {
        return;
    }
    Hero& h = m_hero;
    h.attack.cancel();
    h.prone = false;
    const int level = use->level;
    const int value = use->values.value;
    const int facing = h.body.facing;
    const float f = static_cast<float>(facing);
    const Vec2 c = h.body.box.center();
    const float floorY = h.body.box.bottom();
    push({.kind = EventKind::SkillCast, .value = id, .extra = level, .at = c, .facing = facing});
    h.casting = skill;
    const auto cast = [&](const int ticks) {
        h.castLength = ticks;
        h.castTicks = ticks;
    };
    switch (skill) {
    case SkillId::PebbleToss: {
        cast(18);
        const Vec2 from = c + Vec2{f * 30.0f, -20.0f};
        (void)m_shots.spawn({.position = from, .velocity = {f * 13.0f, -1.5f}, .size = {26.0f, 26.0f}, .lifetimeTicks = 50, .team = kTeamHero,
                             .owner = 1, .damage = 1, .tileContact = gamelib::core::TileContact::Remove, .gravity = 0.08f,
                             .extra = {.hero = true, .look = ShotLook::Pebble, .targetsLeft = 1, .percent = value}});
        break;
    }
    case SkillId::PowerStrike:
        cast(24);
        (void)h.skill.start({.startup = 7, .active = 4, .recovery = 13, .hitbox = {-10.0f, -90.0f, 150.0f, 130.0f}, .damage = value,
                             .knockback = {5.0f, -4.0f}, .stunTicks = 20, .hitStopTicks = 5, .cancelFrom = -1, .id = 20},
                            facing);
        break;
    case SkillId::Cleave:
        cast(28);
        (void)h.skill.start({.startup = 8, .active = 6, .recovery = 14, .hitbox = {-150.0f, -130.0f, 330.0f, 180.0f}, .damage = value,
                             .knockback = {5.0f, -4.5f}, .stunTicks = 18, .hitStopTicks = 4, .cancelFrom = -1, .id = 21},
                            facing);
        break;
    case SkillId::Quake: {
        cast(34);
        // The slam lands on tick 12; the shockwave runs out along the floor.
        m_delayed.push_back({.ticks = 12, .area = {c.x - 420.0f, floorY - 120.0f, 840.0f, 140.0f}, .percent = value, .targets = skillTargets(skill, level),
                             .stun = 90, .hitIndex = 0, .skill = skill});
        break;
    }
    case SkillId::Rush:
        cast(22);
        h.rushTicks = 18;
        h.vulnerable.makeInvincible(24);
        break;
    case SkillId::ArcaneBolt: {
        cast(16);
        const Vec2 from = c + Vec2{f * 36.0f, -16.0f};
        const int targets = skillTargets(skill, level);
        (void)m_shots.spawn({.position = from, .velocity = {f * 13.0f, 0.0f}, .size = {34.0f, 34.0f}, .lifetimeTicks = 64, .team = kTeamHero,
                             .owner = 1, .damage = 1, .pierce = targets > 1, .maxTargets = targets,
                             .motion = gamelib::core::ProjectileMotion::Homing, .tileContact = gamelib::core::TileContact::Ignore,
                             .homingTurn = 0.1f, .target = from + Vec2{f * 400.0f, 0.0f},
                             .extra = {.hero = true, .look = ShotLook::Bolt, .targetsLeft = targets, .percent = value, .magic = true}});
        break;
    }
    case SkillId::ChainSpark: {
        cast(20);
        // The nearest monster ahead, then hops to the nearest not yet struck.
        const int targets = skillTargets(skill, level);
        std::vector<std::size_t> struck;
        Vec2 from = c + Vec2{f * 30.0f, -14.0f};
        for (int n = 0; n < targets; ++n) {
            float best = n == 0 ? 520.0f : 320.0f;
            std::optional<std::size_t> pick;
            for (std::size_t i = 0; i < m_monsters.size(); ++i) {
                const Monster& m = m_monsters[i];
                if (!m.active() || std::ranges::find(struck, i) != struck.end()) {
                    continue;
                }
                const Vec2 d = m.box.center() - from;
                if (n == 0 && d.x * f < -10.0f) {
                    continue;
                }
                if (n == 0 && std::abs(d.y) > 160.0f) {
                    continue;
                }
                const float dist = std::sqrt(d.x * d.x + d.y * d.y);
                if (dist < best) {
                    best = dist;
                    pick = i;
                }
            }
            if (!pick) {
                break;
            }
            struck.push_back(*pick);
            Monster& m = m_monsters[*pick];
            push({.kind = EventKind::Chain, .value = n, .at = from, .to = m.box.center()});
            from = m.box.center();
            hitMonster(m, value, true, 30, 0.6f, n, facing);
        }
        if (struck.empty() && m_boss && m_boss->fighting() && std::abs(m_boss->box.center().x - c.x) < 560.0f) {
            push({.kind = EventKind::Chain, .value = 0, .at = from, .to = m_boss->box.center()});
            hitBoss(value, true, 0);
        } else if (struck.empty()) {
            push({.kind = EventKind::Chain, .value = 0, .at = from, .to = from + Vec2{f * 260.0f, 0.0f}});
        }
        break;
    }
    case SkillId::Starfall: {
        cast(30);
        const Rect area{facing > 0 ? c.x - 40.0f : c.x - 560.0f, floorY - 380.0f, 600.0f, 420.0f};
        const int targets = skillTargets(skill, level);
        push({.kind = EventKind::Star, .value = 0, .at = {area.center().x, area.top()}, .to = {area.width, area.height}});
        m_delayed.push_back({.ticks = 14, .area = area, .percent = value, .magic = true, .targets = targets, .hitIndex = 0, .skill = skill});
        m_delayed.push_back({.ticks = 28, .area = area, .percent = value, .magic = true, .targets = targets, .hitIndex = 1, .skill = skill});
        break;
    }
    case SkillId::IronBody:
        cast(24);
        m_char.buffs.apply(kBuffIronBody, {.defense = value}, use->values.durationTicks);
        push({.kind = EventKind::Buff, .value = id, .at = c, .onHero = true});
        break;
    case SkillId::WarCry:
        cast(28);
        m_char.buffs.apply(kBuffWarCry, {.attackPercent = value}, use->values.durationTicks);
        push({.kind = EventKind::Buff, .value = id, .at = c, .onHero = true});
        break;
    case SkillId::ManaShield:
        cast(24);
        m_char.buffs.apply(kBuffManaShield, {.shield = value}, use->values.durationTicks);
        push({.kind = EventKind::Buff, .value = id, .at = c, .onHero = true});
        break;
    case SkillId::Swiftwind:
        cast(24);
        m_char.buffs.apply(kBuffSwiftwind, {.speed = value, .jump = value / 2}, use->values.durationTicks);
        push({.kind = EventKind::Buff, .value = id, .at = c, .onHero = true});
        break;
    case SkillId::Bloom: {
        cast(30);
        const int amount = m_char.hp.max * value / 100 + m_derived.magic * 2;
        m_char.hp.restore(amount);
        push({.kind = EventKind::Heal, .value = amount, .at = c, .onHero = true});
        break;
    }
    default: break;
    }
    m_char.refreshResources();
    refreshDerived();
}

void Session::tickHero(const Input& input)
{
    Hero& h = m_hero;
    gamelib::core::PlatformerBody& b = h.body;
    h.previous = b.box;
    h.vulnerable.tick();
    h.hurtTicks = std::max(0, h.hurtTicks - 1);
    h.hyperTicks = std::max(0, h.hyperTicks - 1);
    h.pickupCooldown = std::max(0, h.pickupCooldown - 1);
    const gamelib::core::TileWorld w = world();
    const bool free = h.hurtTicks == 0 && h.castTicks == 0 && h.rushTicks == 0;

    // Up: talk to someone, else take a portal - before it reaches the ladders.
    bool upUsed = false;
    if (input.upPressed && b.grounded && free) {
        if (const std::optional<NpcId> npc = npcInReach()) {
            m_talk = *npc;
            startTalk(*npc);
            upUsed = true;
        } else {
            const auto open = [this](const int exit) { return portalOpen(exitPortal(exit).second); };
            if (const std::optional<gamelib::core::RoomChange> change =
                    worldGraph().exitAt(static_cast<int>(m_map), h.previous, b.box, true, open)) {
                const auto [toMap, toPortal] = exitPortal(worldGraph().exits()[static_cast<std::size_t>(change->exit)].target);
                m_travel = Travel{.map = toMap, .portal = toPortal};
                push({.kind = EventKind::Travel, .value = -1, .at = b.box.bottomCenter()});
                return;
            }
            if (const int p = portalInReach(); p >= 0) {
                push({.kind = EventKind::PortalClosed, .at = map().portals[static_cast<std::size_t>(p)].at});
                upUsed = true;
            }
        }
    }
    if (m_talk) {
        return;
    }

    if (input.useItem != 0) {
        (void)useItem(input.useItem);
    }
    if (free && input.skills != 0 && !b.climbing) {
        for (int i = 0; i < kSkillCount; ++i) {
            if (input.skill(static_cast<SkillId>(i))) {
                useSkill(static_cast<SkillId>(i));
                break;
            }
        }
    }
    // The basic attack: held, it repeats.
    if (input.attackHeld && h.hurtTicks == 0 && h.castTicks == 0 && h.rushTicks == 0 && !b.climbing && !h.attack.busy()) {
        if (input.move != 0 && b.grounded) {
            b.facing = input.move;
        }
        h.prone = false;
        if (h.attack.start(basicClip(weaponStyle()), b.facing) == gamelib::core::AttackPress::Started) {
            push({.kind = EventKind::Swing, .value = weaponStyle(), .at = b.box.center(), .facing = b.facing});
        }
    }
    // Prone: down on the ground, lying flat under low shots.
    const bool wasProne = h.prone;
    h.prone = b.grounded && !b.climbing && input.climb > 0 && input.move == 0 && !input.jumpPressed && free && !h.attack.busy();
    if (h.prone && !wasProne) {
        push({.kind = EventKind::Prone, .at = b.box.bottomCenter()});
    }

    gamelib::core::PlatformerIntent intent;
    intent.move = static_cast<float>(input.move);
    intent.climb = upUsed ? 0.0f : static_cast<float>(input.climb);
    intent.jumpPressed = input.jumpPressed;
    intent.jumpHeld = input.jumpHeld;
    const bool attacking = h.attack.busy() && h.attack.phase() != gamelib::core::AttackPhase::Recovery;
    if (h.hurtTicks > 0 || h.castTicks > 0 || h.rushTicks > 0 || (attacking && b.grounded) || h.prone) {
        intent.move = 0.0f;
        if (!(h.prone && input.jumpPressed)) {
            intent.climb = h.prone ? 1.0f : 0.0f; // prone keeps down held (drop through with jump)
        }
    }
    if (h.castTicks > 0 || h.hurtTicks > 0 || h.rushTicks > 0) {
        intent.jumpPressed = false;
    }
    if (intent.jumpPressed && attacking && b.grounded) {
        intent.jumpPressed = false;
    }
    if (intent.jumpPressed && h.attack.busy()) {
        h.attack.cancel(); // a jump in the recovery cancels the swing
    }

    gamelib::core::PlatformerTuning tuning = m_char.tuning();
    // The hyper jump: a jump in the air while it is learned and paid for.
    const int hyperId = static_cast<int>(SkillId::HyperJump);
    intent.hyperJump = free && m_char.skills.canUse(hyperId, m_char.mp);
    if (intent.jumpPressed && free && !intent.hyperJump && m_char.skillLevel(SkillId::HyperJump) > 0
        && gamelib::core::hyperJumpReady(b, tuning, w)) {
        push({.kind = EventKind::NoMana, .value = hyperId, .at = b.box.center()});
    }
    if (h.rushTicks > 0) {
        --h.rushTicks;
        b.velocity.x = static_cast<float>(b.facing) * 11.0f * (0.5f + 0.5f * static_cast<float>(h.rushTicks) / 18.0f);
        b.lockTicks = std::max(b.lockTicks, 1);
        // Carry what is in the way along.
        const Rect front = b.box.expanded(30.0f);
        for (Monster& m : m_monsters) {
            if (m.active() && overlapsRect(m.box, front) && m.kind().behavior != Behavior::Flyer) {
                const gamelib::core::BoxMove moved = gamelib::core::moveBox(m.box, {b.velocity.x, 0.0f}, w);
                m.box = moved.box;
            }
        }
        if (h.rushTicks % 6 == 5) {
            const int targets = skillTargets(SkillId::Rush, m_char.skillLevel(SkillId::Rush));
            const int value = skillLevelValues(SkillId::Rush, m_char.skillLevel(SkillId::Rush)).value;
            (void)strike(b.box.expanded(40.0f), value / 3, false, targets, ++m_attackId, 12, 0.3f, (18 - h.rushTicks) / 6);
        }
    }
    const bool wasClimbing = b.climbing;
    const gamelib::core::PlatformerStep step = gamelib::core::stepPlatformer(b, intent, tuning, w, {});
    if (step.events.hyperJumped) {
        (void)m_char.skills.use(hyperId, m_char.mp);
        h.hyperUp = step.events.hyperUp;
        h.hyperTicks = 26;
        h.attack.cancel();
        push({.kind = EventKind::HyperJump, .value = h.hyperUp ? 1 : 0, .at = b.box.center(), .facing = b.facing});
    }
    if (step.events.jumped || step.events.climbJumped) {
        push({.kind = EventKind::Jump, .at = b.box.bottomCenter()});
    }
    if (step.events.landed) {
        push({.kind = EventKind::Land, .value = h.airTicks, .at = b.box.bottomCenter()});
        h.landTicks = 0;
    }
    h.airTicks = b.grounded || b.climbing ? 0 : h.airTicks + 1;
    if (b.climbing && !wasClimbing) {
        push({.kind = EventKind::Climb, .at = b.box.center()});
        h.attack.cancel();
    }
    ++h.landTicks;
    h.idleTicks = (std::abs(b.velocity.x) < 0.1f && b.grounded && !h.attack.busy() && h.castTicks == 0) ? h.idleTicks + 1 : 0;
    if (h.castTicks > 0) {
        --h.castTicks;
    }
    // Lying down, the body is short (shots fly over it).
    h.vulnerable.setHurtbox(h.prone ? Rect{b.box.x - 10.0f, b.box.bottom() - 40.0f, b.box.width + 20.0f, 40.0f} : b.box.expanded(-4.0f));
}

void Session::tickAttacks()
{
    Hero& h = m_hero;
    const Vec2 anchor = h.body.box.center();
    const gamelib::core::AttackStep basic = h.attack.tick(anchor);
    if (basic.hit) {
        (void)strike(basic.hit->box, 100, false, 1, basic.hit->attackId, basic.hit->stunTicks, 1.0f);
    }
    const gamelib::core::AttackStep skill = h.skill.tick(anchor);
    if (skill.hit) {
        const int level = m_char.skillLevel(h.casting);
        const int targets = skillTargets(h.casting, std::max(level, 1));
        const int hits = h.casting == SkillId::PowerStrike && level >= 10 ? 2 : 1;
        for (int i = 0; i < hits; ++i) {
            (void)strike(skill.hit->box, skill.hit->damage, false, targets, skill.hit->attackId * 4 + i, skill.hit->stunTicks, 1.0f, i);
        }
    }
    // Delayed strikes (Starfall's waves, Quake's shockwave).
    for (Delayed& d : m_delayed) {
        if (--d.ticks == 0) {
            const int struck = strike(d.area, d.percent, d.magic, d.targets, ++m_attackId, d.stun, d.skill == SkillId::Quake ? 0.4f : 0.6f, d.hitIndex);
            push({.kind = EventKind::SkillHit, .value = static_cast<int>(d.skill), .extra = struck, .at = d.area.center(),
                  .to = {d.area.width, d.area.height}});
        }
    }
    std::erase_if(m_delayed, [](const Delayed& d) { return d.ticks <= 0; });

    // Monster bodies hurt on touch.
    if (h.vulnerable.invincible() || !h.alive) {
        return;
    }
    for (Monster& mon : m_monsters) {
        if (!monsterTouches(mon)) {
            continue;
        }
        if (overlapsRect(mon.box.expanded(-6.0f), h.vulnerable.hurtbox())) {
            const int atk = monsterAttack(mon.level, mon.elite) * (mon.state == MonsterState::Attack ? 13 : 10) / 10;
            damageHero(std::max(1, atk * m_rng.range(90, 110) / 100 - m_derived.defense / 2), mon.box.center().x);
            return;
        }
    }
}

int Session::strike(const Rect& area, const int percent, const bool magic, const int targets, const int attackId, const int stunTicks,
                    const float push, const int hitIndex)
{
    const gamelib::core::Hit hit{.box = area, .team = kTeamHero, .attackId = attackId};
    std::vector<std::size_t> candidates;
    for (std::size_t i = 0; i < m_monsters.size(); ++i) {
        if (m_monsters[i].active() && m_monsters[i].vulnerable.canBeHitBy(hit)) {
            candidates.push_back(i);
        }
    }
    const Vec2 c = m_hero.body.box.center();
    std::ranges::sort(candidates, [&](const std::size_t a, const std::size_t b) {
        return std::abs(m_monsters[a].box.center().x - c.x) < std::abs(m_monsters[b].box.center().x - c.x);
    });
    int struck = 0;
    for (const std::size_t i : candidates) {
        if (struck >= targets) {
            break;
        }
        Monster& m = m_monsters[i];
        (void)m.vulnerable.tryHit(hit);
        hitMonster(m, percent, magic, stunTicks, push, hitIndex, m.box.center().x >= c.x ? 1 : -1);
        ++struck;
    }
    if (struck < targets && m_boss && m_boss->fighting() && m_boss->fade > 0.5f && m_boss->vulnerable.tryHit(hit)) {
        hitBoss(percent, magic, hitIndex);
        ++struck;
    }
    return struck;
}

void Session::hitMonster(Monster& m, const int percent, const bool magic, const int stunTicks, const float push, const int hitIndex,
                         const int direction)
{
    if (!m.active()) {
        return;
    }
    bool crit = false;
    const int dmg = damageRoll(percent, magic, monsterDefense(m.level), crit);
    hurtMonster(m, dmg, direction, static_cast<int>(m_ticks), stunTicks > 14 ? stunTicks : 0, push);
    m_stats.bestHit = std::max(m_stats.bestHit, dmg);
    this->push({.kind = EventKind::Hit, .value = dmg, .extra = hitIndex, .at = {m.box.center().x, m.box.top()}, .crit = crit,
                .facing = direction});
    if (!m.alive()) {
        defeatMonster(m);
    }
}

void Session::hitBoss(const int percent, const bool magic, const int hitIndex)
{
    Boss& b = *m_boss;
    bool crit = false;
    const int dmg = damageRoll(percent, magic, monsterDefense(b.info().level), crit);
    m_stats.bestHit = std::max(m_stats.bestHit, dmg);
    push({.kind = EventKind::Hit, .value = dmg, .extra = hitIndex, .at = {b.box.center().x, b.box.top()}, .crit = crit});
    if (hurtBoss(b, dmg, static_cast<int>(m_ticks))) {
        push({.kind = EventKind::BossDown, .value = static_cast<int>(b.id), .at = b.box.center()});
    }
}

int Session::damageRoll(const int percent, const bool magic, const int defense, bool& crit)
{
    const int power = magic ? m_derived.magic : m_derived.attack;
    int raw = power * percent / 100 * m_rng.range(88, 112) / 100;
    crit = m_rng.range(0, 99) < m_derived.crit;
    if (crit) {
        raw = raw * 160 / 100;
    }
    return std::max(1, raw - defense / 2);
}

void Session::damageHero(int damage, const float fromX)
{
    Hero& h = m_hero;
    if (!h.alive || h.vulnerable.invincible()) {
        return;
    }
    // Mana Shield pays part of the hit with MP.
    if (const int shield = m_char.buffShield(); shield > 0) {
        const int fromMp = std::min(damage * shield / 100, m_char.mp.current);
        m_char.mp.restore(-fromMp);
        damage -= fromMp;
        if (fromMp > 0) {
            push({.kind = EventKind::ShieldHit, .value = fromMp, .at = h.body.box.center(), .onHero = true});
        }
    }
    m_char.hp.restore(-damage);
    h.vulnerable.makeInvincible(kHurtInvincible);
    const bool iron = m_char.buffs.active(kBuffIronBody);
    h.prone = false;
    if (!iron) {
        h.hurtTicks = kHurtStun;
        h.attack.cancel();
        h.skill.cancel();
        h.castTicks = 0;
        h.rushTicks = 0;
        const int dir = h.body.box.center().x >= fromX ? 1 : -1;
        h.body.climbing = false;
        gamelib::core::launch(h.body, {static_cast<float>(dir) * 4.2f, -5.0f}, false);
        h.body.lockTicks = kHurtStun;
    }
    push({.kind = EventKind::Hurt, .value = damage, .at = {h.body.box.center().x, h.body.box.top()}, .onHero = true});
    if (m_char.hp.current <= 0) {
        h.alive = false;
        m_phase = Phase::Dead;
        m_deadTicks = 0;
        ++m_stats.deaths;
        push({.kind = EventKind::Death, .at = h.body.box.center()});
    }
}

void Session::defeatMonster(Monster& m)
{
    ++m_stats.defeated;
    if (m.elite) {
        ++m_stats.elites;
    }
    push({.kind = EventKind::MonsterDown, .value = static_cast<int>(m.species), .extra = m.elite ? 1 : 0, .at = m.box.center()});
    questEvent({.kind = ObjectiveKind::Defeat, .subject = static_cast<int>(m.species)});
    dropLoot(m);
    gainXp(monsterXp(m.level, m.elite), m.box.center());
}

void Session::dropLoot(const Monster& m)
{
    std::vector<Loot> loot;
    const std::int64_t coins = static_cast<std::int64_t>(m.level * 3 + m_rng.range(0, m.level * 2 + 3)) * (m.elite ? 4 : 1);
    loot.push_back({.coins = coins});
    if (m.elite || m_rng.chance(0.5f)) {
        loot.push_back({.stack = {.item = dropItem(m.species), .count = m.elite ? 3 : 1}});
    }
    if (m_rng.chance(m.elite ? 0.35f : 0.07f)) {
        const bool hp = m_rng.chance(0.6f);
        const int item = hp ? (m.level < 12 ? kRedPotion : m.level < 24 ? kOrangePotion : kWhitePotion)
                            : (m.level < 18 ? kBluePotion : kVioletPotion);
        loot.push_back({.stack = {.item = item, .count = 1}});
    }
    if (m_rng.chance(m.elite ? 0.45f : 0.035f)) {
        loot.push_back({.stack = rollDropEquipment(m.level, m_rng, m.elite)});
    }
    if (m_rng.chance(0.01f)) {
        loot.push_back({.stack = {.item = kReturnScroll, .count = 1}});
    }
    if (m_map == MapId::LighthouseHill && m_quests.stateOf(kQuestDimming) == QuestState::Active && m_char.countOf(kLighthouseLens) == 0
        && (m.elite || m_rng.chance(0.2f))) {
        loot.push_back({.stack = {.item = kLighthouseLens, .count = 1}});
    }
    spawnLoot(std::move(loot), m.box.center());
}

void Session::spawnLoot(std::vector<Loot> loot, const Vec2 at)
{
    for (const Loot& l : loot) {
        if (!l.stack.empty()) {
            push({.kind = EventKind::Spawned, .value = l.stack.item, .extra = 100 + l.stack.rarity, .at = at});
        }
    }
    m_drops.scatter(std::span<const Loot>{loot}, at, m_rng);
}

void Session::tickDrops(const Input& input)
{
    m_drops.step(world());
    if (!input.pickup || m_hero.pickupCooldown > 0 || !m_hero.alive) {
        return;
    }
    // One drop per press, then a quick repeat while the key is held.
    if (Drops::Drop* d = m_drops.under(m_hero.body.box.expanded(18.0f))) {
        Loot& loot = d->value;
        const Vec2 at = d->box.center();
        m_hero.pickupCooldown = 6;
        if (loot.coins > 0) {
            m_char.coins += loot.coins;
            m_stats.coinsEarned += loot.coins;
            push({.kind = EventKind::Coins, .value = static_cast<int>(loot.coins), .at = at});
            (void)m_drops.take(d->id);
            return;
        }
        const int before = loot.stack.count;
        const int left = m_char.add(loot.stack);
        if (left == before) {
            push({.kind = EventKind::Full});
            return;
        }
        push({.kind = EventKind::Pickup, .value = loot.stack.item, .extra = loot.stack.rarity, .at = at});
        if (left == 0) {
            (void)m_drops.take(d->id);
        } else {
            loot.stack.count = left;
        }
        refreshCollectQuests();
    }
}

void Session::tickMonsters()
{
    const gamelib::core::TileWorld w = world();
    const HeroView hero{.box = m_hero.body.box, .alive = m_hero.alive && m_phase == Phase::Playing};
    for (Monster& m : m_monsters) {
        const MonsterStep step = stepMonster(m, hero, w, static_cast<int>(m_ticks));
        if (step.lunged) {
            push({.kind = EventKind::EnemyShot, .value = static_cast<int>(m.species), .at = m.box.center()});
        }
        if (step.finished && m.spawn >= 0 && m.spawn < m_spawner.size()) {
            m_spawner.killed(m.spawn);
        }
        if (m.box.top() > map().bounds().bottom() + 200.0f && m.alive()) {
            m.state = MonsterState::Defeat;
            m.stateTicks = m.stateLength = 1;
        }
    }
    std::erase_if(m_monsters, [](const Monster& m) { return m.state == MonsterState::Defeat && m.stateTicks >= m.stateLength; });
}

void Session::tickSpawns()
{
    const std::span<const int> due = m_spawner.tick({.avoid = m_hero.body.box.expanded(80.0f), .retryTicks = kTickRate});
    for (const int point : std::vector<int>(due.begin(), due.end())) {
        spawnInto(point, false);
    }
}

void Session::tickBoss()
{
    if (!m_boss) {
        return;
    }
    Boss& b = *m_boss;
    const BossStep step = stepBoss(b, m_hero.body.box, world(), m_rng, static_cast<int>(m_ticks));
    const BossInfo& info = b.info();
    for (const BossShotRequest& s : step.shots) {
        (void)m_shots.spawn({.position = s.at, .velocity = s.velocity, .size = s.size, .lifetimeTicks = s.lifetime, .team = kTeamMonsters,
                             .owner = 2, .damage = bossAttack(b.id),
                             .tileContact = s.groundWave ? gamelib::core::TileContact::Ignore : gamelib::core::TileContact::Remove,
                             .gravity = s.gravity, .extra = {.look = s.look, .groundWave = s.groundWave}});
    }
    if (!step.shots.empty()) {
        push({.kind = EventKind::EnemyShot, .value = -1, .at = b.box.center()});
    }
    for (const float x : step.hazards) {
        m_hazards.add(x, 60 - b.phase * 8);
    }
    if (!step.hazards.empty()) {
        push({.kind = EventKind::HazardWarn, .value = info.hazardsFromBelow ? 1 : 0, .at = b.box.center()});
    }
    if (step.summon > 0) {
        const int alive = static_cast<int>(std::ranges::count_if(m_monsters, [](const Monster& m) { return m.alive(); }));
        for (int i = 0; i < std::min(step.summon, 6 - alive); ++i) {
            const float x = std::clamp(b.box.center().x + (i % 2 == 0 ? -1.0f : 1.0f) * (140.0f + 70.0f * static_cast<float>(i)),
                                       b.bounds.left() + 60.0f, b.bounds.right() - 60.0f);
            const bool flyer = species(info.minion).behavior == Behavior::Flyer;
            const Vec2 feet{x, flyer ? b.bounds.bottom() - 220.0f : b.bounds.bottom()};
            Monster m = makeMonster(info.minion, std::max(1, info.level - 4), false, feet, b.bounds.left(), b.bounds.right(),
                                    static_cast<std::uint8_t>(m_rng.range(0, 255)));
            m.aggro = 900;
            m_monsters.push_back(m);
            push({.kind = EventKind::Spawned, .value = static_cast<int>(info.minion), .at = feet});
        }
    }
    if (step.landed) {
        push({.kind = EventKind::BossLand, .at = b.box.bottomCenter()});
    }
    if (step.roared) {
        push({.kind = EventKind::BossRoar, .value = static_cast<int>(b.action), .at = b.box.center()});
    }
    if (step.phaseChanged) {
        push({.kind = EventKind::BossPhase, .value = b.phase, .at = b.box.center()});
    }
    if (step.contact && m_hero.alive && !m_hero.vulnerable.invincible() && overlapsRect(b.box.expanded(-16.0f), m_hero.vulnerable.hurtbox())) {
        damageHero(std::max(1, bossAttack(b.id) * m_rng.range(90, 110) / 100 - m_derived.defense / 2), b.box.center().x);
    }
    if (step.finished) {
        defeatBoss();
    }
}

void Session::defeatBoss()
{
    const Boss b = *m_boss;
    m_boss.reset();
    ++m_stats.defeated;
    ++m_stats.bosses;
    m_bossRespawn[static_cast<std::size_t>(b.id)] = b.info().respawnTicks;
    const int quest = bossQuest(b.id);
    const bool story = m_quests.stateOf(quest) == QuestState::Active;
    questEvent({.kind = ObjectiveKind::Defeat, .subject = kBossSubject + static_cast<int>(b.id)});
    for (Monster& m : m_monsters) {
        if (m.alive()) {
            hurtMonster(m, m.hp, 1, static_cast<int>(m_ticks));
        }
    }
    const Vec2 at = b.box.center();
    std::vector<Loot> loot;
    loot.push_back({.coins = static_cast<std::int64_t>(b.info().level) * 80});
    for (int i = 0; i < 2; ++i) {
        ItemStack gear = rollDropEquipment(b.info().level + 2, m_rng, true);
        gear.rarity = std::max(gear.rarity, 1 + i);
        loot.push_back({.stack = gear});
    }
    loot.push_back({.stack = {.item = b.info().level < 25 ? kOrangePotion : kWhitePotion, .count = 5}});
    spawnLoot(std::move(loot), at);
    if (story) {
        const int shard = b.id == BossId::Thornbark ? kDawnShard : b.id == BossId::Frostmane ? kFrostShard : kHeartShard;
        addItem({.item = shard, .count = 1});
    }
    gainXp(bossXp(b.id), at);
    m_saveWanted = true;
    if (b.id == BossId::Murkmaw && story && !m_finished) {
        m_finished = true;
        m_endingNow = true;
        push({.kind = EventKind::Ending});
    }
}

void Session::tickShots()
{
    // Arcane bolts home on the nearest monster still ahead.
    for (auto& s : m_shots.items()) {
        if (!s.extra.hero) {
            if (s.extra.groundWave) {
                s.velocity.y = 0.0f;
            }
            continue;
        }
        if (s.extra.look != ShotLook::Bolt) {
            continue;
        }
        s.target = s.position + s.velocity * 60.0f;
        const float ahead = s.velocity.x < 0.0f ? -1.0f : 1.0f;
        float best = 700.0f;
        for (const Monster& m : m_monsters) {
            if (!m.active()) {
                continue;
            }
            const Vec2 d = m.box.center() - s.position;
            const float dist = std::sqrt(d.x * d.x + d.y * d.y);
            if (dist < best && d.x * ahead > 20.0f) {
                best = dist;
                s.target = m.box.center();
            }
        }
        if (m_boss && m_boss->fighting()) {
            const Vec2 d = m_boss->box.center() - s.position;
            if (std::sqrt(d.x * d.x + d.y * d.y) < best && d.x * ahead > -20.0f) {
                s.target = m_boss->box.center();
            }
        }
    }
    m_shots.step(1.0f, gamelib::core::shotSolid(world()), map().bounds().expanded(100.0f));
    for (auto& s : m_shots.items()) {
        if (s.extra.hero) {
            const gamelib::core::Hit hit = s.hit();
            for (Monster& m : m_monsters) {
                if (s.extra.targetsLeft <= 0 || !m.active()) {
                    continue;
                }
                if (m.vulnerable.tryHit(hit)) {
                    hitMonster(m, s.extra.percent, s.extra.magic, 0, 0.6f, 0, s.velocity.x >= 0.0f ? 1 : -1);
                    --s.extra.targetsLeft;
                }
            }
            if (s.extra.targetsLeft > 0 && m_boss && m_boss->fighting() && m_boss->vulnerable.tryHit(hit)) {
                hitBoss(s.extra.percent, s.extra.magic, 0);
                s.extra.targetsLeft = 0;
            }
        } else if (m_hero.alive && !m_hero.vulnerable.invincible() && overlapsRect(s.box(), m_hero.vulnerable.hurtbox())) {
            damageHero(std::max(1, s.damage * m_rng.range(90, 110) / 100 - m_derived.defense / 2), s.position.x);
            s.extra.spent = !s.extra.groundWave;
        }
    }
    (void)m_shots.removeIf([](const auto& s) { return s.extra.spent || (s.extra.hero && s.extra.targetsLeft <= 0); });
}

void Session::tickHazards()
{
    if (m_hazards.empty()) {
        return;
    }
    const bool below = m_boss && m_boss->info().hazardsFromBelow;
    const float floorY = map().arena ? map().arena->bounds.bottom() : map().bounds().bottom();
    const ShotLook look = m_boss ? m_boss->info().shot : ShotLook::Gloom;
    const int damage = m_boss ? bossAttack(m_boss->id) : 20;
    for (const float x : m_hazards.tick()) {
        if (below) {
            // Roots burst out of the floor.
            push({.kind = EventKind::HazardStrike, .value = 1, .at = {x, floorY}});
            const Rect spike{x - 34.0f, floorY - 150.0f, 68.0f, 150.0f};
            if (m_hero.alive && !m_hero.vulnerable.invincible() && overlapsRect(spike, m_hero.vulnerable.hurtbox())) {
                damageHero(std::max(1, damage - m_derived.defense / 2), x);
            }
        } else {
            push({.kind = EventKind::HazardStrike, .value = 0, .at = {x, map().bounds().top()}});
            (void)m_shots.spawn({.position = {x, map().bounds().top() + 40.0f}, .velocity = {0.0f, 9.5f}, .size = {40.0f, 40.0f},
                                 .lifetimeTicks = 200, .team = kTeamMonsters, .owner = 3, .damage = damage,
                                 .tileContact = gamelib::core::TileContact::Remove, .extra = {.look = look}});
        }
    }
}

// ---------------------------------------------------------------------------
// Developer aids

void Session::debugTeleport(const MapId target, const int portal)
{
    m_travel = Travel{.map = target, .portal = portal};
    travel();
}

void Session::debugLevel(const int level)
{
    while (m_char.level < std::min(level, kMaxLevel)) {
        m_char.gainXp(m_char.xpToNext());
    }
    m_char.autoAssign();
    refreshQuests();
    refreshDerived();
}

void Session::debugJob(const Job job)
{
    if (m_char.level < kJobLevel) {
        debugLevel(kJobLevel);
    }
    m_char.advance(job);
    const bool warrior = job == Job::Warrior;
    m_char.gear.setAt(static_cast<int>(Slot::Weapon), rollEquipment(warrior ? 1010 : 1020, m_rng, false));
    m_char.gear.setAt(static_cast<int>(Slot::Top), rollEquipment(warrior ? 1202 : 1205, m_rng, false));
    m_char.gear.setAt(static_cast<int>(Slot::Hat), rollEquipment(warrior ? 1102 : 1105, m_rng, false));
    m_char.refreshResources();
    refreshQuests();
    refreshDerived();
}

void Session::debugQuestsTo(const int quest)
{
    // Walks the story up to (not including) `quest`: accepts, completes and
    // turns in every earlier quest whose prerequisites are met.
    for (int round = 0; round < 40; ++round) {
        refreshQuests();
        bool any = false;
        for (const auto& def : questDefs()) {
            if (def.id >= quest || m_quests.stateOf(def.id) == QuestState::TurnedIn) {
                continue;
            }
            if (m_quests.stateOf(def.id) == QuestState::Available) {
                (void)m_quests.accept(def.id);
            }
            if (m_quests.stateOf(def.id) == QuestState::Active) {
                for (const auto& o : def.objectives) {
                    for (int i = 0; i < o.required; ++i) {
                        (void)m_quests.onEvent({.kind = o.kind, .subject = o.subject});
                    }
                    if (o.kind == ObjectiveKind::Collect) {
                        (void)m_quests.setCount(ObjectiveKind::Collect, o.subject, o.required);
                    }
                }
            }
            if (m_quests.stateOf(def.id) == QuestState::Complete && m_quests.turnIn(def.id)) {
                any = true;
                if (def.id == kQuestHopToIt) {
                    m_char.skills.setLevel(static_cast<int>(SkillId::HyperJump), 1);
                }
            }
        }
        if (!any) {
            break;
        }
    }
    refreshQuests();
    (void)m_events.take();
}

void Session::debugBossLow()
{
    if (m_boss) {
        m_boss->hp = std::max(1, m_boss->maxHp / 25);
    }
}

void Session::debugTime(const int hour)
{
    m_clock.setTime(hour, 0);
}

void Session::debugGear()
{
    const Job job = m_char.job;
    for (const int id : equipmentIds()) {
        const gamelib::core::ItemDef& def = itemRegistry().at(id);
        if (def.levelRequirement <= m_char.level && (def.jobs & jobBit(job)) != 0) {
            const Slot slot = static_cast<Slot>(def.equipSlot);
            const ItemStack& worn = m_char.gear.at(static_cast<int>(slot));
            if (worn.empty() || itemRegistry().at(worn.item).levelRequirement <= def.levelRequirement) {
                m_char.gear.setAt(static_cast<int>(slot), rollEquipment(id, m_rng, true));
            }
        }
    }
    m_char.add({.item = kWhitePotion, .count = 30});
    m_char.add({.item = kVioletPotion, .count = 30});
    m_char.add({.item = kReturnScroll, .count = 3});
    m_char.coins += 20000;
    m_char.refreshResources();
    m_char.hp.refill();
    m_char.mp.refill();
    refreshDerived();
}

void Session::debugMove(const float x)
{
    const std::optional<Vec2> feet = gamelib::core::topFloor(world(), static_cast<int>(x / kTile));
    if (!feet) {
        return;
    }
    m_hero.body.box = Rect::fromBottomCenter(*feet, kHeroSize);
    m_hero.previous = m_hero.body.box;
}

void Session::debugLearnAll()
{
    for (int round = 0; round < 16; ++round) {
        for (const SkillId s : jobSkills(m_char.job)) {
            (void)learnSkill(s);
        }
        for (const SkillId s : jobSkills(Job::Beginner)) {
            (void)learnSkill(s);
        }
    }
    (void)m_events.take();
}

} // namespace realm::game
