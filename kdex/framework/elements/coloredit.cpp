#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"
#include <cstring>

void color_convert_rgb_to_hsv(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float K = 0.f;
    if (g < b)
    {
        ImSwap(g, b);
        K = -1.f;
    }
    if (r < g)
    {
        ImSwap(r, g);
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = ImFabs(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

void color_convert_hsv_to_rgb(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{
    if (s == 0.0f)
    {
        out_r = out_g = out_b = v;
        return;
    }

    h = ImFmod(h, 1.0f) / (60.0f / 360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out_r = v; out_g = t; out_b = p; break;
    case 1: out_r = q; out_g = v; out_b = p; break;
    case 2: out_r = p; out_g = v; out_b = t; break;
    case 3: out_r = p; out_g = q; out_b = v; break;
    case 4: out_r = t; out_g = p; out_b = v; break;
    case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}

static void color_edit_restore_h(const float* col, float* H)
{
    ImGuiContext& g = *GImGui;
    *H = g.ColorEditSavedHue;
}

static void color_edit_restore_hs(const float* col, float* H, float* S, float* V)
{
    ImGuiContext& g = *GImGui;

    if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
        *H = g.ColorEditSavedHue;

    if (*V == 0.0f)
        *S = g.ColorEditSavedSat;
}

float hue_bar(float col[4], float h, ImVec2 size, bool* value_changed) {

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();

    float w = SCALE(size.x);
    static float move;

    ImVec2 pos = window->DC.CursorPos;
    ImRect rect(pos, pos + ImVec2(w, SCALE(size.y)));

    const ImU32 col_hues[6 + 1]{ IM_COL32(255, 0, 0, IM_F32_TO_INT8_SAT(style.Alpha)), IM_COL32(255, 255, 0, IM_F32_TO_INT8_SAT(style.Alpha)), IM_COL32(0, 255, 0, IM_F32_TO_INT8_SAT(style.Alpha)), IM_COL32(0, 255, 255, IM_F32_TO_INT8_SAT(style.Alpha)), IM_COL32(0, 0, 255, IM_F32_TO_INT8_SAT(style.Alpha)), IM_COL32(255, 0, 255, IM_F32_TO_INT8_SAT(style.Alpha)), IM_COL32(255, 0, 0, IM_F32_TO_INT8_SAT(style.Alpha)) };    const ImU32 col_midgrey = IM_COL32(128, 128, 128, IM_F32_TO_INT8_SAT(style.Alpha));

    for (int i = 0; i < 6; ++i)
        draw->rect_filled_multi_color(window->DrawList, ImVec2(pos.x + i * (w / 6) - (i == 5 ? 1 : 0), rect.Min.y), ImVec2(pos.x + (i + 1) * (w / 6) + (i == 0 ? 1 : 0), rect.Max.y), col_hues[i], col_hues[i + 1], col_hues[i + 1], col_hues[i], SCALE(100.f), i == 0 ? ImDrawFlags_RoundCornersLeft : i == 5 ? ImDrawFlags_RoundCornersRight : ImDrawFlags_RoundCornersNone);

    move = ImLerp(move, h, gui->fixed_speed(25.f));
    draw->circle_filled(window->DrawList, { rect.Min.x + rect.GetHeight() / 2 + (w - rect.GetHeight()) * move, rect.GetCenter().y }, rect.GetHeight() / 2 + SCALE(elements->color_edit.circle_size), draw->get_clr(clr->base_colors.white_clr), SCALE(100.f));

    ImGui::InvisibleButton(xorstr("hue"), rect.GetSize());
    if (IsItemActive()) {
        h = ImSaturate((io.MousePos.x - pos.x) / (rect.GetWidth() - 1));
        *value_changed = true;
    }

    return h;
}

bool alpha_bar(float col[4], float* a, ImVec2 size, bool actived, bool* value_changed) {

    if (!actived) return false;

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();

    float w = SCALE(size.x);
    static float move;

    ImVec2 pos = window->DC.CursorPos;
    ImRect rect(pos, pos + ImVec2(w, SCALE(size.y)));

    move = ImLerp(move, *a, gui->fixed_speed(25.f));

    draw->rect_filled_multi_color(window->DrawList, rect.Min, rect.Max, ImColor{ col[0], col[1], col[2], 0.f }, ImColor{ col[0], col[1], col[2], GImGui->Style.Alpha }, ImColor{ col[0], col[1], col[2], GImGui->Style.Alpha }, ImColor{ col[0], col[1], col[2], 0.f }, SCALE(100.f));
    draw->circle_filled(window->DrawList, { rect.Min.x + rect.GetHeight() / 2 + (w - rect.GetHeight()) * move, rect.GetCenter().y }, rect.GetHeight() / 2 + SCALE(elements->color_edit.circle_size), draw->get_clr(clr->base_colors.white_clr), SCALE(100.f));

    ImGui::InvisibleButton(xorstr("alpha"), rect.GetSize());
    if (IsItemActive()) {
        *a = ImSaturate((io.MousePos.x - pos.x) / (rect.GetWidth() - 1));
        *value_changed = true;
    }
    return false;
}

void color(float col[4], float h, float* s, float* v, const ImVec2& size_arg, bool* value_changed) {

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();

    static ImVec2 move;

    ImVec2 pos = window->DC.CursorPos;
    ImRect rect(pos, pos + SCALE(size_arg));
    ImVec4 hue_color_f(1, 1, 1, style.Alpha); ImGui::ColorConvertHSVtoRGB(h, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);

    draw->rect_filled_multi_color(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->base_colors.white_clr), ColorConvertFloat4ToU32(hue_color_f), ColorConvertFloat4ToU32(hue_color_f), draw->get_clr(clr->base_colors.white_clr), SCALE(elements->color_edit.rounding));
    draw->rect_filled_multi_color(window->DrawList, rect.Min - SCALE(1, 1), rect.Max + SCALE(1, 1), 0, 0, draw->get_clr(clr->base_colors.black_clr), draw->get_clr(clr->base_colors.black_clr), SCALE(elements->color_edit.rounding));

    ImVec2 cursor_pos;
    cursor_pos.x = ImClamp(IM_ROUND(rect.Min.x + ImSaturate(*s) * rect.GetWidth()), rect.Min.x + SCALE(2), rect.Max.x - SCALE(2));
    cursor_pos.y = ImClamp(IM_ROUND(rect.Min.y + ImSaturate(1 - *v) * rect.GetHeight()), rect.Min.y + SCALE(2), rect.Max.y - SCALE(2));

    move = ImLerp(move, cursor_pos, gui->fixed_speed(25.f));

    draw->circle_filled(window->DrawList, move, SCALE(5), draw->get_clr(ImColor(col[0], col[1], col[2], 1.f)));
    draw->circle(window->DrawList, move, SCALE(5), ImColor(255, 255, 255, int(style.Alpha * 255)), 100.f, SCALE(2.f));

    ImGui::InvisibleButton(xorstr("sv"), rect.GetSize());
    if (IsItemActive())
    {
        *s = ImSaturate((io.MousePos.x - rect.Min.x) / (rect.GetWidth() - 1)) <= 0 ? 0.01f : ImSaturate((io.MousePos.x - rect.Min.x) / (rect.GetWidth() - 1));
        *v = (1.0f - ImSaturate((io.MousePos.y - rect.Min.y) / (rect.GetHeight() - 1))) <= 0 ? 0.01f : 1.0f - ImSaturate((io.MousePos.y - rect.Min.y) / (rect.GetHeight() - 1));

        *value_changed = true;
    }
}

bool c_widgets::color_picker(std::string_view label, float col[4], bool alpha)
{
    search->register_label(label);
    struct c_edit
    {
        ImVec4 text = clr->text.text_inactive;
        ImVec2 size;

        float alpha = 0.f;
        bool pressed = false, hovered = false;
    };

    ImGuiWindow* window = GetCurrentWindow();

    ImGuiContext& g_1 = *GImGui;
    const ImGuiStyle& style = g_1.Style;

    const ImGuiID id = window->GetID(label.data());
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos + ImVec2(gui->content_avail().x - SCALE(15.f), SCALE(10.f)), pos + ImVec2(gui->content_avail().x, SCALE(25.f)));
    const ImRect clickable(pos, pos + ImVec2(gui->content_avail().x, SCALE(elements->color_edit.height_size)));

    char buf[64];
    bool value_changed = false;
    float h = 1.f, s = 1.f, v = 1.f, r = col[0], g = col[1], b = col[2], a = col[3];

    ItemSize(clickable, 0);
    if (!ItemAdd(clickable, id)) return false;
    bool hovered, held, pressed = ButtonBehavior(clickable, id, &hovered, &held);

    c_edit* state = gui->anim_container(&state, id);
    if (hovered && g_1.IO.MouseClicked[0] || state->pressed && g_1.IO.MouseClicked[0] && !state->hovered) state->pressed = !state->pressed;

    gui->easing(state->alpha, state->pressed ? 1.f : 0.f, 6.f, static_easing);
    gui->easing(state->text, state->pressed ? clr->text.text_active.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 12.f, dynamic_easing);

    float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
    int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

    color_convert_rgb_to_hsv(r, g, b, h, s, v);
    color_edit_restore_hs(col, &h, &s, &v);

   draw->text_clipped(window->DrawList, var->font.instrument_medium[1], clickable.Min, clickable.Max, draw->get_clr(state->text), label.data(), NULL, NULL, ImVec2(0.0f, 0.5f));
   draw->circle_filled(window->DrawList, rect.GetCenter(), SCALE(6.f), draw->get_clr(ImVec4(col[0], col[1], col[2], col[3])), SCALE(100.f));

   float separator_y = clickable.Min.y + SCALE(elements->color_edit.height_size);
   float separator_width = (clickable.Max.x - clickable.Min.x) * 0.75f;
   float separator_start_x = clickable.Min.x + (clickable.Max.x - clickable.Min.x - separator_width) / 2.0f;
   float separator_end_x = separator_start_x + separator_width;
   draw->line(window->DrawList, ImVec2(separator_start_x, separator_y), ImVec2(separator_end_x, separator_y), draw->get_clr(clr->window.separator), 1.f);

    if (state->alpha > 0.01f)
    {
        gui->push_color(ImGuiCol_WindowBg, draw->get_clr(clr->window.window_layout));
        gui->push_color(ImGuiCol_Border, draw->get_clr(clr->window.window_stroke));

        gui->push_var(ImGuiStyleVar_WindowBorderSize, 1.f);
        gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->color_edit.popup_padding));
        gui->push_var(ImGuiStyleVar_WindowRounding, SCALE(elements->color_edit.rounding));
        gui->push_var(ImGuiStyleVar_ItemSpacing, SCALE(elements->color_edit.popup_spacing));
        gui->push_var(ImGuiStyleVar_Alpha, state->alpha );

        gui->set_next_window_pos(ImClamp(g_1.LastItemData.Rect.GetBR(), ImVec2(0, 0), g_1.IO.DisplaySize - state->size));

        gui->begin((std::stringstream{} << label << xorstr("coloredit_window")).str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_Tooltip);
        {
            state->hovered = IsWindowHovered();
            state->size = gui->window_size() + SCALE(style.WindowPadding);

            if (g_1.IO.MouseWheel != 0.0f) state->pressed = false;

            color(col, h, &s, &v, elements->color_edit.picker_window_size, &value_changed);

            h = hue_bar(col, h, elements->color_edit.hue_line_size, &value_changed);
            alpha_bar(col, &a, elements->color_edit.alpha_line_size, alpha, &value_changed);

            if (alpha)
                ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
            else
                ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));

            if (widgets->text_field(ICON_WRENCH, xorstr(""), xorstr("HEX"), buf, IM_ARRAYSIZE(buf), nullptr, SCALE(elements->color_edit.text_field_size), ImGuiInputTextFlags_CharsUppercase))
            {
                value_changed = true;
                char* p = buf;
                while (*p == '#' || ImCharIsBlankA(*p))
                    p++;
                i[0] = i[1] = i[2] = 0;
                i[3] = 0xFF;
                int ri = sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]);
                r = i[0] / 255.f;
                g = i[1] / 255.f;
                b = i[2] / 255.f;

                color_convert_rgb_to_hsv(r, g, b, h, s, v);
                IM_UNUSED(ri);

            }

            color_convert_rgb_to_hsv(f[0], f[1], f[2], f[0], f[1], f[2]);
            color_edit_restore_hs(col, &f[0], &f[1], &f[2]);
        }
        gui->end();

        gui->pop_var(5);
        gui->pop_color(2);

        color_convert_hsv_to_rgb(h, s, v, r, g, b);

        if (value_changed) {

            g_1.ColorEditSavedHue = h;
            g_1.ColorEditSavedSat = s;
            g_1.ColorEditSavedColor = ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0));

            col[0] = r;
            col[1] = g;
            col[2] = b;
            col[3] = a;
        }
    }

    return value_changed;
}

bool c_widgets::checkbox_with_picker(std::string_view label, bool* callback, float col[4], bool alpha, bool warning)
{
    search->register_label(label);
    struct checkbox_state
    {
        float radius = 0.f, alpha = 0.f, warning_alpha = 0.f, padding = elements->checkbox.text_padding[0];
        float circle_offset = 0.f;
        float popup_alpha = 0.f, warning_popup_alpha = 0.f;

        bool pressed = false, hovered = false;

        ImVec4 text = clr->text.text_inactive;
        ImVec4 layout = clr->widget.layout;
        ImVec4 stroke = clr->widget.stroke;
        ImVec4 circle = clr->widget.checkbox_circle;
        ImVec4 circle_stroke = clr->widget.stroke;

        ImVec2 size;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g1 = *GImGui;
    ImGuiStyle* style = &GetStyle();

    const ImGuiID id = window->GetID(label.data());
    const float width = gui->content_avail().x;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos + ImVec2(width - SCALE(38.f), SCALE(7.f)), pos + ImVec2(width, SCALE(28.f)));
    const ImRect picker(pos + ImVec2(width - SCALE(60.f), SCALE(10.f)), pos + ImVec2(width - SCALE(45), SCALE(25.f)));

    const ImRect clickable(pos, pos + ImVec2(width, SCALE(elements->checkbox.height_size)));

    char buf[64];
    bool value_changed = false;
    float h = 1.f, s = 1.f, v = 1.f, r = col[0], g = col[1], b = col[2], a = col[3];

    ItemSize(clickable, 0);
    if (!ItemAdd(clickable, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(!IsMouseHoveringRect(picker.Min, picker.Max, true) ? clickable : ImRect(), id, &hovered, &held);
    if (pressed && !IsMouseHoveringRect(picker.Min, picker.Max, true)) *callback = !(*callback);

    checkbox_state* state = gui->anim_container(&state, id);

    if (IsMouseHoveringRect(picker.Min, picker.Max, true) && g1.IO.MouseClicked[0] || state->pressed && g1.IO.MouseClicked[0] && !state->hovered) state->pressed = !state->pressed;

    gui->easing(state->circle_offset, *callback ? 8.0f : -8.0f, 25.f, dynamic_easing);
    gui->easing(state->layout, *callback ? clr->base_colors.accent_clr.Value : clr->widget.layout.Value, 12.f, dynamic_easing);
    gui->easing(state->stroke, *callback ? clr->base_colors.accent_clr.Value : ImVec4(1.0f, 1.0f, 1.0f, 0.16f), 12.f, dynamic_easing);
    gui->easing(state->circle, *callback ? clr->base_colors.accent_clr.Value : ImVec4(0.0f, 0.0f, 0.0f, 1.0f), 12.f, dynamic_easing);
    gui->easing(state->circle_stroke, *callback ? clr->base_colors.accent_clr.Value : ImVec4(1.0f, 1.0f, 1.0f, 0.20f), 12.f, dynamic_easing);

    gui->easing(state->alpha, *callback ? 0.1f : 0.f, 12.f, static_easing);
    gui->easing(state->text, *callback ? clr->text.text_active.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 12.f, dynamic_easing);

    gui->easing(state->warning_alpha, var->gui.show_status_func && warning ? 1.f : 0.f, 12.f, static_easing);
    gui->easing(state->padding, var->gui.show_status_func && warning ? elements->checkbox.text_padding[1] : elements->checkbox.text_padding[0], 12.f, dynamic_easing);

    gui->easing(state->popup_alpha, state->pressed ? 1.f : 0.f, 6.f, static_easing);
    gui->easing(state->warning_popup_alpha, var->gui.show_status_func && hovered ? 1.f : 0.f, 4.f, static_easing);

    if (warning && state->warning_popup_alpha > 0.01f)
    {
        gui->push_color(ImGuiCol_WindowBg, draw->get_clr(clr->window.window_layout));
        gui->push_color(ImGuiCol_Border, draw->get_clr(clr->window.window_stroke));

        gui->push_var(ImGuiStyleVar_WindowBorderSize, 1.f);
        gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->checkbox.popup_padding));
        gui->push_var(ImGuiStyleVar_WindowRounding, SCALE(elements->checkbox.popup_rounding));
        gui->push_var(ImGuiStyleVar_Alpha, state->warning_popup_alpha);

        gui->set_next_window_pos(ImClamp(g1.LastItemData.Rect.GetBR() - ImVec2(width, 0), ImVec2(0, 0), g1.IO.DisplaySize));

        gui->begin(std::string(label.data()) + std::to_string(1), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);
        {
            state->size = gui->content_avail() + SCALE(style->WindowPadding * 2);

            gui->render_text(GetWindowDrawList(), var->font.icons[1], draw->get_clr(clr->base_colors.warning_clr), ICON_BUG);
            gui->sameline(0, SCALE(15));
            gui->render_text(GetWindowDrawList(), var->font.instrument_medium[0], draw->get_clr(clr->text.text_active), xorstr("Warning, the function is not safe"));

        }
        gui->end();

        gui->pop_var(4);
        gui->pop_color(2);
    }

    float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
    int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

    color_convert_rgb_to_hsv(r, g, b, h, s, v);
    color_edit_restore_hs(col, &h, &s, &v);

    if (state->popup_alpha > 0.01f)
    {
        gui->push_color(ImGuiCol_WindowBg, draw->get_clr(clr->window.window_layout));
        gui->push_color(ImGuiCol_Border, draw->get_clr(clr->window.window_stroke));

        gui->push_var(ImGuiStyleVar_WindowBorderSize, 1.f);
        gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->color_edit.popup_padding));
        gui->push_var(ImGuiStyleVar_WindowRounding, SCALE(elements->color_edit.rounding));
        gui->push_var(ImGuiStyleVar_ItemSpacing, SCALE(elements->color_edit.popup_spacing));
        gui->push_var(ImGuiStyleVar_Alpha, state->popup_alpha);

        gui->set_next_window_pos(ImClamp(g1.LastItemData.Rect.GetBR(), ImVec2(0, 0), g1.IO.DisplaySize - state->size));

        gui->begin((std::stringstream{} << label << xorstr("coloredit_window")).str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_Tooltip);
        {
            state->hovered = IsWindowHovered();
            state->size = gui->window_size() + SCALE(style->WindowPadding);

            if (g1.IO.MouseWheel != 0.0f) state->pressed = false;

            color(col, h, &s, &v, elements->color_edit.picker_window_size, &value_changed);
            h = hue_bar(col, h, elements->color_edit.hue_line_size, &value_changed);
            alpha_bar(col, &a, elements->color_edit.alpha_line_size, alpha, &value_changed);

            if (alpha)
                ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
            else
                ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));

            if (widgets->text_field(ICON_WRENCH, xorstr(""), xorstr("HEX"), buf, IM_ARRAYSIZE(buf), nullptr, SCALE(elements->color_edit.text_field_size), ImGuiInputTextFlags_CharsUppercase))
            {
                value_changed = true;
                char* p = buf;
                while (*p == '#' || ImCharIsBlankA(*p))
                    p++;
                i[0] = i[1] = i[2] = 0;
                i[3] = 0xFF;
                int ri = sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]);
                r = i[0] / 255.f;
                g = i[1] / 255.f;
                b = i[2] / 255.f;
                color_convert_rgb_to_hsv(r, g, b, h, s, v);
                IM_UNUSED(ri);

            }

            color_convert_rgb_to_hsv(f[0], f[1], f[2], f[0], f[1], f[2]);
            color_edit_restore_hs(col, &f[0], &f[1], &f[2]);
        }
        gui->end();

        gui->pop_var(5);
        gui->pop_color(2);

        color_convert_hsv_to_rgb(h, s, v, r, g, b);

        if (value_changed) {

            g1.ColorEditSavedHue = h;
            g1.ColorEditSavedSat = s;
            g1.ColorEditSavedColor = ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0));

            col[0] = r;
            col[1] = g;
            col[2] = b;
            col[3] = a;
        }
    }

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->widget.layout), SCALE(elements->checkbox.rounding));
    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->base_colors.accent_clr, state->alpha), SCALE(elements->checkbox.rounding));
    draw->rect(window->DrawList, rect.Min, rect.Max - ImVec2(1.f, 1.f), draw->get_clr(state->stroke), SCALE(elements->checkbox.rounding), 0, 1.2f);
    draw->circle_filled(window->DrawList, rect.GetCenter() + SCALE(state->circle_offset, 0), SCALE(elements->checkbox.circle_scale + 0.8f), draw->get_clr(state->circle), SCALE(100.f));
    draw->circle(window->DrawList, rect.GetCenter() + SCALE(state->circle_offset, 0), SCALE(elements->checkbox.circle_scale + 0.8f), draw->get_clr(state->circle_stroke), SCALE(100.f), 1.2f);
    draw->circle_filled(window->DrawList, picker.GetCenter(), SCALE(6.f), draw->get_clr(ImVec4(col[0], col[1], col[2], col[3])), SCALE(100.f));

    const bool in_search = strstr(window->Name, xorstr("search_window")) != nullptr || strstr(window->Name, xorstr("search_results_window")) != nullptr;
    if (!in_search)
        draw->text_clipped(window->DrawList, var->font.icons[1], clickable.Min, clickable.Max, draw->get_clr(clr->base_colors.warning_clr, state->warning_alpha), ICON_BUG, 0, 0, { 0.0, 0.5 });
    const float label_padding = in_search ? 0.f : state->padding;
    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], clickable.Min + SCALE(label_padding, 0), clickable.Max, draw->get_clr(state->text), label.data(), gui->text_end(label.data()), 0, { 0.0, 0.5 });

    float separator_y = clickable.Min.y + SCALE(elements->checkbox.height_size);
    float separator_width = (clickable.Max.x - clickable.Min.x) * 0.75f;
    float separator_start_x = clickable.Min.x + (clickable.Max.x - clickable.Min.x - separator_width) / 2.0f;
    float separator_end_x = separator_start_x + separator_width;
    draw->line(window->DrawList, ImVec2(separator_start_x, separator_y), ImVec2(separator_end_x, separator_y), draw->get_clr(clr->window.separator), 1.f);

    return pressed;
}
