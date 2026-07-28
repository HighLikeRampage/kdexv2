#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_internal.h"
#include "colors.h"
#include "variables.h"
#include "elements.h"
#include "search.h"

#include <Windows.h>
#include <functional>
#include <vector>
#include <sstream>
#include <string>

using namespace ImGui;

#define SCALE(...) scale_impl(__VA_ARGS__, var->gui.dpi)

inline ImVec2 scale_impl(const ImVec2& vec, float dpi) {
    return ImVec2(roundf(vec.x * dpi), roundf(vec.y * dpi));
}

inline ImVec2 scale_impl(float x, float y, float dpi) {
    return ImVec2(roundf(x * dpi), roundf(y * dpi));
}

inline float scale_impl(float var, float dpi) {
    return roundf(var * dpi);
}

enum watermark_pos {
    top_left = 0,
    top_right = 1,
    bottom_left = 2,
    bottom_right = 3,
};

enum positions
{
    pos_all,
    pos_x,
    pos_y
};

enum easing_type
{
    static_easing,
    dynamic_easing
};

class c_gui
{
public:

    float fixed_speed(float speed) { return speed * ImGui::GetIO().DeltaTime; }

    template <typename T>
    T* anim_container(T** state_ptr, ImGuiID id)
    {
        T* state = static_cast<T*>(GetStateStorage()->GetVoidPtr(id));
        if (!state)
            GetStateStorage()->SetVoidPtr(id, state = new T());

        *state_ptr = state;
        return state;
    }

    template<typename T>
    T easing(T& value, T val, float speed, int type, bool dynamic_round = false)
    {
        constexpr float epsilon = 0.1f;

        if (type == static_easing)
        {
            if constexpr (std::is_same<T, ImVec4>::value)
            {
                return { 1.f, 1.f, 1.f, 1.f };
            }
            else
            {
                T step = fixed_speed(speed * 2);

                if (value < val)
                {
                    value += step;
                    if (value > val) value = val;
                }
                else if (value > val)
                {
                    value -= step;
                    if (value < val) value = val;
                }
            }
        }
        else if (type == dynamic_easing)
        {
            if constexpr (std::is_same<T, ImVec4>::value)
            {
                value = ImLerp(value, val, fixed_speed(speed));
            }
            else
                value = ImLerp(value, val + (dynamic_round ? 0.5f : 0.f), fixed_speed(speed));
        }

        return value;
    }

    bool begin(std::string_view name = var->window.window_name, bool* p_open = nullptr, ImGuiWindowFlags flags = var->window.window_flags, float shadow_size = var->window.window_shadow_size, const ImU32 shadow_col = clr->window.window_shadow);

    void end();

    bool render_text(ImDrawList* draw_list, ImFont* font, ImU32 col, std::string_view text, const char* text_end = nullptr);

    void push_color(ImGuiCol idx, ImU32 col);

    void pop_color(int count = 1);

    void push_var(ImGuiStyleVar idx, float val);

    void push_var(ImGuiStyleVar idx, const ImVec2& val);

    void pop_var(int count = 1);

    void push_font(ImFont* font);

    void pop_font();

    void set_pos(const ImVec2& pos, int type);

    void set_pos(float pos, int type);

    ImVec2 get_pos();

    void set_screen_pos(const ImVec2& pos, int type);

    void set_screen_pos(float pos, int type);

    ImVec2 get_screen_pos();

    void begin_group();

    void end_group();

    void begin_content(std::string_view id, const ImVec2& size, const ImVec2& padding = ImVec2(0, 0), const ImVec2& spacing = ImVec2(0, 0), ImGuiWindowFlags flags = 0);

    void end_content();

    void sameline(float offset_from_start_x = 0.f, float spacing_w = -1.f);

    void dummy(const ImVec2& size);

    bool begin_def_child(std::string_view name, const ImVec2& size_arg = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0);

    void end_def_child();

    bool begin_child(std::string_view name, std::string_view icon, const ImVec2& size_arg = ImVec2(0, 0), ImGuiChildFlags child_flags = NULL, ImGuiWindowFlags window_flags = NULL);

    void end_child();

    void watermark(std::string name, std::vector<std::string> function, watermark_pos type, bool* visible);

    void hud_navigator(std::string_view name, int* func_selected, std::vector<std::string> function, bool* show_navigator);

    void set_next_window_pos(const ImVec2& pos, ImGuiCond cond = 0, const ImVec2& pivot = ImVec2(0, 0));

    void set_next_window_size(const ImVec2& size, ImGuiCond cond = 0);

    ImVec2 text_size(ImFont* font, const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.f);

    const char* text_end(const char* text);

    ImVec2 window_size();

    ImVec2 window_pos();

    float window_width();

    float window_height();

    ImVec2 content_avail();

    ImVec2 content_max();

    void set_window_focus();

    void set_style();

    void draw_decorations(bool draw_logo = true);

    struct GuiFrameContext {
        bool is_menu;
        bool is_launch;
        ImVec2 pos;
        ImVec2 size;
        ImDrawList* drawlist;
        float menu_alpha;
    };

    void render();

    void draw_spinner_screen(const GuiFrameContext& ctx, double duration, const std::function<void()>& on_done);
    bool render_auth_screens(const GuiFrameContext& ctx);
    bool render_launch_screen(const GuiFrameContext& ctx);
    void render_menu_screen(const GuiFrameContext& ctx);

};

inline std::unique_ptr<c_gui> gui = std::make_unique<c_gui>();

bool lua_field_ex(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data);

class c_widgets
{
public:
    bool section(std::string_view icon, std::string_view name, int section_id, int& section_variable);

    bool category(std::string_view name);

    bool checkbox(std::string_view label, bool* callback, bool warning = false, int* key = nullptr, int* mode = nullptr, float extra_padding = 0.f, float width = 0.f);

    bool checkbox_with_picker(std::string_view label, bool* callback, float col[4], bool alpha, bool warning = false);

    bool slider_int(std::string_view label, int* v, int v_min, int v_max, const char* format = "%d");

    bool slider_float(std::string_view label, float* v, float v_min, float v_max, const char* format = "%.1f");

    bool dropdown(std::string_view label, int* current_item, const std::vector<std::string>& items, int max_count = 6, const ImVec2& size = ImVec2(0, 0));

    void multi_dropdown(std::string_view label, bool variable[], const std::vector<std::string>& labels, int max_count = 6, const ImVec2& size = ImVec2(0, 0));

    bool range_int(std::string_view label, int* v1, int* v2, int v_min, int v_max, int range, const char* display_format = "%d%%");

    bool range_float(std::string_view label, float* v1, float* v2, float v_min, float v_max, float range, const char* display_format = "%.1f");

    bool color_picker(std::string_view label, float col[4], bool alpha = false);

    bool text_field(std::string_view icon, std::string_view label, std::string_view hint, char* buf, size_t buf_size, bool* active = nullptr, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = NULL);

    bool multiline_text(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback = 0, void* user_data = 0);

    bool button(std::string_view name, const ImVec2& size = ImVec2(0, 0), bool separator = true);

    bool settings_button(std::string_view id_name, bool callback, const ImVec2& size = ImVec2(30, 30));

    bool sidebar_toggle(std::string_view name, bool& minimized, float alpha_multiplier = 1.0f);

    bool key_select(std::string_view label, int* key);

    bool begin_list(std::string_view name, const ImVec2& size_arg = ImVec2(0, 0));

    void end_list();

    bool list_content(std::string_view label, bool active, ImDrawFlags flags = 0, bool draw_checkmark = true);
    bool list_content_col(std::string_view label, bool active, const ImVec4& text_color, ImDrawFlags flags = 0, bool draw_checkmark = true);

    bool lua(std::string_view name, std::string_view desc, bool* active);

    bool lua_field(std::string_view label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags);

    bool folder(std::string_view label, bool active);

};

inline std::unique_ptr<c_widgets> widgets = std::make_unique<c_widgets>();

enum fade_direction : int
{
    vertically,
    horizontally,
    diagonally,
    diagonally_reversed,
};

class c_draw
{
public:
    ImU32 get_clr(const ImVec4& col, float alpha = 1.f);

    void text(ImDrawList* draw_list, const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL, float wrap_width = NULL, const ImVec4* cpu_fine_clip_rect = 0);

    void text_clipped(ImDrawList* draw_list, ImFont* font, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 color, const char* text, const char* text_display_end = NULL, const ImVec2* text_size_if_known = NULL, const ImVec2& align = ImVec2(0.f, 0.f), const ImRect* clip_rect = NULL);

    void radial_gradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out);

    void set_linear_color_alpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1);

    void line(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f);

    void rect(ImDrawList* draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f);

    void rect_filled(ImDrawList* draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0);

    void rect_filled_multi_color(ImDrawList* draw, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding = 0.f, ImDrawFlags flags = 0);

    void fade_rect_filled(ImDrawList* draw, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 col_one, ImU32 col_two, fade_direction direction, float rounding = 0.f, ImDrawFlags flags = 0);

    void rounded_gradient_rect(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 col_one, ImU32 col_two, ImU32 border_col, float rounding = 12.f, ImDrawFlags flags = 0, float thickness = 1.0f);

    void shadow_rect(ImDrawList* draw_list, const ImVec2& obj_min, const ImVec2& obj_max, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0, float obj_rounding = 0.0f);

    void circle(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col, int num_segments = 0, float thickness = 1.0f);

    void circle_filled(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col, int num_segments = 0);

    void shadow_circle(ImDrawList* draw_list, const ImVec2& obj_center, float obj_radius, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0, int obj_num_segments = 12);

    void triangle(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness = 1.f);

    void triangle_filled(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col);

    void image(ImDrawList* draw_list, ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE);

    void image_rounded(ImDrawList* draw_list, ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, float rounding = 1.f, ImDrawFlags flags = 0);

    void shadow_convex_poly(ImDrawList* draw_list, const ImVec2* points, int points_count, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0);

    void shadow_ngon(ImDrawList* draw_list, const ImVec2& obj_center, float obj_radius, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags, int obj_num_segments);

    void rotate_start(ImDrawList* draw_list);

    void rotate_end(ImDrawList* draw_list, float rad, ImVec2 center = ImVec2(0, 0));

    void push_clip_rect(ImDrawList* draw_list, const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false);

    void pop_clip_rect(ImDrawList* draw_list);

    void render_checkmark(ImDrawList* draw_list, ImVec2 pos_min, ImVec2 pos_max, ImU32 col, float sz, float st);

    void render_color_rect_with_alpha_checkboard(ImDrawList* draw_list, ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding = NULL, ImDrawFlags flags = NULL);
};

inline std::unique_ptr<c_draw> draw = std::make_unique<c_draw>();

enum notify_type
{
    success = 0,
    warning = 1,
    error = 2
};

struct notify_state
{
    int notify_id;
    std::string text;
    float delay;
    notify_type notify_type = success;

    ImVec2 window_size = { 0, 0 };
    bool active_notify = true;

    float notify_timer = 0.f, notify_alpha = 0.f, notify_pos = 0.f;
};

class c_notify
{
public:
    void setup_notify();
    void add_notify(std::string_view text, float delay, notify_type type);

private:
    ImVec2 render_notify(int cur_notify_value, float notify_alpha, float notify_percentage, float notify_pos, std::string_view text, notify_type type);

    float notify_time = 15.f;
    int notify_count = 0;

    std::vector<notify_state> notifications;

};

inline std::unique_ptr<c_notify> notify = std::make_unique<c_notify>();

template <typename T>
inline void update_format_value(char* value_buf, size_t buf_size, const char* format, T* p_data, float fixed_speed, T& slow_value) {

    T* data_ptr = static_cast<T*>(p_data);
    T target_value = *data_ptr;

    if constexpr (std::is_arithmetic_v<T>) {

        T step = (target_value - slow_value) * gui->fixed_speed(fixed_speed);
        if (step == 0 && slow_value != target_value) step = (target_value > slow_value) ? 1 : -1;
        slow_value += step;
    }

    if constexpr (std::is_floating_point_v<T>) {
        if (std::fabs(slow_value) < std::numeric_limits<T>::epsilon()) {
            slow_value = 0.f;
        }
    }

    snprintf(value_buf, buf_size, format, slow_value);
}
