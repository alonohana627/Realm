#pragma once

// Realm's people and its story: the sixteen townsfolk, the quests (GameLib's
// core::QuestBook) - the island's tutorial chain, the job advancement, the
// main story of the dimming beacon and the side quests - and every
// conversation as a core::DialogueTree built with core::addNpcTalk. Quest
// words ("available 5", "turnin 5") are read back by core::questCondition and
// core::questEffect; the game's own words are the session's ("job beginner",
// "night", "shop", "sail", "rest").

#include "maps.hpp"
#include "rules.hpp"

#include <gamelib/core/dialogue.hpp>
#include <gamelib/core/quests.hpp>

#include <span>
#include <string>
#include <string_view>

namespace realm::game {

[[nodiscard]] std::string_view npcName(NpcId npc);
[[nodiscard]] std::string_view npcRole(NpcId npc);
[[nodiscard]] std::string npcRoot(NpcId npc); // the conversation's start node

// Quests.
enum Quest : int {
    kQuestNewLeaf = 1,
    kQuestHopToIt = 2,
    kQuestHettie = 3,
    kQuestShellShock = 4,
    kQuestAcrossWater = 5,
    kQuestPinch = 6,
    kQuestFeathers = 7,
    kQuestPantry = 8,
    kQuestWarrior = 9,
    kQuestMage = 10,
    kQuestDimming = 11,
    kQuestSeekChief = 12,
    kQuestBranches = 13,
    kQuestSpores = 14,
    kQuestBramble = 15,
    kQuestDelivery = 16,
    kQuestGrove = 17,
    kQuestUpThePass = 18,
    kQuestRations = 19,
    kQuestRockslide = 20,
    kQuestSummit = 21,
    kQuestBeneath = 22,
    kQuestUndercroft = 23,
    kQuestGlimmer = 24,
    kQuestRunes = 25,
    kQuestHeart = 26,
};

// Objective subjects: species ids for monsters, kBossSubject + boss for bosses.
inline constexpr int kBossSubject = 100;

// Quest flags for core::QuestBook::refresh.
inline constexpr int kFlagBeginner = 1;
inline constexpr int kFlagHasJob = 2;

struct QuestText {
    int id = 0;
    std::string_view title;
    std::string_view summary; // one line for the quest log
    bool main = false;        // the main story (and the tutorial)
};

[[nodiscard]] std::span<const gamelib::core::QuestDef> questDefs();
[[nodiscard]] const QuestText& questText(int id);
[[nodiscard]] const gamelib::core::DialogueTree& dialogueTree();
// The quest a boss's defeat completes.
[[nodiscard]] int bossQuest(BossId boss);
// The main story's chapter for the quests turned in (1 island .. 5 the gloom).
[[nodiscard]] int chapterOf(const gamelib::core::QuestBook& book);

} // namespace realm::game
