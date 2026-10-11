#include "items.hpp"

#include <gamelib/core/weighted.hpp>

#include <algorithm>
#include <array>
#include <map>
#include <string>
#include <vector>

namespace realm::game {

namespace {

// helpers

using gamelib::core::ItemDef;
using gamelib::core::ItemStack;
using gamelib::core::ItemStat;

// The avatar's kinds (avatar.frag, anime_look.hpp).
namespace hat {
constexpr int Beret = 1, Wizard = 2, Bandana = 3, Helmet = 4, Feather = 5, Flower = 6, Hood = 7;
}
namespace top {
constexpr int Tee = 0, Tunic = 1, Robe = 2, Plate = 3, Coat = 4, Vest = 6, Sailor = 7;
}
namespace bottom {
constexpr int Pants = 0, Shorts = 1, Skirt = 2, Greaves = 3;
}
namespace shoes {
constexpr int Boots = 0, Shoes = 1, Sabatons = 2, Sandals = 3;
}
namespace gloves {
constexpr int Cloth = 1, Gauntlets = 2;
}
namespace weapon {
constexpr int Branch = 1, Sword = 2, Axe = 3, Staff = 4, Wand = 5, Broadsword = 6;
}
// kAnimePalette indices.
enum Sw : int { Cream, Plum, Red, Orange, Yellow, Leaf, Forest, Sky, Blue, Navy, Purple, Pink, Brown, Tan, Slate, Gold };

constexpr std::uint32_t kW = jobBit(Job::Warrior);
constexpr std::uint32_t kM = jobBit(Job::Mage);

struct Equip {
    int id;
    std::string_view name;
    Slot slot;
    int level;
    std::uint32_t jobs;
    std::vector<ItemStat> stats;
    ItemLook look;
    std::string_view text;
};

ItemStat st(const Bonus b, const int v)
{
    return {.stat = static_cast<int>(b), .value = v};
}

std::vector<Equip> equipment()
{
    using B = Bonus;
    using S = Slot;
    return {
        // Weapons.
        {1000, "Willow Branch", S::Weapon, 1, kAnyJob, {st(B::Attack, 8)}, {weapon::Branch, 0, Leaf}, "A springy branch. Better than nothing!"},
        {1001, "Training Sword", S::Weapon, 5, kAnyJob, {st(B::Attack, 16)}, {weapon::Sword, 0, Sky}, "A blunt blade from the island's dojo."},
        {1002, "Oak Wand", S::Weapon, 5, kAnyJob, {st(B::Attack, 6), st(B::Magic, 14)}, {weapon::Wand, 0, Yellow}, "A twig that hums when you hold it."},
        {1010, "Iron Sword", S::Weapon, 10, kW, {st(B::Attack, 28)}, {weapon::Sword, 0, Sky}, "A warrior's first real sword."},
        {1011, "Hand Axe", S::Weapon, 12, kW, {st(B::Attack, 33), st(B::Speed, -2)}, {weapon::Axe, 0, Orange}, "Heavy, honest, splits crates and crabs."},
        {1012, "Steel Sword", S::Weapon, 18, kW, {st(B::Attack, 42)}, {weapon::Sword, 0, Sky}, "Forged in Saltwhistle by Garrick."},
        {1013, "Woodsman Axe", S::Weapon, 20, kW, {st(B::Attack, 47), st(B::Str, 2)}, {weapon::Axe, 0, Leaf}, "Fernwood's pride, light as a feather."},
        {1014, "Knight's Broadsword", S::Weapon, 25, kW, {st(B::Attack, 58)}, {weapon::Broadsword, 0, Gold}, "Two hands, one mighty arc."},
        {1015, "Frostbrand", S::Weapon, 30, kW, {st(B::Attack, 72), st(B::Crit, 3)}, {weapon::Broadsword, 0, Sky}, "Cold to the touch, never dull."},
        {1016, "Dawnblade", S::Weapon, 35, kW, {st(B::Attack, 90), st(B::Str, 4)}, {weapon::Sword, 0, Gold}, "It glows like the first light of morning."},
        {1020, "Apprentice Staff", S::Weapon, 10, kM, {st(B::Magic, 30)}, {weapon::Staff, 0, Sky}, "Every mage of Saltwhistle starts with one."},
        {1021, "Star Wand", S::Weapon, 12, kM, {st(B::Magic, 33), st(B::Speed, 2)}, {weapon::Wand, 0, Yellow}, "A wand tipped with a sleepy star."},
        {1022, "Coral Staff", S::Weapon, 18, kM, {st(B::Magic, 45)}, {weapon::Staff, 0, Pink}, "Grown, not carved, in the tide pools."},
        {1023, "Moon Wand", S::Weapon, 20, kM, {st(B::Magic, 49), st(B::Int, 2)}, {weapon::Wand, 0, Purple}, "Brighter at night."},
        {1024, "Sage Staff", S::Weapon, 25, kM, {st(B::Magic, 61)}, {weapon::Staff, 0, Leaf}, "Mossbough's old sages leaned on these."},
        {1025, "Frost Scepter", S::Weapon, 30, kM, {st(B::Magic, 76), st(B::Crit, 3)}, {weapon::Wand, 0, Sky}, "Snowflakes swirl around its star."},
        {1026, "Dawn Staff", S::Weapon, 35, kM, {st(B::Magic, 94), st(B::Int, 4)}, {weapon::Staff, 0, Gold}, "Holds a sliver of sunrise."},
        // Hats.
        {1100, "Leather Beret", S::Hat, 3, kAnyJob, {st(B::Defense, 3)}, {hat::Beret, 0, Brown}, "Soft, comfy, a little floppy."},
        {1101, "Flower Crown", S::Hat, 5, kAnyJob, {st(B::Defense, 2), st(B::Int, 1), st(B::Agi, 1)}, {hat::Flower, 0, Pink}, "Petals from Petal Isle's meadow."},
        {1102, "Red Bandana", S::Hat, 10, kW, {st(B::Defense, 6), st(B::Str, 1)}, {hat::Bandana, 0, Red}, "Tied tight before a fight."},
        {1103, "Iron Helmet", S::Hat, 18, kW, {st(B::Defense, 14)}, {hat::Helmet, 0, Slate}, "Dents easily, but saves heads."},
        {1104, "Knight Helm", S::Hat, 28, kW, {st(B::Defense, 24), st(B::Hp, 40)}, {hat::Helmet, 0, Gold}, "Topped with a proud plume."},
        {1105, "Pointed Hat", S::Hat, 10, kM, {st(B::Defense, 4), st(B::Int, 2)}, {hat::Wizard, 0, Blue}, "Every mage needs a pointy hat."},
        {1106, "Star Hat", S::Hat, 18, kM, {st(B::Defense, 8), st(B::Int, 3), st(B::Mp, 30)}, {hat::Wizard, 0, Purple}, "The star on top twinkles."},
        {1107, "Sage Hood", S::Hat, 28, kM, {st(B::Defense, 13), st(B::Int, 5)}, {hat::Hood, 0, Forest}, "Warm and wise."},
        {1108, "Traveler's Cap", S::Hat, 15, kAnyJob, {st(B::Defense, 8), st(B::Speed, 3)}, {hat::Feather, 0, Navy}, "With a gull feather from the cliffs."},
        // Tops.
        {1200, "Plain Tee", S::Top, 1, kAnyJob, {st(B::Defense, 2)}, {top::Tee, 0xF7F3E8, Orange}, "Fresh from the island laundry."},
        {1201, "Sailor Blouse", S::Top, 5, kAnyJob, {st(B::Defense, 5)}, {top::Sailor, 0xF8F8FF, Red}, "A sea breeze in cloth form."},
        {1202, "Leather Tunic", S::Top, 10, kW, {st(B::Defense, 10), st(B::Hp, 20)}, {top::Tunic, 0x9C6A44, Gold}, "Stitched for brawling."},
        {1203, "Iron Plate", S::Top, 18, kW, {st(B::Defense, 20), st(B::Hp, 40)}, {top::Plate, 0xC8D4E8, Gold}, "Clanks a little when you run."},
        {1204, "Knight Plate", S::Top, 28, kW, {st(B::Defense, 32), st(B::Hp, 80)}, {top::Plate, 0xF0D890, Red}, "The knights of old wore these."},
        {1205, "Apprentice Robe", S::Top, 10, kM, {st(B::Defense, 6), st(B::Mp, 25)}, {top::Robe, 0x5E8AE8, Sky}, "Wide sleeves for hiding notes."},
        {1206, "Star Robe", S::Top, 18, kM, {st(B::Defense, 12), st(B::Mp, 45), st(B::Int, 2)}, {top::Robe, 0x7E62D8, Sky}, "Embroidered with tiny stars."},
        {1207, "Sage Robe", S::Top, 28, kM, {st(B::Defense, 20), st(B::Mp, 80), st(B::Int, 4)}, {top::Robe, 0x4E9A6A, Gold}, "Smells of old books and pine."},
        {1208, "Traveler Coat", S::Top, 15, kAnyJob, {st(B::Defense, 12), st(B::Agi, 2)}, {top::Coat, 0xE08A4A, Gold}, "Many pockets, all of them full."},
        {1209, "Island Vest", S::Top, 5, kAnyJob, {st(B::Defense, 4), st(B::Agi, 1)}, {top::Vest, 0x6AB8E0, Gold}, "Hettie sews them herself."},
        // Bottoms.
        {1300, "Cotton Shorts", S::Bottom, 1, kAnyJob, {st(B::Defense, 1)}, {bottom::Shorts, 0, Blue}, "Good for splashing in puddles."},
        {1301, "Ruffled Skirt", S::Bottom, 5, kAnyJob, {st(B::Defense, 3), st(B::Agi, 1)}, {bottom::Skirt, 0, Pink}, "It twirls when you jump."},
        {1302, "Work Pants", S::Bottom, 5, kAnyJob, {st(B::Defense, 4)}, {bottom::Pants, 0, Brown}, "Patched at both knees."},
        {1303, "Iron Greaves", S::Bottom, 18, kW, {st(B::Defense, 12)}, {bottom::Greaves, 0, Slate}, "Heavy, but your shins thank you."},
        {1304, "Silk Skirt", S::Bottom, 12, kM, {st(B::Defense, 5), st(B::Mp, 20)}, {bottom::Skirt, 0, Navy}, "Light as a cloud."},
        {1305, "Knight Greaves", S::Bottom, 28, kW, {st(B::Defense, 20)}, {bottom::Greaves, 0, Gold}, "Polished every morning."},
        {1306, "Sage Trousers", S::Bottom, 25, kM, {st(B::Defense, 11), st(B::Int, 3)}, {bottom::Pants, 0, Forest}, "Comfortable for long journeys."},
        // Shoes.
        {1400, "Red Sneakers", S::Shoes, 1, kAnyJob, {st(B::Defense, 1)}, {shoes::Shoes, 0, Red}, "Squeak a little on wood."},
        {1401, "Leather Boots", S::Shoes, 5, kAnyJob, {st(B::Defense, 3)}, {shoes::Boots, 0, Brown}, "Broken in by someone else."},
        {1402, "Beach Sandals", S::Shoes, 8, kAnyJob, {st(B::Defense, 1), st(B::Speed, 4)}, {shoes::Sandals, 0, Orange}, "Flip, flop, faster."},
        {1403, "Iron Sabatons", S::Shoes, 18, kW, {st(B::Defense, 9)}, {shoes::Sabatons, 0, Slate}, "Thunk. Thunk. Thunk."},
        {1404, "Winged Boots", S::Shoes, 20, kAnyJob, {st(B::Defense, 5), st(B::Speed, 6), st(B::Jump, 5)}, {shoes::Boots, 0, Sky}, "Tiny wings flutter at the heel."},
        {1405, "Knight Sabatons", S::Shoes, 28, kW, {st(B::Defense, 15)}, {shoes::Sabatons, 0, Gold}, "March in style."},
        {1406, "Star Slippers", S::Shoes, 18, kM, {st(B::Defense, 5), st(B::Mp, 30)}, {shoes::Shoes, 0, Purple}, "Soft as moonlight."},
        // Gloves.
        {1500, "Work Gloves", S::Gloves, 5, kAnyJob, {st(B::Defense, 2), st(B::Attack, 1)}, {gloves::Cloth, 0, Tan}, "For honest work and dishonest crabs."},
        {1501, "Iron Gauntlets", S::Gloves, 18, kW, {st(B::Defense, 6), st(B::Attack, 3)}, {gloves::Gauntlets, 0, Slate}, "Punch with confidence."},
        {1502, "Silk Gloves", S::Gloves, 15, kM, {st(B::Defense, 3), st(B::Magic, 4)}, {gloves::Cloth, 0, Cream}, "A mage's fingers stay nimble."},
        {1503, "Knight Gauntlets", S::Gloves, 28, kW, {st(B::Defense, 10), st(B::Attack, 6)}, {gloves::Gauntlets, 0, Gold}, "Gleaming, gold-trimmed."},
    };
}

// Price of a piece by its level.
int equipPrice(const int level)
{
    return 60 + level * level * 6;
}

struct Catalogue {
    gamelib::core::ItemRegistry registry;
    std::map<int, ItemInfo> info;
    std::vector<int> equipment;
};

Catalogue build()
{
    Catalogue c;
    const auto use = [&](const int id, std::string_view name, const int price, const int heal, const int mana, const int icon,
                         std::string_view text) {
        c.registry.add({.id = id, .name = std::string{name}, .kind = static_cast<int>(Tab::Use), .stackLimit = 100, .price = price,
                        .sellPrice = price / 4});
        c.info[id] = {.id = id, .tab = Tab::Use, .heal = heal, .mana = mana, .icon = icon, .text = text};
    };
    use(kRedPotion, "Red Potion", 25, 50, 0, 1, "A sweet berry tonic. Restores 50 HP.");
    use(kOrangePotion, "Orange Potion", 80, 150, 0, 2, "Tastes of sunshine. Restores 150 HP.");
    use(kWhitePotion, "White Potion", 220, 350, 0, 3, "Creamy and filling. Restores 350 HP.");
    use(kBluePotion, "Blue Potion", 30, 0, 40, 4, "Fizzy and cool. Restores 40 MP.");
    use(kVioletPotion, "Violet Potion", 100, 0, 120, 5, "Lavender and starlight. Restores 120 MP.");
    use(kHoneyElixir, "Honey Elixir", 420, 300, 150, 6, "Mossbough honey. Restores 300 HP and 150 MP.");
    use(kReturnScroll, "Return Scroll", 120, 0, 0, 7, "Read it to return to the nearest town.");

    static constexpr std::array<std::string_view, 14> kDrops{
        "Sprout Cap", "Dew Drop", "Swirl Shell", "Crab Claw", "Puff Feather", "Hog Bristle", "Stump Twig",
        "Glow Spore", "Wisp Leaf", "Snow Tuft", "Pebble Heart", "Ice Bead", "Bat Wing", "Rune Chip"};
    static constexpr std::array<std::string_view, 14> kDropText{
        "A soft little mushroom cap.", "It wobbles like jelly.", "A pretty spiral shell.", "Still a bit pinchy.",
        "Fluffy and light.", "Bristly and warm.", "A twig with a tiny leaf.", "It glows in the dark.", "Rustles by itself.",
        "Cold and fluffy.", "A warm stone heart.", "Never melts.", "Leathery and small.", "Carved with old runes."};
    for (int i = 0; i < 14; ++i) {
        const int id = kFirstDrop + i;
        c.registry.add({.id = id, .name = std::string{kDrops[static_cast<std::size_t>(i)]}, .kind = static_cast<int>(Tab::Etc),
                        .stackLimit = 200, .price = 0, .sellPrice = 4 + i * 3});
        c.info[id] = {.id = id, .tab = Tab::Etc, .icon = 20 + i, .text = kDropText[static_cast<std::size_t>(i)]};
    }
    const auto quest = [&](const int id, std::string_view name, const int icon, std::string_view text) {
        c.registry.add({.id = id, .name = std::string{name}, .kind = static_cast<int>(Tab::Etc), .stackLimit = 1});
        c.info[id] = {.id = id, .tab = Tab::Etc, .icon = icon, .text = text};
    };
    quest(kNanaLetter, "Nana's Letter", 40, "Sealed with a pressed petal. For Harbormaster Quill.");
    quest(kDottieParcel, "Dottie's Parcel", 41, "A parcel of spices for Lila in Mossbough.");
    quest(kDawnShard, "Shard of Dawn", 42, "Warm, golden, humming softly.");
    quest(kFrostShard, "Shard of Frost", 43, "Cold and blue, it hums the same tune.");
    quest(kHeartShard, "Heart of the Beacon", 44, "The last shard: it pulses like a heartbeat.");
    quest(kLighthouseLens, "Lighthouse Lens", 45, "A cracked old lens from Lighthouse Hill.");

    for (const Equip& e : equipment()) {
        const int price = equipPrice(e.level);
        c.registry.add({.id = e.id, .name = std::string{e.name}, .kind = static_cast<int>(Tab::Equip), .stackLimit = 1, .price = price,
                        .sellPrice = price / 5, .equipSlot = static_cast<int>(e.slot), .levelRequirement = e.level, .jobs = e.jobs,
                        .stats = e.stats});
        c.info[e.id] = {.id = e.id, .tab = Tab::Equip, .look = e.look, .icon = 60 + static_cast<int>(e.slot), .text = e.text};
        c.equipment.push_back(e.id);
    }
    return c;
}

const Catalogue& catalogue()
{
    static const Catalogue c = build();
    return c;
}

// The bonuses a slot can roll, by weight.
const gamelib::core::WeightedTable<Bonus>& rollTable(const Slot slot)
{
    using B = Bonus;
    static const std::array<gamelib::core::WeightedTable<Bonus>, kSlotCount> tables{{
        gamelib::core::WeightedTable<Bonus>{{{B::Attack, 3}, {B::Magic, 3}, {B::Str, 2}, {B::Int, 2}, {B::Crit, 2}, {B::Agi, 1}}},
        gamelib::core::WeightedTable<Bonus>{{{B::Defense, 3}, {B::Hp, 2}, {B::Mp, 2}, {B::Int, 1}, {B::Vit, 1}}},
        gamelib::core::WeightedTable<Bonus>{{{B::Defense, 3}, {B::Hp, 3}, {B::Vit, 2}, {B::Str, 1}, {B::Int, 1}}},
        gamelib::core::WeightedTable<Bonus>{{{B::Defense, 3}, {B::Agi, 2}, {B::Hp, 1}, {B::Mp, 1}}},
        gamelib::core::WeightedTable<Bonus>{{{B::Speed, 3}, {B::Jump, 2}, {B::Agi, 2}, {B::Defense, 1}}},
        gamelib::core::WeightedTable<Bonus>{{{B::Attack, 2}, {B::Magic, 2}, {B::Crit, 2}, {B::Agi, 1}}},
    }};
    return tables[static_cast<std::size_t>(slot)];
}

int rollValue(const Bonus b, const int level, gamelib::core::Rng& rng)
{
    const int scale = 1 + level / 8;
    switch (b) {
    case Bonus::Hp: return 10 * scale + rng.range(0, 10 * scale);
    case Bonus::Mp: return 8 * scale + rng.range(0, 8 * scale);
    case Bonus::Crit: return 1 + rng.range(0, 1 + level / 15);
    case Bonus::Speed:
    case Bonus::Jump: return 2 + rng.range(0, 2 + level / 15);
    case Bonus::Attack:
    case Bonus::Magic: return scale + rng.range(0, 1 + scale);
    case Bonus::Defense: return 2 * scale + rng.range(0, 2 * scale);
    default: return 1 + rng.range(0, scale);
    }
}

} // namespace

const gamelib::core::ItemRegistry& itemRegistry()
{
    return catalogue().registry;
}

const ItemInfo& itemInfo(const int id)
{
    static const ItemInfo none{};
    const auto it = catalogue().info.find(id);
    return it == catalogue().info.end() ? none : it->second;
}

Tab tabOf(const int id)
{
    return itemInfo(id).tab;
}

std::span<const int> equipmentIds()
{
    return catalogue().equipment;
}

std::string_view itemName(const int id)
{
    const ItemDef* def = itemRegistry().find(id);
    return def ? std::string_view{def->name} : std::string_view{"?"};
}

bool isEquipment(const int id)
{
    return tabOf(id) == Tab::Equip && id >= 1000;
}

Slot slotOf(const int id)
{
    const ItemDef* def = itemRegistry().find(id);
    return def && def->equipSlot >= 0 ? static_cast<Slot>(def->equipSlot) : Slot::Weapon;
}

int stackBonus(const ItemStack& stack, const Bonus bonus)
{
    if (stack.empty()) {
        return 0;
    }
    int sum = 0;
    if (const ItemDef* def = itemRegistry().find(stack.item)) {
        for (const ItemStat& s : def->stats) {
            sum += s.stat == static_cast<int>(bonus) ? s.value : 0;
        }
    }
    for (const ItemStat& s : stack.bonuses) {
        sum += s.stat == static_cast<int>(bonus) ? s.value : 0;
    }
    return sum;
}

ItemStack rollEquipment(const int item, gamelib::core::Rng& rng, const bool elite)
{
    ItemStack s{.item = item, .count = 1};
    const int roll = rng.range(0, 999);
    const int bonus = elite ? 120 : 0;
    s.rarity = roll < 10 + bonus / 4 ? 3 : roll < 60 + bonus / 2 ? 2 : roll < 230 + bonus ? 1 : 0;
    const ItemDef& def = itemRegistry().at(item);
    for (int i = 0; i < s.rarity; ++i) {
        const Bonus b = rollTable(static_cast<Slot>(def.equipSlot)).pick(rng);
        s.bonuses.push_back(st(b, rollValue(b, def.levelRequirement, rng)));
    }
    return s;
}

ItemStack rollDropEquipment(const int level, gamelib::core::Rng& rng, const bool elite)
{
    std::vector<int> fit;
    for (const int id : equipmentIds()) {
        const int need = itemRegistry().at(id).levelRequirement;
        if (need <= level + 2 && need >= level - 12) {
            fit.push_back(id);
        }
    }
    if (fit.empty()) {
        fit.push_back(kWillowBranch);
    }
    return rollEquipment(fit[static_cast<std::size_t>(rng.range(0, static_cast<int>(fit.size()) - 1))], rng, elite);
}

int sellValue(const ItemStack& stack)
{
    const ItemDef* def = itemRegistry().find(stack.item);
    if (!def) {
        return 0;
    }
    if (def->stackLimit > 1 || !isEquipment(stack.item)) {
        return def->sellPrice;
    }
    return def->sellPrice * (2 + stack.rarity * 2) / 2 + static_cast<int>(stack.bonuses.size()) * 15;
}

} // namespace realm::game
