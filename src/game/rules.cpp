#include "rules.hpp"

namespace realm::game {

std::string_view regionName(const Region region)
{
    switch (region) {
    case Region::Island: return "Petal Isle";
    case Region::Coast: return "Saltwhistle Coast";
    case Region::Forest: return "Fernwood";
    case Region::Mountain: return "Windcrest Pass";
    case Region::Depths: return "The Undercroft";
    case Region::Count: break;
    }
    return "";
}

std::string_view jobName(const Job job)
{
    switch (job) {
    case Job::Beginner: return "Beginner";
    case Job::Warrior: return "Warrior";
    case Job::Mage: return "Mage";
    case Job::Count: break;
    }
    return "";
}

std::string_view coreName(const Core stat)
{
    switch (stat) {
    case Core::Str: return "Strength";
    case Core::Agi: return "Agility";
    case Core::Int: return "Intellect";
    case Core::Vit: return "Vitality";
    case Core::Count: break;
    }
    return "";
}

std::string_view coreShort(const Core stat)
{
    switch (stat) {
    case Core::Str: return "str";
    case Core::Agi: return "agi";
    case Core::Int: return "int";
    case Core::Vit: return "vit";
    case Core::Count: break;
    }
    return "";
}

std::string_view bonusName(const Bonus bonus)
{
    switch (bonus) {
    case Bonus::Str: return "strength";
    case Bonus::Agi: return "agility";
    case Bonus::Int: return "intellect";
    case Bonus::Vit: return "vitality";
    case Bonus::Attack: return "attack";
    case Bonus::Magic: return "magic";
    case Bonus::Defense: return "defense";
    case Bonus::Hp: return "max hp";
    case Bonus::Mp: return "max mp";
    case Bonus::Crit: return "critical";
    case Bonus::Speed: return "speed";
    case Bonus::Jump: return "jump";
    case Bonus::Count: break;
    }
    return "";
}

std::string_view slotName(const Slot slot)
{
    switch (slot) {
    case Slot::Weapon: return "weapon";
    case Slot::Hat: return "hat";
    case Slot::Top: return "top";
    case Slot::Bottom: return "bottom";
    case Slot::Shoes: return "shoes";
    case Slot::Gloves: return "gloves";
    case Slot::Count: break;
    }
    return "";
}

std::string_view tabName(const Tab tab)
{
    switch (tab) {
    case Tab::Equip: return "equip";
    case Tab::Use: return "use";
    case Tab::Etc: return "etc";
    case Tab::Count: break;
    }
    return "";
}

std::string_view rarityName(const Rarity rarity)
{
    switch (rarity) {
    case Rarity::Common: return "common";
    case Rarity::Rare: return "rare";
    case Rarity::Epic: return "epic";
    case Rarity::Unique: return "unique";
    case Rarity::Count: break;
    }
    return "";
}

} // namespace realm::game
