#include "../settings/functions.h"

bool c_widgets::button(std::string_view name, const ImVec2& size, bool separator)
{
    struct button_state
    {
        ImVec4 layout = clr->widget.layout;
        ImVec4 text = clr->base_colors.white_clr;

        float alpha = 0.f;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(name.data());

    const ImVec2 pos = window->DC.CursorPos + SCALE(0, elements->button.padding);
    const ImRect rect(pos, pos + ImVec2(size.x <= 0.f ? gui->content_avail().x : size.x, size.y <= 0.f ? SCALE(elements->button.button_size) : size.y));

    ItemSize(ImRect(rect.Min, rect.Max + SCALE(0, elements->button.padding * 2)), 0);
    if (!ItemAdd(ImRect(rect.Min, rect.Max + SCALE(0, elements->button.padding * 2)), id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);

    button_state* state = gui->anim_container(&state, id);
    gui->easing(state->layout, IsItemActive() ? clr->base_colors.accent_clr.Value : clr->widget.layout.Value, 16.f, dynamic_easing);
    gui->easing(state->text, IsItemActive() ? clr->base_colors.black_clr.Value : clr->base_colors.white_clr.Value, 16.f, dynamic_easing);
    gui->easing(state->alpha, hovered ? 0.35f : 0.05f, 16.f, dynamic_easing);

    ImVec4 accent = clr->base_colors.accent_clr.Value;
    float r_val = ImClamp(accent.x, 0.0f, 1.0f);
    float g_val = ImClamp(accent.y, 0.0f, 1.0f);
    float b_val = ImClamp(accent.z, 0.0f, 1.0f);

    float gradient_alpha = state->alpha * 0.7f;

    ImU32 col_top = IM_COL32((int)(r_val * 255), (int)(g_val * 255), (int)(b_val * 255), (int)(255 * gradient_alpha));
    ImU32 col_bot = IM_COL32((int)(r_val * 255), (int)(g_val * 255), (int)(b_val * 255), 0);
    ImU32 border_col = IM_COL32(80, 80, 80, 80);

    draw->rounded_gradient_rect(window->DrawList, rect.Min, rect.Max, col_top, col_bot, border_col, SCALE(elements->button.rounding));

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min, rect.Max, draw->get_clr(state->text), name.data(), NULL, NULL, { 0.5, 0.5 });

    if (separator) {
        float separator_y = rect.Min.y + (size.y <= 0.f ? SCALE(elements->button.button_size + elements->button.padding) : size.y + SCALE(elements->button.padding));
        float separator_width = (rect.Max.x - rect.Min.x) * 0.75f;
        float separator_start_x = rect.Min.x + (rect.Max.x - rect.Min.x - separator_width) / 2.0f;
        float separator_end_x = separator_start_x + separator_width;
        draw->line(window->DrawList, ImVec2(separator_start_x, separator_y), ImVec2(separator_end_x, separator_y), draw->get_clr(clr->window.separator), 1.f);
    }

    return pressed;
}

bool c_widgets::settings_button(std::string_view id_name, bool callback, const ImVec2& size)
{
    struct s_button_state
    {
        ImVec4 layout = clr->widget.layout;

        float rotation;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(id_name.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + size);

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);

    s_button_state* state = gui->anim_container(&state, id);
    gui->easing(state->layout, callback ? clr->base_colors.accent_clr.Value : clr->widget.layout.Value, 16.f, dynamic_easing);
    gui->easing(state->rotation, hovered ? 360.f : 0.f, 4.f, dynamic_easing);

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(state->layout), SCALE(elements->button.rounding + 1));
    draw->rect(window->DrawList, rect.Min, rect.Max, IM_COL32(80, 80, 80, 80), SCALE(elements->button.rounding));

    draw->rotate_start(window->DrawList);
    {
        draw->text_clipped(window->DrawList, var->font.icons[3], rect.Min, rect.Max, draw->get_clr(clr->text.text_active), ICON_WRENCH, NULL, NULL, { 0.5, 0.5 });
    }
    draw->rotate_end(window->DrawList, state->rotation, { 0, 0 });

    return pressed;
}

bool c_widgets::sidebar_toggle(std::string_view name, bool& minimized, float alpha_multiplier)
{
    struct toggle_state
    {
        float alpha = 0.f;
        ImVec4 layout = clr->widget.layout;
        ImVec4 text = clr->base_colors.white_clr;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiID id = window->GetID(name.data());

    ImVec2 pos = window->DC.CursorPos;

    float size = SCALE(16);
    ImRect rect(pos, pos + ImVec2(size, size));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed) minimized = !minimized;

    toggle_state* state = gui->anim_container(&state, id);

    if (alpha_multiplier > 0.1f) {
        gui->easing(state->alpha, hovered ? 1.f : 0.5f, 8.f, dynamic_easing);
        gui->easing(state->layout, hovered ? clr->base_colors.accent_clr.Value : clr->window.window_stroke.Value, 8.f, dynamic_easing);
    }
    gui->easing(state->text, clr->base_colors.white_clr.Value, 8.f, dynamic_easing);

    ImU32 bg_col = draw->get_clr(clr->window.window_layout, alpha_multiplier * 0.8f);
    ImU32 border_col = draw->get_clr(state->layout, alpha_multiplier);

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, bg_col, SCALE(elements->button.rounding));
    draw->rect(window->DrawList, rect.Min, rect.Max, border_col, SCALE(elements->button.rounding));

    ImU32 col = draw->get_clr(state->text, state->alpha * alpha_multiplier);
    const char* icon = minimized ? xorstr(">") : xorstr("<");

    ImVec2 text_size = gui->text_size(var->font.instrument_medium[1], icon);
    ImVec2 text_pos = ImVec2(
        rect.Min.x + (size - text_size.x) * 0.5f,
        rect.Min.y + (size - text_size.y) * 0.5f
    );

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], text_pos, text_pos + text_size, col, icon, NULL, NULL, { 0.0f, 0.0f });

    return pressed;
}
