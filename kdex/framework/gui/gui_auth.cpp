#include "../Globals.hpp"
#include "../game/Core/Config.hpp"
#include "../game/Core/Core.hpp"
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

bool c_gui::render_auth_screens(const GuiFrameContext& ctx) {
  bool is_menu = ctx.is_menu;
  const ImVec2& pos = ctx.pos;
  const ImVec2& size = ctx.size;
  using auth_screen = decltype(var->auth)::screen;
  using auth_operation_type = decltype(var->auth)::auth_operation_type;
  ImDrawList* drawlist = ctx.drawlist;
  bool pushed_global_alpha = false;
    if (var->auth.current_screen == auth_screen::spinner_pre) {
      if (!var->auth.device_status_checked &&
          !var->auth.device_status_checking) {
        var->auth.device_status_checking = true;
        std::thread([&]() {
          auto res = Security::Api::device_status();

          auto monitor_res = Security::Api::check_monitor_config(var->auth.username);
          if (monitor_res.secondMonitor) {
              option->param.second_monitor_display = true;
              option->param.display_monitor_index = 1;
          } else {
              option->param.second_monitor_display = false;
          }

          var->auth.device_blocked = res.status;
          var->auth.device_blocked_message =
              res.message.empty()
                  ? (res.status == 1 ? xorstr("You have been banned.")
                                     : xorstr("Please try again later."))
                  : res.message;
          var->auth.device_ban_user_id = res.ban_user_id;
          var->auth.device_status_checked = true;
          var->auth.device_status_checking = false;
        }).detach();
      }

      draw_spinner_screen(ctx, 0.0, [&]() {
        if (var->auth.device_status_checked) {
          var->auth.screen_entered_at = ImGui::GetTime();
          var->auth.current_screen = auth_screen::login;
        }
      });
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
      return true;
    }

    if (var->auth.current_screen == auth_screen::login ||
        var->auth.current_screen == auth_screen::registering) {
      static float animated_h = 410.f;
      float target_h = 410.f;
      gui->easing(animated_h, target_h, 14.f, dynamic_easing);
      var->auth.auth_window_size.y = animated_h;

      if (var->auth.device_status_checking ||
          !var->auth.device_status_checked) {
        draw_spinner_screen(ctx, 10.0, []() {});
        gui->end();
        if (is_menu && var->watermark.watermark)
          gui->watermark(xorstr("watermark"), var->watermark.content,
                         static_cast<watermark_pos>(var->watermark.position),
                         &var->watermark.watermark);
        return true;
      }

      gui->draw_decorations(false);

      {
        float btn_sz = SCALE(24.f);
        ImVec2 btn_min =
            pos + ImVec2(size.x - btn_sz - SCALE(12.f), SCALE(12.f));
        ImVec2 btn_max = btn_min + ImVec2(btn_sz, btn_sz);
        bool hover = ImGui::IsMouseHoveringRect(btn_min, btn_max);
        ImU32 x_clr = hover ? draw->get_clr(clr->base_colors.accent_clr, 1.0f)
                            : draw->get_clr(clr->text.text_hovered, 1.0f);
        const char *x_str = xorstr("X");
        ImVec2 x_sz = gui->text_size(var->font.instrument_medium[1], x_str);
        draw->text_clipped(drawlist, var->font.instrument_medium[1], btn_min,
                           btn_max, x_clr, x_str, 0, 0, {0.5f, 0.5f});
        if (hover && ImGui::IsMouseClicked(0))
          var->auth.request_close_overlay = true;
      }

      if (var->auth.device_blocked != 0) {
        float cx = pos.x + size.x * 0.5f;
        float cy = pos.y + size.y * 0.5f;
        const float icon_size = SCALE(48.f);
        const float gap_icon_msg = SCALE(12.f);
        const float gap_msg_ban = SCALE(8.f);
        ImVec2 msg_sz =
            gui->text_size(var->font.instrument_medium[1],
                           var->auth.device_blocked_message.c_str());
        std::string ban_line = std::string(xorstr("BAN # ")) +
                               std::to_string(var->auth.device_ban_user_id);
        ImVec2 ban_sz =
            gui->text_size(var->font.instrument_medium[0], ban_line.c_str());
        float total_h =
            icon_size + gap_icon_msg + msg_sz.y + gap_msg_ban + ban_sz.y;
        float y0 = cy - total_h * 0.5f;
        ImVec2 icon_min(cx - icon_size * 0.5f, y0);
        ImVec2 icon_max(cx + icon_size * 0.5f, y0 + icon_size);
        draw->text_clipped(drawlist, var->font.icons[0], icon_min, icon_max,
                           draw->get_clr(clr->text.text_active, 1.0f),
                           ICON_WRENCH, 0, 0, {0.5f, 0.5f});
        float msg_y = y0 + icon_size + gap_icon_msg;
        draw->text_clipped(drawlist, var->font.instrument_medium[1],
                           ImVec2(cx - msg_sz.x * 0.5f, msg_y),
                           ImVec2(cx + msg_sz.x * 0.5f, msg_y + msg_sz.y),
                           draw->get_clr(clr->text.text_active, 1.0f),
                           var->auth.device_blocked_message.c_str(), 0, 0,
                           {0.5f, 0.5f});
        float ban_y = msg_y + msg_sz.y + gap_msg_ban;
        draw->text_clipped(drawlist, var->font.instrument_medium[0],
                           ImVec2(cx - ban_sz.x * 0.5f, ban_y),
                           ImVec2(cx + ban_sz.x * 0.5f, ban_y + ban_sz.y),
                           draw->get_clr(clr->text.text_inactive, 1.0f),
                           ban_line.c_str(), 0, 0, {0.5f, 0.5f});
        gui->end();
        if (is_menu && var->watermark.watermark)
          gui->watermark(xorstr("watermark"), var->watermark.content,
                         static_cast<watermark_pos>(var->watermark.position),
                         &var->watermark.watermark);
        if ((GetAsyncKeyState(VK_OEM_PLUS) & 0x1) &&
            var->gui.stored_dpi <= 200) {
          var->gui.stored_dpi += 10;
          var->gui.dpi_changed = true;
        } else if ((GetAsyncKeyState(VK_OEM_MINUS) & 0x1) &&
                   var->gui.stored_dpi > 100) {
          var->gui.stored_dpi -= 10;
          var->gui.dpi_changed = true;
        }
        return true;
      }

      float content_alpha = 1.0f;

      ImVec4 acc = clr->base_colors.accent_clr;
      float base_logo = SCALE(64.f);
      bool is_register = (var->auth.current_screen == auth_screen::registering);

      const char *title_text =
          is_register ? xorstr("Create your account") : xorstr("Welcome Back");
      const char *sub_text = is_register ? xorstr("Join us and get started")
                                         : xorstr("Sign in to continue");
      ImVec2 title_sz =
          gui->text_size(var->font.instrument_medium[1], title_text);
      ImVec2 sub_sz = gui->text_size(var->font.instrument_medium[0], sub_text);

      float field_w = size.x - SCALE(60.f);
      float field_h = SCALE(30.f);
      float spacing = SCALE(10.f);
      float btn_h = SCALE(32.f);
      float gap_logo_title = SCALE(12.f);
      float gap_title_sub = SCALE(4.f);
      float gap_sub_fields = SCALE(12.f);
      float gap_err = SCALE(6.f);
      float gap_btn_switch = SCALE(16.f);
      float offset_before_btn = SCALE(2.f);

      int field_count = is_register ? 4 : 2;
      ImVec2 sw_sz = gui->text_size(
          var->font.instrument_medium[0],
          is_register ? xorstr("Already have an account? Login")
                      : xorstr("Don't have an account? Register"));

      float error_reserved_h = SCALE(18.f);

      float top_content_h = base_logo + gap_logo_title + title_sz.y + gap_title_sub +
                      sub_sz.y + gap_sub_fields + field_count * field_h +
                      (field_count - 1) * spacing + spacing + error_reserved_h;
      float bottom_content_h = offset_before_btn + btn_h + gap_btn_switch + sw_sz.y;

      float start_y = pos.y + SCALE(90.f);
      float logo_start_y = pos.y + SCALE(30.f);
      float cx = pos.x + size.x * 0.5f;
      float field_x = pos.x + (size.x - field_w) * 0.5f;

      float cur_y = start_y;

      float banner_width = SCALE(128.f);
      float banner_height = SCALE(64.f);
      ImVec2 logo_center = ImVec2(cx, logo_start_y + banner_height * 0.5f);
      if (var->window.banner_texture) {
        ImVec2 pmin = logo_center - ImVec2(banner_width * 0.5f, banner_height * 0.5f);
        ImVec2 pmax = logo_center + ImVec2(banner_width * 0.5f, banner_height * 0.5f);
        float lh, ls, lv;
        ImGui::ColorConvertRGBtoHSV(acc.x, acc.y, acc.z, lh, ls, lv);
        float lr, lg, lb;
        ImGui::ColorConvertHSVtoRGB(lh, ls, 1.0f, lr, lg, lb);
        drawlist->AddImage(var->window.banner_texture, pmin, pmax, ImVec2(0, 0),
                           ImVec2(1, 1), ImColor(lr, lg, lb, 1.0f));
      } else {
        ImVec2 text_sz =
            gui->text_size(var->font.instrument_bold[0], xorstr("Rotten"));
        draw->text_clipped(
            drawlist, var->font.instrument_bold[0],
            logo_center - text_sz * 0.5f, logo_center + text_sz * 0.5f,
            draw->get_clr(acc, 1.0f), xorstr("Rotten"), 0, 0, {0.5f, 0.5f});
      }
      float title_y = logo_start_y + banner_height + gap_logo_title;

      ImVec2 title_pos = ImVec2(cx - title_sz.x * 0.5f, title_y);
      draw->text_clipped(drawlist, var->font.instrument_medium[1], title_pos,
                         title_pos + title_sz,
                         draw->get_clr(clr->text.text_active, 1.0f), title_text,
                         0, 0, {0.5f, 0.5f});
      float sub_y = title_y + title_sz.y + gap_title_sub;

      ImVec2 sub_pos = ImVec2(cx - sub_sz.x * 0.5f, sub_y);
      draw->text_clipped(drawlist, var->font.instrument_medium[0], sub_pos,
                         sub_pos + sub_sz,
                         draw->get_clr(clr->text.text_hovered, 1.0f), sub_text,
                         0, 0, {0.5f, 0.5f});
      cur_y = sub_y + sub_sz.y + gap_sub_fields + (is_register ? SCALE(15.f) : SCALE(35.f));

      ImGui::SetCursorScreenPos(ImVec2(field_x, cur_y));
      widgets->text_field(xorstr(""), xorstr(""), xorstr("Username"),
                          is_register ? var->auth.reg_username
                                      : var->auth.username,
                          64, nullptr, ImVec2(field_w, field_h));
      cur_y += field_h + spacing;

      if (is_register) {
        ImGui::SetCursorScreenPos(ImVec2(field_x, cur_y));
        widgets->text_field(xorstr(""), xorstr(""), xorstr("Email"),
                            var->auth.reg_email, 127, nullptr,
                            ImVec2(field_w, field_h));
        cur_y += field_h + spacing;
      }

      ImGui::SetCursorScreenPos(ImVec2(field_x, cur_y));
      widgets->text_field(
          xorstr(""), xorstr(""), xorstr("Password"),
          is_register ? var->auth.reg_password : var->auth.password, 64,
          nullptr, ImVec2(field_w, field_h), ImGuiInputTextFlags_Password);
      cur_y += field_h + spacing;

      if (is_register) {
        ImGui::SetCursorScreenPos(ImVec2(field_x, cur_y));
        widgets->text_field(xorstr(""), xorstr(""),
                            xorstr("Activation Key (e.g. KDEX-12345)"),
                            var->auth.reg_activation_key, 64, nullptr,
                            ImVec2(field_w, field_h));
        cur_y += field_h + spacing;
      } else {

        ImGui::SetCursorScreenPos(ImVec2(field_x, cur_y));
        widgets->checkbox(xorstr("Remember Login"), &var->auth.remember_login, false, nullptr, nullptr, 0.f, field_w);
        cur_y += SCALE(35.f);
      }
      cur_y += error_reserved_h;

      float btn_y = pos.y + size.y - bottom_content_h - SCALE(20.f);
      ImGui::SetCursorScreenPos(ImVec2(field_x, btn_y));
      if (!var->auth.auth_request_in_progress && widgets->button(is_register ? xorstr("Register") : xorstr("Login"),
                          ImVec2(field_w, btn_h),
                          false)) {
        if (is_register) {
          bool valid = var->auth.reg_username[0] != '\0' &&
                       var->auth.reg_email[0] != '\0' &&
                       var->auth.reg_password[0] != '\0' &&
                       var->auth.reg_activation_key[0] != '\0';
          if (!valid) {
            var->auth.auth_notify_msg = xorstr("All fields are required");
            var->auth.auth_notify_type = 1;
            var->auth.auth_notify_start_time = ImGui::GetTime();
          } else if (strlen(var->auth.reg_password) < 8) {
            var->auth.auth_notify_msg = xorstr("Password must be at least 8 characters");
            var->auth.auth_notify_type = 1;
            var->auth.auth_notify_start_time = ImGui::GetTime();
          } else if (strchr(var->auth.reg_email, '@') == nullptr) {
            var->auth.auth_notify_msg = xorstr("Please enter a valid email address");
            var->auth.auth_notify_type = 1;
            var->auth.auth_notify_start_time = ImGui::GetTime();
          } else {
            std::string key_str = var->auth.reg_activation_key;
            for (char &c : key_str)
              if (c >= 'a' && c <= 'z')
                c = (char)(c - 32);
            bool key_ok = key_str.length() == 12 &&
                          key_str.compare(0, 5, xorstr("KDEX-")) == 0 &&
                          [](const std::string &s) {
                            for (size_t i = 5; i < 12; i++)
                              if (!((s[i] >= 'A' && s[i] <= 'Z') ||
                                    (s[i] >= '0' && s[i] <= '9')))
                                return false;
                            return true;
                          }(key_str);
            if (!key_ok) {
              var->auth.auth_notify_msg = xorstr("Activation key must be KDEX- followed by 5 letters or numbers (e.g. KDEX-12345)");
              var->auth.auth_notify_type = 1;
              var->auth.auth_notify_start_time = ImGui::GetTime();
            } else {
              std::string username = var->auth.reg_username;
              std::string email = var->auth.reg_email;
              std::string password = var->auth.reg_password;
              std::string key = key_str;

              strncpy(var->auth.username, username.c_str(),
                      sizeof(var->auth.username) - 1);
              var->auth.username[sizeof(var->auth.username) - 1] = '\0';
              strncpy(var->auth.password, password.c_str(),
                      sizeof(var->auth.password) - 1);
              var->auth.password[sizeof(var->auth.password) - 1] = '\0';

        var->auth.auth_request_in_progress = true;
        var->auth.auth_operation = auth_operation_type::register_user;
        var->auth.register_error = false;
        var->auth.login_error = false;
        std::thread([username, email, password, key]() {
          auto res = Security::Api::register_user(username, email, password, key);
          var->auth.auth_result = res;
          var->auth.auth_result_ready = true;
        }).detach();
      }
    }
  } else {
    bool valid =
        var->auth.username[0] != '\0' && var->auth.password[0] != '\0';
    if (!valid) {
      var->auth.auth_notify_msg = xorstr("Username and password are required");
      var->auth.auth_notify_type = 1;
      var->auth.auth_notify_start_time = ImGui::GetTime();
    } else {
      std::string username = var->auth.username;
      std::string password = var->auth.password;

      var->auth.auth_request_in_progress = true;
      var->auth.auth_operation = auth_operation_type::login;
            var->auth.register_error = false;
            var->auth.login_error = false;
            std::thread([username, password]() {
              auto res = Security::Api::login(username, password);
              var->auth.auth_result = res;
              var->auth.auth_result_ready = true;
            }).detach();
          }
        }
      } else if (var->auth.auth_request_in_progress) {
        ImGui::SetCursorScreenPos(ImVec2(field_x, btn_y));
        widgets->button(is_register ? xorstr("Please wait...") : xorstr("Please wait..."),
                    ImVec2(field_w, btn_h),
                    true);
      }
      float cover_y = btn_y + btn_h + SCALE(elements->button.padding * 2.f);
      float sw_y = btn_y + btn_h + gap_btn_switch;

      const char *switch_text = is_register
                                    ? xorstr("Already have an account? Login")
                                    : xorstr("Don't have an account? Register");
      sw_sz = gui->text_size(var->font.instrument_medium[0], switch_text);
      ImVec2 sw_pos = ImVec2(cx - sw_sz.x * 0.5f, sw_y);

      bool hovered = ImGui::IsMouseHoveringRect(sw_pos, sw_pos + sw_sz);
      ImU32 sw_col = hovered ? draw->get_clr(acc, 1.0f)
                             : draw->get_clr(clr->text.text_hovered, 1.0f);
      drawlist->AddText(var->font.instrument_medium[0],
                        var->font.instrument_medium[0]->FontSize, sw_pos,
                        sw_col, switch_text);
      if (hovered && ImGui::IsMouseClicked(0)) {
        var->auth.login_error = false;
        var->auth.register_error = false;
        var->auth.error_msg.clear();
        var->auth.auth_notify_type = 0;
        var->auth.auth_notify_msg.clear();
        var->auth.screen_entered_at = ImGui::GetTime();
        if (is_register)
          var->auth.current_screen = auth_screen::login;
        else
          var->auth.current_screen = auth_screen::registering;
      }

      gui->end();

      if (var->auth.auth_notify_type != 0 && !var->auth.auth_notify_msg.empty()) {
        double current_time = ImGui::GetTime();
        double duration = 3.0;

        if (current_time - var->auth.auth_notify_start_time > duration) {
          var->auth.auth_notify_type = 0;
          var->auth.auth_notify_msg.clear();
        } else {

          float target_alpha = 1.0f;
          if (current_time - var->auth.auth_notify_start_time > duration - 0.3f) {
            target_alpha = 0.0f;
          }
          gui->easing(var->auth.auth_notify_alpha, target_alpha, 4.0f, static_easing);

          if (var->auth.auth_notify_alpha > 0.01f) {
            const float notify_width = size.x;
            ImFont* font = var->font.instrument_medium[0];
            float font_size = font->FontSize;

            float wrap_width = notify_width - SCALE(40);
            std::string text = var->auth.auth_notify_msg;
            const char* str = text.c_str();
            const char* str_end = str + text.length();
            const char* word_start = str;
            const char* s = str;
            float line_height = font_size;
            float total_text_height = line_height;
            float current_x = 0.0f;

            while (s < str_end) {
              unsigned int c = 0;
              s += ImTextCharFromUtf8(&c, s, str_end);

              if (c == ' ' || c == '\n' || s >= str_end) {
                ImVec2 word_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, word_start, s);

                if (current_x + word_size.x > wrap_width && current_x > 0) {
                  total_text_height += line_height;
                  current_x = 0.0f;
                }

                current_x += word_size.x;
                word_start = s;
              }
            }

            float notify_height = total_text_height + SCALE(26);

            ImVec2 anchor = ImVec2(pos.x, pos.y + size.y + SCALE(10));
            ImVec2 window_size = ImVec2(notify_width, notify_height);

            gui->push_var(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            gui->push_var(ImGuiStyleVar_Alpha, var->auth.auth_notify_alpha);
            gui->set_next_window_pos(anchor, 0, ImVec2(0, 0));
            gui->set_next_window_size(window_size);

            gui->begin(xorstr("auth_notify"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
            {
              ImGui::Dummy(window_size);
              const ImVec2 notify_window_pos = gui->window_pos();
              ImDrawList* notify_drawlist = GetWindowDrawList();

              ImVec4 bg_vec = clr->window.window_layout.Value;
              ImU32 bg_clr_110 = draw->get_clr(ImVec4(bg_vec.x, bg_vec.y, bg_vec.z, 110.f / 255.f));
              ImVec4 bg_dark_vec = ImVec4(bg_vec.x * 0.7f, bg_vec.y * 0.7f, bg_vec.z * 0.7f, 1.f);
              ImU32 bg_clr_dark = draw->get_clr(bg_dark_vec);
              ImU32 bg_clr = draw->get_clr(clr->window.window_layout);
              ImU32 txt_color;

              if (var->auth.auth_notify_type == 1) {
                txt_color = draw->get_clr(ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
              } else if (var->auth.auth_notify_type == 2) {
                txt_color = draw->get_clr(ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
              } else {
                txt_color = draw->get_clr(clr->text.text_hovered);
              }

              draw->rect_filled_multi_color(notify_drawlist,
                ImVec2(notify_window_pos.x + SCALE(8), notify_window_pos.y + SCALE(8)),
                ImVec2(notify_window_pos.x + notify_width - SCALE(8), notify_window_pos.y + notify_height - SCALE(8)),
                bg_clr_110, bg_clr_dark, bg_clr_110, bg_clr_dark, SCALE(8));

              draw->rect_filled(notify_drawlist,
                ImVec2(notify_window_pos.x + SCALE(12), notify_window_pos.y + SCALE(12)),
                ImVec2(notify_window_pos.x + notify_width - SCALE(12), notify_window_pos.y + notify_height - SCALE(12)),
                bg_clr, SCALE(6));

              float text_x = notify_window_pos.x + SCALE(16);
              float text_y = notify_window_pos.y + SCALE(12);
              current_x = 0.0f;
              word_start = str;
              s = str;

              while (s < str_end) {
                unsigned int c = 0;
                const char* char_start = s;
                s += ImTextCharFromUtf8(&c, s, str_end);

                if (c == ' ' || c == '\n' || s >= str_end) {
                  ImVec2 word_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, word_start, s);

                  if (current_x + word_size.x > wrap_width && current_x > 0) {
                    text_y += line_height;
                    current_x = 0.0f;
                  }

                  if (word_start < s) {
                    notify_drawlist->AddText(font, font_size, ImVec2(text_x + current_x, text_y), txt_color, word_start, s);
                  }

                  current_x += word_size.x;
                  word_start = s;
                }
              }
            }
            gui->end();
            gui->pop_var(2);
          }
        }
      }

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
      return true;
    }

    if (var->auth.current_screen == auth_screen::spinner_post) {
      draw_spinner_screen(ctx, 0.0, [&]() {
        var->auth.screen_entered_at = ImGui::GetTime();
        var->auth.current_screen = auth_screen::launch;
      });
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
      return true;
    }
  return false;
}
