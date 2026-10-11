#include "profile.hpp"

namespace realm::app {

void Prefs::read(const gamelib::storage::IniFile& ini)
{
    lastSlot = ini.getInt("game", "last_slot", 0, 2).value_or(0);
    minimap = ini.getInt("game", "minimap", 0, 2).value_or(1);
}

void Prefs::write(gamelib::storage::IniFile& ini) const
{
    ini.setInt("game", "last_slot", lastSlot);
    ini.setInt("game", "minimap", minimap);
}

ProfileFile openProfile()
{
    return gamelib::app::openArcadeProfile<Prefs>(kAppName, kDataDirVariable);
}

} // namespace realm::app
