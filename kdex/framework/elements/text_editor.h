#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../thirdparty/imgui/imgui.h"

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <regex>

class c_text_editor
{
public:
	enum class pallete_index
	{
		def,
		keyword,
		number,
		string,
		char_literal,
		punctuation,
		preprocessor,
		identifier,
		known_identifier,
		preproc_identifier,
		comment,
		multi_line_comment,
		background,
		cursor,
		selection,
		error_marker,
		breakpoint,
		line_number,
		current_line_fill,
		current_line_fill_inactive,
		current_line_edge,
		max
	};

	enum class selection_mode
	{
		normal,
		word,
		line
	};

	struct breakpoint
	{
		int m_line;
		bool m_enabled;
		std::string m_condition;

		bool operator <(const breakpoint& other) const { return m_line < other.m_line; }
	};

	struct coordinates
	{
		int m_line, m_column;
		coordinates() : m_line(0), m_column(0) {}
		coordinates(int a_line, int a_column) : m_line(a_line), m_column(a_column)
		{

		}
		static coordinates invalid() { static coordinates invalid(-1, -1); return invalid; }

		bool operator ==(const coordinates& o) const
		{
			return m_line == o.m_line && m_column == o.m_column;
		}

		bool operator !=(const coordinates& o) const
		{
			return m_line != o.m_line || m_column != o.m_column;
		}

		bool operator <(const coordinates& o) const
		{
			if (m_line != o.m_line)
				return m_line < o.m_line;
			return m_column < o.m_column;
		}

		bool operator >(const coordinates& o) const
		{
			if (m_line != o.m_line)
				return m_line > o.m_line;
			return m_column > o.m_column;
		}

		bool operator <=(const coordinates& o) const
		{
			if (m_line != o.m_line)
				return m_line < o.m_line;
			return m_column <= o.m_column;
		}

		bool operator >=(const coordinates& o) const
		{
			if (m_line != o.m_line)
				return m_line > o.m_line;
			return m_column >= o.m_column;
		}
	};

	struct identifier
	{
		coordinates m_location;
		std::string m_declaration;
	};

	typedef std::string string;
	typedef std::unordered_map<std::string, identifier> identifiers;
	typedef std::unordered_set<std::string> keywords;
	typedef std::map<int, std::string> error_markers;
	typedef std::unordered_set<int> breakpoints;
	typedef std::vector<ImU32> pallete;
	typedef uint8_t char_t;

	struct glyph
	{
		char_t m_char;
		pallete_index m_color_idex = pallete_index::def;
		bool m_comment : 1;
		bool m_multiline_comment : 1;
		bool m_preprocessor : 1;

		glyph(char_t a_char, pallete_index a_color_index) : m_char(a_char), m_color_idex(a_color_index),
			m_comment(false), m_multiline_comment(false), m_preprocessor(false) {}
	};

	typedef std::vector<glyph> line;
	typedef std::vector<line> lines;

	struct language_definition
	{
		typedef std::pair<std::string, pallete_index> token_regex_string;
		typedef std::vector<token_regex_string> token_regex_strings;
		typedef bool(*token_ize_callback)(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, pallete_index& paletteIndex);

		std::string m_name;
		keywords m_keywords;
		identifiers m_identifiers;
		identifiers m_preproc_identifiers;
		token_regex_strings m_token_regex_strings;
		std::string m_comment_start, m_comment_end, m_single_line_comment;
		char m_preproc_char;
		bool m_auto_indentation;
		bool m_case_sensitive;
		token_ize_callback m_tokenize;

		language_definition()
			: m_preproc_char('#'), m_auto_indentation(true), m_case_sensitive(true), m_tokenize(nullptr)
		{
		}

		static const language_definition& CPlusPlus();
		static const language_definition& HLSL();
		static const language_definition& GLSL();
		static const language_definition& C();
		static const language_definition& SQL();
		static const language_definition& AngelScript();
		static const language_definition& Lua();
	};

	c_text_editor();
	~c_text_editor();

	void set_language_definition(const language_definition& aLanguageDef);
	const language_definition& get_language_definition() const { return m_language_definition; }

	const pallete& get_pallete() const { return m_pallete_base; }
	void set_pallete(const pallete& aValue);

	void set_error_markers(const error_markers& aMarkers) { m_error_markers = aMarkers; }
	void set_breakpoints(const breakpoints& aMarkers) { m_breakpoints = aMarkers; }

	void render(const char* aTitle, const ImVec2& aSize = ImVec2(), bool aBorder = false);
	void render_internal();
	void set_text(const std::string& aText);
	std::string get_text() const;

	void set_text_lines(const std::vector<std::string>& aLines);
	std::vector<std::string> get_text_lines() const;

	std::string get_selected_text() const;
	std::string get_current_line_text()const;

	int get_total_lines() const { return (int)m_lines.size(); }
	bool is_overwrite() const { return m_overwrite; }

	void set_read_only(bool aValue);
	bool is_read_only() const { return m_read_only; }

	bool is_text_changed() const { return m_text_changed; }
	bool is_cursor_position_changed() const { return m_cursor_position_changed; }

	bool is_colorizer_enabled() const { return m_colorizer_enabled; }
	void set_colorizer_enable(bool aValue);

	coordinates get_cursor_position() const { return get_actual_cursor_coordinates(); }
	void set_cursor_position(const coordinates& aPosition);

	void set_handle_mouse_inputs(bool aValue) { m_handle_mouse_inputs = aValue; }
	bool is_handle_mouse_inputs_enabled() const { return m_handle_mouse_inputs; }

	void set_handle_keyboard_inputs(bool aValue) { m_handle_keyboard_inputs = aValue; }
	bool is_handle_keyboard_inputs_enabled() const { return m_handle_keyboard_inputs; }

	void set_im_gui_child_ignored(bool aValue) { m_ignore_imgui_child = aValue; }
	bool is_im_gui_child_ignored() const { return m_ignore_imgui_child; }

	void set_show_whitespaces(bool aValue) { m_show_whitespaces = aValue; }
	bool is_show_whitespaces_enabled() const { return m_show_whitespaces; }

	void set_tab_size(int aValue);
	int get_tab_size() const { return m_tab_size; }

	void insert_text(const std::string& aValue);
	void insert_text(const char* aValue);

	void move_up(int aAmount = 1, bool aSelect = false);
	void move_down(int aAmount = 1, bool aSelect = false);
	void move_left(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void move_right(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void move_top(bool aSelect = false);
	void move_bottom(bool aSelect = false);
	void move_home(bool aSelect = false);
	void move_end(bool aSelect = false);

	void set_selection_start(const coordinates& aPosition);
	void set_selection_end(const coordinates& aPosition);
	void set_selection(const coordinates& aStart, const coordinates& aEnd, selection_mode aMode = selection_mode::normal);
	void select_word_under_cursor();
	void select_all();
	bool has_selection() const;

	void copy();
	void cut();
	void paste();
	void delete_();

	bool can_undo() const;
	bool can_redo() const;
	void undo(int aSteps = 1);
	void redo(int aSteps = 1);

	static const pallete& get_dark_palette();
	static const pallete& get_light_palette();
	static const pallete& get_retro_blue_palette();

	friend struct undo_record;

	struct editor_state
	{
		coordinates m_cursor_position;

		coordinates m_selection_start;
		coordinates m_selection_end;

		editor_state()
			: m_cursor_position(0, 0)
			, m_selection_start(0, 0)
			, m_selection_end(0, 0)
		{}
	};

	struct undo_record
	{
		undo_record() {}
		~undo_record() {}

		undo_record(
			const std::string& aAdded,
			const c_text_editor::coordinates aAddedStart,
			const c_text_editor::coordinates aAddedEnd,
			const std::string& aRemoved,
			const c_text_editor::coordinates aRemovedStart,
			const c_text_editor::coordinates aRemovedEnd,
			c_text_editor::editor_state& aBefore,
			c_text_editor::editor_state& aAfter);

		std::string m_added;
		coordinates m_added_start;
		coordinates m_added_end;

		std::string m_removed;
		coordinates m_removed_start;
		coordinates m_removed_end;

		editor_state m_before;
		editor_state m_after;

		void undo(c_text_editor* aEditor);
		void redo(c_text_editor* aEditor);
	};

	typedef std::vector<undo_record> undo_buffer;

private:
	typedef std::vector<std::pair<std::regex, pallete_index>> regex_list;

	void process_inputs();
	void colorize(int aFromLine = 0, int aCount = -1);
	void colorize_range(int aFromLine = 0, int aToLine = 0);
	void colorize_internal();
	void handle_keyboard_inputs();
	void handle_mouse_inputs();
	float text_distance_to_line_start(const coordinates& aFrom) const;
	void ensure_cursor_visible();
	int get_page_size() const;
	std::string get_text(const coordinates& aStart, const coordinates& aEnd) const;
	coordinates get_actual_cursor_coordinates() const;
	coordinates sanitize_coordinates(const coordinates& aValue) const;
	void advance(coordinates& aCoordinates) const;
	void delete_range(const coordinates& aStart, const coordinates& aEnd);
	int insert_text_at(coordinates& aWhere, const char* aValue);
	void add_undo(undo_record& aValue);
	coordinates screen_pos_to_coordinates(const ImVec2& aPosition) const;
	coordinates find_word_start(const coordinates& aFrom) const;
	coordinates find_word_end(const coordinates& aFrom) const;
	coordinates find_next_word(const coordinates& aFrom) const;
	int get_character_index(const coordinates& aCoordinates) const;
	int get_character_column(int aLine, int aIndex) const;
	int get_line_character_count(int aLine) const;
	int get_line_max_column(int aLine) const;
	bool is_on_word_boundary(const coordinates& aAt) const;
	void remove_line(int aStart, int aEnd);
	void remove_line(int aIndex);
	line& insert_line(int aIndex);
	std::string get_word_under_cursor() const;
	std::string get_word_at(const coordinates& aCoords) const;
	ImU32 get_glyph_color(const glyph& aGlyph) const;

	void enter_character(ImWchar aChar, bool aShift);
	void backspace();
	void delete_selection();
	std::string get_selected_text(const coordinates& aStart, const coordinates& aEnd) const;

	float m_line_spacing;
	lines m_lines;
	editor_state m_state;
	undo_buffer m_undo_buffer;
	int m_undo_index;

	int m_tab_size;
	bool m_overwrite;
	bool m_read_only;
	bool m_within_render;
	bool m_scroll_to_cursor;
	bool m_scroll_to_top;
	bool m_text_changed;
	bool m_colorizer_enabled;
	float m_text_start;
	int m_left_margin;
	bool m_cursor_position_changed;
	int m_color_range_min, m_color_range_max;
	selection_mode m_selection_mode;
	bool m_check_comments;
	float m_last_click;
	bool m_handle_keyboard_inputs;
	bool m_handle_mouse_inputs;
	bool m_ignore_imgui_child;
	bool m_show_whitespaces;
	uint64_t m_start_time;

	pallete m_pallete_base;
	pallete m_pallete;
	language_definition m_language_definition;
	regex_list m_regex_list;

	bool m_check_multiline_comments;
	coordinates m_interactive_start, m_interactive_end;
	std::string m_line_buffer;
	uint64_t m_start_time_my;

	error_markers m_error_markers;
	breakpoints m_breakpoints;
	ImVec2 m_char_advance;
};
