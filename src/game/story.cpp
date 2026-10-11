#include "story.hpp"

#include "items.hpp"
#include "monsters.hpp"

#include <gamelib/core/quest_talk.hpp>

#include <array>
#include <vector>

namespace realm::game {

namespace {

// helpers

using gamelib::core::NpcTalk;
using gamelib::core::Objective;
using gamelib::core::ObjectiveKind;
using gamelib::core::QuestDef;

constexpr int npc(const NpcId id)
{
    return static_cast<int>(id);
}

constexpr int mob(const SpeciesId id)
{
    return static_cast<int>(id);
}

constexpr int boss(const BossId id)
{
    return kBossSubject + static_cast<int>(id);
}

std::vector<QuestDef> buildQuests()
{
    using N = NpcId;
    using S = SpeciesId;
    using O = ObjectiveKind;
    std::vector<QuestDef> q;
    // Petal Isle: the tutorial chain.
    q.push_back({.id = kQuestNewLeaf, .giver = npc(N::NanaRue), .turnInTo = npc(N::Pip),
                 .objectives = {{O::Talk, npc(N::Pip), 1}}, .reward = {.experience = 15, .coins = 30}});
    q.push_back({.id = kQuestHopToIt, .giver = npc(N::Pip), .requiredQuests = {kQuestNewLeaf},
                 .objectives = {{O::Collect, dropItem(S::Sproutcap), 5}},
                 .reward = {.experience = 45, .coins = 60, .items = {{kRedPotion, 10}}}});
    q.push_back({.id = kQuestHettie, .giver = npc(N::Hettie), .requiredQuests = {kQuestNewLeaf},
                 .objectives = {{O::Defeat, mob(S::Dewdrop), 8}},
                 .reward = {.experience = 70, .coins = 150, .items = {{1100, 1}, {kBluePotion, 5}}}});
    q.push_back({.id = kQuestShellShock, .giver = npc(N::NanaRue), .minLevel = 4, .requiredQuests = {kQuestHopToIt, kQuestHettie},
                 .objectives = {{O::Defeat, mob(S::Swirlsnail), 8}},
                 .reward = {.experience = 140, .coins = 250, .items = {{1209, 1}}}});
    q.push_back({.id = kQuestAcrossWater, .giver = npc(N::NanaRue), .turnInTo = npc(N::Quill), .minLevel = 6,
                 .requiredQuests = {kQuestShellShock}, .objectives = {{O::Deliver, npc(N::Quill), 1}},
                 .reward = {.experience = 220, .coins = 300}});
    // Saltwhistle.
    q.push_back({.id = kQuestPinch, .giver = npc(N::Quill), .requiredQuests = {kQuestAcrossWater},
                 .objectives = {{O::Defeat, mob(S::Pinchy), 12}},
                 .reward = {.experience = 380, .coins = 400, .items = {{1201, 1}}}});
    q.push_back({.id = kQuestFeathers, .giver = npc(N::Finn), .minLevel = 8, .requiredQuests = {kQuestAcrossWater},
                 .objectives = {{O::Collect, dropItem(S::Puffgull), 10}},
                 .reward = {.experience = 450, .coins = 350, .items = {{1402, 1}, {kBluePotion, 10}}}});
    q.push_back({.id = kQuestPantry, .giver = npc(N::Rosa), .minLevel = 9, .requiredQuests = {kQuestAcrossWater},
                 .objectives = {{O::Collect, dropItem(S::BarrelHog), 8}},
                 .reward = {.experience = 520, .coins = 300, .items = {{kOrangePotion, 15}}}});
    q.push_back({.id = kQuestWarrior, .giver = npc(N::Kestrel), .minLevel = kJobLevel, .requiredQuests = {kQuestAcrossWater},
                 .requiredFlags = {kFlagBeginner}, .objectives = {{O::Defeat, mob(S::BarrelHog), 15}},
                 .reward = {.experience = 300, .coins = 200, .items = {{1010, 1}, {1202, 1}}}});
    q.push_back({.id = kQuestMage, .giver = npc(N::Wren), .minLevel = kJobLevel, .requiredQuests = {kQuestAcrossWater},
                 .requiredFlags = {kFlagBeginner}, .objectives = {{O::Defeat, mob(S::Puffgull), 15}},
                 .reward = {.experience = 300, .coins = 200, .items = {{1020, 1}, {1205, 1}}}});
    q.push_back({.id = kQuestDimming, .giver = npc(N::Quill), .minLevel = 11, .requiredQuests = {kQuestPinch},
                 .requiredFlags = {kFlagHasJob}, .objectives = {{O::Collect, kLighthouseLens, 1}},
                 .reward = {.experience = 800, .coins = 500, .items = {{kOrangePotion, 10}}}});
    q.push_back({.id = kQuestSeekChief, .giver = npc(N::Quill), .turnInTo = npc(N::Oakhart), .requiredQuests = {kQuestDimming},
                 .objectives = {{O::Talk, npc(N::Oakhart), 1}}, .reward = {.experience = 400, .coins = 200}});
    // The Fernwood.
    q.push_back({.id = kQuestBranches, .giver = npc(N::Oakhart), .requiredQuests = {kQuestSeekChief},
                 .objectives = {{O::Defeat, mob(S::Stumpling), 15}},
                 .reward = {.experience = 1100, .coins = 600, .items = {{1108, 1}}}});
    q.push_back({.id = kQuestSpores, .giver = npc(N::Lila), .minLevel = 14,
                 .objectives = {{O::Collect, dropItem(S::Glowcap), 10}},
                 .reward = {.experience = 1300, .coins = 400, .items = {{kHoneyElixir, 5}}}});
    q.push_back({.id = kQuestBramble, .giver = npc(N::Bramble), .minLevel = 16,
                 .objectives = {{O::Defeat, mob(S::Leafwisp), 15}},
                 .reward = {.experience = 1700, .coins = 700, .items = {{1404, 1}}}});
    q.push_back({.id = kQuestDelivery, .giver = npc(N::Dottie), .turnInTo = npc(N::Lila), .minLevel = 12,
                 .requiredQuests = {kQuestAcrossWater}, .objectives = {{O::Deliver, npc(N::Lila), 1}},
                 .reward = {.experience = 600, .coins = 500, .items = {{kVioletPotion, 10}}}});
    q.push_back({.id = kQuestGrove, .giver = npc(N::Oakhart), .minLevel = 17, .requiredQuests = {kQuestBranches},
                 .objectives = {{O::Defeat, boss(BossId::Thornbark), 1}},
                 .reward = {.experience = 3200, .coins = 1500, .items = {{kHoneyElixir, 5}}}});
    // Windcrest Pass.
    q.push_back({.id = kQuestUpThePass, .giver = npc(N::Oakhart), .turnInTo = npc(N::Tamsin), .requiredQuests = {kQuestGrove},
                 .objectives = {{O::Talk, npc(N::Tamsin), 1}}, .reward = {.experience = 800, .coins = 300}});
    q.push_back({.id = kQuestRations, .giver = npc(N::Tamsin), .minLevel = 20, .requiredQuests = {kQuestUpThePass},
                 .objectives = {{O::Collect, dropItem(S::Snowhog), 8}},
                 .reward = {.experience = 2600, .coins = 800, .items = {{kWhitePotion, 10}}}});
    q.push_back({.id = kQuestRockslide, .giver = npc(N::Tamsin), .minLevel = 22, .requiredQuests = {kQuestUpThePass},
                 .objectives = {{O::Defeat, mob(S::Pebblet), 18}},
                 .reward = {.experience = 3400, .coins = 900, .items = {{kHoneyElixir, 5}}}});
    q.push_back({.id = kQuestSummit, .giver = npc(N::Tamsin), .minLevel = 25, .requiredQuests = {kQuestUpThePass},
                 .objectives = {{O::Defeat, boss(BossId::Frostmane), 1}},
                 .reward = {.experience = 7000, .coins = 2500, .items = {{kWhitePotion, 10}}}});
    // The Undercroft.
    q.push_back({.id = kQuestBeneath, .giver = npc(N::Tamsin), .turnInTo = npc(N::Quill), .requiredQuests = {kQuestSummit},
                 .objectives = {{O::Talk, npc(N::Quill), 1}}, .reward = {.experience = 1500, .coins = 500}});
    q.push_back({.id = kQuestUndercroft, .giver = npc(N::Quill), .turnInTo = npc(N::Nix), .requiredQuests = {kQuestBeneath},
                 .objectives = {{O::Talk, npc(N::Nix), 1}}, .reward = {.experience = 1500, .coins = 500}});
    q.push_back({.id = kQuestGlimmer, .giver = npc(N::Nix), .requiredQuests = {kQuestUndercroft},
                 .objectives = {{O::Defeat, mob(S::CaveFlitter), 15}},
                 .reward = {.experience = 6500, .coins = 1500, .items = {{kHoneyElixir, 8}}}});
    q.push_back({.id = kQuestRunes, .giver = npc(N::Nix), .minLevel = 29, .requiredQuests = {kQuestUndercroft},
                 .objectives = {{O::Collect, dropItem(S::Runeshell), 10}},
                 .reward = {.experience = 8000, .coins = 2000, .items = {{kWhitePotion, 15}}}});
    q.push_back({.id = kQuestHeart, .giver = npc(N::Nix), .turnInTo = npc(N::Quill), .minLevel = 30,
                 .requiredQuests = {kQuestGlimmer}, .objectives = {{O::Defeat, boss(BossId::Murkmaw), 1}},
                 .reward = {.experience = 20000, .coins = 10000, .items = {{kHoneyElixir, 20}}}});
    return q;
}

const std::vector<QuestDef>& quests()
{
    static const std::vector<QuestDef> q = buildQuests();
    return q;
}

const std::array<QuestText, 26>& texts()
{
    static const std::array<QuestText, 26> t{{
        {kQuestNewLeaf, "A New Leaf", "Say hello to Pip by the swing tree in Petalbrook.", true},
        {kQuestHopToIt, "Hop to It", "Bring Pip 5 Sprout Caps from the Sproutcaps in Sprout Meadow.", true},
        {kQuestHettie, "Hettie's Errand", "Defeat 8 Dewdrops on Sprout Meadow or Puddle Path for Hettie.", true},
        {kQuestShellShock, "Shell Shock", "Defeat 8 Swirlsnails on Puddle Path or Breezy Bluff for Nana Rue.", true},
        {kQuestAcrossWater, "Across the Water", "Sail from Petal Pier and give Nana's letter to Harbormaster Quill in Saltwhistle.", true},
        {kQuestPinch, "A Pinch of Trouble", "Chase off 12 Pinchies on Gull Cliffs for Quill.", true},
        {kQuestFeathers, "Feathers for Finn", "Collect 10 Puff Feathers from the Puffgulls for Finn.", false},
        {kQuestPantry, "Rosa's Pantry", "Bring Rosa 8 Hog Bristles from the Barrel Hogs.", false},
        {kQuestWarrior, "The Warrior's Path", "Defeat 15 Barrel Hogs to prove your strength to Kestrel.", true},
        {kQuestMage, "The Mage's Path", "Defeat 15 Puffgulls to prove your focus to Wren.", true},
        {kQuestDimming, "The Dimming Light", "Find the old Lighthouse Lens on Lighthouse Hill for Quill.", true},
        {kQuestSeekChief, "Seek the Chief", "Travel through the Fernwood to Mossbough and talk to Chief Oakhart.", true},
        {kQuestBranches, "Branches and Bridges", "Calm 15 Stumplings that crowd Mossbough's roads.", true},
        {kQuestSpores, "Glowing Spores", "Collect 10 Glow Spores from the Glowcaps for Lila.", false},
        {kQuestBramble, "Bramble's Hunt", "Defeat 15 Leafwisps for Bramble.", false},
        {kQuestDelivery, "Special Delivery", "Carry Dottie's parcel of spices to Lila in Mossbough.", false},
        {kQuestGrove, "The Grove's Heart", "Calm Thornbark in his grove beyond Glowcap Hollow.", true},
        {kQuestUpThePass, "Up the Pass", "Climb from Mossbough to Windcrest Pass and find Tamsin's camp.", true},
        {kQuestRations, "Cold Rations", "Bring Tamsin 8 Snow Tufts from the Snowhogs.", false},
        {kQuestRockslide, "Rockslide", "Clear 18 Pebblets off the mountain paths.", false},
        {kQuestSummit, "The Summit Shard", "Face Frostmane at the Summit Shrine.", true},
        {kQuestBeneath, "Beneath Saltwhistle", "Bring both shards back to Quill in Saltwhistle.", true},
        {kQuestUndercroft, "Into the Undercroft", "Go down the old door in the Market and find Nix.", true},
        {kQuestGlimmer, "Glimmer and Gloom", "Defeat 15 Cave Flitters in the Undercroft.", true},
        {kQuestRunes, "Old Runes", "Collect 10 Rune Chips from the Runeshells for Nix.", false},
        {kQuestHeart, "The Heart of the Gloom", "Defeat Murkmaw in the Heart of the Gloom, then return to Quill.", true},
    }};
    return t;
}

gamelib::core::DialogueTree buildTree()
{
    using gamelib::core::addNpcTalk;
    gamelib::core::DialogueTree tree;
    const auto shop = gamelib::core::DialogueChoice{.label = "Let me see your goods.", .effects = {"shop"}};

    addNpcTalk(tree, {.id = "nana", .speaker = std::string{npcName(NpcId::NanaRue)},
                      .handIns = {{kQuestShellShock, "Not a snail left on the path! You have grown strong, dear. "
                                                     "Here, Hettie sewed this vest for you."}},
                      .offers = {{kQuestNewLeaf, "Oh! A new face on Petal Isle - and just in time. Welcome to Petalbrook, dear! "
                                                 "Before anything else, go and say hello to Pip by the swing tree. "
                                                 "He has been waiting all week for someone new.",
                                  "Off you go. Walk with the arrow keys, and press {up} to talk to people.",
                                  "Pip is by the big tree in the middle of the village."},
                                 {kQuestShellShock, "The Swirlsnails have been nibbling the path to the pier again. "
                                                    "Could you shoo eight of them away? They live on Puddle Path and Breezy Bluff.",
                                  "Thank you. Mind their shells - they are harder than they look!",
                                  "Eight Swirlsnails, dear. Puddle Path is east of the village."},
                                 {kQuestAcrossWater, "Our little isle has taught you all it can. Across the water lies Saltwhistle, "
                                                     "a busy harbor town. Its beacon has been dimming lately, and the harbormaster "
                                                     "is an old friend. Would you take him this letter?",
                                  "Captain Marlo's ship sails from Petal Pier, past Breezy Bluff. Safe travels, dear one.",
                                  "Petal Pier is past Puddle Path and Breezy Bluff. Captain Marlo will take you across."}},
                      .lines = {{"night", "The stars are lovely tonight. Don't stay out too late!"},
                                {"done 5", "Write to me from Saltwhistle, won't you?"},
                                {"", "Petalbrook is small, but we look after each other here."}}});

    addNpcTalk(tree, {.id = "pip", .speaker = std::string{npcName(NpcId::Pip)},
                      .handIns = {{kQuestNewLeaf, "Hi! Hi hi hi! You're the new adventurer? Nana said you'd come! I'm Pip!"},
                                  {kQuestHopToIt, "Five caps! You're amazing! Okay, okay - here's the secret. Jump, and while "
                                                  "you're still in the air, jump AGAIN! Whoosh! Hold up to fly straight up. "
                                                  "That's the Hyper Jump! (Open your skills with {skills} to make it stronger.)"}},
                      .offers = {{kQuestHopToIt, "Wanna learn something super cool? Bring me five Sprout Caps from the "
                                                 "Sproutcaps in Sprout Meadow, west of the village! Press {attack} to bonk them, "
                                                 "and press {pickup} to pick up what they drop!",
                                  "Yay! Sprout Meadow is that way! West! Through the swirly portal - press {up} in it!",
                                  "Five Sprout Caps! The Sproutcaps live in Sprout Meadow, west of here."}},
                      .lines = {{"night", "I'm not sleepy! ...okay, maybe a little."},
                                {"", "When I grow up I'm gonna sail with Captain Marlo!"}}});

    addNpcTalk(tree, {.id = "hettie", .speaker = std::string{npcName(NpcId::Hettie)},
                      .handIns = {{kQuestHettie, "My laundry line is safe again! Thank you, sweetie. Take this beret - and "
                                                 "some potions. Press {inventory} to look in your bag, {equipment} for what you wear."}},
                      .offers = {{kQuestHettie, "Those wobbly Dewdrops keep bouncing onto my laundry line! Could you "
                                                "chase off eight of them? Sprout Meadow and Puddle Path are full of them.",
                                  "Bless you. Come back for potions any time - I've the best on the isle!",
                                  "Eight Dewdrops, sweetie. They bounce about on Sprout Meadow and Puddle Path."}},
                      .lines = {{"", "Red potions for your health, blue for your magic. Always keep a few!"}},
                      .services = {shop}});

    addNpcTalk(tree, {.id = "marlo", .speaker = std::string{npcName(NpcId::Marlo)},
                      .lines = {{"", "Ahoy! The Breezy Gull sails between Petal Pier and Saltwhistle whenever you like. "
                                     "Ten coins for the crossing."}},
                      .services = {{.label = "Set sail! (10 coins)", .effects = {"sail"}}},
                      .menuText = "Where to, sailor?",
                      .goodbyeLabel = "Not today."});

    addNpcTalk(tree, {.id = "quill", .speaker = std::string{npcName(NpcId::Quill)},
                      .handIns = {{kQuestAcrossWater, "A letter from Rue! She writes that you are kind and brave. Good, because "
                                                      "Saltwhistle needs both. Welcome to the harbor."},
                                  {kQuestPinch, "The docks are quiet again. Have this blouse - every sailor needs one."},
                                  {kQuestDimming, "The old lens! Look - see these marks? It was carved for three shards of "
                                                  "light. Two are lost somewhere in the wild lands, and without them the beacon "
                                                  "fades. Chief Oakhart of Mossbough may know more."},
                                  {kQuestBeneath, "The Shard of Dawn... and the Shard of Frost! They hum together. And listen - "
                                                  "the ground hums back. The third shard is below us, in the Undercroft. The old "
                                                  "door in the Market is open now."},
                                  {kQuestHeart, "The Heart of the Beacon! You did it. Look at the lighthouse - it shines over "
                                                "the whole sea again. Saltwhistle will never forget you."}},
                      .offers = {{kQuestPinch, "Before anything else: the Pinchies on Gull Cliffs keep crawling onto the docks "
                                               "and snipping our ropes. Could you chase off twelve of them?",
                                  "Gull Cliffs is west of the Docks. Mind the claws!",
                                  "Twelve Pinchies, on Gull Cliffs to the west."},
                                 {kQuestDimming, "Our beacon is dimming, and nobody knows why. The keepers of old left a lens "
                                                 "on Lighthouse Hill. The Barrel Hogs there may have rolled off with it. Find it?",
                                  "Lighthouse Hill is past Tidepool Shore, the far west of the coast.",
                                  "The lens is somewhere on Lighthouse Hill. The Barrel Hogs there love shiny things."},
                                 {kQuestUndercroft, "Below the Market lies the Undercroft, the old cellars of the first "
                                                    "harbor. An explorer called Nix went down weeks ago. Find her, and the third shard.",
                                  "The door is in the Market, past Kestrel's yard. Press up at it.",
                                  "Nix is camped at the bottom of the Undercroft Stairs."}},
                      .lines = {{"done 26", "The beacon's light reaches all the way to Petal Isle now. Rue will see it."},
                                {"", "Ships come and go, but the beacon must always shine."}}});

    addNpcTalk(tree, {.id = "finn", .speaker = std::string{npcName(NpcId::Finn)},
                      .handIns = {{kQuestFeathers, "Fluffy! These'll stuff the softest pillow at sea. Here - sandals. "
                                                   "You'll run faster in them, I promise."}},
                      .offers = {{kQuestFeathers, "I'm stuffing a pillow for the long voyages. Got ten Puff Feathers for me? "
                                                  "Puffgulls drop them on Gull Cliffs and Lighthouse Hill.",
                                  "Thanks, mate!", "Ten Puff Feathers, mate. The Puffgulls fly over the cliffs."}},
                      .lines = {{"", "Smell that salt? Nothing like it."}}});

    addNpcTalk(tree, {.id = "rosa", .speaker = std::string{npcName(NpcId::Rosa)},
                      .handIns = {{kQuestPantry, "Oh, these will make a lovely stew. Take some potions for the road, dear."}},
                      .offers = {{kQuestPantry, "My pantry is empty and the stew pot is cold! Bring me eight Hog Bristles "
                                                "from the Barrel Hogs on Tidepool Shore?",
                                  "You're a treasure.", "Eight Hog Bristles, dear. Barrel Hogs roll about on Tidepool Shore."}},
                      .lines = {{"night", "A bed by the fire for a weary traveler?"},
                                {"", "Welcome to the Sleepy Gull! Rest here any time."}},
                      .services = {{.label = "Rest at the inn (20 coins)", .effects = {"rest"}}}});

    addNpcTalk(tree, {.id = "dottie", .speaker = std::string{npcName(NpcId::Dottie)},
                      .handIns = {},
                      .offers = {{kQuestDelivery, "Could you carry this parcel of spices to Lila in Mossbough? It's a long "
                                                  "walk through the Fernwood for my old legs.",
                                  "Lila's hut is on the lowest branches of Mossbough.",
                                  "Lila is in Mossbough, through Fernwood Trail east of the Market."}},
                      .lines = {{"", "Potions, scrolls, everything a traveler needs!"}},
                      .services = {shop}});

    addNpcTalk(tree, {.id = "garrick", .speaker = std::string{npcName(NpcId::Garrick)},
                      .lines = {{"job beginner", "Choose a path first, kid. Then come back for real steel."},
                                {"", "Every piece here was hammered on that anvil. Take a look."}},
                      .services = {{.label = "Show me your gear.", .effects = {"gearshop"}}}});

    addNpcTalk(tree, {.id = "kestrel", .speaker = std::string{npcName(NpcId::Kestrel)},
                      .handIns = {{kQuestWarrior, "Fifteen hogs and not a scratch. Kneel... there. From this day you are a "
                                                  "WARRIOR. Take this sword and tunic. Open your skills with {skills} - you have "
                                                  "new ones to learn."}},
                      .offers = {{kQuestWarrior, "So, you want to be a warrior? Strength is earned, not given. Defeat fifteen "
                                                 "Barrel Hogs on Tidepool Shore, and I'll take you as my student.",
                                  "Good. Hit hard, stand firm.", "Fifteen Barrel Hogs. Tidepool Shore, west of the cliffs."}},
                      .lines = {{"job warrior", "Keep your guard up and your sword sharp, warrior."},
                                {"job mage", "A mage, eh? Wren chose well. Don't let me catch you skipping leg day."},
                                {"", "Warriors wear heavy armor and strike hard and close. Come back at level 10."}}});

    addNpcTalk(tree, {.id = "wren", .speaker = std::string{npcName(NpcId::Wren)},
                      .handIns = {{kQuestMage, "Your focus is true. Close your eyes... and open them as a MAGE. This staff "
                                               "and robe are yours. Press {skills} - your spells are waiting."}},
                      .offers = {{kQuestMage, "Magic is patience and focus. Show me yours: defeat fifteen Puffgulls on Gull "
                                              "Cliffs, without losing heart.",
                                  "The wind will guide you.", "Fifteen Puffgulls. They glide over Gull Cliffs."}},
                      .lines = {{"job mage", "Every spell starts with a breath. Mind your MP."},
                                {"job warrior", "A warrior. Kestrel will be pleased. Do visit my library."},
                                {"", "Mages bend starlight and wind. Come back at level 10 if that calls to you."}}});

    addNpcTalk(tree, {.id = "oakhart", .speaker = std::string{npcName(NpcId::Oakhart)},
                      .handIns = {{kQuestSeekChief, "Quill sent you? Then the beacon fades. The Shard of Dawn was kept in our "
                                                    "grove, by the oldest tree of all: Thornbark. But Thornbark has woken angry."},
                                  {kQuestBranches, "The roads are clear. Mossbough owes you. Take this cap - it suits a traveler."},
                                  {kQuestGrove, "Thornbark sleeps peacefully again, and you carry the Shard of Dawn. The second "
                                                "shard was taken up Windcrest Pass long ago. Our scout Tamsin camps up there."}},
                      .offers = {{kQuestBranches, "The Stumplings have taken over the roads around Mossbough. Calm fifteen of "
                                                  "them, and I'll trust you with the grove.",
                                  "Thank you, traveler.", "Fifteen Stumplings, on Fernwood Trail and around the village."},
                                 {kQuestGrove, "Beyond Glowcap Hollow is Thornbark's Grove. Calm the old tree, and bring back "
                                               "the Shard of Dawn. Be ready - he is strong.",
                                  "The forest is with you.", "Thornbark's Grove is beyond Glowcap Hollow, past Canopy Walk."},
                                 {kQuestUpThePass, "Climb the path from the top branches of Mossbough to Windcrest Pass, and "
                                                   "find Tamsin. She will help you find the second shard.",
                                  "Dress warmly!", "The path up the mountain starts from Mossbough's highest branches."}},
                      .lines = {{"", "Mossbough has stood in these branches for three hundred years."}}});

    addNpcTalk(tree, {.id = "lila", .speaker = std::string{npcName(NpcId::Lila)},
                      .handIns = {{kQuestDelivery, "Dottie's spices! My herb tea will finally taste right. Thank you!"},
                                  {kQuestSpores, "Ooh, they're still glowing. Here, my honey elixirs - the best in the Fernwood."}},
                      .offers = {{kQuestSpores, "I brew my potions with Glow Spores. Could you bring me ten? The Glowcaps grow "
                                                "on Canopy Walk and in Glowcap Hollow.",
                                  "Wonderful!", "Ten Glow Spores, please."}},
                      .lines = {{"", "Herbs, honey and potions, fresh from the branches."}},
                      .services = {shop}});

    addNpcTalk(tree, {.id = "bramble", .speaker = std::string{npcName(NpcId::Bramble)},
                      .handIns = {{kQuestBramble, "Quiet woods again. These boots were my father's - they'll carry you far."}},
                      .offers = {{kQuestBramble, "The Leafwisps get bold at dusk. Thin them out for me - fifteen should do.",
                                  "Good hunting.", "Fifteen Leafwisps, on Canopy Walk and in the Hollow."}},
                      .lines = {{"", "Listen to the trees. They warn you before trouble comes."}}});

    addNpcTalk(tree, {.id = "tamsin", .speaker = std::string{npcName(NpcId::Tamsin)},
                      .handIns = {{kQuestUpThePass, "Oakhart's messenger! Warm your hands by the fire. So, you seek the Shard of Frost..."},
                                  {kQuestRations, "Real food at last. Here, take some potions."},
                                  {kQuestRockslide, "The paths are clear! You climb like a mountain goat."},
                                  {kQuestSummit, "Frostmane is calm, and the Shard of Frost is yours. Take both shards to Quill."}},
                      .offers = {{kQuestRations, "Rations are running low up here. Bring me eight Snow Tufts from the Snowhogs?",
                                  "Thanks!", "Eight Snow Tufts. Snowhogs charge about on the foothills and the ledge."},
                                 {kQuestRockslide, "Pebblets keep rolling onto the paths. Could you clear eighteen?",
                                  "Watch your step.", "Eighteen Pebblets, on the climb and the foothills."},
                                 {kQuestSummit, "The Shard of Frost lies in the Summit Shrine, guarded by Frostmane. Up Cliffside "
                                                "Climb and across Frostbite Ledge. Ready?",
                                  "May the wind be at your back.", "The Summit Shrine is past Frostbite Ledge."},
                                 {kQuestBeneath, "Take both shards back to Quill in Saltwhistle. He'll know what to do.",
                                  "Safe journey down!", "Quill is at the Saltwhistle Docks."}},
                      .lines = {{"", "Cold up here, isn't it? Stay by the fire a while."}},
                      .services = {shop}});

    addNpcTalk(tree, {.id = "nix", .speaker = std::string{npcName(NpcId::Nix)},
                      .handIns = {{kQuestUndercroft, "A visitor! Quill sent you? Good timing. Something down here is drinking the light."},
                                  {kQuestGlimmer, "The tunnels are safer now. Here, take these elixirs."},
                                  {kQuestRunes, "These runes... they tell of the Heart of the Beacon, and of the gloom that swallowed it."}},
                      .offers = {{kQuestGlimmer, "Cave Flitters swarm the tunnels and snuff out my lanterns. Clear fifteen?",
                                  "Thanks, friend.", "Fifteen Cave Flitters, in the Glimmer Tunnels and beyond."},
                                 {kQuestRunes, "The Runeshells carry chips of old carved stone. Bring me ten - I want to read them.",
                                  "Careful!", "Ten Rune Chips from the Runeshells."},
                                 {kQuestHeart, "It's Murkmaw - a gloom that grew around the Heart of the Beacon, deep below. "
                                               "Defeat it, and the light comes home. Then tell Quill.",
                                  "Go. I'll keep a lantern lit for you.", "Murkmaw waits in the Heart of the Gloom, past the Deep Hollow."}},
                      .lines = {{"done 26", "The Undercroft feels warm now. Who knew?"},
                                {"", "Mind the dark. Bring potions."}}});

    return tree;
}

} // namespace

std::string_view npcName(const NpcId id)
{
    switch (id) {
    case NpcId::NanaRue: return "Nana Rue";
    case NpcId::Pip: return "Pip";
    case NpcId::Hettie: return "Hettie";
    case NpcId::Marlo: return "Captain Marlo";
    case NpcId::Quill: return "Quill";
    case NpcId::Finn: return "Finn";
    case NpcId::Rosa: return "Rosa";
    case NpcId::Dottie: return "Dottie";
    case NpcId::Garrick: return "Garrick";
    case NpcId::Kestrel: return "Kestrel";
    case NpcId::Wren: return "Wren";
    case NpcId::Oakhart: return "Chief Oakhart";
    case NpcId::Lila: return "Lila";
    case NpcId::Bramble: return "Bramble";
    case NpcId::Tamsin: return "Tamsin";
    case NpcId::Nix: return "Nix";
    case NpcId::Count: break;
    }
    return "";
}

std::string_view npcRole(const NpcId id)
{
    switch (id) {
    case NpcId::NanaRue: return "elder of petalbrook";
    case NpcId::Pip: return "village kid";
    case NpcId::Hettie: return "island goods";
    case NpcId::Marlo: return "captain of the breezy gull";
    case NpcId::Quill: return "harbormaster";
    case NpcId::Finn: return "sailor";
    case NpcId::Rosa: return "innkeeper";
    case NpcId::Dottie: return "general store";
    case NpcId::Garrick: return "armorsmith";
    case NpcId::Kestrel: return "warrior instructor";
    case NpcId::Wren: return "mage instructor";
    case NpcId::Oakhart: return "chief of mossbough";
    case NpcId::Lila: return "herbalist";
    case NpcId::Bramble: return "ranger";
    case NpcId::Tamsin: return "mountain scout";
    case NpcId::Nix: return "explorer";
    case NpcId::Count: break;
    }
    return "";
}

std::string npcRoot(const NpcId id)
{
    static constexpr std::array<std::string_view, kNpcCount> kRoots{"nana", "pip", "hettie", "marlo", "quill", "finn", "rosa", "dottie",
                                                                    "garrick", "kestrel", "wren", "oakhart", "lila", "bramble", "tamsin", "nix"};
    return std::string{kRoots[static_cast<std::size_t>(id)]};
}

std::span<const QuestDef> questDefs()
{
    return quests();
}

const QuestText& questText(const int id)
{
    static const QuestText none{};
    for (const QuestText& t : texts()) {
        if (t.id == id) {
            return t;
        }
    }
    return none;
}

const gamelib::core::DialogueTree& dialogueTree()
{
    static const gamelib::core::DialogueTree tree = buildTree();
    return tree;
}

int bossQuest(const BossId id)
{
    switch (id) {
    case BossId::Thornbark: return kQuestGrove;
    case BossId::Frostmane: return kQuestSummit;
    case BossId::Murkmaw: return kQuestHeart;
    case BossId::Count: break;
    }
    return 0;
}

int chapterOf(const gamelib::core::QuestBook& book)
{
    using S = gamelib::core::QuestState;
    if (book.stateOf(kQuestUndercroft) == S::TurnedIn || book.stateOf(kQuestBeneath) == S::TurnedIn) {
        return 5;
    }
    if (book.stateOf(kQuestGrove) == S::TurnedIn) {
        return 4;
    }
    if (book.stateOf(kQuestSeekChief) == S::TurnedIn || book.stateOf(kQuestDimming) == S::TurnedIn) {
        return 3;
    }
    if (book.stateOf(kQuestAcrossWater) == S::TurnedIn) {
        return 2;
    }
    return 1;
}

} // namespace realm::game
