/*
	sdui.h - v0.4.0 (2020-02-02) - public domain
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

static int sdui_internal_is_event_for_window(const SDL_Event* event, uint32_t sdl2_window_id)
{
	switch (event->type)
	{
		case SDL_WINDOWEVENT:
			return event->window.windowID == sdl2_window_id;
		case SDL_KEYDOWN:
			return event->key.windowID == sdl2_window_id;
		case SDL_KEYUP:
			return event->key.windowID == sdl2_window_id;
		case SDL_TEXTEDITING:
			return event->edit.windowID == sdl2_window_id;
		case SDL_TEXTINPUT:
			return event->text.windowID == sdl2_window_id;
		case SDL_MOUSEMOTION:
			return event->motion.windowID == sdl2_window_id;
		case SDL_MOUSEBUTTONDOWN:
			return event->button.windowID == sdl2_window_id;
		case SDL_MOUSEBUTTONUP:
			return event->button.windowID == sdl2_window_id;
		case SDL_MOUSEWHEEL:
			return event->wheel.windowID == sdl2_window_id;
		case SDL_DROPFILE:
			return event->drop.windowID == sdl2_window_id;
		case SDL_DROPTEXT:
			return event->drop.windowID == sdl2_window_id;
		case SDL_DROPBEGIN:
			return event->drop.windowID == sdl2_window_id;
		case SDL_DROPCOMPLETE:
			return event->drop.windowID == sdl2_window_id;
		default:
			return 1;
	}
}

static int sdui_internal_poll_event(SDL_Event* event, uint32_t sdl2_window_id)
{
	static volatile SDL_mutex* event_queue_mutex;
	static volatile int queue_event_count;
	static volatile SDL_Event queue_event_table[0x400];

	SDL_mutex* mutex = (SDL_mutex*)event_queue_mutex;
	if (!mutex)
	{
		mutex = SDL_CreateMutex();
		if (!mutex)
			return 0;

		if (SDL_AtomicCASPtr((void**)&event_queue_mutex, 0, (void*)mutex) == SDL_FALSE)
		{
			SDL_DestroyMutex(mutex);
			mutex = (SDL_mutex*)event_queue_mutex;
		}
	}

	SDL_LockMutex(mutex);

	int event_count = queue_event_count;
	for (int i = 0; i != event_count; ++i)
		if (sdui_internal_is_event_for_window((const SDL_Event*)(queue_event_table + 1), sdl2_window_id))
		{
			memcpy(event, (SDL_Event*)(queue_event_table + i), sizeof(SDL_Event));

			if (i != event_count - 1)
				memcpy((SDL_Event*)(queue_event_table + i), (SDL_Event*)(queue_event_table + event_count - 1), sizeof(SDL_Event));

			queue_event_count = event_count - 1;
			SDL_UnlockMutex(mutex);
			return 1;
		}

	while (SDL_PollEvent(event))
	{
		if (sdui_internal_is_event_for_window(event, sdl2_window_id))
		{
			queue_event_count = event_count;
			SDL_UnlockMutex(mutex);
			return 1;
		}

		if (event_count != (sizeof(queue_event_table) / sizeof(SDL_Event)))
			memcpy((SDL_Event*)(queue_event_table + event_count++), event, sizeof(SDL_Event));
	}

	queue_event_count = event_count;
	SDL_UnlockMutex(mutex);
	return 0;
}

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

sdui_window_t* sdui_get_window_address(sdui_ui_t* gui, uint32_t window_id)
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
	if (!(window->parameters.style_flags & SDUI_WINDOW_HAS_TEXT) && !(window->parameters.style_flags & SDUI_WINDOW_HAS_BORDER) && !(window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU))
	{
		window->state_flags |= SDIU_WINDOW_STATE_SINGLE_COLOR;
		if (window->texture_handle)
		{
			SDL_DestroyTexture(window->texture_handle);
			window->texture_handle = 0;
		}
		window->w = window->parameters.w;
		window->h = window->parameters.h;
		window->state_flags &= ~SDIU_WINDOW_STATE_UPDATE_TEXTURE;
		return 0;
	}
	else
		window->state_flags &= ~SDIU_WINDOW_STATE_SINGLE_COLOR;

	int w = window->parameters.w;
	int h = window->parameters.h;

	if (!w || !h)
	{
		if (window->texture_handle)
		{
			SDL_DestroyTexture(window->texture_handle);
			window->texture_handle = 0;
		}
		window->w = 0;
		window->h = 0;
		window->state_flags &= ~SDIU_WINDOW_STATE_UPDATE_TEXTURE;
		return 0;
	}

	int border_h = (window->parameters.border_thickness < (h / 2)) ? window->parameters.border_thickness : (h / 2);
	int border_w = (window->parameters.border_thickness < (w / 2)) ? window->parameters.border_thickness : (w / 2);
	size_t dropdown_menu_item_count = 0;

	if ((window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && window->text && (window->state_flags & SDIU_WINDOW_STATE_SELECTED))
	{
		dropdown_menu_item_count = sdui_calculate_menu_item_count(window->text);
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

	uint32_t background_color = ((window->parameters.style_flags & SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR) && !((window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && (window->state_flags & SDIU_WINDOW_STATE_SELECTED)) &&
		(window->state_flags & SDIU_WINDOW_STATE_UNDER_CURSOR)) ? window->parameters.pointed_background_color : window->parameters.background_color;

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
		if (window->state_flags & SDIU_WINDOW_STATE_SELECTED)
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

		//sdui_draw_minus_icon_bitmap(menu_icon_size, menu_icon_size, w * 4, (window->state_flags & SDIU_WINDOW_STATE_SELECTED) ? window->parameters.pointed_background_color : background_color, window->parameters.text_color, image + ((size_t)menu_icon_y * (size_t)w + (size_t)menu_icon_x));
		//sdui_draw_plus_icon_bitmap(menu_icon_size, menu_icon_size, w * 4, (window->state_flags & SDIU_WINDOW_STATE_SELECTED) ? window->parameters.pointed_background_color : background_color, window->parameters.text_color, image + ((size_t)menu_icon_y * (size_t)w + (size_t)menu_icon_x));
		sdui_draw_dropdown_icon_bitmap(menu_icon_size, menu_icon_size, w * 4, (window->state_flags & SDIU_WINDOW_STATE_SELECTED) ? window->parameters.pointed_background_color : background_color, window->parameters.text_color, image + ((size_t)menu_icon_y * (size_t)w + (size_t)menu_icon_x));
	}

	if (window->parameters.style_flags & SDUI_WINDOW_HAS_TEXT)
	{
		size_t display_string_offset;
		size_t display_string_length;
		if (window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU)
		{
			display_string_offset = sdui_get_unicode_line_offset(window->text, window->selected_item);
			display_string_length = sdui_unicode_line_size(window->text + display_string_offset);
		}
		else
		{
			display_string_offset = 0;
			display_string_length = window->text_length;
		}

		SDL_Rect string_rectangle;
		sdui_calculate_string_rectangle(gui->font, (float)window->parameters.font_size, display_string_length, window->text + display_string_offset, &string_rectangle);
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

		if (window->state_flags & SDIU_WINDOW_STATE_SELECTED)
			sdui_draw_string_with_select(w, h, image, (float)window->text_x, (float)window->text_y, gui->font, (float)window->parameters.font_size, display_string_length, window->text_selection_offset, window->text_selection_length, window->text + display_string_offset, window->parameters.text_color, sdui_inverse_color(window->parameters.text_color));
		else
			sdui_draw_string(w, h, image, (float)window->text_x, (float)window->text_y, gui->font, (float)window->parameters.font_size, display_string_length, window->text + display_string_offset, window->parameters.text_color);

		//sdui_draw_string(w, h, image, text_offset_x, text_offset_y, gui->test_font.font, window->font_size, display_string_length, window->text + display_string_offset, window->text_color);

		if ((window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU) && (window->state_flags & SDIU_WINDOW_STATE_SELECTED))
			sdui_draw_string(w, h, image, (float)window->text_x, (float)window->parameters.h, gui->font, (float)window->parameters.font_size, window->text_length, window->text, window->parameters.text_color);
	}

	assert(window->texture_handle);
	int texture_update_error = SDL_UpdateTexture(window->texture_handle, 0, image, w * 4);
	assert(!texture_update_error);

	window->state_flags &= ~SDIU_WINDOW_STATE_UPDATE_TEXTURE;
	return 0;
}

int sdui_set_window_unicode_text_by_address(sdui_ui_t* gui, sdui_window_t* window, const uint32_t* text)
{
	size_t text_length = sdui_unicode_length(text);

	if (window->text_allocation_length)
	{
		if (window->text_allocation_length < text_length + 1)
		{
			window->text_allocation_length = ((text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
			window->text = (uint32_t*)realloc(window->text, window->text_allocation_length * sizeof(uint32_t));
			assert(window->text);
		}
	}
	else
	{
		window->text_allocation_length = ((text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
		window->text = (uint32_t*)malloc(window->text_allocation_length * sizeof(uint32_t));
		assert(window->text);
	}

	window->text_length = text_length;
	memcpy(window->text, text, (text_length + 1) * sizeof(uint32_t));
	window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
	return 0;
}

int sdui_set_window_unicode_text(sdui_ui_t* gui, uint32_t window_id, const uint32_t* text)
{
	return sdui_set_window_unicode_text_by_address(gui, sdui_get_window_address(gui, window_id), text);
}

int sdui_set_window_text(sdui_ui_t* gui, uint32_t window_id, const char* text)
{
	size_t text_length = strlen(text);

	size_t unicode_length;
	int error = sdui_utf8_to_unicode(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &unicode_length, (uint32_t*)gui->temporal_buffer);
	if (error == ENOBUFS)
	{
		if (sdui_grow_gui_temporal_buffer(gui, (unicode_length + 1) * sizeof(uint32_t)))
			return ENOMEM;
		error = sdui_utf8_to_unicode(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &unicode_length, (uint32_t*)gui->temporal_buffer);
		if (error)
			return error;
	}
	else if (error)
		return error;

	*((uint32_t*)gui->temporal_buffer + unicode_length) = 0;
	return sdui_set_window_unicode_text_by_address(gui, sdui_get_window_address(gui, window_id), (const uint32_t*)gui->temporal_buffer);
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

int sdui_append_window_unicode_text_by_address(sdui_ui_t* gui, sdui_window_t* window, const uint32_t* text)
{
	size_t text_length = sdui_unicode_length(text);

	if (window->text_allocation_length)
	{
		if (window->text_allocation_length < window->text_length + text_length + 1)
		{
			window->text_allocation_length = ((window->text_length + text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
			window->text = (uint32_t*)realloc(window->text, window->text_allocation_length * sizeof(uint32_t));
			assert(window->text);
		}
	}
	else
	{
		window->text_allocation_length = ((text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
		window->text = (uint32_t*)malloc(window->text, window->text_allocation_length * sizeof(uint32_t));
		assert(window->text);
	}

	memcpy(window->text + window->text_length, text, (text_length + 1) * sizeof(uint32_t));
	window->text_length += text_length;
	window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
	return 0;
}

int sdui_append_window_unicode_text(sdui_ui_t* gui, uint32_t window_id, const uint32_t* text)
{
	return sdui_append_window_unicode_text_by_address(gui, sdui_get_window_address(gui, window_id), text);
}

int sdui_append_window_text(sdui_ui_t* gui, uint32_t window_id, const char* text)
{
	size_t text_length = strlen(text);

	size_t unicode_length;
	int error = sdui_utf8_to_unicode(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &unicode_length, (uint32_t*)gui->temporal_buffer);
	if (error == ENOBUFS)
	{
		if (sdui_grow_gui_temporal_buffer(gui, (unicode_length + 1) * sizeof(uint32_t)))
			return ENOMEM;
		error = sdui_utf8_to_unicode(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &unicode_length, (uint32_t*)gui->temporal_buffer);
		if (error)
			return error;
	}
	else if (error)
		return error;

	*((uint32_t*)gui->temporal_buffer + unicode_length) = 0;
	return sdui_append_window_unicode_text_by_address(gui, sdui_get_window_address(gui, window_id), (const uint32_t*)gui->temporal_buffer);
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

	gui->window_handle = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, main_window_w, main_window_h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!gui->window_handle)
	{
		SDL_Quit();
		free(gui);
		return ENOSYS;
	}

	gui->sdl2_window_id = SDL_GetWindowID(gui->window_handle);

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
	gui->select.x = 0;
	gui->select.y = 0;
	gui->select.w = 0;
	gui->select.h = 0;
	gui->control_key_active = 0;
	gui->pointed_window_id = 0;
	gui->selected_window_id = 0;
	gui->event_window_id = gui_window_table[main_window_index].id;
	gui->main_window_id = gui_window_table[main_window_index].id;
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
		gui->window_table[i].parameters.create_text = gui_window_table[i].create_text;
		gui->window_table[i].parameters.user_data = gui_window_table[i].user_data;
		gui->window_table[i].parameters.callback = gui_window_table[i].callback;
		
		if (gui->window_table[i].parameters.style_flags & SDUI_WINDOW_HAS_TEXT)
		{
			if (gui->window_table[i].parameters.create_text)
			{
				size_t text_length = sdui_string_size(gui->window_table[i].parameters.create_text);
				size_t unicode_length;
				int unicode_error = sdui_utf8_to_unicode(text_length, gui->window_table[i].parameters.create_text, 0, &unicode_length, 0);
				if (unicode_error && unicode_error != ENOBUFS)
				{
					for (int j = i; j--;)
						if (gui->window_table[j].text)
							free(gui->window_table[j].text);
					free(gui->temporal_buffer);
					SDL_DestroyRenderer(gui->renderer_handle);
					SDL_DestroyWindow(gui->window_handle);
					SDL_Quit();
					free(gui);
					return unicode_error;
				}

				gui->window_table[i].text_length = unicode_length;
				gui->window_table[i].text_allocation_length = ((unicode_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
				gui->window_table[i].text = (uint32_t*)malloc(gui->window_table[i].text_allocation_length * sizeof(uint32_t));
				assert(gui->window_table[i].text);
				sdui_utf8_to_unicode(text_length, gui->window_table[i].parameters.create_text, unicode_length, &unicode_length, gui->window_table[i].text);
				gui->window_table[i].text[unicode_length] = 0;
			}
			else
			{
				gui->window_table[i].text_length = 0;
				gui->window_table[i].text_allocation_length = SDUI_TEXT_ALLOCATION_GRANULARITY;
				gui->window_table[i].text = (uint32_t*)malloc(gui->window_table[i].text_allocation_length * sizeof(uint32_t));
				assert(gui->window_table[i].text);
				*gui->window_table[i].text = 0;
			}
		}
		else
		{
			gui->window_table[i].text_length = 0;
			gui->window_table[i].text = 0;
			gui->window_table[i].text_allocation_length = 0;
		}

		gui->window_table[i].text_selection_offset = (size_t)~0;
		gui->window_table[i].text_selection_length = 0;
		gui->window_table[i].pointed_item = 0;
		gui->window_table[i].selected_item = 0;
		gui->window_table[i].text_x = 0;
		gui->window_table[i].text_y = 0;
		gui->window_table[i].z = -1;
		gui->window_table[i].base_x = 0;
		gui->window_table[i].base_y = 0;
		gui->window_table[i].w = 0;
		gui->window_table[i].h = 0;
		gui->window_table[i].state_flags = SDIU_WINDOW_STATE_UPDATE_TEXTURE;
		gui->window_table[i].texture_handle = 0;
	}

	for (int top_level_set = 1; top_level_set;)
	{
		top_level_set = 0;
		for (size_t i = 0; i != gui_window_count; ++i)
			for (size_t j = 0; j != gui_window_count; ++j)
				if ((gui->window_table[j].parameters.style_flags & SDUI_WINDOW_IS_POPUP) && (gui->window_table[j].parameters.id == gui->window_table[i].parameters.parent_id) && !(gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_POPUP))
				{
					gui->window_table[i].parameters.style_flags |= SDUI_WINDOW_IS_POPUP;
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
			free(gui->window_table[i].text);
	}
	free(gui->temporal_buffer);
	SDL_StopTextInput();
	SDL_DestroyRenderer(gui->renderer_handle);
	SDL_DestroyWindow(gui->window_handle);
	SDL_Quit();
	free(gui);
}

int sdui_replace_window_text_by_address(sdui_ui_t* gui, sdui_window_t* window, size_t offset, size_t old_text_length, const uint32_t* new_text)
{
	if (offset > window->text_length)
		offset = window->text_length;

	if (offset + old_text_length > window->text_length)
		old_text_length = window->text_length - offset;

	size_t new_text_length = sdui_unicode_length(new_text);

	if (!window->text_allocation_length)
	{
		assert(!window->text_length);

		window->text_allocation_length = ((new_text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
		window->text = (uint32_t*)malloc(window->text_allocation_length * sizeof(uint32_t));
		assert(window->text);
		memcpy(window->text, new_text, (new_text_length + 1) * sizeof(uint32_t));
		window->state_flags = SDIU_WINDOW_STATE_UPDATE_TEXTURE;
		return 0;
	}

	size_t end_text_length = window->text_length - (offset + old_text_length);

	if (offset + new_text_length + end_text_length + 1 > window->text_allocation_length)
	{
		window->text_allocation_length = ((new_text_length + 1) + (SDUI_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(SDUI_TEXT_ALLOCATION_GRANULARITY - 1);
		window->text = (uint32_t*)malloc(window->text_allocation_length * sizeof(uint32_t));
		assert(window->text);
	}

	memmove(window->text + offset + new_text_length, window->text + offset + old_text_length, end_text_length * sizeof(uint32_t));
	memcpy(window->text + offset, new_text, new_text_length * sizeof(uint32_t));
	window->text_length = offset + new_text_length + end_text_length;
	window->text[window->text_length] = 0;
	window->state_flags = SDIU_WINDOW_STATE_UPDATE_TEXTURE;
	return 0;
}

int sdui_get_window_text_by_address(sdui_ui_t* gui, sdui_window_t* window, size_t buffer_size, char* buffer, size_t* text_length)
{
	if (window->text)
		return sdui_unicode_to_utf8(window->text_length + 1, window->text, buffer_size, text_length, buffer);
	else
		return ENOENT;
}

int sdui_get_window_text(sdui_ui_t* gui, uint32_t window_id, size_t buffer_size, char* buffer, size_t* text_length)
{
	return sdui_get_window_text_by_address(gui, sdui_get_window_address(gui, window_id), buffer_size, buffer, text_length);
}

size_t sdui_calculate_menu_item_count(const uint32_t* menu_string)
{
	size_t line_count = 1;
	while (*menu_string)
	{
		if (*menu_string == (uint32_t)'\n' || *menu_string == (uint32_t)'\r')
			++line_count;
		++menu_string;
	}
	return line_count;
}

sdui_window_t* sdui_select_window_text(sdui_ui_t* gui, int x, int y, int w, int h)
{
	sdui_window_t* window = sdui_get_window_by_coordinate(gui, x, y);
	if (window && (window->parameters.style_flags & (SDUI_WINDOW_IS_SELECTABLE | SDUI_WINDOW_HAS_TEXT)) == (SDUI_WINDOW_IS_SELECTABLE | SDUI_WINDOW_HAS_TEXT))
	{
		assert(x >= (window->base_x + window->parameters.x) && y >= (window->base_y + window->parameters.y));

		int relative_x = x - (window->base_x + window->parameters.x);
		int relative_y = y - (window->base_y + window->parameters.y);
		SDL_Rect text_relative_select = { relative_x - window->text_x, relative_y - window->text_y, w, h };
		size_t offset;
		size_t length;
		sdui_select_from_string(gui->font, (float)window->parameters.font_size, window->text_length, window->text, &text_relative_select, &offset, &length);

		if (window->text_selection_offset != offset || window->text_selection_length != length)
		{
			window->text_selection_offset = offset;
			window->text_selection_length = length;
			window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
		}
	}
	return window;
}

size_t sdui_get_window_item_by_coordinate(sdui_window_t* window, int x, int y)
{
	if ((window->parameters.style_flags & (SDUI_WINDOW_IS_DROPDOWN_MENU | SDUI_WINDOW_HAS_TEXT)) == (SDUI_WINDOW_IS_DROPDOWN_MENU | SDUI_WINDOW_HAS_TEXT) &&
		(x >= window->base_x + window->parameters.x && x < window->base_x + window->parameters.x + window->w) &&
		(y >= window->base_y + window->parameters.y && y < window->base_y + window->parameters.y + window->h))
	{
		int menu_item_count = (int)sdui_calculate_menu_item_count(window->text);
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

int sdui_encode_utf8(sdui_ui_t* gui, const uint32_t* text, size_t* length)
{
	size_t text_length = sdui_unicode_length(text);
	size_t utf8_length;

	int text_encoder_error = sdui_unicode_to_utf8(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &utf8_length, (uint32_t*)gui->temporal_buffer);
	if (text_encoder_error == ENOBUFS)
	{
		if (sdui_grow_gui_temporal_buffer(gui, (utf8_length + 1) * sizeof(char)))
			return ENOMEM;
		text_encoder_error = sdui_utf8_to_unicode(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &utf8_length, (uint32_t*)gui->temporal_buffer);
		if (text_encoder_error)
			return text_encoder_error;
	}
	else if (text_encoder_error)
		return text_encoder_error;

	*((char*)gui->temporal_buffer + utf8_length) = 0;
	*length = utf8_length;
	return 0;
}

int sdui_decode_utf8(sdui_ui_t* gui, const char* text, size_t* length)
{
	size_t text_length = strlen(text);
	size_t unicode_length;

	int text_decoder_error = sdui_utf8_to_unicode(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &unicode_length, (uint32_t*)gui->temporal_buffer);
	if (text_decoder_error == ENOBUFS)
	{
		if (sdui_grow_gui_temporal_buffer(gui, (unicode_length + 1) * sizeof(uint32_t)))
			return ENOMEM;
		text_decoder_error = sdui_utf8_to_unicode(text_length, text, gui->temporal_buffer_size / sizeof(uint32_t) - 1, &unicode_length, (uint32_t*)gui->temporal_buffer);
		if (text_decoder_error)
			return text_decoder_error;
	}
	else if (text_decoder_error)
		return text_decoder_error;

	*((uint32_t*)gui->temporal_buffer + unicode_length) = 0;
	*length = unicode_length;
	return 0;
}

int sdui_poll_event(sdui_ui_t* gui)
{
	//sdui_window_t* debug_window_table[16];
	//for (size_t i = 0; i != 16; ++i)
	//	debug_window_table[i] = gui->window_table + i;

	if (sdui_internal_poll_event(&gui->event, gui->sdl2_window_id))
	{
		switch (gui->event.type)
		{
			case SDL_MOUSEMOTION:
			{
				sdui_window_t* old_pointed_window = gui->pointed_window_id ? sdui_get_window_address(gui, gui->pointed_window_id) : 0;
				sdui_window_t* new_pointed_window = sdui_get_window_by_coordinate(gui, gui->event.motion.x, gui->event.motion.y);

				if (new_pointed_window != old_pointed_window)
				{
					if (old_pointed_window)
					{
						old_pointed_window->state_flags &= ~SDIU_WINDOW_STATE_UNDER_CURSOR;
						if (old_pointed_window->parameters.style_flags & SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR)
							old_pointed_window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
					}
					if (new_pointed_window)
					{
						new_pointed_window->state_flags |= SDIU_WINDOW_STATE_UNDER_CURSOR;
						if (new_pointed_window->parameters.style_flags & SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR)
							new_pointed_window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
					}
				}

				if (new_pointed_window)
				{
					gui->pointed_window_id = new_pointed_window->parameters.id;
					size_t pointed_item = sdui_get_window_item_by_coordinate(new_pointed_window, gui->event.motion.x, gui->event.motion.y);
					if (pointed_item != new_pointed_window->pointed_item)
					{
						new_pointed_window->pointed_item = pointed_item;
						new_pointed_window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
					}
				}
				else
					gui->pointed_window_id = 0;

				gui->event_window_id = gui->pointed_window_id ? gui->pointed_window_id : gui->main_window_id;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				if (gui->event.button.button == SDL_BUTTON_RIGHT)
				{

				}
				else if (gui->event.button.button == SDL_BUTTON_LEFT)
				{
					gui->select.x = gui->event.button.x;
					gui->select.y = gui->event.button.y;
					gui->select.w = 0;
					gui->select.h = 0;

					sdui_window_t* old_select_window = gui->selected_window_id ? sdui_get_window_address(gui, gui->selected_window_id) : 0;
					sdui_window_t* new_select_window = sdui_get_window_by_coordinate(gui, gui->event.button.x, gui->event.button.y);
					if (new_select_window && !(new_select_window->parameters.style_flags & SDUI_WINDOW_IS_SELECTABLE))
						new_select_window = 0;

					if (new_select_window == old_select_window)
					{
						if (new_select_window && (new_select_window->parameters.style_flags & SDUI_WINDOW_IS_DROPDOWN_MENU))
						{
							new_select_window->pointed_item = sdui_get_window_item_by_coordinate(new_select_window, gui->event.button.x, gui->event.button.y);
							new_select_window->selected_item = new_select_window->pointed_item;
							new_select_window->state_flags &= ~SDIU_WINDOW_STATE_SELECTED;
							new_select_window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
							new_select_window = 0;
						}
					}
					else
					{
						if (old_select_window)
						{
							old_select_window->state_flags &= ~SDIU_WINDOW_STATE_SELECTED;
							old_select_window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
						}

						if (new_select_window)
						{
							new_select_window->state_flags |= (SDIU_WINDOW_STATE_UPDATE_TEXTURE | SDIU_WINDOW_STATE_SELECTED);
						}
					}

					gui->selected_window_id = new_select_window ? new_select_window->parameters.id : 0;
				}

				gui->event_window_id = gui->selected_window_id ? gui->selected_window_id : gui->main_window_id;
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				if (gui->select.x < gui->event.button.x)
					gui->select.w = gui->event.button.x - gui->select.x;
				else
				{
					gui->select.w = gui->select.x - gui->event.button.x;
					gui->select.x = gui->event.button.x;
				}
				if (!gui->select.w)
					gui->select.w = 1;

				if (gui->select.y < gui->event.button.y)
					gui->select.h = gui->event.button.y - gui->select.y;
				else
				{
					gui->select.h = gui->select.y - gui->event.button.y;
					gui->select.y = gui->event.button.y;
				}
				if (!gui->select.h)
					gui->select.h = 1;


				sdui_select_window_text(gui, gui->select.x, gui->select.y, gui->select.w, gui->select.h);

				gui->event_window_id = gui->selected_window_id ? gui->selected_window_id : gui->main_window_id;
				break;
			}
			case SDL_TEXTINPUT:
			{
				if (gui->selected_window_id)
				{
					sdui_window_t* text_window = sdui_get_window_address(gui, gui->selected_window_id);
					if ((text_window->parameters.style_flags & (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_EDITABLE)) == (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_EDITABLE))
					{
						size_t text_length;
						if (!sdui_decode_utf8(gui, gui->event.text.text, &text_length))
						{
							sdui_replace_window_text_by_address(gui, text_window, text_window->text_selection_offset, text_window->text_selection_length, (const uint32_t*)gui->temporal_buffer);
							if (text_window->text_selection_offset != (size_t)~0)
								text_window->text_selection_offset += text_length;
							text_window->text_selection_length = 0;
						}
					}
				}

				gui->event_window_id = gui->selected_window_id ? gui->selected_window_id : gui->main_window_id;
				break;
			}
			case SDL_KEYDOWN:
			{
				if (gui->selected_window_id)
				{
					sdui_window_t* text_window = sdui_get_window_address(gui, gui->selected_window_id);
					if ((text_window->parameters.style_flags & (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_EDITABLE)) == (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_EDITABLE))
					{
						if (gui->event.key.keysym.sym == SDLK_BACKSPACE && text_window->text_length)
						{
							const uint32_t null_terminator = 0;
							if (text_window->text_selection_length)
							{
								sdui_replace_window_text_by_address(gui, text_window, text_window->text_selection_offset, text_window->text_selection_length, &null_terminator);
								text_window->text_selection_length = 0;
							}
							else
							{
								if (text_window->text_selection_offset >= text_window->text_length)
									sdui_replace_window_text_by_address(gui, text_window, text_window->text_length - 1, 1, &null_terminator);
								else
								{
									size_t new_selection_offset = text_window->text_selection_offset ? text_window->text_selection_offset - 1 : 0;
									sdui_replace_window_text_by_address(gui, text_window, new_selection_offset, 1, &null_terminator);
									text_window->text_selection_offset = new_selection_offset;
								}
							}
						}
						else if (gui->event.key.keysym.sym == SDLK_RETURN)
						{
							const uint32_t return_text[2] = { (uint32_t)'\n', 0 };
							if (text_window->text_selection_length)
							{
								sdui_replace_window_text_by_address(gui, text_window, text_window->text_selection_offset, text_window->text_selection_length, return_text);
								text_window->text_selection_offset++;
								text_window->text_selection_length = 0;
							}
							else
							{
								if (text_window->text_selection_offset >= text_window->text_length)
								{
									sdui_replace_window_text_by_address(gui, text_window, text_window->text_length, 0, return_text);
									text_window->text_selection_offset = (size_t)~0;
								}
								else
								{
									sdui_replace_window_text_by_address(gui, text_window, text_window->text_selection_offset, 0, return_text);
									text_window->text_selection_offset++;
								}
							}
						}
						else if (gui->control_key_active && gui->event.key.keysym.sym == SDLK_v)
						{
							if (gui->selected_window_id)
							{
								if ((text_window->parameters.style_flags & (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_EDITABLE)) == (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_EDITABLE))
								{
									char* clipboard = SDL_GetClipboardText();
									if (clipboard)
									{
										size_t clipboard_length = strlen(clipboard);
										size_t unicode_length;
										if (!sdui_decode_utf8(gui, clipboard, &unicode_length))
										{
											sdui_replace_window_text_by_address(gui, text_window, text_window->text_selection_offset, text_window->text_selection_length, (const uint32_t*)gui->temporal_buffer);
											if (text_window->text_selection_offset != (size_t)~0)
												text_window->text_selection_offset += unicode_length;
											text_window->text_selection_length = 0;
										}

										SDL_free(clipboard);
									}
								}
							}
						}
						else if (gui->control_key_active && gui->event.key.keysym.sym == SDLK_x)
						{
							const uint32_t null_terminator = 0;
							if (text_window->text_selection_length)
							{
								sdui_replace_window_text_by_address(gui, text_window, text_window->text_selection_offset, text_window->text_selection_length, &null_terminator);
								text_window->text_selection_length = 0;
							}
							else
							{
								sdui_set_window_unicode_text_by_address(gui, text_window, &null_terminator);
								text_window->text_selection_offset = (size_t)~0;
							}
						}
					}

					if (gui->control_key_active && gui->event.key.keysym.sym == SDLK_c)
					{
						if (gui->selected_window_id && (text_window->parameters.style_flags & (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_SELECTABLE)) == (SDUI_WINDOW_HAS_TEXT | SDUI_WINDOW_TEXT_IS_SELECTABLE))
						{
							size_t utf8_length;
							if (text_window->text_selection_offset < text_window->text_length && text_window->text_selection_length)
							{
								uint32_t cut_character = text_window->text[text_window->text_selection_offset + text_window->text_selection_length];
								text_window->text[text_window->text_selection_offset + text_window->text_selection_length] = 0;
								if (!sdui_encode_utf8(gui, text_window->text + text_window->text_selection_offset, &utf8_length))
									SDL_SetClipboardText((const char*)gui->temporal_buffer);
								text_window->text[text_window->text_selection_offset + text_window->text_selection_length] = cut_character;
							}
							else
							{
								if (!sdui_encode_utf8(gui, text_window->text, &utf8_length))
									SDL_SetClipboardText((const char*)gui->temporal_buffer);
							}
						}
					}
				}

				if (gui->event.key.keysym.sym == SDLK_LCTRL)
					gui->control_key_active = 1;

				gui->event_window_id = gui->selected_window_id ? gui->selected_window_id : gui->main_window_id;
				break;
			}
			case SDL_KEYUP:
			{
				if (gui->event.key.keysym.sym == SDLK_LCTRL)
					gui->control_key_active = 0;

				gui->event_window_id = gui->selected_window_id ? gui->selected_window_id : gui->main_window_id;
				break;
			}
			default:
			{
				gui->event_window_id = gui->main_window_id;
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
	SDL_Rect rectangle = { window->base_x + window->parameters.x, window->base_y + window->parameters.y, window->w, window->h };
	if (window->state_flags & SDIU_WINDOW_STATE_SINGLE_COLOR)
	{
		if ((window->parameters.style_flags & SDUI_WINDOW_IS_ACTIVE_UNDER_CURSOR) && (window->state_flags & SDIU_WINDOW_STATE_UNDER_CURSOR))
			SDL_SetRenderDrawColor(gui->renderer_handle, (window->parameters.pointed_background_color >> 24) & 0xFF, (window->parameters.pointed_background_color >> 16) & 0xFF, (window->parameters.pointed_background_color >> 8) & 0xFF, (window->parameters.pointed_background_color >> 0) & 0xFF);
		else
			SDL_SetRenderDrawColor(gui->renderer_handle, (window->parameters.background_color >> 24) & 0xFF, (window->parameters.background_color >> 16) & 0xFF, (window->parameters.background_color >> 8) & 0xFF, (window->parameters.background_color >> 0) & 0xFF);
		SDL_RenderFillRect(gui->renderer_handle, &rectangle);
	}
	else if (window->texture_handle)
	{
		//int set_color_error = SDL_SetRenderDrawColor(gui->renderer_handle, 0xFF, 0xFF, 0xFF, 0xFF);
		//assert(!set_color_error);
		int copy_error = SDL_RenderCopy(gui->renderer_handle, window->texture_handle, 0, &rectangle);
		assert(!copy_error);
	}

	return 0;
}

int sdui_update_windows_z_coordinas(sdui_ui_t* gui)
{
	assert(gui->temporal_buffer_size >= (gui->window_count * 2 * sizeof(size_t)));// When gui is created temporal_buffer_size needs to be made atleat this big
	
	for (size_t i = 0; i != gui->window_count; ++i)
		gui->window_table[i].state_flags &= ~(SDIU_WINDOW_STATE_ITERATED | SDIU_WINDOW_STATE_SELECT_INCREMENTED | SDIU_WINDOW_STATE_HIDDEN);

	sdui_window_t* main_window = sdui_get_window_address(gui, gui->main_window_id);

	main_window->z = (main_window->parameters.style_flags & SDUI_WINDOW_IS_VISIBLE) ? ((main_window->parameters.style_flags & SDUI_WINDOW_IS_HIGHLIGHT) ? SDUI_BASE_Z_HIGHLIGHT : ((main_window->parameters.style_flags & SDUI_WINDOW_IS_POPUP) ? SDUI_BASE_Z_POPUP : SDUI_BASE_Z_NORMAL)) : SDUI_BASE_Z_HIDDEN;
	main_window->state_flags |= SDIU_WINDOW_STATE_ITERATED;

	size_t parent_count = 0;
	size_t* parent_table = (size_t*)gui->temporal_buffer;
	size_t child_count = 1;
	size_t* child_table = (size_t*)gui->temporal_buffer + gui->window_count;
	child_table[0] = ((size_t)((uintptr_t)main_window - (uintptr_t)gui->window_table) / sizeof(sdui_window_t));

	int z_reordered = 0;

	while (child_count)
	{
		parent_count = child_count;
		size_t* table_swap = parent_table;
		parent_table = child_table;
		child_table = table_swap;
		child_count = 0;

		for (size_t i = 0; i != gui->window_count; ++i)
			if (!(gui->window_table[i].state_flags & SDIU_WINDOW_STATE_ITERATED))
				for (size_t j = 0; j != parent_count; ++j)
					if (gui->window_table[parent_table[j]].parameters.id == gui->window_table[i].parameters.parent_id)
					{
						int old_z = gui->window_table[i].z;

						gui->window_table[i].z = gui->window_table[parent_table[j]].z + 1;

						if ((gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_HIGHLIGHT) && (gui->window_table[i].z < SDUI_BASE_Z_HIGHLIGHT))
							gui->window_table[i].z += SDUI_BASE_Z_HIGHLIGHT;
						else if ((gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_POPUP) && (gui->window_table[i].z < SDUI_BASE_Z_POPUP))
							gui->window_table[i].z += SDUI_BASE_Z_POPUP;

						gui->window_table[i].state_flags |= (gui->window_table[parent_table[j]].state_flags & (SDIU_WINDOW_STATE_SELECT_INCREMENTED | SDIU_WINDOW_STATE_HIDDEN)) | SDIU_WINDOW_STATE_ITERATED;
						if ((gui->window_table[i].state_flags & SDIU_WINDOW_STATE_SELECTED) && !(gui->window_table[i].state_flags & SDIU_WINDOW_STATE_SELECT_INCREMENTED))
						{
							gui->window_table[i].z += SDUI_SELECT_Z_INCRMENT;
							gui->window_table[i].state_flags |= SDIU_WINDOW_STATE_SELECT_INCREMENTED;
						}

						if (!(gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_VISIBLE))
							gui->window_table[i].state_flags |= SDIU_WINDOW_STATE_HIDDEN;

						if (gui->window_table[i].state_flags & SDIU_WINDOW_STATE_HIDDEN)
							gui->window_table[i].z = SDUI_BASE_Z_HIDDEN;

						child_table[child_count++] = i;

						if (gui->window_table[i].z != old_z)
							z_reordered = 1;

						j = parent_count - 1;
					}
	}

	return z_reordered;

	/* 
	// old version does not support window selection
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		int z;
		if (gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_HIGHLIGHT)
			z = SDUI_BASE_Z_HIGHLIGHT;
		else if (gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_POPUP)
			z = SDUI_BASE_Z_POPUP;
		else
			z = SDUI_BASE_Z_NORMAL;

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
	*/
}

void sdui_sort_windows_by_z_coordinate(sdui_ui_t* gui)
{
	assert(gui->temporal_buffer_size >= (gui->window_count * sizeof(sdui_window_t)));// When gui is created temporal_buffer_size needs to be made atleat this big

	sdui_window_t* source_table = (sdui_window_t*)gui->temporal_buffer;
	sdui_window_t* destination_table = gui->window_table;

	sdui_copy(source_table, destination_table, gui->window_count * sizeof(sdui_window_t));

	int count_table[16];
	int offset_table[16];
	for (int i = 0; i != 3; ++i)
	{
		for (int j = 0; j != 16; ++j)
			count_table[j] = 0;
		for (int j = 0; j != gui->window_count; ++j)
			count_table[(source_table[j].z >> (i * 4)) & 0xF]++;
		offset_table[0] = 0;
		for (int j = 1; j != 16; ++j)
			offset_table[j] = offset_table[j - 1] + count_table[j - 1];
		for (int j = 0; j != gui->window_count; ++j)
			destination_table[offset_table[(source_table[j].z >> (i * 4)) & 0xF]++] = source_table[j];
		sdui_window_t* swap = source_table;
		source_table = destination_table;
		destination_table = swap;
	}

	/*
	// old verion. i do not like it
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
	*/
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
	if (sdui_update_windows_z_coordinas(gui))
		sdui_sort_windows_by_z_coordinate(gui);
	sdui_update_windows_parent_x_and_y_coordinas(gui, 0, 0);
	
	for (size_t i = 0; i != gui->window_count; ++i)
		if ((gui->window_table[i].state_flags & SDIU_WINDOW_STATE_UPDATE_TEXTURE) && !(gui->window_table[i].state_flags & SDIU_WINDOW_STATE_HIDDEN))
			sdui_create_window_texture(gui, gui->window_table + i);

	/*
	sdui_window_t* debug_window_table[128];
	for (size_t i = 0; i != 128; ++i)
		debug_window_table[i] = gui->window_table + i;
	*/

	SDL_SetRenderDrawColor(gui->renderer_handle, 0, 0, 0, 0xFF);
	SDL_RenderClear(gui->renderer_handle);
	SDL_Rect window_area;
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		int error = 0;
		if (gui->window_table[i].parameters.style_flags & SDUI_WINDOW_IS_VISIBLE)
		{
			if (gui->window_table[i].parameters.style_flags & SDUI_WINDOW_HAS_RENDER_CALLBACK)
			{
				window_area.x = gui->window_table[i].base_x + gui->window_table[i].parameters.x;
				window_area.y = gui->window_table[i].base_y + gui->window_table[i].parameters.y;
				window_area.w = gui->window_table[i].w;
				window_area.h = gui->window_table[i].h;
				error = gui->window_table[i].parameters.callback(gui->renderer_handle, gui->window_table[i].parameters.style_flags, gui->window_table[i].state_flags, &window_area, gui->window_table[i].parameters.user_data);
			}
			else
				error = sdui_draw_window(gui, gui->window_table + i);
			if (error)
				return error;
		}
	}
	SDL_RenderPresent(gui->renderer_handle);

	return 0;
}

void sdui_move_window_by_address(sdui_ui_t* gui, sdui_window_t* window, int x, int y, int w, int h)
{
	if (x > -1)
		window->parameters.x = x;
	if (y > -1)
		window->parameters.y = y;
	int update_texture = 0;
	if (w > -1)
	{
		window->parameters.w = w;
		update_texture = 1;
	}
	if (h > -1)
	{
		window->parameters.h = h;
		update_texture = 1;
	}
	if (update_texture)
		window->state_flags |= SDIU_WINDOW_STATE_UPDATE_TEXTURE;
}

void sdui_move_window(sdui_ui_t* gui, uint32_t window_id, int x, int y, int w, int h)
{
	sdui_window_t* window = sdui_get_window_address(gui, window_id);
	if (window)
		sdui_move_window_by_address(gui, window, x, y, w, h);
}