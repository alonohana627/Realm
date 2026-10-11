#pragma once

// Realm's items: the catalogue (GameLib's core::ItemRegistry, built once) of
// potions, scrolls, monster drops, quest items and equipment with level and
// job requirements, what each piece looks like on the avatar, and the loot
// rules - the rarity roll and the random bonuses rolled from a weighted
// table per slot.

#include "rules.hpp"

#include <gamelib/core/items.hpp>
#include <gamelib/core/rng.hpp>

#include <cstdint>
#include <span>
#include <string_view>

namespace realm::game {

// Use items.
inline constexpr int kRedPotion = 1;    // +50 HP
inline constexpr int kOrangePotion = 2; // +150 HP
inline constexpr int kWhitePotion = 3;  // +350 HP
inline constexpr int kBluePotion = 4;   // +40 MP
inline constexpr int kVioletPotion = 5; // +120 MP
inline constexpr int kHoneyElixir = 6;  // +300 HP and +150 MP
inline constexpr int kReturnScroll = 7; // back to the nearest town
inline constexpr int kUseItemCount = 7;

// Monster drops (etc), one per species: kFirstDrop + species.
inline constexpr int kFirstDrop = 100;

// Quest items (etc).
inline constexpr int kNanaLetter = 200;
inline constexpr int kDottieParcel = 201;
inline constexpr int kDawnShard = 202;
inline constexpr int kFrostShard = 203;
inline constexpr int kHeartShard = 204;
inline constexpr int kLighthouseLens = 205;

// Equipment ids are 1000 and up (see items.cpp).
inline constexpr int kWillowBranch = 1000;
inline constexpr int kPlainTee = 1200;
inline constexpr int kCottonShorts = 1300;
inline constexpr int kRedSneakers = 1400;

// Job masks for core::ItemDef::jobs.
inline constexpr std::uint32_t kAnyJob = 0xFFFFFFFFu;
[[nodiscard]] constexpr std::uint32_t jobBit(const Job job) { return 1u << static_cast<unsigned>(job); }

// The avatar layer an item swaps (avatar.frag's kinds).
struct ItemLook {
    int style = 0;            // the slot's kind in the avatar shader
    std::uint32_t color = 0;  // sRGB: the top's cloth (tops), else unused
    int swatch = 0;           // a kAnimePalette index (hats, bottoms, shoes, gloves' accent)
};

// What the catalogue knows beyond core::ItemDef.
struct ItemInfo {
    int id = 0;
    Tab tab = Tab::Etc;
    int heal = 0;            // potions: HP restored
    int mana = 0;            // potions: MP restored
    ItemLook look;
    int icon = 0;            // the ui shader's icon family
    std::string_view text;   // one line for the item card
};

[[nodiscard]] const gamelib::core::ItemRegistry& itemRegistry();
[[nodiscard]] const ItemInfo& itemInfo(int id); // a default ItemInfo for unknown ids
[[nodiscard]] Tab tabOf(int id);
[[nodiscard]] std::span<const int> equipmentIds();
[[nodiscard]] std::string_view itemName(int id);
[[nodiscard]] bool isEquipment(int id);
[[nodiscard]] Slot slotOf(int id);

[[nodiscard]] inline Rarity rarityOf(const gamelib::core::ItemStack& stack)
{
    return static_cast<Rarity>(std::clamp(stack.rarity, 0, kRarityCount - 1));
}

// A stack's total for one bonus: the item's fixed stats plus its rolls.
[[nodiscard]] int stackBonus(const gamelib::core::ItemStack& stack, Bonus bonus);

// A new piece as it drops: the rarity rolled (elites roll better) and that
// many random bonuses from the slot's table.
[[nodiscard]] gamelib::core::ItemStack rollEquipment(int item, gamelib::core::Rng& rng, bool elite);
// A random piece for a monster of `level` (from the pieces of about that level).
[[nodiscard]] gamelib::core::ItemStack rollDropEquipment(int level, gamelib::core::Rng& rng, bool elite);

// What a shop pays for a stack (equipment by rarity and rolls).
[[nodiscard]] int sellValue(const gamelib::core::ItemStack& stack);

} // namespace realm::game
