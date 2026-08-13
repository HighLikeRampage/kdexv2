#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"

bool begin_list_box_ex(std::string_view name, ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    struct c_list
    {
        float state = 0, slow = 0;
    };

    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    IM_ASSERT(id != 0);

    const ImGuiChildFlags ImGuiChildFlags_SupportedMask_ = ImGuiChildFlags_Border | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_FrameStyle;
    IM_UNUSED(ImGuiChildFlags_SupportedMask_);

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    if (window_flags & ImGuiWindowFlags_AlwaysUseWindowPadding) child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
#endif

    if (child_flags & ImGuiChildFlags_AutoResizeX) child_flags &= ~ImGuiChildFlags_ResizeX;
    if (child_flags & ImGuiChildFlags_AutoResizeY) child_flags &= ~ImGuiChildFlags_ResizeY;

    window_flags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar;
    window_flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);

    if (child_flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize)) window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    if ((child_flags & (ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY)) == 0) window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasChildFlags;
    g.NextWindowData.ChildFlags = child_flags;

    const ImVec2 size_avail = GetContentRegionAvail();
    const ImVec2 size_default((child_flags & ImGuiChildFlags_AutoResizeX) ? 0.0f : size_avail.x, (child_flags & ImGuiChildFlags_AutoResizeY) ? 0.0f : size_avail.y);

    c_list* state = gui->anim_container(&state, id);

    ImVec2 size = size_arg;
    if (size.y <= 0) size.y = state->state + SCALE(elements->listbox.list_padding.y) * 2;

    gui->set_next_window_size(size);
    gui->set_next_window_pos(parent_window->DC.CursorPos);

    draw->rect_filled(GetWindowDrawList(), parent_window->DC.CursorPos, parent_window->DC.CursorPos + ImVec2(size.x, size.y), draw->get_clr(clr->window.window_layout), SCALE(elements->listbox.rounding));
    draw->rect(GetWindowDrawList(), parent_window->DC.CursorPos, parent_window->DC.CursorPos + ImVec2(size.x, size.y), draw->get_clr(clr->window.window_stroke), SCALE(elements->listbox.rounding));

    float separator_y = parent_window->DC.CursorPos.y + size.y + SCALE(elements->listbox.padding);
    float separator_width = size.x * 0.75f;
    float separator_start_x = parent_window->DC.CursorPos.x + (size.x - separator_width) / 2.0f;
    float separator_end_x = separator_start_x + separator_width;
    draw->line(GetWindowDrawList(), ImVec2(separator_start_x, separator_y), ImVec2(separator_end_x, separator_y), draw->get_clr(clr->window.separator), 1.f);

    const char* temp_window_name;

    if (name.data())
        ImFormatStringToTempBuffer(&temp_window_name, NULL, xorstr("%s/%s_%08X"), parent_window->Name, name, id);
    else
        ImFormatStringToTempBuffer(&temp_window_name, NULL, xorstr("%s/%08X"), parent_window->Name, id);

    const bool ret = gui->begin(temp_window_name, NULL, window_flags | ImGuiWindowFlags_NoBackground);

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;

    state->slow = ImLerp(state->slow, child_window->ContentSize.y, gui->fixed_speed(12));
    state->state = state->slow;

    if (child_window->BeginCount == 1) parent_window->DC.CursorPos = child_window->Pos;

    const ImGuiID temp_id_for_activation = ImHashStr(xorstr("##Child"), 0, id);
    if (g.ActiveId == temp_id_for_activation) ClearActiveID();

    if (g.NavActivateId == id && !(window_flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(temp_id_for_activation, child_window);
        g.ActiveIdSource = g.NavInputSource;
    }
    return ret;
}

bool c_widgets::begin_list(std::string_view name, const ImVec2& size_arg)
{
    ImGuiID id = GetCurrentWindow()->GetID(name.data());

    gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->listbox.list_padding));
    gui->push_var(ImGuiStyleVar_ItemSpacing, SCALE(elements->listbox.list_spacing));

    gui->set_pos(gui->get_pos().y + SCALE(elements->listbox.padding), pos_y);
    return begin_list_box_ex(name.data(), id, size_arg, ImGuiChildFlags_None, (size_arg.y > 0.f) ? (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoMove) : (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar));
}

void c_widgets::end_list()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* child_window = g.CurrentWindow;

    gui->pop_var(2);

    IM_ASSERT(g.WithinEndChild == false);
    IM_ASSERT(child_window->Flags & ImGuiWindowFlags_ChildWindow);

    if (child_window->ContentSize.y <= 0.f) {
        ImVec2 pad = child_window->WindowPadding;
        ImVec2 text_min = child_window->Pos + pad;
        ImVec2 text_max = child_window->Pos + child_window->Size - pad;
        draw->text_clipped(GetWindowDrawList(), var->font.instrument_medium[0], text_min, text_max, draw->get_clr(clr->text.text_inactive), xorstr("It's still empty here"), 0, 0, { 0.5f, 0.5f }, NULL);
    }

    g.WithinEndChild = true;
    ImVec2 child_size = child_window->Size;
    gui->end();

    gui->set_pos(gui->get_pos().y + SCALE(elements->listbox.padding), pos_y);

    if (child_window->BeginCount == 1)
    {
        ImGuiWindow* parent_window = g.CurrentWindow;
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + child_size);
        ItemSize(child_size);

        if (child_window->Flags & ImGuiWindowFlags_NavFlattened) parent_window->DC.NavLayersActiveMaskNext |= child_window->DC.NavLayersActiveMaskNext;
        if (g.HoveredWindow == child_window)  g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
    }
    g.WithinEndChild = false;
    g.LogLinePosY = -FLT_MAX;
}

bool c_widgets::list_content(std::string_view label, bool active, ImDrawFlags flags, bool draw_checkmark)
{
    struct c_list
    {
        float alpha = 0.f, offset = 10.f;
        ImVec4 text = clr->text.text_inactive;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const float width = gui->content_avail().x;
    const float height = SCALE(elements->dropdown.selection_height);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(width, height));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed) MarkItemEdited(id);

    c_list* state = gui->anim_container(&state, id);

    gui->easing(state->text, (IsItemActive() || active) ? clr->text.text_active.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 12.f, dynamic_easing);
    gui->easing(state->offset, active ? (draw_checkmark ? elements->listbox.text_padding[1] : elements->listbox.text_padding[0]) : elements->listbox.text_padding[0], 16.f, dynamic_easing, true);
    gui->easing(state->alpha, hovered ? 0.35f : active ? 0.15f : 0.05f, 16.f, dynamic_easing);

    ImVec4 accent = clr->base_colors.accent_clr.Value;
    float r_val = ImClamp(accent.x, 0.0f, 1.0f);
    float g_val = ImClamp(accent.y, 0.0f, 1.0f);
    float b_val = ImClamp(accent.z, 0.0f, 1.0f);
    float gradient_alpha = state->alpha * 0.7f;
    ImU32 col_bg = IM_COL32((int)(r_val * 255), (int)(g_val * 255), (int)(b_val * 255), (int)(255 * gradient_alpha));
    ImU32 border_col = IM_COL32(80, 80, 80, 80);
    float border_alpha = ImClamp(state->alpha + 0.15f, 0.15f, 0.5f);

    draw->rounded_gradient_rect(window->DrawList, rect.Min, rect.Max, col_bg, col_bg, border_col, SCALE(elements->listbox.rounding), ImDrawFlags_RoundCornersAll);
    draw->rect(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->widget.stroke, border_alpha), SCALE(elements->listbox.rounding), ImDrawFlags_RoundCornersAll);

    if (draw_checkmark)
        draw->render_checkmark(window->DrawList, rect.Min, rect.Min + ImVec2(height, height), draw->get_clr(clr->base_colors.accent_clr, active ? 1.f : 0.f), SCALE(9), SCALE(1.5f));

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min + SCALE(state->offset, 0), rect.Max, draw->get_clr(state->text), label.data(), gui->text_end(label.data()), 0, { 0.0, 0.5 }, NULL);

    return pressed;
}

bool c_widgets::list_content_col(std::string_view label, bool active, const ImVec4& text_color, ImDrawFlags flags, bool draw_checkmark)
{
    struct c_list
    {
        float alpha = 0.f, offset = 10.f;
        ImVec4 text;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const float width = gui->content_avail().x;
    const float height = SCALE(elements->dropdown.selection_height);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(width, height));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed) MarkItemEdited(id);

    c_list* state = gui->anim_container(&state, id);

    state->text = text_color;
    gui->easing(state->offset, active ? (draw_checkmark ? elements->listbox.text_padding[1] : elements->listbox.text_padding[0]) : elements->listbox.text_padding[0], 16.f, dynamic_easing, true);
    gui->easing(state->alpha, hovered ? 0.35f : active ? 0.15f : 0.05f, 16.f, dynamic_easing);

    ImVec4 accent = clr->base_colors.accent_clr.Value;
    float r_val = ImClamp(accent.x, 0.0f, 1.0f);
    float g_val = ImClamp(accent.y, 0.0f, 1.0f);
    float b_val = ImClamp(accent.z, 0.0f, 1.0f);
    float gradient_alpha = state->alpha * 0.7f;
    ImU32 col_bg = IM_COL32((int)(r_val * 255), (int)(g_val * 255), (int)(b_val * 255), (int)(255 * gradient_alpha));
    ImU32 border_col = IM_COL32(80, 80, 80, 80);
    float border_alpha = ImClamp(state->alpha + 0.15f, 0.15f, 0.5f);

    draw->rounded_gradient_rect(window->DrawList, rect.Min, rect.Max, col_bg, col_bg, border_col, SCALE(elements->listbox.rounding), ImDrawFlags_RoundCornersAll);
    draw->rect(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->widget.stroke, border_alpha), SCALE(elements->listbox.rounding), ImDrawFlags_RoundCornersAll);

    if (draw_checkmark)
        draw->render_checkmark(window->DrawList, rect.Min, rect.Min + ImVec2(height, height), draw->get_clr(clr->base_colors.accent_clr, active ? 1.f : 0.f), SCALE(9), SCALE(1.5f));

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min + SCALE(state->offset, 0), rect.Max, draw->get_clr(state->text), label.data(), gui->text_end(label.data()), 0, { 0.0, 0.5 }, NULL);

    return pressed;
}
