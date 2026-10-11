#include "dialogue_view.hpp"

#include "look.hpp"
#include "ui_kit.hpp"


#include <gamelib/core/easing.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace realm::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::draw::Font;

constexpr Rect kBox{360.0f, 770.0f, 1260.0f, 250.0f};

gamelib::ui::DialogueBoxStyle boxStyle(const Shapes& shapes, const std::shared_ptr<float>& time)
{
    const gamelib::draw::VectorTextShadow outline{.color = srgb(0x2A3E5A, 0.85f), .offset = {0.0f, 0.0f}, .weight = 4.2f};
    gamelib::ui::DialogueBoxStyle s;
    s.panel.painter = [&shapes](gamelib::draw::DrawList& list, const Rect& rect) {
        addUi(list, shapes, Ui::Window, rect.expanded(kit::kMargin), ui_color::kWindow, ui_color::kTitle, 0.0f, 0.0f, 24.0f);
    };
    s.font = Font::vector(33.0f, 1.3f);
    s.speakerFont = Font::vector(29.0f, 1.7f).withShadow(outline);
    s.padding = 32.0f;
    s.portraitSize = 300.0f; // the text starts past the leaning portrait
    s.portraitGap = 20.0f;
    s.text = ui_color::kInk;
    s.speaker = srgb(0xFFFFFF);
    s.marker = ui_color::kGold;
    s.choiceDim = ui_color::kSoftInk;
    s.charsPerSecond = 55.0f;
    s.sentencePause = 0.2f;
    s.commaPause = 0.07f;
    s.confirmGuard = 0.08f;
    s.tagAt = gamelib::core::Vec2{332.0f, -36.0f};
    s.tagPadding = {28.0f, 15.0f};
    s.choiceHeight = 56.0f;
    s.choiceFont = Font::vector(29.0f, 1.25f);
    s.selectedWeight = 1.25f;
    s.tagPainter = [&shapes](gamelib::draw::DrawList& list, const Rect& tag) {
        addUi(list, shapes, Ui::Button, tag.expanded(kit::kMargin), srgb(0x7CC2E4), srgb(0xFFFFFF), 0.0f);
    };
    s.choicePainter = [&shapes, time](gamelib::draw::DrawList& list, const Rect& row, const std::size_t, const bool selected) {
        if (!selected) {
            return;
        }
        const Rect pill{row.x - 6.0f, row.y + 3.0f, row.width + 12.0f, row.height - 6.0f};
        addUi(list, shapes, Ui::Button, pill.expanded(kit::kMargin), ui_color::kButton, ui_color::kGold, 1.0f);
        const float nudge = std::sin(*time * 8.0f) * 3.0f;
        addUi(list, shapes, Ui::Arrow, Rect{pill.x - 26.0f + nudge, pill.center().y - 11.0f, 22.0f, 22.0f}.expanded(kit::kMargin),
              ui_color::kGold, ui_color::kGold);
    };
    s.markerPainter = [&shapes, time](gamelib::draw::DrawList& list, const gamelib::core::Vec2 at, const bool last) {
        const float bob = std::sin(*time * 6.0f) * 4.0f;
        const gamelib::core::Color c = last ? ui_color::kTitle : ui_color::kGold;
        addUi(list, shapes, Ui::Arrow, Rect{at.x - 22.0f, at.y - 26.0f + bob, 26.0f, 26.0f}.expanded(kit::kMargin), c, c, 1.0f);
    };
    return s;
}

} // namespace

DialogueView::DialogueView(const Shapes& shapes)
    : m_box{kBox, boxStyle(shapes, m_time)}
{
}

void DialogueView::open(std::string speaker, std::string role, const std::string_view text, std::vector<std::string> choices,
                        const std::optional<game::NpcId> portrait)
{
    if (!m_box.isOpen()) {
        m_age = 0.0f;
    }
    m_role = std::move(role);
    m_portrait = portrait;
    m_box.open(std::move(speaker), text, std::move(choices));
}

void DialogueView::update(const float seconds)
{
    if (!m_box.isOpen()) {
        return;
    }
    m_age += seconds;
    m_box.update(seconds);
}

DialogueStep DialogueView::handle(const gamelib::ui::MenuInput input)
{
    using gamelib::ui::DialogueEvent;
    const gamelib::ui::DialogueResult r = m_box.handle(input);
    switch (r.event) {
    case DialogueEvent::Completed:
    case DialogueEvent::NextPage: return DialogueStep::Completed;
    case DialogueEvent::Finished: return DialogueStep::Next;
    case DialogueEvent::Chose: m_choice = r.choice; return DialogueStep::Chose;
    case DialogueEvent::None: break;
    }
    return DialogueStep::None;
}

void DialogueView::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const float time) const
{
    if (!m_box.isOpen()) {
        return;
    }
    *m_time = time;
    const float pop = gamelib::core::easeOutBack(std::min(m_age / 0.25f, 1.0f));
    const float slide = (1.0f - pop) * 60.0f;
    list.setLayer(gamelib::draw::layers::Hud + 40);
    gamelib::ui::drawDialogueBox(list, m_box, time, {0.0f, slide});
    if (!m_role.empty() && !m_box.speaker().empty()) {
        // The role in the box's top right corner, clear of the text.
        kit::label(list, m_role, kBox.right() - 28.0f, kBox.y + 26.0f + slide, 21.0f, ui_color::kSoftInk, gamelib::draw::Align::Right);
    }
    // The speaker leans in over the box's left end, eyes open (the
    // owner's rule: closed eyes only as a reaction).
    list.setLayer(gamelib::draw::layers::Hud + 42);
    if (m_portrait) {
        anime::Avatar a = npcAvatar(*m_portrait);
        a.feet = {kBox.x + 160.0f, kBox.bottom() + 70.0f + slide};
        a.height = 600.0f;
        a.facing = 1.0f;
        a.pose = anime::AvatarPose::Idle;
        a.phase = time * 0.6f;
        a.expression = anime::Expression::Neutral;
        a.nextExpression = anime::Expression::Neutral;
        a.seed = static_cast<std::uint8_t>(40 + static_cast<int>(*m_portrait));
        list.add(anime::avatarSprite(a, shapes.avatar));
    }
}

} // namespace realm::app
