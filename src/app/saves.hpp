#pragma once

// The three character slots (GameLib's storage::SaveSlots: a versioned file
// per slot with a checksum and a backup). Each holds one character's whole
// story - the look from the creator, job, level, stats, skills, the three
// bag tabs, gear, coins, quests, world flags, where the hero stands, the
// clock, the bosses' timers, the maps seen, the journey's numbers - and the
// character's own key bindings ([keys], GameLib's action-binding text), as
// a MapleStory character keeps its keyboard.

#include "session.hpp"

#include <gamelib/platform/actions.hpp>
#include <gamelib/storage/save_slots.hpp>

#include <optional>

namespace realm::app {

inline constexpr int kSlotCount = 3;

struct LoadedSlot {
    game::SaveData data;
    gamelib::storage::IniFile ini; // the [keys] for the action map
};

class Saves {
public:
    Saves();

    // nullopt for an empty or unreadable slot.
    [[nodiscard]] std::optional<LoadedSlot> load(int slot);
    [[nodiscard]] bool exists(int slot) const;
    [[nodiscard]] bool readable(int slot) const;
    void save(int slot, const game::SaveData& data, const gamelib::platform::ActionMap& actions);
    void erase(int slot);

private:
    gamelib::storage::SaveSlots m_slots;
};

// The character's key bindings: the job's defaults with the slot's [keys].
[[nodiscard]] gamelib::platform::ActionMap actionsFor(game::Job job, const gamelib::storage::IniFile* saved);

} // namespace realm::app
