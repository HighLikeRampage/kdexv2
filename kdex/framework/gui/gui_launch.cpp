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

bool c_gui::render_launch_screen(const GuiFrameContext& ctx) {
  bool is_menu = ctx.is_menu;
  const ImVec2& pos = ctx.pos;
  const ImVec2& size = ctx.size;
  using auth_screen = decltype(var->auth)::screen;
  using auth_operation_type = decltype(var->auth)::auth_operation_type;
  ImDrawList* drawlist = ctx.drawlist;
  bool pushed_global_alpha = false;
    if (var->auth.current_screen == auth_screen::launch) {
      float launch_content_alpha = 1.0f;

      if (var->auth.launch_state >= 1 && var->auth.launch_state <= 2) {
        gui->draw_decorations(false);
        {
          float btn_sz = SCALE(24.f);
          ImVec2 btn_min =
              pos + ImVec2(size.x - btn_sz - SCALE(12.f), SCALE(12.f));
          ImVec2 btn_max = btn_min + ImVec2(btn_sz, btn_sz);
          bool hover = ImGui::IsMouseHoveringRect(btn_min, btn_max);
          ImU32 x_clr = hover ? draw->get_clr(clr->base_colors.accent_clr, 1.0f)
                              : draw->get_clr(clr->text.text_hovered, 1.0f);
          draw->text_clipped(drawlist, var->font.instrument_medium[1], btn_min,
                             btn_max, x_clr, xorstr("X"), 0, 0, {0.5f, 0.5f});
          if (hover && ImGui::IsMouseClicked(0))
            var->auth.request_close_overlay = true;
        }
        ImVec2 c =
            pos + ImVec2(size.x * 0.5f,
                         size.y * 0.5f + SCALE(40.f));
        float t = (float)ImGui::GetTime();
        float base_logo = SCALE(70.f);
        float vertical_offset =
            SCALE(75.f);
        ImVec4 acc_sp = clr->base_colors.accent_clr;
        ImVec4 w = ImVec4(1.f, 1.f, 1.f, 1.f);
        auto lerp4 = [](const ImVec4 &a, const ImVec4 &b, float k) {
          return ImVec4(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
                        a.z + (b.z - a.z) * k, 1.f);
        };
        ImU32 faint = draw->get_clr(acc_sp, 0.15f);
        if (var->window.banner_texture) {
          float launch_banner_width = SCALE(140.f);
          float launch_banner_height = SCALE(70.f);
          ImVec2 logo_center = c + ImVec2(0.f, -vertical_offset);
          ImVec2 pmin =
              logo_center - ImVec2(launch_banner_width * 0.5f, launch_banner_height * 0.5f);
          ImVec2 pmax =
              logo_center + ImVec2(launch_banner_width * 0.5f, launch_banner_height * 0.5f);
          float lh, ls, lv;
          ImGui::ColorConvertRGBtoHSV(acc_sp.x, acc_sp.y, acc_sp.z, lh, ls, lv);
          float lr, lg, lb;
          ImGui::ColorConvertHSVtoRGB(lh, ls, 1.0f, lr, lg, lb);
          drawlist->AddImage(var->window.banner_texture, pmin, pmax, ImVec2(0, 0),
                             ImVec2(1, 1), ImColor(lr, lg, lb, 1.0f));
        } else {
          ImVec2 text_sz =
              gui->text_size(var->font.instrument_bold[0], xorstr("Rotten"));
          ImVec2 logo_center = c + ImVec2(0.f, -vertical_offset);
          draw->text_clipped(drawlist, var->font.instrument_bold[0],
                             logo_center - text_sz * 0.5f,
                             logo_center + text_sz * 0.5f,
                             draw->get_clr(acc_sp, 1.0f), xorstr("Rotten"), 0,
                             0, {0.5f, 0.5f});
        }
        ImVec2 sc = c;
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
          ImU32 col = draw->get_clr(lerp4(acc_sp, w, 0.08f * k), alpha);
          drawlist->AddLine(p1, p2, col, thick);
        }

        drawlist->AddCircleFilled(head_pos, th_max * 0.55f,
                                  draw->get_clr(acc_sp, 1.0f), 20);
        drawlist->AddCircleFilled(head_pos, th_max * 0.95f,
                                  draw->get_clr(acc_sp, 0.22f), 24);
        drawlist->AddCircleFilled(head_pos, th_max * 0.35f,
                                  draw->get_clr(lerp4(acc_sp, w, 0.6f), 1.0f),
                                  16);

        drawlist->AddCircleFilled(tail_pos, th_min * 0.55f,
                                  draw->get_clr(acc_sp, 0.35f), 12);
        int dot_count = (int)(ImGui::GetTime() * 2.5f) % 4;
        char wait_buf[32];
        snprintf(wait_buf, sizeof(wait_buf), xorstr("Awaiting%.*s"), dot_count,
                 xorstr("...."));
        ImVec2 msg_sz =
            gui->text_size(var->font.instrument_medium[0], wait_buf);
        float msg_y = sc.y + SCALE(50.f);
        draw->text_clipped(drawlist, var->font.instrument_medium[0],
                           ImVec2(c.x - msg_sz.x * 0.5f, msg_y),
                           ImVec2(c.x + msg_sz.x * 0.5f, msg_y + msg_sz.y),
                           draw->get_clr(clr->text.text_hovered, 1.0f),
                           wait_buf, 0, 0, {0.5f, 0.5f});
        if (pushed_global_alpha) {
          ImGui::PopStyleVar();
          pushed_global_alpha = false;
        }
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
        draw->text_clipped(drawlist, var->font.instrument_medium[1], btn_min,
                           btn_max, x_clr, x_str, 0, 0, {0.5f, 0.5f});
        if (hover && ImGui::IsMouseClicked(0))
          var->auth.request_close_overlay = true;
      }

      ImVec4 acc = clr->base_colors.accent_clr;
      float cx = pos.x + size.x * 0.5f;
      float line_h = SCALE(20.f);
      float spacing = SCALE(6.f);
      float cur_y = pos.y + SCALE(40.f);

      const char *title = xorstr("Ready to Launch");
      ImVec2 title_sz = gui->text_size(var->font.instrument_bold[0], title);
      draw->text_clipped(drawlist, var->font.instrument_bold[0],
                         ImVec2(cx - title_sz.x * 0.5f, cur_y),
                         ImVec2(cx + title_sz.x * 0.5f, cur_y + title_sz.y),
                         draw->get_clr(acc, 1.0f), title, 0, 0, {0.5f, 0.5f});
      cur_y += title_sz.y + spacing;

      line_h = SCALE(22.f);
      spacing = SCALE(22.f);
      cur_y = pos.y + SCALE(75.f);

      const char *uname_label = xorstr("Username:");
      ImVec2 ul_sz =
          gui->text_size(var->font.instrument_medium[0], uname_label);
      draw->text_clipped(drawlist, var->font.instrument_medium[0],
                         ImVec2(pos.x + SCALE(20.f), cur_y),
                         ImVec2(pos.x + size.x - SCALE(20.f), cur_y + ul_sz.y),
                         draw->get_clr(clr->text.text_hovered, 1.0f),
                         uname_label, 0, 0, {0.0f, 0.5f});
      cur_y += line_h;
      draw->text_clipped(drawlist, var->font.instrument_medium[1],
                         ImVec2(pos.x + SCALE(20.f), cur_y),
                         ImVec2(pos.x + size.x - SCALE(20.f), cur_y + line_h),
                         draw->get_clr(clr->text.text_active, 1.0f),
                         var->auth.username, 0, 0, {0.0f, 0.5f});
      cur_y += line_h + spacing;

      std::time_t now = std::time(nullptr);
      std::tm *lt = std::localtime(&now);
      char datetime_buf[64];
      std::strftime(datetime_buf, sizeof(datetime_buf), "%Y-%m-%d  %H:%M:%S",
                    lt);
      const char *dt_label = xorstr("Date & Time:");
      ImVec2 dt_sz = gui->text_size(var->font.instrument_medium[0], dt_label);
      draw->text_clipped(drawlist, var->font.instrument_medium[0],
                         ImVec2(pos.x + SCALE(20.f), cur_y),
                         ImVec2(pos.x + size.x - SCALE(20.f), cur_y + dt_sz.y),
                         draw->get_clr(clr->text.text_hovered, 1.0f), dt_label,
                         0, 0, {0.0f, 0.5f});
      cur_y += line_h;
      draw->text_clipped(drawlist, var->font.instrument_medium[1],
                         ImVec2(pos.x + SCALE(20.f), cur_y),
                         ImVec2(pos.x + size.x - SCALE(20.f), cur_y + line_h),
                         draw->get_clr(clr->text.text_active, 1.0f),
                         datetime_buf, 0, 0, {0.0f, 0.5f});
      cur_y += line_h + spacing;

      const char *days_label = xorstr("Days Left:");
      ImVec2 dl_sz = gui->text_size(var->font.instrument_medium[0], days_label);
      draw->text_clipped(drawlist, var->font.instrument_medium[0],
                         ImVec2(pos.x + SCALE(20.f), cur_y),
                         ImVec2(pos.x + size.x - SCALE(20.f), cur_y + dl_sz.y),
                         draw->get_clr(clr->text.text_hovered, 1.0f),
                         days_label, 0, 0, {0.0f, 0.5f});
      cur_y += line_h;
      char days_val[32];
      snprintf(days_val, sizeof(days_val), "%d",
               var->auth.subscription_days_left >= 0
                   ? var->auth.subscription_days_left
                   : 0);
      draw->text_clipped(drawlist, var->font.instrument_medium[1],
                         ImVec2(pos.x + SCALE(20.f), cur_y),
                         ImVec2(pos.x + size.x - SCALE(20.f), cur_y + line_h),
                         draw->get_clr(clr->base_colors.accent_clr, 1.0f),
                         days_val, 0, 0, {0.0f, 0.5f});

      cur_y = pos.y + size.y - SCALE(105.f);

      if (var->auth.launch_state == 0) {
        float btn_w = size.x - SCALE(40.f);
        float btn_h = SCALE(36.f);

        ImGui::SetCursorScreenPos(ImVec2(pos.x + SCALE(20.f), cur_y));
        widgets->checkbox(option->name.stream_proof, &option->param.stream_proof, false, nullptr, nullptr, 0.f, btn_w);
        cur_y += SCALE(45.f);

        ImGui::SetCursorScreenPos(
            ImVec2(pos.x + (size.x - btn_w) * 0.5f, cur_y));
        if (widgets->button(xorstr("Launch"), ImVec2(btn_w, btn_h), false)) {
          var->auth.launch_clicked = true;
        }
      }

      if (pushed_global_alpha) {
        ImGui::PopStyleVar();
        pushed_global_alpha = false;
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
      return true;
    }
  return false;
}
