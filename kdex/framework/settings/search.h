#pragma once
#include <string>
#include <vector>
#include <variant>
#include "functions.h"
#include <unordered_set>
#include "../../game/Security/xorstr.hpp"
#include "../../game/Core/Features/HitboneList.hpp"

struct find_elements_checkbox
{
	std::string name;
	bool* callback;
	bool warning = false;
	int* key = nullptr;
	int* mode = nullptr;
};

struct find_elements_clr_checkbox
{
	std::string name;
	bool* callback;
	float* col;
	bool alpha = false;
	bool warning = false;
};

struct find_elements_slider_int
{
	std::string name;
	int* value;
	int min;
	int max;
	const char* format = xorstr("%d");
};

struct find_elements_slider_float
{
	std::string name;
	float* value;
	float min;
	float max;
	const char* format = xorstr("%.1f");
};

struct find_elements_dropdown
{
	std::string name;
	int* value;
	const std::vector<std::string>& items;
};

struct find_elements_multi_dropdown
{
	std::string name;
	bool* value;
	const std::vector<std::string>& items;
};

struct find_elements_range_int
{
	std::string name;
	int* v1;
	int* v2;
	int min;
	int max;
	int range;
	const char* format = xorstr("%d");
};

struct find_elements_range_float
{
	std::string name;
	float* v1;
	float* v2;
	float min;
	float max;
	float range;
	const char* format = xorstr("%.1f");
};

struct find_elements_clr_color
{
	std::string name;
	float* col;
	bool alpha = false;
	bool warning = false;
};

struct find_elements_color
{
	std::string name;
	float* col;
	bool alpha = false;
};

struct find_elements_textfield
{
	std::string_view icon;
	std::string_view name;
	std::string_view hint;
	char* value;
};

struct options_t
{
	bool enable_aimbot{ true };
	bool fiveguard_bypass{ false };
	bool fov_circle{ false };
	float fov_size{ 100.f };
	float fov_color[4]{ 0.5f, 0.3f, 1.f, 1.f };
	float smoothing{ 1.f };
	int hitbone{ 0 };
	int aim_distance{ 100 };
	bool ignore_npcs{ false };
	bool ignore_players{ false };
	bool check_visible{ false };
	bool target_combat_roll{ false };
    bool target_jump{ false };
	bool target_line{ false };

	bool silent_aim{ false };
	bool magic_bullets{ false };
	bool silent_fov_circle{ false };
	float silent_fov_size{ 100.f };
	bool silent_smart_fov{ false };
		bool silent_dual_fov{ false };
		float silent_fov_near{ 100.f };
		float silent_fov_far{ 200.f };
		int silent_dual_fov_distance{ 150 };
	float silent_fov_color[4]{ 0.5f, 0.3f, 1.f, 1.f };
	int silent_hitbone{ 0 };
	int silent_hit_chance{ 100 };
	float silent_jitter{ 0.005f };
	bool silent_ignore_npcs{ false };
	bool silent_ignore_players{ false };
	bool silent_check_visible{ false };
    bool silent_target_combat_roll{ false };
    bool silent_target_jump{ false };
	bool silent_target_line{ false };
	int silent_max_distance{ 300 };
	int silent_reaction_time{ 0 };
	bool silent_random_hitbone{ false };
	bool silent_aim_curving{ false };
	float silent_curve_strength{ 0.3f };
	int magic_max_distance{ 200 };

	bool enable_triggerbot{ true };
	int triggerbot_delay{ 0 };
	int triggerbot_hit_chance{ 100 };
	int triggerbot_delay_jitter{ 0 };
	int triggerbot_shot_duration{ 500 };
	int triggerbot_cooldown{ 0 };
	int triggerbot_max_distance{ 300 };
	bool triggerbot_curving{ false };
	float triggerbot_curve_strength{ 0.3f };
	bool triggerbot_ignore_npcs{ false };
	bool triggerbot_ignore_friends{ false };
	bool triggerbot_check_visible{ false };
    bool triggerbot_target_combat_roll{ false };
    bool triggerbot_target_jump{ false };
	bool triggerbot_fov_circle{ false };
	float triggerbot_fov_size{ 100.f };
	float triggerbot_fov_color[4]{ 1.f, 1.f, 1.f, 1.f };
    int triggerbot_hitbone{ 0 };
	bool triggerbot_target_line{ false };

	bool disable_aimbot_while_flashed{ false };
	bool disable_aimbot_through_smoke{ false };
	float assist_delay{ 0.150 };
	bool enable_recoil{ true };
	bool return_crosshair{ false };
	int enable_bullet{ 6 };
	int x_axis{ 50 };
	int y_axis{ 75 };
	int hitting_chance_one{ 1 };
	int hitting_chance_two{ 100 };
	bool disable_triggerbot_through_smoke{ false };
	bool disable_triggerbot_while_flashed{ true };
	bool enable_antiaimbot{ true };
	int add_lby_flip_yaw{ 50 };
	int real_yaw{ 0 };

	bool enable_esp{ true };
	bool dormant{ false };
	bool bounding_box{ true };
	int box_style{ 0 };
	float box_color[4]{ 1.f, 1.f, 1.f, 1.f };
	bool skeleton{ true };
    bool names{ true };
	bool distance{ true };
	bool dotbones{ false };
	bool dotbones_select[20]{ true, true, true, true, true, true, true, true, true, true,
		true, true, true, true, true, true, true, true, true, true };
	bool arrows{ true };
	float arrows_color[4]{ 0.478f, 0.200f, 1.000f, 1.000f };
	float arrows_size{ 12.0f };
	bool health_bar{ true };
	bool snaplines{ false };
	float esp_max_distance{ 500.f };
	bool team_check{ true };
	bool esp_ignore_npcs{ false };
	bool esp_ignore_dead{ false };
	bool show_local_player{ false };
    float team_check_color[4]{ 0.000f, 0.659f, 1.000f, 0.537f };

    bool admin_check{ true };
    float admin_check_color[4]{ 1.000f, 0.843f, 0.000f, 1.000f };

    bool playerlist_display_peds{ false };

	bool tracer{ false };
	float tracer_duration{ 2.0f };
	float tracer_color[4]{ 1.f, 1.f, 1.f, 1.f };
	float tracer_thickness{ 1.0f };

	bool player_chams{ false };
	bool player_chams_flicker{ false };
	float player_chams_color[4]{ 1.f, 0.f, 1.f, 1.f };
	bool tint_players{ false };
	float tint_players_color[4]{ 1.f, 0.f, 1.f, 1.f };
	bool override_sky_color{ false };
	float sky_color[4]{ 0.1f, 0.3f, 0.9f, 1.0f };
	bool enable_sky_zenith_color{ false };
	float sky_zenith_color[4]{ 0.6f, 0.2f, 0.9f, 1.0f };
	bool enable_sky_sun_color{ false };
	float sky_sun_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	bool enable_sky_sun_disc{ false };
	float sky_sun_disc_color[4]{ 1.0f, 0.2f, 0.2f, 1.0f };
	float sky_sun_disc_size{ 1.0f };
	bool enable_sky_moon{ false };
	float sky_moon_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	float sky_moon_disc_size{ 1.0f };
	float sky_moon_iten{ 1.0f };
	bool enable_sky_stars{ false };
	float sky_stars_iten{ 10.0f };
	bool enable_sky_clouds{ false };
	float sky_cloud_density_mult{ 1.0f };
	float sky_cloud_mid_col[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	float sky_cloud_base_col[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	float sky_cloud_overall_strength{ 1.0f };
	bool enable_light_rays{ false };
	float light_ray_mult{ 1.0f };
	float light_ray_col[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	bool enable_fog_haze{ false };
	float fog_haze_col[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	float fog_haze_density{ 1.0f };

	bool visible_check{ true };
    float visible_check_color[4]{ 0.000f, 0.659f, 1.000f, 0.537f };
    bool admin_check_esp{ true };
    float admin_check_esp_color[4]{ 1.000f, 0.843f, 0.000f, 1.000f };
	bool outline_text{ false };
	float opacity{ 1.f };
	float line_thickness{ 1.f };
	float font_size{ 14.f };
	float icon_size{ 1.f };

	bool enable_chams{ false };
	float chams_visible_color[4]{ 0.f, 1.f, 0.f, 1.f };
	float chams_invisible_color[4]{ 1.f, 0.f, 0.f, 1.f };
	bool wireframe{ false };
	bool flat_shading{ false };

	bool visualize_aimbot{ true };
	float visualize_aimbot_color[4]{ 1.f, 1.f, 1.f, 1.f };

	bool player_behind_wall{ true };
	float player_behind_wall_color[4]{ 1.f, 1.f, 1.f, 1.f };
	int behind_selection{ 0 };
	bool teammate{ true };
	bool teammate_behind_wall{ true };
	float teammate_behind_wall_color[4]{ 1.f, 1.f, 1.f, 1.f };

	bool physgun{ false };
	float physgun_fov{ 55.0f };
	int physgun_range{ 120 };

	bool no_clip{ false };
	bool spinbot{ false };
	bool strafe{ false };
	int strafe_speed{ 10 };
	bool teleport_behind_enemy{ false };
	bool god_mode{ false };
	bool anti_afk{ false };
	int anti_afk_speed{ 10 };
	bool fake_lag{ false };
	int fake_lag_speed{ 40 };
	bool always_lag{ false };
	bool anti_aim_block{ false };
	bool teleport{ false };
	bool super_jump{ false };
	bool fast_run{ false };
	float run_speed{ 1.0f };
	bool horn_boost_enabled{ false };
	float horn_boost_speed{ 10.0f };
	bool modify_vehicle_acceleration{ false };
	float vehicle_acceleration_value{ 10.0f };
	bool modify_vehicle_traction{ false };
	float vehicle_traction_value{ 2.0f };
	float teleport_behind_enemy_distance{ 2.0f };
	float teleport_behind_enemy_fov{ 55.0f };
	int teleport_behind_enemy_range{ 500 };
	float noclip_speed{ 3.0f };
	float max_health{ 100.0f };
	float max_armor{ 100.0f };
	bool beast_jump{ false };
	bool inf_combat_roll{ false };
	bool explosive_fist{ false };
	bool fire_ammo{ false };
	bool explosive_ammo{ false };
	bool no_rag_doll{ false };
	bool invisible{ false };
	bool invisible_spoof{ false };
	bool force_weapon_wheel{ false };
	bool steal_car{ false };
	bool disable_melee{ false };
	bool disable_evasive_dives{ false };
	bool disable_player_lockon{ false };
	bool disable_unarmed_drivebys{ false };
	float tp_all_vehicles_height{ 200.0f };
	bool no_ragdoll_bullet{ false };
	bool no_ragdoll_explosion{ false };
	bool no_ragdoll_fire{ false };
	bool no_ragdoll_vehicle{ false };
	bool treat_as_player_targeting{ false };
	bool block_weapon_switch{ false };
	bool no_collision{ false };
	bool shrink_enabled{ false };
	float shrink_scale{ 1.0f };
	bool big_ped_enabled{ false };
	float big_ped_scale{ 2.0f };
		bool infinite_stamina{ false };
		bool seat_belt{ false };
		bool anti_headshot{ false };
		bool anti_health_spoof{ false };
		bool electron{ false };
		bool custom_fov{ false };
		float fov_value{ 70.f };
		bool damage_boost{ false };
		float damage_boost_value{ 1.0f };
		float recoil_value{ 0.1f };
		float spread_value{ 0.5f };
	bool no_reload{ false };
	float weapon_range{ 250.0f };
	bool weapon_size_enabled{ false };
	float weapon_size_value{ 1.0f };

	bool infinite_ammo{ false };
	bool no_recoil_exploit{ false };
	bool no_spread{ false };
	bool tp_to_bullet{ false };
	bool freeze_ammo{ false };
	bool give_all_weapons{ false };
	bool rapid_fire{ false };
	bool weapon_spoof_enabled{ false };

	bool remove_flashbang_effect{ true };
	bool remove_smoke_grenades{ true };
	bool remove_skybox{ true };
	int the_power_of_flashbang_one{ 0 };
	int the_power_of_flashbang_two{ 100 };

	float transparent_walls_one{ 0 };
	float transparent_walls_two{ 10 };

	bool custom_crosshair{ false };
	bool watermark{ false };
	bool fps_counter{ true };
	bool coordinates{ false };
	bool radar{ false };
	bool enhanced_minimap{ false };

	bool stream_proof{ true };
	bool array_list{ false };
	bool array_list_col{ false };
	bool vsync{ true };
	bool second_monitor_display{ false };
	int display_monitor_index{ 1 };
	bool enable_rgb_particles{ false };
	int process_priority{ 0 };

	int menu_keybind{ 0x2D };
	int unload_keybind{ 0x21 };
	bool should_unload{ false };
	bool web_only{ false };

	int crosshair_style{ 0 };
	float crosshair_size{ 8.0f };
	float crosshair_thickness{ 2.0f };
	float crosshair_color[4]{ 1.f, 1.f, 1.f, 1.f };

	float smooth_horizontal{ 12.f };
	float smooth_vertical{ 12.f };
	int aimbot_speed{ 12 };
	bool aim_curving{ true };
	float curve_strength{ 0.3f };
	bool in_vehicle_aimbot{ true };
	bool use_prediction{ false };
	float prediction_time{ 0.0f };
	float randomize_angle{ 0.0f };

	bool smart_trigger{ false };
	int miss_chance{ 0 };

	bool friends_marker{ false };
	float snaplines_color[4]{ 1.f, 1.f, 1.f, 1.f };
	bool filled_box{ false };
	float filled_box_color[4]{ 0.f, 0.f, 0.f, 0.5f };
	float skeleton_color[4]{ 0.478f, 0.200f, 1.000f, 1.000f };
	float names_color[4]{ 1.f, 1.f, 1.f, 1.f };
    int names_pos{ 2 };
	bool weapon_name{ true };
	float weapon_name_color[4]{ 1.f, 1.f, 1.f, 1.f };
    int weapon_name_pos{ 2 };
	float distance_color[4]{ 1.f, 1.f, 1.f, 1.f };
	float dotbones_color[4]{ 1.f, 1.f, 1.f, 1.f };
    int distance_pos{ 2 };
	bool armor_bar{ true };
    int armor_bar_pos{ 2 };
    int health_bar_pos{ 2 };

	bool vehicle_esp{ false };
	float vehicle_max_distance{ 1000.f };
	bool vehicle_snaplines{ false };
	float vehicle_snaplines_color[4]{ 1.f, 1.f, 1.f, 1.f };
	bool vehicle_name{ false };
	bool vehicle_distance{ false };
	bool vehicle_lock{ false };

	bool object_esp{ false };
	bool object_name{ false };
	bool object_distance{ false };
	float object_max_distance{ 150.f };
	float object_name_color[4]{ 1.f, 1.f, 1.f, 1.f };

	bool vehicle_god_mode{ false };
	bool bring_vehicle{ false };
	bool warp_into_vehicle{ false };
	bool explode_vehicle{ false };
	bool change_plate{ false };
	char plate[16]{ "" };
	char vehicle_spawn_name[64]{ "" };
	bool vehicle_spawn_networked{ true };
	char ped_spawn_name[64]{ "" };
	bool ped_spawn_networked{ true };
	char prop_spawn_name[64]{ "" };
	bool prop_spawn_networked{ true };
	bool repair_vehicle{ false };
	bool apply_color_change{ false };
	float primary_vehicle_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	float secondary_vehicle_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	bool modify_gravity{ false };
	float gravity_value{ 9.8f };
	bool rocket_boost{ false };
	bool fake_fps_enabled{ false };
	bool fake_fps_range_mode{ false };
	int fake_fps_target{ 144 };
	int fake_fps_min{ 60 };
	int fake_fps_max{ 144 };
};

struct names_t
{
	std::string enable_aimbot{ std::string(xorstr("Enable aimbot")) };
	std::string silent_aim{ std::string(xorstr("Silent aim" )) };
	std::string silent_smart_fov{ std::string(xorstr("Smart FOV" )) };
		std::string silent_dual_fov{ std::string(xorstr("Dual FOV" )) };
		std::string silent_fov_near{ std::string(xorstr("Near FOV" )) };
		std::string silent_fov_far{ std::string(xorstr("Far FOV" )) };
		std::string silent_dual_fov_distance{ std::string(xorstr("Distance Threshold" )) };
	std::string disable_aimbot_while_flashed{ std::string(xorstr("Disable aimbot while flashed" )) };
	std::string disable_aimbot_through_smoke{ std::string(xorstr("Disable aimbot through smoke" )) };
	std::string assist_delay{ std::string(xorstr("Assist delay" )) };

	std::string enable_recoil{ std::string(xorstr("Enable recoil" )) };
	std::string return_crosshair{ std::string(xorstr("Return crosshair" )) };
	std::string enable_bullet{ std::string(xorstr("Enable bullet" )) };
	std::string x_axis{ std::string(xorstr("X-Axis" )) };
	std::string y_axis{ std::string(xorstr("Y-Axis" )) };

	std::string enable_triggerbot{ std::string(xorstr("Enable triggerbot" )) };
	std::string hitting_chance{ std::string(xorstr("Hitting chance" )) };
	std::string disable_triggerbot_through_smoke{ std::string(xorstr("Disable triggerbot through smoke" )) };
	std::string disable_triggerbot_while_flashed{ std::string(xorstr("Disable triggerbot while flashed" )) };
    std::string triggerbot_hitbone{ std::string(xorstr("Triggerbot Hitbox" )) };

	std::string enable_antiaimbot{ std::string(xorstr("Enable antiaimbot" )) };
	std::string add_lby_flip_yaw{ std::string(xorstr("Add LBY flip yaw" )) };
	std::string real_yaw{ std::string(xorstr("Real yaw" )) };

	std::string enable_esp{ std::string(xorstr("Enable ESP" )) };
	std::string dormant{ std::string(xorstr("Dormant" )) };
	std::string bounding_box{ std::string(xorstr("Boxes" )) };
	std::string box_style{ std::string(xorstr("Box Style" )) };
	std::string box_color{ std::string(xorstr("Box Color" )) };
	std::string skeleton{ std::string(xorstr("Skeleton" )) };
	std::string names{ std::string(xorstr("Tagnames" )) };
	std::string distance{ std::string(xorstr("Esp Distance" )) };
	std::string dotbones{ std::string(xorstr("Dot Bones" )) };
	std::string health_bar{ std::string(xorstr("Health bar" )) };
	std::string snaplines{ std::string(xorstr("Snaplines" )) };
	std::string esp_max_distance{ std::string(xorstr("Max Distance" )) };
	std::string team_check{ std::string(xorstr("Team Check" )) };
    std::string team_check_color{ std::string(xorstr("Team Color" )) };
	std::string esp_ignore_npcs{ std::string(xorstr("Ignore NPCs" )) };
	std::string esp_ignore_dead{ std::string(xorstr("Ignore Dead" )) };
	std::string show_local_player{ std::string(xorstr("Show Local Player" )) };
    std::string playerlist_display_peds{ std::string(xorstr("Display Peds" )) };

	std::string tracer{ std::string(xorstr("Bullet Tracers" )) };
	std::string tracer_duration{ std::string(xorstr("Tracer Duration" )) };
	std::string tracer_color{ std::string(xorstr("Tracer Color" )) };
	std::string tracer_thickness{ std::string(xorstr("Tracer Thickness" )) };

	std::string player_chams{ std::string(xorstr("Player Chams" )) };
	std::string player_chams_flicker{ std::string(xorstr("Flicker Effect" )) };
	std::string player_chams_color{ std::string(xorstr("Player Chams Color" )) };
	std::string tint_players{ std::string(xorstr("Tint Players" )) };
	std::string tint_players_color{ std::string(xorstr("Tint Color" )) };
	std::string sky_color{ std::string(xorstr("Sky Color" )) };
	std::string override_sky_color{ std::string(xorstr("Override Sky Color" )) };
	std::string enable_sky_zenith_color{ std::string(xorstr("Enable Zenith Color" )) };
	std::string sky_zenith_color{ std::string(xorstr("Zenith Color" )) };
	std::string enable_sky_sun_color{ std::string(xorstr("Enable Sun Color" )) };
	std::string sky_sun_color{ std::string(xorstr("Sun Color" )) };
	std::string enable_sky_sun_disc{ std::string(xorstr("Enable Sun Disc" )) };
	std::string sky_sun_disc_color{ std::string(xorstr("Sun Disc Color" )) };
	std::string sky_sun_disc_size{ std::string(xorstr("Sun Disc Size" )) };
	std::string enable_sky_moon{ std::string(xorstr("Enable Moon" )) };
	std::string sky_moon_color{ std::string(xorstr("Moon Color" )) };
	std::string sky_moon_disc_size{ std::string(xorstr("Moon Disc Size" )) };
	std::string sky_moon_iten{ std::string(xorstr("Moon Intensity" )) };
	std::string enable_sky_stars{ std::string(xorstr("Enable Stars" )) };
	std::string sky_stars_iten{ std::string(xorstr("Stars Intensity" )) };
	std::string enable_sky_clouds{ std::string(xorstr("Enable Clouds" )) };
	std::string sky_cloud_density_mult{ std::string(xorstr("Cloud Density" )) };
	std::string sky_cloud_mid_col{ std::string(xorstr("Cloud Mid Color" )) };
	std::string sky_cloud_base_col{ std::string(xorstr("Cloud Base Color" )) };
	std::string sky_cloud_overall_strength{ std::string(xorstr("Cloud Strength" )) };
	std::string enable_light_rays{ std::string(xorstr("Enable Godrays" )) };
	std::string light_ray_mult{ std::string(xorstr("Light Ray Multiplier" )) };
	std::string light_ray_col{ std::string(xorstr("Light Ray Color" )) };
	std::string enable_fog_haze{ std::string(xorstr("Enable Fog Haze" )) };
	std::string fog_haze_col{ std::string(xorstr("Fog Haze Color" )) };
	std::string fog_haze_density{ std::string(xorstr("Fog Haze Density" )) };

	std::string visible_check{ std::string(xorstr("Visible Check" )) };
    std::string visible_check_color{ std::string(xorstr("Visible Color" )) };
	std::string arrows{ std::string(xorstr("Arrows" )) };
	std::string arrows_color{ std::string(xorstr("Arrows Color" )) };
	std::string arrows_size{ std::string(xorstr("Arrows Size" )) };
	std::string outline_text{ std::string(xorstr("Outline Text" )) };
	std::string opacity{ std::string(xorstr("Opacity" )) };
	std::string line_thickness{ std::string(xorstr("Line Thickness" )) };
	std::string font_size{ std::string(xorstr("Font Size" )) };
	std::string enable_chams{ std::string(xorstr("Enable Chams" )) };
	std::string chams_visible_color{ std::string(xorstr("Visible Color" )) };
	std::string chams_invisible_color{ std::string(xorstr("Invisible Color" )) };
	std::string wireframe{ std::string(xorstr("Wireframe" )) };
	std::string flat_shading{ std::string(xorstr("Flat Shading" )) };
	std::string visualize_aimbot{ std::string(xorstr("Visualize aimbot" )) };

	std::string player_behind_wall{ std::string(xorstr("Player behind wall" )) };
	std::string behind_selection{ std::string(xorstr("Behind selection" )) };
	std::string teammate{ std::string(xorstr("Teammate" )) };
	std::string teammate_behind_wall{ std::string(xorstr("Teammate behind wall" )) };

	std::string physgun{ std::string(xorstr("PhysGun" )) };
	std::string physgun_fov{ std::string(xorstr("PhysGun FOV" )) };
	std::string physgun_range{ std::string(xorstr("PhysGun Range" )) };

	std::string no_clip{ std::string(xorstr("No Clip" )) };
	std::string spinbot{ std::string(xorstr("Spinbot" )) };
	std::string strafe{ std::string(xorstr("Strafe" )) };
	std::string strafe_speed{ std::string(xorstr("Strafe Speed" )) };
	std::string teleport_behind_enemy{ std::string(xorstr("TP Behind" )) };
	std::string god_mode{ std::string(xorstr("God Mode" )) };
	std::string anti_afk{ std::string(xorstr("Anti AFK" )) };
	std::string anti_afk_speed{ std::string(xorstr("Anti AFK Delay" )) };
	std::string teleport{ std::string(xorstr("Teleport" )) };
	std::string teleport_behind_enemy_distance{ std::string(xorstr("TB Distance" )) };
	std::string teleport_behind_enemy_fov{ std::string(xorstr("TB FOV" )) };
	std::string teleport_behind_enemy_range{ std::string(xorstr("TB Search Range" )) };
	std::string super_jump{ std::string(xorstr("Super Jump" )) };
	std::string fast_run{ std::string(xorstr("Fast Run" )) };
	std::string run_speed{ std::string(xorstr("Speed" )) };
	std::string horn_boost{ std::string(xorstr("Horn Boost" )) };
	std::string horn_boost_speed{ std::string(xorstr("Boost Speed" )) };
	std::string modify_vehicle_acceleration{ std::string(xorstr("Acceleration" )) };
	std::string vehicle_acceleration_value{ std::string(xorstr("Acceleration Value" )) };
	std::string modify_vehicle_traction{ std::string(xorstr("Traction" )) };
	std::string vehicle_traction_value{ std::string(xorstr("Traction Value" )) };
	std::string noclip_speed{ std::string(xorstr("Noclip Speed" )) };
	std::string max_health{ std::string(xorstr("Health" )) };
	std::string max_armor{ std::string(xorstr("Armor" )) };
	std::string beast_jump{ std::string(xorstr("Beast Jump" )) };
	std::string inf_combat_roll{ std::string(xorstr("Infinite Combat Roll" )) };
	std::string explosive_fist{ std::string(xorstr("Explosive Fist" )) };
	std::string fire_ammo{ std::string(xorstr("Fire Ammo" )) };
	std::string explosive_ammo{ std::string(xorstr("Explosive Ammo" )) };
	std::string no_rag_doll{ std::string(xorstr("No RagDoll" )) };
	std::string vehicle_god_mode{ std::string(xorstr("Vehicle God Mode" )) };
	std::string force_weapon_wheel{ std::string(xorstr("Force Weapon Wheel" )) };
	std::string steal_car{ std::string(xorstr("Steal Car" )) };
	std::string disable_melee{ std::string(xorstr("Disable Melee" )) };
	std::string disable_evasive_dives{ std::string(xorstr("Disable Evasive Dives" )) };
	std::string disable_player_lockon{ std::string(xorstr("Disable Player Lockon" )) };
	std::string disable_unarmed_drivebys{ std::string(xorstr("Disable Unarmed Drivebys" )) };
	std::string tp_all_vehicles_height{ std::string(xorstr("TP Height Offset" )) };
	std::string no_ragdoll_bullet{ std::string(xorstr("No Ragdoll (Bullet)" )) };
	std::string no_ragdoll_explosion{ std::string(xorstr("No Ragdoll (Explosion)" )) };
	std::string no_ragdoll_fire{ std::string(xorstr("No Ragdoll (Fire)" )) };
	std::string no_ragdoll_vehicle{ std::string(xorstr("No Ragdoll (Vehicle)" )) };
	std::string treat_as_player_targeting{ std::string(xorstr("Treat Player Targeting" )) };
	std::string block_weapon_switch{ std::string(xorstr("Block Weapon Switch" )) };
	std::string no_collision{ std::string(xorstr("No Collision" )) };
	std::string shrink_enabled{ std::string(xorstr("Shrink" )) };
	std::string shrink_scale{ std::string(xorstr("Shrink Scale" )) };
	std::string big_ped_enabled{ std::string(xorstr("Big Ped" )) };
	std::string big_ped_scale{ std::string(xorstr("Big Ped Scale" )) };
	std::string infinite_ammo{ std::string(xorstr("Infinite Ammo" )) };
	std::string no_recoil_exploit{ std::string(xorstr("No Recoil" )) };
	std::string no_spread{ std::string(xorstr("No Spread" )) };
	std::string give_all_weapons{ std::string(xorstr("Give All Weapons" )) };
	std::string rapid_fire{ std::string(xorstr("Rapid Fire" )) };
	std::string invisible{ std::string(xorstr("Invisible" )) };
	std::string invisible_spoof{ std::string(xorstr("Invisible Anti Esp" )) };

	std::string remove_flashbang_effect{ std::string(xorstr("Remove flashbang effect" )) };
	std::string remove_smoke_grenades{ std::string(xorstr("Remove smoke grenades" )) };
	std::string remove_skybox{ std::string(xorstr("Remove skybox" )) };
	std::string the_power_of_flashbang{ std::string(xorstr("The power of flashbang" )) };

	std::string transparent_walls{ std::string(xorstr("Transparent walls" )) };
	std::string fiveguard_bypass{ std::string(xorstr("Fiveguard Bypass" )) };
	std::string fov_circle{ std::string(xorstr("FOV Circle" )) };
	std::string fov_size{ std::string(xorstr("FOV Size" )) };
	std::string fov_color{ std::string(xorstr("FOV Color" )) };
	std::string smoothing{ std::string(xorstr("Smoothing" )) };
	std::string hitbone{ std::string(xorstr("Hitbone" )) };
	std::string silent_hitbone{ std::string(xorstr("Silent Hitbox" )) };
	std::string silent_jitter{ std::string(xorstr("Silent Jitter" )) };
	std::string aim_distance{ std::string(xorstr("Aim Distance" )) };
	std::string ignore_npcs{ std::string(xorstr("Ignore NPCs" )) };
	std::string ignore_players{ std::string(xorstr("Ignore Players" )) };
	std::string check_visible{ std::string(xorstr("Check Visible" )) };
	std::string target_combat_roll{ std::string(xorstr("Target CombatRoll" )) };
    std::string target_jump{ std::string(xorstr("Target Jump" )) };
	std::string target_line{ std::string(xorstr("Target Line" )) };
	std::string silent_target_line{ std::string(xorstr("Target Line" )) };
	std::string triggerbot_target_line{ std::string(xorstr("Target Line" )) };
	std::string magic_bullets{ std::string(xorstr("Magic Bullets" )) };
	std::string hit_chance{ std::string(xorstr("Hit Chance" )) };
	std::string delay_ms{ std::string(xorstr("Delay (ms)" )) };
	std::string silent_max_distance{ std::string(xorstr("Max Distance" )) };
	std::string silent_reaction_time{ std::string(xorstr("Reaction Time" )) };
	std::string silent_random_hitbone{ std::string(xorstr("Randomize Hitbox" )) };
	std::string silent_aim_curving{ std::string(xorstr("Aim Curving" )) };
	std::string silent_curve_strength{ std::string(xorstr("Curve Strength" )) };
	std::string triggerbot_curving{ std::string(xorstr("Aim Curving" )) };
	std::string triggerbot_curve_strength{ std::string(xorstr("Curve Strength" )) };
	std::string magic_hit_chance{ std::string(xorstr("Magic Hit Chance" )) };
	std::string magic_max_distance{ std::string(xorstr("Magic Max Distance" )) };
	std::string triggerbot_delay_jitter{ std::string(xorstr("Delay Jitter" )) };
	std::string triggerbot_shot_duration{ std::string(xorstr("Shot Duration" )) };
	std::string triggerbot_cooldown{ std::string(xorstr("Cooldown" )) };
	std::string triggerbot_max_distance{ std::string(xorstr("Max Distance" )) };
	std::string triggerbot_ignore_npcs{ std::string(xorstr("Ignore NPCs" )) };
	std::string triggerbot_ignore_friends{ std::string(xorstr("Ignore Friends" )) };
	std::string triggerbot_check_visible{ std::string(xorstr("Check Visible" )) };
	std::string triggerbot_fov_circle{ std::string(xorstr("FOV Circle" )) };
	std::string triggerbot_fov_size{ std::string(xorstr("FOV Size" )) };
	std::string triggerbot_fov_color{ std::string(xorstr("FOV Color" )) };

	std::string custom_crosshair{ std::string(xorstr("Custom Crosshair" )) };
	std::string watermark{ std::string(xorstr("Watermark" )) };
	std::string fps_counter{ std::string(xorstr("FPS Counter" )) };
	std::string coordinates{ std::string(xorstr("Coordinates" )) };
	std::string radar{ std::string(xorstr("Radar" )) };
	std::string enhanced_minimap{ std::string(xorstr("Enhanced Minimap" )) };

	std::string stream_proof{ std::string(xorstr("Stream Proof" )) };
	std::string array_list{ std::string(xorstr("Array List" )) };
	std::string array_list_col{ std::string(xorstr("Array List Color" )) };
	std::string vsync{ std::string(xorstr("VSync" )) };
	std::string second_monitor_display{ std::string(xorstr("Second Monitor" )) };
	std::string display_monitor_index{ std::string(xorstr("Monitor Index" )) };
	std::string enable_rgb_particles{ std::string(xorstr("Enable RGB Particles" )) };
	std::string process_priority{ std::string(xorstr("Process Priority" )) };
	std::string menu_keybind{ std::string(xorstr("Menu Keybind" )) };
	std::string unload_keybind{ std::string(xorstr("Unload Keybind" )) };
	std::string web_only{ std::string(xorstr("Web-Only" )) };

	std::string crosshair_style{ std::string(xorstr("Crosshair Style" )) };
	std::string crosshair_size{ std::string(xorstr("Crosshair Size" )) };
	std::string crosshair_thickness{ std::string(xorstr("Crosshair Thickness" )) };
	std::string crosshair_color{ std::string(xorstr("Crosshair Color" )) };
	std::string ui_font_size{ std::string(xorstr("Text Size" )) };
	std::string ui_icon_size{ std::string(xorstr("Icon Size" )) };

	std::string smooth_horizontal{ std::string(xorstr("Smooth Horizontal" )) };
	std::string smooth_vertical{ std::string(xorstr("Smooth Vertical" )) };
	std::string aimbot_speed{ std::string(xorstr("Aimbot Speed" )) };
	std::string aim_curving{ std::string(xorstr("Aim Curving" )) };
	std::string curve_strength{ std::string(xorstr("Curve Strength" )) };
	std::string in_vehicle_aimbot{ std::string(xorstr("In Vehicle Aimbot" )) };
	std::string use_prediction{ std::string(xorstr("Use Prediction" )) };
	std::string prediction_time{ std::string(xorstr("Prediction Time" )) };
	std::string randomize_angle{ std::string(xorstr("Randomize Angle" )) };

	std::string smart_trigger{ std::string(xorstr("Smart Trigger" )) };
	std::string miss_chance{ std::string(xorstr("Miss Chance" )) };

	std::string friends_marker{ std::string(xorstr("Friends Marker" )) };
	std::string snaplines_color{ std::string(xorstr("Snaplines Color" )) };
	std::string filled_box{ std::string(xorstr("Filled Box" )) };
	std::string filled_box_color{ std::string(xorstr("Filled Box Color" )) };
	std::string skeleton_color{ std::string(xorstr("Skeleton Color" )) };
	std::string names_color{ std::string(xorstr("Names Color" )) };
	std::string names_pos{ std::string(xorstr("Names Position" )) };
	std::string weapon_name{ std::string(xorstr("Weapon Name" )) };
	std::string weapon_name_color{ std::string(xorstr("Weapon Name Color" )) };
	std::string weapon_name_pos{ std::string(xorstr("Weapon Name Position" )) };
	std::string distance_color{ std::string(xorstr("Distance Color" )) };
	std::string dotbones_color{ std::string(xorstr("Dot Bones Color" )) };
	std::string distance_pos{ std::string(xorstr("Distance Position" )) };
	std::string armor_bar{ std::string(xorstr("Armor Bar" )) };
	std::string armor_bar_pos{ std::string(xorstr("Armor Bar Position" )) };
	std::string health_bar_pos{ std::string(xorstr("Health Bar Position" )) };

	std::string vehicle_esp{ std::string(xorstr("Vehicle ESP" )) };
	std::string vehicle_max_distance{ std::string(xorstr("Vehicle Distance" )) };
	std::string vehicle_snaplines{ std::string(xorstr("Vehicle Snaplines" )) };
	std::string vehicle_name{ std::string(xorstr("Vehicle Name" )) };
	std::string vehicle_distance{ std::string(xorstr("Vehicle Distance" )) };
	std::string vehicle_lock{ std::string(xorstr("Show Lock/Unlock" )) };

	std::string object_esp{ std::string(xorstr("Objects ESP" )) };
	std::string object_name{ std::string(xorstr("Object Name" )) };
	std::string object_distance{ std::string(xorstr("Object Distance" )) };
	std::string object_max_distance{ std::string(xorstr("Object Distance" )) };
	std::string object_name_color{ std::string(xorstr("Object Name Color" )) };

	std::string bring_vehicle{ std::string(xorstr("Bring Vehicle")) };
	std::string warp_into_vehicle{ std::string(xorstr("Warp Into Vehicle")) };
	std::string infinite_stamina{ std::string(xorstr("Infinite Stamina" )) };
	std::string seat_belt{ std::string(xorstr("Seat Belt" )) };
		std::string anti_headshot{ std::string(xorstr("Anti Headshot" )) };
		std::string anti_health_spoof{ std::string(xorstr("Anti Health Spoof" )) };
		std::string electron{ std::string(xorstr("Electron Bypass" )) };
		std::string custom_fov{ std::string(xorstr("Force 4th Person" )) };
		std::string fov_value{ std::string(xorstr("FOV Value" )) };
		std::string damage_boost{ std::string(xorstr("Damage Boost" )) };
		std::string damage_boost_value{ std::string(xorstr("Damage Multiplier" )) };
		std::string recoil_value{ std::string(xorstr("Recoil Value" )) };
		std::string spread_value{ std::string(xorstr("Spread Value" )) };
	std::string no_reload{ std::string(xorstr("No Reload" )) };
	std::string weapon_size{ std::string(xorstr("Weapon Size" )) };

	std::string explode_vehicle{ std::string(xorstr("Explode Vehicle" )) };
	std::string repair_vehicle{ std::string(xorstr("Repair Vehicle" )) };
	std::string rocket_boost{ std::string(xorstr("Rocket Boost" )) };
	std::string change_plate{ std::string(xorstr("Change Plate" )) };
	std::string apply_color_change{ std::string(xorstr("Apply Color Change" )) };
	std::string primary_vehicle_color{ std::string(xorstr("Primary Color" )) };
	std::string secondary_vehicle_color{ std::string(xorstr("Secondary Color" )) };
	std::string modify_gravity{ std::string(xorstr("Modify Gravity" )) };
	std::string gravity_value{ std::string(xorstr("Gravity Value" )) };
	std::string weapon_range{ std::string(xorstr("Weapon Range" )) };
	std::string fake_lag{ std::string(xorstr("Fake Lag" )) };
	std::string fake_lag_speed{ std::string(xorstr("Fake Lag Speed" )) };
	std::string always_lag{ std::string(xorstr("Always Lag" )) };
	std::string anti_aim_block{ std::string(xorstr("Anti Aim" )) };
	std::string fake_fps_enabled{ std::string(xorstr("Fake FPS")) };
	std::string fake_fps_range_mode{ std::string(xorstr("Range Mode")) };
	std::string fake_fps_target{ std::string(xorstr("Target FPS")) };
	std::string fake_fps_min{ std::string(xorstr("Min FPS")) };
	std::string fake_fps_max{ std::string(xorstr("Max FPS")) };
};

struct keys_t
{
	int enable_aimbot_key{ 0 }; int enable_aimbot_mode{ 1 };
	int enable_esp_key{ 0 }; int enable_esp_mode{ 1 };
	int silent_aim_key{ 0 }; int silent_aim_mode{ 1 };
	int triggerbot_key{ 0 }; int triggerbot_mode{ 1 };
	int friends_marker_key{ 0 }; int friends_marker_mode{ 1 };
	int physgun_key{ 0 }; int physgun_mode{ 1 };
	int noclip_key{ 0 }; int noclip_mode{ 1 };
	int strafe_key{ 0 }; int strafe_mode{ 1 };
	int teleport_behind_enemy_key{ 0 }; int teleport_behind_enemy_mode{ 1 };
	int vehicle_lock_key{ 0 }; int vehicle_lock_mode{ 1 };
	int bring_vehicle_key{ 0 }; int bring_vehicle_mode{ 1 };
	int warp_into_vehicle_key{ 0 }; int warp_into_vehicle_mode{ 1 };
	int explode_vehicle_key{ 0 }; int explode_vehicle_mode{ 1 };
	int horn_boost_key{ 0 }; int horn_boost_mode{ 1 };
	int fake_lag_key{ 0 }; int fake_lag_mode{ 1 };
	int anti_aim_block_key{ 0 }; int anti_aim_block_mode{ 1 };
	int max_health_key{ 0 }; int max_health_mode{ 1 };
	int max_armor_key{ 0 }; int max_armor_mode{ 1 };
};

struct items_t
{
	std::vector<std::string> real_yaw{ std::string(xorstr("Disabled")), std::string(xorstr("Real")), std::string(xorstr("Fake")) };
	std::vector<std::string> behind_selection{ std::string(xorstr("Default")), std::string(xorstr("Automatic")), std::string(xorstr("Manual")) };
	std::vector<std::string> hitbones{ Core::Features::BuildHitboneNameList() };
    std::vector<std::string> box_styles{ std::string(xorstr("Corner")), std::string(xorstr("Corner Filled")) };
	std::vector<std::string> positions{ std::string(xorstr("Top")), std::string(xorstr("Right")), std::string(xorstr("Bottom")), std::string(xorstr("Left")) };
	std::vector<std::string> dotbones_bones{
		std::string(xorstr("Head")), std::string(xorstr("Neck")), std::string(xorstr("Spine")), std::string(xorstr("Pelvis")),
		std::string(xorstr("L UpperArm")), std::string(xorstr("L Forearm")), std::string(xorstr("L Hand")),
		std::string(xorstr("R UpperArm")), std::string(xorstr("R Forearm")), std::string(xorstr("R Hand")),
		std::string(xorstr("L Thigh")), std::string(xorstr("L Calf")), std::string(xorstr("L Foot")), std::string(xorstr("L Toe")),
		std::string(xorstr("R Thigh")), std::string(xorstr("R Calf")), std::string(xorstr("R Foot")), std::string(xorstr("R Toe")),
		std::string(xorstr("L Clavicle")), std::string(xorstr("R Clavicle"))
	};
	std::vector<std::string> crosshair_styles{ std::string(xorstr("Dot")), std::string(xorstr("Cross")), std::string(xorstr("X")) };
	std::vector<std::string> process_priorities{ std::string(xorstr("Idle")), std::string(xorstr("Below Normal")), std::string(xorstr("Normal")), std::string(xorstr("Above Normal")), std::string(xorstr("High")), std::string(xorstr("Realtime")) };
};

struct options
{
	options_t param;
	names_t name;
	items_t item;
	keys_t key;
};

inline options* option = new options();

class c_search
{
public:
	bool text_field_hovered{ false };
	float window_alpha{ 0.f };
	float slide_offset{ 0.f };
	float content_scale{ 0.f };
	bool active_searcing{ false };

	std::string search_element{ };
	char search_buf[256]{ };

	using all_elements = std::variant<find_elements_checkbox, find_elements_clr_checkbox, find_elements_slider_int, find_elements_slider_float, find_elements_dropdown, find_elements_multi_dropdown, find_elements_range_int, find_elements_range_float, find_elements_color, find_elements_clr_color, find_elements_textfield>;

	std::vector<all_elements> search_elements;
	std::unordered_set<std::string> allowed_names;

	void init_options();
	void search();
	void register_label(std::string_view label) { allowed_names.emplace(label); }

private:
	bool initialized{ false };
};

inline std::unique_ptr<c_search> search = std::make_unique<c_search>();
