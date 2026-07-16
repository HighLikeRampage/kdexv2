#include "../settings/functions.h"

bool c_widgets::section(std::string_view icon, std::string_view name, int section_id, int& section_variable)
{
    struct section_state
    {
        float alpha = 0;
        float width = 0;
        float text_alpha = 0;
        ImVec4 text = clr->text.text_inactive, icon = clr->text.text_inactive;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(name.data());

    section_state* state = gui->anim_container(&state, id);

    const bool selected = section_id == section_variable;

    ImVec2 text_sz = gui->text_size(var->font.instrument_medium[0], name.data());
    float min_w = SCALE(42.f);
    float max_w = SCALE(42.f) + text_sz.x + SCALE(24.f);

    const ImVec2 pos = window->DC.CursorPos;
    float row_h = SCALE(elements->section.section_height);
    ImRect rect(pos, pos + ImVec2(state->width > 0 ? state->width : min_w, row_h));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed) section_variable = section_id;

    bool expanded = selected || hovered;
    gui->easing(state->width, expanded ? max_w : min_w, 12.f, dynamic_easing);
    gui->easing(state->text_alpha, expanded ? 1.f : 0.f, 12.f, dynamic_easing);
    gui->easing(state->text, selected ? clr->base_colors.accent_clr.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 8.f, dynamic_easing);
    gui->easing(state->icon, selected ? clr->base_colors.accent_clr.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 8.f, dynamic_easing);
    gui->easing(state->alpha, selected ? 1.f : 0.f, 4.f, static_easing);

    rect.Max.x = rect.Min.x + state->width;

    if (selected)
    {
        float inset = SCALE(4.f);
        float y_pad = SCALE(2.f);
        ImRect box(rect.Min + ImVec2(inset, y_pad), rect.Max - ImVec2(inset, y_pad));

        ImU32 bg_col_top = draw->get_clr(clr->base_colors.accent_clr, state->alpha * 0.01f);
        ImU32 bg_col_bot = draw->get_clr(clr->base_colors.accent_clr, state->alpha * 0.15f);
        ImU32 border_col = IM_COL32(80, 80, 80, (int)(80 * state->alpha));

        draw->fade_rect_filled(window->DrawList, box.Min, box.Max, bg_col_top, bg_col_bot, vertically, SCALE(elements->button.rounding));
        draw->rect(window->DrawList, box.Min, box.Max, border_col, SCALE(elements->button.rounding));

        float line_w = (box.Max.x - box.Min.x) * 0.55f;
        ImVec2 center_bot = ImVec2(box.Min.x + (box.Max.x - box.Min.x) * 0.5f, box.Max.y);
        ImU32 line_col = draw->get_clr(clr->base_colors.accent_clr, state->alpha);

        draw->rect_filled(window->DrawList,
            ImVec2(center_bot.x - line_w * 0.5f, center_bot.y - SCALE(2.f)),
            ImVec2(center_bot.x + line_w * 0.5f, center_bot.y),
            line_col, SCALE(2.f));
    }

    float icon_w = SCALE(42.f);
    ImVec2 icon_min = rect.Min;
    ImVec2 icon_max = ImVec2(rect.Min.x + icon_w, rect.Max.y);
    draw->text_clipped(window->DrawList, var->font.icons[0], icon_min, icon_max, draw->get_clr(state->icon), icon.data(), NULL, NULL, { 0.5, 0.5 });

    if (state->text_alpha > 0.05f)
    {
        ImVec2 text_min = ImVec2(rect.Min.x + icon_w - SCALE(2.f), rect.Min.y);
        ImVec2 text_max = ImVec2(rect.Max.x - SCALE(8.f), rect.Max.y);
        draw->text_clipped(window->DrawList, var->font.instrument_medium[0], text_min, text_max, draw->get_clr(state->text, state->text_alpha), name.data(), NULL, NULL, { 0.0, 0.5 });
    }

    return pressed;
}

bool c_widgets::category(std::string_view name)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(name.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(gui->content_avail().x, SCALE(elements->category.category_height)));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    float text_alpha = (var->window.sidebar_width - 50.f) / 115.f;
    text_alpha = ImClamp(text_alpha, 0.f, 1.f);

    if (text_alpha > 0.1f)
    {
        ImVec2 text_sz = gui->text_size(var->font.instrument_medium[0], name.data());
        float start_x = rect.Min.x + (rect.GetWidth() - text_sz.x) * 0.5f;
        ImVec2 text_pos_min = ImVec2(start_x, rect.Min.y);
        ImVec2 text_pos_max = ImVec2(rect.Max.x, rect.Max.y);
        draw->text_clipped(window->DrawList, var->font.instrument_medium[0], text_pos_min, text_pos_max, draw->get_clr(clr->text.text_inactive, text_alpha), name.data(), NULL, NULL, { 0.0, 0.5 });
    }

    return false;
}
