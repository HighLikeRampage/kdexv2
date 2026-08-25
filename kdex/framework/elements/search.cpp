#pragma once
#include "../settings/search.h"
#include "../../game/Security/xorstr.hpp"
#include <algorithm>

void c_search::init_options()
{
	if (!initialized)
	{
		search_elements.push_back(find_elements_checkbox{ option->name.enable_aimbot, &option->param.enable_aimbot, true, &option->key.enable_aimbot_key, &option->key.enable_aimbot_mode });
		search_elements.push_back(find_elements_checkbox{ option->name.fiveguard_bypass, &option->param.fiveguard_bypass });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.fov_circle, &option->param.fov_circle, option->param.fov_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.fov_size, &option->param.fov_size, 10.f, 1000.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.smoothing, &option->param.smoothing, 1.f, 20.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_dropdown{ option->name.hitbone, &option->param.hitbone, option->item.hitbones });
		search_elements.push_back(find_elements_slider_int{ option->name.aim_distance, &option->param.aim_distance, 10, 1000, xorstr("%dm") });
		search_elements.push_back(find_elements_checkbox{ option->name.ignore_npcs, &option->param.ignore_npcs });
		search_elements.push_back(find_elements_checkbox{ option->name.ignore_players, &option->param.ignore_players });
		search_elements.push_back(find_elements_checkbox{ option->name.check_visible, &option->param.check_visible });
		search_elements.push_back(find_elements_checkbox{ option->name.target_combat_roll, &option->param.target_combat_roll });
        search_elements.push_back(find_elements_checkbox{ option->name.target_jump, &option->param.target_jump });
		search_elements.push_back(find_elements_checkbox{ option->name.target_line, &option->param.target_line });

		search_elements.push_back(find_elements_slider_float{ option->name.smooth_horizontal, &option->param.smooth_horizontal, 0.f, 20.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.smooth_vertical, &option->param.smooth_vertical, 0.f, 20.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_int{ option->name.aimbot_speed, &option->param.aimbot_speed, 1, 100, xorstr("%d") });
		search_elements.push_back(find_elements_checkbox{ option->name.aim_curving, &option->param.aim_curving });
		search_elements.push_back(find_elements_slider_float{ option->name.curve_strength, &option->param.curve_strength, 0.f, 1.f, xorstr("%.2f") });
		search_elements.push_back(find_elements_checkbox{ option->name.in_vehicle_aimbot, &option->param.in_vehicle_aimbot });
		search_elements.push_back(find_elements_checkbox{ option->name.use_prediction, &option->param.use_prediction });
		search_elements.push_back(find_elements_slider_float{ option->name.prediction_time, &option->param.prediction_time, 0.f, 1.f, xorstr("%.2fs") });
		search_elements.push_back(find_elements_slider_float{ option->name.randomize_angle, &option->param.randomize_angle, 0.f, 10.f, xorstr("%.1f") });

		search_elements.push_back(find_elements_checkbox{ option->name.silent_aim, &option->param.silent_aim });
		search_elements.push_back(find_elements_checkbox{ option->name.magic_bullets, &option->param.magic_bullets });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.fov_circle + std::string(xorstr(" Silent")), &option->param.silent_fov_circle, option->param.silent_fov_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.fov_size + std::string(xorstr(" Silent")), &option->param.silent_fov_size, 10.f, 1000.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.silent_smart_fov, &option->param.silent_smart_fov });
		search_elements.push_back(find_elements_checkbox{ option->name.silent_dual_fov, &option->param.silent_dual_fov });
		search_elements.push_back(find_elements_slider_float{ option->name.silent_fov_near, &option->param.silent_fov_near, 10.f, 500.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.silent_fov_far, &option->param.silent_fov_far, 50.f, 1000.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_int{ option->name.silent_dual_fov_distance, &option->param.silent_dual_fov_distance, 10, 500, xorstr("%dm") });
		search_elements.push_back(find_elements_dropdown{ option->name.silent_hitbone, &option->param.silent_hitbone, option->item.hitbones });
		search_elements.push_back(find_elements_slider_float{ option->name.silent_jitter, &option->param.silent_jitter, 0.0f, 0.05f, xorstr("%.4f") });
		search_elements.push_back(find_elements_slider_int{ option->name.hit_chance + std::string(xorstr(" Silent")), &option->param.silent_hit_chance, 0, 100, xorstr("%d%%") });
		search_elements.push_back(find_elements_checkbox{ option->name.ignore_npcs + std::string(xorstr(" Silent")), &option->param.silent_ignore_npcs });
		search_elements.push_back(find_elements_checkbox{ option->name.ignore_players + std::string(xorstr(" Silent")), &option->param.silent_ignore_players });
		search_elements.push_back(find_elements_checkbox{ option->name.check_visible + std::string(xorstr(" Silent")), &option->param.silent_check_visible });
		search_elements.push_back(find_elements_checkbox{ option->name.target_combat_roll + std::string(xorstr(" Silent")), &option->param.silent_target_combat_roll });
		search_elements.push_back(find_elements_checkbox{ option->name.silent_target_line + std::string(xorstr(" Silent")), &option->param.silent_target_line });
		search_elements.push_back(find_elements_checkbox{ option->name.silent_aim_curving + std::string(xorstr(" Silent")), &option->param.silent_aim_curving });
		search_elements.push_back(find_elements_slider_float{ option->name.silent_curve_strength + std::string(xorstr(" Silent")), &option->param.silent_curve_strength, 0.f, 1.f, xorstr("%.2f") });
		search_elements.push_back(find_elements_slider_int{ option->name.miss_chance, &option->param.miss_chance, 0, 100, xorstr("%d%%") });
		search_elements.push_back(find_elements_checkbox{ option->name.disable_aimbot_while_flashed + std::string(xorstr(" Silent")), &option->param.disable_aimbot_while_flashed });
		search_elements.push_back(find_elements_checkbox{ option->name.disable_aimbot_through_smoke + std::string(xorstr(" Silent")), &option->param.disable_aimbot_through_smoke });
		search_elements.push_back(find_elements_slider_float{ option->name.assist_delay, &option->param.assist_delay, 0.0f, 1.0f, xorstr("%.3f") });

		search_elements.push_back(find_elements_checkbox{ option->name.enable_triggerbot, &option->param.enable_triggerbot, true });
		search_elements.push_back(find_elements_slider_int{ option->name.delay_ms, &option->param.triggerbot_delay, 0, 1000, xorstr("%dms") });
		search_elements.push_back(find_elements_slider_int{ option->name.hit_chance + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_hit_chance, 0, 100, xorstr("%d%%") });
		search_elements.push_back(find_elements_checkbox{ option->name.triggerbot_ignore_npcs + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_ignore_npcs });
		search_elements.push_back(find_elements_checkbox{ option->name.triggerbot_ignore_friends + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_ignore_friends });
		search_elements.push_back(find_elements_checkbox{ option->name.triggerbot_check_visible + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_check_visible });
		search_elements.push_back(find_elements_checkbox{ option->name.target_combat_roll + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_target_combat_roll });
		search_elements.push_back(find_elements_checkbox{ option->name.triggerbot_target_line + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_target_line });
		search_elements.push_back(find_elements_checkbox{ option->name.triggerbot_curving + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_curving });
		search_elements.push_back(find_elements_slider_float{ option->name.triggerbot_curve_strength + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_curve_strength, 0.f, 1.f, xorstr("%.2f") });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.triggerbot_fov_circle + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_fov_circle, option->param.triggerbot_fov_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.triggerbot_fov_size + std::string(xorstr(" Triggerbot")), &option->param.triggerbot_fov_size, 10.f, 1000.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_dropdown{ option->name.triggerbot_hitbone, &option->param.triggerbot_hitbone, option->item.hitbones });
		search_elements.push_back(find_elements_checkbox{ option->name.smart_trigger, &option->param.smart_trigger });
		search_elements.push_back(find_elements_range_int{ option->name.hitting_chance, &option->param.hitting_chance_one, &option->param.hitting_chance_two, 0, 100, 1, xorstr("%d%%")});
		search_elements.push_back(find_elements_checkbox{ option->name.disable_triggerbot_through_smoke, &option->param.disable_triggerbot_through_smoke });
		search_elements.push_back(find_elements_checkbox{ option->name.disable_triggerbot_while_flashed, &option->param.disable_triggerbot_while_flashed, true });

		search_elements.push_back(find_elements_checkbox{ option->name.enable_antiaimbot, &option->param.enable_antiaimbot, true });
		search_elements.push_back(find_elements_slider_int{ option->name.add_lby_flip_yaw, &option->param.add_lby_flip_yaw, 0, 180, xorstr("%d%%") });
		search_elements.push_back(find_elements_dropdown{ option->name.real_yaw, &option->param.real_yaw, option->item.real_yaw });

		search_elements.push_back(find_elements_checkbox{ option->name.enable_recoil, &option->param.enable_recoil });
		search_elements.push_back(find_elements_checkbox{ option->name.return_crosshair, &option->param.return_crosshair, true });
		search_elements.push_back(find_elements_slider_int{ option->name.enable_bullet, &option->param.enable_bullet, 1, 10 });
		search_elements.push_back(find_elements_slider_int{ option->name.x_axis, &option->param.x_axis, 0, 100, xorstr("%d%%") });
		search_elements.push_back(find_elements_slider_int{ option->name.y_axis, &option->param.y_axis, 0, 100, xorstr("%d%%") });

		search_elements.push_back(find_elements_checkbox{ option->name.enable_esp, &option->param.enable_esp, true, &option->key.enable_esp_key, &option->key.enable_esp_mode });
		search_elements.push_back(find_elements_checkbox{ option->name.dormant, &option->param.dormant });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.bounding_box, &option->param.bounding_box, option->param.box_color, true, false });
		search_elements.push_back(find_elements_dropdown{ option->name.box_style, &option->param.box_style, option->item.box_styles });

		search_elements.push_back(find_elements_checkbox{ option->name.health_bar, &option->param.health_bar });
		search_elements.push_back(find_elements_dropdown{ option->name.health_bar_pos, &option->param.health_bar_pos, option->item.positions });

		search_elements.push_back(find_elements_checkbox{ option->name.armor_bar, &option->param.armor_bar });
		search_elements.push_back(find_elements_dropdown{ option->name.armor_bar_pos, &option->param.armor_bar_pos, option->item.positions });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.skeleton, &option->param.skeleton, option->param.skeleton_color, true, false });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.names, &option->param.names, option->param.names_color, true, false });
		search_elements.push_back(find_elements_dropdown{ option->name.names_pos, &option->param.names_pos, option->item.positions });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.weapon_name, &option->param.weapon_name, option->param.weapon_name_color, true, false });
		search_elements.push_back(find_elements_dropdown{ option->name.weapon_name_pos, &option->param.weapon_name_pos, option->item.positions });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.distance, &option->param.distance, option->param.distance_color, true, false });
		search_elements.push_back(find_elements_checkbox{ option->name.dotbones, &option->param.dotbones });
		search_elements.push_back(find_elements_dropdown{ option->name.distance_pos, &option->param.distance_pos, option->item.positions });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.snaplines, &option->param.snaplines, option->param.snaplines_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.esp_max_distance, &option->param.esp_max_distance, 10.f, 2000.f, xorstr("%.1fm") });

        search_elements.push_back(find_elements_clr_checkbox{ option->name.team_check, &option->param.team_check, option->param.team_check_color, true, false });
        search_elements.push_back(find_elements_clr_checkbox{ option->name.visible_check, &option->param.visible_check, option->param.visible_check_color, true, false });
        search_elements.push_back(find_elements_clr_checkbox{ option->name.arrows, &option->param.arrows, option->param.arrows_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.arrows_size, &option->param.arrows_size, 5.f, 30.f, xorstr("%.1f") });

        search_elements.push_back(find_elements_checkbox{ std::string(xorstr("Silent: ")) + option->name.target_jump, &option->param.silent_target_jump });
        search_elements.push_back(find_elements_checkbox{ std::string(xorstr("Triggerbot: ")) + option->name.target_jump, &option->param.triggerbot_target_jump });

		search_elements.push_back(find_elements_checkbox{ option->name.outline_text, &option->param.outline_text });
		search_elements.push_back(find_elements_slider_float{ option->name.opacity, &option->param.opacity, 0.f, 1.f, xorstr("%.2f") });
		search_elements.push_back(find_elements_slider_float{ option->name.line_thickness, &option->param.line_thickness, 0.5f, 5.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.ui_font_size, &option->param.font_size, 5.f, 30.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.ui_icon_size, &option->param.icon_size, 0.1f, 3.f, xorstr("%.2f") });

		search_elements.push_back(find_elements_checkbox{ option->name.enable_chams, &option->param.enable_chams });
		search_elements.push_back(find_elements_color{ option->name.chams_visible_color, option->param.chams_visible_color, true });
		search_elements.push_back(find_elements_color{ option->name.chams_invisible_color, option->param.chams_invisible_color, true });
		search_elements.push_back(find_elements_checkbox{ option->name.wireframe, &option->param.wireframe });
		search_elements.push_back(find_elements_checkbox{ option->name.flat_shading, &option->param.flat_shading });

		search_elements.push_back(find_elements_checkbox{ std::string(xorstr("Friendlist Keybind")), &var->friends_tab.enable_setfriend_keybind, true, &var->friends_tab.setfriend_key, &var->friends_tab.setfriend_key_mode });

		search_elements.push_back(find_elements_checkbox{ option->name.esp_ignore_npcs, &option->param.esp_ignore_npcs });
		search_elements.push_back(find_elements_checkbox{ option->name.esp_ignore_dead, &option->param.esp_ignore_dead });
		search_elements.push_back(find_elements_checkbox{ option->name.show_local_player, &option->param.show_local_player });
		search_elements.push_back(find_elements_checkbox{ option->name.playerlist_display_peds, &option->param.playerlist_display_peds });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.tracer, &option->param.tracer, option->param.tracer_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.tracer_duration, &option->param.tracer_duration, 0.1f, 10.f, xorstr("%.1fs") });
		search_elements.push_back(find_elements_slider_float{ option->name.tracer_thickness, &option->param.tracer_thickness, 0.1f, 10.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.visualize_aimbot, &option->param.visualize_aimbot, option->param.visualize_aimbot_color, true, false });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.player_chams, &option->param.player_chams, option->param.player_chams_color, true, false });
		search_elements.push_back(find_elements_checkbox{ option->name.player_chams_flicker, &option->param.player_chams_flicker });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.override_sky_color, &option->param.override_sky_color, option->param.sky_color, true, false });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_sky_zenith_color, &option->param.enable_sky_zenith_color });
		search_elements.push_back(find_elements_clr_color{ option->name.sky_zenith_color, option->param.sky_zenith_color, true, false });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_sky_sun_color, &option->param.enable_sky_sun_color });
		search_elements.push_back(find_elements_clr_color{ option->name.sky_sun_color, option->param.sky_sun_color, true, false });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_sky_sun_disc, &option->param.enable_sky_sun_disc });
		search_elements.push_back(find_elements_clr_color{ option->name.sky_sun_disc_color, option->param.sky_sun_disc_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.sky_sun_disc_size, &option->param.sky_sun_disc_size, 0.0f, 10.0f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_sky_moon, &option->param.enable_sky_moon });
		search_elements.push_back(find_elements_clr_color{ option->name.sky_moon_color, option->param.sky_moon_color, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.sky_moon_disc_size, &option->param.sky_moon_disc_size, 0.0f, 10.0f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.sky_moon_iten, &option->param.sky_moon_iten, 0.0f, 10.0f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_sky_stars, &option->param.enable_sky_stars });
		search_elements.push_back(find_elements_slider_float{ option->name.sky_stars_iten, &option->param.sky_stars_iten, 0.0f, 50.0f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_sky_clouds, &option->param.enable_sky_clouds });
		search_elements.push_back(find_elements_slider_float{ option->name.sky_cloud_density_mult, &option->param.sky_cloud_density_mult, 0.0f, 10.0f, xorstr("%.1f") });
		search_elements.push_back(find_elements_clr_color{ option->name.sky_cloud_mid_col, option->param.sky_cloud_mid_col, true, false });
		search_elements.push_back(find_elements_clr_color{ option->name.sky_cloud_base_col, option->param.sky_cloud_base_col, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.sky_cloud_overall_strength, &option->param.sky_cloud_overall_strength, 0.0f, 10.0f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_light_rays, &option->param.enable_light_rays });
		search_elements.push_back(find_elements_slider_float{ option->name.light_ray_mult, &option->param.light_ray_mult, 0.0f, 10.0f, xorstr("%.1f") });
		search_elements.push_back(find_elements_clr_color{ option->name.light_ray_col, option->param.light_ray_col, true, false });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_fog_haze, &option->param.enable_fog_haze });
		search_elements.push_back(find_elements_clr_color{ option->name.fog_haze_col, option->param.fog_haze_col, true, false });
		search_elements.push_back(find_elements_slider_float{ option->name.fog_haze_density, &option->param.fog_haze_density, 0.0f, 10.0f, xorstr("%.1f") });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.player_behind_wall, &option->param.player_behind_wall, option->param.player_behind_wall_color, true, false });
		search_elements.push_back(find_elements_dropdown{ option->name.behind_selection, &option->param.behind_selection, option->item.behind_selection });
		search_elements.push_back(find_elements_checkbox{ option->name.teammate, &option->param.teammate });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.teammate_behind_wall, &option->param.teammate_behind_wall, option->param.teammate_behind_wall_color, true, false });

		search_elements.push_back(find_elements_checkbox{ option->name.no_clip, &option->param.no_clip, true, &option->key.noclip_key, &option->key.noclip_mode });
		search_elements.push_back(find_elements_checkbox{ option->name.spinbot, &option->param.spinbot });
		search_elements.push_back(find_elements_slider_float{ option->name.noclip_speed, &option->param.noclip_speed, 1.f, 50.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.strafe, &option->param.strafe });
		search_elements.push_back(find_elements_checkbox{ option->name.teleport_behind_enemy, &option->param.teleport_behind_enemy });
		search_elements.push_back(find_elements_slider_float{ option->name.teleport_behind_enemy_distance, &option->param.teleport_behind_enemy_distance, 1.f, 20.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.teleport_behind_enemy_fov, &option->param.teleport_behind_enemy_fov, 10.f, 180.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_int{ option->name.teleport_behind_enemy_range, &option->param.teleport_behind_enemy_range, 10, 500, xorstr("%dm") });

		search_elements.push_back(find_elements_checkbox{ option->name.god_mode, &option->param.god_mode });
		search_elements.push_back(find_elements_checkbox{ option->name.teleport, &option->param.teleport });

		search_elements.push_back(find_elements_checkbox{ option->name.bring_vehicle, &option->param.bring_vehicle });
		search_elements.push_back(find_elements_checkbox{ option->name.warp_into_vehicle, &option->param.warp_into_vehicle });
		search_elements.push_back(find_elements_checkbox{ option->name.explode_vehicle, &option->param.explode_vehicle });
		search_elements.push_back(find_elements_checkbox{ option->name.fast_run, &option->param.fast_run });
		search_elements.push_back(find_elements_checkbox{ option->name.vehicle_god_mode, &option->param.vehicle_god_mode });
		search_elements.push_back(find_elements_checkbox{ option->name.horn_boost, &option->param.horn_boost_enabled });
		search_elements.push_back(find_elements_checkbox{ option->name.modify_vehicle_acceleration, &option->param.modify_vehicle_acceleration });
		search_elements.push_back(find_elements_checkbox{ option->name.modify_vehicle_traction, &option->param.modify_vehicle_traction });
		search_elements.push_back(find_elements_slider_float{ option->name.run_speed, &option->param.run_speed, 1.f, 10.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.max_health, &option->param.max_health, 100.f, 1000.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.max_armor, &option->param.max_armor, 100.f, 1000.f, xorstr("%.1f") });

		search_elements.push_back(find_elements_checkbox{ option->name.beast_jump, &option->param.beast_jump });
		search_elements.push_back(find_elements_checkbox{ option->name.super_jump, &option->param.super_jump });
		search_elements.push_back(find_elements_checkbox{ option->name.inf_combat_roll, &option->param.inf_combat_roll });
		search_elements.push_back(find_elements_checkbox{ option->name.explosive_fist, &option->param.explosive_fist });
		search_elements.push_back(find_elements_checkbox{ option->name.fire_ammo, &option->param.fire_ammo });
		search_elements.push_back(find_elements_checkbox{ option->name.explosive_ammo, &option->param.explosive_ammo });
		search_elements.push_back(find_elements_checkbox{ option->name.no_rag_doll, &option->param.no_rag_doll });
		search_elements.push_back(find_elements_checkbox{ option->name.force_weapon_wheel, &option->param.force_weapon_wheel });
		search_elements.push_back(find_elements_checkbox{ option->name.steal_car, &option->param.steal_car });
		search_elements.push_back(find_elements_checkbox{ option->name.disable_melee, &option->param.disable_melee });
		search_elements.push_back(find_elements_checkbox{ option->name.disable_evasive_dives, &option->param.disable_evasive_dives });
		search_elements.push_back(find_elements_checkbox{ option->name.disable_player_lockon, &option->param.disable_player_lockon });
		search_elements.push_back(find_elements_checkbox{ option->name.disable_unarmed_drivebys, &option->param.disable_unarmed_drivebys });
		search_elements.push_back(find_elements_checkbox{ option->name.no_ragdoll_bullet, &option->param.no_ragdoll_bullet });
		search_elements.push_back(find_elements_checkbox{ option->name.no_ragdoll_explosion, &option->param.no_ragdoll_explosion });
		search_elements.push_back(find_elements_checkbox{ option->name.no_ragdoll_fire, &option->param.no_ragdoll_fire });
		search_elements.push_back(find_elements_checkbox{ option->name.no_ragdoll_vehicle, &option->param.no_ragdoll_vehicle });
		search_elements.push_back(find_elements_checkbox{ option->name.treat_as_player_targeting, &option->param.treat_as_player_targeting });
		search_elements.push_back(find_elements_checkbox{ option->name.block_weapon_switch, &option->param.block_weapon_switch });
		search_elements.push_back(find_elements_checkbox{ option->name.no_collision, &option->param.no_collision });
		search_elements.push_back(find_elements_checkbox{ option->name.shrink_enabled, &option->param.shrink_enabled });
		search_elements.push_back(find_elements_slider_float{ option->name.shrink_scale, &option->param.shrink_scale, 0.1f, 1.f, xorstr("%.2f") });
		search_elements.push_back(find_elements_checkbox{ option->name.big_ped_enabled, &option->param.big_ped_enabled });
		search_elements.push_back(find_elements_slider_float{ option->name.big_ped_scale, &option->param.big_ped_scale, 1.1f, 10.f, xorstr("%.1f") });

		search_elements.push_back(find_elements_checkbox{ option->name.infinite_stamina, &option->param.infinite_stamina });
		search_elements.push_back(find_elements_checkbox{ option->name.seat_belt, &option->param.seat_belt });
		search_elements.push_back(find_elements_checkbox{ option->name.anti_headshot, &option->param.anti_headshot });
		search_elements.push_back(find_elements_checkbox{ option->name.electron, &option->param.electron });
		search_elements.push_back(find_elements_checkbox{ option->name.custom_fov, &option->param.custom_fov });
		search_elements.push_back(find_elements_slider_float{ option->name.fov_value, &option->param.fov_value, 10.f, 150.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.damage_boost, &option->param.damage_boost });
		search_elements.push_back(find_elements_slider_float{ option->name.damage_boost_value, &option->param.damage_boost_value, 1.f, 10.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ option->name.recoil_value, &option->param.recoil_value, 0.f, 1.f, xorstr("%.2f") });
		search_elements.push_back(find_elements_slider_float{ option->name.spread_value, &option->param.spread_value, 0.f, 1.f, xorstr("%.2f") });
		search_elements.push_back(find_elements_checkbox{ option->name.no_reload, &option->param.no_reload });
		search_elements.push_back(find_elements_slider_float{ option->name.weapon_range, &option->param.weapon_range, 10.f, 1000.f, xorstr("%.1fm") });

		search_elements.push_back(find_elements_checkbox{ option->name.infinite_ammo, &option->param.infinite_ammo });
		search_elements.push_back(find_elements_checkbox{ option->name.no_recoil_exploit, &option->param.no_recoil_exploit });
		search_elements.push_back(find_elements_checkbox{ option->name.no_spread, &option->param.no_spread });
		search_elements.push_back(find_elements_checkbox{ option->name.rapid_fire, &option->param.rapid_fire });
		search_elements.push_back(find_elements_checkbox{ option->name.invisible, &option->param.invisible });

		search_elements.push_back(find_elements_checkbox{ option->name.remove_flashbang_effect, &option->param.remove_flashbang_effect });
		search_elements.push_back(find_elements_checkbox{ option->name.remove_smoke_grenades, &option->param.remove_smoke_grenades });
		search_elements.push_back(find_elements_checkbox{ option->name.remove_skybox, &option->param.remove_skybox, true });
		search_elements.push_back(find_elements_range_int{ option->name.the_power_of_flashbang, &option->param.the_power_of_flashbang_one, &option->param.the_power_of_flashbang_two, 0, 100, 1, xorstr("%d%%")});

		search_elements.push_back(find_elements_range_float{ option->name.transparent_walls, &option->param.transparent_walls_one,  &option->param.transparent_walls_two, 0.0f, 10.0f, 0.1f, xorstr("%.1f") });

		search_elements.push_back(find_elements_checkbox{ option->name.invisible_spoof, &option->param.invisible_spoof });

		search_elements.push_back(find_elements_clr_checkbox{ option->name.auto_peek, &option->param.auto_peek, option->param.auto_peek_color, true, true });

		search_elements.push_back(find_elements_checkbox{ option->name.hitbox_expander, &option->param.hitbox_expander, true, &option->key.hitbox_expander_key, &option->key.hitbox_expander_mode });
		search_elements.push_back(find_elements_checkbox{ option->name.hitbox_magic_bullets, &option->param.hitbox_magic_bullets, false });
		search_elements.push_back(find_elements_slider_float{ option->name.hitbox_expander_size, &option->param.hitbox_expander_size, 0.0f, 3.0f, xorstr("%.2f") });

		search_elements.push_back(find_elements_checkbox{ option->name.vehicle_esp, &option->param.vehicle_esp });
		search_elements.push_back(find_elements_slider_float{ option->name.vehicle_max_distance, &option->param.vehicle_max_distance, 0.f, 2000.f, xorstr("%.1fm") });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.vehicle_snaplines, &option->param.vehicle_snaplines, option->param.vehicle_snaplines_color, true, false });
		search_elements.push_back(find_elements_checkbox{ option->name.vehicle_name, &option->param.vehicle_name });
		search_elements.push_back(find_elements_checkbox{ option->name.vehicle_distance, &option->param.vehicle_distance });
		search_elements.push_back(find_elements_checkbox{ option->name.vehicle_lock, &option->param.vehicle_lock });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.vehicle_3d_box, &option->param.vehicle_3d_box, option->param.vehicle_3d_box_color, true, false });

		search_elements.push_back(find_elements_checkbox{ option->name.object_esp, &option->param.object_esp });
		search_elements.push_back(find_elements_slider_float{ option->name.object_max_distance, &option->param.object_max_distance, 0.f, 2000.f, xorstr("%.1fm") });
		search_elements.push_back(find_elements_checkbox{ option->name.object_name, &option->param.object_name });
		search_elements.push_back(find_elements_checkbox{ option->name.object_distance, &option->param.object_distance });
		search_elements.push_back(find_elements_clr_checkbox{ option->name.object_3d_box, &option->param.object_3d_box, option->param.object_3d_box_color, true, false });

		search_elements.push_back(find_elements_checkbox{ option->name.vehicle_god_mode, &option->param.vehicle_god_mode });

		search_elements.push_back(find_elements_checkbox{ xorstr("Watermark"), &option->param.watermark });
		search_elements.push_back(find_elements_checkbox{ xorstr("FPS Counter"), &option->param.fps_counter });
		search_elements.push_back(find_elements_checkbox{ xorstr("Coordinates"), &option->param.coordinates });
		search_elements.push_back(find_elements_checkbox{ xorstr("Radar"), &option->param.radar });
		search_elements.push_back(find_elements_checkbox{ xorstr("Enhanced Minimap"), &option->param.enhanced_minimap });
		search_elements.push_back(find_elements_checkbox{ xorstr("Stream Proof"), &option->param.stream_proof });
		search_elements.push_back(find_elements_checkbox{ xorstr("Array List"), &option->param.array_list });
		search_elements.push_back(find_elements_checkbox{ xorstr("VSync"), &option->param.vsync });
		search_elements.push_back(find_elements_checkbox{ option->name.enable_rgb_particles, &option->param.enable_rgb_particles });
		search_elements.push_back(find_elements_dropdown{ option->name.process_priority, &option->param.process_priority, option->item.process_priorities });

		search_elements.push_back(find_elements_clr_checkbox{ xorstr("Custom Crosshair"), &option->param.custom_crosshair, option->param.crosshair_color, true, false });
		search_elements.push_back(find_elements_dropdown{ xorstr("Crosshair Style"), &option->param.crosshair_style, option->item.crosshair_styles });
		search_elements.push_back(find_elements_slider_float{ xorstr("Crosshair Size"), &option->param.crosshair_size, 1.f, 50.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_slider_float{ xorstr("Crosshair Thickness"), &option->param.crosshair_thickness, 0.5f, 10.f, xorstr("%.1f") });
		search_elements.push_back(find_elements_checkbox{ option->name.web_only, &option->param.web_only });
	}

	initialized = true;
}

void c_search::search()
{
	ImGuiContext& g = *GImGui;
	const ImVec2 pos = GetWindowPos();
	const ImVec2 size = GetWindowSize();
	ImDrawList* drawlist = GetWindowDrawList();

	bool hovered = g.HoveredWindow && (strstr(g.HoveredWindow->Name, xorstr("search_window")) || strstr(g.HoveredWindow->Name, xorstr("search_results_window")) || strstr(g.HoveredWindow->Name, xorstr("dropdown_window")) || strstr(g.HoveredWindow->Name, xorstr("coloredit_window")) || strstr(g.HoveredWindow->Name, xorstr("keybind_window")));
	if (!hovered && GetIO().MouseClicked[0] && window_alpha > 0.5f && !text_field_hovered && g.ActiveId == 0)
		active_searcing = false;

	bool found_any = false;
	if (!search_element.empty())
	{
		for (const auto& element : search_elements)
		{

			std::visit([&](const auto& e)
				{
					using T = std::decay_t<decltype(e)>;

                    if (!allowed_names.empty()) {
                        if (allowed_names.find(std::string(e.name)) == allowed_names.end())
                            return;
                    }
					std::string search_element_lower = search_element;
					std::transform(search_element_lower.begin(), search_element_lower.end(), search_element_lower.begin(), [](unsigned char c) { return std::tolower(c); });

                    std::string name_lower = std::string(e.name);
					std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), [](unsigned char c) { return std::tolower(c); });

					if (!search_element_lower.empty() && name_lower.find(search_element_lower) == std::string::npos)
						return;

					found_any = true;

					if constexpr (std::is_same_v<T, find_elements_checkbox>)
					{
						widgets->checkbox(e.name, e.callback, e.warning, e.key, e.mode);
					}
					if constexpr (std::is_same_v<T, find_elements_clr_checkbox>)
					{
						widgets->checkbox_with_picker(e.name, e.callback, e.col, e.alpha, e.warning);
					}
					if constexpr (std::is_same_v < T, find_elements_slider_int >)
					{
						widgets->slider_int(e.name, e.value, e.min, e.max, e.format);
					}
					if constexpr (std::is_same_v < T, find_elements_slider_float >)
					{
						widgets->slider_float(e.name, e.value, e.min, e.max, e.format);
					}
					if constexpr (std::is_same_v < T, find_elements_dropdown >)
					{
						widgets->dropdown(e.name, e.value, e.items);
					}
					if constexpr (std::is_same_v < T, find_elements_multi_dropdown >)
					{
						widgets->multi_dropdown(e.name, e.value, e.items);
					}
					if constexpr (std::is_same_v < T, find_elements_range_int >)
					{
						widgets->range_int(e.name, e.v1, e.v2, e.min, e.max, e.range, e.format);
					}
					if constexpr (std::is_same_v < T, find_elements_range_float >)
					{
						widgets->range_float(e.name, e.v1, e.v2, e.min, e.max, e.range, e.format);
					}
					if constexpr (std::is_same_v < T, find_elements_color >)
					{
						widgets->color_picker(e.name, e.col, e.alpha);
					}
					if constexpr (std::is_same_v < T, find_elements_clr_color >)
					{
						widgets->color_picker(e.name, e.col, e.alpha);
					}
					if constexpr (std::is_same_v < T, find_elements_textfield >)
					{
						widgets->text_field(e.icon, e.name, e.hint, e.value, sizeof e.value);
					}
				}, element);
		}
	}

	if (!found_any)
		draw->text_clipped(drawlist, var->font.instrument_medium[1], pos, pos + size, draw->get_clr(clr->text.text_active), xorstr("No such functions were found"), NULL, NULL, ImVec2(0.5f, 0.5f));
}
