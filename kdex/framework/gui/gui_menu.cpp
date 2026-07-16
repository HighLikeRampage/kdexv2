#include "../Globals.hpp"
#include "../game/Core/Config.hpp"
#include "../game/Core/Core.hpp"
#include "../game/Security/xorstr.hpp"
#include "gui/gui_internal.hpp"
#include "settings/dashboard_sync.h"
#include "settings/functions.h"
#include "settings/options_config.h"
#include "settings/search.h"
#include <Security/Api/api.hpp>
#include <array>
#include <cstring>
#include <ctime>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>

void c_gui::render_menu_screen(const GuiFrameContext &ctx) {
  bool is_menu = ctx.is_menu;
  const ImVec2 &pos = ctx.pos;
  const ImVec2 &size = ctx.size;
  float menu_alpha = ctx.menu_alpha;
  using auth_screen = decltype(var->auth)::screen;
  ImDrawList *drawlist = ctx.drawlist;
  bool pushed_global_alpha = false;
  if (g_MenuInfo.IsOpen) {
    if (!var->auth.fade_done_once &&
        var->auth.current_screen == auth_screen::menu) {
      if (!var->auth.fading) {
        var->auth.fading = true;
        var->auth.fade_t0 = ImGui::GetTime();
      }
    }

    if (var->auth.fading) {
      double elapsed = ImGui::GetTime() - var->auth.fade_t0;
      float spinner_alpha = 1.0f;
      float menu_alpha = 0.0f;
      float bg_alpha = 1.0f;

      if (elapsed < 0.9) {
        spinner_alpha = 1.0f;
        menu_alpha = 0.0f;
        bg_alpha = 1.0f;
      } else {
        float raw_k = (float)ImClamp((elapsed - 0.9) / 0.45, 0.0, 1.0);
        float k = 1.f - powf(1.f - raw_k, 3.f);
        spinner_alpha = 1.0f - k;
        bg_alpha = 1.0f - k;
        menu_alpha = k;

        if (!var->auth.logo_fading) {
          var->auth.logo_fading = true;
          var->auth.logo_fade_t0 = ImGui::GetTime();
        }

        if (k >= 1.0f) {
          var->auth.fading = false;
          var->auth.fade_done_once = true;
          var->auth.first_open_done = true;
        }
      }

      if (bg_alpha > 0.0f) {
        gui->draw_decorations(false);
      }

      if (spinner_alpha > 0.0f) {
        ImVec2 sc = pos + ImVec2(size.x * 0.5f, size.y * 0.5f - SCALE(10.f));
        float t = (float)ImGui::GetTime();

        ImVec4 acc = clr->base_colors.accent_clr;
        ImVec4 w = ImVec4(1.f, 1.f, 1.f, 1.f);
        auto lerp4 = [](const ImVec4 &a, const ImVec4 &b, float k) {
          return ImVec4(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
                        a.z + (b.z - a.z) * k, 1.f);
        };
        ImU32 faint = draw->get_clr(acc, 0.15f * spinner_alpha);

        float A = SCALE(38.f);
        float th_max = SCALE(7.5f);
        float th_min = SCALE(2.2f);

        auto lemniscate = [A](float u) -> ImVec2 {
          float s = sinf(u), cs = cosf(u);
          float d = 1.f + s * s;
          return ImVec2(A * cs / d, A * s * cs / d);
        };

        {
          const int guide_pts = 96;
          for (int i = 0; i < guide_pts; i++) {
            float u = (float)i / guide_pts * 2.f * IM_PI;
            drawlist->PathLineTo(sc + lemniscate(u));
          }
          drawlist->PathStroke(faint, ImDrawFlags_Closed, SCALE(1.0f));
        }

        const float cycle = 2.4f;
        float phase = fmodf(t, cycle) / cycle;
        float head_u = phase * 2.f * IM_PI;
        float trail_arc = 1.55f * IM_PI;

        const int trail_pts = 96;
        ImVec2 tail_pos = sc + lemniscate(head_u - trail_arc);
        ImVec2 head_pos = sc + lemniscate(head_u);

        for (int i = 0; i < trail_pts; i++) {
          float k1 = (float)i / (float)trail_pts;
          float k2 = (float)(i + 1) / (float)trail_pts;
          float u1 = head_u - trail_arc * (1.f - k1);
          float u2 = head_u - trail_arc * (1.f - k2);
          ImVec2 p1 = sc + lemniscate(u1);
          ImVec2 p2 = sc + lemniscate(u2);

          float k = 0.5f * (k1 + k2);
          float alpha = powf(k, 0.75f);
          float thick = th_min + (th_max - th_min) * powf(k, 0.55f);
          ImU32 col = draw->get_clr(lerp4(acc, w, 0.08f * k),
                                    alpha * spinner_alpha);
          drawlist->AddLine(p1, p2, col, thick);
        }

        drawlist->AddCircleFilled(head_pos, th_max * 0.55f,
                                  draw->get_clr(acc, spinner_alpha), 20);
        drawlist->AddCircleFilled(head_pos, th_max * 0.95f,
                                  draw->get_clr(acc, 0.22f * spinner_alpha), 24);
        drawlist->AddCircleFilled(
            head_pos, th_max * 0.35f,
            draw->get_clr(lerp4(acc, w, 0.6f), spinner_alpha), 16);

        drawlist->AddCircleFilled(tail_pos, th_min * 0.55f,
                                  draw->get_clr(acc, 0.35f * spinner_alpha),
                                  12);
      }

      if (menu_alpha < 1.0f) {
        gui->push_var(ImGuiStyleVar_Alpha,
                      menu_alpha * ImGui::GetStyle().Alpha);
        pushed_global_alpha = true;
      }
    }

    static bool last_config_auto_loaded = false;
    if (!last_config_auto_loaded && var->auth.authenticated &&
        !var->auth.access_token.empty()) {
      last_config_auto_loaded = true;
      std::thread([token = var->auth.access_token]() {
        std::vector<Security::Api::ConfigEntry> list =
            Security::Api::config_list(token);
        if (!list.empty()) {
          const auto &latest = list[0];
          std::string name_out, code_out, data_out;
          if (Security::Api::config_get(token, latest.id, name_out, code_out,
                                        data_out)) {
            nlohmann::json parsed;
            std::string result =
                Core::g_Config.LoadCfg(name_out, data_out, &parsed);
            bool ok = result.find(xorstr("Error")) == std::string::npos;
            if (ok) {
              if (parsed.contains(xorstr("Options")))
                OptionsConfig::ApplyOptionsParamFromJson(
                    parsed[xorstr("Options")], &option->param);
              else if (parsed.contains(xorstr("Draw"))) {
                const auto &dr = parsed[xorstr("Draw")];
                if (dr.contains(xorstr("fov_circle")))
                  option->param.fov_circle = dr[xorstr("fov_circle")];
                if (dr.contains(xorstr("fov_size")))
                  option->param.fov_size = dr[xorstr("fov_size")];
                if (dr.contains(xorstr("fov_color")) &&
                    dr[xorstr("fov_color")].is_array() &&
                    dr[xorstr("fov_color")].size() >= 4)
                  for (int i = 0; i < 4; i++)
                    option->param.fov_color[i] = dr[xorstr("fov_color")][i];
                if (dr.contains(xorstr("silent_fov_circle")))
                  option->param.silent_fov_circle =
                      dr[xorstr("silent_fov_circle")];
                if (dr.contains(xorstr("silent_fov_size")))
                  option->param.silent_fov_size = dr[xorstr("silent_fov_size")];
                if (dr.contains(xorstr("silent_smart_fov")))
                  option->param.silent_smart_fov =
                      dr[xorstr("silent_smart_fov")];
                if (dr.contains(xorstr("silent_fov_near")))
                  option->param.silent_fov_near = dr[xorstr("silent_fov_near")];
                if (dr.contains(xorstr("silent_fov_far")))
                  option->param.silent_fov_far = dr[xorstr("silent_fov_far")];
                if (dr.contains(xorstr("silent_dual_fov")))
                  option->param.silent_dual_fov = dr[xorstr("silent_dual_fov")];
                if (dr.contains(xorstr("silent_dual_fov_distance")))
                  option->param.silent_dual_fov_distance =
                      dr[xorstr("silent_dual_fov_distance")];
                if (dr.contains(xorstr("silent_fov_color")) &&
                    dr[xorstr("silent_fov_color")].is_array() &&
                    dr[xorstr("silent_fov_color")].size() >= 4)
                  for (int i = 0; i < 4; i++)
                    option->param.silent_fov_color[i] =
                        dr[xorstr("silent_fov_color")][i];
                if (dr.contains(xorstr("triggerbot_fov_circle")))
                  option->param.triggerbot_fov_circle =
                      dr[xorstr("triggerbot_fov_circle")];
                if (dr.contains(xorstr("triggerbot_fov_size")))
                  option->param.triggerbot_fov_size =
                      dr[xorstr("triggerbot_fov_size")];
                if (dr.contains(xorstr("triggerbot_fov_color")) &&
                    dr[xorstr("triggerbot_fov_color")].is_array() &&
                    dr[xorstr("triggerbot_fov_color")].size() >= 4)
                  for (int i = 0; i < 4; i++)
                    option->param.triggerbot_fov_color[i] =
                        dr[xorstr("triggerbot_fov_color")][i];
                if (dr.contains(xorstr("crosshair_style")))
                  option->param.crosshair_style = dr[xorstr("crosshair_style")];
                if (dr.contains(xorstr("crosshair_size")))
                  option->param.crosshair_size = dr[xorstr("crosshair_size")];
                if (dr.contains(xorstr("crosshair_thickness")))
                  option->param.crosshair_thickness =
                      dr[xorstr("crosshair_thickness")];
                if (dr.contains(xorstr("crosshair_color")) &&
                    dr[xorstr("crosshair_color")].is_array() &&
                    dr[xorstr("crosshair_color")].size() >= 4)
                  for (int i = 0; i < 4; i++)
                    option->param.crosshair_color[i] =
                        dr[xorstr("crosshair_color")][i];
              }
            }
            if (ok)
              notify->add_notify(
                  std::string(xorstr("Config loaded with name: ")) + name_out,
                  2000, notify_type::success);
          }
        }
      }).detach();
    }

    if (var->auth.pending_remote_config_id > 0 && var->auth.authenticated &&
        !var->auth.access_token.empty()) {
      int id = var->auth.pending_remote_config_id;
      var->auth.pending_remote_config_id = 0;
      std::thread([id, token = var->auth.access_token]() {
        std::string name_out, code_out, data_out;
        if (Security::Api::config_get(token, id, name_out, code_out,
                                      data_out)) {
          nlohmann::json parsed;
          std::string result =
              Core::g_Config.LoadCfg(name_out, data_out, &parsed);
          bool ok = result.find(xorstr("Error")) == std::string::npos;
          if (ok) {
            if (parsed.contains(xorstr("Options")))
              OptionsConfig::ApplyOptionsParamFromJson(
                  parsed[xorstr("Options")], &option->param);
            else if (parsed.contains(xorstr("Draw"))) {
              const auto &dr = parsed[xorstr("Draw")];
              if (dr.contains(xorstr("fov_circle")))
                option->param.fov_circle = dr[xorstr("fov_circle")];
              if (dr.contains(xorstr("fov_size")))
                option->param.fov_size = dr[xorstr("fov_size")];
              if (dr.contains(xorstr("fov_color")) &&
                  dr[xorstr("fov_color")].is_array() &&
                  dr[xorstr("fov_color")].size() >= 4)
                for (int i = 0; i < 4; i++)
                  option->param.fov_color[i] = dr[xorstr("fov_color")][i];
              if (dr.contains(xorstr("silent_fov_circle")))
                option->param.silent_fov_circle =
                    dr[xorstr("silent_fov_circle")];
              if (dr.contains(xorstr("silent_fov_size")))
                option->param.silent_fov_size = dr[xorstr("silent_fov_size")];
              if (dr.contains(xorstr("silent_smart_fov")))
                option->param.silent_smart_fov = dr[xorstr("silent_smart_fov")];
              if (dr.contains(xorstr("silent_fov_near")))
                option->param.silent_fov_near = dr[xorstr("silent_fov_near")];
              if (dr.contains(xorstr("silent_fov_far")))
                option->param.silent_fov_far = dr[xorstr("silent_fov_far")];
              if (dr.contains(xorstr("silent_dual_fov")))
                option->param.silent_dual_fov = dr[xorstr("silent_dual_fov")];
              if (dr.contains(xorstr("silent_dual_fov_distance")))
                option->param.silent_dual_fov_distance =
                    dr[xorstr("silent_dual_fov_distance")];
              if (dr.contains(xorstr("silent_fov_color")) &&
                  dr[xorstr("silent_fov_color")].is_array() &&
                  dr[xorstr("silent_fov_color")].size() >= 4)
                for (int i = 0; i < 4; i++)
                  option->param.silent_fov_color[i] =
                      dr[xorstr("silent_fov_color")][i];
              if (dr.contains(xorstr("triggerbot_fov_circle")))
                option->param.triggerbot_fov_circle =
                    dr[xorstr("triggerbot_fov_circle")];
              if (dr.contains(xorstr("triggerbot_fov_size")))
                option->param.triggerbot_fov_size =
                    dr[xorstr("triggerbot_fov_size")];
              if (dr.contains(xorstr("triggerbot_fov_color")) &&
                  dr[xorstr("triggerbot_fov_color")].is_array() &&
                  dr[xorstr("triggerbot_fov_color")].size() >= 4)
                for (int i = 0; i < 4; i++)
                  option->param.triggerbot_fov_color[i] =
                      dr[xorstr("triggerbot_fov_color")][i];
              if (dr.contains(xorstr("crosshair_style")))
                option->param.crosshair_style = dr[xorstr("crosshair_style")];
              if (dr.contains(xorstr("crosshair_size")))
                option->param.crosshair_size = dr[xorstr("crosshair_size")];
              if (dr.contains(xorstr("crosshair_thickness")))
                option->param.crosshair_thickness =
                    dr[xorstr("crosshair_thickness")];
              if (dr.contains(xorstr("crosshair_color")) &&
                  dr[xorstr("crosshair_color")].is_array() &&
                  dr[xorstr("crosshair_color")].size() >= 4)
                for (int i = 0; i < 4; i++)
                  option->param.crosshair_color[i] =
                      dr[xorstr("crosshair_color")][i];
            }
          }
          if (ok)
            Core::g_Config.SetLastConfigId(id);
          notify->add_notify(
              ok ? xorstr("Config applied from dashboard") : result, 2000,
              ok ? notify_type::success : notify_type::error);
        }
      }).detach();
    }

    const float bottom_bar_height = SCALE(42.f);

    ImGui::SetCursorPos(ImVec2(0, 0));
    gui->begin_group();
    {

      bool show_header = false;
      if (show_header) {
        gui->begin_content(
            xorstr("header"), ImVec2(gui->content_avail().x, SCALE(32.f)),
            SCALE(10, 1), SCALE(10, 1),
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        {

          gui->push_var(ImGuiStyleVar_Alpha,
                        elements->section.section_alpha * GetStyle().Alpha);
          {
            gui->begin_group();
            {
              widgets->button(xorstr("Run"), SCALE(60, 30));
              gui->sameline();
              widgets->button(xorstr("Save"), SCALE(65, 30));
              gui->sameline();
              widgets->button(xorstr("Open in external editor"),
                              SCALE(135, 30));
            }
            gui->end_group();
          }
          gui->pop_var(1);
        }
        gui->end_content();
      }

      gui->easing(elements->section.section_alpha,
                  elements->section.section_count ==
                          elements->section.section_count_active
                      ? 1.f
                      : 0.f,
                  30.f, static_easing);
      gui->easing(elements->section.sub_section_alpha,
                  elements->section.sub_section_count ==
                          elements->section.sub_section_count_active
                      ? 1.f
                      : 0.f,
                  30.f, static_easing);
      if (elements->section.section_alpha == 0.f &&
          elements->section.section_add == 0.f)
        elements->section.section_count_active =
            elements->section.section_count;
      if (elements->section.sub_section_alpha == 0.f)
        elements->section.sub_section_count_active =
            elements->section.sub_section_count;

      gui->push_var(ImGuiStyleVar_Alpha,
                    elements->section.section_alpha *
                        elements->section.sub_section_alpha * GetStyle().Alpha);
      gui->begin_content(xorstr("content"),
                         ImVec2(gui->content_avail().x,
                                      gui->content_avail().y - bottom_bar_height),
                         SCALE(10, 10), SCALE(10, 8));
      {

        if (elements->section.section_count_active == 0) {
          float third_width = floorf((gui->content_avail().x - SCALE(20)) / 3);

          gui->begin_group();
          {
            gui->begin_child(xorstr("Aimbot"), ICON_FOCUS,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(option->name.enable_aimbot,
                                &option->param.enable_aimbot, false,
                                &option->key.enable_aimbot_key,
                                &option->key.enable_aimbot_mode);
              widgets->checkbox_with_picker(
                  option->name.fov_circle, &option->param.fov_circle,
                  option->param.fov_color, true, false);
              widgets->slider_float(option->name.fov_size,
                                    &option->param.fov_size, 0.f, 500.f,
                                    xorstr("%.0f"));
              widgets->dropdown(xorstr("Aimbot Hitbone"),
                                &option->param.hitbone,
                                option->item.hitbones);
              widgets->slider_int(option->name.aim_distance,
                                  &option->param.aim_distance, 0, 1000,
                                  xorstr("%d"));
              widgets->checkbox(option->name.ignore_npcs,
                                &option->param.ignore_npcs);
              widgets->checkbox(option->name.ignore_players,
                                &option->param.ignore_players);
              widgets->checkbox(option->name.check_visible,
                                &option->param.check_visible);
              widgets->checkbox(option->name.target_combat_roll,
                                &option->param.target_combat_roll);
              widgets->checkbox(option->name.target_jump,
                                &option->param.target_jump);
              widgets->checkbox(option->name.target_line,
                                &option->param.target_line);
              widgets->slider_float(option->name.smooth_horizontal,
                                    &option->param.smooth_horizontal, 0.f, 20.f,
                                    xorstr("%.1f"));
              widgets->slider_float(option->name.smooth_vertical,
                                    &option->param.smooth_vertical, 0.f, 20.f,
                                    xorstr("%.1f"));
              widgets->slider_int(option->name.aimbot_speed,
                                  &option->param.aimbot_speed, 1, 100,
                                  xorstr("%d"));
              widgets->checkbox(option->name.aim_curving,
                                &option->param.aim_curving);
              if (option->param.aim_curving)
                widgets->slider_float(option->name.curve_strength,
                                      &option->param.curve_strength, 0.f, 1.f,
                                      xorstr("%.2f"));
              widgets->checkbox(option->name.in_vehicle_aimbot,
                                &option->param.in_vehicle_aimbot);
              widgets->checkbox(option->name.use_prediction,
                                &option->param.use_prediction);
              if (option->param.use_prediction)
                widgets->slider_float(option->name.prediction_time,
                                      &option->param.prediction_time, 0.f, 1.f,
                                      xorstr("%.2fs"));
              widgets->slider_float(option->name.randomize_angle,
                                    &option->param.randomize_angle, 0.f, 10.f,
                                    xorstr("%.1f"));
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Silent Aim"), ICON_FOCUS,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(
                  option->name.silent_aim, &option->param.silent_aim, false,
                  &option->key.silent_aim_key, &option->key.silent_aim_mode);
              widgets->checkbox(option->name.magic_bullets,
                                &option->param.magic_bullets);
              widgets->checkbox_with_picker(
                  option->name.fov_circle, &option->param.silent_fov_circle,
                  option->param.silent_fov_color, true, false);
              widgets->slider_float(option->name.fov_size,
                                    &option->param.silent_fov_size, 0.f, 500.f,
                                    xorstr("%.0f"));
              widgets->checkbox(option->name.silent_smart_fov,
                                &option->param.silent_smart_fov);
              widgets->slider_float(option->name.silent_jitter,
                                    &option->param.silent_jitter, 0.f, 0.05f,
                                    xorstr("%.4f"));
              widgets->dropdown(xorstr("Silent Hitbone"),
                                &option->param.silent_hitbone, option->item.hitbones);
              widgets->checkbox(option->name.silent_dual_fov,
                                &option->param.silent_dual_fov);
              if (option->param.silent_dual_fov ||
                  option->param.silent_smart_fov) {
                widgets->slider_float(option->name.silent_fov_near,
                                      &option->param.silent_fov_near, 0.f,
                                      500.f, xorstr("%.0f"));
                widgets->slider_float(option->name.silent_fov_far,
                                      &option->param.silent_fov_far, 0.f, 800.f,
                                      xorstr("%.0f"));
                widgets->slider_int(option->name.silent_dual_fov_distance,
                                    &option->param.silent_dual_fov_distance, 0,
                                    2000, xorstr("%dm"));
              }
              widgets->slider_int(option->name.miss_chance,
                                  &option->param.miss_chance, 0, 100,
                                  xorstr("%d%%"));
              widgets->checkbox(option->name.ignore_npcs,
                                &option->param.silent_ignore_npcs);
              widgets->checkbox(option->name.ignore_players,
                                &option->param.silent_ignore_players);
              widgets->checkbox(option->name.check_visible,
                                &option->param.silent_check_visible);
              widgets->checkbox(option->name.target_combat_roll,
                                &option->param.silent_target_combat_roll);
              widgets->checkbox(option->name.target_jump,
                                &option->param.silent_target_jump);
              widgets->checkbox(option->name.silent_target_line,
                                &option->param.silent_target_line);
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Triggerbot"), ICON_FOCUS,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(option->name.enable_triggerbot,
                                &option->param.enable_triggerbot, false,
                                &option->key.triggerbot_key,
                                &option->key.triggerbot_mode);
              widgets->slider_int(option->name.delay_ms,
                                  &option->param.triggerbot_delay, 0, 1000,
                                  xorstr("%dms"));
              widgets->slider_int(option->name.hit_chance,
                                  &option->param.triggerbot_hit_chance, 0, 100,
                                  xorstr("%d%%"));
              widgets->dropdown(xorstr("Triggerbot Hitbone"),
                                &option->param.triggerbot_hitbone,
                                option->item.hitbones);
              widgets->checkbox(option->name.target_combat_roll,
                                &option->param.triggerbot_target_combat_roll);
              widgets->checkbox(option->name.target_jump,
                                &option->param.triggerbot_target_jump);
              widgets->checkbox(option->name.triggerbot_target_line,
                                &option->param.triggerbot_target_line);
              widgets->checkbox_with_picker(
                  option->name.triggerbot_fov_circle,
                  &option->param.triggerbot_fov_circle,
                  option->param.triggerbot_fov_color, true, false);
              widgets->slider_float(option->name.triggerbot_fov_size,
                                    &option->param.triggerbot_fov_size, 0.f,
                                    500.f, xorstr("%.0f"));
              widgets->checkbox(option->name.triggerbot_ignore_npcs,
                                &option->param.triggerbot_ignore_npcs);
              widgets->checkbox(option->name.triggerbot_ignore_friends,
                                &option->param.triggerbot_ignore_friends);
              widgets->checkbox(option->name.triggerbot_check_visible,
                                &option->param.triggerbot_check_visible);
            }
            gui->end_child();
          }
          gui->end_group();
        } else if (elements->section.section_count_active == 1) {
          float third_width = floorf((gui->content_avail().x - SCALE(20)) / 3);

          gui->begin_group();
          {
            gui->begin_child(xorstr("Players ESP"), ICON_GROUP,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(
                  option->name.enable_esp, &option->param.enable_esp, false,
                  &option->key.enable_esp_key, &option->key.enable_esp_mode);
              widgets->checkbox_with_picker(
                  option->name.team_check, &option->param.team_check,
                  option->param.team_check_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.visible_check, &option->param.visible_check,
                  option->param.visible_check_color, true, false);
              widgets->checkbox_with_picker(
                  xorstr("Admin Detection"), &option->param.admin_check_esp,
                  option->param.admin_check_esp_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.bounding_box, &option->param.bounding_box,
                  option->param.box_color, true, false);
              widgets->checkbox(option->name.health_bar,
                                &option->param.health_bar);
              widgets->checkbox(option->name.armor_bar,
                                &option->param.armor_bar);
              widgets->checkbox(option->name.radar, &option->param.radar);
              widgets->checkbox_with_picker(
                  option->name.player_chams, &option->param.player_chams,
                  option->param.player_chams_color, true, false);
              if (option->param.player_chams) {
                widgets->checkbox(option->name.player_chams_flicker,
                                  &option->param.player_chams_flicker);
              }
              widgets->checkbox_with_picker(
                  option->name.tint_players, &option->param.tint_players,
                  option->param.tint_players_color, true, false);
              widgets->checkbox_with_picker(option->name.override_sky_color,
                                            &option->param.override_sky_color,
                                            option->param.sky_color, true,
                                            false);
              if (option->param.override_sky_color) {
                widgets->checkbox(option->name.enable_sky_zenith_color,
                                  &option->param.enable_sky_zenith_color);
                if (option->param.enable_sky_zenith_color)
                  widgets->color_picker(option->name.sky_zenith_color,
                                        option->param.sky_zenith_color, true);
                widgets->checkbox(option->name.enable_sky_sun_color,
                                  &option->param.enable_sky_sun_color);
                if (option->param.enable_sky_sun_color)
                  widgets->color_picker(option->name.sky_sun_color,
                                        option->param.sky_sun_color, true);
                widgets->checkbox(option->name.enable_sky_sun_disc,
                                  &option->param.enable_sky_sun_disc);
                if (option->param.enable_sky_sun_disc) {
                  widgets->color_picker(option->name.sky_sun_disc_color,
                                        option->param.sky_sun_disc_color, true);
                  widgets->slider_float(option->name.sky_sun_disc_size,
                                        &option->param.sky_sun_disc_size, 0.0f,
                                        10.0f, xorstr("%.1f"));
                }
                widgets->checkbox(option->name.enable_sky_moon,
                                  &option->param.enable_sky_moon);
                if (option->param.enable_sky_moon) {
                  widgets->color_picker(option->name.sky_moon_color,
                                        option->param.sky_moon_color, true);
                  widgets->slider_float(option->name.sky_moon_disc_size,
                                        &option->param.sky_moon_disc_size, 0.0f,
                                        10.0f, xorstr("%.1f"));
                  widgets->slider_float(option->name.sky_moon_iten,
                                        &option->param.sky_moon_iten, 0.0f,
                                        10.0f, xorstr("%.1f"));
                }
                widgets->checkbox(option->name.enable_sky_stars,
                                  &option->param.enable_sky_stars);
                if (option->param.enable_sky_stars)
                  widgets->slider_float(option->name.sky_stars_iten,
                                        &option->param.sky_stars_iten, 0.0f,
                                        50.0f, xorstr("%.1f"));
                widgets->checkbox(option->name.enable_sky_clouds,
                                  &option->param.enable_sky_clouds);
                if (option->param.enable_sky_clouds) {
                  widgets->slider_float(option->name.sky_cloud_density_mult,
                                        &option->param.sky_cloud_density_mult,
                                        0.0f, 10.0f, xorstr("%.1f"));
                  widgets->color_picker(option->name.sky_cloud_mid_col,
                                        option->param.sky_cloud_mid_col, true);
                  widgets->color_picker(option->name.sky_cloud_base_col,
                                        option->param.sky_cloud_base_col, true);
                  widgets->slider_float(
                      option->name.sky_cloud_overall_strength,
                      &option->param.sky_cloud_overall_strength, 0.0f, 10.0f,
                      xorstr("%.1f"));
                }
                widgets->checkbox(option->name.enable_light_rays,
                                  &option->param.enable_light_rays);
                if (option->param.enable_light_rays) {
                  widgets->slider_float(option->name.light_ray_mult,
                                        &option->param.light_ray_mult, 0.0f,
                                        10.0f, xorstr("%.1f"));
                  widgets->color_picker(option->name.light_ray_col,
                                        option->param.light_ray_col, true);
                }
                widgets->checkbox(option->name.enable_fog_haze,
                                  &option->param.enable_fog_haze);
                if (option->param.enable_fog_haze) {
                  widgets->color_picker(option->name.fog_haze_col,
                                        option->param.fog_haze_col, true);
                  widgets->slider_float(option->name.fog_haze_density,
                                        &option->param.fog_haze_density, 0.0f,
                                        10.0f, xorstr("%.1f"));
                }
              }
              widgets->checkbox_with_picker(
                  option->name.skeleton, &option->param.skeleton,
                  option->param.skeleton_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.names, &option->param.names,
                  option->param.names_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.weapon_name, &option->param.weapon_name,
                  option->param.weapon_name_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.distance, &option->param.distance,
                  option->param.distance_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.snaplines, &option->param.snaplines,
                  option->param.snaplines_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.dotbones, &option->param.dotbones,
                  option->param.dotbones_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.tracer, &option->param.tracer,
                  option->param.tracer_color, true, false);
              widgets->checkbox_with_picker(
                  option->name.arrows, &option->param.arrows,
                  option->param.arrows_color, true, false);
              widgets->checkbox(option->name.coordinates,
                                &option->param.coordinates);
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Vehicles ESP"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(option->name.vehicle_esp,
                                &option->param.vehicle_esp);
              widgets->checkbox_with_picker(
                  option->name.vehicle_snaplines,
                  &option->param.vehicle_snaplines,
                  option->param.vehicle_snaplines_color, true, false);
              widgets->checkbox(option->name.vehicle_name,
                                &option->param.vehicle_name);
              widgets->checkbox(option->name.vehicle_distance,
                                &option->param.vehicle_distance);
            }
            gui->end_child();

            gui->begin_child(xorstr("Objects ESP"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(option->name.object_esp,
                                &option->param.object_esp);
              widgets->checkbox_with_picker(
                  option->name.object_name, &option->param.object_name,
                  option->param.object_name_color, true, false);
              widgets->checkbox(option->name.object_distance,
                                &option->param.object_distance);
            }
            gui->end_child();

            gui->begin_child(xorstr("Sizes"), ICON_FOCUS,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->slider_float(option->name.esp_max_distance,
                                    &option->param.esp_max_distance, 0.f,
                                    2000.f, xorstr("%.0fm"));
              widgets->slider_float(option->name.vehicle_max_distance,
                                    &option->param.vehicle_max_distance, 0.f,
                                    2000.f, xorstr("%.1fm"));
              widgets->slider_float(option->name.object_max_distance,
                                    &option->param.object_max_distance, 0.f,
                                    2000.f, xorstr("%.1fm"));
              widgets->slider_float(option->name.ui_font_size,
                                    &option->param.font_size, 8.0f, 24.0f,
                                    xorstr("%.1fpx"));
              widgets->slider_float(option->name.ui_icon_size,
                                    &option->param.icon_size, 0.1f, 3.0f,
                                    xorstr("%.2fx"));
              widgets->slider_float(option->name.arrows_size,
                                    &option->param.arrows_size, 6.0f, 24.0f,
                                    xorstr("%.0fpx"));
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Settings"), ICON_FOCUS,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(option->name.esp_ignore_npcs,
                                &option->param.esp_ignore_npcs);
              widgets->checkbox(option->name.esp_ignore_dead,
                                &option->param.esp_ignore_dead);
              widgets->multi_dropdown(xorstr("Dot Bones: Bones"),
                                      option->param.dotbones_select,
                                      option->item.dotbones_bones);
              widgets->dropdown(option->name.box_style,
                                &option->param.box_style,
                                option->item.box_styles);
              widgets->dropdown(option->name.health_bar_pos,
                                &option->param.health_bar_pos,
                                option->item.positions);
              widgets->dropdown(option->name.armor_bar_pos,
                                &option->param.armor_bar_pos,
                                option->item.positions);
              widgets->dropdown(option->name.names_pos,
                                &option->param.names_pos,
                                option->item.positions);
              widgets->dropdown(option->name.distance_pos,
                                &option->param.distance_pos,
                                option->item.positions);
              widgets->dropdown(option->name.weapon_name_pos,
                                &option->param.weapon_name_pos,
                                option->item.positions);

              if (option->param.tracer) {
                widgets->slider_float(option->name.tracer_duration,
                                      &option->param.tracer_duration, 0.1f,
                                      5.0f, xorstr("%.1fs"));
                widgets->slider_float(option->name.tracer_thickness,
                                      &option->param.tracer_thickness, 0.1f,
                                      5.0f, xorstr("%.1fpx"));
              }
            }
            gui->end_child();
          }
          gui->end_group();
        } else if (elements->section.section_count_active == 2) {
          float third_width = floorf((gui->content_avail().x - SCALE(20)) / 3);

          gui->begin_group();
          {
            gui->begin_child(xorstr("Movement Exploits"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_None);
            {
              widgets->checkbox(option->name.super_jump,
                                &option->param.super_jump);
              widgets->checkbox(option->name.beast_jump,
                                &option->param.beast_jump);
              widgets->checkbox(option->name.explosive_fist,
                                &option->param.explosive_fist);
              widgets->checkbox(option->name.fast_run, &option->param.fast_run);
              widgets->slider_float(option->name.run_speed,
                                    &option->param.run_speed, 1.0f, 10.0f,
                                    xorstr("%.1f m/s"));
              widgets->checkbox(option->name.fake_lag, &option->param.fake_lag,
                                false, &option->key.fake_lag_key,
                                &option->key.fake_lag_mode);
              if (option->param.fake_lag) {
                widgets->checkbox(option->name.always_lag,
                                  &option->param.always_lag);
              }
              widgets->slider_int(option->name.fake_lag_speed,
                                  &option->param.fake_lag_speed, 40, 1000,
                                  xorstr("%d ms"));
              widgets->checkbox(option->name.no_clip, &option->param.no_clip,
                                false, &option->key.noclip_key,
                                &option->key.noclip_mode);
              widgets->slider_float(option->name.noclip_speed,
                                    &option->param.noclip_speed, 0.5f, 50.0f,
                                    xorstr("%.1fx"));
              widgets->checkbox(option->name.spinbot, &option->param.spinbot);
              widgets->checkbox(xorstr("Freecam"),
                                &Core::g_Config.Player->FreeCam, false,
                                &Core::g_Config.Player->FreeCamKey);
              widgets->slider_float(xorstr("Freecam Speed"),
                                    &Core::g_Config.Player->FreeCamSpeed, 0.1f,
                                    20.0f, xorstr("%.1f"));
              if (Core::g_Config.Player->FreeCam) {
                widgets->checkbox(xorstr("Freecam Teleport"),
                                  &Core::g_Config.Player->FreeCamTeleport,
                                  false,
                                  &Core::g_Config.Player->FreeCamTeleportKey);
              }
              widgets->checkbox(option->name.strafe, &option->param.strafe,
                                false, &option->key.strafe_key,
                                &option->key.strafe_mode);
              widgets->slider_int(option->name.strafe_speed,
                                  &option->param.strafe_speed, 1, 500,
                                  xorstr("%d ms"));
              widgets->checkbox(option->name.invisible,
                                &option->param.invisible);
              if (option->param.invisible) {
                widgets->checkbox(option->name.invisible_spoof,
                                  &option->param.invisible_spoof);
              }
              widgets->checkbox(option->name.teleport_behind_enemy,
                                &option->param.teleport_behind_enemy, false,
                                &option->key.teleport_behind_enemy_key,
                                &option->key.teleport_behind_enemy_mode);
              widgets->slider_float(
                  option->name.teleport_behind_enemy_distance,
                  &option->param.teleport_behind_enemy_distance, 1.0f, 10.0f,
                  xorstr("%.2fm"));
              widgets->slider_float(option->name.teleport_behind_enemy_fov,
                                    &option->param.teleport_behind_enemy_fov,
                                    10.0f, 200.0f, xorstr("%.0f"));
              widgets->slider_int(option->name.teleport_behind_enemy_range,
                                  &option->param.teleport_behind_enemy_range,
                                  10, 500, xorstr("%dm"));
              widgets->checkbox(option->name.god_mode, &option->param.god_mode);
              widgets->checkbox(option->name.anti_afk, &option->param.anti_afk);
              widgets->slider_int(option->name.anti_afk_speed,
                                  &option->param.anti_afk_speed, 1, 60,
                                  xorstr("%d s"));
              widgets->checkbox(option->name.custom_fov,
                                &option->param.custom_fov);
              widgets->checkbox(option->name.infinite_stamina,
                                &option->param.infinite_stamina);
              widgets->checkbox(option->name.anti_headshot,
                                &option->param.anti_headshot);
              widgets->slider_float(option->name.max_health,
                                    &option->param.max_health, 0.f, 1000.f,
                                    xorstr("%.0f"));
              widgets->slider_float(option->name.max_armor,
                                    &option->param.max_armor, 0.f, 1000.f,
                                    xorstr("%.0f"));
              widgets->checkbox(option->name.inf_combat_roll,
                                &option->param.inf_combat_roll);
              widgets->checkbox(option->name.force_weapon_wheel,
                                &option->param.force_weapon_wheel);
              widgets->checkbox(option->name.shrink_enabled,
                                &option->param.shrink_enabled);
              widgets->checkbox(option->name.big_ped_enabled,
                                &option->param.big_ped_enabled);
              if (option->param.big_ped_enabled)
                widgets->slider_float(option->name.big_ped_scale,
                                      &option->param.big_ped_scale,
                                      1.1f, 10.f, xorstr("%.1f"));
              if (widgets->button(xorstr("Teleport to Waypoint"),
                                  ImVec2(gui->content_avail().x, SCALE(30)))) {
                option->param.teleport = true;
                notify->add_notify(xorstr("Teleported To Waypoint"), 2000,
                                   notify_type::success);
              }
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Weapon Exploits"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_None);
            {
              widgets->slider_float(option->name.recoil_value,
                                    &option->param.recoil_value, 0.0f, 2.0f,
                                    xorstr("%.2f"));
              widgets->slider_float(option->name.spread_value,
                                    &option->param.spread_value, 0.0f, 2.0f,
                                    xorstr("%.2f"));
              widgets->checkbox(option->name.no_reload,
                                &option->param.no_reload);
              widgets->checkbox(xorstr("TP to Bullet"),
                                &option->param.tp_to_bullet);
              widgets->checkbox(xorstr("Freeze Ammo"),
                                &option->param.freeze_ammo);
              widgets->slider_float(option->name.weapon_range,
                                    &option->param.weapon_range, 50.0f, 1000.0f,
                                    xorstr("%.0f"));
              widgets->checkbox(option->name.infinite_ammo,
                                &option->param.infinite_ammo);
              widgets->checkbox(option->name.damage_boost,
                                &option->param.damage_boost);
              widgets->slider_float(option->name.damage_boost_value,
                                    &option->param.damage_boost_value, 1.0f,
                                    100.0f, xorstr("x%.1f"));
              widgets->checkbox(option->name.explosive_ammo,
                                &option->param.explosive_ammo);
              widgets->checkbox(option->name.fire_ammo,
                                &option->param.fire_ammo);
              widgets->checkbox(option->name.weapon_size,
                                &option->param.weapon_size_enabled);
              widgets->slider_float(xorstr("Size Value"),
                                    &option->param.weapon_size_value, 1.0f,
                                    10.0f, xorstr("%.1f"));

              gui->dummy(SCALE(0, 6));

              static int selected_weapon_group = 0;
              static int selected_weapon_component = 0;
              static int prev_weapon_group = -1;
              static std::vector<std::string> weapon_group_names;
              static std::vector<std::string> weapon_component_names;
              if (weapon_group_names.empty()) {
                for (size_t i = 0; i < Core::Features::Exploits::g_WeaponGroupCount; ++i) {
                  weapon_group_names.emplace_back(Core::Features::Exploits::g_WeaponGroups[i].WeaponName);
                }
              }

              widgets->dropdown(xorstr("Weapon"), &selected_weapon_group,
                                weapon_group_names, 8);

              if (selected_weapon_group != prev_weapon_group) {
                selected_weapon_component = 0;
                prev_weapon_group = selected_weapon_group;
              }

              const auto* selected_group = &Core::Features::Exploits::g_WeaponGroups[selected_weapon_group];
              weapon_component_names.clear();
              for (size_t i = 0; i < selected_group->Count; ++i) {
                weapon_component_names.emplace_back(selected_group->Components[i].Name);
              }
              if (selected_weapon_component >= static_cast<int>(selected_group->Count))
                selected_weapon_component = 0;
              widgets->dropdown(xorstr("Attachments"), &selected_weapon_component,
                                weapon_component_names, 8);

              if (widgets->button(xorstr("Spawn Weapon"), ImVec2(gui->content_avail().x, SCALE(30)))) {
                const uint64_t ped = NativeCaller::g_NativeCaller.Invoke(Natives::PLAYER_PED_ID);
                if (ped) {
                  NativeCaller::g_NativeCaller.Invoke(Natives::GIVE_WEAPON_TO_PED, ped, selected_group->WeaponHash, 9999, false, true);
                  NativeCaller::g_NativeCaller.Invoke(Natives::SET_CURRENT_PED_WEAPON, ped, selected_group->WeaponHash, true);
                  notify->add_notify(xorstr("Weapon Spawned"), 2000, notify_type::success);
                }
              }

              gui->dummy(SCALE(0, 6));

              static std::atomic<bool> attachment_busy{false};

              if (widgets->button(xorstr("Give Attachment"), ImVec2(gui->content_avail().x, SCALE(30)))) {
                if (!attachment_busy.load()) {
                  attachment_busy.store(true);
                  uint32_t wh = selected_group->WeaponHash;
                  auto comp = selected_group->Components[selected_weapon_component];
                  std::thread([wh, comp]() {
                    Core::Features::Exploits::g_WeaponAttachments.GiveComponent(wh, comp);
                    attachment_busy.store(false);
                  }).detach();
                  notify->add_notify(xorstr("Giving Attachment..."), 2000, notify_type::success);
                }
              }

              if (widgets->button(xorstr("Remove Attachment"), ImVec2(gui->content_avail().x, SCALE(30)))) {
                if (!attachment_busy.load()) {
                  attachment_busy.store(true);
                  uint32_t wh = selected_group->WeaponHash;
                  auto comp = selected_group->Components[selected_weapon_component];
                  std::thread([wh, comp]() {
                    Core::Features::Exploits::g_WeaponAttachments.RemoveComponent(wh, comp);
                    attachment_busy.store(false);
                  }).detach();
                  notify->add_notify(xorstr("Removing Attachment..."), 2000, notify_type::success);
                }
              }

              gui->dummy(SCALE(0, 6));

              if (widgets->button(xorstr("Give All Attachments"), ImVec2(gui->content_avail().x, SCALE(30)))) {
                if (!attachment_busy.load()) {
                  attachment_busy.store(true);
                  auto grp = *selected_group;
                  std::thread([grp]() {
                    Core::Features::Exploits::g_WeaponAttachments.GiveAll(grp);
                    attachment_busy.store(false);
                  }).detach();
                  notify->add_notify(xorstr("Giving All Attachments..."), 2000, notify_type::success);
                }
              }

              if (widgets->button(xorstr("Remove All Attachments"), ImVec2(gui->content_avail().x, SCALE(30)))) {
                if (!attachment_busy.load()) {
                  attachment_busy.store(true);
                  auto grp = *selected_group;
                  std::thread([grp]() {
                    Core::Features::Exploits::g_WeaponAttachments.RemoveAll(grp);
                    attachment_busy.store(false);
                  }).detach();
                  notify->add_notify(xorstr("Removing All Attachments..."), 2000, notify_type::success);
                }
              }

              gui->dummy(SCALE(0, 6));

              if (widgets->button(xorstr("Give All Weapons"))) {
                option->param.give_all_weapons = true;
                notify->add_notify(xorstr("Gave All Weapons"), 2000,
                                   notify_type::success);
              }
            }
            gui->end_child();

            gui->begin_child(xorstr("Vehicle Exploits"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_None);
            {
              widgets->checkbox(option->name.vehicle_god_mode,
                                &option->param.vehicle_god_mode);
              widgets->checkbox(option->name.seat_belt,
                                &option->param.seat_belt);
              widgets->checkbox(option->name.steal_car,
                                &option->param.steal_car);
              widgets->checkbox(option->name.horn_boost,
                                &option->param.horn_boost_enabled, false,
                                &option->key.horn_boost_key,
                                &option->key.horn_boost_mode);
              widgets->checkbox(option->name.modify_vehicle_acceleration,
                                &option->param.modify_vehicle_acceleration);
              widgets->checkbox(option->name.modify_vehicle_traction,
                                &option->param.modify_vehicle_traction);
              widgets->checkbox(option->name.modify_gravity,
                                &option->param.modify_gravity);
              widgets->checkbox(option->name.vehicle_lock,
                                &option->param.vehicle_lock, false,
                                &option->key.vehicle_lock_key,
                                &option->key.vehicle_lock_mode);
              widgets->checkbox(option->name.bring_vehicle,
                                &option->param.bring_vehicle, false,
                                &option->key.bring_vehicle_key,
                                &option->key.bring_vehicle_mode);
              widgets->checkbox(option->name.warp_into_vehicle,
                                &option->param.warp_into_vehicle, false,
                                &option->key.warp_into_vehicle_key,
                                &option->key.warp_into_vehicle_mode);

              gui->dummy(SCALE(0, 8));
              widgets->text_field(ICON_WRENCH, xorstr(""),
                                  xorstr("Vehicle name"),
                                  option->param.vehicle_spawn_name, 64);
              widgets->checkbox(xorstr("Networked"), &option->param.vehicle_spawn_networked);
              if (widgets->button(xorstr("Spawn & Warp"), ImVec2(gui->content_avail().x, SCALE(30)))) {
                if (strlen(option->param.vehicle_spawn_name) > 0) {
                  Core::Features::Exploits::g_VehicleSpawner.SpawnByName(
                    option->param.vehicle_spawn_name, 0.f, true, option->param.vehicle_spawn_networked
                  );
                  notify->add_notify(xorstr("Vehicle spawned!"), 2000, notify_type::success);
                } else {
                  notify->add_notify(xorstr("Enter a vehicle name first!"), 2000, notify_type::error);
                }
              }

              gui->dummy(SCALE(0, 4));
              widgets->color_picker(option->name.primary_vehicle_color.c_str(),
                                    option->param.primary_vehicle_color, true);
              widgets->color_picker(
                  option->name.secondary_vehicle_color.c_str(),
                  option->param.secondary_vehicle_color, true);
              if (widgets->button(xorstr("Apply Color Change"),
                                  ImVec2(gui->content_avail().x, SCALE(30)))) {
                option->param.apply_color_change = true;
                notify->add_notify(xorstr("Colors Applied Successfully"), 2000,
                                   notify_type::success);
              }
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Ped Flags"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_None);
            {
              widgets->checkbox(option->name.anti_aim_block,
                                &option->param.anti_aim_block, false,
                                &option->key.anti_aim_block_key,
                                &option->key.anti_aim_block_mode);
              widgets->checkbox(std::string(xorstr("Kill Anti Aim")),
                                &Core::Config::Player::AntiBubbleAll);
              widgets->checkbox(option->name.disable_melee,
                                &option->param.disable_melee);
              widgets->checkbox(option->name.no_ragdoll_bullet,
                                &option->param.no_ragdoll_bullet);
              widgets->checkbox(option->name.no_ragdoll_explosion,
                                &option->param.no_ragdoll_explosion);
              widgets->checkbox(option->name.no_ragdoll_fire,
                                &option->param.no_ragdoll_fire);
              widgets->checkbox(option->name.no_ragdoll_vehicle,
                                &option->param.no_ragdoll_vehicle);
              widgets->checkbox(option->name.block_weapon_switch,
                                &option->param.block_weapon_switch);
              widgets->checkbox(option->name.no_collision,
                                &option->param.no_collision);
            }
            gui->end_child();

            gui->begin_child(xorstr("Vehicles Settings"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_None);
            {
              widgets->slider_float(option->name.horn_boost_speed,
                                    &option->param.horn_boost_speed, 1.0f,
                                    200.0f, xorstr("%.0f"));
              widgets->slider_float(option->name.vehicle_acceleration_value,
                                    &option->param.vehicle_acceleration_value,
                                    1.0f, 100.0f, xorstr("%.1f"));
              widgets->slider_float(option->name.vehicle_traction_value,
                                    &option->param.vehicle_traction_value, 1.0f,
                                    10.0f, xorstr("%.1f"));
              widgets->slider_float(option->name.gravity_value,
                                    &option->param.gravity_value, -50.0f, 50.0f,
                                    xorstr("%.1f"));
            }
            gui->end_child();

            gui->begin_child(xorstr("Bonus Exploits"), ICON_BUG,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_None);
            {
              widgets->slider_float(option->name.tp_all_vehicles_height,
                                    &option->param.tp_all_vehicles_height,
                                    -500.0f, 500.0f, xorstr("%.0fm"));

              std::string btnText = (option->param.tp_all_vehicles_height >= 0)
                                        ? xorstr("TP All Into Air")
                                        : xorstr("TP All Underground");
              if (widgets->button(btnText.c_str(),
                                  ImVec2(gui->content_avail().x, SCALE(30)))) {
                Core::Features::Exploits::TeleportAllVehiclesHeight(
                    option->param.tp_all_vehicles_height);
                notify->add_notify(
                    xorstr("Teleporting all vehicles Successfully"), 2000,
                    notify_type::success);
              }

              gui->dummy(SCALE(0, 6));

              widgets->slider_float(xorstr("Run Boosting Power"),
                                    &Core::Features::Exploits::g_RunBoosting.Power,
                                    1.0f, 100.0f, xorstr("%.1f"));

              static bool run_boosting_enabled = false;
              const bool run_boosting_prev = run_boosting_enabled;
              widgets->checkbox(xorstr("Run Boosting"),
                                &run_boosting_enabled);
              if (run_boosting_enabled != run_boosting_prev) {
                if (run_boosting_enabled) {
                  Core::Features::Exploits::g_RunBoosting.Start();
                } else {
                  Core::Features::Exploits::g_RunBoosting.Stop();
                }
              }

              gui->dummy(SCALE(0, 6));

              static bool outfit_changer_enabled = false;
              const bool outfit_changer_prev = outfit_changer_enabled;
              widgets->checkbox(xorstr("Outfit Changer"),
                                &outfit_changer_enabled);
              if (outfit_changer_enabled != outfit_changer_prev) {
                if (outfit_changer_enabled) {
                  Core::Features::Exploits::g_OutfitChanger.Start();
                } else {
                  Core::Features::Exploits::g_OutfitChanger.Stop();
                }
              }

              gui->dummy(SCALE(0, 6));

              static bool solo_session_enabled = false;
              static std::atomic<bool> solo_session_busy{false};
              const bool solo_session_prev = solo_session_enabled;
              widgets->checkbox(xorstr("Solo Session"),
                                &solo_session_enabled);
              if (solo_session_enabled != solo_session_prev && !solo_session_busy.load()) {
                solo_session_busy.store(true);
                bool enable = solo_session_enabled;
                std::thread([enable]() {
                  if (enable)
                    Core::Features::Exploits::StartSoloSession();
                  else
                    Core::Features::Exploits::StopSoloSession();
                  solo_session_busy.store(false);
                }).detach();
              }
            }
            gui->end_child();
          }
          gui->end_group();
        } else if (elements->section.section_count_active == 3) {
          float gap = SCALE(8);
          float third_width = (gui->content_avail().x - gap * 2) / 3;
          gui->begin_group();
          {
            if (widgets->button(xorstr("Explode All"),
                                ImVec2(third_width, SCALE(30)))) {
              Core::Features::Exploits::ExplodeAll();
            }
            gui->sameline(0, gap);
            if (widgets->button(xorstr("Kill All"),
                                ImVec2(third_width, SCALE(30)))) {
              Core::Features::Exploits::KillAll();
            }
            gui->sameline(0, gap);
            if (widgets->button(xorstr("Ragdoll All"),
                                ImVec2(third_width, SCALE(30)))) {
              Core::Features::Exploits::RagdollAll();
            }
          }
          gui->end_group();
          gui->dummy(SCALE(0, 4));
          float half_width = (gui->content_avail().x - SCALE(10)) / 2;
          float height = gui->content_avail().y - SCALE(40);

          static CPed *selected_ped = nullptr;

          static std::vector<Core::SDK::Game::EntityStruct>
              *player_list_copy_ptr = nullptr;
          if (!player_list_copy_ptr)
            player_list_copy_ptr =
                new std::vector<Core::SDK::Game::EntityStruct>();
          auto &player_list_copy = *player_list_copy_ptr;

          static int player_list_copy_tick = 0;
          if (++player_list_copy_tick >= 2) {
            player_list_copy_tick = 0;
            std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);

            CPed *prev_selected = selected_ped;

            player_list_copy = Core::SDK::Game::EntityList;

            std::sort(player_list_copy.begin(), player_list_copy.end(),
                      [](const Core::SDK::Game::EntityStruct &a,
                         const Core::SDK::Game::EntityStruct &b) {
                        return a.Distance < b.Distance;
                      });

            selected_ped = nullptr;
            if (prev_selected) {
              for (const auto &e : player_list_copy) {
                if (e.Ped == prev_selected) {
                  selected_ped = prev_selected;
                  break;
                }
              }
            }
            if (!selected_ped) {
              var->friends_tab.selected_player_id = 0;
              var->friends_tab.selected_player_ped = nullptr;
            }
            else {
              var->friends_tab.selected_player_ped = selected_ped;
            }
          }

          gui->begin_group();
          {
            gui->begin_child(xorstr("Players List"), ICON_GROUP,
                             ImVec2(half_width, height), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              gui->dummy(SCALE(0, 6));
              widgets->text_field(ICON_GROUP, xorstr(""),
                                  xorstr("Filter players..."),
                                  var->friends_tab.player_search, 128);

              gui->dummy(SCALE(0, 4));
              widgets->checkbox(option->name.playerlist_display_peds,
                                &option->param.playerlist_display_peds, false,
                                nullptr, nullptr, 10.f);

              float player_list_height =
                  gui->content_avail().y -
                  SCALE(elements->listbox.padding * 2.f + 6.f);
              if (player_list_height < SCALE(120.f))
                player_list_height = SCALE(120.f);

              widgets->begin_list(
                  xorstr("PLAYER_LIST"),
                  ImVec2(gui->content_avail().x, player_list_height));
              {
                int i = 0;
                int rendered_players = 0;
                for (const auto &entity : player_list_copy) {
                  if (entity.Ped == Core::SDK::Pointers::pLocalPlayer) {
                    i++;
                    continue;
                  }
                  if (!option->param.playerlist_display_peds &&
                      !entity.IsPlayer) {
                    i++;
                    continue;
                  }
                  std::string player_name = entity.NetworkInfo.UserName;
                  if (!entity.IsPlayer)
                    player_name = xorstr("NPC");

                  if (var->friends_tab.player_search[0] != '\0') {
                    std::string search_term = var->friends_tab.player_search;
                    std::string lower_name = player_name;
                    std::transform(lower_name.begin(), lower_name.end(),
                                   lower_name.begin(), ::tolower);
                    std::string lower_search = search_term;
                    std::transform(lower_search.begin(), lower_search.end(),
                                   lower_search.begin(), ::tolower);

                    if (lower_name.find(lower_search) == std::string::npos) {
                      i++;
                      continue;
                    }
                  }

                  std::string player_label =
                      player_name + xorstr("##") +
                      std::to_string((uintptr_t)entity.Ped);

                  bool is_active = (entity.Ped == selected_ped);
                  bool is_friend =
                      entity.IsFriend ||
                      (Core::SDK::Game::FriendMap.find(entity.Ped) !=
                           Core::SDK::Game::FriendMap.end() &&
                       Core::SDK::Game::FriendMap[entity.Ped]);
                  ImDrawFlags flags = (i == 0) ? ImDrawFlags_RoundCornersTop
                                      : (i == player_list_copy.size() - 1)
                                          ? ImDrawFlags_RoundCornersBottom
                                          : ImDrawFlags_RoundCornersNone;
                  if (player_list_copy.size() == 1)
                    flags = ImDrawFlags_RoundCornersAll;

                  if (is_friend) {
                    ImVec4 friend_col =
                        ImVec4(option->param.team_check_color[0],
                               option->param.team_check_color[1],
                               option->param.team_check_color[2],
                               option->param.team_check_color[3]);
                    if (widgets->list_content_col(player_label.c_str(),
                                                  is_active, friend_col, flags)) {
                      selected_ped = entity.Ped;
                      var->friends_tab.selected_player_id = entity.Id;
                      var->friends_tab.selected_player_ped = entity.Ped;
                    }
                  } else {
                    if (widgets->list_content(player_label.c_str(), is_active,
                                              flags)) {
                      selected_ped = entity.Ped;
                      var->friends_tab.selected_player_id = entity.Id;
                      var->friends_tab.selected_player_ped = entity.Ped;
                    }
                  }
                  rendered_players++;
                  i++;
                }

                if (rendered_players == 0) {
                  ImVec2 text_sz =
                      gui->text_size(var->font.instrument_medium[0],
                                     xorstr("Players Not Found"));
                  ImGui::SetCursorPos(
                      ImVec2((gui->content_avail().x - text_sz.x) * 0.5f,
                             (gui->content_avail().y - text_sz.y) * 0.5f));
                  ImGui::TextColored(clr->text.text_inactive,
                                     xorstr("Players Not Found"));
                }
              }
              widgets->end_list();
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          const Core::SDK::Game::EntityStruct *sel = nullptr;
          for (const auto &e : player_list_copy) {
            if (e.Ped == selected_ped) {
              sel = &e;
              break;
            }
          }

          gui->begin_group();
          {
            gui->begin_child(xorstr("Player Information"), ICON_GROUP,
                             ImVec2(half_width, height), ImGuiChildFlags_None,
                             ImGuiWindowFlags_None);
            {
              if (sel) {
                const auto &entity = *sel;
                std::string player_name = (!entity.IsPlayer)
                                              ? xorstr("NPC")
                                              : entity.NetworkInfo.UserName;
                char dist_buf[64];
                snprintf(dist_buf, sizeof(dist_buf), xorstr(" (%dm)"),
                         (int)entity.Distance);
                static CPed* cached_handle_ped = nullptr;
                static std::atomic<int> cached_ped_handle{0};
                static std::atomic<bool> handle_resolving{false};
                if (entity.Ped != cached_handle_ped) {
                  cached_handle_ped = entity.Ped;
                  cached_ped_handle.store(0);
                  if (!handle_resolving.exchange(true)) {
                    CPed* ped_copy = entity.Ped;
                    std::thread([ped_copy]() {
                      int h = Core::Features::Exploits::CallPointerToHandle((uintptr_t)ped_copy);
                      cached_ped_handle.store(h);
                      handle_resolving.store(false);
                    }).detach();
                  }
                }
                int ped_h = cached_ped_handle.load();
                std::string header =
                    player_name + " (" + std::to_string(entity.Id) +
                    ", Ped: " + (ped_h ? std::to_string(ped_h) : std::string("...")) + ")" +
                    dist_buf;
                gui->dummy(SCALE(0, 8));
                gui->render_text(
                    GetWindowDrawList(), var->font.instrument_medium[1],
                    draw->get_clr(clr->text.text_active), header.c_str());
                gui->dummy(SCALE(0, 4));

                std::string health_armor =
                    (std::string(xorstr("Health: ")) +
                     std::to_string((int)entity.Health) + xorstr(", Armor: ") +
                     std::to_string((int)entity.Armor));
                gui->render_text(
                    GetWindowDrawList(), var->font.instrument_medium[1],
                    draw->get_clr(clr->text.text_active), health_armor.c_str());
                gui->dummy(SCALE(0, 4));
                {
                  char coord_buf[128];
                  snprintf(coord_buf, sizeof(coord_buf),
                           xorstr("X: %.2f Y: %.2f Z: %.2f"), entity.Pos.x,
                           entity.Pos.y, entity.Pos.z);
                  gui->render_text(
                      GetWindowDrawList(), var->font.instrument_medium[1],
                      draw->get_clr(clr->text.text_active), coord_buf);
                }
                gui->dummy(SCALE(0, 4));
                if (entity.IsPlayer) {
                  std::string discord_name = entity.NetworkInfo.DiscordId;
                  if (discord_name.empty())
                    discord_name = xorstr("n/a");
                  std::string discord_text =
                      std::string(xorstr("Discord: ")) + discord_name;

                  ImVec2 text_sz = gui->text_size(
                      var->font.instrument_medium[1], discord_text.c_str());
                  ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                  bool hover = ImGui::IsMouseHoveringRect(cursor_pos,
                                                          cursor_pos + text_sz);
                  ImU32 text_clr =
                      hover ? draw->get_clr(clr->base_colors.accent_clr)
                            : draw->get_clr(clr->text.text_active);

                  if (hover && ImGui::IsMouseClicked(0) &&
                      !entity.NetworkInfo.DiscordId.empty()) {
                    ImGui::SetClipboardText(
                        entity.NetworkInfo.DiscordId.c_str());
                    notify->add_notify(xorstr("Discord copied to clipboard"),
                                       2000, notify_type::success);
                  }
                }

                gui->dummy(SCALE(0, 4));
                float gap = SCALE(8);
                float half_width = (gui->content_avail().x - gap) / 2;

                if (widgets->button(xorstr("Teleport"),
                                    ImVec2(half_width, SCALE(30)))) {
                  if (Core::SDK::Pointers::pLocalPlayer) {
                    D3DXVECTOR3 pos = entity.Pos;
                    ((CPed *)Core::SDK::Pointers::pLocalPlayer)->SetPos(pos);
                    notify->add_notify(
                        xorstr("Teleported to player Successfully"), 2000,
                        notify_type::success);
                  }
                }
                gui->sameline(0, gap);
                if (widgets->button(xorstr("Spawn Vehicles"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::SpawnVehiclesOnPlayer(entity.Ped,
                                                                  entity.Pos);
                }

                if (widgets->button(xorstr("Explode Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::ExplodePlayer(entity.Ped,
                                                          entity.Pos);
                }
                gui->sameline(0, gap);
                if (widgets->button(xorstr("Kill Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::KillPlayer(entity.Ped, entity.Pos);
                }

                if (widgets->button(xorstr("Taze Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::TazePlayer(entity.Ped, entity.Pos);
                }
                gui->sameline(0, gap);
                if (widgets->button(xorstr("Ragdoll Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::RagdollPlayer(entity.Ped,
                                                          entity.Pos);
                }

                if (widgets->button(xorstr("Bug Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::BugPlayer(entity.Ped, entity.Pos);
                }
                gui->sameline(0, gap);
                if (widgets->button(xorstr("Ran Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::RanPlayer(entity.Ped, entity.Pos);
                }

                if (widgets->button(xorstr("Crash Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  Core::Features::Exploits::CrashPlayer(entity.Ped, entity.Pos);
                }
                gui->sameline(0, gap);
                bool is_humping = Core::Features::Exploits::g_EatPlayerActive.load() &&
                                  Core::Features::Exploits::g_EatPlayerTargetPed.load() == (uintptr_t)entity.Ped;
                if (widgets->button(is_humping ? xorstr("Stop Hump") : xorstr("Hump Player"),
                                    ImVec2(half_width, SCALE(30)))) {
                  if (is_humping) {
                    Core::Features::Exploits::g_EatPlayerActive.store(false);
                  } else {
                    Core::Features::Exploits::g_EatPlayerActive.store(false);
                    Core::Features::Exploits::StartEatPlayerThread(entity.Id, (uint64_t)entity.Ped);
                  }
                }

                bool is_friend = entity.IsFriend ||
                                 (Core::SDK::Game::FriendMap.find(entity.Ped) !=
                                      Core::SDK::Game::FriendMap.end() &&
                                  Core::SDK::Game::FriendMap[entity.Ped]);
                if (widgets->button(is_friend ? xorstr("Remove Friend")
                                              : xorstr("Add Friend"),
                                    ImVec2(half_width, SCALE(30)))) {
                  bool newState = !is_friend;
                  Core::SDK::Game::FriendMap[entity.Ped] = newState;
                  notify->add_notify(newState ? xorstr("Added to friends")
                                              : xorstr("Removed from friends"),
                                     2000, notify_type::success);
                }
                gui->sameline(0, gap);

                std::string spectate_label =
                    (Core::g_Config.Player->SpectateEnabled &&
                     var->friends_tab.spectating_ped_id == entity.Id)
                        ? xorstr("Stop Spectating")
                        : xorstr("Spectate");
                if (widgets->button(spectate_label.c_str(),
                                    ImVec2(half_width, SCALE(30)))) {
                  if (Core::g_Config.Player->SpectateEnabled &&
                      var->friends_tab.spectating_ped_id == entity.Id) {
                    Core::Features::Exploits::SpectatePed(0, false);
                    var->friends_tab.spectating_ped_id = 0;
                    notify->add_notify(xorstr("Stopped spectating"), 2000,
                                       notify_type::success);
                  } else {
                    Core::Features::Exploits::SpectatePed(
                        reinterpret_cast<uint64_t>(entity.Ped), true);
                    var->friends_tab.spectating_ped_id = entity.Id;
                    notify->add_notify(entity.IsPlayer
                                           ? xorstr("Spectating player")
                                           : xorstr("Spectating NPC"),
                                       2000, notify_type::success);
                  }
                }
                if (widgets->button(xorstr("Copy Outfit"),
                                    ImVec2(gui->content_avail().x, SCALE(30)))) {
                  CPed *local_player = Core::SDK::Pointers::pLocalPlayer;
                  CPed *selected_ped_ptr = entity.Ped;
                  if (!local_player || !selected_ped_ptr) {
                    notify->add_notify(xorstr("Failed to copy outfit"), 2000,
                                       notify_type::error);
                  } else {
                    uintptr_t src_drawhandler = Core::Mem.Read<uintptr_t>(
                        reinterpret_cast<uintptr_t>(selected_ped_ptr) + 0x48);
                    uintptr_t dst_drawhandler = Core::Mem.Read<uintptr_t>(
                        reinterpret_cast<uintptr_t>(local_player) + 0x48);

                    if (!src_drawhandler || !dst_drawhandler) {
                      notify->add_notify(xorstr("Failed to copy outfit"), 2000,
                                         notify_type::error);
                    } else {
                      constexpr std::array<uintptr_t, 8> original_offsets = {
                          0x100, 0xF8, 0x114, 0x108, 0xEC, 0xF4, 0xFC, 0x10C};

                      for (uintptr_t off : original_offsets) {
                        uint64_t val =
                            Core::Mem.Read<uint64_t>(src_drawhandler + off);
                        Core::Mem.Write<uint64_t>(dst_drawhandler + off, val);
                      }

                      constexpr uintptr_t user_offsets[] = {0xF0, 0xF8, 0x100,
                                                            0x108, 0x110};

                      for (uintptr_t off : user_offsets) {
                        uint64_t val =
                            Core::Mem.Read<uint64_t>(src_drawhandler + off);
                        int tex =
                            Core::Mem.Read<int>(src_drawhandler + off + 0x8);

                        Core::Mem.Write<uint64_t>(dst_drawhandler + off, val);
                        Core::Mem.Write<int>(dst_drawhandler + off + 0x8, tex);
                      }

                      NativeCaller::g_NativeCaller.Invoke(Natives::_ASSIGN_PLAYER_TO_PED,
                                                 0x80000000, local_player);
                      NativeCaller::g_NativeCaller.Invoke(Natives::_ASSIGN_PLAYER_TO_PED,
                                                 0x80000001, local_player);
                      NativeCaller::g_NativeCaller.Invoke(Natives::_ASSIGN_PLAYER_TO_PED,
                                                 0x80000002, local_player);

                      Core::Mem.Write<uintptr_t>(
                          reinterpret_cast<uintptr_t>(local_player) + 0xAC, 1);

                      notify->add_notify(xorstr("Outfit copied"), 2000,
                                         notify_type::success);
                    }
                  }
                }
              } else {
                const ImVec2 ep_pos = GetWindowPos();
                const ImVec2 ep_size = GetWindowSize();
                const ImVec2 pad = SCALE(20.f, 20.f);
                ImVec2 pmin = ep_pos + pad;
                ImVec2 pmax = ep_pos + ep_size - pad;
                draw->text_clipped(GetWindowDrawList(),
                                   var->font.instrument_medium[0], pmin, pmax,
                                   draw->get_clr(clr->text.text_inactive),
                                   xorstr("Select a player"), NULL, NULL,
                                   ImVec2(0.5f, 0.5f), NULL);
              }
            }
            gui->end_child();
          }
          gui->end_group();
        } else if (elements->section.section_count_active == 4) {
          float half_width = (gui->content_avail().x - SCALE(10)) / 2;
          float height = gui->content_avail().y - SCALE(40);

          static CPed *selected_friend_ped = nullptr;

          static std::vector<Core::SDK::Game::EntityStruct>
              *friends_list_copy_ptr = nullptr;
          if (!friends_list_copy_ptr)
            friends_list_copy_ptr =
                new std::vector<Core::SDK::Game::EntityStruct>();
          auto &friends_list_copy = *friends_list_copy_ptr;

          static int friends_list_copy_tick = 0;
          if (++friends_list_copy_tick >= 2) {
            friends_list_copy_tick = 0;
            std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
            friends_list_copy = Core::SDK::Game::EntityList;
          }

          gui->begin_group();
          {
            gui->begin_child(xorstr("Friends List"), ICON_GROUP,
                             ImVec2(half_width, height), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              gui->dummy(SCALE(0, 6));
              if (!search->active_searcing) {
                widgets->text_field(ICON_GROUP, xorstr(""),
                                    xorstr("Filter friends..."),
                                    var->friends_tab.friend_search, 128);
              }

              gui->dummy(SCALE(0, 4));

              float friends_list_height =
                  gui->content_avail().y -
                  SCALE(elements->listbox.padding * 2.f + 6.f);
              if (friends_list_height < SCALE(120.f))
                friends_list_height = SCALE(120.f);

              widgets->begin_list(
                  xorstr("FRIENDS_LIST"),
                  ImVec2(gui->content_avail().x, friends_list_height));
              {
                int count = 0;
                for (const auto &entity : friends_list_copy) {
                  if (entity.Ped == Core::SDK::Pointers::pLocalPlayer)
                    continue;

                  bool is_friend =
                      entity.IsFriend ||
                      (Core::SDK::Game::FriendMap.find(entity.Ped) !=
                           Core::SDK::Game::FriendMap.end() &&
                       Core::SDK::Game::FriendMap[entity.Ped]);
                  if (!is_friend)
                    continue;

                  std::string player_name = (!entity.IsPlayer)
                                                ? xorstr("NPC")
                                                : entity.NetworkInfo.UserName;

                  if (var->friends_tab.friend_search[0] != '\0') {
                    std::string search_term = var->friends_tab.friend_search;
                    std::string lower_name = player_name;
                    std::transform(lower_name.begin(), lower_name.end(),
                                   lower_name.begin(), ::tolower);
                    std::string lower_search = search_term;
                    std::transform(lower_search.begin(), lower_search.end(),
                                   lower_search.begin(), ::tolower);

                    if (lower_name.find(lower_search) == std::string::npos)
                      continue;
                  }

                  std::string player_label =
                      player_name + xorstr("##") +
                      std::to_string((uintptr_t)entity.Ped);

                  bool is_active = (entity.Ped == selected_friend_ped);
                  ImVec4 friend_col = ImVec4(option->param.team_check_color[0],
                                             option->param.team_check_color[1],
                                             option->param.team_check_color[2],
                                             option->param.team_check_color[3]);

                  if (widgets->list_content_col(player_label.c_str(), is_active,
                                                friend_col))
                    selected_friend_ped = entity.Ped;

                  count++;
                }

                if (count == 0) {
                  ImVec2 text_sz =
                      gui->text_size(var->font.instrument_medium[0],
                                     xorstr("Players Not Found"));
                  ImGui::SetCursorPos(
                      ImVec2((gui->content_avail().x - text_sz.x) * 0.5f,
                             (gui->content_avail().y - text_sz.y) * 0.5f));
                  ImGui::TextColored(clr->text.text_inactive,
                                     xorstr("Players Not Found"));
                }
              }
              widgets->end_list();
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          const Core::SDK::Game::EntityStruct *sel = nullptr;
          for (const auto &e : friends_list_copy) {
            if (e.Ped == selected_friend_ped) {
              sel = &e;
              break;
            }
          }
          if (!sel && !friends_list_copy.empty()) {
            for (const auto &e : friends_list_copy) {
              bool is_friend =
                  e.IsFriend || (Core::SDK::Game::FriendMap.find(e.Ped) !=
                                     Core::SDK::Game::FriendMap.end() &&
                                 Core::SDK::Game::FriendMap[e.Ped]);
              if (is_friend && e.Ped != Core::SDK::Pointers::pLocalPlayer) {
                selected_friend_ped = e.Ped;
                sel = &e;
                break;
              }
            }
          }

          gui->begin_group();
          {
            float key_camera_h = SCALE(125);
            float spacing_h = ImGui::GetStyle().ItemSpacing.y;
            float right_total_h = height;
            float actions_h = right_total_h - key_camera_h - spacing_h -
                              SCALE(elements->child.header_height) + SCALE(2.f);

            gui->begin_child(xorstr("Key Camera"), ICON_FOCUS,
                             ImVec2(half_width, key_camera_h),
                             ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(xorstr("Enable Set Friend Keybind"),
                                &var->friends_tab.enable_setfriend_keybind,
                                false, &var->friends_tab.setfriend_key,
                                &var->friends_tab.setfriend_key_mode);
              widgets->checkbox_with_picker(
                  xorstr("FOV Circle"), &var->friends_tab.setfriend_draw_fov,
                  var->friends_tab.setfriend_fov_color, true, false);
              widgets->slider_float(xorstr("FOV Size"),
                                    &var->friends_tab.setfriend_fov, 10.f,
                                    500.f, xorstr("%.0f"));
            }
            gui->end_child();

            gui->begin_child(
                xorstr("Actions"), ICON_WRENCH, ImVec2(half_width, actions_h),
                ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
            {
              if (sel) {
                const auto &entity = *sel;
                float gap_friends = SCALE(8);
                float half_width_friends =
                    (gui->content_avail().x - gap_friends) / 2;
                if (widgets->button(xorstr("Teleport to Friend"),
                                    ImVec2(half_width_friends, SCALE(30)))) {
                  if (Core::SDK::Pointers::pLocalPlayer) {
                    D3DXVECTOR3 pos = entity.Pos;
                    ((CPed *)Core::SDK::Pointers::pLocalPlayer)->SetPos(pos);
                    notify->add_notify(xorstr("Teleported to friend"), 2000,
                                       notify_type::success);
                  }
                }
                gui->sameline(0, gap_friends);
                if (widgets->button(xorstr("Remove Friend"),
                                    ImVec2(half_width_friends, SCALE(30)))) {
                  Core::SDK::Game::FriendMap[entity.Ped] = false;
                  selected_friend_ped = nullptr;
                  notify->add_notify(xorstr("Removed from friends"), 2000,
                                     notify_type::success);
                }
              } else {
                ImVec2 text_sz = gui->text_size(var->font.instrument_medium[0],
                                                xorstr("Select a friend"));
                ImGui::SetCursorPos(ImVec2((half_width - text_sz.x) * 0.5f,
                                           (actions_h - text_sz.y) * 0.5f));
                ImGui::TextColored(clr->text.text_inactive,
                                   xorstr("Select a friend"));
              }
            }
            gui->end_child();
          }
          gui->end_group();
        } else if (elements->section.section_count_active == 7) {
          float half_width = (gui->content_avail().x - SCALE(10)) / 2;
          float height = gui->content_avail().y - SCALE(40);

          static int selected_resource_idx = -1;

          gui->begin_group();
          {
            gui->begin_child(xorstr("Resources List"), ICON_WRENCH,
                             ImVec2(half_width, height), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              gui->dummy(SCALE(0, 6));
              float list_height = gui->content_avail().y -
                                  SCALE(elements->listbox.padding * 2.f + 6.f);
              if (list_height < SCALE(120.f))
                list_height = SCALE(120.f);

              widgets->begin_list(xorstr("RESOURCES_LIST"),
                                  ImVec2(gui->content_avail().x, list_height));
              {
                std::lock_guard<std::mutex> lock(
                    Core::Features::Exploits::vResourcesMutex);
                std::string filter = search->search_buf;
                std::transform(filter.begin(), filter.end(), filter.begin(),
                               ::tolower);

                for (int i = 0;
                     i < (int)Core::Features::Exploits::vResources.size();
                     i++) {
                  const auto &res = Core::Features::Exploits::vResources[i];

                  if (!filter.empty()) {
                    std::string name_lower = res.Path;
                    std::transform(name_lower.begin(), name_lower.end(),
                                   name_lower.begin(), ::tolower);
                    if (name_lower.find(filter) == std::string::npos)
                      continue;
                  }

                  std::string label = xorstr("    ") + res.Path + xorstr("##") +
                                      std::to_string(i);
                  bool is_active = (selected_resource_idx == i);

                  ImU32 dot_col =
                      (res.State ==
                       Core::Features::Exploits::eResourceState::Started)
                          ? IM_COL32(0, 255, 0, 255)
                          : IM_COL32(255, 0, 0, 255);

                  ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                  ImDrawList *draw_list = ImGui::GetWindowDrawList();
                  float row_height = SCALE(30.f);
                  float dot_radius = SCALE(3.5f);

                  draw_list->AddCircleFilled(
                      cursor_pos + ImVec2(SCALE(12.f), row_height * 0.65f),
                      dot_radius, dot_col);

                  if (widgets->list_content(label.c_str(), is_active, 0, false))
                    selected_resource_idx = i;
                }

                if (Core::Features::Exploits::vResources.empty()) {
                  ImVec2 text_sz =
                      gui->text_size(var->font.instrument_medium[0],
                                     xorstr("Resources Not Found"));
                  ImGui::SetCursorPos(
                      ImVec2((gui->content_avail().x - text_sz.x) * 0.5f,
                             (gui->content_avail().y - text_sz.y) * 0.5f));
                  ImGui::TextColored(clr->text.text_inactive,
                                     xorstr("Resources Not Found"));
                }
              }
              widgets->end_list();
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Actions"), ICON_WRENCH,
                             ImVec2(half_width, height), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              Core::Features::Exploits::Resources_t res;
              bool found = false;
              {
                std::lock_guard<std::mutex> lock(
                    Core::Features::Exploits::vResourcesMutex);
                if (selected_resource_idx >= 0 &&
                    selected_resource_idx <
                        (int)Core::Features::Exploits::vResources.size()) {
                  res = Core::Features::Exploits::vResources
                      [selected_resource_idx];
                  found = true;
                }
              }

              if (found) {
                gui->dummy(SCALE(0, 8));

                auto render_info = [&](const char *label, const char *value,
                                       ImU32 value_col = 0) {
                  ImGui::TextColored(clr->text.text_inactive, label);
                  ImGui::SameLine();
                  if (value_col == 0)
                    ImGui::TextColored(clr->text.text_active, value);
                  else
                    ImGui::TextColored(
                        ImGui::ColorConvertU32ToFloat4(value_col), value);
                };

                render_info(xorstr("Name: "), res.Path.c_str());

                std::string state_name;
                ImU32 state_col = IM_COL32(200, 200, 200, 255);
                switch (res.State) {
                case Core::Features::Exploits::eResourceState::Started:
                  state_name = xorstr("Started");
                  state_col = IM_COL32(0, 255, 0, 255);
                  break;
                case Core::Features::Exploits::eResourceState::Stopped:
                  state_name = xorstr("Stopped");
                  state_col = IM_COL32(255, 0, 0, 255);
                  break;
                case Core::Features::Exploits::eResourceState::Starting:
                  state_name = xorstr("Starting");
                  state_col = IM_COL32(255, 255, 0, 255);
                  break;
                case Core::Features::Exploits::eResourceState::Stopping:
                  state_name = xorstr("Stopping");
                  state_col = IM_COL32(255, 165, 0, 255);
                  break;
                case Core::Features::Exploits::eResourceState::Uninitialized:
                  state_name = xorstr("Uninitialized");
                  state_col = IM_COL32(150, 150, 150, 255);
                  break;
                default:
                  state_name = xorstr("Unknown");
                  break;
                }
                if (state_name.empty()) {
                  render_info(xorstr(""), "", state_col);
                } else {
                  render_info(xorstr("State: "), state_name.c_str(), state_col);
                }

                gui->dummy(SCALE(0, 20));

                using ES = Core::Features::Exploits::eResourceState;
                bool is_uninitialized = (res.State == ES::Uninitialized);
                bool is_started       = (res.State == ES::Started);
                bool is_stopped       = (res.State == ES::Stopped);
                bool is_transitioning = (res.State == ES::Starting || res.State == ES::Stopping);


                gui->dummy(SCALE(0, 8));
                ImGui::BeginDisabled(!is_started || is_transitioning);
                if (widgets->button(xorstr("Stop Resource"),
                        ImVec2(gui->content_avail().x, SCALE(30)))) {
                  Core::Features::Exploits::g_ResourceList.stop(res.Pointer);
                  notify->add_notify(xorstr("Stopped resource successfully"),
                                     2000, notify_type::success);
                }
                ImGui::EndDisabled();


                gui->dummy(SCALE(0, 8));
                ImGui::BeginDisabled(!is_stopped || is_transitioning);
                if (widgets->button(xorstr("Start Resource"),
                        ImVec2(gui->content_avail().x, SCALE(30)))) {
                  Core::Features::Exploits::g_ResourceList.start(res.Pointer);
                  notify->add_notify(xorstr("Started resource successfully"),
                                     2000, notify_type::success);
                }
                ImGui::EndDisabled();


                gui->dummy(SCALE(0, 8));
                ImGui::BeginDisabled(is_uninitialized || is_transitioning);
                if (widgets->button(xorstr("Destroy Resource"),
                        ImVec2(gui->content_avail().x, SCALE(30)))) {
                  Core::Features::Exploits::g_ResourceList.destroy(res.Pointer);
                  notify->add_notify(xorstr("Destroyed resource successfully"),
                                     2000, notify_type::success);
                }
                ImGui::EndDisabled();


                gui->dummy(SCALE(0, 8));
                ImGui::BeginDisabled(!is_uninitialized || is_transitioning);
                if (widgets->button(xorstr("Revive Resource"),
                        ImVec2(gui->content_avail().x, SCALE(30)))) {
                  Core::Features::Exploits::g_ResourceList.revive(res.Pointer);
                  notify->add_notify(xorstr("Revived resource successfully"),
                                     2000, notify_type::success);
                }
                ImGui::EndDisabled();
              } else {
                ImVec2 text_sz = gui->text_size(var->font.instrument_medium[0],
                                                xorstr("Select a resource"));
                ImGui::SetCursorPos(ImVec2((half_width - text_sz.x) * 0.5f,
                                           (height - text_sz.y) * 0.5f));
                ImGui::TextColored(clr->text.text_inactive,
                                   xorstr("Select a resource"));
              }
            }
            gui->end_child();
          }
          gui->end_group();

        } else if (elements->section.section_count_active == 8) {
          float third_width = floorf((gui->content_avail().x - SCALE(20)) / 3);

          static std::vector<Security::Api::ConfigEntry> *config_list_ptr =
              nullptr;
          if (!config_list_ptr)
            config_list_ptr = new std::vector<Security::Api::ConfigEntry>();
          auto &config_list = *config_list_ptr;

          static int config_count = 0;
          static int last_config_section = -1;
          static bool config_list_dirty = true;

          gui->begin_group();
          {
            gui->begin_child(xorstr("Configs"), ICON_WRENCH,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              if (config_list_dirty || last_config_section != 6) {
                if (var->auth.authenticated && !var->auth.access_token.empty())
                  config_list =
                      Security::Api::config_list(var->auth.access_token);
                else
                  config_list.clear();
                config_list_dirty = false;
                last_config_section = 6;
                if (config_count >= (int)config_list.size())
                  config_count = 0;
              }

              if (!config_list.empty()) {
                std::vector<std::string> config_names;
                for (const auto &e : config_list)
                  config_names.push_back(e.name);
                widgets->dropdown(xorstr("Selected config"), &config_count,
                                  config_names, (int)config_names.size());
              } else {
                ImGui::BeginDisabled(true);
                std::vector<std::string> empty_names = {
                    xorstr("No Configs Found")};
                widgets->dropdown(xorstr("Selected config"), &config_count,
                                  empty_names, 1);
                ImGui::EndDisabled();
              }

              if (widgets->button(xorstr("Load config"))) {
                if (config_count >= 0 &&
                    config_count < (int)config_list.size()) {
                  std::string name_out, code_out, data_out;
                  if (Security::Api::config_get(var->auth.access_token,
                                                config_list[config_count].id,
                                                name_out, code_out, data_out)) {
                    nlohmann::json parsed;
                    std::string result =
                        Core::g_Config.LoadCfg(name_out, data_out, &parsed);
                    bool ok = result.find(xorstr("Error")) == std::string::npos;
                    if (ok) {
                      if (parsed.contains(xorstr("Options")))
                        OptionsConfig::ApplyOptionsParamFromJson(
                            parsed[xorstr("Options")], &option->param);
                      else if (parsed.contains(xorstr("Draw"))) {
                        const auto &dr = parsed[xorstr("Draw")];
                        if (dr.contains(xorstr("fov_circle")))
                          option->param.fov_circle = dr[xorstr("fov_circle")];
                        if (dr.contains(xorstr("fov_size")))
                          option->param.fov_size = dr[xorstr("fov_size")];
                        if (dr.contains(xorstr("fov_color")) &&
                            dr[xorstr("fov_color")].is_array() &&
                            dr[xorstr("fov_color")].size() >= 4)
                          for (int i = 0; i < 4; i++)
                            option->param.fov_color[i] =
                                dr[xorstr("fov_color")][i];
                        if (dr.contains(xorstr("silent_fov_circle")))
                          option->param.silent_fov_circle =
                              dr[xorstr("silent_fov_circle")];
                        if (dr.contains(xorstr("silent_fov_size")))
                          option->param.silent_fov_size =
                              dr[xorstr("silent_fov_size")];
                        if (dr.contains(xorstr("silent_smart_fov")))
                          option->param.silent_smart_fov =
                              dr[xorstr("silent_smart_fov")];
                        if (dr.contains(xorstr("silent_fov_near")))
                          option->param.silent_fov_near =
                              dr[xorstr("silent_fov_near")];
                        if (dr.contains(xorstr("silent_fov_far")))
                          option->param.silent_fov_far =
                              dr[xorstr("silent_fov_far")];
                        if (dr.contains(xorstr("silent_dual_fov")))
                          option->param.silent_dual_fov =
                              dr[xorstr("silent_dual_fov")];
                        if (dr.contains(xorstr("silent_dual_fov_distance")))
                          option->param.silent_dual_fov_distance =
                              dr[xorstr("silent_dual_fov_distance")];
                        if (dr.contains(xorstr("silent_fov_color")) &&
                            dr[xorstr("silent_fov_color")].is_array() &&
                            dr[xorstr("silent_fov_color")].size() >= 4)
                          for (int i = 0; i < 4; i++)
                            option->param.silent_fov_color[i] =
                                dr[xorstr("silent_fov_color")][i];
                        if (dr.contains(xorstr("triggerbot_fov_circle")))
                          option->param.triggerbot_fov_circle =
                              dr[xorstr("triggerbot_fov_circle")];
                        if (dr.contains(xorstr("triggerbot_fov_size")))
                          option->param.triggerbot_fov_size =
                              dr[xorstr("triggerbot_fov_size")];
                        if (dr.contains(xorstr("triggerbot_fov_color")) &&
                            dr[xorstr("triggerbot_fov_color")].is_array() &&
                            dr[xorstr("triggerbot_fov_color")].size() >= 4)
                          for (int i = 0; i < 4; i++)
                            option->param.triggerbot_fov_color[i] =
                                dr[xorstr("triggerbot_fov_color")][i];
                        if (dr.contains(xorstr("crosshair_style")))
                          option->param.crosshair_style =
                              dr[xorstr("crosshair_style")];
                        if (dr.contains(xorstr("crosshair_size")))
                          option->param.crosshair_size =
                              dr[xorstr("crosshair_size")];
                        if (dr.contains(xorstr("crosshair_thickness")))
                          option->param.crosshair_thickness =
                              dr[xorstr("crosshair_thickness")];
                        if (dr.contains(xorstr("crosshair_color")) &&
                            dr[xorstr("crosshair_color")].is_array() &&
                            dr[xorstr("crosshair_color")].size() >= 4)
                          for (int i = 0; i < 4; i++)
                            option->param.crosshair_color[i] =
                                dr[xorstr("crosshair_color")][i];
                      }
                    }
                    if (ok) {
                      Core::g_Config.SetLastConfigId(
                          config_list[config_count].id);
                      nlohmann::json payload;
                      payload[xorstr("Options")] =
                          OptionsConfig::OptionsParamToJson(option->param);
                      Security::Api::live_config_update(var->auth.access_token,
                                                        payload.dump());
                    }
                    notify->add_notify(
                        ok ? xorstr("Config loaded successfully") : result,
                        2000, ok ? notify_type::success : notify_type::error);
                  } else {
                    notify->add_notify(
                        xorstr("Failed to load config from server"), 2000,
                        notify_type::error);
                  }
                } else {
                  notify->add_notify(xorstr("Select a config"), 2000,
                                     notify_type::error);
                }
              }

              if (widgets->button(xorstr("Copy code")) && config_count >= 0 &&
                  config_count < (int)config_list.size()) {
                const std::string &share_code = config_list[config_count].code;
                if (!share_code.empty()) {
                  ImGui::SetClipboardText(share_code.c_str());
                  notify->add_notify(xorstr("Code copied to clipboard"), 2000,
                                     notify_type::success);
                } else {
                  notify->add_notify(xorstr("Config has no code"), 2000,
                                     notify_type::error);
                }
              }

              if (widgets->button(xorstr("Delete config"))) {
                if (config_count >= 0 &&
                    config_count < (int)config_list.size()) {
                  int id = config_list[config_count].id;
                  if (Security::Api::config_delete(var->auth.access_token,
                                                   id)) {
                    config_list_dirty = true;
                    notify->add_notify(xorstr("Config deleted"), 2000,
                                       notify_type::success);
                  } else {
                    notify->add_notify(xorstr("Failed to delete config"), 2000,
                                       notify_type::error);
                  }
                } else {
                  notify->add_notify(xorstr("Select a config"), 2000,
                                     notify_type::error);
                }
              }
            }
            gui->end_child();

            gui->begin_child(xorstr("Save / Import"), ICON_WRENCH,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              static char cfg_name[64] = {""};
              static char cfg_code[4096] = {""};

              if (!search->active_searcing) {
                widgets->text_field(ICON_WRENCH, xorstr("Config name"),
                                    xorstr("Enter name"), cfg_name,
                                    IM_ARRAYSIZE(cfg_name));
              }

              if (widgets->button(xorstr("Save config")) && cfg_name[0]) {
                if (!var->auth.authenticated ||
                    var->auth.access_token.empty()) {
                  notify->add_notify(xorstr("You must be logged in to save"),
                                     2000, notify_type::error);
                } else {
                  nlohmann::json merge;
                  merge[xorstr("Options")] =
                      OptionsConfig::OptionsParamToJson(option->param);
                  std::string code =
                      Core::g_Config.GetCurrentConfigCode(&merge);
                  if (code.empty()) {
                    notify->add_notify(xorstr("Failed to build config data"),
                                       2000, notify_type::error);
                  } else {
                    int id = Security::Api::config_create(
                        var->auth.access_token, std::string(cfg_name), code);
                    if (id) {
                      config_list_dirty = true;
                      Core::g_Config.SetLastConfigId(id);
                      notify->add_notify(
                          xorstr("Config saved (8-char code assigned)"), 2000,
                          notify_type::success);
                      memset(cfg_name, 0, IM_ARRAYSIZE(cfg_name));
                    } else {
                      notify->add_notify(
                          xorstr("Save failed (name already exists?)"), 2000,
                          notify_type::error);
                    }
                  }
                }
              } else if (IsItemHovered() && IsItemClicked() && !cfg_name[0]) {
                notify->add_notify(xorstr("Enter a name for the config"), 2000,
                                   notify_type::error);
              };

              gui->dummy(SCALE(0, 10));

              if (!search->active_searcing) {
                widgets->text_field(ICON_WRENCH, xorstr("Code or export"),
                                    xorstr("Paste 8-char code or full export"),
                                    cfg_code, IM_ARRAYSIZE(cfg_code));
              }

              if (widgets->button(xorstr("Import config"),
                                  ImVec2(gui->content_avail().x, SCALE(30)))) {
                if (cfg_code[0]) {
                  std::string pasted(cfg_code);
                  while (!pasted.empty() &&
                         (pasted.back() == ' ' || pasted.back() == '\r' ||
                          pasted.back() == '\n'))
                    pasted.pop_back();
                  while (!pasted.empty() &&
                         (pasted.front() == ' ' || pasted.front() == '\r' ||
                          pasted.front() == '\n'))
                    pasted.erase(0, 1);
                  if (pasted.size() == 8) {
                    std::string code_upper = pasted;
                    for (size_t i = 0; i < 8; i++)
                      code_upper[i] =
                          (char)toupper((unsigned char)code_upper[i]);
                    bool is_code = true;
                    for (size_t i = 0; i < 8; i++) {
                      char c = code_upper[i];
                      if ((c < 'A' || c > 'Z') && (c < '0' || c > '9')) {
                        is_code = false;
                        break;
                      }
                    }
                    if (is_code) {
                      std::string name_out, data_out;
                      if (Security::Api::config_get_by_code(
                              code_upper, name_out, data_out)) {
                        nlohmann::json parsed;
                        std::string result =
                            Core::g_Config.LoadCfg(name_out, data_out, &parsed);
                        bool ok =
                            result.find(xorstr("Error")) == std::string::npos;
                        if (ok) {
                          if (parsed.contains(xorstr("Options")))
                            OptionsConfig::ApplyOptionsParamFromJson(
                                parsed[xorstr("Options")], &option->param);
                          else if (parsed.contains(xorstr("Draw"))) {
                            const auto &dr = parsed[xorstr("Draw")];
                            if (dr.contains(xorstr("fov_circle")))
                              option->param.fov_circle =
                                  dr[xorstr("fov_circle")];
                            if (dr.contains(xorstr("fov_size")))
                              option->param.fov_size = dr[xorstr("fov_size")];
                            if (dr.contains(xorstr("fov_color")) &&
                                dr[xorstr("fov_color")].is_array() &&
                                dr[xorstr("fov_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.fov_color[i] =
                                    dr[xorstr("fov_color")][i];
                            if (dr.contains(xorstr("silent_fov_circle")))
                              option->param.silent_fov_circle =
                                  dr[xorstr("silent_fov_circle")];
                            if (dr.contains(xorstr("silent_fov_size")))
                              option->param.silent_fov_size =
                                  dr[xorstr("silent_fov_size")];
                            if (dr.contains(xorstr("silent_smart_fov")))
                              option->param.silent_smart_fov =
                                  dr[xorstr("silent_smart_fov")];
                            if (dr.contains(xorstr("silent_fov_near")))
                              option->param.silent_fov_near =
                                  dr[xorstr("silent_fov_near")];
                            if (dr.contains(xorstr("silent_fov_far")))
                              option->param.silent_fov_far =
                                  dr[xorstr("silent_fov_far")];
                            if (dr.contains(xorstr("silent_dual_fov")))
                              option->param.silent_dual_fov =
                                  dr[xorstr("silent_dual_fov")];
                            if (dr.contains(xorstr("silent_dual_fov_distance")))
                              option->param.silent_dual_fov_distance =
                                  dr[xorstr("silent_dual_fov_distance")];
                            if (dr.contains(xorstr("silent_fov_color")) &&
                                dr[xorstr("silent_fov_color")].is_array() &&
                                dr[xorstr("silent_fov_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.silent_fov_color[i] =
                                    dr[xorstr("silent_fov_color")][i];
                            if (dr.contains(xorstr("triggerbot_fov_circle")))
                              option->param.triggerbot_fov_circle =
                                  dr[xorstr("triggerbot_fov_circle")];
                            if (dr.contains(xorstr("triggerbot_fov_size")))
                              option->param.triggerbot_fov_size =
                                  dr[xorstr("triggerbot_fov_size")];
                            if (dr.contains(xorstr("triggerbot_fov_color")) &&
                                dr[xorstr("triggerbot_fov_color")].is_array() &&
                                dr[xorstr("triggerbot_fov_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.triggerbot_fov_color[i] =
                                    dr[xorstr("triggerbot_fov_color")][i];
                            if (dr.contains(xorstr("crosshair_style")))
                              option->param.crosshair_style =
                                  dr[xorstr("crosshair_style")];
                            if (dr.contains(xorstr("crosshair_size")))
                              option->param.crosshair_size =
                                  dr[xorstr("crosshair_size")];
                            if (dr.contains(xorstr("crosshair_thickness")))
                              option->param.crosshair_thickness =
                                  dr[xorstr("crosshair_thickness")];
                            if (dr.contains(xorstr("crosshair_color")) &&
                                dr[xorstr("crosshair_color")].is_array() &&
                                dr[xorstr("crosshair_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.crosshair_color[i] =
                                    dr[xorstr("crosshair_color")][i];
                          }
                        }
                        notify->add_notify(
                            ok ? xorstr("Config imported and loaded") : result,
                            2000,
                            ok ? notify_type::success : notify_type::error);
                        if (ok)
                          memset(cfg_code, 0, IM_ARRAYSIZE(cfg_code));
                      } else {
                        notify->add_notify(
                            xorstr("Config not found for this code"), 2000,
                            notify_type::error);
                      }
                    } else {
                      notify->add_notify(
                          xorstr("Invalid code (8 characters A-Z, 0-9)"), 2000,
                          notify_type::error);
                    }
                  } else {
                    if (!var->auth.authenticated ||
                        var->auth.access_token.empty()) {
                      notify->add_notify(
                          xorstr("You must be logged in to import (save)"),
                          2000, notify_type::error);
                    } else {
                      std::string cfg_name_str =
                          cfg_name[0] ? std::string(cfg_name)
                                      : std::string(xorstr("Imported config"));
                      int id = Security::Api::config_import(
                          var->auth.access_token, cfg_name_str, pasted);
                      if (id) {
                        nlohmann::json parsed;
                        std::string result = Core::g_Config.LoadCfg(
                            cfg_name_str, pasted, &parsed);
                        bool ok =
                            result.find(xorstr("Error")) == std::string::npos;
                        if (ok) {
                          if (parsed.contains(xorstr("Options")))
                            OptionsConfig::ApplyOptionsParamFromJson(
                                parsed[xorstr("Options")], &option->param);
                          else if (parsed.contains(xorstr("Draw"))) {
                            const auto &dr = parsed[xorstr("Draw")];
                            if (dr.contains(xorstr("fov_circle")))
                              option->param.fov_circle =
                                  dr[xorstr("fov_circle")];
                            if (dr.contains(xorstr("fov_size")))
                              option->param.fov_size = dr[xorstr("fov_size")];
                            if (dr.contains(xorstr("fov_color")) &&
                                dr[xorstr("fov_color")].is_array() &&
                                dr[xorstr("fov_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.fov_color[i] =
                                    dr[xorstr("fov_color")][i];
                            if (dr.contains(xorstr("silent_fov_circle")))
                              option->param.silent_fov_circle =
                                  dr[xorstr("silent_fov_circle")];
                            if (dr.contains(xorstr("silent_fov_size")))
                              option->param.silent_fov_size =
                                  dr[xorstr("silent_fov_size")];
                            if (dr.contains(xorstr("silent_smart_fov")))
                              option->param.silent_smart_fov =
                                  dr[xorstr("silent_smart_fov")];
                            if (dr.contains(xorstr("silent_fov_near")))
                              option->param.silent_fov_near =
                                  dr[xorstr("silent_fov_near")];
                            if (dr.contains(xorstr("silent_fov_far")))
                              option->param.silent_fov_far =
                                  dr[xorstr("silent_fov_far")];
                            if (dr.contains(xorstr("silent_dual_fov")))
                              option->param.silent_dual_fov =
                                  dr[xorstr("silent_dual_fov")];
                            if (dr.contains(xorstr("silent_dual_fov_distance")))
                              option->param.silent_dual_fov_distance =
                                  dr[xorstr("silent_dual_fov_distance")];
                            if (dr.contains(xorstr("silent_fov_color")) &&
                                dr[xorstr("silent_fov_color")].is_array() &&
                                dr[xorstr("silent_fov_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.silent_fov_color[i] =
                                    dr[xorstr("silent_fov_color")][i];
                            if (dr.contains(xorstr("triggerbot_fov_circle")))
                              option->param.triggerbot_fov_circle =
                                  dr[xorstr("triggerbot_fov_circle")];
                            if (dr.contains(xorstr("triggerbot_fov_size")))
                              option->param.triggerbot_fov_size =
                                  dr[xorstr("triggerbot_fov_size")];
                            if (dr.contains(xorstr("triggerbot_fov_color")) &&
                                dr[xorstr("triggerbot_fov_color")].is_array() &&
                                dr[xorstr("triggerbot_fov_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.triggerbot_fov_color[i] =
                                    dr[xorstr("triggerbot_fov_color")][i];
                            if (dr.contains(xorstr("crosshair_style")))
                              option->param.crosshair_style =
                                  dr[xorstr("crosshair_style")];
                            if (dr.contains(xorstr("crosshair_size")))
                              option->param.crosshair_size =
                                  dr[xorstr("crosshair_size")];
                            if (dr.contains(xorstr("crosshair_thickness")))
                              option->param.crosshair_thickness =
                                  dr[xorstr("crosshair_thickness")];
                            if (dr.contains(xorstr("crosshair_color")) &&
                                dr[xorstr("crosshair_color")].is_array() &&
                                dr[xorstr("crosshair_color")].size() >= 4)
                              for (int i = 0; i < 4; i++)
                                option->param.crosshair_color[i] =
                                    dr[xorstr("crosshair_color")][i];
                          }
                        }
                        notify->add_notify(
                            ok ? xorstr("Config imported and loaded") : result,
                            2000,
                            ok ? notify_type::success : notify_type::error);
                        config_list_dirty = true;
                        if (ok)
                          memset(cfg_code, 0, IM_ARRAYSIZE(cfg_code));
                      } else {
                        notify->add_notify(xorstr("Import failed"), 2000,
                                           notify_type::error);
                      }
                    }
                  }
                } else if (IsItemHovered() && IsItemClicked() && !cfg_code[0]) {
                  notify->add_notify(
                      xorstr("Paste an 8-char code or full export"), 2000,
                      notify_type::error);
                }
              }
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Fake Fps"), ICON_WRENCH,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(xorstr("Fake Fps"),
                                &option->param.fake_fps_enabled);
              widgets->checkbox(xorstr("Range mode"),
                                &option->param.fake_fps_range_mode);
              widgets->slider_int(xorstr("Fps"), &option->param.fake_fps_target,
                                  1, 5000);
              widgets->slider_int(xorstr("Fps Min"),
                                  &option->param.fake_fps_min, 1, 5000);
              widgets->slider_int(xorstr("Fps Max"),
                                  &option->param.fake_fps_max, 1, 5000);
            }
            gui->end_child();

            gui->begin_child(xorstr("Crosshair"), ICON_WRENCH,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(xorstr("Crosshair"),
                                &option->param.custom_crosshair);
              static std::vector<std::string> crosshair_styles = {
                  std::string(xorstr("Cross")), std::string(xorstr("Dot")),
                  std::string(xorstr("Plus"))};
              widgets->dropdown(xorstr("Style"), &option->param.crosshair_style,
                                crosshair_styles);
              widgets->slider_float(
                  xorstr("Size"), &option->param.crosshair_size, 1.0f, 100.0f);
              widgets->slider_float(xorstr("Thickness"),
                                    &option->param.crosshair_thickness, 1.0f,
                                    100.0f);
              widgets->color_picker(xorstr("Color"),
                                    option->param.crosshair_color);
            }
            gui->end_child();
          }
          gui->end_group();

          gui->sameline();

          gui->begin_group();
          {
            gui->begin_child(xorstr("Config"), ICON_WRENCH,
                             ImVec2(third_width, 0), ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar);
            {
              widgets->checkbox(option->name.vsync, &option->param.vsync);
              widgets->checkbox(option->name.stream_proof,
                                &option->param.stream_proof);
              widgets->checkbox(option->name.second_monitor_display,
                                &option->param.second_monitor_display);

              if (option->param.second_monitor_display) {
                static std::vector<std::string> *monitor_names_ptr = nullptr;
                if (!monitor_names_ptr)
                  monitor_names_ptr = new std::vector<std::string>();
                auto &monitor_names = *monitor_names_ptr;

                static double last_monitor_update = 0.0;
                double current_time = ImGui::GetTime();
                if (monitor_names.empty() ||
                    current_time - last_monitor_update > 5.0) {
                  std::vector<MONITORINFOEXW> monitors;
                  EnumDisplayMonitors(
                      nullptr, nullptr,
                      [](HMONITOR hMon, HDC, LPRECT, LPARAM lp) -> BOOL {
                        MONITORINFOEXW mi;
                        mi.cbSize = sizeof(mi);
                        if (GetMonitorInfoW(hMon, (MONITORINFO *)&mi)) {
                          reinterpret_cast<std::vector<MONITORINFOEXW> *>(lp)
                              ->push_back(mi);
                        }
                        return TRUE;
                      },
                      reinterpret_cast<LPARAM>(&monitors));

                  std::sort(
                      monitors.begin(), monitors.end(),
                      [](const MONITORINFOEXW &a, const MONITORINFOEXW &b) {
                        if (a.rcMonitor.left != b.rcMonitor.left)
                          return a.rcMonitor.left < b.rcMonitor.left;
                        return a.rcMonitor.top < b.rcMonitor.top;
                      });

                  monitor_names.clear();
                  for (size_t i = 0; i < monitors.size(); ++i) {
                    const auto &mi = monitors[i];
                    std::string name;
                    int len = WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1,
                                                  nullptr, 0, nullptr, nullptr);
                    if (len > 0) {
                      name.resize(len - 1);
                      WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, &name[0],
                                          len - 1, nullptr, nullptr);
                    } else {
                      name = std::string(xorstr("Monitor ")) + std::to_string(i + 1);
                    }
                    if (mi.dwFlags & MONITORINFOF_PRIMARY) {
                      name += xorstr(" (Primary)");
                    }
                    monitor_names.push_back(name);
                  }
                  last_monitor_update = current_time;
                }

                if (option->param.display_monitor_index < 0 ||
                    option->param.display_monitor_index >=
                        (int)monitor_names.size()) {
                  option->param.display_monitor_index = 0;
                }
                widgets->dropdown(xorstr("Select Monitor"),
                                  &option->param.display_monitor_index,
                                  monitor_names);
              }

              widgets->checkbox(option->name.watermark,
                                &option->param.watermark);

              static std::vector<std::string> *corner_positions_ptr = nullptr;
              if (!corner_positions_ptr)
                corner_positions_ptr = new std::vector<std::string>{
                    std::string(xorstr("Top Left")),
                    std::string(xorstr("Top Right")),
                    std::string(xorstr("Bottom Left")),
                    std::string(xorstr("Bottom Right"))};
              auto &corner_positions = *corner_positions_ptr;
              if (option->param.watermark) {
                widgets->dropdown(xorstr("Watermark position"),
                                  &var->watermark.position, corner_positions);
              }

              gui->dummy(SCALE(0, 6));
              widgets->checkbox(xorstr("Notifications"),
                                &var->notifications.enabled);
              if (var->notifications.enabled) {
                widgets->dropdown(xorstr("Notifications position"),
                                  &var->notifications.position,
                                  corner_positions);
              }

              gui->dummy(SCALE(0, 8));
              widgets->color_picker(xorstr("Menu Color"), var->gui.accent_clr,
                                    true);
              gui->dummy(SCALE(0, 8));
              widgets->checkbox(option->name.web_only, &option->param.web_only);
              gui->dummy(SCALE(0, 8));
              gui->render_text(GetWindowDrawList(),
                               var->font.instrument_medium[1],
                               draw->get_clr(clr->text.text_active),
                               option->name.menu_keybind.c_str());
              gui->dummy(SCALE(0, 8));
              widgets->key_select(xorstr("##menu_keybind"),
                                  &option->param.menu_keybind);
              gui->dummy(SCALE(0, 8));
              gui->render_text(GetWindowDrawList(),
                               var->font.instrument_medium[1],
                               draw->get_clr(clr->text.text_active),
                               option->name.unload_keybind.c_str());
              gui->dummy(SCALE(0, 8));
              widgets->key_select(xorstr("##unload_keybind"),
                                  &option->param.unload_keybind);
              gui->dummy(SCALE(0, 8));
              if (widgets->button(xorstr("Unload"),
                                  ImVec2(gui->content_avail().x, SCALE(30)))) {
                option->param.should_unload = true;
              }
            }
            gui->end_child();
          }
          gui->end_group();

        } else if (elements->section.section_count_active == 5) {

          struct EventEntry { uint16_t id; const char* name; };
          static const EventEntry kEventList[] = {
            {3,  "SCRIPT_ARRAY_DATA_VERIFY_EVENT"},
            {4,  "REQUEST_CONTROL_EVENT"},
            {5,  "GIVE_CONTROL_EVENT"},
            {6,  "WEAPON_DAMAGE_EVENT"},
            {7,  "REQUEST_PICKUP_EVENT"},
            {8,  "REQUEST_MAP_PICKUP_EVENT"},
            {11, "RESPAWN_PLAYER_PED_EVENT"},
            {12, "GIVE_WEAPON_EVENT"},
            {13, "REMOVE_WEAPON_EVENT"},
            {14, "REMOVE_ALL_WEAPONS_EVENT"},
            {15, "VEHICLE_COMPONENT_CONTROL_EVENT"},
            {16, "FIRE_EVENT"},
            {17, "EXPLOSION_EVENT"},
            {18, "START_PROJECTILE_EVENT"},
            {19, "UPDATE_PROJECTILE_TARGET_EVENT"},
            {20, "REMOVE_PROJECTILE_ENTITY_EVENT"},
            {21, "BREAK_PROJECTILE_TARGET_LOCK_EVENT"},
            {22, "ALTER_WANTED_LEVEL_EVENT"},
            {23, "CHANGE_RADIO_STATION_EVENT"},
            {24, "RAGDOLL_REQUEST_EVENT"},
            {25, "PLAYER_TAUNT_EVENT"},
            {26, "PLAYER_CARD_STAT_EVENT"},
            {27, "DOOR_BREAK_EVENT"},
            {28, "SCRIPTED_GAME_EVENT"},
            {29, "REMOTE_SCRIPT_INFO_EVENT"},
            {30, "REMOTE_SCRIPT_LEAVE_EVENT"},
            {31, "MARK_AS_NO_LONGER_NEEDED_EVENT"},
            {32, "CONVERT_TO_SCRIPT_ENTITY_EVENT"},
            {33, "SCRIPT_WORLD_STATE_EVENT"},
            {34, "CLEAR_AREA_EVENT"},
            {35, "CLEAR_RECTANGLE_AREA_EVENT"},
            {36, "NETWORK_REQUEST_SYNCED_SCENE_EVENT"},
            {37, "NETWORK_START_SYNCED_SCENE_EVENT"},
            {38, "NETWORK_STOP_SYNCED_SCENE_EVENT"},
            {39, "NETWORK_UPDATE_SYNCED_SCENE_EVENT"},
            {40, "INCIDENT_ENTITY_EVENT"},
            {41, "GIVE_PED_SCRIPTED_TASK_EVENT"},
            {42, "GIVE_PED_SEQUENCE_TASK_EVENT"},
            {43, "NETWORK_CLEAR_PED_TASKS_EVENT"},
            {44, "NETWORK_START_PED_ARREST_EVENT"},
            {45, "NETWORK_START_PED_UNCUFF_EVENT"},
            {46, "NETWORK_SOUND_CAR_HORN_EVENT"},
            {47, "NETWORK_ENTITY_AREA_STATUS_EVENT"},
            {48, "NETWORK_GARAGE_OCCUPIED_STATUS_EVENT"},
            {49, "PED_CONVERSATION_LINE_EVENT"},
            {50, "SCRIPT_ENTITY_STATE_CHANGE_EVENT"},
            {51, "NETWORK_PLAY_SOUND_EVENT"},
            {52, "NETWORK_STOP_SOUND_EVENT"},
            {53, "NETWORK_PLAY_AIRDEFENSE_FIRE_EVENT"},
            {54, "NETWORK_BANK_REQUEST_EVENT"},
            {55, "NETWORK_AUDIO_BARK_EVENT"},
            {56, "REQUEST_DOOR_EVENT"},
            {57, "NETWORK_TRAIN_REPORT_EVENT"},
            {58, "NETWORK_TRAIN_REQUEST_EVENT"},
            {59, "NETWORK_INCREMENT_STAT_EVENT"},
            {60, "MODIFY_VEHICLE_LOCK_WORLD_STATE_DATA"},
            {61, "MODIFY_PTFX_WORD_STATE_DATA_SCRIPTED_EVOLVE_EVENT"},
            {62, "REQUEST_PHONE_EXPLOSION_EVENT"},
            {63, "REQUEST_DETACHMENT_EVENT"},
            {64, "KICK_VOTES_EVENT"},
            {65, "GIVE_PICKUP_REWARDS_EVENT"},
            {66, "BLOW_UP_VEHICLE_EVENT"},
            {67, "NETWORK_SPECIAL_FIRE_EQUIPPED_WEAPON"},
            {68, "NETWORK_RESPONDED_TO_THREAT_EVENT"},
            {69, "NETWORK_SHOUT_TARGET_POSITION"},
            {70, "VOICE_DRIVEN_MOUTH_MOVEMENT_FINISHED_EVENT"},
            {71, "PICKUP_DESTROYED_EVENT"},
            {72, "UPDATE_PLAYER_SCARS_EVENT"},
            {73, "NETWORK_CHECK_EXE_SIZE_EVENT"},
            {74, "NETWORK_PTFX_EVENT"},
            {75, "NETWORK_PED_SEEN_DEAD_PED_EVENT"},
            {76, "REMOVE_STICKY_BOMB_EVENT"},
            {77, "NETWORK_CHECK_CODE_CRCS_EVENT"},
            {78, "INFORM_SILENCED_GUNSHOT_EVENT"},
            {79, "PED_PLAY_PAIN_EVENT"},
            {80, "CACHE_PLAYER_HEAD_BLEND_DATA_EVENT"},
            {81, "REMOVE_PED_FROM_PEDGROUP_EVENT"},
            {82, "REPORT_MYSELF_EVENT"},
            {83, "REPORT_CASH_SPAWN_EVENT"},
            {84, "ACTIVATE_VEHICLE_SPECIAL_ABILITY_EVENT"},
            {85, "BLOCK_WEAPON_SELECTION"},
            {86, "NETWORK_CHECK_CATALOG_CRC"},
          };
          static constexpr int kEventCount = (int)(sizeof(kEventList) / sizeof(kEventList[0]));

          static int   evt_selected    = 0;
          static char  evt_name_buf[256]     = {};
          static char  evt_payload_buf[2048] = {};
          static bool  evt_use_server  = false;
          static int   evt_loop_ms     = 1000;
          static int   evt_sub_tab     = 0;
          static char  evt_filter[128] = {};
          static bool  evt_name_custom = false;
          static bool  evt_capturing   = false;

          if (evt_name_buf[0] == '\0' && kEventCount > 0)
            strncpy_s(evt_name_buf, kEventList[0].name, _TRUNCATE);

          float gap = SCALE(8);


          {
            float btn_half = (gui->content_avail().x - gap) / 2;
            bool  cap_now  = evt_capturing;
            if (cap_now)
              ImGui::PushStyleColor(ImGuiCol_Button,
                draw->get_clr(clr->base_colors.accent_clr, 0.22f));
            if (widgets->button(xorstr("Start Capturing"), ImVec2(btn_half, SCALE(30))))
              evt_capturing = true;
            if (cap_now)
              ImGui::PopStyleColor();

            ImGui::SameLine(0, gap);

            bool stop_now = !evt_capturing;
            if (stop_now)
              ImGui::PushStyleColor(ImGuiCol_Button,
                draw->get_clr(clr->base_colors.accent_clr, 0.22f));
            if (widgets->button(xorstr("Stop Capturing"), ImVec2(btn_half, SCALE(30))))
              evt_capturing = false;
            if (stop_now)
              ImGui::PopStyleColor();
          }

          gui->dummy(SCALE(0, 4));



          float half_width = (gui->content_avail().x - gap) / 2;
          float child_h    = gui->content_avail().y - SCALE(38);
          float origin_x   = ImGui::GetCursorPosX();
          float origin_y   = ImGui::GetCursorPosY();


          gui->begin_child(xorstr("Event List"), FA_COPY,
                           ImVec2(half_width, child_h), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar);
          {
            gui->dummy(SCALE(0, 4));
            widgets->text_field(FA_SEARCH, xorstr("##evtfilter"),
                                xorstr("Search events..."),
                                evt_filter, (int)sizeof(evt_filter));
            gui->dummy(SCALE(0, 4));

            ImGui::BeginChild(xorstr("##evtscroll"), gui->content_avail(), false,
                              ImGuiWindowFlags_NoScrollbar);
            {
              std::string filter_lower(evt_filter);
              for (char& c : filter_lower) c = (char)tolower((unsigned char)c);

              for (int i = 0; i < kEventCount; i++) {
                const EventEntry& e = kEventList[i];

                if (!filter_lower.empty()) {
                  std::string nl(e.name);
                  for (char& c : nl) c = (char)tolower((unsigned char)c);
                  if (nl.find(filter_lower) == std::string::npos) continue;
                }

                bool sel = (evt_selected == i);
                if (sel && !evt_name_custom)
                  strncpy_s(evt_name_buf, e.name, _TRUNCATE);

                if (sel)
                  ImGui::PushStyleColor(ImGuiCol_Button,
                    draw->get_clr(clr->base_colors.accent_clr, 0.18f));
                if (widgets->button(e.name,
                    ImVec2(ImGui::GetContentRegionAvail().x, SCALE(30)))) {
                  evt_selected    = i;
                  evt_name_custom = false;
                  strncpy_s(evt_name_buf, e.name, _TRUNCATE);
                }
                if (sel)
                  ImGui::PopStyleColor();
                gui->dummy(SCALE(0, 2));
              }
            }
            ImGui::EndChild();
          }
          gui->end_child();


          ImGui::SetCursorPos(ImVec2(origin_x + half_width + gap, origin_y));


          gui->begin_child(xorstr("Event Detail"), FA_COPY,
                           ImVec2(half_width, child_h), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar);
          {
            gui->dummy(SCALE(0, 8));


            {
              ImDrawList* dl  = ImGui::GetWindowDrawList();
              ImVec2      cur = ImGui::GetCursorScreenPos();
              float       aw  = ImGui::GetContentRegionAvail().x;
              float       bh  = SCALE(22);
              float       bp  = SCALE(8);
              char        id_label[32];
              snprintf(id_label, sizeof(id_label), "ID: %d", kEventList[evt_selected].id);
              ImVec2 id_sz = ImGui::CalcTextSize(id_label);
              float  bw    = id_sz.x + bp * 2;
              ImVec2 bmin(cur.x + aw - bw, cur.y + (bh - bh) * 0.5f);
              ImVec2 bmax(cur.x + aw,       bmin.y + bh);
              dl->AddRectFilled(bmin, bmax,
                draw->get_clr(clr->base_colors.accent_clr, 0.15f), SCALE(6));
              dl->AddText(ImVec2(bmin.x + bp, bmin.y + (bh - id_sz.y) * 0.5f),
                draw->get_clr(clr->base_colors.accent_clr), id_label);
              ImGui::SetCursorScreenPos(cur);
              ImGui::PushFont(var->font.instrument_bold[0]);
              ImGui::PushStyleColor(ImGuiCol_Text, draw->get_clr(clr->text.text_active));
              ImGui::TextUnformatted(kEventList[evt_selected].name);
              ImGui::PopStyleColor();
              ImGui::PopFont();
              ImGui::SetCursorScreenPos(ImVec2(cur.x, cur.y + SCALE(26)));
            }

            gui->dummy(SCALE(0, 8));


            {
              const char* sub_labels[] = { xorstr("Execute"), xorstr("Loop") };
              float sub_w = (ImGui::GetContentRegionAvail().x - gap) / 2;
              for (int t = 0; t < 2; t++) {
                if (t > 0) ImGui::SameLine(0, gap);
                bool active = (evt_sub_tab == t);
                if (active)
                  ImGui::PushStyleColor(ImGuiCol_Button,
                    draw->get_clr(clr->base_colors.accent_clr, 0.22f));
                if (widgets->button(sub_labels[t], ImVec2(sub_w, SCALE(30))))
                  evt_sub_tab = t;
                if (active)
                  ImGui::PopStyleColor();
              }
            }

            gui->dummy(SCALE(0, 10));

            if (evt_sub_tab == 0) {

              widgets->text_field(FA_TAG, xorstr("##evtname"),
                                  xorstr("Event name..."),
                                  evt_name_buf, (int)sizeof(evt_name_buf));
              if (ImGui::IsItemEdited()) evt_name_custom = true;

              gui->dummy(SCALE(0, 6));

              widgets->text_field(FA_WIFI, xorstr("##evtpayload"),
                                  xorstr("Payload  e.g. [1, 2.5, \"str\"] or hex..."),
                                  evt_payload_buf, (int)sizeof(evt_payload_buf));

              gui->dummy(SCALE(0, 8));

              {
                float bh2 = (ImGui::GetContentRegionAvail().x - gap) / 2;
                bool loc = !evt_use_server;
                if (loc)
                  ImGui::PushStyleColor(ImGuiCol_Button,
                    draw->get_clr(clr->base_colors.accent_clr, 0.22f));
                if (widgets->button(xorstr("Local"), ImVec2(bh2, SCALE(30))))
                  evt_use_server = false;
                if (loc) ImGui::PopStyleColor();
                ImGui::SameLine(0, gap);
                bool srv = evt_use_server;
                if (srv)
                  ImGui::PushStyleColor(ImGuiCol_Button,
                    draw->get_clr(clr->base_colors.accent_clr, 0.22f));
                if (widgets->button(xorstr("Server"), ImVec2(0, SCALE(30))))
                  evt_use_server = true;
                if (srv) ImGui::PopStyleColor();
              }

              float rem = ImGui::GetContentRegionAvail().y - SCALE(42);
              if (rem > 0) gui->dummy(ImVec2(0, rem));

              ImGui::PushStyleColor(ImGuiCol_Button,
                draw->get_clr(clr->base_colors.accent_clr, 0.22f));
              if (widgets->button(xorstr("Fire Event"),
                  ImVec2(ImGui::GetContentRegionAvail().x, SCALE(30)))) {
                Core::Features::Exploits::g_EventExecutor.Execute(
                  evt_name_buf,
                  evt_payload_buf[0] ? evt_payload_buf : nullptr,
                  evt_use_server);
              }
              ImGui::PopStyleColor();

            } else {

              bool loop_now = Core::Features::Exploits::g_EventExecutor.IsLoopActive();

              widgets->text_field(FA_TAG, xorstr("##evtloopname"),
                                  xorstr("Event name..."),
                                  evt_name_buf, (int)sizeof(evt_name_buf));
              if (ImGui::IsItemEdited()) evt_name_custom = true;

              gui->dummy(SCALE(0, 6));

              widgets->text_field(FA_WIFI, xorstr("##evtlooppayload"),
                                  xorstr("Payload..."),
                                  evt_payload_buf, (int)sizeof(evt_payload_buf));

              gui->dummy(SCALE(0, 8));
              widgets->slider_int(xorstr("Interval (ms)"), &evt_loop_ms, 50, 10000, xorstr("%d ms"));
              gui->dummy(SCALE(0, 8));

              {
                float bh2 = (ImGui::GetContentRegionAvail().x - gap) / 2;
                bool loc = !evt_use_server;
                if (loc)
                  ImGui::PushStyleColor(ImGuiCol_Button,
                    draw->get_clr(clr->base_colors.accent_clr, 0.22f));
                if (widgets->button(xorstr("Local"), ImVec2(bh2, SCALE(30))))
                  evt_use_server = false;
                if (loc) ImGui::PopStyleColor();
                ImGui::SameLine(0, gap);
                bool srv = evt_use_server;
                if (srv)
                  ImGui::PushStyleColor(ImGuiCol_Button,
                    draw->get_clr(clr->base_colors.accent_clr, 0.22f));
                if (widgets->button(xorstr("Server"), ImVec2(0, SCALE(30))))
                  evt_use_server = true;
                if (srv) ImGui::PopStyleColor();
              }

              float rem = ImGui::GetContentRegionAvail().y - SCALE(42);
              if (rem > 0) gui->dummy(ImVec2(0, rem));

              if (loop_now)
                ImGui::PushStyleColor(ImGuiCol_Button,
                  draw->get_clr(clr->base_colors.accent_clr, 0.22f));
              if (widgets->button(loop_now ? xorstr("Stop Loop") : xorstr("Start Loop"),
                  ImVec2(ImGui::GetContentRegionAvail().x, SCALE(30)))) {
                if (loop_now) {
                  Core::Features::Exploits::g_EventExecutor.SetLoopActive(false);
                } else {
                  Core::Features::Exploits::g_EventExecutor.SetLoopConfig(
                    evt_name_buf,
                    evt_payload_buf[0] ? evt_payload_buf : nullptr,
                    evt_use_server, evt_loop_ms);
                  Core::Features::Exploits::g_EventExecutor.SetLoopActive(true);
                }
              }
              if (loop_now)
                ImGui::PopStyleColor();
            }
          }
          gui->end_child();

        } else if (elements->section.section_count_active == 6) {

          static char  exec_buf[65536] = {};
          static int   exec_resource   = 0;

          static std::vector<std::string>* exec_resource_list_ptr = nullptr;
          if (!exec_resource_list_ptr) {
            exec_resource_list_ptr = new std::vector<std::string>();
            exec_resource_list_ptr->push_back(std::string(xorstr("monitor")));
          }
          auto& exec_resource_list = *exec_resource_list_ptr;

          float avail_w = gui->content_avail().x;
          float avail_h = gui->content_avail().y;
          float btn_bar_h = SCALE(46);
          float editor_h  = avail_h - btn_bar_h - SCALE(8);

          ImVec2 editor_size(avail_w, editor_h > 0 ? editor_h : SCALE(100));
          widgets->lua_field(xorstr("##execfield"), exec_buf, sizeof(exec_buf),
                             editor_size,
                             ImGuiInputTextFlags_Multiline |
                             ImGuiInputTextFlags_AllowTabInput);

          gui->dummy(SCALE(0, 8));

          {
            float resource_label_w = ImGui::CalcTextSize(xorstr("Resource")).x + SCALE(8);
            float dropdown_w       = SCALE(120);
            float buttons_w        = avail_w - resource_label_w - dropdown_w - SCALE(24);
            float third_btn        = (buttons_w - SCALE(12)) / 3;

            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(34, 139, 34, 220));
            if (widgets->button(xorstr(FA_PLAY "  Execute"), ImVec2(third_btn, SCALE(34))))
              (void)0;
            ImGui::PopStyleColor();

            gui->sameline(0, SCALE(6));
            if (widgets->button(xorstr("Clear"), ImVec2(third_btn, SCALE(34))))
              memset(exec_buf, 0, sizeof(exec_buf));

            gui->sameline(0, SCALE(6));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(180, 40, 40, 220));
            if (widgets->button(xorstr(FA_STOP_BTN "  Reset"), ImVec2(third_btn, SCALE(34))))
              memset(exec_buf, 0, sizeof(exec_buf));
            ImGui::PopStyleColor();

            gui->sameline(0, SCALE(6));
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, draw->get_clr(clr->text.text_inactive));
            ImGui::TextUnformatted(xorstr("Resource"));
            ImGui::PopStyleColor();

            gui->sameline(0, SCALE(4));
            widgets->dropdown(xorstr("##execres"), &exec_resource, exec_resource_list,
                              (int)exec_resource_list.size());
          }
        }
      }
      gui->end_content();
      gui->pop_var();

      if (pushed_global_alpha)
        gui->pop_var();

      {
        float logo_reserve = SCALE(70.f);
        float user_reserve = SCALE(120.f);
        float tabs_area_width = size.x - logo_reserve - user_reserve;

        gui->begin_content(
            xorstr("bottom_bar"),
            ImVec2(gui->content_avail().x, bottom_bar_height), SCALE(0, 0),
            SCALE(6, 0),
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        {
          if (var->window.top_left_glow_texture && var->window.bot_right_glow_texture) {
            ImDrawList* nav_dl = ImGui::GetWindowDrawList();
            ImVec2 nav_pos = ImGui::GetWindowPos();
            ImVec2 nav_size = ImGui::GetWindowSize();
            ImU32 glow_color = draw->get_clr(clr->base_colors.accent_clr, 0.50f);
            nav_dl->PushClipRect(nav_pos, nav_pos + nav_size, true);
            draw->image_rounded(nav_dl, var->window.top_left_glow_texture,
                pos, pos + size, ImVec2(0, 0), ImVec2(1, 1), glow_color, 0.f);
            draw->image_rounded(nav_dl, var->window.bot_right_glow_texture,
                pos, pos + size, ImVec2(0, 0), ImVec2(1, 1), glow_color, 0.f);
            nav_dl->PopClipRect();
          }

          int tab_count = (int)elements->section.section_list[0].size();

          float total_w = 0.f;
          {
            for (int i = 0; i < tab_count; i++) {
              ImVec2 ts = gui->text_size(
                  var->font.instrument_medium[0],
                  elements->section.section_list[1][i].data());
              bool expanded_now = (i == elements->section.section_count);
              float w = expanded_now ? (SCALE(42.f) + ts.x + SCALE(24.f))
                                     : SCALE(42.f);
              total_w += w;
            }
            total_w += SCALE(6.f) * (tab_count - 1);
          }

          float start_x = logo_reserve + (tabs_area_width - total_w) * 0.5f;
          if (start_x < logo_reserve) start_x = logo_reserve;

          gui->set_pos(start_x, pos_x);
          gui->set_pos((bottom_bar_height -
                        SCALE(elements->section.section_height)) *
                           0.5f,
                       pos_y);

          for (int i = 0; i < tab_count; i++) {
            if (i > 0) gui->sameline();
            widgets->section(elements->section.section_list[0][i].data(),
                             elements->section.section_list[1][i].data(), i,
                             elements->section.section_count);
          }
        }
        gui->end_content();
      }
    }
    gui->end();

    if (is_menu && var->watermark.watermark)
      gui->watermark(xorstr("watermark"), var->watermark.content,
                     static_cast<watermark_pos>(var->watermark.position),
                     &var->watermark.watermark);

    if ((GetAsyncKeyState(VK_OEM_PLUS) & 0x1) && var->gui.stored_dpi <= 200) {
      var->gui.stored_dpi += 10;
      var->gui.dpi_changed = true;
    } else if ((GetAsyncKeyState(VK_OEM_MINUS) & 0x1) &&
               var->gui.stored_dpi > 100) {
      var->gui.stored_dpi -= 10;
      var->gui.dpi_changed = true;
    }
  }
}