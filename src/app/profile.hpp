#pragma once

// What Realm keeps outside the character slots, in realm.ini in the user
// data folder (REALM_DATA_DIR moves it): GameLib's standard settings, the
// fastest finished story, the slot played last and the minimap's size. Key
// bindings belong to each character (MapleStory-like) and live in its slot.

#include <gamelib/app/arcade_profile.hpp>
#include <gamelib/storage/ini.hpp>

#include <string_view>

namespace realm::app {

struct Prefs {
    int lastSlot = 0; // 0..2
    int minimap = 1;  // 0 hidden, 1 small, 2 large

    void read(const gamelib::storage::IniFile& ini);
    void write(gamelib::storage::IniFile& ini) const;
    friend bool operator==(const Prefs&, const Prefs&) = default;
};

using ProfileFile = gamelib::app::ArcadeProfileFileOf<Prefs>;

[[nodiscard]] ProfileFile openProfile();

inline constexpr std::string_view kAppName = "Realm";
inline constexpr std::string_view kDataDirVariable = "REALM_DATA_DIR";
// The fastest story to the end (ticks, lower is better).
inline constexpr std::string_view kBestTime = "story_ticks";

} // namespace realm::app
