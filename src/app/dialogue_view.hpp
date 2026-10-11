#pragma once

// The conversation box: GameLib's ui::DialogueBox (pages, the typewriter
// reveal pausing after punctuation, choices) in Realm's soft window with a
// sky-blue name tag, the speaker's role in the corner and the speaker's chibi
// avatar leaning in big over the box's left end (its eyes open: closed
// eyes are for reactions only). One node at a time: the game
// walks the core::DialogueRunner and passes the text through
// platform::withKeys, so "{attack}" shows the player's own key.

#include "shapes.hpp"

#include "maps.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/platform/actions.hpp>
#include <gamelib/ui/dialogue_box.hpp>
#include <gamelib/ui/menu.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace realm::app {

enum class DialogueStep { None, Completed, Next, Chose };

class DialogueView {
public:
    explicit DialogueView(const Shapes& shapes);

    void open(std::string speaker, std::string role, std::string_view text, std::vector<std::string> choices,
              std::optional<game::NpcId> portrait);
    void close() { m_box.close(); }
    [[nodiscard]] bool isOpen() const { return m_box.isOpen(); }
    void update(float seconds);
    // Confirm completes the page, then turns it; on the last page it ends
    // (Next) or picks the selected choice (Chose).
    DialogueStep handle(gamelib::ui::MenuInput input);
    [[nodiscard]] std::size_t choice() const { return m_choice; }
    [[nodiscard]] int typedThisFrame() const { return static_cast<int>(m_box.revealedLastUpdate()); }
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, float time) const;

private:
    // The frame's time for the painters' bobbing arrows (shared with them,
    // so it survives the view being moved).
    std::shared_ptr<float> m_time = std::make_shared<float>(0.0f);
    gamelib::ui::DialogueBox m_box;
    std::string m_role;
    std::optional<game::NpcId> m_portrait;
    std::size_t m_choice = 0;
    float m_age = 0.0f;
};

} // namespace realm::app
