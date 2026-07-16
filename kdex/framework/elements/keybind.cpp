#include "../settings/functions.h"
#include "../../game/Security/xorstr.hpp"

namespace {
	static char s_key_name_buf[32];

	static const char* get_key_name(int index) {
		switch (index) {
		case 0:  strcpy_s(s_key_name_buf, xorstr("None")); return s_key_name_buf;
		case 1:  strcpy_s(s_key_name_buf, xorstr("Mouse 1")); return s_key_name_buf;
		case 2:  strcpy_s(s_key_name_buf, xorstr("Mouse 2")); return s_key_name_buf;
		case 3:  strcpy_s(s_key_name_buf, xorstr("CN")); return s_key_name_buf;
		case 4:  strcpy_s(s_key_name_buf, xorstr("Mouse 3")); return s_key_name_buf;
		case 5:  strcpy_s(s_key_name_buf, xorstr("Mouse 4")); return s_key_name_buf;
		case 6:  strcpy_s(s_key_name_buf, xorstr("Mouse 5")); return s_key_name_buf;
		case 7:  strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 8:  strcpy_s(s_key_name_buf, xorstr("Back")); return s_key_name_buf;
		case 9:  strcpy_s(s_key_name_buf, xorstr("Tab")); return s_key_name_buf;
		case 10: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 11: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 12: strcpy_s(s_key_name_buf, xorstr("CLR")); return s_key_name_buf;
		case 13: strcpy_s(s_key_name_buf, xorstr("Enter")); return s_key_name_buf;
		case 14: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 15: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 16: strcpy_s(s_key_name_buf, xorstr("Shift")); return s_key_name_buf;
		case 17: strcpy_s(s_key_name_buf, xorstr("CTL")); return s_key_name_buf;
		case 18: strcpy_s(s_key_name_buf, xorstr("Menu")); return s_key_name_buf;
		case 19: strcpy_s(s_key_name_buf, xorstr("Pause")); return s_key_name_buf;
		case 20: strcpy_s(s_key_name_buf, xorstr("Caps")); return s_key_name_buf;
		case 21: strcpy_s(s_key_name_buf, xorstr("KAN")); return s_key_name_buf;
		case 22: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 23: strcpy_s(s_key_name_buf, xorstr("JUN")); return s_key_name_buf;
		case 24: strcpy_s(s_key_name_buf, xorstr("FIN")); return s_key_name_buf;
		case 25: strcpy_s(s_key_name_buf, xorstr("KAN")); return s_key_name_buf;
		case 26: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 27: strcpy_s(s_key_name_buf, xorstr("Escape")); return s_key_name_buf;
		case 28: strcpy_s(s_key_name_buf, xorstr("CON")); return s_key_name_buf;
		case 29: strcpy_s(s_key_name_buf, xorstr("NCO")); return s_key_name_buf;
		case 30: strcpy_s(s_key_name_buf, xorstr("ACC")); return s_key_name_buf;
		case 31: strcpy_s(s_key_name_buf, xorstr("MAD")); return s_key_name_buf;
		case 32: strcpy_s(s_key_name_buf, xorstr("Space")); return s_key_name_buf;
		case 33: strcpy_s(s_key_name_buf, xorstr("PGU")); return s_key_name_buf;
		case 34: strcpy_s(s_key_name_buf, xorstr("PGD")); return s_key_name_buf;
		case 35: strcpy_s(s_key_name_buf, xorstr("End")); return s_key_name_buf;
		case 36: strcpy_s(s_key_name_buf, xorstr("Home")); return s_key_name_buf;
		case 37: strcpy_s(s_key_name_buf, xorstr("Left")); return s_key_name_buf;
		case 38: strcpy_s(s_key_name_buf, xorstr("Up")); return s_key_name_buf;
		case 39: strcpy_s(s_key_name_buf, xorstr("Right")); return s_key_name_buf;
		case 40: strcpy_s(s_key_name_buf, xorstr("Down")); return s_key_name_buf;
		case 41: strcpy_s(s_key_name_buf, xorstr("SEL")); return s_key_name_buf;
		case 42: strcpy_s(s_key_name_buf, xorstr("PRI")); return s_key_name_buf;
		case 43: strcpy_s(s_key_name_buf, xorstr("EXE")); return s_key_name_buf;
		case 44: strcpy_s(s_key_name_buf, xorstr("PRI")); return s_key_name_buf;
		case 45: strcpy_s(s_key_name_buf, xorstr("INS")); return s_key_name_buf;
		case 46: strcpy_s(s_key_name_buf, xorstr("Delete")); return s_key_name_buf;
		case 47: strcpy_s(s_key_name_buf, xorstr("HEL")); return s_key_name_buf;
		case 48: strcpy_s(s_key_name_buf, xorstr("0")); return s_key_name_buf;
		case 49: strcpy_s(s_key_name_buf, xorstr("1")); return s_key_name_buf;
		case 50: strcpy_s(s_key_name_buf, xorstr("2")); return s_key_name_buf;
		case 51: strcpy_s(s_key_name_buf, xorstr("3")); return s_key_name_buf;
		case 52: strcpy_s(s_key_name_buf, xorstr("4")); return s_key_name_buf;
		case 53: strcpy_s(s_key_name_buf, xorstr("5")); return s_key_name_buf;
		case 54: strcpy_s(s_key_name_buf, xorstr("6")); return s_key_name_buf;
		case 55: strcpy_s(s_key_name_buf, xorstr("7")); return s_key_name_buf;
		case 56: strcpy_s(s_key_name_buf, xorstr("8")); return s_key_name_buf;
		case 57: strcpy_s(s_key_name_buf, xorstr("9")); return s_key_name_buf;
		case 58: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 59: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 60: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 61: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 62: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 63: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 64: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 65: strcpy_s(s_key_name_buf, xorstr("A")); return s_key_name_buf;
		case 66: strcpy_s(s_key_name_buf, xorstr("B")); return s_key_name_buf;
		case 67: strcpy_s(s_key_name_buf, xorstr("C")); return s_key_name_buf;
		case 68: strcpy_s(s_key_name_buf, xorstr("D")); return s_key_name_buf;
		case 69: strcpy_s(s_key_name_buf, xorstr("E")); return s_key_name_buf;
		case 70: strcpy_s(s_key_name_buf, xorstr("F")); return s_key_name_buf;
		case 71: strcpy_s(s_key_name_buf, xorstr("G")); return s_key_name_buf;
		case 72: strcpy_s(s_key_name_buf, xorstr("H")); return s_key_name_buf;
		case 73: strcpy_s(s_key_name_buf, xorstr("I")); return s_key_name_buf;
		case 74: strcpy_s(s_key_name_buf, xorstr("J")); return s_key_name_buf;
		case 75: strcpy_s(s_key_name_buf, xorstr("K")); return s_key_name_buf;
		case 76: strcpy_s(s_key_name_buf, xorstr("L")); return s_key_name_buf;
		case 77: strcpy_s(s_key_name_buf, xorstr("M")); return s_key_name_buf;
		case 78: strcpy_s(s_key_name_buf, xorstr("N")); return s_key_name_buf;
		case 79: strcpy_s(s_key_name_buf, xorstr("O")); return s_key_name_buf;
		case 80: strcpy_s(s_key_name_buf, xorstr("P")); return s_key_name_buf;
		case 81: strcpy_s(s_key_name_buf, xorstr("Q")); return s_key_name_buf;
		case 82: strcpy_s(s_key_name_buf, xorstr("R")); return s_key_name_buf;
		case 83: strcpy_s(s_key_name_buf, xorstr("S")); return s_key_name_buf;
		case 84: strcpy_s(s_key_name_buf, xorstr("T")); return s_key_name_buf;
		case 85: strcpy_s(s_key_name_buf, xorstr("U")); return s_key_name_buf;
		case 86: strcpy_s(s_key_name_buf, xorstr("V")); return s_key_name_buf;
		case 87: strcpy_s(s_key_name_buf, xorstr("W")); return s_key_name_buf;
		case 88: strcpy_s(s_key_name_buf, xorstr("X")); return s_key_name_buf;
		case 89: strcpy_s(s_key_name_buf, xorstr("Y")); return s_key_name_buf;
		case 90: strcpy_s(s_key_name_buf, xorstr("Z")); return s_key_name_buf;
		case 91: strcpy_s(s_key_name_buf, xorstr("WIN")); return s_key_name_buf;
		case 92: strcpy_s(s_key_name_buf, xorstr("WIN")); return s_key_name_buf;
		case 93: strcpy_s(s_key_name_buf, xorstr("APP")); return s_key_name_buf;
		case 94: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 95: strcpy_s(s_key_name_buf, xorstr("SLE")); return s_key_name_buf;
		case 96: strcpy_s(s_key_name_buf, xorstr("Num 0")); return s_key_name_buf;
		case 97: strcpy_s(s_key_name_buf, xorstr("Num 1")); return s_key_name_buf;
		case 98: strcpy_s(s_key_name_buf, xorstr("Num 2")); return s_key_name_buf;
		case 99: strcpy_s(s_key_name_buf, xorstr("Num 3")); return s_key_name_buf;
		case 100: strcpy_s(s_key_name_buf, xorstr("Num 4")); return s_key_name_buf;
		case 101: strcpy_s(s_key_name_buf, xorstr("Num 5")); return s_key_name_buf;
		case 102: strcpy_s(s_key_name_buf, xorstr("Num 6")); return s_key_name_buf;
		case 103: strcpy_s(s_key_name_buf, xorstr("Num 7")); return s_key_name_buf;
		case 104: strcpy_s(s_key_name_buf, xorstr("Num 8")); return s_key_name_buf;
		case 105: strcpy_s(s_key_name_buf, xorstr("Num 9")); return s_key_name_buf;
		case 106: strcpy_s(s_key_name_buf, xorstr("MUL")); return s_key_name_buf;
		case 107: strcpy_s(s_key_name_buf, xorstr("ADD")); return s_key_name_buf;
		case 108: strcpy_s(s_key_name_buf, xorstr("SEP")); return s_key_name_buf;
		case 109: strcpy_s(s_key_name_buf, xorstr("MIN")); return s_key_name_buf;
		case 110: strcpy_s(s_key_name_buf, xorstr("Delete")); return s_key_name_buf;
		case 111: strcpy_s(s_key_name_buf, xorstr("DIV")); return s_key_name_buf;
		case 112: strcpy_s(s_key_name_buf, xorstr("F1")); return s_key_name_buf;
		case 113: strcpy_s(s_key_name_buf, xorstr("F2")); return s_key_name_buf;
		case 114: strcpy_s(s_key_name_buf, xorstr("F3")); return s_key_name_buf;
		case 115: strcpy_s(s_key_name_buf, xorstr("F4")); return s_key_name_buf;
		case 116: strcpy_s(s_key_name_buf, xorstr("F5")); return s_key_name_buf;
		case 117: strcpy_s(s_key_name_buf, xorstr("F6")); return s_key_name_buf;
		case 118: strcpy_s(s_key_name_buf, xorstr("F7")); return s_key_name_buf;
		case 119: strcpy_s(s_key_name_buf, xorstr("F8")); return s_key_name_buf;
		case 120: strcpy_s(s_key_name_buf, xorstr("F9")); return s_key_name_buf;
		case 121: strcpy_s(s_key_name_buf, xorstr("F10")); return s_key_name_buf;
		case 122: strcpy_s(s_key_name_buf, xorstr("F11")); return s_key_name_buf;
		case 123: strcpy_s(s_key_name_buf, xorstr("F12")); return s_key_name_buf;
		case 124: strcpy_s(s_key_name_buf, xorstr("F13")); return s_key_name_buf;
		case 125: strcpy_s(s_key_name_buf, xorstr("F14")); return s_key_name_buf;
		case 126: strcpy_s(s_key_name_buf, xorstr("F15")); return s_key_name_buf;
		case 127: strcpy_s(s_key_name_buf, xorstr("F16")); return s_key_name_buf;
		case 128: strcpy_s(s_key_name_buf, xorstr("F17")); return s_key_name_buf;
		case 129: strcpy_s(s_key_name_buf, xorstr("F18")); return s_key_name_buf;
		case 130: strcpy_s(s_key_name_buf, xorstr("F19")); return s_key_name_buf;
		case 131: strcpy_s(s_key_name_buf, xorstr("F20")); return s_key_name_buf;
		case 132: strcpy_s(s_key_name_buf, xorstr("F21")); return s_key_name_buf;
		case 133: strcpy_s(s_key_name_buf, xorstr("F22")); return s_key_name_buf;
		case 134: strcpy_s(s_key_name_buf, xorstr("F23")); return s_key_name_buf;
		case 135: strcpy_s(s_key_name_buf, xorstr("F24")); return s_key_name_buf;
		case 136: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 137: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 138: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 139: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 140: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 141: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 142: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 143: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 144: strcpy_s(s_key_name_buf, xorstr("NUM")); return s_key_name_buf;
		case 145: strcpy_s(s_key_name_buf, xorstr("SCR")); return s_key_name_buf;
		case 146: strcpy_s(s_key_name_buf, xorstr("EQU")); return s_key_name_buf;
		case 147: strcpy_s(s_key_name_buf, xorstr("MAS")); return s_key_name_buf;
		case 148: strcpy_s(s_key_name_buf, xorstr("TOY")); return s_key_name_buf;
		case 149: strcpy_s(s_key_name_buf, xorstr("OYA")); return s_key_name_buf;
		case 150: strcpy_s(s_key_name_buf, xorstr("OYA")); return s_key_name_buf;
		case 151: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 152: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 153: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 154: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 155: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 156: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 157: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 158: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 159: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		case 160: strcpy_s(s_key_name_buf, xorstr("Shift")); return s_key_name_buf;
		case 161: strcpy_s(s_key_name_buf, xorstr("Shift")); return s_key_name_buf;
		case 162: strcpy_s(s_key_name_buf, xorstr("Ctrl")); return s_key_name_buf;
		case 163: strcpy_s(s_key_name_buf, xorstr("Ctrl")); return s_key_name_buf;
		case 164: strcpy_s(s_key_name_buf, xorstr("Alt")); return s_key_name_buf;
		case 165: strcpy_s(s_key_name_buf, xorstr("Alt")); return s_key_name_buf;
		default: strcpy_s(s_key_name_buf, xorstr("-")); return s_key_name_buf;
		}
	}
}

static const char* keys(int index) { return get_key_name(index); }

bool c_widgets::key_select(std::string_view label, int* key)
{
    static int last_frame = -1;
    static bool g_previous_keys[256] = { false };
    static bool g_current_keys[256] = { false };
    if (last_frame != ImGui::GetFrameCount()) {
        for (int i = 0; i < 256; i++) {
            g_previous_keys[i] = g_current_keys[i];
            g_current_keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
        }
        last_frame = ImGui::GetFrameCount();
    }

    struct keybind_state
    {
        ImVec4 text = clr->base_colors.white_clr;
        float alpha = 0.f;
    };

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const float width = gui->content_avail().x;
    const float rect_size = SCALE(elements->keybind.keybind_size);

    const ImRect total_bb(pos, pos + ImVec2(width, rect_size));

    ItemSize(ImRect(total_bb.Min, total_bb.Max + ImVec2(0, SCALE(3))), 0.f);
    if (!ItemAdd(total_bb, id, &total_bb)) return false;

    char buf_display[64];
    strcpy_s(buf_display, xorstr("-"));

    bool value_changed = false;
    int k = *key;

    std::string active_key = "";
    active_key += keys(*key);

    if (*key != 0 && g.ActiveId != id) {
        strcpy_s(buf_display, active_key.c_str());
    }
    else if (g.ActiveId == id) {
        strcpy_s(buf_display, xorstr("..."));
    }

    bool hovered = ItemHoverable(total_bb, id, 0);

    if (hovered && GetIO().MouseClicked[0])
    {
        if (g.ActiveId != id) {
            memset(GetIO().MouseDown, 0, sizeof(GetIO().MouseDown));
            memset(GetIO().KeysDown, 0, sizeof(GetIO().KeysDown));
            *key = 0;
        }
        ImGui::SetActiveID(id, window);
        ImGui::FocusWindow(window);
    }
    else if (GetIO().MouseClicked[0]) {

        if (g.ActiveId == id) ImGui::ClearActiveID();
    }

    if (g.ActiveId == id) {
        const int key_escape = 0x1B;

        if (g_current_keys[key_escape] && !g_previous_keys[key_escape]) {
            *key = 0;
            value_changed = true;
            ImGui::ClearActiveID();
        }
        else {
            for (auto i = 0; i < 5; i++) {
                if (GetIO().MouseClicked[i]) {
                    if (i == 0 && hovered) continue;
                    switch (i) {
                    case 0: k = 0x01; break;
                    case 1: k = 0x02; break;
                    case 2: k = 0x04; break;
                    case 3: k = 0x05; break;
                    case 4: k = 0x06; break;
                    }
                    value_changed = true;
                    ImGui::ClearActiveID();
                }
            }

            if (!value_changed) {
                for (auto i = 0x08; i <= 0xA5; i++) {
                    if (i == key_escape) continue;
                    if (g_current_keys[i] && !g_previous_keys[i]) {
                        k = i;
                        value_changed = true;
                        ImGui::ClearActiveID();
                    }
                }
            }

            if (value_changed) {
                *key = k;
            }
        }
    }

    keybind_state* state = gui->anim_container(&state, id);
    gui->easing(state->text, g.ActiveId == id ? clr->base_colors.black_clr.Value : clr->base_colors.white_clr.Value, 16.f, dynamic_easing);
    gui->easing(state->alpha, hovered ? 0.35f : 0.05f, 16.f, dynamic_easing);

    ImVec4 accent = clr->base_colors.accent_clr.Value;
    float r_val = ImClamp(accent.x, 0.0f, 1.0f);
    float g_val = ImClamp(accent.y, 0.0f, 1.0f);
    float b_val = ImClamp(accent.z, 0.0f, 1.0f);
    float gradient_alpha = state->alpha * 0.7f;
    ImU32 col_top = IM_COL32((int)(r_val * 255), (int)(g_val * 255), (int)(b_val * 255), (int)(255 * gradient_alpha));
    ImU32 col_bot = IM_COL32((int)(r_val * 255), (int)(g_val * 255), (int)(b_val * 255), 0);
    ImU32 border_col = IM_COL32(80, 80, 80, 80);

    draw->rounded_gradient_rect(window->DrawList, total_bb.Min, total_bb.Max, col_top, col_bot, border_col, SCALE(elements->keybind.rounding));

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], total_bb.Min, total_bb.Max, draw->get_clr(state->text), buf_display, 0, 0, { 0.5, 0.5 });

    float separator_y = total_bb.Min.y + SCALE(elements->keybind.keybind_size + elements->keybind.padding + 2);
    float separator_width = (total_bb.Max.x - total_bb.Min.x) * 0.75f;
    float separator_start_x = total_bb.Min.x + (total_bb.Max.x - total_bb.Min.x - separator_width) / 2.0f;
    float separator_end_x = separator_start_x + separator_width;
    draw->line(window->DrawList, ImVec2(separator_start_x, separator_y), ImVec2(separator_end_x, separator_y), draw->get_clr(clr->window.separator), 1.f);

    return value_changed;
}
