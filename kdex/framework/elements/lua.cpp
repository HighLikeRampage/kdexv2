#include "../settings/functions.h"

bool load_button(std::string_view name, bool* active)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(name.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + SCALE(elements->lua.load_size));

    bool hovered = IsMouseHoveringRect(rect.Min, rect.Max);
    bool pressed = hovered && GetIO().MouseClicked[0];

    float* state = gui->anim_container(&state, id);

    gui->easing(*state, *active ? 1.f : 0.f, 8.f, static_easing);

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->lua.title), SCALE(elements->lua.rounding));
    draw->rect(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->lua.stroke), SCALE(elements->lua.rounding));
    draw->rect_filled(window->DrawList, rect.GetCenter() - SCALE(3, 3), rect.GetCenter() + SCALE(3, 3), draw->get_clr(clr->lua.desc), SCALE(1));

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->base_colors.accent_clr, 1.f - *state), SCALE(elements->lua.rounding));
    draw->circle_filled(window->DrawList, rect.GetCenter(), SCALE(3), draw->get_clr(clr->lua.text, 1.f - *state));

    return pressed;
}

bool f_button(std::string_view fid, std::string_view name, bool red)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(fid.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + SCALE(elements->lua.button_size));

    bool hovered = IsMouseHoveringRect(rect.Min, rect.Max);
    bool pressed = hovered && GetIO().MouseClicked[0];

    ImVec4* state = gui->anim_container(&state, id);

    gui->easing(*state, hovered ? clr->lua.text.Value : clr->lua.desc.Value, 12.f, dynamic_easing);

    draw->text_clipped(window->DrawList, var->font.icons[4], rect.Min, rect.Max, draw->get_clr(red ? clr->lua.red.Value : *state), name.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    return pressed;
}

bool c_widgets::lua(std::string_view name, std::string_view desc, bool* active)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(name.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + SCALE(elements->lua.size));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    {
        ImU32 col_top = IM_COL32(255, 255, 255, (int)(255 * 0.05f));
        ImU32 col_bot = IM_COL32(50, 50, 50, (int)(255 * 0.02f));
        ImU32 border_col = IM_COL32(80, 80, 80, 80);

        draw->rounded_gradient_rect(window->DrawList, rect.Min, rect.Max, col_top, col_bot, border_col, SCALE(elements->lua.rounding));
    }

    draw->line(window->DrawList, rect.Min + SCALE(0, elements->lua.title_size), ImVec2(rect.Max.x, rect.Min.y + SCALE(elements->lua.title_size)), draw->get_clr(clr->lua.stroke), SCALE(1));

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min + SCALE(10, 0), ImVec2(rect.Max.x, rect.Min.y + SCALE(elements->lua.title_size)), draw->get_clr(clr->lua.text), name.data(), NULL, NULL, ImVec2(0.f, 0.5f));
    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min + SCALE(10, elements->lua.title_size + 10), rect.Max, draw->get_clr(clr->lua.desc), desc.data(), NULL, NULL, ImVec2(0.f, 0.f));

    ImVec2 stored_pos = gui->get_screen_pos();

    gui->set_screen_pos(rect.Max - SCALE(elements->lua.load_size + ImVec2(10, 10)), pos_all);
    if (load_button((std::stringstream{} << name << "load_button").str(), active))
        *active = !*active;

    gui->set_screen_pos(ImVec2(rect.Min.x + SCALE(10), rect.Max.y - SCALE(12 + elements->lua.button_size.y)), pos_all);
    f_button((std::stringstream{} << name << "one_button").str(), "Q", true);

    gui->set_screen_pos(ImVec2(rect.Min.x + SCALE(30), rect.Max.y - SCALE(12 + elements->lua.button_size.y)), pos_all);
    f_button((std::stringstream{} << name << "two_button").str(), "K", false);

    gui->set_screen_pos(ImVec2(rect.Min.x + SCALE(50), rect.Max.y - SCALE(12 + elements->lua.button_size.y)), pos_all);
    f_button((std::stringstream{} << name << "three_button").str(), "C", false);

    gui->set_screen_pos(ImVec2(rect.Min.x + SCALE(70), rect.Max.y - SCALE(12 + elements->lua.button_size.y)), pos_all);
    if (f_button((std::stringstream{} << name << "four_button").str(), "A", false))
        elements->section.section_count = 6;

    gui->set_screen_pos(stored_pos, pos_all);

    return true;
}

#include <unordered_set>

std::unordered_set<std::string> main_keywords =
{
    "break", "do", "else", "elseif", "end", "for", "function", "goto", "if", "in", "repeat", "then", "until", "while"
};

std::unordered_set<std::string> add_keywords =
{
    "return", "and", "or", "not", "nil", "true", "false", "local"
};

void lua_text(const std::vector<std::pair<std::string, ImU32>>& text_with_colors, const ImVec2& pos)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 offset = { 0, 0 };
    float max_width = 0;
    float total_height = 0;

    float y_spacing = 0.0f;

    for (size_t i = 0; i < text_with_colors.size(); ++i)
    {
        const std::string& text = text_with_colors[i].first;
        ImU32 color = text_with_colors[i].second;

        std::string full_text = text;
        std::string line;
        size_t start_pos = 0;

        while (start_pos < full_text.size())
        {
            size_t end_pos = full_text.find('\n', start_pos);
            if (end_pos == std::string::npos)
                end_pos = full_text.size();

            line = full_text.substr(start_pos, end_pos - start_pos);

            draw_list->AddText(pos + offset, color, line.c_str());

            offset.x += ImGui::CalcTextSize(line.c_str()).x;

            if (end_pos < full_text.size())
            {
                offset.x = 0;
                offset.y += ImGui::GetTextLineHeight() + y_spacing;
            }

            start_pos = end_pos + 1;
        }

        max_width = (std::max)(max_width, offset.x);
        total_height = offset.y + ImGui::GetTextLineHeight();
    }

    offset.x += max_width;
    offset.y = total_height;
}

void lua_syntax(const std::string& text, const ImVec2& pos)
{
    std::vector<std::pair<std::string, ImU32>> text_with_colors;
    size_t start_pos = 0, len = text.size();

    auto handle_string = [&](const std::string& content, size_t& pos)
        {
            char quote_char = content[pos];
            size_t end_pos = pos + 1;

            while (end_pos < content.size())
            {
                if (content[end_pos] == '\\' && end_pos + 1 < content.size() && (content[end_pos + 1] == quote_char || content[end_pos + 1] == '\\'))
                    end_pos += 2;
                else if (content[end_pos] == quote_char)
                    break;
                else
                    end_pos++;
            }

            std::string str = content.substr(pos, end_pos - pos + 1);
            text_with_colors.push_back({ str, draw->get_clr(ImColor(206, 145, 120)) });
            pos = end_pos + 1;
        };

    while (start_pos < len)
    {
        size_t end_pos = start_pos;

        if (text[start_pos] == ' ' || text[start_pos] == '\n')
        {
            text_with_colors.push_back({ std::string(1, text[start_pos]), draw->get_clr(ImColor(255, 255, 255)) });
            start_pos++;
            continue;
        }

        if (text[start_pos] == '-' && start_pos + 1 < len && text[start_pos + 1] == '-')
        {
            if (start_pos + 2 < len && text[start_pos + 2] == '[' && text[start_pos + 3] == '[')
            {
                size_t comment_end = text.find("--]]", start_pos + 4);
                if (comment_end != std::string::npos)
                {
                    std::string multi_line_comment = text.substr(start_pos, comment_end - start_pos + 4);
                    text_with_colors.push_back({ multi_line_comment, draw->get_clr(ImColor(106, 153, 85)) });
                    start_pos = comment_end + 4;
                    continue;
                }
            }
            else
            {
                size_t comment_end = text.find("\n", start_pos);
                comment_end = (comment_end == std::string::npos) ? len : comment_end;
                std::string comment = text.substr(start_pos, comment_end - start_pos);
                text_with_colors.push_back({ comment, draw->get_clr(ImColor(106, 153, 85)) });
                start_pos = comment_end;
                continue;
            }
        }

        if (text[start_pos] == '"' || text[start_pos] == '\'')
        {
            handle_string(text, start_pos);
            continue;
        }

        if (isdigit(text[start_pos]) || (text[start_pos] == '.' && isdigit(text[start_pos + 1])))
        {
            size_t number_end = start_pos;
            bool has_dot = (text[start_pos] == '.');
            while (number_end < len && (isdigit(text[number_end]) || (text[number_end] == '.' && !has_dot)))
            {
                if (text[number_end] == '.') has_dot = true;
                number_end++;
            }
            std::string number = text.substr(start_pos, number_end - start_pos);
            text_with_colors.push_back({ number, draw->get_clr(ImColor(181, 206, 168)) });
            start_pos = number_end;
            continue;
        }

        if (text[start_pos] == '(' || text[start_pos] == ')' || text[start_pos] == '{' || text[start_pos] == '}')
        {
            text_with_colors.push_back({ std::string(1, text[start_pos]), draw->get_clr(ImColor(255, 255, 255)) });
            start_pos++;
            continue;
        }

        while (end_pos < len && !isspace(text[end_pos]) && text[end_pos] != '(' && text[end_pos] != ')' && text[end_pos] != '{' && text[end_pos] != '}')
            end_pos++;

        std::string word = text.substr(start_pos, end_pos - start_pos);

        if (main_keywords.find(word) != main_keywords.end())
            text_with_colors.push_back({ word, draw->get_clr(ImColor(197, 134, 192)) });
        else if (add_keywords.find(word) != add_keywords.end())
            text_with_colors.push_back({ word, draw->get_clr(ImColor(86, 156, 214)) });
        else if (end_pos < len && text[end_pos] == '(')
            text_with_colors.push_back({ word, draw->get_clr(ImColor(220, 220, 170)) });
        else
        {
            if (isalpha(word[0]) || word[0] == '_')
                text_with_colors.push_back({ word, draw->get_clr(ImColor(156, 220, 254)) });
            else
                text_with_colors.push_back({ word, draw->get_clr(ImColor(255, 255, 255)) });
        }

        start_pos = end_pos;
    }

    lua_text(text_with_colors, pos);
}

namespace ImStb
{
#include "imstb_textedit.h"
}

static int input_text_calc_text_len_and_line_count(const char* text_begin, const char** out_text_end)
{
    int line_count = 0;
    const char* s = text_begin;
    while (true)
    {
        const char* s_eol = strchr(s, '\n');
        line_count++;
        if (s_eol == NULL)
        {
            s = s + strlen(s);
            break;
        }
        s = s_eol + 1;
    }
    *out_text_end = s;
    return line_count;
}

static ImVec2 input_text_calc_text_size(ImGuiContext* ctx, const char* text_begin, const char* text_end, const char** remaining = NULL, ImVec2* out_offset = NULL, bool stop_on_new_line = false)
{
    ImGuiContext& g = *ctx;
    ImFont* font = g.Font;
    const float line_height = g.FontSize;
    const float scale = line_height / font->FontSize;

    ImVec2 text_size = ImVec2(0, 0);
    float line_width = 0.0f;

    const char* s = text_begin;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c == '\n')
        {
            text_size.x = ImMax(text_size.x, line_width);
            text_size.y += line_height;
            line_width = 0.0f;
            if (stop_on_new_line)
                break;
            continue;
        }
        if (c == '\r')
            continue;

        const float char_width = ((int)c < font->IndexAdvanceX.Size ? font->IndexAdvanceX.Data[c] : font->FallbackAdvanceX) * scale;
        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (out_offset)
        *out_offset = ImVec2(line_width, text_size.y + line_height);

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

namespace ImStb
{
    static int     STB_TEXTEDIT_STRINGLEN(const ImGuiInputTextState* obj) { return obj->CurLenA; }
    static char    STB_TEXTEDIT_GETCHAR(const ImGuiInputTextState* obj, int idx) { IM_ASSERT(idx <= obj->CurLenA); return obj->TextA[idx]; }
    static float   STB_TEXTEDIT_GETWIDTH(ImGuiInputTextState* obj, int line_start_idx, int char_idx) { unsigned int c; ImTextCharFromUtf8(&c, obj->TextA.Data + line_start_idx + char_idx, obj->TextA.Data + obj->TextA.Size); if ((ImWchar)c == '\n') return IMSTB_TEXTEDIT_GETWIDTH_NEWLINE; ImGuiContext& g = *obj->Ctx; return g.Font->GetCharAdvance((ImWchar)c) * g.FontScale; }
    static char    STB_TEXTEDIT_NEWLINE = '\n';
    static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, ImGuiInputTextState* obj, int line_start_idx)
    {
        const char* text = obj->TextA.Data;
        const char* text_remaining = NULL;
        const ImVec2 size = input_text_calc_text_size(obj->Ctx, text + line_start_idx, text + obj->CurLenA, &text_remaining, NULL, true);
        r->x0 = 0.0f;
        r->x1 = size.x;
        r->baseline_y_delta = size.y;
        r->ymin = 0.0f;
        r->ymax = size.y;
        r->num_chars = (int)(text_remaining - (text + line_start_idx));
    }

#define IMSTB_TEXTEDIT_GETNEXTCHARINDEX  IMSTB_TEXTEDIT_GETNEXTCHARINDEX_IMPL
#define IMSTB_TEXTEDIT_GETPREVCHARINDEX  IMSTB_TEXTEDIT_GETPREVCHARINDEX_IMPL

    static int IMSTB_TEXTEDIT_GETNEXTCHARINDEX_IMPL(ImGuiInputTextState* obj, int idx)
    {
        if (idx >= obj->CurLenA)
            return obj->CurLenA + 1;
        unsigned int c;
        return idx + ImTextCharFromUtf8(&c, obj->TextA.Data + idx, obj->TextA.Data + obj->TextA.Size);
    }

    static int IMSTB_TEXTEDIT_GETPREVCHARINDEX_IMPL(ImGuiInputTextState* obj, int idx)
    {
        if (idx <= 0)
            return -1;
        const char* p = ImTextFindPreviousUtf8Codepoint(obj->TextA.Data, obj->TextA.Data + idx);
        return (int)(p - obj->TextA.Data);
    }

    static bool ImCharIsSeparatorW(unsigned int c)
    {
        static const unsigned int separator_list[] =
        {
            ',', 0x3001, '.', 0x3002, ';', 0xFF1B, '(', 0xFF08, ')', 0xFF09, '{', 0xFF5B, '}', 0xFF5D,
            '[', 0x300C, ']', 0x300D, '|', 0xFF5C, '!', 0xFF01, '\\', 0xFFE5, '/', 0x30FB, 0xFF0F,
            '\n', '\r',
        };
        for (unsigned int separator : separator_list)
            if (c == separator)
                return true;
        return false;
    }

    static int is_word_boundary_from_right(ImGuiInputTextState* obj, int idx)
    {

        if ((obj->Flags & ImGuiInputTextFlags_Password) || idx <= 0)
            return 0;

        const char* curr_p = obj->TextA.Data + idx;
        const char* prev_p = ImTextFindPreviousUtf8Codepoint(obj->TextA.Data, curr_p);
        unsigned int curr_c; ImTextCharFromUtf8(&curr_c, curr_p, obj->TextA.Data + obj->TextA.Size);
        unsigned int prev_c; ImTextCharFromUtf8(&prev_c, prev_p, obj->TextA.Data + obj->TextA.Size);

        bool prev_white = ImCharIsBlankW(prev_c);
        bool prev_separ = ImCharIsSeparatorW(prev_c);
        bool curr_white = ImCharIsBlankW(curr_c);
        bool curr_separ = ImCharIsSeparatorW(curr_c);
        return ((prev_white || prev_separ) && !(curr_separ || curr_white)) || (curr_separ && !prev_separ);
    }
    static int is_word_boundary_from_left(ImGuiInputTextState* obj, int idx)
    {
        if ((obj->Flags & ImGuiInputTextFlags_Password) || idx <= 0)
            return 0;

        const char* curr_p = obj->TextA.Data + idx;
        const char* prev_p = ImTextFindPreviousUtf8Codepoint(obj->TextA.Data, curr_p);
        unsigned int prev_c; ImTextCharFromUtf8(&prev_c, curr_p, obj->TextA.Data + obj->TextA.Size);
        unsigned int curr_c; ImTextCharFromUtf8(&curr_c, prev_p, obj->TextA.Data + obj->TextA.Size);

        bool prev_white = ImCharIsBlankW(prev_c);
        bool prev_separ = ImCharIsSeparatorW(prev_c);
        bool curr_white = ImCharIsBlankW(curr_c);
        bool curr_separ = ImCharIsSeparatorW(curr_c);
        return ((prev_white) && !(curr_separ || curr_white)) || (curr_separ && !prev_separ);
    }
    static int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(ImGuiInputTextState* obj, int idx)
    {
        idx = IMSTB_TEXTEDIT_GETPREVCHARINDEX(obj, idx);
        while (idx >= 0 && !is_word_boundary_from_right(obj, idx))
            idx = IMSTB_TEXTEDIT_GETPREVCHARINDEX(obj, idx);
        return idx < 0 ? 0 : idx;
    }
    static int  STB_TEXTEDIT_MOVEWORDRIGHT_MAC(ImGuiInputTextState* obj, int idx)
    {
        int len = obj->CurLenA;
        idx = IMSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
        while (idx < len && !is_word_boundary_from_left(obj, idx))
            idx = IMSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
        return idx > len ? len : idx;
    }
    static int  STB_TEXTEDIT_MOVEWORDRIGHT_WIN(ImGuiInputTextState* obj, int idx)
    {
        idx = IMSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
        int len = obj->CurLenA;
        while (idx < len && !is_word_boundary_from_right(obj, idx))
            idx = IMSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
        return idx > len ? len : idx;
    }
    static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(ImGuiInputTextState* obj, int idx) { ImGuiContext& g = *obj->Ctx; if (g.IO.ConfigMacOSXBehaviors) return STB_TEXTEDIT_MOVEWORDRIGHT_MAC(obj, idx); else return STB_TEXTEDIT_MOVEWORDRIGHT_WIN(obj, idx); }
#define STB_TEXTEDIT_MOVEWORDLEFT       STB_TEXTEDIT_MOVEWORDLEFT_IMPL
#define STB_TEXTEDIT_MOVEWORDRIGHT      STB_TEXTEDIT_MOVEWORDRIGHT_IMPL

    static void STB_TEXTEDIT_DELETECHARS(ImGuiInputTextState* obj, int pos, int n)
    {
        char* dst = obj->TextA.Data + pos;

        obj->Edited = true;
        obj->CurLenA -= n;

        const char* src = obj->TextA.Data + pos + n;
        while (char c = *src++)
            *dst++ = c;
        *dst = '\0';
    }

    static bool STB_TEXTEDIT_INSERTCHARS(ImGuiInputTextState* obj, int pos, const char* new_text, int new_text_len)
    {
        const bool is_resizable = (obj->Flags & ImGuiInputTextFlags_CallbackResize) != 0;
        const int text_len = obj->CurLenA;
        IM_ASSERT(pos <= text_len);

        if (!is_resizable && (new_text_len + obj->CurLenA + 1 > obj->BufCapacityA))
            return false;

        if (new_text_len + text_len + 1 > obj->TextA.Size)
        {
            if (!is_resizable)
                return false;
            obj->TextA.resize(text_len + ImClamp(new_text_len, 32, ImMax(256, new_text_len)) + 1);
        }

        char* text = obj->TextA.Data;
        if (pos != text_len)
            memmove(text + pos + new_text_len, text + pos, (size_t)(text_len - pos));
        memcpy(text + pos, new_text, (size_t)new_text_len);

        obj->Edited = true;
        obj->CurLenA += new_text_len;
        obj->TextA[obj->CurLenA] = '\0';

        return true;
    }

#define STB_TEXTEDIT_K_LEFT         0x200000
#define STB_TEXTEDIT_K_RIGHT        0x200001
#define STB_TEXTEDIT_K_UP           0x200002
#define STB_TEXTEDIT_K_DOWN         0x200003
#define STB_TEXTEDIT_K_LINESTART    0x200004
#define STB_TEXTEDIT_K_LINEEND      0x200005
#define STB_TEXTEDIT_K_TEXTSTART    0x200006
#define STB_TEXTEDIT_K_TEXTEND      0x200007
#define STB_TEXTEDIT_K_DELETE       0x200008
#define STB_TEXTEDIT_K_BACKSPACE    0x200009
#define STB_TEXTEDIT_K_UNDO         0x20000A
#define STB_TEXTEDIT_K_REDO         0x20000B
#define STB_TEXTEDIT_K_WORDLEFT     0x20000C
#define STB_TEXTEDIT_K_WORDRIGHT    0x20000D
#define STB_TEXTEDIT_K_PGUP         0x20000E
#define STB_TEXTEDIT_K_PGDOWN       0x20000F
#define STB_TEXTEDIT_K_SHIFT        0x400000

#define IMSTB_TEXTEDIT_IMPLEMENTATION
#define IMSTB_TEXTEDIT_memmove memmove
#include "imstb_textedit.h"

    static void stb_textedit_replace(ImGuiInputTextState* str, STB_TexteditState* state, const IMSTB_TEXTEDIT_CHARTYPE* text, int text_len)
    {
        stb_text_makeundo_replace(str, state, 0, str->CurLenA, text_len);
        ImStb::STB_TEXTEDIT_DELETECHARS(str, 0, str->CurLenA);
        state->cursor = state->select_start = state->select_end = 0;
        if (text_len <= 0)
            return;
        if (ImStb::STB_TEXTEDIT_INSERTCHARS(str, 0, text, text_len))
        {
            state->cursor = state->select_start = state->select_end = text_len;
            state->has_preferred_x = 0;
            return;
        }
        IM_ASSERT(0);
    }

}

static bool input_text_filter_character(ImGuiContext* ctx, unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data, bool input_source_is_clipboard = false)
{
    unsigned int c = *p_char;

    bool apply_named_filters = true;
    if (c < 0x20)
    {
        bool pass = false;
        pass |= (c == '\n') && (flags & ImGuiInputTextFlags_Multiline) != 0;
        pass |= (c == '\t') && (flags & ImGuiInputTextFlags_AllowTabInput) != 0;
        if (!pass)
            return false;
        apply_named_filters = false;
    }

    if (input_source_is_clipboard == false)
    {

        if (c == 127)
            return false;

        if (c >= 0xE000 && c <= 0xF8FF)
            return false;
    }

    if (c > IM_UNICODE_CODEPOINT_MAX)
        return false;

    if (apply_named_filters && (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsScientific | (ImGuiInputTextFlags)ImGuiInputTextFlags_LocalizeDecimalPoint)))
    {

        ImGuiContext& g = *ctx;
        const unsigned c_decimal_point = (unsigned int)g.PlatformIO.Platform_LocaleDecimalPoint;
        if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific | (ImGuiInputTextFlags)ImGuiInputTextFlags_LocalizeDecimalPoint))
            if (c == '.' || c == ',')
                c = c_decimal_point;

        if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific | ImGuiInputTextFlags_CharsHexadecimal))
            if (c >= 0xFF01 && c <= 0xFF5E)
                c = c - 0xFF01 + 0x21;

        if (flags & ImGuiInputTextFlags_CharsDecimal)
            if (!(c >= '0' && c <= '9') && (c != c_decimal_point) && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsScientific)
            if (!(c >= '0' && c <= '9') && (c != c_decimal_point) && (c != '-') && (c != '+') && (c != '*') && (c != '/') && (c != 'e') && (c != 'E'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsHexadecimal)
            if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsUppercase)
            if (c >= 'a' && c <= 'z')
                c += (unsigned int)('A' - 'a');

        if (flags & ImGuiInputTextFlags_CharsNoBlank)
            if (ImCharIsBlankW(c))
                return false;

        *p_char = c;
    }

    if (flags & ImGuiInputTextFlags_CallbackCharFilter)
    {
        ImGuiContext& g = *GImGui;
        ImGuiInputTextCallbackData callback_data;
        callback_data.Ctx = &g;
        callback_data.EventFlag = ImGuiInputTextFlags_CallbackCharFilter;
        callback_data.EventChar = (ImWchar)c;
        callback_data.Flags = flags;
        callback_data.UserData = user_data;
        if (callback(&callback_data) != 0)
            return false;
        *p_char = callback_data.EventChar;
        if (!callback_data.EventChar)
            return false;
    }

    return true;
}

static void input_text_reconcile_undo_state_after_user_callback(ImGuiInputTextState* state, const char* new_buf_a, int new_length_a)
{
    const char* old_buf = state->CallbackTextBackup.Data;
    const int old_length = state->CallbackTextBackup.Size - 1;

    const int shorter_length = ImMin(old_length, new_length_a);
    int first_diff;
    for (first_diff = 0; first_diff < shorter_length; first_diff++)
        if (old_buf[first_diff] != new_buf_a[first_diff])
            break;
    if (first_diff == old_length && first_diff == new_length_a)
        return;

    int old_last_diff = old_length - 1;
    int new_last_diff = new_length_a - 1;
    for (; old_last_diff >= first_diff && new_last_diff >= first_diff; old_last_diff--, new_last_diff--)
        if (old_buf[old_last_diff] != new_buf_a[new_last_diff])
            break;

    const int insert_len = new_last_diff - first_diff + 1;
    const int delete_len = old_last_diff - first_diff + 1;
    if (insert_len > 0 || delete_len > 0)
        if (IMSTB_TEXTEDIT_CHARTYPE* p = stb_text_createundo(&state->Stb->undostate, first_diff, delete_len, insert_len))
            for (int i = 0; i < delete_len; i++)
                p[i] = old_buf[first_diff + i];
}

bool lua_field_ex(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data)
{
    struct lua_field_state
    {
        int line_count{ 0 };
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    IM_ASSERT(buf != NULL && buf_size >= 0);
    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));
    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackCompletion) && (flags & ImGuiInputTextFlags_AllowTabInput)));

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    const bool RENDER_SELECTION_WHEN_INACTIVE = false;
    const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;

    if (is_multiline)
        BeginGroup();
    const ImGuiID id = window->GetID(label);

    lua_field_state* animstate = gui->anim_container(&animstate, id);

    const float side_width{ 10 };
    const ImRect numbering(window->DC.CursorPos, window->DC.CursorPos + ImVec2(gui->text_size(GetDefaultFont(), "A").x * std::to_string(animstate->line_count).length() + SCALE(side_width), size_arg.y));
    const ImRect rect(ImVec2(numbering.Max.x, numbering.Min.y), window->DC.CursorPos + size_arg);

    ImGuiWindow* draw_window = window;
    ImVec2 inner_size = rect.GetSize();
    ImGuiLastItemData item_data_backup;
    if (is_multiline)
    {
        ImVec2 backup_pos = window->DC.CursorPos;
        ItemSize(rect, style.FramePadding.y);
        if (!ItemAdd(rect, id, &rect, ImGuiItemFlags_Inputable))
        {
            gui->end_group();
            return false;
        }
        item_data_backup = g.LastItemData;
        window->DC.CursorPos = backup_pos;

        if (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_FromTabbing) && (flags & ImGuiInputTextFlags_AllowTabInput))
            g.NavActivateId = 0;

        const ImGuiID backup_activate_id = g.NavActivateId;
        if (g.ActiveId == id)
            g.NavActivateId = 0;

        PushStyleColor(ImGuiCol_ChildBg, draw->get_clr({ 0.f, 0.f, 0.f, 0.f }));
        PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
        PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        SetCursorPos(GetCursorPos() + ImVec2(numbering.GetWidth(), 0));
        bool child_visible = gui->begin_def_child((std::stringstream{} << label << id).str().c_str(), rect.GetSize(), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
        g.NavActivateId = backup_activate_id;
        PopStyleVar(3);
        PopStyleColor();
        if (!child_visible)
        {
            gui->end_def_child();
            gui->end_group();
            return false;
        }
        draw_window = g.CurrentWindow;
        draw_window->DC.NavLayersActiveMaskNext |= (1 << draw_window->DC.NavLayerCurrent);
        draw_window->DC.CursorPos += style.FramePadding;
        inner_size.x -= draw_window->ScrollbarSizes.x;
    }
    else
    {

        ItemSize(rect, style.FramePadding.y);
        if (!(flags & ImGuiInputTextFlags_MergedItem))
            if (!ItemAdd(rect, id, &rect, ImGuiItemFlags_Inputable))
                return false;
    }
    const bool hovered = ItemHoverable(rect, id, g.LastItemData.InFlags);
    if (hovered)
        SetMouseCursor(ImGuiMouseCursor_TextInput);

    ImGuiInputTextState* state = GetInputTextState(id);

    if (g.LastItemData.InFlags & ImGuiItemFlags_ReadOnly)
        flags |= ImGuiInputTextFlags_ReadOnly;
    const bool is_readonly = (flags & ImGuiInputTextFlags_ReadOnly) != 0;
    const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
    const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
    const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
    if (is_resizable)
        IM_ASSERT(callback != NULL);

    const bool input_requested_by_nav = (g.ActiveId != id) && ((g.NavActivateId == id) && ((g.NavActivateFlags & ImGuiActivateFlags_PreferInput) || (g.NavInputSource == ImGuiInputSource_Keyboard)));

    const bool user_clicked = hovered && io.MouseClicked[0];
    const bool user_scroll_finish = is_multiline && state != NULL && g.ActiveId == 0 && g.ActiveIdPreviousFrame == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
    const bool user_scroll_active = is_multiline && state != NULL && g.ActiveId == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
    bool clear_active_id = false;
    bool select_all = false;

    float scroll_y = is_multiline ? draw_window->Scroll.y : FLT_MAX;

    const bool init_reload_from_user_buf = (state != NULL && state->ReloadUserBuf);
    const bool init_changed_specs = (state != NULL && state->Stb->single_line != !is_multiline);
    const bool init_make_active = (user_clicked || user_scroll_finish || input_requested_by_nav);
    const bool init_state = (init_make_active || user_scroll_active);
    if ((init_state && g.ActiveId != id) || init_changed_specs || init_reload_from_user_buf)
    {

        state = &g.InputTextState;
        state->CursorAnimReset();
        state->ReloadUserBuf = false;

        InputTextDeactivateHook(state->ID);

        const int buf_len = (int)strlen(buf);
        if (!init_reload_from_user_buf)
        {

            state->InitialTextA.resize(buf_len + 1);
            memcpy(state->InitialTextA.Data, buf, buf_len + 1);
        }

        bool recycle_state = (state->ID == id && !init_changed_specs && !init_reload_from_user_buf);
        if (recycle_state && (state->CurLenA != buf_len || (strncmp(state->TextA.Data, buf, buf_len) != 0)))
            recycle_state = false;

        state->ID = id;
        state->TextA.resize(buf_size + 1);
        state->CurLenA = (int)strlen(buf);
        memcpy(state->TextA.Data, buf, state->CurLenA + 1);

        if (recycle_state)
        {

            state->CursorClamp();
        }
        else
        {
            state->Scroll = ImVec2(0.0f, 0.0f);
            stb_textedit_initialize_state(state->Stb, !is_multiline);
        }

        if (init_reload_from_user_buf)
        {
            state->Stb->select_start = state->ReloadSelectionStart;
            state->Stb->cursor = state->Stb->select_end = state->ReloadSelectionEnd;
            state->CursorClamp();
        }
        else if (!is_multiline)
        {
            if (flags & ImGuiInputTextFlags_AutoSelectAll)
                select_all = true;
            if (input_requested_by_nav && (!recycle_state || !(g.NavActivateFlags & ImGuiActivateFlags_TryToPreserveState)))
                select_all = true;
            if (user_clicked && io.KeyCtrl)
                select_all = true;
        }

        if (flags & ImGuiInputTextFlags_AlwaysOverwrite)
            state->Stb->insert_mode = 1;
    }

    const bool is_osx = io.ConfigMacOSXBehaviors;
    if (g.ActiveId != id && init_make_active)
    {
        IM_ASSERT(state && state->ID == id);
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
    }
    if (g.ActiveId == id)
    {

        if (user_clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        if (is_multiline || (flags & ImGuiInputTextFlags_CallbackHistory))
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
        SetKeyOwner(ImGuiKey_Enter, id);
        SetKeyOwner(ImGuiKey_KeypadEnter, id);
        SetKeyOwner(ImGuiKey_Home, id);
        SetKeyOwner(ImGuiKey_End, id);
        if (is_multiline)
        {
            SetKeyOwner(ImGuiKey_PageUp, id);
            SetKeyOwner(ImGuiKey_PageDown, id);
        }

        if (is_osx)
            SetKeyOwner(ImGuiMod_Alt, id);

        if (is_multiline && state != NULL)
            state->Scroll.y = draw_window->Scroll.y;
    }

    if (g.ActiveId == id && state == NULL)
        ClearActiveID();

    if (g.ActiveId == id && io.MouseClicked[0] && !init_state && !init_make_active)
        clear_active_id = true;

    bool render_cursor = (g.ActiveId == id) || (state && user_scroll_active);
    bool render_selection = state && (state->HasSelection() || select_all) && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
    bool value_changed = false;
    bool validated = false;

    const bool buf_display_from_state = (render_cursor || render_selection || g.ActiveId == id) && !is_readonly && state;
    const bool is_displaying_hint = (hint != NULL && (buf_display_from_state ? state->TextA.Data : buf)[0] == 0);

    if (is_password && !is_displaying_hint)
    {
        const ImFontGlyph* glyph = g.Font->FindGlyph('*');
        ImFont* password_font = &g.InputTextPasswordFont;
        password_font->FontSize = g.Font->FontSize;
        password_font->Scale = g.Font->Scale;
        password_font->Ascent = g.Font->Ascent;
        password_font->Descent = g.Font->Descent;
        password_font->ContainerAtlas = g.Font->ContainerAtlas;
        password_font->FallbackGlyph = glyph;
        password_font->FallbackAdvanceX = glyph->AdvanceX;
        IM_ASSERT(password_font->Glyphs.empty() && password_font->IndexAdvanceX.empty() && password_font->IndexLookup.empty());
        PushFont(password_font);
    }

    if (g.ActiveId == id)
    {
        IM_ASSERT(state != NULL);
        state->Edited = false;
        state->BufCapacityA = buf_size;
        state->Flags = flags;

        g.ActiveIdAllowOverlap = !io.MouseDown[0];

        const float mouse_x = (io.MousePos.x - rect.Min.x - style.FramePadding.x) + state->Scroll.x;
        const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y) : (g.FontSize * 0.5f));

        if (select_all)
        {
            state->SelectAll();
            state->SelectedAllMouseLock = true;
        }
        else if (hovered && io.MouseClickedCount[0] >= 2 && !io.KeyShift)
        {
            stb_textedit_click(state, state->Stb, mouse_x, mouse_y);
            const int multiclick_count = (io.MouseClickedCount[0] - 2);
            if ((multiclick_count % 2) == 0)
            {

                const bool is_bol = (state->Stb->cursor == 0) || ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb->cursor - 1) == '\n';
                if (STB_TEXT_HAS_SELECTION(state->Stb) || !is_bol)
                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);

                if (!STB_TEXT_HAS_SELECTION(state->Stb))
                    ImStb::stb_textedit_prep_selection_at_cursor(state->Stb);
                state->Stb->cursor = ImStb::STB_TEXTEDIT_MOVEWORDRIGHT_MAC(state, state->Stb->cursor);
                state->Stb->select_end = state->Stb->cursor;
                ImStb::stb_textedit_clamp(state, state->Stb);
            }
            else
            {

                const bool is_eol = ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb->cursor) == '\n';
                state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART);
                state->OnKeyPressed(STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT);
                state->OnKeyPressed(STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT);
                if (!is_eol && is_multiline)
                {
                    ImSwap(state->Stb->select_start, state->Stb->select_end);
                    state->Stb->cursor = state->Stb->select_end;
                }
                state->CursorFollow = false;
            }
            state->CursorAnimReset();
        }
        else if (io.MouseClicked[0] && !state->SelectedAllMouseLock)
        {
            if (hovered)
            {
                if (io.KeyShift)
                    stb_textedit_drag(state, state->Stb, mouse_x, mouse_y);
                else
                    stb_textedit_click(state, state->Stb, mouse_x, mouse_y);
                state->CursorAnimReset();
            }
        }
        else if (io.MouseDown[0] && !state->SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
        {
            stb_textedit_drag(state, state->Stb, mouse_x, mouse_y);
            state->CursorAnimReset();
            state->CursorFollow = true;
        }
        if (state->SelectedAllMouseLock && !io.MouseDown[0])
            state->SelectedAllMouseLock = false;

        if ((flags & ImGuiInputTextFlags_AllowTabInput) && !is_readonly)
        {
            if (Shortcut(ImGuiKey_Tab, ImGuiInputFlags_Repeat, id))
            {
                unsigned int c = '\t';
                if (input_text_filter_character(&g, &c, flags, callback, callback_user_data))
                    state->OnCharPressed(c);
            }

        }

        const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeyCtrl);
        if (io.InputQueueCharacters.Size > 0)
        {
            if (!ignore_char_inputs && !is_readonly && !input_requested_by_nav)
                for (int n = 0; n < io.InputQueueCharacters.Size; n++)
                {

                    unsigned int c = (unsigned int)io.InputQueueCharacters[n];
                    if (c == '\t')
                        continue;
                    if (input_text_filter_character(&g, &c, flags, callback, callback_user_data))
                        state->OnCharPressed(c);
                }

            io.InputQueueCharacters.resize(0);
        }
    }

    bool revert_edit = false;
    if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
    {
        IM_ASSERT(state != NULL);

        const int row_count_per_page = ImMax((int)((inner_size.y - style.FramePadding.y) / g.FontSize), 1);
        state->Stb->row_count_per_page = row_count_per_page;

        const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
        const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;
        const bool is_startend_key_down = is_osx && io.KeyCtrl && !io.KeySuper && !io.KeyAlt;

        const ImGuiInputFlags f_repeat = ImGuiInputFlags_Repeat;
        const bool is_cut = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_X, f_repeat, id) || Shortcut(ImGuiMod_Shift | ImGuiKey_Delete, f_repeat, id)) && !is_readonly && !is_password && (!is_multiline || state->HasSelection());
        const bool is_copy = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, 0, id) || Shortcut(ImGuiMod_Ctrl | ImGuiKey_Insert, 0, id)) && !is_password && (!is_multiline || state->HasSelection());
        const bool is_paste = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, f_repeat, id) || Shortcut(ImGuiMod_Shift | ImGuiKey_Insert, f_repeat, id)) && !is_readonly;
        const bool is_undo = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, f_repeat, id)) && !is_readonly && is_undoable;
        const bool is_redo = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, f_repeat, id) || (is_osx && Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, f_repeat, id))) && !is_readonly && is_undoable;
        const bool is_select_all = Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, 0, id);

        const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
        const bool is_enter_pressed = IsKeyPressed(ImGuiKey_Enter, true) || IsKeyPressed(ImGuiKey_KeypadEnter, true);
        const bool is_gamepad_validate = nav_gamepad_active && (IsKeyPressed(ImGuiKey_NavGamepadActivate, false) || IsKeyPressed(ImGuiKey_NavGamepadInput, false));
        const bool is_cancel = Shortcut(ImGuiKey_Escape, f_repeat, id) || (nav_gamepad_active && Shortcut(ImGuiKey_NavGamepadCancel, f_repeat, id));

        if (IsKeyPressed(ImGuiKey_LeftArrow)) { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); }
        else if (IsKeyPressed(ImGuiKey_RightArrow)) { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); }
        else if (IsKeyPressed(ImGuiKey_UpArrow) && is_multiline) { if (io.KeyCtrl) SetScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); }
        else if (IsKeyPressed(ImGuiKey_DownArrow) && is_multiline) { if (io.KeyCtrl) SetScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY())); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); }
        else if (IsKeyPressed(ImGuiKey_PageUp) && is_multiline) { state->OnKeyPressed(STB_TEXTEDIT_K_PGUP | k_mask); scroll_y -= row_count_per_page * g.FontSize; }
        else if (IsKeyPressed(ImGuiKey_PageDown) && is_multiline) { state->OnKeyPressed(STB_TEXTEDIT_K_PGDOWN | k_mask); scroll_y += row_count_per_page * g.FontSize; }
        else if (IsKeyPressed(ImGuiKey_Home)) { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
        else if (IsKeyPressed(ImGuiKey_End)) { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
        else if (IsKeyPressed(ImGuiKey_Delete) && !is_readonly && !is_cut)
        {
            if (!state->HasSelection())
            {

                if (is_wordmove_key_down)
                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
            }
            state->OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask);
        }
        else if (IsKeyPressed(ImGuiKey_Backspace) && !is_readonly)
        {
            if (!state->HasSelection())
            {
                if (is_wordmove_key_down)
                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
                else if (is_osx && io.KeyCtrl && !io.KeyAlt && !io.KeySuper)
                    state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
            }
            state->OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
        }
        else if (is_enter_pressed || is_gamepad_validate)
        {

            bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
            if (!is_multiline || is_gamepad_validate || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
            {
                validated = true;
                if (io.ConfigInputTextEnterKeepActive && !is_multiline)
                    state->SelectAll();
                else
                    clear_active_id = true;
            }
            else if (!is_readonly)
            {
                unsigned int c = '\n';
                if (input_text_filter_character(&g, &c, flags, callback, callback_user_data))
                    state->OnCharPressed(c);
            }
        }
        else if (is_cancel)
        {
            if (flags & ImGuiInputTextFlags_EscapeClearsAll)
            {
                if (buf[0] != 0)
                {
                    revert_edit = true;
                }
                else
                {
                    render_cursor = render_selection = false;
                    clear_active_id = true;
                }
            }
            else
            {
                clear_active_id = revert_edit = true;
                render_cursor = render_selection = false;
            }
        }
        else if (is_undo || is_redo)
        {
            state->OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
            state->ClearSelection();
        }
        else if (is_select_all)
        {
            state->SelectAll();
            state->CursorFollow = true;
        }
        else if (is_cut || is_copy)
        {

            if (g.PlatformIO.Platform_SetClipboardTextFn != NULL)
            {
                const int ib = state->HasSelection() ? ImMin(state->Stb->select_start, state->Stb->select_end) : 0;
                const int ie = state->HasSelection() ? ImMax(state->Stb->select_start, state->Stb->select_end) : state->CurLenA;

                char backup = state->TextA.Data[ie];
                state->TextA.Data[ie] = 0;
                SetClipboardText(state->TextA.Data + ib);
                state->TextA.Data[ie] = backup;
            }
            if (is_cut)
            {
                if (!state->HasSelection())
                    state->SelectAll();
                state->CursorFollow = true;
                stb_textedit_cut(state, state->Stb);
            }
        }
        else if (is_paste)
        {
            if (const char* clipboard = GetClipboardText())
            {

                const int clipboard_len = (int)strlen(clipboard);
                char* clipboard_filtered = (char*)IM_ALLOC(clipboard_len + 1);
                int clipboard_filtered_len = 0;
                for (const char* s = clipboard; *s != 0; )
                {
                    unsigned int c;
                    int len = ImTextCharFromUtf8(&c, s, NULL);
                    s += len;
                    if (!input_text_filter_character(&g, &c, flags, callback, callback_user_data, true))
                        continue;
                    memcpy(clipboard_filtered + clipboard_filtered_len, s - len, len);
                    clipboard_filtered_len += len;
                }
                clipboard_filtered[clipboard_filtered_len] = 0;
                if (clipboard_filtered_len > 0)
                {
                    stb_textedit_paste(state, state->Stb, clipboard_filtered, clipboard_filtered_len);
                    state->CursorFollow = true;
                }
                MemFree(clipboard_filtered);
            }
        }

        render_selection |= state->HasSelection() && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
    }

    const char* apply_new_text = NULL;
    int apply_new_text_length = 0;
    if (g.ActiveId == id)
    {
        IM_ASSERT(state != NULL);
        if (revert_edit && !is_readonly)
        {
            if (flags & ImGuiInputTextFlags_EscapeClearsAll)
            {

                IM_ASSERT(buf[0] != 0);
                apply_new_text = "";
                apply_new_text_length = 0;
                value_changed = true;
                IMSTB_TEXTEDIT_CHARTYPE empty_string;
                stb_textedit_replace(state, state->Stb, &empty_string, 0);
            }
            else if (strcmp(buf, state->InitialTextA.Data) != 0)
            {
                apply_new_text = state->InitialTextA.Data;
                apply_new_text_length = state->InitialTextA.Size - 1;

                value_changed = true;
                stb_textedit_replace(state, state->Stb, state->InitialTextA.Data, state->InitialTextA.Size - 1);
            }
        }

        const bool apply_edit_back_to_user_buffer = !revert_edit || (validated && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
        if (apply_edit_back_to_user_buffer)
        {

            if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackAlways)) != 0)
            {
                IM_ASSERT(callback != NULL);

                ImGuiInputTextFlags event_flag = 0;
                ImGuiKey event_key = ImGuiKey_None;
                if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && Shortcut(ImGuiKey_Tab, 0, id))
                {
                    event_flag = ImGuiInputTextFlags_CallbackCompletion;
                    event_key = ImGuiKey_Tab;
                }
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(ImGuiKey_UpArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_UpArrow;
                }
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(ImGuiKey_DownArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_DownArrow;
                }
                else if ((flags & ImGuiInputTextFlags_CallbackEdit) && state->Edited)
                {
                    event_flag = ImGuiInputTextFlags_CallbackEdit;
                }
                else if (flags & ImGuiInputTextFlags_CallbackAlways)
                {
                    event_flag = ImGuiInputTextFlags_CallbackAlways;
                }

                if (event_flag)
                {
                    ImGuiInputTextCallbackData callback_data;
                    callback_data.Ctx = &g;
                    callback_data.EventFlag = event_flag;
                    callback_data.Flags = flags;
                    callback_data.UserData = callback_user_data;

                    state->CallbackTextBackup.resize(state->CurLenA + 1);
                    memcpy(state->CallbackTextBackup.Data, state->TextA.Data, state->CurLenA + 1);

                    char* callback_buf = is_readonly ? buf : state->TextA.Data;
                    callback_data.EventKey = event_key;
                    callback_data.Buf = callback_buf;
                    callback_data.BufTextLen = state->CurLenA;
                    callback_data.BufSize = state->BufCapacityA;
                    callback_data.BufDirty = false;

                    const int utf8_cursor_pos = callback_data.CursorPos = state->Stb->cursor;
                    const int utf8_selection_start = callback_data.SelectionStart = state->Stb->select_start;
                    const int utf8_selection_end = callback_data.SelectionEnd = state->Stb->select_end;

                    callback(&callback_data);

                    callback_buf = is_readonly ? buf : state->TextA.Data;
                    IM_ASSERT(callback_data.Buf == callback_buf);
                    IM_ASSERT(callback_data.BufSize == state->BufCapacityA);
                    IM_ASSERT(callback_data.Flags == flags);
                    const bool buf_dirty = callback_data.BufDirty;
                    if (callback_data.CursorPos != utf8_cursor_pos || buf_dirty) { state->Stb->cursor = callback_data.CursorPos; state->CursorFollow = true; }
                    if (callback_data.SelectionStart != utf8_selection_start || buf_dirty) { state->Stb->select_start = (callback_data.SelectionStart == callback_data.CursorPos) ? state->Stb->cursor : callback_data.SelectionStart; }
                    if (callback_data.SelectionEnd != utf8_selection_end || buf_dirty) { state->Stb->select_end = (callback_data.SelectionEnd == callback_data.SelectionStart) ? state->Stb->select_start : callback_data.SelectionEnd; }
                    if (buf_dirty)
                    {

                        IM_ASSERT(callback_data.BufTextLen == (int)strlen(callback_data.Buf));
                        input_text_reconcile_undo_state_after_user_callback(state, callback_data.Buf, callback_data.BufTextLen);
                        state->CurLenA = callback_data.BufTextLen;
                        state->TextA.Size = state->CurLenA + 1;
                        state->CursorAnimReset();
                    }
                }
            }

            if (!is_readonly && strcmp(state->TextA.Data, buf) != 0)
            {
                apply_new_text = state->TextA.Data;
                apply_new_text_length = state->CurLenA;
                value_changed = true;
            }
        }
    }

    if (g.InputTextDeactivatedState.ID == id)
    {
        if (g.ActiveId != id && IsItemDeactivatedAfterEdit() && !is_readonly && strcmp(g.InputTextDeactivatedState.TextA.Data, buf) != 0)
        {
            apply_new_text = g.InputTextDeactivatedState.TextA.Data;
            apply_new_text_length = g.InputTextDeactivatedState.TextA.Size - 1;
            value_changed = true;

        }
        g.InputTextDeactivatedState.ID = 0;
    }

    if (apply_new_text != NULL)
    {

        IM_ASSERT(apply_new_text_length >= 0);
        if (is_resizable)
        {
            ImGuiInputTextCallbackData callback_data;
            callback_data.Ctx = &g;
            callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
            callback_data.Flags = flags;
            callback_data.Buf = buf;
            callback_data.BufTextLen = apply_new_text_length;
            callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
            callback_data.UserData = callback_user_data;
            callback(&callback_data);
            buf = callback_data.Buf;
            buf_size = callback_data.BufSize;
            apply_new_text_length = ImMin(callback_data.BufTextLen, buf_size - 1);
            IM_ASSERT(apply_new_text_length <= buf_size);
        }

        ImStrncpy(buf, apply_new_text, ImMin(apply_new_text_length + 1, buf_size));
    }

    if (g.ActiveId == id && clear_active_id)
        ClearActiveID();
    else if (g.ActiveId == id)
        g.WantTextInputNextFrame = 1;

    if (!is_multiline)
    {
        RenderNavHighlight(rect, id);
        RenderFrame(rect.Min, rect.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    }

    const ImVec4 clip_rect(rect.Min.x, rect.Min.y, rect.Min.x + inner_size.x, rect.Min.y + inner_size.y);
    ImVec2 draw_pos = is_multiline ? draw_window->DC.CursorPos : rect.Min + style.FramePadding;
    ImVec2 text_size(0.0f, 0.0f);

    const int buf_display_max_length = 2 * 1024 * 1024;
    const char* buf_display = buf_display_from_state ? state->TextA.Data : buf;
    const char* buf_display_end = NULL;
    if (is_displaying_hint)
    {
        buf_display = hint;
        buf_display_end = hint + strlen(hint);
    }

    if (render_cursor || render_selection)
    {
        IM_ASSERT(state != NULL);
        if (!is_displaying_hint)
            buf_display_end = buf_display + state->CurLenA;

        const char* text_begin = state->TextA.Data;
        const char* text_end = text_begin + state->CurLenA;
        ImVec2 cursor_offset, select_start_offset;

        {

            int cursor_line_no = render_cursor ? -1 : -1000;
            int selmin_line_no = render_selection ? -1 : -1000;
            const char* cursor_ptr = render_cursor ? text_begin + state->Stb->cursor : NULL;
            const char* selmin_ptr = render_selection ? text_begin + ImMin(state->Stb->select_start, state->Stb->select_end) : NULL;

            int line_count = 1;
            if (is_multiline)
            {
                for (const char* s = text_begin; (s = (const char*)memchr(s, '\n', (size_t)(text_end - s))) != NULL; s++)
                {
                    if (cursor_line_no == -1 && s >= cursor_ptr) { cursor_line_no = line_count; }
                    if (selmin_line_no == -1 && s >= selmin_ptr) { selmin_line_no = line_count; }
                    line_count++;
                }
            }
            if (cursor_line_no == -1)
                cursor_line_no = line_count;
            if (selmin_line_no == -1)
                selmin_line_no = line_count;

            cursor_offset.x = input_text_calc_text_size(&g, ImStrbol(cursor_ptr, text_begin), cursor_ptr).x;
            cursor_offset.y = cursor_line_no * g.FontSize;
            if (selmin_line_no >= 0)
            {
                select_start_offset.x = input_text_calc_text_size(&g, ImStrbol(selmin_ptr, text_begin), selmin_ptr).x;
                select_start_offset.y = selmin_line_no * g.FontSize;
            }

            if (is_multiline)
                text_size = ImVec2(inner_size.x, line_count * g.FontSize);
        }

        if (render_cursor && state->CursorFollow)
        {

            if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
            {
                const float scroll_increment_x = inner_size.x * 0.25f;
                const float visible_width = inner_size.x - style.FramePadding.x;
                if (cursor_offset.x < state->Scroll.x)
                    state->Scroll.x = IM_TRUNC(ImMax(0.0f, cursor_offset.x - scroll_increment_x));
                if (cursor_offset.x - visible_width >= state->Scroll.x)
                    state->Scroll.x = IM_TRUNC(cursor_offset.x - visible_width + scroll_increment_x);
            }
            else
            {
                state->Scroll.y = 0.0f;
            }

            if (is_multiline)
            {

                if (cursor_offset.y - g.FontSize < scroll_y)
                    scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
                else if (cursor_offset.y - (inner_size.y - style.FramePadding.y * 2.0f) >= scroll_y)
                    scroll_y = cursor_offset.y - inner_size.y + style.FramePadding.y * 2.0f;
                const float scroll_max_y = ImMax((text_size.y + style.FramePadding.y * 2.0f) - inner_size.y, 0.0f);
                scroll_y = ImClamp(scroll_y, 0.0f, scroll_max_y);
                draw_pos.y += (draw_window->Scroll.y - scroll_y);
                draw_window->Scroll.y = scroll_y;
            }

            state->CursorFollow = false;
        }

        const ImVec2 draw_scroll = ImVec2(state->Scroll.x, 0.0f);
        if (render_selection)
        {
            const char* text_selected_begin = text_begin + ImMin(state->Stb->select_start, state->Stb->select_end);
            const char* text_selected_end = text_begin + ImMax(state->Stb->select_start, state->Stb->select_end);

            ImU32 bg_color = GetColorU32(ImGuiCol_TextSelectedBg, render_cursor ? 1.0f : 0.6f);
            float bg_offy_up = is_multiline ? 0.0f : -1.0f;
            float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
            ImVec2 rect_pos = draw_pos + select_start_offset - draw_scroll;
            for (const char* p = text_selected_begin; p < text_selected_end; )
            {
                if (rect_pos.y > clip_rect.w + g.FontSize)
                    break;
                if (rect_pos.y < clip_rect.y)
                {
                    p = (const char*)memchr((void*)p, '\n', text_selected_end - p);
                    p = p ? p + 1 : text_selected_end;
                }
                else
                {
                    ImVec2 rect_size = input_text_calc_text_size(&g, p, text_selected_end, &p, NULL, true);
                    if (rect_size.x <= 0.0f) rect_size.x = IM_TRUNC(g.Font->GetCharAdvance((ImWchar)' ') * 0.50f);
                    ImRect rect(rect_pos + ImVec2(0.0f, bg_offy_up - g.FontSize), rect_pos + ImVec2(rect_size.x, bg_offy_dn));
                    rect.ClipWith(clip_rect);
                    if (rect.Overlaps(clip_rect))
                        draw_window->DrawList->AddRectFilled(rect.Min + SCALE(0, 3), rect.Max + SCALE(0, 1), draw->get_clr(clr->base_colors.accent_clr, 0.2f), SCALE(elements->text_field.rounding - 1));
                    rect_pos.x = draw_pos.x - draw_scroll.x;
                }
                rect_pos.y += g.FontSize;
            }
        }

        if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
        {
            ImU32 col = GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
            lua_syntax(buf_display, draw_pos - draw_scroll);
        }

        ImVec2* animstate = gui->anim_container(&animstate, id);

        if (render_cursor)
        {
            state->CursorAnim += io.DeltaTime;
            bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || ImFmod(state->CursorAnim, 1.20f) <= 0.80f;
            ImVec2 cursor_screen_pos = ImTrunc(draw_pos + cursor_offset - draw_scroll);
            ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
            gui->easing(animstate->x, cursor_screen_rect.Min.x - rect.Min.x, 20, dynamic_easing);
            gui->easing(animstate->y, cursor_screen_rect.Min.y - rect.Min.y, 20, dynamic_easing);
            if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
                draw_window->DrawList->AddLine(rect.Min + *animstate + SCALE(0, 2), ImVec2(rect.Min.x + animstate->x, rect.Min.y + animstate->y + SCALE(10)), GetColorU32(ImGuiCol_Text));

            if (!is_readonly)
            {
                g.PlatformImeData.WantVisible = true;
                g.PlatformImeData.InputPos = ImVec2(cursor_screen_pos.x - 1.0f, cursor_screen_pos.y - g.FontSize);
                g.PlatformImeData.InputLineHeight = g.FontSize;
            }
        }
    }
    else
    {

        if (is_multiline)
            text_size = ImVec2(inner_size.x, input_text_calc_text_len_and_line_count(buf_display, &buf_display_end) * g.FontSize);
        else if (!is_displaying_hint && g.ActiveId == id)
            buf_display_end = buf_display + state->CurLenA;
        else if (!is_displaying_hint)
            buf_display_end = buf_display + strlen(buf_display);

        if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
        {
            ImU32 col = GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
            lua_syntax(buf_display, draw_pos);
        }
    }

    int line_count = 1;

    for (const char* p = buf_display; *p != '\0'; ++p) {
        if (*p == '\n') {
            line_count++;
        }
    }

    animstate->line_count = line_count;

    draw->push_clip_rect(window->DrawList, numbering.Min, numbering.Max, true);
    for (int i = 0; i < animstate->line_count; ++i)
        draw->text_clipped(window->DrawList, GetFont(), numbering.Min + ImVec2(0, GetTextLineHeight() * i - GetCurrentWindow()->Scroll.y + SCALE(3)), ImVec2(numbering.Max.x - SCALE(8), numbering.Min.y + GetTextLineHeight() * (i + 1) - GetCurrentWindow()->Scroll.y + SCALE(5)), draw->get_clr(ImColor(79, 79, 98)), std::to_string(i).c_str(), NULL, NULL, ImVec2(1.f, 0.f));
    draw->pop_clip_rect(window->DrawList);

    if (is_password && !is_displaying_hint)
        PopFont();

    if (is_multiline)
    {

        Dummy(ImVec2(text_size.x, text_size.y + style.FramePadding.y));
        g.NextItemData.ItemFlags |= (ImGuiItemFlags)ImGuiItemFlags_Inputable | ImGuiItemFlags_NoTabStop;
        gui->end_def_child();
        item_data_backup.StatusFlags |= (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredWindow);

        gui->end_group();
        if (g.LastItemData.ID == 0 || g.LastItemData.ID != GetWindowScrollbarID(draw_window, ImGuiAxis_Y))
        {
            g.LastItemData.ID = id;
            g.LastItemData.InFlags = item_data_backup.InFlags;
            g.LastItemData.StatusFlags = item_data_backup.StatusFlags;
        }
    }

    if (g.LogEnabled && (!is_password || is_displaying_hint))
    {
        LogSetNextTextDecoration("{", "}");
        LogRenderedText(&draw_pos, buf_display, buf_display_end);
    }

    if (value_changed && !(flags & ImGuiInputTextFlags_NoMarkEdited))
        MarkItemEdited(id);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Inputable);
    if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
        return validated;
    else
        return value_changed;
}

bool c_widgets::lua_field(std::string_view label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags)
{
    return lua_field_ex(label.data(), NULL, buf, (int)buf_size, size, flags, 0, 0);
}

bool c_widgets::folder(std::string_view label, bool active)
{
    struct c_folder
    {
        float alpha = 0.f;
        ImVec4 text = clr->text.text_inactive;
        ImVec4 icon = clr->text.text_inactive;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiStyle* style = &GetStyle();
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(gui->content_avail().x, SCALE(elements->dropdown.selection_height)));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id)) return false;

    bool hovered = IsItemHovered(), pressed = hovered && GetIO().MouseClicked[0];
    if (pressed) MarkItemEdited(id);

    c_folder* state = gui->anim_container(&state, id);

    gui->easing(state->icon, active ? clr->base_colors.accent_clr.Value : clr->text.text_inactive.Value, 8.f, dynamic_easing);
    gui->easing(state->alpha, active ? 1.f : 0.f, 16.f, dynamic_easing);
    gui->easing(state->text, active ? clr->text.text_active.Value : clr->text.text_inactive.Value, 8.f, dynamic_easing);

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr({ 1.f, 1.f, 1.f, 0.02f }, state->alpha), SCALE(elements->lua.rounding));

    draw->text_clipped(window->DrawList, var->font.instrument_medium[1], rect.Min + SCALE(40, 0), rect.Max, draw->get_clr(state->text), label.data(), NULL, NULL, { 0.0, 0.5 }, NULL);
    draw->text_clipped(window->DrawList, var->font.icons[4], rect.Min + SCALE(15, 0), rect.Max, draw->get_clr(state->icon), "K", NULL, NULL, {0.0, 0.5}, NULL);

    return pressed;
}
