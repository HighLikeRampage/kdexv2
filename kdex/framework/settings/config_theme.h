#pragma once
#include "../../game/Security/Api/json.hpp"
#include "imgui.h"
#include "colors.h"
#include "elements.h"
#include "variables.h"
#include "../../game/Security/xorstr.hpp"
#include <string>

namespace ConfigTheme {

inline nlohmann::json ImColToJson(const ImColor& c) {
    return nlohmann::json::array({ c.Value.x, c.Value.y, c.Value.z, c.Value.w });
}
inline ImColor JsonToImCol(const nlohmann::json& j) {
    if (j.is_array() && j.size() == 4)
        return ImColor((float)j[0], (float)j[1], (float)j[2], (float)j[3]);
    return ImColor(0, 0, 0, 1);
}

inline nlohmann::json GetThemeConfigJson() {
    nlohmann::json j;
    if (!clr || !elements || !var) return j;
    auto& colors = j[xorstr("Colors")];
    auto& base = colors[xorstr("base_colors")];
    base[xorstr("accent_clr")] = ImColToJson(clr->base_colors.accent_clr);
    base[xorstr("white_clr")] = ImColToJson(clr->base_colors.white_clr);
    base[xorstr("black_clr")] = ImColToJson(clr->base_colors.black_clr);
    base[xorstr("warning_clr")] = ImColToJson(clr->base_colors.warning_clr);
    base[xorstr("error_clr")] = ImColToJson(clr->base_colors.error_clr);
    base[xorstr("success_clr")] = ImColToJson(clr->base_colors.success_clr);
    auto& win = colors[xorstr("window")];
    win[xorstr("window_layout")] = ImColToJson(clr->window.window_layout);
    win[xorstr("window_shadow")] = ImColToJson(clr->window.window_shadow);
    win[xorstr("window_section")] = ImColToJson(clr->window.window_section);
    win[xorstr("window_stroke")] = ImColToJson(clr->window.window_stroke);
    win[xorstr("scrollbar_clr")] = ImColToJson(clr->window.scrollbar_clr);
    win[xorstr("separator")] = ImColToJson(clr->window.separator);
    auto& child = colors[xorstr("child")];
    child[xorstr("child_header")] = ImColToJson(clr->child.child_header);
    child[xorstr("child_layout")] = ImColToJson(clr->child.child_layout);
    child[xorstr("child_stroke")] = ImColToJson(clr->child.child_stroke);
    auto& sect = colors[xorstr("section")];
    sect[xorstr("layout")] = ImColToJson(clr->section.layout);
    auto& widget = colors[xorstr("widget")];
    widget[xorstr("layout")] = ImColToJson(clr->widget.layout);
    widget[xorstr("stroke")] = ImColToJson(clr->widget.stroke);
    widget[xorstr("dropdown_selected")] = ImColToJson(clr->widget.dropdown_selected);
    widget[xorstr("checkbox_circle")] = ImColToJson(clr->widget.checkbox_circle);
    auto& text = colors[xorstr("text")];
    text[xorstr("text_active")] = ImColToJson(clr->text.text_active);
    text[xorstr("text_hovered")] = ImColToJson(clr->text.text_hovered);
    text[xorstr("text_inactive")] = ImColToJson(clr->text.text_inactive);
    auto& lua = colors[xorstr("lua")];
    lua[xorstr("background")] = ImColToJson(clr->lua.background);
    lua[xorstr("title")] = ImColToJson(clr->lua.title);
    lua[xorstr("text")] = ImColToJson(clr->lua.text);
    lua[xorstr("desc")] = ImColToJson(clr->lua.desc);
    lua[xorstr("stroke")] = ImColToJson(clr->lua.stroke);
    lua[xorstr("red")] = ImColToJson(clr->lua.red);

    auto& vars = j[xorstr("Variables")];
    vars[xorstr("gui")][xorstr("hints_of_func")] = var->gui.hints_of_func;
    vars[xorstr("gui")][xorstr("show_status_func")] = var->gui.show_status_func;
    vars[xorstr("gui")][xorstr("accent_color_menu")] = var->gui.accent_color_menu;
    vars[xorstr("gui")][xorstr("dpi")] = var->gui.dpi;
    vars[xorstr("gui")][xorstr("accent_clr")] = nlohmann::json::array({ var->gui.accent_clr[0], var->gui.accent_clr[1], var->gui.accent_clr[2], var->gui.accent_clr[3] });
    vars[xorstr("gui")][xorstr("stored_dpi")] = var->gui.stored_dpi;
    vars[xorstr("window")][xorstr("window_rounding")] = var->window.window_rounding;
    vars[xorstr("window")][xorstr("sidebar_minimized")] = var->window.sidebar_minimized;
    vars[xorstr("window")][xorstr("sidebar_width")] = var->window.sidebar_width;
    vars[xorstr("search")][xorstr("layout_alpha")] = var->search.layout_alpha;
    vars[xorstr("search")][xorstr("search_active")] = var->search.search_active;

    vars[xorstr("auth")][xorstr("remember_login")] = var->auth.remember_login;
    if (var->auth.remember_login) {
        vars[xorstr("auth")][xorstr("username")] = var->auth.username;
        vars[xorstr("auth")][xorstr("access_token")] = var->auth.access_token;
        vars[xorstr("auth")][xorstr("refresh_token")] = var->auth.refresh_token;
    }

    auto& el = j[xorstr("Elements")];
    el[xorstr("section")][xorstr("rounding")] = elements->section.rounding;
    el[xorstr("section")][xorstr("section_height")] = elements->section.section_height;
    el[xorstr("section")][xorstr("text_padding")] = nlohmann::json::array({ elements->section.text_padding[0], elements->section.text_padding[1] });
    el[xorstr("section")][xorstr("section_alpha")] = elements->section.section_alpha;
    el[xorstr("section")][xorstr("sub_section_alpha")] = elements->section.sub_section_alpha;
    el[xorstr("child")][xorstr("child_padding")] = nlohmann::json::array({ elements->child.child_padding.x, elements->child.child_padding.y });
    el[xorstr("child")][xorstr("child_spacing")] = nlohmann::json::array({ elements->child.child_spacing.x, elements->child.child_spacing.y });
    el[xorstr("child")][xorstr("header_height")] = elements->child.header_height;
    el[xorstr("child")][xorstr("rounding")] = elements->child.rounding;
    el[xorstr("checkbox")][xorstr("rounding")] = elements->checkbox.rounding;
    el[xorstr("checkbox")][xorstr("height_size")] = elements->checkbox.height_size;
    el[xorstr("slider")][xorstr("rounding")] = elements->slider.rounding;
    el[xorstr("slider")][xorstr("height_size")] = elements->slider.height_size;
    el[xorstr("dropdown")][xorstr("selection_height")] = elements->dropdown.selection_height;
    el[xorstr("dropdown")][xorstr("rounding")] = elements->dropdown.rounding;
    el[xorstr("notify")][xorstr("rounding")] = elements->notify.rounding;
    el[xorstr("color_edit")][xorstr("height_size")] = elements->color_edit.height_size;
    el[xorstr("color_edit")][xorstr("rounding")] = elements->color_edit.rounding;
    el[xorstr("button")][xorstr("rounding")] = elements->button.rounding;
    el[xorstr("keybind")][xorstr("rounding")] = elements->keybind.rounding;
    el[xorstr("listbox")][xorstr("rounding")] = elements->listbox.rounding;
    return j;
}

inline void ApplyThemeFromConfigJson(const nlohmann::json& j) {
    if (!clr || !elements || !var) return;
    if (j.contains(xorstr("Colors"))) {
        const auto& c = j[xorstr("Colors")];
        if (c.contains(xorstr("base_colors"))) {
            const auto& b = c[xorstr("base_colors")];
            if (b.contains(xorstr("accent_clr"))) clr->base_colors.accent_clr = JsonToImCol(b[xorstr("accent_clr")]);
            if (b.contains(xorstr("white_clr"))) clr->base_colors.white_clr = JsonToImCol(b[xorstr("white_clr")]);
            if (b.contains(xorstr("black_clr"))) clr->base_colors.black_clr = JsonToImCol(b[xorstr("black_clr")]);
            if (b.contains(xorstr("warning_clr"))) clr->base_colors.warning_clr = JsonToImCol(b[xorstr("warning_clr")]);
            if (b.contains(xorstr("error_clr"))) clr->base_colors.error_clr = JsonToImCol(b[xorstr("error_clr")]);
            if (b.contains(xorstr("success_clr"))) clr->base_colors.success_clr = JsonToImCol(b[xorstr("success_clr")]);
        }
        if (c.contains(xorstr("window"))) {
            const auto& w = c[xorstr("window")];
            if (w.contains(xorstr("window_layout"))) clr->window.window_layout = JsonToImCol(w[xorstr("window_layout")]);
            if (w.contains(xorstr("window_shadow"))) clr->window.window_shadow = JsonToImCol(w[xorstr("window_shadow")]);
            if (w.contains(xorstr("window_section"))) clr->window.window_section = JsonToImCol(w[xorstr("window_section")]);
            if (w.contains(xorstr("window_stroke"))) clr->window.window_stroke = JsonToImCol(w[xorstr("window_stroke")]);
            if (w.contains(xorstr("scrollbar_clr"))) clr->window.scrollbar_clr = JsonToImCol(w[xorstr("scrollbar_clr")]);
            if (w.contains(xorstr("separator"))) clr->window.separator = JsonToImCol(w[xorstr("separator")]);
        }
        if (c.contains(xorstr("child"))) {
            const auto& ch = c[xorstr("child")];
            if (ch.contains(xorstr("child_header"))) clr->child.child_header = JsonToImCol(ch[xorstr("child_header")]);
            if (ch.contains(xorstr("child_layout"))) clr->child.child_layout = JsonToImCol(ch[xorstr("child_layout")]);
            if (ch.contains(xorstr("child_stroke"))) clr->child.child_stroke = JsonToImCol(ch[xorstr("child_stroke")]);
        }
        if (c.contains(xorstr("section")) && c[xorstr("section")].contains(xorstr("layout"))) clr->section.layout = JsonToImCol(c[xorstr("section")][xorstr("layout")]);
        if (c.contains(xorstr("widget"))) {
            const auto& w = c[xorstr("widget")];
            if (w.contains(xorstr("layout"))) clr->widget.layout = JsonToImCol(w[xorstr("layout")]);
            if (w.contains(xorstr("stroke"))) clr->widget.stroke = JsonToImCol(w[xorstr("stroke")]);
            if (w.contains(xorstr("dropdown_selected"))) clr->widget.dropdown_selected = JsonToImCol(w[xorstr("dropdown_selected")]);
            if (w.contains(xorstr("checkbox_circle"))) clr->widget.checkbox_circle = JsonToImCol(w[xorstr("checkbox_circle")]);
        }
        if (c.contains(xorstr("text"))) {
            const auto& t = c[xorstr("text")];
            if (t.contains(xorstr("text_active"))) clr->text.text_active = JsonToImCol(t[xorstr("text_active")]);
            if (t.contains(xorstr("text_hovered"))) clr->text.text_hovered = JsonToImCol(t[xorstr("text_hovered")]);
            if (t.contains(xorstr("text_inactive"))) clr->text.text_inactive = JsonToImCol(t[xorstr("text_inactive")]);
        }
        if (c.contains(xorstr("lua"))) {
            const auto& l = c[xorstr("lua")];
            if (l.contains(xorstr("background"))) clr->lua.background = JsonToImCol(l[xorstr("background")]);
            if (l.contains(xorstr("title"))) clr->lua.title = JsonToImCol(l[xorstr("title")]);
            if (l.contains(xorstr("text"))) clr->lua.text = JsonToImCol(l[xorstr("text")]);
            if (l.contains(xorstr("desc"))) clr->lua.desc = JsonToImCol(l[xorstr("desc")]);
            if (l.contains(xorstr("stroke"))) clr->lua.stroke = JsonToImCol(l[xorstr("stroke")]);
            if (l.contains(xorstr("red"))) clr->lua.red = JsonToImCol(l[xorstr("red")]);
        }
    }
    if (j.contains(xorstr("Variables"))) {
        const auto& v = j[xorstr("Variables")];
        if (v.contains(xorstr("gui"))) {
            const auto& g = v[xorstr("gui")];
            if (g.contains(xorstr("hints_of_func"))) var->gui.hints_of_func = g[xorstr("hints_of_func")];
            if (g.contains(xorstr("show_status_func"))) var->gui.show_status_func = g[xorstr("show_status_func")];
            if (g.contains(xorstr("accent_color_menu"))) var->gui.accent_color_menu = g[xorstr("accent_color_menu")];
            if (g.contains(xorstr("dpi"))) var->gui.dpi = g[xorstr("dpi")];
            if (g.contains(xorstr("accent_clr")) && g[xorstr("accent_clr")].is_array() && g[xorstr("accent_clr")].size() >= 4) {
                var->gui.accent_clr[0] = g[xorstr("accent_clr")][0];
                var->gui.accent_clr[1] = g[xorstr("accent_clr")][1];
                var->gui.accent_clr[2] = g[xorstr("accent_clr")][2];
                var->gui.accent_clr[3] = g[xorstr("accent_clr")][3];
            }
            if (g.contains(xorstr("stored_dpi"))) var->gui.stored_dpi = g[xorstr("stored_dpi")];
        }
        if (v.contains(xorstr("window"))) {
            const auto& w = v[xorstr("window")];
            if (w.contains(xorstr("window_rounding"))) var->window.window_rounding = w[xorstr("window_rounding")];
            if (w.contains(xorstr("sidebar_minimized"))) var->window.sidebar_minimized = w[xorstr("sidebar_minimized")];
            if (w.contains(xorstr("sidebar_width"))) var->window.sidebar_width = w[xorstr("sidebar_width")];
        }
        if (v.contains(xorstr("search"))) {
            const auto& s = v[xorstr("search")];
            if (s.contains(xorstr("layout_alpha"))) var->search.layout_alpha = s[xorstr("layout_alpha")];
            if (s.contains(xorstr("search_active"))) var->search.search_active = s[xorstr("search_active")];
        }

        if (v.contains(xorstr("auth"))) {
            const auto& a = v[xorstr("auth")];
            if (a.contains(xorstr("remember_login"))) var->auth.remember_login = a[xorstr("remember_login")];
            if (var->auth.remember_login) {
                if (a.contains(xorstr("username"))) {
                    std::string username_str = a[xorstr("username")];
                    strncpy_s(var->auth.username, username_str.c_str(), _TRUNCATE);
                }
                if (a.contains(xorstr("access_token"))) var->auth.access_token = a[xorstr("access_token")];
                if (a.contains(xorstr("refresh_token"))) var->auth.refresh_token = a[xorstr("refresh_token")];
                if (!var->auth.access_token.empty()) {
                    var->auth.authenticated = true;
                }
            }
        }
    }
    if (j.contains(xorstr("Elements"))) {
        const auto& e = j[xorstr("Elements")];
        if (e.contains(xorstr("section"))) {
            const auto& s = e[xorstr("section")];
            if (s.contains(xorstr("rounding"))) elements->section.rounding = s[xorstr("rounding")];
            if (s.contains(xorstr("section_height"))) elements->section.section_height = s[xorstr("section_height")];
            if (s.contains(xorstr("text_padding")) && s[xorstr("text_padding")].is_array() && s[xorstr("text_padding")].size() >= 2) {
                elements->section.text_padding[0] = s[xorstr("text_padding")][0];
                elements->section.text_padding[1] = s[xorstr("text_padding")][1];
            }
            if (s.contains(xorstr("section_alpha"))) elements->section.section_alpha = s[xorstr("section_alpha")];
            if (s.contains(xorstr("sub_section_alpha"))) elements->section.sub_section_alpha = s[xorstr("sub_section_alpha")];
        }
        if (e.contains(xorstr("child"))) {
            const auto& ch = e[xorstr("child")];
            if (ch.contains(xorstr("child_padding")) && ch[xorstr("child_padding")].is_array() && ch[xorstr("child_padding")].size() >= 2) {
                elements->child.child_padding.x = ch[xorstr("child_padding")][0];
                elements->child.child_padding.y = ch[xorstr("child_padding")][1];
            }
            if (ch.contains(xorstr("child_spacing")) && ch[xorstr("child_spacing")].is_array() && ch[xorstr("child_spacing")].size() >= 2) {
                elements->child.child_spacing.x = ch[xorstr("child_spacing")][0];
                elements->child.child_spacing.y = ch[xorstr("child_spacing")][1];
            }
            if (ch.contains(xorstr("header_height"))) elements->child.header_height = ch[xorstr("header_height")];
            if (ch.contains(xorstr("rounding"))) elements->child.rounding = ch[xorstr("rounding")];
        }
        if (e.contains(xorstr("checkbox"))) {
            const auto& cb = e[xorstr("checkbox")];
            if (cb.contains(xorstr("rounding"))) elements->checkbox.rounding = cb[xorstr("rounding")];
            if (cb.contains(xorstr("height_size"))) elements->checkbox.height_size = cb[xorstr("height_size")];
        }
        if (e.contains(xorstr("slider"))) {
            const auto& sl = e[xorstr("slider")];
            if (sl.contains(xorstr("rounding"))) elements->slider.rounding = sl[xorstr("rounding")];
            if (sl.contains(xorstr("height_size"))) elements->slider.height_size = sl[xorstr("height_size")];
        }
        if (e.contains(xorstr("dropdown"))) {
            const auto& d = e[xorstr("dropdown")];
            if (d.contains(xorstr("selection_height"))) elements->dropdown.selection_height = d[xorstr("selection_height")];
            if (d.contains(xorstr("rounding"))) elements->dropdown.rounding = d[xorstr("rounding")];
        }
        if (e.contains(xorstr("notify")) && e[xorstr("notify")].contains(xorstr("rounding"))) elements->notify.rounding = e[xorstr("notify")][xorstr("rounding")];
        if (e.contains(xorstr("color_edit"))) {
            const auto& ce = e[xorstr("color_edit")];
            if (ce.contains(xorstr("height_size"))) elements->color_edit.height_size = ce[xorstr("height_size")];
            if (ce.contains(xorstr("rounding"))) elements->color_edit.rounding = ce[xorstr("rounding")];
        }
        if (e.contains(xorstr("button")) && e[xorstr("button")].contains(xorstr("rounding"))) elements->button.rounding = e[xorstr("button")][xorstr("rounding")];
        if (e.contains(xorstr("keybind")) && e[xorstr("keybind")].contains(xorstr("rounding"))) elements->keybind.rounding = e[xorstr("keybind")][xorstr("rounding")];
        if (e.contains(xorstr("listbox")) && e[xorstr("listbox")].contains(xorstr("rounding"))) elements->listbox.rounding = e[xorstr("listbox")][xorstr("rounding")];
    }
}

}
