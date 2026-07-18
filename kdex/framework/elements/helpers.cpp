#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"
#include <cstdio>

void c_gui::push_color(ImGuiCol idx, ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorStack.push_back(backup);
    if (g.DebugFlashStyleColorIdx != idx)
        g.Style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void c_gui::pop_color(int count)
{
    ImGuiContext& g = *GImGui;
    if (g.ColorStack.Size < count)
    {
        IM_ASSERT_USER_ERROR(g.ColorStack.Size > count, xorstr("Calling PopStyleColor() too many times!"));
        count = g.ColorStack.Size;
    }
    while (count > 0)
    {
        ImGuiColorMod& backup = g.ColorStack.back();
        g.Style.Colors[backup.Col] = backup.BackupValue;
        g.ColorStack.pop_back();
        count--;
    }
}

void c_gui::push_var(ImGuiStyleVar idx, float val)
{
    ImGuiContext& g = *GImGui;
    const ImGuiDataVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type != ImGuiDataType_Float || var_info->Count != 1)
    {
        IM_ASSERT_USER_ERROR(0, xorstr("Calling PushStyleVar() variant with wrong type!"));
        return;
    }
    float* pvar = (float*)var_info->GetVarPtr(&g.Style);
    g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
    *pvar = val;
}

void c_gui::push_var(ImGuiStyleVar idx, const ImVec2& val)
{
    ImGuiContext& g = *GImGui;
    const ImGuiDataVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type != ImGuiDataType_Float || var_info->Count != 2)
    {
        IM_ASSERT_USER_ERROR(0, xorstr("Calling PushStyleVar() variant with wrong type!"));
        return;
    }
    ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
    g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
    *pvar = val;
}

void c_gui::pop_var(int count)
{
    ImGuiContext& g = *GImGui;
    if (g.StyleVarStack.Size < count)
    {
        IM_ASSERT_USER_ERROR(g.StyleVarStack.Size > count, xorstr("Calling PopStyleVar() too many times!"));
        count = g.StyleVarStack.Size;
    }
    while (count > 0)
    {
        ImGuiStyleMod& backup = g.StyleVarStack.back();
        const ImGuiDataVarInfo* info = GetStyleVarInfo(backup.VarIdx);
        void* data = info->GetVarPtr(&g.Style);
        if (info->Type == ImGuiDataType_Float && info->Count == 1) { ((float*)data)[0] = backup.BackupFloat[0]; }
        else if (info->Type == ImGuiDataType_Float && info->Count == 2) { ((float*)data)[0] = backup.BackupFloat[0]; ((float*)data)[1] = backup.BackupFloat[1]; }
        g.StyleVarStack.pop_back();
        count--;
    }
}

void c_gui::push_font(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    if (font == NULL)
        font = GetDefaultFont();
    g.FontStack.push_back(font);
    SetCurrentFont(font);
    g.CurrentWindow->DrawList->_SetTextureID(font->ContainerAtlas->TexID);
}

void c_gui::pop_font()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.FontStack.Size > 0);
    g.FontStack.pop_back();
    ImFont* font = g.FontStack.Size == 0 ? GetDefaultFont() : g.FontStack.back();
    SetCurrentFont(font);
    g.CurrentWindow->DrawList->_SetTextureID(font->ContainerAtlas->TexID);
}

void c_gui::set_pos(const ImVec2& pos, int type)
{
    ImGuiWindow* window = GetCurrentWindow();

    if (type == pos_all)
        window->DC.CursorPos = window->Pos - window->Scroll + pos;
    else if (type == pos_x)
        window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + pos.x;
    else if (type == pos_y)
        window->DC.CursorPos.y = window->Pos.y - window->Scroll.y + pos.y;

    window->DC.IsSetPos = true;
}

void c_gui::set_pos(float pos, int type)
{
    set_pos(ImVec2(pos, pos), type);
}

ImVec2 c_gui::get_pos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos - window->Pos + window->Scroll;
}

void c_gui::set_screen_pos(const ImVec2& pos, int type)
{
    ImGuiWindow* window = GetCurrentWindow();

    if (type == pos_all)
        window->DC.CursorPos = pos;
    else if (type == pos_x)
        window->DC.CursorPos.x = pos.x;
    else if (type == pos_y)
        window->DC.CursorPos.y = pos.y;

    window->DC.IsSetPos = true;
}

void c_gui::set_screen_pos(float pos, int type)
{
    set_screen_pos(ImVec2(pos, pos), type);
}

ImVec2 c_gui::get_screen_pos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos;
}

void c_gui::begin_group()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    g.GroupStack.resize(g.GroupStack.Size + 1);
    ImGuiGroupData& group_data = g.GroupStack.back();
    group_data.WindowID = window->ID;
    group_data.BackupCursorPos = window->DC.CursorPos;
    group_data.BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
    group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
    group_data.BackupIndent = window->DC.Indent;
    group_data.BackupGroupOffset = window->DC.GroupOffset;
    group_data.BackupCurrLineSize = window->DC.CurrLineSize;
    group_data.BackupCurrLineTextBaseOffset = window->DC.CurrLineTextBaseOffset;
    group_data.BackupActiveIdIsAlive = g.ActiveIdIsAlive;
    group_data.BackupHoveredIdIsAlive = g.HoveredId != 0;
    group_data.BackupIsSameLine = window->DC.IsSameLine;
    group_data.BackupActiveIdPreviousFrameIsAlive = g.ActiveIdPreviousFrameIsAlive;
    group_data.EmitItem = true;

    window->DC.GroupOffset.x = window->DC.CursorPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
    window->DC.Indent = window->DC.GroupOffset;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
    if (g.LogEnabled) g.LogLinePosY = -FLT_MAX;
}

void c_gui::end_group()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.GroupStack.Size > 0);

    ImGuiGroupData& group_data = g.GroupStack.back();
    IM_ASSERT(group_data.WindowID == window->ID);

    if (window->DC.IsSetPos) ErrorCheckUsingSetCursorPosToExtendParentBoundaries();

    ImRect group_bb(group_data.BackupCursorPos, ImMax(window->DC.CursorMaxPos, group_data.BackupCursorPos));

    window->DC.CursorPos = group_data.BackupCursorPos;
    window->DC.CursorPosPrevLine = group_data.BackupCursorPosPrevLine;
    window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, window->DC.CursorMaxPos);
    window->DC.Indent = group_data.BackupIndent;
    window->DC.GroupOffset = group_data.BackupGroupOffset;
    window->DC.CurrLineSize = group_data.BackupCurrLineSize;
    window->DC.CurrLineTextBaseOffset = group_data.BackupCurrLineTextBaseOffset;
    window->DC.IsSameLine = group_data.BackupIsSameLine;
    if (g.LogEnabled) g.LogLinePosY = -FLT_MAX;

    if (!group_data.EmitItem)
    {
        g.GroupStack.pop_back();
        return;
    }

    window->DC.CurrLineTextBaseOffset = ImMax(window->DC.PrevLineTextBaseOffset, group_data.BackupCurrLineTextBaseOffset);
    ImGui::ItemSize(group_bb.GetSize());
    ItemAdd(group_bb, 0, NULL, ImGuiItemFlags_NoTabStop);

    const bool group_contains_curr_active_id = (group_data.BackupActiveIdIsAlive != g.ActiveId) && (g.ActiveIdIsAlive == g.ActiveId) && g.ActiveId;
    const bool group_contains_prev_active_id = (group_data.BackupActiveIdPreviousFrameIsAlive == false) && (g.ActiveIdPreviousFrameIsAlive == true);
    if (group_contains_curr_active_id) g.LastItemData.ID = g.ActiveId;
    else if (group_contains_prev_active_id) g.LastItemData.ID = g.ActiveIdPreviousFrame;
    g.LastItemData.Rect = group_bb;

    const bool group_contains_curr_hovered_id = (group_data.BackupHoveredIdIsAlive == false) && g.HoveredId != 0;
    if (group_contains_curr_hovered_id) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;

    if (group_contains_curr_active_id && g.ActiveIdHasBeenEditedThisFrame)  g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Edited;

    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDeactivated;
    if (group_contains_prev_active_id && g.ActiveId != g.ActiveIdPreviousFrame) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Deactivated;

    g.GroupStack.pop_back();
    if (g.DebugShowGroupRects) window->DrawList->AddRect(group_bb.Min, group_bb.Max, IM_COL32(255, 0, 255, 255));
}

void c_gui::begin_content(std::string_view id, const ImVec2& size, const ImVec2& padding, const ImVec2& spacing, ImGuiWindowFlags flags)
{
    gui->push_var(ImGuiStyleVar_WindowPadding, padding);
    gui->begin_def_child(id, size, ImGuiChildFlags_None, flags | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    gui->push_var(ImGuiStyleVar_ItemSpacing, spacing);
}

void c_gui::end_content()
{
    gui->pop_var();
    gui->end_def_child();
    gui->pop_var();
}

bool c_gui::render_text(ImDrawList* draw_list, ImFont* font, ImU32 col, std::string_view text, const char* text_end)
{

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(text.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + gui->text_size(font, text.data()) );

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    draw->text(draw_list, font, font->FontSize, pos, col, text.data(), text_end);

    return false;
}

void c_gui::sameline(float offset_from_start_x, float spacing_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    if (offset_from_start_x != 0.0f)
    {
        if (spacing_w < 0.0f)
            spacing_w = 0.0f;
        window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + offset_from_start_x + spacing_w + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    else
    {
        if (spacing_w < 0.0f)
            spacing_w = g.Style.ItemSpacing.x;
        window->DC.CursorPos.x = window->DC.CursorPosPrevLine.x + spacing_w;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    window->DC.CurrLineSize = window->DC.PrevLineSize;
    window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
    window->DC.IsSameLine = true;
}

void c_gui::dummy(const ImVec2& size)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(size);
    ItemAdd(bb, 0);
}

bool begin_def_child_ex(const char* name, ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
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
    ImVec2 size = CalcItemSize(size_arg, size_default.x, size_default.y);
    size.y -= SCALE(2.f);

    gui->set_next_window_size(size);

    const char* temp_window_name;

    if (name)
        ImFormatStringToTempBuffer(&temp_window_name, NULL, xorstr("%s/%s_%08X"), parent_window->Name, name, id);
    else
        ImFormatStringToTempBuffer(&temp_window_name, NULL, xorstr("%s/%08X"), parent_window->Name, id);

    const bool ret = gui->begin(temp_window_name, NULL, window_flags | ImGuiWindowFlags_NoBackground);

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;

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

bool c_gui::begin_def_child(std::string_view name, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    ImGuiID id = GetCurrentWindow()->GetID(name.data());
    return begin_def_child_ex(name.data(), id, size_arg, child_flags, window_flags);
}

void c_gui::end_def_child()
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

    gui->end();

    if (scroll_max_y > 1.0f) {
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        float shadow_h = SCALE(30.f);
        ImU32 col_solid = IM_COL32(0, 0, 0, 100);
        ImU32 col_trans = IM_COL32(0, 0, 0, 0);

        ImVec2 menu_pos = gui->window_pos();
        ImVec2 menu_size = gui->window_size();
        float content_x_min = menu_pos.x;
        float content_x_max = menu_pos.x + menu_size.x;

        if (scroll_y > 1.0f) {
            ImVec2 t_min = ImVec2(content_x_min, menu_pos.y + 1.0f);
            ImVec2 t_max = ImVec2(content_x_max, menu_pos.y + shadow_h + 1.0f);

            float rounding = SCALE(var->window.window_rounding);
            draw->fade_rect_filled(draw_list, t_min, t_max, col_solid, col_trans,
                                   vertically, rounding,
                                   ImDrawFlags_RoundCornersTop);
        }

        if (scroll_y < scroll_max_y - 1.0f) {
            ImVec2 b_min = ImVec2(content_x_min, child_pos.y + child_size.y - shadow_h);
            ImVec2 b_max = ImVec2(content_x_max, child_pos.y + child_size.y);

            float rounding = SCALE(var->window.window_rounding);
            draw->fade_rect_filled(draw_list, b_min, b_max, col_trans, col_solid, vertically, rounding, ImDrawFlags_RoundCornersBottom);
        }
    }

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

void c_gui::set_next_window_pos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond));
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasPos;
    g.NextWindowData.PosVal = pos;
    g.NextWindowData.PosPivotVal = pivot;
    g.NextWindowData.PosCond = cond ? cond : ImGuiCond_Always;
}

void c_gui::set_next_window_size(const ImVec2& size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond));
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSize;
    g.NextWindowData.SizeVal = size;
    g.NextWindowData.SizeCond = cond ? cond : ImGuiCond_Always;
}

void c_gui::set_window_focus()
{
    FocusWindow(GImGui->CurrentWindow);
}

ImVec2 c_gui::text_size(ImFont* fontm, const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
{
    this->push_font(fontm);

    ImGuiContext& g = *GImGui;

    const char* text_display_end;
    if (hide_text_after_double_hash)
        text_display_end = FindRenderedTextEnd(text, text_end);
    else
        text_display_end = text_end;

    ImFont* font = g.Font;
    const float font_size = g.FontSize;
    if (text == text_display_end)
        return ImVec2(0.0f, font_size);
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);
    text_size.x = IM_TRUNC(text_size.x + 0.99999f);

    this->pop_font();

    return text_size;
}

ImVec2 c_gui::window_size()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size;
}

ImVec2 c_gui::window_pos()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    return window->Pos;
}

float c_gui::window_width()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.x;
}

float c_gui::window_height()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.y;
}

ImVec2 c_gui::content_avail()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 mx = (window->DC.CurrentColumns || g.CurrentTable) ? window->WorkRect.Max : window->ContentRegionRect.Max;
    return mx - window->DC.CursorPos;
}

ImVec2 c_gui::content_max()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 mx = (window->DC.CurrentColumns || g.CurrentTable) ? window->WorkRect.Max : window->ContentRegionRect.Max;
    return mx - window->Pos;
}

const char* c_gui::text_end(const char* text)
{
    const char* text_display_end = text;
    const char* text_end = text + strlen(text);

    while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
        text_display_end++;

    return text_display_end;
}

void c_gui::set_style()
{
    ImGuiStyle* style = &GetStyle();

    style->WindowBorderSize = SCALE(var->window.window_border_size);
    style->WindowRounding = SCALE(var->window.window_rounding);
    style->WindowPadding = SCALE(var->window.window_padding);

    style->ScrollbarSize = SCALE(var->window.scrollbar_size);
    style->ItemSpacing = SCALE(var->window.item_spacing);
}

void c_gui::draw_decorations(bool draw_logo)
{
    const ImVec2 pos = gui->window_pos();
    const ImVec2 size = gui->window_size();
    ImDrawList* drawlist = GetWindowDrawList();

    draw->rect_filled(drawlist, pos, pos + size, draw->get_clr(clr->window.window_layout), SCALE(var->window.window_rounding));

    if (var->window.top_left_glow_texture && var->window.bot_right_glow_texture) {
        ImU32 glow_color = draw->get_clr(clr->base_colors.accent_clr, 0.50f);
        draw->image_rounded(drawlist, var->window.top_left_glow_texture, pos, pos + size,
            ImVec2(0, 0), ImVec2(1, 1), glow_color, SCALE(var->window.window_rounding));
        draw->image_rounded(drawlist, var->window.bot_right_glow_texture, pos, pos + size,
            ImVec2(0, 0), ImVec2(1, 1), glow_color, SCALE(var->window.window_rounding));
    }

    draw->rect(drawlist, pos, pos + size, draw->get_clr(clr->window.window_stroke), SCALE(var->window.window_rounding));

    float bottom_bar_h = SCALE(42.f);

    float logo_alpha = 1.0f;
    if (var->auth.logo_fading) {
        double logo_elapsed = ImGui::GetTime() - var->auth.logo_fade_t0;
        logo_alpha = (float)ImClamp(logo_elapsed / 0.5, 0.0, 1.0);
        if (logo_alpha >= 1.0f) {
            var->auth.logo_fading = false;
        }
    } else if (!var->auth.fade_done_once) {
        logo_alpha = 0.0f;
    }

    if (draw_logo && var->window.logo_texture) {
        float logo_size = SCALE(38.f);
        float logo_x = SCALE(14.f);
        float logo_y = size.y - bottom_bar_h + (bottom_bar_h - logo_size) * 0.5f;

        ImVec4 ac = clr->base_colors.accent_clr;
        drawlist->AddImage(var->window.logo_texture,
            pos + ImVec2(logo_x, logo_y),
            pos + ImVec2(logo_x + logo_size, logo_y + logo_size),
            ImVec2(0, 0), ImVec2(1, 1), ImColor(ac.x, ac.y, ac.z, logo_alpha));
    }
    else if (draw_logo) {
        float logo_area_w = SCALE(60.f);
        ImVec2 logo_min = pos + ImVec2(SCALE(10.f), size.y - bottom_bar_h);
        ImVec2 logo_max = pos + ImVec2(SCALE(10.f) + logo_area_w, size.y);
        draw->text_clipped(drawlist, var->font.icons[2], logo_min, logo_max,
                           draw->get_clr(clr->base_colors.accent_clr, logo_alpha),
                           xorstr("M"), 0, 0, {0.5, 0.5});
    }

    if (draw_logo && var->auth.username[0]) {
        ImVec2 text_size = gui->text_size(var->font.instrument_medium[0], var->auth.username);
        float pad_r = SCALE(18.f);
        ImVec2 br_min = pos + ImVec2(size.x - pad_r - text_size.x - SCALE(80.f),
                                     size.y - bottom_bar_h);
        ImVec2 br_max = pos + ImVec2(size.x - pad_r, size.y);
        draw->text_clipped(drawlist, var->font.instrument_medium[0],
            br_min, br_max,
            draw->get_clr(clr->text.text_hovered, logo_alpha),
            var->auth.username, 0, 0, { 1.0f, 0.5f });
    }
}
