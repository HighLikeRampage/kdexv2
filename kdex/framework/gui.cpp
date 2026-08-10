#include "framework/Globals.hpp"
#include "../game/Core/Config.hpp"
#include "../game/Core/Core.hpp"
#include "../game/Core/Features/Dumper/Dumper.hpp"
#include "../game/Security/xorstr.hpp"
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
#include "gui/gui_internal.hpp"

void c_gui::render() {
  static bool s_initialized_anim_size = false;
  static std::string s_last_error_msg;
  static float s_last_error_time = 0.f;
  if (!s_initialized_anim_size) {
    s_initialized_anim_size = true;
    var->auth.anim_window_size = var->auth.auth_window_size;
  }

  notify->setup_notify();

  using auth_screen = decltype(var->auth)::screen;
  using auth_operation_type = decltype(var->auth)::auth_operation_type;

  if (var->auth.auth_result_ready) {
    var->auth.auth_result_ready = false;
    var->auth.auth_request_in_progress = false;

    auto& res = var->auth.auth_result;
    if (var->auth.auth_operation == auth_operation_type::register_user) {
      if (!res.success) {
        var->auth.auth_notify_msg = res.error_message;
        var->auth.auth_notify_type = 1;
        var->auth.auth_notify_start_time = ImGui::GetTime();
      } else if (res.days_left <= 0) {
        var->auth.auth_notify_msg = xorstr("No subscription time remaining");
        var->auth.auth_notify_type = 1;
        var->auth.auth_notify_start_time = ImGui::GetTime();
      } else {
        var->auth.register_error = false;
        var->auth.login_error = false;

        var->auth.access_token = res.access_token;
        var->auth.refresh_token = res.refresh_token;
        var->auth.subscription_days_left = res.days_left;
        var->auth.subscription_active = res.days_left > 0;
        var->auth.subscription_expires_at = res.subscription_expires_at;
        var->auth.authenticated = true;
        g_MenuInfo.IsLogged = true;

        std::thread([token = var->auth.access_token]() {
          Security::Api::fivem_set_logged(token, true);
        }).detach();

        var->auth.current_screen = auth_screen::spinner_post;
        notify->add_notify(xorstr("Account registered successfully!"), 2000, notify_type::success);
      }
    } else if (var->auth.auth_operation == auth_operation_type::login) {
      if (!res.success) {
        var->auth.auth_notify_msg = res.error_message;
        var->auth.auth_notify_type = 1;
        var->auth.auth_notify_start_time = ImGui::GetTime();
      } else if (res.days_left <= 0) {
        var->auth.auth_notify_msg = xorstr("No subscription time remaining");
        var->auth.auth_notify_type = 1;
        var->auth.auth_notify_start_time = ImGui::GetTime();
      } else {
        var->auth.login_error = false;
        var->auth.authenticated = true;
        g_MenuInfo.IsLogged = true;

        var->auth.access_token = res.access_token;
        var->auth.refresh_token = res.refresh_token;
        var->auth.subscription_days_left = res.days_left;
        var->auth.subscription_active = res.days_left > 0;
        var->auth.subscription_expires_at = res.subscription_expires_at;

        std::thread([token = var->auth.access_token]() {
          Security::Api::fivem_set_logged(token, true);
        }).detach();

        var->auth.current_screen = auth_screen::spinner_post;
      }
    }
    var->auth.auth_operation = auth_operation_type::none;
  }

  using auth_screen_early = decltype(var->auth)::screen;
  bool is_menu = (var->auth.current_screen == auth_screen_early::menu);
  bool is_launch = (var->auth.current_screen == auth_screen_early::launch);
  ImVec2 target_size =
      is_menu ? (g_MenuInfo.IsOpen ? var->window.window_size : ImVec2(0.f, 0.f))
      : is_launch ? var->auth.launch_window_size
                  : var->auth.auth_window_size;
  if (is_menu && var->auth.first_open_done) {
    var->auth.anim_window_size = target_size;
  } else {
    float speed = (is_menu && !var->auth.first_open_done) ? 250.f : 170.f;
    gui->easing(var->auth.anim_window_size.x, target_size.x, speed,
                static_easing);
    gui->easing(var->auth.anim_window_size.y, target_size.y, speed,
                static_easing);
  }
  gui->set_next_window_size(SCALE(var->auth.anim_window_size));
  {
    ImVec2 display = ImGui::GetIO().DisplaySize;
    ImVec2 win_sz = SCALE(var->auth.anim_window_size);
    ImGuiCond cond = (is_menu && !var->auth.first_open_done)
                         ? ImGuiCond_Always
                         : ImGuiCond_FirstUseEver;
    gui->set_next_window_pos(
        ImVec2((display.x - win_sz.x) * 0.5f, (display.y - win_sz.y) * 0.5f),
        cond);
  }

  gui->begin();
  {
    gui->set_style();

    static ImVec2 drag_start_mouse;
    static ImVec2 drag_start_window;
    static bool is_dragging = false;

    ImVec2 drag_area_size = ImVec2(gui->window_size().x, SCALE(50.f));
    ImGui::SetCursorScreenPos(gui->window_pos());
    ImGui::InvisibleButton(xorstr("##drag_area"), drag_area_size);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
      is_dragging = true;
      drag_start_mouse = ImGui::GetMousePos();
      drag_start_window = gui->window_pos();
    }

    if (is_dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      ImVec2 current_mouse = ImGui::GetMousePos();
      ImVec2 delta = ImVec2(current_mouse.x - drag_start_mouse.x, current_mouse.y - drag_start_mouse.y);
      ImGui::SetWindowPos(ImVec2(drag_start_window.x + delta.x, drag_start_window.y + delta.y));
    } else {
      is_dragging = false;
    }

    ImGui::SetCursorPosY(0);

    if (var->auth.authenticated && !var->auth.access_token.empty() && !g_IsInjectedDll) {
      static double last_game_state_send = 0.0;
      double t = ImGui::GetTime();
      if (t - last_game_state_send >= 3.0) {
        last_game_state_send = t;
        nlohmann::json players_arr = nlohmann::json::array();
        {
          std::lock_guard<std::mutex> lock_gs(Core::SDK::Game::EntityListMutex);
          for (const auto &entity : Core::SDK::Game::EntityList) {
            if (entity.Ped == Core::SDK::Pointers::pLocalPlayer)
              continue;
            if (!option->param.playerlist_display_peds && !entity.IsPlayer)
              continue;
            std::string player_name = entity.NetworkInfo.UserName;
            if (!entity.IsPlayer)
              player_name = xorstr("NPC");
            nlohmann::json p;
            p[xorstr("name")] = player_name;
            if (entity.Id > 0)
              p[xorstr("serverId")] = entity.Id;
            players_arr.push_back(std::move(p));
          }
        }
        if (players_arr.size() <= 256)
          SubmitPendingGameState(players_arr.dump());
      }
    }

    if (is_menu && g_MenuInfo.IsOpen)
      gui->draw_decorations();

    clr->base_colors.accent_clr =
        ImVec4(var->gui.accent_clr[0], var->gui.accent_clr[1],
               var->gui.accent_clr[2], 1.f);

    const ImVec2 pos = gui->window_pos();
    const ImVec2 size = gui->window_size();
    ImDrawList *drawlist = GetWindowDrawList();
    float menu_alpha = 1.0f;

    var->watermark.watermark = option->param.watermark;
    if (var->watermark.watermark) {
      ImGuiIO &io = GetIO();
      int fps_i = (int)(io.Framerate + 0.5f);
      int ms_i = fps_i > 0 ? (int)(1000.0f / (float)fps_i + 0.5f) : 0;
      var->watermark.content = {xorstr("Rotten"),
                                std::to_string(fps_i) + xorstr("FPS"),
                                std::to_string(ms_i) + xorstr("ms")};
    }
    GuiFrameContext ctx{is_menu, is_launch, pos, size, drawlist, menu_alpha};

    if (render_auth_screens(ctx))
      return;
    if (render_launch_screen(ctx))
      return;

    render_menu_screen(ctx);
  }
}
