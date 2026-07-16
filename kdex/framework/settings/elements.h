#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <string>
#include "../../game/Security/xorstr.hpp"

#define FA_GUN       "\xEE\x86\x9B"
#define FA_SEARCH    "\xEF\x80\x82"
#define FA_GEAR      "\xEF\x80\x93"
#define FA_CLOCK     "\xEF\x80\x97"
#define FA_TAG       "\xEF\x80\xAB"
#define FA_LIST      "\xEF\x80\xBA"
#define FA_EDIT      "\xEF\x81\x84"
#define FA_PLAY      "\xEF\x81\x8B"
#define FA_STOP_BTN  "\xEF\x81\x8D"
#define ICON_FOCUS   "\xEF\x81\x9B"
#define FA_EYE       "\xEF\x81\xAE"
#define FA_FOLDER    "\xEF\x81\xBB"
#define FA_GLOBE     "\xEF\x82\xAC"
#define ICON_WRENCH  "\xEF\x82\xAD"
#define FA_USERS     "\xEF\x83\x80"
#define ICON_GROUP   FA_USERS
#define FA_COPY      "\xEF\x83\x85"
#define FA_BOLT      "\xEF\x83\xA7"
#define FA_TERMINAL  "\xEF\x84\xA0"
#define FA_CODE      "\xEF\x84\xA1"
#define ICON_BUG     "\xEF\x86\x88"
#define FA_KEYBOARD  "\xEF\x84\x9C"
#define FA_WIFI      "\xEF\x87\xAB"
#define FA_HANDSHAKE "\xEF\x8A\xB5"

struct multi_combo {
  bool active;
  std::string name;
};

class c_elements {
public:
  struct {
    std::vector<std::vector<std::string>> section_list{
        {
            FA_GUN,
            FA_EYE,
            FA_GLOBE,
            FA_USERS,
            FA_HANDSHAKE,
            FA_COPY,
            FA_CODE,
            FA_FOLDER,
            FA_GEAR
        },
        {std::string(xorstr("Aim")), std::string(xorstr("Visuals")), std::string(xorstr("Exploits")), std::string(xorstr("Players")), std::string(xorstr("Friends")),
         std::string(xorstr("Events")), std::string(xorstr("Executor")), std::string(xorstr("Resources")), std::string(xorstr("Configs"))}};

    std::vector<std::string> sub_section_list{};

    float rounding = 10.f;
    float section_height = 35.f;
    float text_padding[2]{15, 40};

    int sub_section_count = 0;
    int sub_section_count_active = 0;

    int section_count = 0;
    int section_count_active = 0;

    int section_add = 0;
    float section_alpha = 0.f;
    float sub_section_alpha = 0.f;

  } section;

  struct {
    std::vector<std::string> category_list{std::string(xorstr("Features")), std::string(xorstr("Configs"))};

    float category_height = 38.f;
    float text_padding = 15.f;

  } category;

  struct {
    std::vector<std::string> sub_section_list{std::string(xorstr("Aimbot")), std::string(xorstr("Assist")), std::string(xorstr("Others"))};

    float sub_section_height = 40.f;

    int sub_section_count = 0;
    int sub_section_count_active = 0;

  } sub_section;

  struct {
    ImVec2 child_padding{10, 0};
    ImVec2 child_spacing{0, 0};

    float header_height = 32.f;
    float rounding = 10.f;

  } child;

  struct {
    ImVec2 popup_padding = {15, 15};
    float popup_rounding = 10.f;

    float rounding = 100.f;
    float height_size = 35.f;
    float text_padding[2]{0, 25};
    float circle_scale = 6.f;

  } checkbox;

  struct {
    ImVec2 drag_size{12, 12};

    float rounding = 100.f;
    float padding = 10.f;
    float line_size = 5.f;
    float height_size = 50.f;
    float text_changed = 25.f;

  } slider;

  struct {
    ImVec2 dropdown_padding{0, 0};
    ImVec2 dropdown_spacing{0, 0};

    float dropdown_border_size = 1.f;
    float selection_height = 35.f;
    float dropdown_height = 75.f;
    float dropdown_size = 30.f;

    float padding = 10.f;
    float rounding = 10.f;
    float mark_size = 8.f;
    float mark_stroke = 1.2f;
    float text_padding = 10.f;

    bool open_popup = false;

  } dropdown;

  struct {
    ImGuiWindowFlags notify_flags{
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_AlwaysAutoResize};

    ImVec2 notify_setup_padding{10, 10};
    ImVec2 notify_setup_spacing{10, 5};

    ImVec2 notify_padding{10, 9};
    ImVec2 motify_spacing{15, 15};

    float line_height = 3.f;
    float rounding = 10.f;
    float logo_size = 18.f;
    float logo_gap = 8.f;

  } notify;

  struct {
    ImVec2 picker_window_size{150, 130};

    ImVec2 hue_line_size{150, 8};
    ImVec2 alpha_line_size{150, 8};
    ImVec2 text_field_size{150, 30};

    ImVec2 popup_padding{10, 10};
    ImVec2 popup_spacing{10, 10};

    float height_size = 35.f;
    float line_height = 3.f;
    float rounding = 10.f;

    float circle_size = 1.f;

  } color_edit;

  struct {
    float field_height = 75.f;
    float field_size = 30.f;
    float padding = 10.f;

    float rounding = 10.f;
    float text_padding = 10.f;
    float line_padding = 8.f;

  } text_field;

  struct {
    float button_size = 30.f;
    float rounding = 10.f;
    float padding = 10.f;

  } button;

  struct {
    float keybind_size = 35.f;
    float rounding = 10.f;
    float padding = 10.f;

  } keybind;

  struct {
    ImVec2 list_padding{10, 10};
    ImVec2 list_spacing{10, 10};

    float mark_size = 8.f;
    float mark_stroke = 1.f;
    float rounding = 10.f;
    float padding = 10.f;

    float content_size = 30.f;
    float text_padding[2]{10, 35};

  } listbox;

  struct {
    ImVec2 mark_setup_padding{20, 20};

    ImVec2 mark_padding{15, 15};
    ImVec2 mark_spacing{15, 15};

    float rounding = 15.f;

  } warermark;

  struct {
    ImVec2 hud_setup_padding{20, 20};

    ImVec2 hud_padding{10, 0};
    ImVec2 hud_spacing{10, 0};

    float rounding = 8.f;

  } hud_navigation;

  struct {
    ImVec2 size{190, 110};
    float title_size{25};
    float rounding = 8.f;

    ImVec2 load_size{20, 20};
    ImVec2 button_size{15, 15};
  } lua;
};

inline std::unique_ptr<c_elements> elements = std::make_unique<c_elements>();
