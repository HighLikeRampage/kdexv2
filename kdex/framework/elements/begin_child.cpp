#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"

bool begin_child_ex(std::string_view name, std::string_view icon, ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    struct c_child
    {
        float state = 0;
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

    c_child* state = gui->anim_container(&state, id);

    ImVec2 size = size_arg;
    if (size.x <= 0) size.x = (gui->content_max().x - GetStyle().WindowPadding.x * 2) / 2;

    if (size.y <= 0) {
        size.y = state->state;
        if (size.y > 0) size.y += SCALE(12.f);
    } else {
        size.y -= SCALE(5.f);
    }

    gui->set_next_window_size(size);
    gui->set_next_window_pos(parent_window->DC.CursorPos + SCALE(0, elements->child.header_height));

    ImVec2 p_min = { floorf(parent_window->DC.CursorPos.x), floorf(parent_window->DC.CursorPos.y) };
    ImVec2 p_max = { floorf(p_min.x + size.x), floorf(p_min.y + size.y + SCALE(elements->child.header_height)) };

    draw->rect_filled(GetWindowDrawList(), p_min, p_max, draw->get_clr(clr->child.child_layout, 0.1f), SCALE(elements->child.rounding));

    ImU32 bg_col_top = draw->get_clr(clr->base_colors.accent_clr, 0.15f);
    ImU32 bg_col_bot = draw->get_clr(clr->base_colors.accent_clr, 0.01f);
    draw->rect(GetWindowDrawList(), p_min, p_max - ImVec2(1, 1), draw->get_clr(clr->child.child_stroke), SCALE(elements->child.rounding), 0, 1.0f);

    draw->text_clipped(GetWindowDrawList(), var->font.instrument_medium[0], p_min + ImVec2(0, SCALE(2.f)), p_min + ImVec2(size.x, SCALE(elements->child.header_height)), draw->get_clr(clr->base_colors.accent_clr, 1.0f), name.data(), gui->text_end(name.data()), 0, {0.5, 0.5});

    const char* temp_window_name;

    if (name.data())
        ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);
    else
        ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);

    const bool ret = gui->begin(temp_window_name, NULL, window_flags | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;

    if (child_window->BeginCount == 1) parent_window->DC.CursorPos = child_window->Pos;

    const ImGuiID temp_id_for_activation = ImHashStr(xorstr("##Child"), 0, id);
    if (g.ActiveId == temp_id_for_activation) ClearActiveID();

    state->state = child_window->ContentSize.y;

    if (g.NavActivateId == id && !(window_flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(temp_id_for_activation, child_window);
        g.ActiveIdSource = g.NavInputSource;
    }
    return ret;
}

bool c_gui::begin_child(std::string_view name, std::string_view icon, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    ImGuiID id = GetCurrentWindow()->GetID(name.data());

    push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->child.child_padding));
    push_var(ImGuiStyleVar_ItemSpacing, SCALE(elements->child.child_spacing));
    ImGuiWindowFlags extra_flags = ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoMove;
    return begin_child_ex(name.data(), icon.data(), id, size_arg, child_flags, window_flags | extra_flags);

}

void c_gui::end_child()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* child_window = g.CurrentWindow;

    IM_ASSERT(g.WithinEndChild == false);
    IM_ASSERT(child_window->Flags & ImGuiWindowFlags_ChildWindow);

    g.WithinEndChild = true;
    ImVec2 child_size = child_window->Size;
    ImVec2 child_pos = child_window->Pos;
    float scroll_y = child_window->Scroll.y;
    float scroll_max_y = child_window->ScrollMax.y;

    pop_var(2);

    end();

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
