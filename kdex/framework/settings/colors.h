#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <memory>
#include <vector>

class c_colors
{
public:
	struct
	{
		ImColor accent_clr{ 255, 255, 255, 255 };
		ImColor white_clr{ 255, 255, 255 };
		ImColor black_clr{ 0, 0, 0 };

		ImColor warning_clr{ 255, 205, 150 };
		ImColor error_clr{ 220, 110, 110 };
		ImColor success_clr{ 120, 255, 130 };

	} base_colors;

	struct
	{
		ImColor window_layout{ 13, 13, 16 };
		ImColor window_shadow{ 0, 0, 0, 0 };

		ImColor window_section{ 12, 12, 12 };
		ImColor window_stroke{ 44, 44, 52, 255 };

		ImColor scrollbar_clr{ 30, 30, 30 };
		ImColor separator{ 255, 255, 255, 20 };

	} window;

	struct
	{
		ImColor child_header{ 15, 15, 15 };
		ImColor child_layout{ 10, 10, 10 };
		ImColor child_stroke{ 255, 255, 255, 25 };

	} child;

	struct
	{
		ImColor layout{ 0, 0, 0 };

	} section;

	struct
	{
		ImColor layout{ 18, 18, 18 };
		ImColor stroke{ 255, 255, 255, 35 };

		ImColor dropdown_selected{ 45, 45, 45 };
		ImColor checkbox_circle{ 60, 60, 60 };

	} widget;

	struct
	{
		ImColor text_active{ 255, 255, 255, 255 };
		ImColor text_hovered{ 200, 200, 200, 255 };
		ImColor text_inactive{ 120, 120, 120, 255 };

	} text;

	struct
	{
		ImColor background{ 12, 12, 12 };
		ImColor title{ 20, 20, 20 };
		ImColor text{ 255, 255, 255 };
		ImColor desc{ 150, 150, 150 };
		ImColor stroke{ 255, 255, 255, 30 };
		ImColor red{ 255, 100, 100 };
	} lua;
};

inline std::unique_ptr<c_colors> clr = std::make_unique<c_colors>();
