#pragma once

#include "../settings/functions.h"
#include "../game/Security/xorstr.hpp"

inline void gui_handle_dpi_hotkeys() {
  if ((GetAsyncKeyState(VK_OEM_PLUS) & 0x1) && var->gui.stored_dpi <= 200) {
    var->gui.stored_dpi += 10;
    var->gui.dpi_changed = true;
  } else if ((GetAsyncKeyState(VK_OEM_MINUS) & 0x1) && var->gui.stored_dpi > 100) {
    var->gui.stored_dpi -= 10;
    var->gui.dpi_changed = true;
  }
}

inline void gui_draw_watermark_if_menu(const c_gui::GuiFrameContext& ctx) {
  if (ctx.is_menu && var->watermark.watermark)
    gui->watermark(xorstr("watermark"), var->watermark.content,
                   static_cast<watermark_pos>(var->watermark.position),
                   &var->watermark.watermark);
}
