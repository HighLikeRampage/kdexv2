#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"

void c_notify::add_notify(std::string_view text, float delay, notify_type type)
{
    if (notifications.size() >= 3)
        notifications.erase(notifications.begin());

    notifications.push_back({ notify_count++, std::string(text), delay, type });
}

void c_notify::setup_notify()
{
    if (!var->notifications.enabled)
    {
        notifications.clear();
        return;
    }

    int cur_notify_value = 0;
    float accumulated_height = 0.f;

    for (auto& notification : notifications)
    {
        cur_notify_value++;
        if (notification.active_notify)
            notification.notify_timer += 1000.f / (notification.delay * 10);

        if (notification.notify_timer >= notify_time)
            notification.active_notify = false;

        gui->easing(notification.notify_alpha, notification.active_notify ? 1.f : 0.f, 4.f, static_easing);

        if (notification.notify_alpha > 0.f)
        {
            float target_position = accumulated_height + elements->notify.notify_setup_padding.y;
            gui->easing(notification.notify_pos, target_position, 8.f, dynamic_easing);

            ImVec2 window_size = render_notify(cur_notify_value, notification.notify_alpha, notification.notify_timer, notification.notify_pos, notification.text, notification.notify_type);

            accumulated_height += window_size.y + elements->notify.notify_setup_spacing.y;
        }
    }
}

ImVec2 c_notify::render_notify(int cur_notify_value, float notify_alpha, float notify_percentage, float notify_pos, std::string_view text, notify_type type)
{
    ImGuiStyle* style = &GetStyle();

    ImGuiIO& io = GetIO();

    const float pad_x = elements->notify.notify_setup_padding.x;
    const float pad_y = elements->notify.notify_setup_padding.y;

    float wm_offset = 0.f;
    if (var->watermark.watermark &&
        var->watermark.position == var->notifications.position)
    {
        wm_offset = var->watermark.last_size.y + elements->notify.notify_setup_spacing.y;
    }

    const char* label_text = "Notification: ";
    ImVec2 label_size = gui->text_size(var->font.instrument_medium[1], label_text);
    ImVec2 content_size = gui->text_size(var->font.instrument_medium[1], text.data());
    float total_width = label_size.x + content_size.x + SCALE(40);
    float total_height = SCALE(50);
    ImVec2 s = ImVec2(total_width, total_height);

    ImVec2 anchor;
    ImVec2 pivot;

    switch (static_cast<watermark_pos>(var->notifications.position))
    {
    case top_left:
        anchor = ImVec2(pad_x, pad_y + wm_offset + notify_pos);
        pivot = ImVec2(0.f, 0.f);
        break;
    case top_right:
        anchor = ImVec2(io.DisplaySize.x - pad_x, pad_y + wm_offset + notify_pos);
        pivot = ImVec2(1.f, 0.f);
        break;
    case bottom_left:
        anchor = ImVec2(pad_x, io.DisplaySize.y - pad_y - wm_offset - notify_pos);
        pivot = ImVec2(0.f, 1.f);
        break;
    case bottom_right:
    default:
        anchor = ImVec2(io.DisplaySize.x - pad_x, io.DisplaySize.y - pad_y - wm_offset - notify_pos);
        pivot = ImVec2(1.f, 1.f);
        break;
    }

    gui->push_var(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    gui->push_var(ImGuiStyleVar_Alpha, notify_alpha);
    gui->set_next_window_pos(anchor, 0, pivot);

    gui->begin(std::string(xorstr("notify")) + std::to_string(cur_notify_value), nullptr, elements->notify.notify_flags);
    {
        ImGui::Dummy(s);
        const ImVec2 pos = gui->window_pos();
        ImDrawList* drawlist = GetWindowDrawList();

        ImVec4 bg_vec = clr->window.window_layout;
        ImU32 bg_clr_110 = draw->get_clr(ImVec4(bg_vec.x, bg_vec.y, bg_vec.z, 110.f / 255.f));
        ImVec4 bg_dark_vec = ImVec4(bg_vec.x * 0.7f, bg_vec.y * 0.7f, bg_vec.z * 0.7f, 1.f);
        ImU32 bg_clr_dark = draw->get_clr(bg_dark_vec);
        ImU32 bg_clr = draw->get_clr(clr->window.window_layout);
        ImU32 txt_gray_clr = draw->get_clr(clr->text.text_hovered);
        ImU32 txt_accent_clr = draw->get_clr(clr->base_colors.accent_clr);

        draw->rect_filled_multi_color(drawlist,
            ImVec2(pos.x + SCALE(10), pos.y + SCALE(10)),
            ImVec2(pos.x + total_width - SCALE(10), pos.y + SCALE(37)),
            bg_clr_110, bg_clr_dark, bg_clr_110, bg_clr_dark, SCALE(10));

        draw->rect_filled(drawlist,
            ImVec2(pos.x + SCALE(14), pos.y + SCALE(14)),
            ImVec2(pos.x + total_width - SCALE(14), pos.y + SCALE(33)),
            bg_clr, SCALE(8));

        ImVec2 label_pos = ImVec2(pos.x + SCALE(20), pos.y + SCALE(14));
        draw->text(drawlist, var->font.instrument_medium[1], var->font.instrument_medium[1]->FontSize, label_pos, txt_accent_clr, label_text);

        ImVec2 content_pos = ImVec2(pos.x + SCALE(20) + label_size.x, pos.y + SCALE(14));
        draw->text(drawlist, var->font.instrument_medium[1], var->font.instrument_medium[1]->FontSize, content_pos, txt_gray_clr, text.data());
    }
    gui->end();
    gui->pop_var(2);

    return s;
}
