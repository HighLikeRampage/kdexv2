#include "../settings/functions.h"

void c_gui::hud_navigator(std::string_view name, int* func_selected, std::vector<std::string> function, bool* show_navigator)
{
	struct hud_functional
	{
		ImVec2 content_size;
		float alpha;
	};

	ImGuiStyle* style = &GetStyle();

	hud_functional* state = gui->anim_container(&state, GetID(name.data()));
	gui->easing(state->alpha, *show_navigator ? 1.f : 0.f, 4.f, static_easing);

	if (state->alpha >= 0.01f)
	{

		gui->push_var(ImGuiStyleVar_WindowRounding, SCALE(elements->hud_navigation.rounding));
		gui->push_var(ImGuiStyleVar_WindowPadding, SCALE(elements->hud_navigation.hud_padding));
		gui->push_var(ImGuiStyleVar_ItemSpacing, SCALE(elements->hud_navigation.hud_spacing));
		gui->push_var(ImGuiStyleVar_Alpha, state->alpha);

		gui->set_next_window_pos(GetIO().DisplaySize - state->content_size);
		gui->begin(name.data() + std::to_string(1), nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		{
			const ImVec2 pos = gui->window_pos();
			const ImVec2 size = gui->window_size();
			ImDrawList* drawlist = GetWindowDrawList();

			draw->rect_filled(drawlist, pos, pos + size, draw->get_clr(clr->window.window_layout), SCALE(style->WindowRounding));
			draw->rect(drawlist, pos, pos + size, draw->get_clr(clr->window.window_stroke), SCALE(style->WindowRounding));

			if (widgets->button(xorstr("UP"), SCALE(280, 25)) && *func_selected > 0)
				*func_selected -= 1;

			if (widgets->begin_list(xorstr("LISTBOX") + std::to_string(1), SCALE(280, 0)))
			{
				for (int i = 0; i < function.size(); i++)
				{
					ImDrawFlags flags = (i == 0) ? ImDrawFlags_RoundCornersTop : (i == function.size() - 1) ? ImDrawFlags_RoundCornersBottom : ImDrawFlags_RoundCornersNone;
					if (function.size() == 1) flags = ImDrawFlags_RoundCornersAll;

					if (widgets->list_content(function[i].data(), (i == *func_selected), flags)) *func_selected = i;
				}
			}
			widgets->end_list();

			if (widgets->button(xorstr("DOWN"), SCALE(280, 25)) && *func_selected < (function.size() - 1))
				*func_selected += 1;

			state->content_size = gui->window_size() + SCALE(elements->hud_navigation.hud_setup_padding);
		}
		gui->end();

		gui->pop_var(4);
	}
}
