/*
	sdui.h - v0.1 (2020-01-20) - public domain
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

#include "sdui.h"
#include <assert.h>
#include <string.h>

int sdui_grow_gui_temporal_buffer(sdui_ui_t* gui, size_t required_size)
{
	if (gui->temporal_buffer_size >= required_size)
		return 0;

	void* buffer = realloc(gui->temporal_buffer, required_size);
	if (!buffer)
		return ENOMEM;

	gui->temporal_buffer_size = required_size;
	gui->temporal_buffer = buffer;
	return 0;
}

sdui_window_t* sdui_get_window_by_id(sdui_ui_t* gui, uint32_t window_id)
{
	for (size_t i = 0; i != gui->window_count; ++i)
		if (gui->window_table[i].parameters.id == window_id)
			return gui->window_table + i;
	return 0;
}

sdui_window_t* sdui_get_window_by_coordinate(sdui_ui_t* gui, int x, int y)
{
	// Windows need to be sorted by z and has to be paint_data set correctly

	for (size_t i = gui->window_count - 1; i != -1; --i)
		if ((x >= gui->window_table[i].base_x + gui->window_table[i].parameters.x) &&
			(x < gui->window_table[i].base_x + gui->window_table[i].parameters.x + gui->window_table[i].w) &&
			(y >= gui->window_table[i].base_y + gui->window_table[i].parameters.y) &&
			(y < gui->window_table[i].base_y + gui->window_table[i].parameters.y + gui->window_table[i].h))
			return gui->window_table + i;
	return 0;
}

sdui_window_t* sdui_get_main_window(sdui_ui_t* gui)
{
	// one main window must allways exist

	for (size_t i = 0;; ++i)
		if (!gui->window_table[i].parameters.parent_id)
			return gui->window_table + i;
}

int sdui_create_window_texture(sdui_ui_t* gui, sdui_window_t* window)
{


	int w = window->parameters.w;
	int h = window->parameters.h;

	if (!w || !h)
	{
		if (window->texture_handle)
			SDL_DestroyTexture(window->texture_handle);
		window->w = 0;
		window->h = 0;
		window->texture_handle = 0;
		window->update_texture = 0;
		return 0;
	}

	int border_h = (window->parameters.border_thickness < (h / 2)) ? window->parameters.border_thickness : (h / 2);
	int border_w = (window->parameters.border_thickness < (w / 2)) ? window->parameters.border_thickness : (w / 2);
	size_t dropdown_menu_item_count = 0;

	if ((window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && window->parameters.text && window->window_is_selected)
	{
		dropdown_menu_item_count = sdui_get_string_line_count(window->parameters.text);
		h += (2 * border_h) + ((int)dropdown_menu_item_count * window->parameters.font_size);
	}

	if (window->w != w || window->h != h)
	{
		if (window->texture_handle)
		{
			SDL_DestroyTexture(window->texture_handle);
			window->texture_handle = 0;
		}

		window->w = w;
		window->h = h;
		window->texture_handle = SDL_CreateTexture(gui->renderer_handle, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);
		if (!window->texture_handle)
		{
			window->w = 0;
			window->h = 0;
			return ENOMEM;
		}
	}

	if (gui->temporal_buffer_size < ((size_t)h * (size_t)w * 4))
	{
		int grow_gui_temporal_buffer_error = sdui_grow_gui_temporal_buffer(gui, (size_t)h * (size_t)w * 4);
		if (grow_gui_temporal_buffer_error)
			return grow_gui_temporal_buffer_error;
	}

	uint32_t background_color = ((window->parameters.style_flags & SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR) && !((window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && (window->window_is_selected)) &&
		(window == gui->pointed_window)) ? window->parameters.pointed_background_color : window->parameters.background_color;

	uint32_t* image = (uint32_t*)gui->temporal_buffer;

	for (int y = 0; y != border_h; ++y)
		for (int x = 0; x != w; ++x)
			image[y * w + x] = window->parameters.border_color;
	for (int y = border_h; y != (h - border_h); ++y)
	{
		for (int x = 0; x != border_w; ++x)
			image[y * w + x] = window->parameters.border_color;
		for (int x = border_w; x != (w - border_w); ++x)
			image[y * w + x] = background_color;
		for (int x = (w - border_w); x != w; ++x)
			image[y * w + x] = window->parameters.border_color;
	}
	for (int y = (h - border_h); y != h; ++y)
		for (int x = 0; x != w; ++x)
			image[y * w + x] = window->parameters.border_color;

	if (window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU)
	{
		if (window->window_is_selected)
		{
			sdui_fill_rectengle(w - (2 * border_w), window->parameters.h - (2 * border_h), w * sizeof(uint32_t), window->parameters.pointed_background_color, image + ((border_h * w) + border_w));
			sdui_fill_rectengle(w - (2 * border_w), window->parameters.font_size, w * sizeof(uint32_t), window->parameters.pointed_background_color, image + (((window->parameters.h + (window->pointed_item * window->parameters.font_size)) * w) + border_w));
		}

		int menu_icon_size = window->parameters.font_size ? window->parameters.font_size : (window->h < w ? window->h : w);
		if (menu_icon_size > (window->h - (2 * border_h)))
			menu_icon_size = (window->h - (2 * border_h));
		if (menu_icon_size > (window->w - (2 * border_w)))
			menu_icon_size = (window->w - (2 * border_w));
		int menu_icon_x = window->w - (menu_icon_size + border_w);
		int menu_icon_y = (window->parameters.h / 2) - (menu_icon_size / 2);
		sdui_draw_dropdown_icon_bitmap(menu_icon_size, menu_icon_size, w * 4, window->window_is_selected ? window->parameters.pointed_background_color : background_color, window->parameters.text_color, image + ((size_t)menu_icon_y * (size_t)w + (size_t)menu_icon_x));
	}

	if (window->parameters.text)
	{
		size_t display_string_offset;
		size_t display_string_length;
		if (window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU)
		{
			display_string_offset = sdui_get_string_line_offset(window->parameters.text, window->selected_item);
			display_string_length = sdui_string_line_size(window->parameters.text + display_string_offset);
		}
		else
		{
			display_string_offset = 0;
			display_string_length = window->text_length;
		}

		SDL_Rect string_rectangle;
		sdui_calculate_string_rectangle(gui->font, window->parameters.font_size, display_string_length, window->parameters.text + display_string_offset, &string_rectangle);
		if (window->parameters.style_flags & SDUI_CENTER_WINDOW_TEXT_HORIZONTALLY)
			window->text_x = ((window->w - string_rectangle.w) / 2) - string_rectangle.x;
		else
			window->text_x = window->parameters.border_thickness + 1;
		if (window->parameters.style_flags & SDUI_CENTER_WINDOW_TEXT_VERTICALLY)
			window->text_y = ((window->parameters.h - string_rectangle.h) / 2) - string_rectangle.y;
		else
			window->text_y = window->parameters.border_thickness + 1;

		//for (int y = text_offset_y + string_rectangle.y; y != text_offset_y + string_rectangle.y + string_rectangle.h; ++y)
		//	for (int x = text_offset_x + string_rectangle.x; x != text_offset_x + string_rectangle.x + string_rectangle.w; ++x)
		//		image[y * w + x] = 0xFF00FFFF;

		sdui_draw_string_with_select(w, h, image, window->text_x, window->text_y, gui->font, window->parameters.font_size, display_string_length, window->text_selection_offset, window->text_selection_length, window->parameters.text + display_string_offset, window->parameters.text_color, 0xFF00FFFF);

		//sdui_draw_string(w, h, image, text_offset_x, text_offset_y, gui->test_font.font, window->font_size, display_string_length, window->text + display_string_offset, window->text_color);

		if ((window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && window->window_is_selected)
			sdui_draw_string(w, h, image, window->text_x, window->parameters.h, gui->font, window->parameters.font_size, window->text_length, window->parameters.text, window->parameters.text_color);
	}

	int texture_update_error = SDL_UpdateTexture(window->texture_handle, 0, image, w * 4);
	assert(!texture_update_error);

	window->update_texture = 0;
	return 0;
}

int sdui_set_window_text(sdui_ui_t* gui, uint32_t window_id, const char* text)
{
	sdui_window_t* window = sdui_get_window_by_id(gui, window_id);
	if (!window)
		return ENOENT;

	size_t text_length = sdui_string_size(text);

	if (window->parameters.text)
	{
		if (text_length == window->text_length && !memcmp(window->parameters.text, text, text_length))
			return 0;

		if (window->text_allocation_length < text_length + 1)
		{
			size_t new_allocation_size = ((text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
			char* new_allocation = (char*)realloc(window->parameters.text, new_allocation_size);
			if (!new_allocation)
				return ENOMEM;
			window->text_allocation_length = new_allocation_size;
			window->parameters.text = new_allocation;
		}

		window->text_length = text_length;
		memcpy(window->parameters.text, text, text_length + 1);
		window->update_texture = 1;
	}
	else
	{
		size_t new_allocation_size = ((text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
		window->parameters.text = (char*)malloc(new_allocation_size);
		if (!window->parameters.text)
			return ENOMEM;
		window->text_allocation_length = new_allocation_size;

		window->text_length = text_length;
		memcpy(window->parameters.text, text, text_length + 1);
		window->update_texture = 1;
	}
	return 0;
}

int sdui_set_window_text_with_signed_number(sdui_ui_t* gui, uint32_t window_id, int32_t value)
{
	char text_buffer[16];
	text_buffer[sdui_print_signed(value, text_buffer)] = 0;
	return sdui_set_window_text(gui, window_id, text_buffer);
}

int sdui_set_window_text_with_unsigned_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[sdui_print_unsigned(value, text_buffer)] = 0;
	return sdui_set_window_text(gui, window_id, text_buffer);
}

int sdui_set_window_text_with_hexadecimal_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[sdui_print_hex(value, text_buffer)] = 0;
	return sdui_set_window_text(gui, window_id, text_buffer);
}

int sdui_append_window_text(sdui_ui_t* gui, uint32_t window_id, const char* text)
{
	sdui_window_t* window = sdui_get_window_by_id(gui, window_id);
	if (!window)
		return ENOENT;

	size_t text_length = strlen(text);

	if (!text_length)
		return 0;

	if (window->parameters.text)
	{
		if (window->text_allocation_length < window->text_length + text_length + 1)
		{
			size_t new_allocation_size = ((window->text_length + text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
			char* new_allocation = (char*)realloc(window->parameters.text, new_allocation_size);
			if (!new_allocation)
				return ENOMEM;
			window->text_allocation_length = new_allocation_size;
			window->parameters.text = new_allocation;
		}

		memcpy(window->parameters.text + window->text_length, text, text_length + 1);
		window->text_length += text_length;
		window->update_texture = 1;
	}
	else
	{
		size_t new_allocation_size = ((text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
		window->parameters.text = (char*)malloc(new_allocation_size);
		if (!window->parameters.text)
			return ENOMEM;
		window->text_allocation_length = new_allocation_size;

		window->text_length = text_length;
		memcpy(window->parameters.text, text, text_length + 1);
		window->update_texture = 1;
	}
	return 0;
}

int sdui_append_window_text_with_signed_number(sdui_ui_t* gui, uint32_t window_id, int32_t value)
{
	char text_buffer[16];
	text_buffer[sdui_print_signed(value, text_buffer)] = 0;
	return sdui_append_window_text(gui, window_id, text_buffer);
}

int sdui_append_window_text_with_unsigned_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[sdui_print_unsigned(value, text_buffer)] = 0;
	return sdui_append_window_text(gui, window_id, text_buffer);
}

int sdui_append_window_text_with_hexadecimal_number(sdui_ui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[sdui_print_hex(value, text_buffer)] = 0;
	return sdui_append_window_text(gui, window_id, text_buffer);
}

int sdui_create_gui(const char* title, sdui_ui_t** gui_address, size_t gui_window_count, const sdui_window_parameters_t* gui_window_table, uint32_t frame_per_second)
{
	const size_t temporal_buffer_initial_size = 0x10000;

	if (!gui_window_count)
		return EINVAL;

	size_t main_window_index = (size_t)~0;
	for (size_t i = 0; i != gui_window_count; ++i)
		if (!gui_window_table[i].parent_id)
		{
			if (main_window_index != (size_t)~0)
				return EINVAL;// multiple main windows
			main_window_index = i;
		}

	if (main_window_index == (size_t)~0)
		return EINVAL;// no main window

	for (size_t i = 0; i != gui_window_count; ++i)
	{
		if (!gui_window_table[i].id || (gui_window_table[i].id == gui_window_table[i].parent_id))
			return EINVAL;// invalid id

		int parent_exists = !gui_window_table[i].parent_id ? 1 : 0;
		for (size_t j = 0; j != gui_window_count; ++j)
		{
			if ((j != i) && (gui_window_table[j].id == gui_window_table[i].id))
				return EINVAL;// duplicate id
			if (gui_window_table[j].id == gui_window_table[i].parent_id)
				parent_exists = 1;
		}

		if (!parent_exists)
			return EINVAL;// invalid parent id
	}

	int main_window_h = gui_window_table[main_window_index].h;
	int main_window_w = gui_window_table[main_window_index].w;

	sdui_ui_t* gui = (sdui_ui_t*)malloc(sizeof(sdui_ui_t) + (gui_window_count * sizeof(sdui_window_t)));
	if (!gui)
		return ENOMEM;

	if (SDL_Init(SDL_INIT_VIDEO))
	{
		free(gui);
		return ENOSYS;
	}

	gui->window_handle = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, main_window_w, main_window_h, SDL_WINDOW_SHOWN);
	if (!gui->window_handle)
	{
		SDL_Quit();
		free(gui);
		return ENOSYS;
	}

	gui->renderer_handle = SDL_CreateRenderer(gui->window_handle, -1, SDL_RENDERER_ACCELERATED);
	if (!gui->renderer_handle)
	{
		SDL_DestroyWindow(gui->window_handle);
		SDL_Quit();
		free(gui);
		return ENOSYS;
	}

	SDL_SetRenderDrawBlendMode(gui->renderer_handle, SDL_BLENDMODE_BLEND);

	SDL_StartTextInput();

	void* temporal_buffer = malloc(temporal_buffer_initial_size);
	if (!temporal_buffer)
	{
		SDL_DestroyRenderer(gui->renderer_handle);
		SDL_DestroyWindow(gui->window_handle);
		SDL_Quit();
		free(gui);
		return ENOSYS;
	}

	uint32_t frame_ms_duration = 1000 / frame_per_second;

	gui->window_table = (sdui_window_t*)((uintptr_t)gui + sizeof(sdui_ui_t));
	gui->select_x = 0;
	gui->select_y = 0;
	gui->control_key_active = 0;
	gui->pointed_window = 0;
	gui->selected_window = 0;
	gui->event_window = gui->window_table + main_window_index;
	gui->main_window = gui->window_table + main_window_index;
	gui->frame_timestamp = SDL_GetTicks() - frame_ms_duration;
	gui->frame_duration = frame_ms_duration;
	gui->window_count = gui_window_count;
	gui->temporal_buffer_size = temporal_buffer_initial_size;
	gui->temporal_buffer = temporal_buffer;

	for (size_t i = 0; i != gui_window_count; ++i)
	{
		if (i != main_window_index)
		{
			gui->window_table[i].parameters.x = gui_window_table[i].x;
			gui->window_table[i].parameters.y = gui_window_table[i].y;
		}
		else
		{
			gui->window_table[i].parameters.x = 0;
			gui->window_table[i].parameters.y = 0;
		}
		gui->window_table[i].parameters.w = gui_window_table[i].w;
		gui->window_table[i].parameters.h = gui_window_table[i].h;
		gui->window_table[i].parameters.border_thickness = gui_window_table[i].border_thickness;
		gui->window_table[i].parameters.font_size = gui_window_table[i].font_size;
		gui->window_table[i].parameters.style_flags = gui_window_table[i].style_flags;
		gui->window_table[i].parameters.parent_id = gui_window_table[i].parent_id;
		gui->window_table[i].parameters.id = gui_window_table[i].id;
		gui->window_table[i].parameters.background_color = gui_window_table[i].background_color;
		gui->window_table[i].parameters.pointed_background_color = gui_window_table[i].pointed_background_color;
		gui->window_table[i].parameters.border_color = gui_window_table[i].border_color;
		gui->window_table[i].parameters.text_color = gui_window_table[i].text_color;

		if (gui_window_table[i].text)
		{
			size_t text_length = sdui_string_size(gui_window_table[i].text);
			gui->window_table[i].text_length = text_length;
			gui->window_table[i].text_allocation_length = ((text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
			gui->window_table[i].parameters.text = (char*)malloc(gui->window_table[i].text_allocation_length);

			assert(gui->window_table[i].parameters.text);

			memcpy(gui->window_table[i].parameters.text, gui_window_table[i].text, gui->window_table[i].text_length + 1);
		}
		else
		{
			gui->window_table[i].text_length = 0;
			gui->window_table[i].parameters.text = 0;
			gui->window_table[i].text_allocation_length = 0;
		}

		gui->window_table[i].text_selection_offset = 0;
		gui->window_table[i].text_selection_length = 0;
		gui->window_table[i].text_allocation_length = 0;
		gui->window_table[i].pointed_item = 0;
		gui->window_table[i].selected_item = 0;
		gui->window_table[i].text_x = 0;
		gui->window_table[i].text_y = 0;
		gui->window_table[i].z = 0;
		gui->window_table[i].base_x = 0;
		gui->window_table[i].base_y = 0;
		gui->window_table[i].w = 0;
		gui->window_table[i].h = 0;
		gui->window_table[i].window_is_under_cursor = 0;
		gui->window_table[i].window_is_selected = 0;
		gui->window_table[i].texture_handle = 0;
		gui->window_table[i].update_texture = 1;
	}

	for (int top_level_set = 1; top_level_set;)
	{
		top_level_set = 0;
		for (size_t i = 0; i != gui_window_count; ++i)
			for (size_t j = 0; j != gui_window_count; ++j)
				if ((gui->window_table[j].parameters.style_flags & SDUI_WINDOW_IS_TOP_LEVEL) && (gui->window_table[j].parameters.id == gui->window_table[i].parameters.parent_id) && !(gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_TOP_LEVEL))
				{
					gui->window_table[i].parameters.style_flags |= SDUI_WINDOW_IS_TOP_LEVEL;
					top_level_set = 1;
				}
	}

	int font_load_error = sdui_load_truetype_font("c:\\windows\\fonts\\arial.ttf", &gui->font);
	assert(!font_load_error);

	*gui_address = gui;
	return 0;
}

void sdui_close_gui(sdui_ui_t* gui)
{
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		if (gui->window_table[i].texture_handle)
			SDL_DestroyTexture(gui->window_table[i].texture_handle);
		if (gui->window_table[i].text_allocation_length)
			free(gui->window_table[i].parameters.text);
	}
	free(gui->temporal_buffer);
	SDL_StopTextInput();
	SDL_DestroyRenderer(gui->renderer_handle);
	SDL_DestroyWindow(gui->window_handle);
	SDL_Quit();
	free(gui);
}

sdui_window_t* sdui_select_window_text(sdui_ui_t* gui, int x, int y, int w, int h)
{
	sdui_window_t* window = sdui_get_window_by_coordinate(gui, x, y);
	if (window && (window->parameters.style_flags & SDUI_WINDOW_IS_SELECTABLE) && window->parameters.text)
	{
		assert(x >= (window->base_x + window->parameters.x) && y >= (window->base_y + window->parameters.y));

		int relative_x = x - (window->base_x + window->parameters.x);
		int relative_y = y - (window->base_y + window->parameters.y);
		SDL_Rect text_relative_select = { relative_x - window->text_x, relative_y - window->text_y, w, h };
		size_t offset;
		size_t length;
		sdui_select_from_string(gui->font, window->parameters.font_size, window->text_length, window->parameters.text, &text_relative_select, &offset, &length);

		if (window->text_selection_offset != offset || window->text_selection_length != length)
		{
			window->text_selection_offset = offset;
			window->text_selection_length = length;
			window->update_texture = 1;
		}
	}
	return window;
}

size_t sdui_get_window_item_by_coordinate(sdui_window_t* window, int x, int y)
{
	if ((window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && window->parameters.text &&
		(x >= window->base_x + window->parameters.x && x < window->base_x + window->parameters.x + window->w) &&
		(y >= window->base_y + window->parameters.y && y < window->base_y + window->parameters.y + window->h))
	{
		int menu_item_count = (int)sdui_get_string_line_count(window->parameters.text);
		if (menu_item_count)
		{
			int menu_h = window->h - window->parameters.h;
			int memu_y = y - (window->base_y + window->parameters.y + window->parameters.h);
			int menu_item_h = menu_h / menu_item_count;
			size_t selected_item = (memu_y < 0) ? window->selected_item : (size_t)(memu_y / menu_item_h);
			if (selected_item == menu_item_count)
				selected_item = menu_item_count - 1;
			return selected_item;
		}
	}
	return 0;
}

int sdui_poll_event(sdui_ui_t* gui)
{
	//sdui_window_t* debug_window_table[16];
	//for (size_t i = 0; i != 16; ++i)
	//	debug_window_table[i] = gui->window_table + i;

	if (SDL_PollEvent(&gui->event))
	{
		switch (gui->event.type)
		{
			case SDL_MOUSEMOTION:
			{
				sdui_window_t* pointed_window = sdui_get_window_by_coordinate(gui, gui->event.motion.x, gui->event.motion.y);
				if (gui->pointed_window != pointed_window)
				{
					if (gui->pointed_window && (gui->pointed_window->parameters.style_flags & SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR))
						gui->pointed_window->update_texture = 1;
					if (pointed_window && (pointed_window->parameters.style_flags & SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR))
						pointed_window->update_texture = 1;
				}
				gui->pointed_window = pointed_window;
				if (gui->pointed_window)
				{
					size_t pointed_item = sdui_get_window_item_by_coordinate(gui->pointed_window, gui->event.motion.x, gui->event.motion.y);
					if (pointed_item != gui->pointed_window->pointed_item)
					{
						gui->pointed_window->pointed_item = pointed_item;
						gui->pointed_window->update_texture = 1;
					}
				}
				gui->event_window = gui->pointed_window ? gui->pointed_window : gui->main_window;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				if (gui->event.button.button == SDL_BUTTON_RIGHT)
				{

				}
				else if (gui->event.button.button == SDL_BUTTON_LEFT)
				{
					gui->select_x = gui->event.button.x;
					gui->select_y = gui->event.button.y;
					gui->select_w = 0;
					gui->select_h = 0;

					sdui_window_t* select_window = sdui_get_window_by_coordinate(gui, gui->event.button.x, gui->event.button.y);
					if (gui->selected_window && gui->selected_window != select_window)
					{
						gui->selected_window->window_is_selected = 0;
						gui->selected_window->update_texture = 1;
					}
					if (select_window && (select_window->parameters.style_flags & SDUI_WINDOW_IS_SELECTABLE))
					{
						if ((gui->selected_window == select_window) && (select_window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU))
						{
							gui->selected_window->pointed_item = sdui_get_window_item_by_coordinate(select_window, gui->event.button.x, gui->event.button.y);
							gui->selected_window->selected_item = gui->selected_window->pointed_item;
							gui->selected_window->window_is_selected = 0;
							gui->selected_window->update_texture = 1;
							gui->selected_window = 0;
						}
						else
						{
							if (gui->selected_window != select_window)
							{
								gui->selected_window = select_window;
								gui->selected_window->window_is_selected = 1;
								gui->selected_window->update_texture = 1;
							}
							if ((select_window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && select_window->parameters.text)
							{
								size_t menu_item_count = sdui_get_string_line_count(select_window->parameters.text);
								if (menu_item_count)
								{
									size_t selected_item = sdui_get_window_item_by_coordinate(select_window, gui->event.button.x, gui->event.button.y);
									if (selected_item != select_window->selected_item)
									{
										select_window->selected_item = selected_item;
										select_window->window_is_selected = 1;
										select_window->update_texture = 1;
									}
								}
							}
						}
					}
					else
						gui->selected_window = 0;
				}

				gui->event_window = gui->selected_window ? gui->selected_window : gui->main_window;
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				if (gui->select_x < gui->event.button.x)
					gui->select_w = gui->event.button.x - gui->select_x;
				else
				{
					gui->select_w = gui->select_x - gui->event.button.x;
					gui->select_x = gui->event.button.x;
				}
				if (!gui->select_w)
					gui->select_w = 1;

				if (gui->select_y < gui->event.button.y)
					gui->select_h = gui->event.button.y - gui->select_y;
				else
				{
					gui->select_h = gui->select_y - gui->event.button.y;
					gui->select_y = gui->event.button.y;
				}
				if (!gui->select_h)
					gui->select_h = 1;


				sdui_select_window_text(gui, gui->select_x, gui->select_y, gui->select_w, gui->select_h);

				gui->event_window = gui->main_window;
				break;
			}
			case SDL_TEXTINPUT:
			{
				if (gui->selected_window && gui->selected_window->parameters.text)
				{
					size_t append_length = strlen(gui->event.text.text);

					if (gui->selected_window->text_length + append_length + 1 > gui->selected_window->text_allocation_length)
					{
						gui->selected_window->text_allocation_length = ((gui->selected_window->text_length + append_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
						gui->selected_window->parameters.text = (char*)realloc(gui->selected_window->parameters.text, gui->selected_window->text_allocation_length);
						assert(gui->selected_window->parameters.text);
					}

					memcpy(gui->selected_window->parameters.text + gui->selected_window->text_length, gui->event.text.text, append_length + 1);
					gui->selected_window->text_length += append_length;
					gui->selected_window->update_texture = 1;
				}

				gui->event_window = gui->selected_window ? gui->selected_window : gui->main_window;
				break;
			}
			case SDL_KEYDOWN:
			{
				if (gui->selected_window && gui->selected_window->parameters.text)
				{
					if (gui->event.key.keysym.sym == SDLK_BACKSPACE)
					{
						if (gui->selected_window->text_length)
						{
							gui->selected_window->text_length--;
							gui->selected_window->parameters.text[gui->selected_window->text_length] = 0;
							gui->selected_window->update_texture = 1;
						}
					}
					else if (gui->event.key.keysym.sym == SDLK_v && gui->control_key_active)
					{
						char* clipboard = SDL_GetClipboardText();
						if (clipboard)
						{
							size_t append_length = strlen(clipboard);

							if (gui->selected_window->text_length + append_length + 1 > gui->selected_window->text_allocation_length)
							{
								gui->selected_window->text_allocation_length = ((gui->selected_window->text_length + append_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
								gui->selected_window->parameters.text = (char*)realloc(gui->selected_window->parameters.text, gui->selected_window->text_allocation_length);
								assert(gui->selected_window->parameters.text);
							}

							memcpy(gui->selected_window->parameters.text + gui->selected_window->text_length, clipboard, append_length + 1);
							gui->selected_window->text_length += append_length;
							gui->selected_window->update_texture = 1;

							SDL_free(clipboard);
						}
					}
					else if (gui->event.key.keysym.sym == SDLK_c && gui->control_key_active)
					{
						SDL_SetClipboardText(gui->selected_window->parameters.text);
					}
					else if (gui->event.key.keysym.sym == SDLK_x && gui->control_key_active)
					{
						gui->selected_window->text_length = 0;
						gui->selected_window->parameters.text[0] = 0;
						gui->selected_window->update_texture = 1;
					}
				}

				if (gui->event.key.keysym.sym == SDLK_LCTRL)
					gui->control_key_active = 1;

				gui->event_window = gui->selected_window ? gui->selected_window : gui->main_window;
				break;
			}
			case SDL_KEYUP:
			{
				if (gui->event.key.keysym.sym == SDLK_LCTRL)
					gui->control_key_active = 0;

				gui->event_window = gui->selected_window ? gui->selected_window : gui->main_window;
				break;
			}
			default:
			{
				gui->event_window = gui->main_window;
				break;
			}
		}

		return 1;
	}
	else
		return 0;
}

int sdui_draw_window(sdui_ui_t* gui, sdui_window_t* window)
{
	//SDL_Rect rectangle = { window->paint_data.base_x + window->x, window->paint_data.base_y + window->y, window->w, window->h };
	//SDL_SetRenderDrawColor(gui->renderer_handle, (window->background_color >> 0) & 0xFF, (window->background_color >> 8) & 0xFF, (window->background_color >> 16) & 0xFF, 0xFF);
	//return SDL_RenderFillRect(gui->renderer_handle, &rectangle);

	if (window->texture_handle)
	{
		SDL_Rect rectangle = { window->base_x + window->parameters.x, window->base_y + window->parameters.y, window->w, window->h };
		//int set_color_error = SDL_SetRenderDrawColor(gui->renderer_handle, 0xFF, 0xFF, 0xFF, 0xFF);
		//assert(!set_color_error);
		int copy_error = SDL_RenderCopy(gui->renderer_handle, window->texture_handle, 0, &rectangle);
		assert(!copy_error);
	}

	return 0;
}

void sdui_update_windows_z_coordinas(sdui_ui_t* gui)
{
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		int z = (gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_TOP_LEVEL) ? 0x1000 : 0;
		uint32_t parent_id = gui->window_table[i].parameters.parent_id;
		size_t parent_index = (size_t)~0;
		do
		{
			parent_index = (size_t)~0;
			for (size_t j = 0; parent_id && j != gui->window_count; ++j)
				if (gui->window_table[j].parameters.id == parent_id)
				{
					parent_index = j;
					parent_id = gui->window_table[j].parameters.parent_id;
					++z;
				}
		} while (parent_index != (size_t)~0);
		gui->window_table[i].z = z;
	}
}

void sdui_sort_windows_by_z_coordinate(sdui_ui_t* gui)
{
	// this sort is slow but, it doas not matter coz gui->window_count is small and this table is probably sorted anyway

	if (gui->window_count)
	{
		sdui_window_t tmp;
		for (size_t i = 1; i != gui->window_count;)
			if (gui->window_table[i - 1].z > gui->window_table[i].z)
			{
				memcpy(&tmp, gui->window_table + i - 1, sizeof(sdui_window_t));
				memcpy(gui->window_table + i - 1, gui->window_table + i, sizeof(sdui_window_t));
				memcpy(gui->window_table + i, &tmp, sizeof(sdui_window_t));
				if (i)
					--i;
			}
			else
				++i;
	}
}

void sdui_update_windows_parent_x_and_y_coordinas(sdui_ui_t* gui, int base_x, int base_y)
{
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		size_t parent_index = (size_t)~0;
		for (size_t j = 0; parent_index == (size_t)~0 && j != i; ++j)
			if (gui->window_table[j].parameters.id == gui->window_table[i].parameters.parent_id)
				parent_index = j;
		if (parent_index != (size_t)~0)
		{
			gui->window_table[i].base_x = base_x + gui->window_table[parent_index].base_x + gui->window_table[parent_index].parameters.x;
			gui->window_table[i].base_y = base_y + gui->window_table[parent_index].base_y + gui->window_table[parent_index].parameters.y;
		}
		else
		{
			gui->window_table[i].base_x = base_x;
			gui->window_table[i].base_y = base_y;
		}
	}
}

int sdui_draw_gui(sdui_ui_t* gui)
{
	sdui_update_windows_z_coordinas(gui);
	sdui_sort_windows_by_z_coordinate(gui);
	sdui_update_windows_parent_x_and_y_coordinas(gui, 0, 0);
	gui->main_window = sdui_get_main_window(gui);

	for (size_t i = 0; i != gui->window_count; ++i)
		if (gui->window_table[i].update_texture)
			sdui_create_window_texture(gui, gui->window_table + i);

	//sdui_window_t* debug_window_table[96];
	//for (size_t i = 0; i != 96; ++i)
	//	debug_window_table[i] = gui->window_table + i;



	SDL_SetRenderDrawColor(gui->renderer_handle, 0, 0, 0, 0xFF);
	SDL_RenderClear(gui->renderer_handle);
	for (size_t i = 0; i != gui->window_count; ++i)
		if (gui->window_table + 1 != gui->selected_window)
		{
			int error = sdui_draw_window(gui, gui->window_table + i);
			if (error)
				return error;
		}
	if (gui->selected_window)
		sdui_draw_window(gui, gui->selected_window);
	SDL_RenderPresent(gui->renderer_handle);

	return 0;
}