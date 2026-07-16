	#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <string>
#include <vector>
#include "../../game/Security/xorstr.hpp"
#include "../../game/Security/Api/api.hpp"

struct keybind
{
	bool* callback = nullptr;
	int* key = nullptr;
	int* mode = nullptr;
};

class c_variables
{
public:
	struct
	{
		ImGuiWindowFlags window_flags					{ ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus };
		std::string window_name							{ std::string(xorstr("Rotten")) };

		ImVec2 window_padding							{ 0, 0 };
		ImVec2 item_spacing								{ 0, 0 };
		ImVec2 window_size								{ 1050, 545 };

		float window_border_size						= 0.f;
		float window_shadow_size						= 0.f;
		float window_rounding							= 15.f;

		float scrollbar_size							= 15.f;
		float scrollbar_width							= 3.f;
		float scrollbar_rounding						= 10.f;

		bool sidebar_minimized = false;
		float sidebar_width = 172.f;

		ImTextureID logo_texture = nullptr;
		ImTextureID banner_texture = nullptr;
		ImTextureID top_left_glow_texture = nullptr;
		ImTextureID bot_right_glow_texture = nullptr;

	} window;

	struct
	{
		bool dpi_changed								= false;
		bool hints_of_func								= true;
		bool show_status_func							= true;
		bool accent_color_menu							= true;

		float dpi										= 1.f;
		float accent_clr[4]								= { 0.88f, 0.88f, 0.88f, 1.f };

		int stored_dpi									= 100;

	} gui;

	struct
	{
		ImFont* brains_mono								[ 4 ]{ nullptr, nullptr, nullptr, nullptr };

		ImFont* instrument_medium						[ 2 ]{ nullptr, nullptr };
		ImFont* instrument_bold							[ 1 ]{ nullptr };
		ImFont* icons									[ 5 ]{ nullptr, nullptr, nullptr, nullptr, nullptr };
		ImFont* code = nullptr;
	} font;

	struct
	{
		char search_buf									[128]{ };
		float layout_alpha								= 0.f;
		bool search_active								= false;

	} search;

	struct
	{
		bool open_popup									= false;

		int index										= -1;
		std::vector										<keybind> update_keybind_system;

	} keybind;

	struct
	{
		std::vector<std::string> content				= { std::string(xorstr("Rotten")), std::string(xorstr("144FPS")), std::string(xorstr("25ms")) };
		bool watermark									= false;
		int position										= 1;
		ImVec2 last_size									{ 0.f, 0.f };

	} watermark;

	struct
	{
		bool enabled										= true;
		int position										= 2;

	} notifications;

	struct
	{
		std::vector<std::string> content				 = { std::string(xorstr("FUNC 1")), std::string(xorstr("FUNC 2")), std::string(xorstr("FUNC 3")) };
		int select_func									 = 0;

		bool navigator									 = true;

	} hud_navigation;

	struct
	{
		enum class screen { spinner_pre, login, registering, spinner_post, launch, menu };
		screen current_screen = screen::spinner_pre;
		double screen_t0 = 0.0;
		double screen_entered_at = 0.0;

		ImVec2 auth_window_size{ 420, 410 };
		ImVec2 launch_window_size{ 420, 410 };
		ImVec2 anim_window_size{ 420, 410 };

		char username[64] = "";
		char password[64] = "";

		char reg_username[64] = "";
		char reg_email[128] = "";
		char reg_password[64] = "";
		char reg_activation_key[64] = "";

		bool login_error = false;
		bool register_error = false;
		std::string error_msg;

		bool authenticated = false;
		std::string access_token;
		std::string refresh_token;
		int subscription_days_left = 0;
		bool subscription_active = false;
		std::string subscription_expires_at;

		int launch_state = 0;
		double launch_fivem_found_time = 0.0;
		bool launch_clicked = false;
		bool request_close_overlay = false;
		bool device_status_checked = false;
		bool device_status_checking = false;
		int device_blocked = 0;
		std::string device_blocked_message;
		int device_ban_user_id = 0;
		int pending_remote_config_id = 0;

		bool first_open_done = false;
		bool fade_done_once = false;
		bool fading = false;
		double fade_t0 = 0.0;
		bool logo_fading = false;
		double logo_fade_t0 = 0.0;

		bool auth_request_in_progress = false;
		enum class auth_operation_type { none, login, register_user };
		auth_operation_type auth_operation = auth_operation_type::none;
		bool auth_result_ready = false;
		Security::Api::AuthResult auth_result;

		std::string auth_notify_msg;
		int auth_notify_type = 0;
		double auth_notify_start_time = 0.0;
		float auth_notify_alpha = 0.0f;

		bool remember_login = false;
	} auth;

	struct
	{
		int sub_tab = 0;
		bool prevent_aim = false;
		bool prevent_trigger = false;
		bool hide_in_players = false;
		bool override_colors = false;

		bool enable_setfriend_keybind = false;
		int setfriend_key = 0;
		int setfriend_key_mode = 1;
		float setfriend_fov = 100.f;
		bool setfriend_draw_fov = false;
		float setfriend_fov_color[4]{ 0.478f, 0.200f, 1.000f, 0.800f };

		char friend_search[128] = "";
		char player_search[128] = "";

		int spectating_ped_id = 0;
		int selected_player_id = 0;
		void* selected_player_ped = nullptr;
	} friends_tab;
};

inline std::unique_ptr<c_variables> var = std::make_unique<c_variables>();
