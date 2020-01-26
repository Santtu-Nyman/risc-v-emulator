/*
	sdui.h - v0.3 (2020-01-26) - public domain
	Authored from 2020 by Santtu Nyman

	This file is part of my RISC-V emulator project.
	It is used for creating user interface for the emulator program using SLD2 library and stb truetype library.
	These libraries are separate from this software and have different authors and licenses.
	This software was originally created for the emulator, but you can use it for what ever you want.

	------------------------------------------------------------------------------
	This software is available under public domain license.
	------------------------------------------------------------------------------
	Public Domain (www.unlicense.org)
	This is free and unencumbered software released into the public domain.
	Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
	software, either in source code form or as a compiled binary, for any purpose,
	commercial or non-commercial, and by any means.
	In jurisdictions that recognize copyright laws, the author or authors of this
	software dedicate any and all copyright interest in the software to the public
	domain. We make this dedication for the benefit of the public at large and to
	the detriment of our heirs and successors. We intend this dedication to be an
	overt act of relinquishment in perpetuity of all present and future rights to
	this software under copyright law.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	------------------------------------------------------------------------------
*/

#ifndef SDUI_H
#define SDUI_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <SDL.h>
#include "sdui_base.h"
#include "sdui_icon.h"
#include "sdui_font.h"

#define SDUI_WINDOW_IS_SELECTABLE 0x1
#define SDUI_WINDOW_IS_VISIBLE 0x2
#define SDUI_WINDOW_IS_HIGHLIGHT 0x4
#define SDUI_WINDOW_IS_POPUP 0x8
#define SDUI_WINDOW_HAS_BORDER 0x10
#define SDUI_WINDOW_HAS_TEXT 0x20
#define SDUI_WINDOW_TEXT_IS_SELECTABLE 0x40
#define SDUI_WINDOW_TEXT_IS_EDITABLE 0x80
#define SDUI_CENTER_WINDOW_TEXT_HORIZONTALLY 0x100
#define SDUI_CENTER_WINDOW_TEXT_VERTICALLY 0x200
#define SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR 0x400
#define SDUI_WINDOW_HAS_RENDER_CALLBACK 0x800
#define SDUI_WINDOW_IS_DROPDOWN_MENU 0x1000

#define SDUI_WINDOW_TYPE_MAIN (SDUI_WINDOW_IS_VISIBLE)
#define SDUI_WINDOW_TYPE_TEXT (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_SELECTABLE | SDUI_WINDOW_IS_SELECTABLE | SDUI_WINDOW_IS_VISIBLE)
#define SDUI_WINDOW_TYPE_TEXT_EDIT (SDUI_WINDOW_TYPE_TEXT | SDUI_WINDOW_TEXT_IS_EDITABLE)
#define SDUI_WINDOW_TYPE_BUTTON (SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR | SDUI_WINDOW_HAS_TEXT | SDUI_CENTER_WINDOW_TEXT_HORIZONTALLY | SDUI_CENTER_WINDOW_TEXT_VERTICALLY | SDUI_WINDOW_IS_SELECTABLE | SDUI_WINDOW_IS_VISIBLE)
#define SDUI_WINDOW_TYPE_DROPDOWN_MENU (SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR | SDUI_WINDOW_IS_DROPDOWN_MENU | SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_IS_SELECTABLE | SDUI_CENTER_WINDOW_TEXT_VERTICALLY | SDUI_WINDOW_IS_VISIBLE)

#define SDUI_TEXT_ALLOCATION_GRANULARITY 128

#define SDUI_BASE_Z_NORMAL 0x000
#define SDUI_BASE_Z_POPUP 0x200
#define SDUI_BASE_Z_HIGHLIGHT 0x400
#define SDUI_SELECT_Z_INCRMENT 0x100
#define SDUI_MAXIMUM_Z 0x5FF

#define SDIU_WINDOW_STATE_ITERATED 0x1
#define SDIU_WINDOW_STATE_UPDATE_TEXTURE 0x2
#define SDIU_WINDOW_STATE_SINGLE_COLOR 0x4
#define SDIU_WINDOW_STATE_UNDER_CURSOR 0x8
#define SDIU_WINDOW_STATE_SELECTED 0x10
#define SDIU_WINDOW_STATE_SELECT_INCREMENTED 0x20

typedef struct sdui_window_parameters_t
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
	uint32_t pointed_background_color;
	uint32_t border_color;
	uint32_t text_color;
	char* text;
	void* user_data;
	int (*callback)(SDL_Renderer* renderer, uint32_t style_flags, uint32_t state_flags, SDL_Rect* draw_area, void* user_data);
} sdui_window_parameters_t;

typedef struct sdui_window_t
{
	sdui_window_parameters_t parameters;
	size_t text_length;
	size_t text_selection_offset;
	size_t text_selection_length;
	size_t text_allocation_length;
	size_t pointed_item;
	size_t selected_item;
	int state_flags;
	int text_x;
	int text_y;
	int z;
	int base_x;
	int base_y;
	int w;
	int h;
	SDL_Texture* texture_handle;
} sdui_window_t;
	
typedef struct sdui_ui_t
{
	SDL_Window* window_handle;
	SDL_Renderer* renderer_handle;
	SDL_Event event;
	SDL_Rect select;
	int control_key_active;
	uint32_t pointed_window_id;
	uint32_t selected_window_id;
	uint32_t event_window_id;
	uint32_t main_window_id;
	uint32_t frame_timestamp;
	uint32_t frame_duration;
	size_t window_count;
	sdui_window_t* window_table;
	size_t temporal_buffer_size;
	stbtt_fontinfo* font;
	void* temporal_buffer;
} sdui_ui_t;

int sdui_create_gui(const char* title, sdui_ui_t** gui_address, size_t gui_window_count, const sdui_window_parameters_t* gui_window_table, uint32_t frame_per_second);

void sdui_close_gui(sdui_ui_t* gui);

int sdui_poll_event(sdui_ui_t* gui);

int sdui_draw_window(sdui_ui_t* gui, sdui_window_t* window);

int sdui_draw_gui(sdui_ui_t* gui);

int sdui_grow_gui_temporal_buffer(sdui_ui_t* gui, size_t required_size);

sdui_window_t* sdui_get_window_by_id(sdui_ui_t* gui, uint32_t window_id);

sdui_window_t* sdui_get_window_by_coordinate(sdui_ui_t* gui, int x, int y);

sdui_window_t* sdui_get_main_window(sdui_ui_t* gui);

int sdui_create_window_texture(sdui_ui_t* gui, sdui_window_t* window);

int sdui_set_window_text(sdui_ui_t* gui, uint32_t window_id, const char* text);

int sdui_set_window_text_with_signed_number(sdui_ui_t* gui, uint32_t window_id, int32_t value);

int sdui_set_window_text_with_unsigned_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value);

int sdui_set_window_text_with_hexadecimal_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value);

int sdui_append_window_text(sdui_ui_t* gui, uint32_t window_id, const char* text);

int sdui_append_window_text_with_signed_number(sdui_ui_t* gui, uint32_t window_id, int32_t value);

int sdui_append_window_text_with_unsigned_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value);

int sdui_append_window_text_with_hexadecimal_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value);

sdui_window_t* sdui_select_window_text(sdui_ui_t* gui, int x, int y, int w, int h);

size_t sdui_get_window_item_by_coordinate(sdui_window_t* window, int x, int y);

int sdui_update_windows_z_coordinas(sdui_ui_t* gui);

void sdui_sort_windows_by_z_coordinate(sdui_ui_t* gui);

void sdui_update_windows_parent_x_and_y_coordinas(sdui_ui_t* gui, int base_x, int base_y);

void sdui_move_window(sdui_ui_t* gui, uint32_t window_id, int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SDUI_H