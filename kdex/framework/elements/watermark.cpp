#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"
#include <ctime>

void c_gui::watermark(std::string name, std::vector<std::string> function, watermark_pos type, bool* visible)
{

    ImGuiStyle* style = &GetStyle();

    struct spectator_state
    {
        float alpha;
        ImVec2 content_size, current_pos, pos;
    };

    spectator_state* state = gui->anim_container(&state, ImGui::GetID(name.c_str()));
    gui->easing(state->alpha, *visible ? 1.f : 0.f, 4.f, static_easing);

    push_var(ImGuiStyleVar_Alpha, state->alpha);

    if (state->alpha >= 0.01f) {

        switch (type)
        {
        case top_left:
            state->pos = SCALE(elements->warermark.mark_setup_padding);
            break;
        case top_right:
            state->pos = ImVec2(ImGui::GetIO().DisplaySize.x - state->content_size.x, SCALE(elements->warermark.mark_setup_padding.y));
            break;
        case bottom_left:
            state->pos = ImVec2(SCALE(elements->warermark.mark_setup_padding.x), ImGui::GetIO().DisplaySize.y - state->content_size.y - SCALE(elements->warermark.mark_setup_padding.y) - SCALE(20));
            break;
        case bottom_right:
            state->pos = ImVec2(ImGui::GetIO().DisplaySize.x - state->content_size.x, GetIO().DisplaySize.y - state->content_size.y - SCALE(elements->warermark.mark_setup_padding.y) - SCALE(20));
            break;
        }

        state->current_pos = ImLerp(state->current_pos, state->pos, fixed_speed(25.f));

        gui->push_var(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        gui->set_next_window_pos(state->current_pos);

        gui->begin(xorstr("watermark"), nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        {
            const ImVec2 wnd_pos = gui->window_pos();
            const ImVec2 wnd_size = gui->window_size();
            ImDrawList* drawlist = GetWindowDrawList();

            ImGuiIO& io = GetIO();
            int fps_i = (int)(io.Framerate + 0.5f);
            int ms_i = fps_i > 0 ? (int)(1000.0f / (float)fps_i + 0.5f) : 0;

            time_t now = time(nullptr);
            tm ltm;
            localtime_s(&ltm, &now);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%I:%M %p", &ltm);

            std::string name_software = xorstr("KDEX | ");
            std::string name_bg = std::string(xorstr("[")) + std::string(var->auth.username) + xorstr("]");
            char stats[128];
            snprintf(stats, sizeof(stats), xorstr(" | %d ms / %s"), ms_i, time_str);

            ImVec2 sz1 = gui->text_size(var->font.instrument_medium[1], name_software.c_str());
            ImVec2 sz2 = gui->text_size(var->font.instrument_medium[1], name_bg.c_str());
            ImVec2 sz3 = gui->text_size(var->font.instrument_medium[1], stats);
            float total_width = sz1.x + sz2.x + sz3.x + SCALE(40);
            float total_height = SCALE(50);
            ImVec2 s = ImVec2(total_width, total_height);
            ImGui::Dummy(s);

            ImVec2 p = wnd_pos;

            ImU32 txt_gray_clr = draw->get_clr(clr->text.text_hovered);
            ImU32 txt_active_clr = draw->get_clr(clr->base_colors.accent_clr);
            ImU32 bg_clr = draw->get_clr(clr->window.window_layout);

            ImVec4 bg_vec = clr->window.window_layout;
            ImU32 bg_clr_110 = draw->get_clr(ImVec4(bg_vec.x, bg_vec.y, bg_vec.z, 110.f / 255.f));
            ImVec4 bg_dark_vec = ImVec4(bg_vec.x * 0.7f, bg_vec.y * 0.7f, bg_vec.z * 0.7f, 1.f);
            ImU32 bg_clr_dark = draw->get_clr(bg_dark_vec);

            draw->rect_filled_multi_color(drawlist,
                ImVec2(p.x + SCALE(10), p.y + SCALE(10)),
                ImVec2(p.x + total_width - SCALE(10), p.y + SCALE(37)),
                bg_clr_110, bg_clr_dark, bg_clr_110, bg_clr_dark, SCALE(10)
            );

            draw->rect_filled(drawlist,
                ImVec2(p.x + SCALE(14), p.y + SCALE(14)),
                ImVec2(p.x + total_width - SCALE(14), p.y + SCALE(33)),
                bg_clr, SCALE(8)
            );

            float current_x = p.x + SCALE(20);
            ImVec2 pos1 = ImVec2(current_x, p.y + SCALE(14));
            draw->text(drawlist, var->font.instrument_medium[1], var->font.instrument_medium[1]->FontSize, pos1, txt_gray_clr, name_software.c_str());
            current_x += sz1.x;

            ImVec2 pos2 = ImVec2(current_x, p.y + SCALE(14));
            draw->text(drawlist, var->font.instrument_medium[1], var->font.instrument_medium[1]->FontSize, pos2, txt_active_clr, name_bg.c_str());
            current_x += sz2.x;

            ImVec2 pos3 = ImVec2(current_x, p.y + SCALE(14));
            draw->text(drawlist, var->font.instrument_medium[1], var->font.instrument_medium[1]->FontSize, pos3, txt_gray_clr, stats);

            state->content_size = s;
            var->watermark.last_size = s;
        }
        gui->end();

        gui->pop_var(1);

    }
    pop_var();
}
