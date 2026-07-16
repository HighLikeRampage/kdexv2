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

void c_gui::draw_spinner_screen(const GuiFrameContext& ctx, double duration,
                                const std::function<void()>& on_done) {
  const ImVec2& pos = ctx.pos;
  const ImVec2& size = ctx.size;
  ImDrawList* drawlist = ctx.drawlist;

      if (var->auth.screen_t0 == 0.0)
        var->auth.screen_t0 = ImGui::GetTime();
      double dt = ImGui::GetTime() - var->auth.screen_t0;

      float spinner_alpha = 1.0f;

      gui->draw_decorations(false);
      ImVec2 c =
          pos + ImVec2(size.x * 0.5f,
                       size.y * 0.5f + SCALE(40.f));
      float t = (float)ImGui::GetTime();
      float base_logo = SCALE(70.f);
      float vertical_offset =
          SCALE(75.f);
      ImVec4 acc = clr->base_colors.accent_clr;
      ImVec4 w = ImVec4(1.f, 1.f, 1.f, 1.f);
      auto lerp4 = [](const ImVec4 &a, const ImVec4 &b, float k) {
        return ImVec4(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
                      a.z + (b.z - a.z) * k, 1.f);
      };
      ImU32 col0 = draw->get_clr(acc, spinner_alpha);
      ImU32 col1 = draw->get_clr(lerp4(acc, w, 0.4f), spinner_alpha);
      ImU32 faint = draw->get_clr(acc, 0.15f * spinner_alpha);

      if (var->window.banner_texture) {
        float spinner_banner_width = SCALE(140.f);
        float spinner_banner_height = SCALE(70.f);
        ImVec2 logo_center = c + ImVec2(0.f, -vertical_offset);
        ImVec2 pmin = logo_center - ImVec2(spinner_banner_width * 0.5f, spinner_banner_height * 0.5f);
        ImVec2 pmax = logo_center + ImVec2(spinner_banner_width * 0.5f, spinner_banner_height * 0.5f);
        {
          float lh, ls, lv;
          ImGui::ColorConvertRGBtoHSV(acc.x, acc.y, acc.z, lh, ls, lv);
          float lr, lg, lb;
          ImGui::ColorConvertHSVtoRGB(lh, ls, 1.0f, lr, lg, lb);
          drawlist->AddImage(var->window.banner_texture, pmin, pmax, ImVec2(0, 0),
                             ImVec2(1, 1), ImColor(lr, lg, lb, spinner_alpha));
        }
      } else {
        ImVec2 text_sz =
            gui->text_size(var->font.instrument_bold[0], xorstr("Rotten"));
        ImVec2 logo_center = c + ImVec2(0.f, -vertical_offset);
        draw->text_clipped(drawlist, var->font.instrument_bold[0],
                           logo_center - text_sz * 0.5f,
                           logo_center + text_sz * 0.5f,
                           draw->get_clr(acc, spinner_alpha), xorstr("Rotten"),
                           0, 0, {0.5f, 0.5f});
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
        ImU32 col = draw->get_clr(lerp4(acc, w, 0.08f * k), alpha * spinner_alpha);
        drawlist->AddLine(p1, p2, col, thick);
      }

      drawlist->AddCircleFilled(head_pos, th_max * 0.55f,
                                draw->get_clr(acc, spinner_alpha), 20);
      drawlist->AddCircleFilled(head_pos, th_max * 0.95f,
                                draw->get_clr(acc, 0.22f * spinner_alpha), 24);
      drawlist->AddCircleFilled(head_pos, th_max * 0.35f,
                                draw->get_clr(lerp4(acc, w, 0.6f), spinner_alpha), 16);

      drawlist->AddCircleFilled(tail_pos, th_min * 0.55f,
                                draw->get_clr(acc, 0.35f * spinner_alpha), 12);

      double effective_duration = duration;
      if (effective_duration < 3.5) effective_duration = 3.5;
      if (dt >= effective_duration) {
        var->auth.screen_t0 = 0.0;
        on_done();
      }
}
