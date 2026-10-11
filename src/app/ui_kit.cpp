#include "ui_kit.hpp"

#include <gamelib/core/timers.hpp>
#include <gamelib/draw/font.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace realm::app::kit {

namespace {

// helpers


} // namespace

Font font(const float size, const float weight)
{
    return Font::vector(size, weight);
}

Font titleFont(const float size)
{
    return Font::vector(size, 1.6f).withShadow({.color = srgb(0x2A4A6A, 0.55f), .offset = {0.0f, 2.0f}, .weight = 2.4f});
}

Font outlined(const float size, const float weight)
{
    return Font::vector(size, weight).withShadow({.color = srgb(0x2A1E3A, 0.85f), .offset = {0.0f, 0.0f}, .weight = weight + 2.2f});
}

void text(gamelib::draw::DrawList& list, const std::string_view t, const float x, const float y, const float size, const Color color,
          const Align align, const float weight)
{
    gamelib::draw::drawText(list, font(size, weight), t, x, y, color, align);
}

void label(gamelib::draw::DrawList& list, const std::string_view t, const float x, const float centerY, const float size,
           const Color color, const Align align, const float weight)
{
    const Font f = font(size, weight);
    gamelib::draw::drawText(list, f, t, x, f.topFor(centerY), color, align);
}

float width(const std::string_view t, const float size, const float weight)
{
    return font(size, weight).width(t);
}

Rect window(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const std::string_view title, const Color accent,
            const float alpha)
{
    constexpr float kTitle = 52.0f;
    addUi(list, shapes, Ui::Window, rect.expanded(kMargin), ui_color::kWindow.withAlpha(alpha), accent, title.empty() ? 0.0f : kTitle,
          0.0f, 22.0f);
    if (!title.empty()) {
        const Font f = titleFont(32.0f);
        gamelib::draw::drawText(list, f, title, rect.center().x, f.topFor(rect.top() + kTitle * 0.5f), srgb(0xFFFFFF, alpha), Align::Center);
    }
    const float top = title.empty() ? 18.0f : kTitle + 16.0f;
    return {rect.x + 22.0f, rect.y + top, rect.width - 44.0f, rect.height - top - 20.0f};
}

void well(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const float alpha)
{
    addUi(list, shapes, Ui::Well, rect.expanded(kMargin), srgb(0xF4E8D2, alpha), ui_color::kTitle, 0.0f, 0.0f, 14.0f);
}

void tooltip(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const float alpha)
{
    addUi(list, shapes, Ui::Tooltip, rect.expanded(kMargin), srgb(0x2E2A44, 0.94f * alpha), srgb(0x8ACBE8), 0.0f, 0.0f, 14.0f);
}

void button(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const std::string_view t, const int state,
            const float size, const Color fill)
{
    addUi(list, shapes, Ui::Button, rect.expanded(kMargin), fill, ui_color::kGold, static_cast<float>(state));
    const float sink = state == 2 ? 3.0f : 0.0f;
    label(list, t, rect.center().x, rect.center().y - 2.0f + sink, size, state == 3 ? ui_color::kSoftInk : ui_color::kInk, Align::Center,
          state == 1 ? 1.6f : 1.35f);
}

void slotIcon(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const int item, const int count, const float alpha)
{
    if (item == 0) {
        return;
    }
    const IconLook icon = itemIcon(item);
    addIcon(list, shapes, Ui::ItemIcon, rect.center(), rect.height * 0.82f, icon.family, icon.style, icon.a.withAlpha(alpha), icon.b);
    if (count > 1) {
        const Font f = outlined(20.0f, 1.4f);
        gamelib::draw::drawText(list, f, std::to_string(count), rect.right() - 6.0f, rect.bottom() - f.height() - 6.0f,
                                srgb(0xFFFFFF, alpha), Align::Right);
    }
}

void slot(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const gamelib::core::ItemStack& stack, const bool selected,
          const float time, const float alpha)
{
    (void)time;
    const game::Rarity rarity = stack.empty() ? game::Rarity::Common : game::rarityOf(stack);
    const float glow = rarity == game::Rarity::Common ? 0.0f : 0.6f + 0.15f * static_cast<float>(rarity);
    addUi(list, shapes, Ui::Slot, rect.expanded(kMargin), srgb(0xF4E8D2, alpha), rarityColor(rarity), glow, selected ? 1.0f : 0.0f, 12.0f);
    if (!stack.empty()) {
        slotIcon(list, shapes, rect, stack.item, stack.count, alpha);
    }
}

void keyCap(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 center, const std::string_view t, const float height)
{
    const float size = height * 0.52f;
    const float w = std::max(height, width(t, size, 1.4f) + height * 0.6f);
    addUi(list, shapes, Ui::KeyCap, Rect{center.x - w * 0.5f, center.y - height * 0.5f, w, height}.expanded(kMargin), srgb(0xF6F0E6),
          srgb(0xFFFFFF));
    label(list, t, center.x, center.y - height * 0.06f, size, ui_color::kInk, Align::Center, 1.4f);
}

void gauge(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const float fill, const float trail, const Color color,
           const Color trailColor)
{
    addUi(list, shapes, Ui::Gauge, rect.expanded(kMargin), color, trailColor, std::clamp(fill, 0.0f, 1.0f), std::clamp(trail, 0.0f, 1.0f));
}

void tab(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const std::string_view t, const bool selected)
{
    addUi(list, shapes, Ui::Tab, rect.expanded(kMargin), selected ? srgb(0xFFF8EA) : srgb(0xE8DCC8), ui_color::kTitle, selected ? 1.0f : 0.0f);
    label(list, t, rect.center().x, rect.center().y + (selected ? 0.0f : 3.0f), 24.0f, selected ? ui_color::kInk : ui_color::kSoftInk,
          Align::Center, selected ? 1.55f : 1.3f);
}

void sweep(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const float remaining)
{
    if (remaining <= 0.0f) {
        return;
    }
    addUi(list, shapes, Ui::Sweep, rect.expanded(kMargin), srgb(0x000000), srgb(0xFFF2C0), remaining, 0.0f, 12.0f);
}

void skillIconAt(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 center, const float size, const game::SkillId skill,
                 const float alpha)
{
    const IconLook icon = skillIcon(skill);
    addIcon(list, shapes, Ui::SkillIcon, center, size, icon.family, icon.style, icon.a.withAlpha(alpha), icon.b);
}

float itemCard(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const gamelib::core::ItemStack& stack,
               const game::Character& character, const bool compare)
{
    if (stack.empty()) {
        return 0.0f;
    }
    const gamelib::core::ItemDef* def = game::itemRegistry().find(stack.item);
    if (def == nullptr) {
        return 0.0f;
    }
    gamelib::ui::ItemCard card;
    const game::Rarity rarity = game::rarityOf(stack);
    card.name = def->name;
    card.nameColor = rarity == game::Rarity::Common ? srgb(0xFFF6E0) : rarityColor(rarity);
    const game::ItemInfo& info = game::itemInfo(stack.item);
    if (game::isEquipment(stack.item)) {
        const game::Slot slot = game::slotOf(stack.item);
        std::string jobs = "any job";
        if (def->jobs == game::jobBit(game::Job::Warrior)) {
            jobs = "warrior";
        } else if (def->jobs == game::jobBit(game::Job::Mage)) {
            jobs = "mage";
        }
        card.subtitle = std::format("{} {} - lv {} - {}", game::rarityName(rarity), game::slotName(slot), def->levelRequirement, jobs);
        card.subtitleWarning = def->levelRequirement > character.level || (def->jobs & game::jobBit(character.job)) == 0;
        const gamelib::core::ItemStack& worn = character.gear.at(static_cast<int>(slot));
        for (const gamelib::core::StatComparison& s : gamelib::core::compareStacks(game::itemRegistry(), stack, compare ? worn : gamelib::core::ItemStack{})) {
            gamelib::ui::ItemCardLine line{.label = std::string{game::bonusName(static_cast<game::Bonus>(s.stat))}, .value = s.value};
            if (compare && !worn.empty() && worn != stack) {
                line.difference = s.difference();
            }
            line.percent = s.stat == static_cast<int>(game::Bonus::Speed) || s.stat == static_cast<int>(game::Bonus::Jump)
                           || s.stat == static_cast<int>(game::Bonus::Crit);
            card.lines.push_back(line);
        }
    } else {
        card.subtitle = std::string{game::tabName(info.tab == game::Tab::Use ? game::Tab::Use : game::Tab::Etc)} + " item";
    }
    card.text = std::string{info.text};
    if (def->sellPrice > 0) {
        card.text += std::format("\nsells for {} coins", groupDigits(game::sellValue(stack)));
    }
    tooltip(list, shapes, rect);
    gamelib::ui::ItemCardStyle style;
    style.nameFont = Font::vector(30.0f, 1.6f);
    style.font = Font::vector(24.0f, 1.3f);
    style.smallFont = Font::vector(20.0f, 1.2f);
    style.text = srgb(0xF6F0E4);
    style.dim = srgb(0xB8B0C8);
    style.warning = srgb(0xFF8A8A);
    style.better = srgb(0x8CF08C);
    style.worse = srgb(0xFF8A7A);
    style.iconSize = 72.0f;
    const Rect inner = rect.expanded(-16.0f);
    const Rect iconRect = gamelib::ui::itemCardIconRect(inner, style);
    addUi(list, shapes, Ui::Slot, iconRect.expanded(kMargin), srgb(0xFFF6E8), rarityColor(rarity), rarity == game::Rarity::Common ? 0.0f : 0.8f,
          0.0f, 12.0f);
    slotIcon(list, shapes, iconRect, stack.item, 1);
    return gamelib::ui::drawItemCard(list, card, inner, style);
}

std::string groupDigits(const long long value)
{
    return gamelib::core::groupDigits(value);
}

} // namespace realm::app::kit
