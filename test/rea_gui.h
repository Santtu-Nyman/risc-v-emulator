#ifndef REL_RISC_V_GUI_H
#define REL_RISC_V_GUI_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include "rel_risc_v_emulator.h"
#include <SDL.h>
#include "stb_truetype.h"

#define REA_WINDOW_IS_SELECTABLE 0x1
#define REA_WINDOW_HAS_BORDER 0x2
#define REA_WINDOW_HAS_TEXT 0x4
#define REA_WINDOW_TEXT_IS_EDITABLE 0x8
#define REA_WINDOW_IS_VISIBLE 0x10
#define REA_WINDOW_IS_TOP_LEVEL 0x20
#define REA_CENTER_WINDOW_TEXT_HORIZONTALLY 0x40
#define REA_CENTER_WINDOW_TEXT_VERTICALLY 0x80
#define REA_WINDOW_IS_DROPDOWN_MENU 0x100

#define REA_GUI_TITLE "RISC-V Emulator"
#define REA_DEFAULT_BORDER_THICKNESS 2

#define REA_TEXT_ALLOCATION_GRANULARITY 128

#define REA_MAIN_WINDOW_ID 1
#define REA_TOP_BAR_WINDOW_ID 11
#define REA_TOP_FILE_BAR_WINDOW_ID 111
#define REA_TOP_FILE_BAR_TEXT_BOX_WINDOW_ID 1111
#define REA_LOAD_BIN_WINDOW_ID 112
#define REA_RESET_BOX_WINDOW_ID 113
#define REA_EXECUTE_BOX_WINDOW_ID 114
#define REA_REGISTER_FORMAT_WINDOW_ID 115
#define REA_REGISTER_FORMAT_MENU_WINDOW_ID 1151
#define REA_FRAME_COUNT_WINDOW_ID 116
#define REA_CODE_SEQUENCE_WINDOW_ID 12
#define REA_CODE_SEQUENCE_TITLE_WINDOW_ID 121
#define REA_CODE_SEQUENCE_VALUE_WINDOW_ID 122
#define REA_REGISTER_WINDOW_ID 13
#define REA_REGISTER_TITLE_WINDOW_ID 131
#define REA_REGISTER_FRAME_WINDOW_ID 132000
#define REA_REGISTER_VALUE_WINDOW_ID 133000
#define REA_MEMORY_MAP_WINDOW_ID 14

typedef struct rea_window_t
{
	int x;
	int y;
	int w;
	int h;
	int border_thickness;
	int font_size;
	uint32_t style_flags;
	uint32_t parent_id;
	uint32_t id;
	uint32_t background_color;
	uint32_t border_color;
	uint32_t text_color;
	size_t text_length;
	char* text;
	struct
	{
		size_t text_allocation_length;
		size_t selected_item;
		int z;
		int base_x;
		int base_y;
		int w;
		int h;
		int window_is_under_cursor;
		int window_is_selected;
		int update_texture;
		SDL_Texture* texture_handle;
	} paint_data;
} rea_window_t;

typedef struct rea_gui_t
{
	SDL_Window* window_handle;
	SDL_Renderer* renderer_handle;
	SDL_Event event;
	int control_key_active;
	rea_window_t* selected_window;
	rea_window_t* event_window;
	rea_window_t* main_window;
	uint32_t frame_timestamp;
	uint32_t frame_duration;
	size_t window_count;
	rea_window_t* window_table;
	size_t temporal_buffer_size;
	void* temporal_buffer;

	struct
	{
		stbtt_fontinfo* font;
	} test_font;
} rea_gui_t;

rea_window_t* rea_get_window_by_id(rea_gui_t* gui, uint32_t window_id);

rea_window_t* rea_get_window_by_coordinate(rea_gui_t* gui, int x, int y);

rea_window_t* rea_get_main_window(rea_gui_t* gui);

int rea_set_window_text(rea_gui_t* gui, uint32_t window_id, const char* text);

int rea_set_window_text_with_signed_number(rea_gui_t* gui, uint32_t window_id, int32_t value);

int rea_set_window_text_with_unsigned_number(rea_gui_t* gui, uint32_t window_id, uint32_t value);

int rea_set_window_text_with_hexadecimal_number(rea_gui_t* gui, uint32_t window_id, uint32_t value);

int rea_append_window_text(rea_gui_t* gui, uint32_t window_id, const char* text);

int rea_append_window_text_with_signed_number(rea_gui_t* gui, uint32_t window_id, int32_t value);

int rea_append_window_text_with_unsigned_number(rea_gui_t* gui, uint32_t window_id, uint32_t value);

int rea_append_window_text_with_hexadecimal_number(rea_gui_t* gui, uint32_t window_id, uint32_t value);

int rea_create_gui(rea_gui_t** gui_address, size_t gui_window_count, const rea_window_t* gui_window_table, uint32_t frame_per_second);

void rea_close_gui(rea_gui_t* gui);

int rea_poll_event(rea_gui_t* gui);

int rea_draw_window(rea_gui_t* gui, rea_window_t* window);

int rea_draw_gui(rea_gui_t* gui);

void rea_draw_dropdown_icon_bitmap(int w, int h, size_t stride, uint32_t background_color, uint32_t icon_color, uint32_t* image);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // REL_RISC_V_GUI_H