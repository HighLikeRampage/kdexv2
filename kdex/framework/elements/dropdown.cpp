#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"

static float calc_combo_size(int items_count, float item_size)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0) return FLT_MAX;

    return item_size * items_count + g.Style.ItemSpacing.y * (items_count - 1);
}

static const char* items_array_getter(void* data, int idx)
{
    const char* const* items = (const char* const*)data;
    return items[idx];
}

bool selectable_ex(std::string_view label, bool active, ImDrawFlags rounded_flag)
{
    struct c_selectable
    {
        float active_alpha = 0.f, hover_alpha = 0.f, offset = 10.f;
        ImVec4 text = clr->text.text_inactive;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(gui->content_avail().x, SCALE(elements->dropdown.selection_height)));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed) MarkItemEdited(id);

    c_selectable* state = gui->anim_container(&state, id);

    gui->easing(state->offset, active ? SCALE(35.f) : SCALE(10.f), 16.f, dynamic_easing, true);
    gui->easing(state->active_alpha, active ? 1.f : 0.f, 16.f, dynamic_easing);
    gui->easing(state->hover_alpha, hovered ? 0.35f : 0.05f, 16.f, dynamic_easing);
    gui->easing(state->text, (IsItemActive() || active) ? clr->text.text_active.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 12.f, dynamic_easing);

    ImVec4 accent = clr->base_colors.accent_clr.Value;
    float r_val = ImClamp(accent.x, 0.0f, 1.0f);
    float g_val = ImClamp(accent.y, 0.0f, 1.0f);
    float b_val = ImClamp(accent.z, 0.0f, 1.0f);
    float gradient_alpha = state->hover_alpha * 0.7f;
    ImU32 col_bg = IM_COL32((int)(r_val * 255), (int)(g_val * 255), (int)(b_val * 255), (int)(255 * gradient_alpha));
    ImU32 border_col = IM_COL32(80, 80, 80, 80);

    draw->rounded_gradient_rect(window->DrawList, rect.Min, rect.Max, col_bg, col_bg, border_col, SCALE(elements->button.rounding), rounded_flag);

    if (state->active_alpha > 0.01f)
        draw->render_checkmark(window->DrawList, rect.Min, rect.Min + SCALE(elements->dropdown.selection_height, elements->dropdown.selection_height), draw->get_clr(clr->base_colors.accent_clr, state->active_alpha), SCALE(9), SCALE(1.5f));

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min + ImVec2(state->offset, 0), rect.Max, draw->get_clr(state->text), label.data(), NULL, NULL, { 0.0, 0.5 }, NULL);

    return pressed;
}

bool selectable(const char* label, bool* p_selected, ImDrawFlags rounded_flag)
{
    if (selectable_ex(label, *p_selected, rounded_flag))
    {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}

bool dropdown_list(std::string_view label, std::string_view preview_value, int val, const ImVec2& size, bool multi, int max_count)
{
    struct c_combo
    {
        ImVec4 text = clr->text.text_inactive;

        bool combo_opened = false, hovered = false;
        float alpha = 0.f, offset = 0.f, selection_size, rotation;
    };

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags();

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(label.data());

    if (val > max_count) val = max_count;
    const ImVec2 pos = window->DC.CursorPos;

    const float width = size.x <= 0 ? gui->content_avail().x : size.x;
    const float height = size.y <= 0 ? elements->dropdown.dropdown_size : size.y;

    const ImRect rect(pos, pos + ImVec2(width, (size.x <= 0.f || size.y <= 0.f) ? SCALE(elements->dropdown.dropdown_height) : height));
    ImRect dropdown;

    if (size.x <= 0.f || size.y <= 0.f)
        dropdown = ImRect(pos + SCALE(0, elements->dropdown.dropdown_height - (elements->dropdown.padding + height)), pos + ImVec2(width, SCALE(elements->dropdown.dropdown_height - elements->dropdown.padding)));
    else
        dropdown = ImRect(pos, pos + ImVec2(width, height));

    ItemSize(rect, 0.f);
    if (!ItemAdd(rect, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);

    c_combo* state = gui->anim_container(&state, id);
    if (hovered && g.IO.MouseClicked[0] || state->combo_opened && g.IO.MouseClicked[0] && !state->hovered) state->combo_opened = !state->combo_opened;

    gui->easing(state->offset, state->combo_opened ? 8.f : 0.f, 25.f, static_easing);
    gui->easing(state->alpha, state->combo_opened ? 1.f : 0.f, 4.f, static_easing);
    gui->easing(state->text, state->combo_opened ? clr->text.text_active.Value : hovered ? clr->text.text_hovered.Value : clr->text.text_inactive.Value, 8.f, dynamic_easing);
    gui->easing(state->rotation, state->combo_opened ? -90.f : 90.f, 8.f, dynamic_easing);
    gui->easing(state->selection_size, state->combo_opened ? SCALE(elements->dropdown.selection_height) + 0.1f : 0, 16.f, dynamic_easing);

    if (!IsRectVisible(rect.Min, rect.Max + ImVec2(0, 2)))
    {
        state->combo_opened = false;
        state->alpha = 0.f;
    }

    if (size.x <= 0.f || size.y <= 0.f)
    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min + SCALE(0, elements->dropdown.padding + 2), rect.Max, draw->get_clr(state->text), label.data(), gui->text_end(label.data()), 0, ImVec2(0.f, 0.f));

    {
        ImU32 col_bg = IM_COL32(255, 255, 255, (int)(255 * (state->combo_opened ? 0.08f : 0.05f)));
        ImU32 border_col = IM_COL32(80, 80, 80, 80);

        draw->rounded_gradient_rect(window->DrawList, dropdown.Min, dropdown.Max, col_bg, col_bg, border_col, SCALE(elements->dropdown.rounding));
    }

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], dropdown.Min + SCALE(elements->dropdown.text_padding, 0), dropdown.Max - SCALE(elements->dropdown.dropdown_size, 0), draw->get_clr(clr->text.text_active), preview_value.data(), NULL, NULL, ImVec2(0.f, 0.5f));

     draw->rotate_start(window->DrawList);
     {
         if (size.x <= 0.f || size.y <= 0.f)
             draw->text_clipped(window->DrawList, var->font.icons[0], dropdown.Max - SCALE(elements->dropdown.dropdown_size, elements->dropdown.dropdown_size), dropdown.Max, draw->get_clr(clr->text.text_inactive), ICON_BUG, gui->text_end(ICON_BUG), 0, {0.5f, 0.5f});
         else
             draw->text_clipped(window->DrawList, var->font.icons[0], dropdown.Max - ImVec2(size.y, size.y), dropdown.Max, draw->get_clr(clr->text.text_inactive), ICON_BUG, gui->text_end(ICON_BUG), 0, { 0.5f, 0.5f });
     }
     draw->rotate_end(window->DrawList, state->rotation);

    if (size.x <= 0.f || size.y <= 0.f) {
        float separator_y = rect.Min.y + SCALE(elements->dropdown.dropdown_height);
        float separator_width = (rect.Max.x - rect.Min.x) * 0.75f;
        float separator_start_x = rect.Min.x + (rect.Max.x - rect.Min.x - separator_width) / 2.0f;
        float separator_end_x = separator_start_x + separator_width;
        draw->line(window->DrawList, ImVec2(separator_start_x, separator_y), ImVec2(separator_end_x, separator_y), draw->get_clr(clr->window.separator), 1.f);
    }

    if (!state->combo_opened && state->alpha < 0.01f) return false;

    gui->push_var(ImGuiStyleVar_WindowBorderSize, elements->dropdown.dropdown_border_size);
    gui->push_var(ImGuiStyleVar_WindowRounding, SCALE(elements->dropdown.rounding));
    gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->dropdown.dropdown_padding));
    gui->push_var(ImGuiStyleVar_ItemSpacing, SCALE(elements->dropdown.dropdown_spacing));
    gui->push_var(ImGuiStyleVar_Alpha, state->alpha);

    gui->push_color(ImGuiCol_WindowBg, draw->get_clr(clr->widget.layout));
    gui->push_color(ImGuiCol_Border, draw->get_clr(clr->widget.stroke));

    ImGuiWindow* root_menu = window;
    while (root_menu->ParentWindow) root_menu = root_menu->ParentWindow;

    bool is_keybind_window = strstr(root_menu->Name, xorstr("keybind_window")) != nullptr;

    float offset_y = SCALE(0, state->offset).y;

    float space_below, space_above;
    if (is_keybind_window) {
        space_below = g.IO.DisplaySize.y - dropdown.GetBL().y - offset_y - SCALE(15.f);
        space_above = dropdown.GetTL().y - offset_y - SCALE(15.f);
    } else {
        space_below = (root_menu->Pos.y + root_menu->Size.y - SCALE(15.f)) - dropdown.GetBL().y - offset_y;
        space_above = dropdown.GetTL().y - offset_y - (root_menu->Pos.y + SCALE(15.f));
    }

    float item_h = SCALE(elements->dropdown.selection_height);
    float max_allowed_height = calc_combo_size(6, item_h);

    bool place_below = true;
    float max_space = space_below;

    if (space_below < calc_combo_size(3, item_h) && space_above > space_below) {
        place_below = false;
        max_space = space_above;
    }

    float final_max_height = ImMax(item_h, ImMin(max_allowed_height, max_space));
    float ideal_height = calc_combo_size(val, state->selection_size);

    if (ideal_height > final_max_height) {
        ideal_height = final_max_height;
    }

    ImVec2 popup_size = ImVec2(dropdown.GetWidth(), ideal_height);
    ImVec2 popup_pos_below = dropdown.GetBL() + ImVec2(0, offset_y);
    ImVec2 popup_pos_above = dropdown.GetTL() - ImVec2(0, popup_size.y + offset_y);

    ImVec2 final_popup_pos = place_below ? popup_pos_below : popup_pos_above;

    gui->set_next_window_pos(final_popup_pos);
    gui->set_next_window_size(popup_size);

    gui->begin((std::stringstream{} << label << xorstr("dropdown_window")).str(), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoScrollbar);
    {
        gui->begin_group();
        state->hovered = IsWindowHovered();

        elements->dropdown.open_popup = state->combo_opened;

        if (!multi)
            if (IsWindowHovered() && g.IO.MouseReleased[0]) state->combo_opened = false;
    }
    return true;
}

void end_combo()
{
    gui->end_group();

    gui->pop_color(2);
    gui->pop_var(5);

    gui->end();
}

bool combo(std::string_view label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items, int max_count, const ImVec2& size)
{
    ImGuiContext& g = *GImGui;

    const std::string* preview_value = nullptr;
    if (*current_item >= 0 && *current_item < static_cast<int>(items.size()))
        preview_value = &items[*current_item];

    if (!dropdown_list(label.data(), preview_value ? preview_value->c_str() : nullptr, static_cast<int>(items.size()), size, false, max_count == 0 ? items.size() : max_count)) return false;

    bool value_changed = false;
    for (int i = 0; i < static_cast<int>(items.size()); i++)
    {
        const std::string& item_text = items[i];
        PushID(i);
        const bool item_selected = (i == *current_item);

        ImDrawFlags flags = (i == 0) ? ImDrawFlags_RoundCornersTop : (i == items.size() - 1) ? ImDrawFlags_RoundCornersBottom : ImDrawFlags_RoundCornersNone;
        if (items.size() == 1) flags = ImDrawFlags_RoundCornersAll;

        if (selectable_ex(item_text.c_str(), item_selected, flags) && *current_item != i)
        {
            value_changed = true;
            *current_item = i;
        }

        if (item_selected) SetItemDefaultFocus();
        PopID();
    }

    end_combo();

    if (value_changed) MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool c_widgets::dropdown(std::string_view label, int* current_item, const std::vector<std::string>& items, int max_count, const ImVec2& size)
{
    search->register_label(label);
    const bool value_changed{ combo(label, current_item, items, -1, max_count == 0 ? items.size() : max_count, size) };
    return value_changed;
}

void c_widgets::multi_dropdown(std::string_view label, bool variable[], const std::vector<std::string>& labels, int max_count, const ImVec2& size)
{
    search->register_label(label);
    ImGuiContext& g = *GImGui;
    std::string preview = std::string(xorstr("Select"));

    for (auto i = 0, j = 0; i < labels.size(); i++)
    {
        if (variable[i])
        {
            if (j)
                preview += std::string(xorstr(", ")) + (std::string)labels[i];
            else
                preview = labels[i];

            j++;
        }
    }

    if (dropdown_list(label.data(), preview.c_str(), labels.size(), size, true, max_count == 0 ? labels.size() : max_count))
    {
        for (auto i = 0; i < labels.size(); i++) {
            ImDrawFlags flags = (i == 0) ? ImDrawFlags_RoundCornersTop : (i == labels.size() - 1) ? ImDrawFlags_RoundCornersBottom : ImDrawFlags_RoundCornersNone;
            if (labels.size() == 1) flags = ImDrawFlags_RoundCornersAll;
            selectable(labels[i].data(), &variable[i], flags);
        }

        end_combo();
    }

    preview = std::string(xorstr("-"));
}
