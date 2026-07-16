#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"
#include <cstring>

bool c_widgets::checkbox(std::string_view label, bool* callback, bool warning, int* key, int* mode, float extra_padding, float width)
{
    search->register_label(label);
    struct checkbox_state
    {
        float radius = 0.f, alpha = 0.f, warning_alpha = 0.f, padding = elements->checkbox.text_padding[0];
        float popup_alpha = 0.f, circle_offset = 0.f;

        float keybind_alpha = 0.f;
        bool pressed = false, hovered = false;
        bool initialized = false;

        ImVec4 gear = clr->text.text_inactive;
        ImVec4 text = clr->text.text_inactive;
        ImVec4 layout = clr->widget.layout;
        ImVec4 stroke = clr->widget.stroke;
        ImVec4 circle = clr->widget.checkbox_circle;
        ImVec4 circle_stroke = clr->widget.stroke;

        ImVec2 size, keybind_size;

        std::vector<std::string> mode = { std::string(xorstr("Toggle")), std::string(xorstr("Hold")) };
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const float content_width = width > 0.f ? width : gui->content_avail().x;

    const ImRect rect(pos + ImVec2(content_width - SCALE(38.f), SCALE(7.f)), pos + ImVec2(content_width, SCALE(28.f)));
    const ImRect clickable(pos, pos + ImVec2(content_width, SCALE(elements->checkbox.height_size)));

    const ImRect keybind(pos + ImVec2(content_width - SCALE(60.f), SCALE(10.f)), pos + ImVec2(content_width - SCALE(45.f), SCALE(25.f)));

    ItemSize(clickable, 0);
    if (!ItemAdd(clickable, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(clickable, id, &hovered, &held);
    if (pressed && !IsMouseHoveringRect(keybind.Min, keybind.Max, true)) *callback = !(*callback);

    checkbox_state* state = gui->anim_container(&state, id);

    if (key != nullptr && IsMouseHoveringRect(keybind.Min, keybind.Max, true) && g.IO.MouseClicked[0] || state->pressed && g.IO.MouseClicked[0] && !state->hovered && !elements->dropdown.open_popup) state->pressed = !state->pressed;

    gui->easing(state->circle_offset, *callback ? 8.0f : -8.0f, 25.f, dynamic_easing);
    gui->easing(state->layout, *callback ? clr->base_colors.accent_clr.Value : clr->widget.layout.Value, 12.f, dynamic_easing);
    gui->easing(state->stroke, *callback ? clr->base_colors.accent_clr.Value : ImVec4(1.0f, 1.0f, 1.0f, 0.16f), 12.f, dynamic_easing);
    gui->easing(state->circle, *callback ? clr->base_colors.accent_clr.Value : ImVec4(0.0f, 0.0f, 0.0f, 1.0f), 12.f, dynamic_easing);
    gui->easing(state->circle_stroke, *callback ? clr->base_colors.accent_clr.Value : ImVec4(1.0f, 1.0f, 1.0f, 0.20f), 12.f, dynamic_easing);

    gui->easing(state->alpha, *callback ? 0.1f : 0.f, 12.f, static_easing);
    gui->easing(state->text, *callback ? clr->text.text_active.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 12.f, dynamic_easing);

    gui->easing(state->warning_alpha, var->gui.show_status_func && warning ? 1.f : 0.f, 12.f, static_easing);
    gui->easing(state->padding, var->gui.show_status_func && warning ? elements->checkbox.text_padding[1] : elements->checkbox.text_padding[0], 12.f, dynamic_easing);

    gui->easing(state->popup_alpha, hovered ? 1.f : 0.f, 4.f, static_easing);
    gui->easing(state->keybind_alpha, state->pressed ? 1.f : 0.f, 6.f, static_easing);

    gui->easing(state->gear, IsMouseHoveringRect(keybind.Min, keybind.Max, true) || state->pressed ? clr->text.text_active.Value : clr->text.text_inactive.Value, 12.f, dynamic_easing);

    if (var->gui.show_status_func && warning && state->popup_alpha > 0.01f)
    {
        gui->push_color(ImGuiCol_WindowBg, draw->get_clr(clr->window.window_layout));
        gui->push_color(ImGuiCol_Border, draw->get_clr(clr->window.window_stroke));

        gui->push_var(ImGuiStyleVar_WindowBorderSize, 1.f);
        gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->checkbox.popup_padding));
        gui->push_var(ImGuiStyleVar_WindowRounding, SCALE(elements->checkbox.popup_rounding));
        gui->push_var(ImGuiStyleVar_Alpha, state->popup_alpha);

        gui->set_next_window_pos(ImClamp(g.LastItemData.Rect.GetBR() - ImVec2(width, 0), ImVec2(0, 0), g.IO.DisplaySize));

        gui->begin(std::string(label.data()) + std::to_string(1), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);
        {
            state->size = gui->window_size() + SCALE(style->WindowPadding);

            gui->render_text(GetWindowDrawList(), var->font.icons[1], draw->get_clr(clr->base_colors.warning_clr), ICON_BUG);
            gui->sameline(0, SCALE(15));
            gui->render_text(GetWindowDrawList(), var->font.instrument_medium[0], draw->get_clr(clr->text.text_active), xorstr("Warning, the function is not safe"));

        }
        gui->end();

        gui->pop_var(4);
        gui->pop_color(2);
    }

    if (state->keybind_alpha > 0.01f)
    {
        gui->push_color(ImGuiCol_WindowBg, draw->get_clr(clr->window.window_layout));
        gui->push_color(ImGuiCol_Border, draw->get_clr(clr->window.window_stroke));

        gui->push_var(ImGuiStyleVar_WindowBorderSize, 1.f);
        gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->color_edit.popup_padding));
        gui->push_var(ImGuiStyleVar_WindowRounding, SCALE(elements->color_edit.rounding));
        gui->push_var(ImGuiStyleVar_ItemSpacing, SCALE(elements->color_edit.popup_spacing));
        gui->push_var(ImGuiStyleVar_Alpha, state->keybind_alpha);

        gui->set_next_window_pos(ImClamp(g.LastItemData.Rect.GetBR(), ImVec2(0, 0), g.IO.DisplaySize - state->size));
        gui->set_next_window_size(ImVec2(SCALE(220), state->keybind_size.y));

        gui->begin((std::stringstream{} << label << xorstr("keybind_window")).str(), nullptr, ImGuiWindowFlags_NoDecoration);
        {
            state->hovered = IsWindowHovered();
            state->size = gui->window_size() + SCALE(style->WindowPadding);

            widgets->key_select(xorstr("Keybind"), key);

            if (mode != nullptr) {
                widgets->dropdown(xorstr("Mode"), mode, state->mode, 0, ImVec2(gui->content_avail().x, SCALE(35)));
            }

        }
        gui->end();

        gui->pop_var(5);
        gui->pop_color(2);
    }

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->widget.layout), SCALE(elements->checkbox.rounding));
    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->base_colors.accent_clr, state->alpha), SCALE(elements->checkbox.rounding));
    draw->rect(window->DrawList, rect.Min, rect.Max - ImVec2(1.f, 1.f), draw->get_clr(state->stroke), SCALE(elements->checkbox.rounding), 0, 1.2f);
    draw->circle_filled(window->DrawList, rect.GetCenter() + SCALE(state->circle_offset, 0), SCALE(elements->checkbox.circle_scale + 0.8f), draw->get_clr(state->circle), SCALE(100.f));
    draw->circle(window->DrawList, rect.GetCenter() + SCALE(state->circle_offset, 0), SCALE(elements->checkbox.circle_scale + 0.8f), draw->get_clr(state->circle_stroke), SCALE(100.f), 1.2f);

    const bool in_search = strstr(window->Name, xorstr("search_window")) != nullptr || strstr(window->Name, xorstr("search_results_window")) != nullptr;
    if (!in_search && key != nullptr)
        draw->text_clipped(window->DrawList, var->font.icons[0], keybind.Min, keybind.Max, draw->get_clr(state->gear), FA_KEYBOARD, 0, 0, { 0.5, 0.5 });
    if (!in_search)
        draw->text_clipped(window->DrawList, var->font.icons[1], clickable.Min, clickable.Max, draw->get_clr(clr->base_colors.warning_clr, state->warning_alpha), ICON_BUG, 0, 0, { 0.0, 0.5 });
    const float label_padding = in_search ? extra_padding : (state->padding + extra_padding);
    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], clickable.Min + SCALE(label_padding, 0), clickable.Max, draw->get_clr(state->text), label.data(), gui->text_end(label.data()), 0, { 0.0, 0.5 });

    float separator_y = clickable.Min.y + SCALE(elements->checkbox.height_size);
    float separator_width = (clickable.Max.x - clickable.Min.x) * 0.75f;
    float separator_start_x = clickable.Min.x + (clickable.Max.x - clickable.Min.x - separator_width) / 2.0f;
    float separator_end_x = separator_start_x + separator_width;
    draw->line(window->DrawList, ImVec2(separator_start_x, separator_y), ImVec2(separator_end_x, separator_y), draw->get_clr(clr->window.separator), 1.f);

    return pressed;
}
