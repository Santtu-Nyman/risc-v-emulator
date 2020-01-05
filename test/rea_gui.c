#include "rea_gui.h"
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

int rea_load_truetype_font(const char* truetype_file_name, stbtt_fontinfo** font_buffer)
{
	int error = 0;
	FILE* handle = fopen(truetype_file_name, "rb");
	if (!handle)
	{
		error = errno;
		return error;
	}
	if (fseek(handle, 0, SEEK_END))
	{
		error = ferror(handle);
		fclose(handle);
		return error;
	}
	size_t size = (size_t)ftell(handle);
	if (size == (size_t)-1)
	{
		error = errno;
		fclose(handle);
		return error;
	}
	if (fseek(handle, 0, SEEK_SET))
	{
		error = ferror(handle);
		fclose(handle);
		return error;
	}
	const size_t font_info_size = ((sizeof(stbtt_fontinfo) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1));
	stbtt_fontinfo* buffer = (stbtt_fontinfo*)malloc(font_info_size + ((size + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1)));
	void* file_buffer = (void*)((uintptr_t)buffer + font_info_size);
	if (!buffer)
	{
		error = errno;
		fclose(handle);
		return error;
	}
	for (size_t index = 0; index != size;)
	{
		size_t read = fread((void*)((uintptr_t)file_buffer + index), 1, size - index, handle);
		if (!read)
		{
			error = ferror(handle);
			if (!error)
				error = EIO;
			free(buffer);
			fclose(handle);
			return error;
		}
		index += read;
	}
	fclose(handle);
	error = stbtt_InitFont(buffer, (const unsigned char*)file_buffer, 0) ? 0 : EBADMSG;
	if (error)
	{
		free(buffer);
		return error;
	}
	*font_buffer = buffer;
	return 0;
}

int rea_calculate_string_rectangle(stbtt_fontinfo* font, float font_height, size_t string_length, const char* string, SDL_Rect* string_rectangle)
{
	if (!string_length)
	{
		string_rectangle->x = 0;
		string_rectangle->y = 0;
		string_rectangle->w = 0;
		string_rectangle->h = 0;
		return 0;
	}

	if (font_height < 0.0f)
		return EINVAL;

	int x;
	int y;
	int w;
	int h;

	int ascent;
	stbtt_GetFontVMetrics(font, &ascent, 0, 0);
	float scale = stbtt_ScaleForPixelHeight(font, font_height);
	int baseline = (int)(scale * (float)ascent);

	assert(string_length);

	float draw_y = 0.0f;
	float draw_x = 0.0f;
	for (size_t i = 0; i != string_length; ++i)
	{
		if (string[i] != (uint32_t)'\n')
		{
			float x_shift = draw_x - floorf(draw_x);
			int advance;
			int lsb;
			int left;
			stbtt_GetCodepointHMetrics(font, (int)string[i], &advance, &lsb);
			int right;
			int bottom;
			int top;
			stbtt_GetCodepointBitmapBoxSubpixel(font, (int)string[i], scale, scale, x_shift, 0, &left, &bottom, &right, &top);
			int blit_x = (int)draw_x + left;
			int blit_y = (int)draw_y + bottom + baseline;
			int blit_w = right - left;
			int blit_h = top - bottom;
			if (i)
			{
				if (blit_x < x)
					x = blit_x;
				if (blit_x + blit_w > x + w)
					w = blit_x + blit_w - x;
				if (blit_y < y)
					y = blit_y;
				if (blit_y + blit_h > y + h)
					h = blit_y + blit_h - y;
			}
			else
			{
				x = blit_x;
				y = blit_y;
				w = blit_w;
				h = blit_h;
			}
			draw_x += (scale * (float)advance);
			if (i + 1 != string_length)
				draw_x += scale * (float)stbtt_GetCodepointKernAdvance(font, (int)string[i], (int)string[i + 1]);
		}
		else
		{
			draw_x = 0.0f;
			draw_y += font_height;
		}
	}

	string_rectangle->x = x;
	string_rectangle->y = y;
	string_rectangle->w = w;
	string_rectangle->h = h;
	return 0;
}

int rea_draw_string(int width, int height, uint32_t* pixels, float x, float y, stbtt_fontinfo* font, float font_height, size_t string_length, const char* string, uint32_t color)
{
	if (width < 1 || height < 1 || font_height < 0.0f)
		return EINVAL;

	uint32_t color_b = ((color >> 8) & 0xFF);
	uint32_t color_g = ((color >> 16) & 0xFF);
	uint32_t color_r = ((color >> 24) & 0xFF);

	float string_x_offset = x;
	int glyph_bitmap_width = (int)font_height * 2 + 1;
	int glyph_bitmap_height = glyph_bitmap_width;
	uint8_t* glyph_bitmap = (uint8_t*)malloc(glyph_bitmap_width * glyph_bitmap_height);
	if (!glyph_bitmap)
		return ENOMEM;
	int ascent;
	stbtt_GetFontVMetrics(font, &ascent, 0, 0);
	float scale = stbtt_ScaleForPixelHeight(font, font_height);
	int baseline = (int)(scale * (float)ascent);

	for (size_t i = 0; i != string_length; ++i)
	{
		if (string[i] != (uint32_t)'\n')
		{
			float x_shift = x - floorf(x);
			int advance;
			int lsb;
			int left;
			int right;
			int bottom;
			int top;
			int w;
			int h;
			stbtt_GetCodepointHMetrics(font, (int)string[i], &advance, &lsb);
			stbtt_GetCodepointBitmapBoxSubpixel(font, (int)string[i], scale, scale, x_shift, 0, &left, &bottom, &right, &top);
			w = right - left;
			h = top - bottom;
			if (w > glyph_bitmap_width && h > glyph_bitmap_height)
			{
				if (w > glyph_bitmap_width)
					glyph_bitmap_width = w;
				if (h > glyph_bitmap_height)
					glyph_bitmap_height = h;
				uint8_t* new_glyph_bitmap = (uint8_t*)realloc(glyph_bitmap, glyph_bitmap_width * glyph_bitmap_height);
				if (!new_glyph_bitmap)
				{
					free(glyph_bitmap);
					return ENOMEM;
				}
				glyph_bitmap = new_glyph_bitmap;
			}
			stbtt_MakeCodepointBitmapSubpixel(font, glyph_bitmap, w, h, w, scale, scale, x_shift, 0, (int)string[i]);
			int blit_y = (int)y + bottom + baseline;
			int blit_y_end = blit_y + h;
			int blit_x = (int)x + left;
			int blit_x_end = blit_x + w;
			if (blit_y < height &&
				blit_y_end > 0 &&
				blit_x < width &&
				blit_x_end > 0)
			{
				int bounding_y = blit_y < 0 ? 0 : blit_y;
				int bounding_y_end = blit_y_end > height ? height : blit_y_end;
				int bounding_x = blit_x < 0 ? 0 : blit_x;
				int bounding_x_end = blit_x_end > width ? width : blit_x_end;
				for (int blend_y = bounding_y; blend_y != bounding_y_end; ++blend_y)
				{
					int glyph_y = blend_y - blit_y;
					uint32_t* row = (uint32_t*)((uintptr_t)pixels + (size_t)blend_y * ((size_t)width * 4));
					for (int blend_x = bounding_x; blend_x != bounding_x_end; ++blend_x)
					{
						/* RRGGBBAA */
						int glyph_x = blend_x - blit_x;
						uint32_t glyph_intensity = (uint32_t)glyph_bitmap[glyph_y * w + glyph_x];
						uint32_t destination = row[blend_x];
						
						uint32_t b = ((((destination >>  8) & 0xFF) * (0xFF - glyph_intensity)) + (color_b * glyph_intensity)) / 0xFF;
						uint32_t g = ((((destination >> 16) & 0xFF) * (0xFF - glyph_intensity)) + (color_g * glyph_intensity)) / 0xFF;
						uint32_t r = ((((destination >> 24) & 0xFF) * (0xFF - glyph_intensity)) + (color_r * glyph_intensity)) / 0xFF;

						row[blend_x] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
					}
				}
			}
			x += (scale * (float)advance);
			if (i + 1 != string_length)
				x += scale * (float)stbtt_GetCodepointKernAdvance(font, (int)string[i], (int)string[i + 1]);
		}
		else
		{
			x = string_x_offset;
			y += font_height;
		}
	}
	free(glyph_bitmap);
	return 0;
}

int rea_grow_gui_temporal_buffer(rea_gui_t* gui, size_t required_size)
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

rea_window_t* rea_get_window_by_id(rea_gui_t* gui, uint32_t window_id)
{
	for (size_t i = 0; i != gui->window_count; ++i)
		if (gui->window_table[i].id == window_id)
			return gui->window_table + i;
	return 0;
}

rea_window_t* rea_get_window_by_coordinate(rea_gui_t* gui, int x, int y)
{
	// Windows need to be sorted by z and has to be paint_data set correctly

	for (size_t i = gui->window_count - 1; i != -1; --i)
		if ((x >= gui->window_table[i].paint_data.base_x + gui->window_table[i].x) &&
			(x < gui->window_table[i].paint_data.base_x + gui->window_table[i].x + gui->window_table[i].paint_data.w) &&
			(y >= gui->window_table[i].paint_data.base_y + gui->window_table[i].y) &&
			(y < gui->window_table[i].paint_data.base_y + gui->window_table[i].y + gui->window_table[i].paint_data.h))
			return gui->window_table + i;
	return 0;
}

rea_window_t* rea_get_main_window(rea_gui_t* gui)
{
	// one main window must allways exist

	for (size_t i = 0;; ++i)
		if (!gui->window_table[i].parent_id)
			return gui->window_table + i;
}

int rea_create_window_texture(rea_gui_t* gui, rea_window_t* window)
{
	int w = window->w;
	int h = window->h;

	if (!w || !h)
	{
		if (window->paint_data.texture_handle)
			SDL_DestroyTexture(window->paint_data.texture_handle);
		window->paint_data.w = 0;
		window->paint_data.h = 0;
		window->paint_data.texture_handle = 0;
		window->paint_data.update_texture = 0;
		return 0;
	}

	int border_h = (window->border_thickness < (h / 2)) ? window->border_thickness : (h / 2);
	int border_w = (window->border_thickness < (w / 2)) ? window->border_thickness : (w / 2);
	size_t dropdown_menu_item_count = 0;

	if ((window->style_flags & REA_WINDOW_IS_DROPDOWN_MENU) && window->text && window->paint_data.window_is_selected)
	{
		dropdown_menu_item_count = rel32_get_string_line_count(window->text);
		h += (2 * border_h) + ((int)dropdown_menu_item_count * window->font_size);
	}

	if (window->paint_data.w != w || window->paint_data.h != h)
	{
		if (window->paint_data.texture_handle)
		{
			SDL_DestroyTexture(window->paint_data.texture_handle);
			window->paint_data.texture_handle = 0;
		}

		window->paint_data.w = w;
		window->paint_data.h = h;
		window->paint_data.texture_handle = SDL_CreateTexture(gui->renderer_handle, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);
		if (!window->paint_data.texture_handle)
		{
			window->paint_data.w = 0;
			window->paint_data.h = 0;
			return ENOMEM;
		}
	}

	if (gui->temporal_buffer_size < ((size_t)h * (size_t)w * 4))
	{
		int grow_gui_temporal_buffer_error = rea_grow_gui_temporal_buffer(gui, (size_t)h * (size_t)w * 4);
		if (grow_gui_temporal_buffer_error)
			return grow_gui_temporal_buffer_error;
	}

	uint32_t* image = (uint32_t*)gui->temporal_buffer;
	
	for (int y = 0; y != border_h; ++y)
		for (int x = 0; x != w; ++x)
			image[y * w + x] = window->border_color;
	for (int y = border_h; y != (h - border_h); ++y)
	{
		for (int x = 0; x != border_w; ++x)
			image[y * w + x] = window->border_color;
		for (int x = border_w; x != (w - border_w); ++x)
			image[y * w + x] = window->background_color;
		for (int x = (w - border_w); x != w; ++x)
			image[y * w + x] = window->border_color;
	}
	for (int y = (h - border_h); y != h; ++y)
		for (int x = 0; x != w; ++x)
			image[y * w + x] = window->border_color;

	if (window->style_flags & REA_WINDOW_IS_DROPDOWN_MENU)
	{
		int menu_icon_size = window->font_size ? window->font_size : (window->h < w ? window->h : w);
		if (menu_icon_size > (window->h - (2 * border_h)))
			menu_icon_size = (window->h - (2 * border_h));
		if (menu_icon_size > (window->w - (2 * border_w)))
			menu_icon_size = (window->w - (2 * border_w));
		int menu_icon_x = window->w - (menu_icon_size + border_w);
		int menu_icon_y = (window->h / 2) - (menu_icon_size / 2);
		rea_draw_dropdown_icon_bitmap(menu_icon_size, menu_icon_size, w * 4, window->background_color, window->text_color, image + ((size_t)menu_icon_y * (size_t)w + (size_t)menu_icon_x));
	}

	if (window->text)
	{
		size_t display_string_offset;
		size_t display_string_length;
		if (window->style_flags & REA_WINDOW_IS_DROPDOWN_MENU)
		{
			display_string_offset = rel32_get_string_line_offset(window->text, window->paint_data.selected_item);
			display_string_length = rel32_string_line_size(window->text + display_string_offset);
		}
		else
		{
			display_string_offset = 0;
			display_string_length = window->text_length;
		}

		int text_offset_x = window->border_thickness + 1;
		int text_offset_y = window->border_thickness + 1;

		SDL_Rect string_rectangle;
		rea_calculate_string_rectangle(gui->test_font.font, window->font_size, display_string_length, window->text + display_string_offset, &string_rectangle);
		if (window->style_flags & REA_CENTER_WINDOW_TEXT_HORIZONTALLY)
			text_offset_x = ((window->w - string_rectangle.w) / 2) - string_rectangle.x;
		if (window->style_flags & REA_CENTER_WINDOW_TEXT_VERTICALLY)
			text_offset_y = ((window->h - string_rectangle.h) / 2) - string_rectangle.y;

		//for (int y = text_offset_y + string_rectangle.y; y != text_offset_y + string_rectangle.y + string_rectangle.h; ++y)
		//	for (int x = text_offset_x + string_rectangle.x; x != text_offset_x + string_rectangle.x + string_rectangle.w; ++x)
		//		image[y * w + x] = 0xFF0000FF;

		rea_draw_string(w, h, image, text_offset_x, text_offset_y, gui->test_font.font, window->font_size, display_string_length, window->text + display_string_offset, window->text_color);
	
		if ((window->style_flags & REA_WINDOW_IS_DROPDOWN_MENU) && window->paint_data.window_is_selected)
			rea_draw_string(w, h, image, text_offset_x, window->h, gui->test_font.font, window->font_size, window->text_length, window->text, window->text_color);
	}

	int texture_update_error = SDL_UpdateTexture(window->paint_data.texture_handle, 0, image, w * 4);
	assert(!texture_update_error);

	window->paint_data.update_texture = 0;
	return 0;
}

int rea_set_window_text(rea_gui_t* gui, uint32_t window_id, const char* text)
{
	rea_window_t* window = rea_get_window_by_id(gui, window_id);
	if (!window)
		return ENOENT;

	size_t text_length = strlen(text);

	if (window->text)
	{
		if (text_length == window->text_length && !memcmp(window->text, text, text_length))
			return 0;

		if (window->paint_data.text_allocation_length < text_length + 1)
		{
			size_t new_allocation_size = ((text_length + 1) + (REA_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(REA_TEXT_ALLOCATION_GRANULARITY - 1);
			char* new_allocation = (char*)realloc(window->text, new_allocation_size);
			if (!new_allocation)
				return ENOMEM;
			window->paint_data.text_allocation_length = new_allocation_size;
			window->text = new_allocation;
		}

		window->text_length = text_length;
		memcpy(window->text, text, text_length + 1);
		window->paint_data.update_texture = 1;
	}
	else
	{
		size_t new_allocation_size = ((text_length + 1) + (REA_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(REA_TEXT_ALLOCATION_GRANULARITY - 1);
		window->text = (char*)malloc(new_allocation_size);
		if (!window->text)
			return ENOMEM;
		window->paint_data.text_allocation_length = new_allocation_size;

		window->text_length = text_length;
		memcpy(window->text, text, text_length + 1);
		window->paint_data.update_texture = 1;
	}
	return 0;
}

int rea_set_window_text_with_signed_number(rea_gui_t* gui, uint32_t window_id, int32_t value)
{
	char text_buffer[16];
	text_buffer[rel32_print_signed(value, text_buffer)] = 0;
	return rea_set_window_text(gui, window_id, text_buffer);
}

int rea_set_window_text_with_unsigned_number(rea_gui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[rel32_print_unsigned(value, text_buffer)] = 0;
	return rea_set_window_text(gui, window_id, text_buffer);
}

int rea_set_window_text_with_hexadecimal_number(rea_gui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[rel32_print_hex(value, text_buffer)] = 0;
	return rea_set_window_text(gui, window_id, text_buffer);
}

int rea_append_window_text(rea_gui_t* gui, uint32_t window_id, const char* text)
{
	rea_window_t* window = rea_get_window_by_id(gui, window_id);
	if (!window)
		return ENOENT;

	size_t text_length = strlen(text);

	if (!text_length)
		return 0;

	if (window->text)
	{
		if (window->paint_data.text_allocation_length < window->text_length + text_length + 1)
		{
			size_t new_allocation_size = ((window->text_length + text_length + 1) + (REA_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(REA_TEXT_ALLOCATION_GRANULARITY - 1);
			char* new_allocation = (char*)realloc(window->text, new_allocation_size);
			if (!new_allocation)
				return ENOMEM;
			window->paint_data.text_allocation_length = new_allocation_size;
			window->text = new_allocation;
		}

		memcpy(window->text + window->text_length, text, text_length + 1);
		window->text_length += text_length;
		window->paint_data.update_texture = 1;
	}
	else
	{
		size_t new_allocation_size = ((text_length + 1) + (REA_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(REA_TEXT_ALLOCATION_GRANULARITY - 1);
		window->text = (char*)malloc(new_allocation_size);
		if (!window->text)
			return ENOMEM;
		window->paint_data.text_allocation_length = new_allocation_size;

		window->text_length = text_length;
		memcpy(window->text, text, text_length + 1);
		window->paint_data.update_texture = 1;
	}
	return 0;
}

int rea_append_window_text_with_signed_number(rea_gui_t* gui, uint32_t window_id, int32_t value)
{
	char text_buffer[16];
	text_buffer[rel32_print_signed(value, text_buffer)] = 0;
	return rea_append_window_text(gui, window_id, text_buffer);
}

int rea_append_window_text_with_unsigned_number(rea_gui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[rel32_print_unsigned(value, text_buffer)] = 0;
	return rea_append_window_text(gui, window_id, text_buffer);
}

int rea_append_window_text_with_hexadecimal_number(rea_gui_t* gui, uint32_t window_id, uint32_t value)
{
	char text_buffer[16];
	text_buffer[rel32_print_hex(value, text_buffer)] = 0;
	return rea_append_window_text(gui, window_id, text_buffer);
}

int rea_create_gui(rea_gui_t** gui_address, size_t gui_window_count, const rea_window_t* gui_window_table, uint32_t frame_per_second)
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

	rea_gui_t* gui = (rea_gui_t*)malloc(sizeof(rea_gui_t) + (gui_window_count * sizeof(rea_window_t)));
	if (!gui)
		return ENOMEM;

	if (SDL_Init(SDL_INIT_VIDEO))
	{
		free(gui);
		return ENOSYS;
	}
	
	gui->window_handle = SDL_CreateWindow(REA_GUI_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, main_window_w, main_window_h, SDL_WINDOW_SHOWN);
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

	gui->window_table = (rea_window_t*)((uintptr_t)gui + sizeof(rea_gui_t));
	gui->control_key_active = 0;
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
			gui->window_table[i].x = gui_window_table[i].x;
			gui->window_table[i].y = gui_window_table[i].y;
		}
		else
		{
			gui->window_table[i].x = 0;
			gui->window_table[i].y = 0;
		}
		gui->window_table[i].w = gui_window_table[i].w;
		gui->window_table[i].h = gui_window_table[i].h;
		gui->window_table[i].border_thickness = gui_window_table[i].border_thickness;
		gui->window_table[i].font_size = gui_window_table[i].font_size;
		gui->window_table[i].style_flags = gui_window_table[i].style_flags;
		gui->window_table[i].parent_id = gui_window_table[i].parent_id;
		gui->window_table[i].id = gui_window_table[i].id;
		gui->window_table[i].background_color = gui_window_table[i].background_color;
		gui->window_table[i].border_color = gui_window_table[i].border_color;
		gui->window_table[i].text_color = gui_window_table[i].text_color;

		if (gui_window_table[i].text)
		{
			gui->window_table[i].text_length = gui_window_table[i].text_length;
			gui->window_table[i].paint_data.text_allocation_length = ((gui->window_table[i].text_length + 1) + (REA_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(REA_TEXT_ALLOCATION_GRANULARITY - 1);
			gui->window_table[i].text = (char*)malloc(gui->window_table[i].paint_data.text_allocation_length);

			assert(gui->window_table[i].text);

			memcpy(gui->window_table[i].text, gui_window_table[i].text, gui->window_table[i].text_length + 1);
		}
		else
		{
			gui->window_table[i].text_length = 0;
			gui->window_table[i].text = 0;
			gui->window_table[i].paint_data.text_allocation_length = 0;
		}

		gui->window_table[i].paint_data.text_allocation_length = 0;
		gui->window_table[i].paint_data.selected_item = 0;
		gui->window_table[i].paint_data.z = 0;
		gui->window_table[i].paint_data.base_x = 0;
		gui->window_table[i].paint_data.base_y = 0;
		gui->window_table[i].paint_data.w = 0;
		gui->window_table[i].paint_data.h = 0;
		gui->window_table[i].paint_data.window_is_under_cursor = 0;
		gui->window_table[i].paint_data.window_is_selected = 0;
		gui->window_table[i].paint_data.texture_handle = 0;
		gui->window_table[i].paint_data.update_texture = 1;
	}

	for (int top_level_set = 1; top_level_set;)
	{
		top_level_set = 0;
		for (size_t i = 0; i != gui_window_count; ++i)
			for (size_t j = 0; j != gui_window_count; ++j)
				if ((gui->window_table[j].style_flags & REA_WINDOW_IS_TOP_LEVEL) && (gui->window_table[j].id == gui->window_table[i].parent_id) && !(gui->window_table[i].style_flags & REA_WINDOW_IS_TOP_LEVEL))
				{
					gui->window_table[i].style_flags |= REA_WINDOW_IS_TOP_LEVEL;
					top_level_set = 1;
				}
	}

	int font_load_error = rea_load_truetype_font("c:\\windows\\fonts\\arial.ttf", &gui->test_font.font);
	assert(!font_load_error);

	*gui_address = gui;
	return 0;
}

void rea_close_gui(rea_gui_t* gui)
{
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		if (gui->window_table[i].paint_data.texture_handle)
			SDL_DestroyTexture(gui->window_table[i].paint_data.texture_handle);
		if (gui->window_table[i].text)
			free(gui->window_table[i].text);
	}
	free(gui->temporal_buffer);
	SDL_StopTextInput();
	SDL_DestroyRenderer(gui->renderer_handle);
	SDL_DestroyWindow(gui->window_handle);
	SDL_Quit();
	free(gui);
}

int rea_poll_event(rea_gui_t* gui)
{
	//rea_window_t* debug_window_table[16];
	//for (size_t i = 0; i != 16; ++i)
	//	debug_window_table[i] = gui->window_table + i;

	if (SDL_PollEvent(&gui->event))
	{
		switch (gui->event.type)
		{
			case SDL_MOUSEBUTTONDOWN:
			{
				rea_window_t* select_window = rea_get_window_by_coordinate(gui, gui->event.button.x, gui->event.button.y);
				if (gui->selected_window && gui->selected_window != select_window)
				{
					gui->selected_window->paint_data.window_is_selected = 0;
					gui->selected_window->paint_data.update_texture = 1;
				}
				if (select_window && (select_window->style_flags & REA_WINDOW_IS_SELECTABLE))
				{
					if (gui->selected_window != select_window)
					{
						gui->selected_window = select_window;
						gui->selected_window->paint_data.window_is_selected = 1;
						gui->selected_window->paint_data.update_texture = 1;
					}
					if ((select_window->style_flags & REA_WINDOW_IS_DROPDOWN_MENU) && select_window->text)
					{
						int menu_item_count = (int)rel32_get_string_line_count(select_window->text);
						if (menu_item_count)
						{
							int menu_h = select_window->paint_data.h - select_window->h;
							int memu_y = gui->event.button.y - (select_window->paint_data.base_y + select_window->y + select_window->h);
							int menu_item_h = menu_h / menu_item_count;
							size_t selected_item = (memu_y < 0) ? select_window->paint_data.selected_item : (memu_y / menu_item_h);
							if (selected_item == menu_item_count)
								selected_item = menu_item_count - 1;
							if (selected_item != select_window->paint_data.selected_item)
							{
								select_window->paint_data.selected_item = selected_item;
								select_window->paint_data.window_is_selected = 1;
								select_window->paint_data.update_texture = 1;
							}
						}
					}
				}
				else
					gui->selected_window = 0;

				gui->event_window = gui->selected_window ? gui->selected_window : gui->main_window;
				break;
			}
			case SDL_TEXTINPUT:
			{
				if (gui->selected_window && gui->selected_window->text)
				{
					size_t append_length = strlen(gui->event.text.text);

					if (gui->selected_window->text_length + append_length + 1 > gui->selected_window->paint_data.text_allocation_length)
					{
						gui->selected_window->paint_data.text_allocation_length = ((gui->selected_window->text_length + append_length + 1) + (REA_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(REA_TEXT_ALLOCATION_GRANULARITY - 1);
						gui->selected_window->text = (char*)realloc(gui->selected_window->text, gui->selected_window->paint_data.text_allocation_length);
						assert(gui->selected_window->text);
					}

					memcpy(gui->selected_window->text + gui->selected_window->text_length, gui->event.text.text, append_length + 1);
					gui->selected_window->text_length += append_length;
					gui->selected_window->paint_data.update_texture = 1;
				}

				gui->event_window = gui->selected_window ? gui->selected_window : gui->main_window;
				break;
			}
			case SDL_KEYDOWN:
			{
				if (gui->selected_window && gui->selected_window->text)
				{
					if (gui->event.key.keysym.sym == SDLK_BACKSPACE)
					{
						if (gui->selected_window->text_length)
						{
							gui->selected_window->text_length--;
							gui->selected_window->text[gui->selected_window->text_length] = 0;
							gui->selected_window->paint_data.update_texture = 1;
						}
					}
					else if (gui->event.key.keysym.sym == SDLK_v && gui->control_key_active)
					{
						char* clipboard = SDL_GetClipboardText();
						if (clipboard)
						{
							size_t append_length = strlen(clipboard);

							if (gui->selected_window->text_length + append_length + 1 > gui->selected_window->paint_data.text_allocation_length)
							{
								gui->selected_window->paint_data.text_allocation_length = ((gui->selected_window->text_length + append_length + 1) + (REA_TEXT_ALLOCATION_GRANULARITY - 1)) & (size_t)~(REA_TEXT_ALLOCATION_GRANULARITY - 1);
								gui->selected_window->text = (char*)realloc(gui->selected_window->text, gui->selected_window->paint_data.text_allocation_length);
								assert(gui->selected_window->text);
							}

							memcpy(gui->selected_window->text + gui->selected_window->text_length, clipboard, append_length + 1);
							gui->selected_window->text_length += append_length;
							gui->selected_window->paint_data.update_texture = 1;

							SDL_free(clipboard);
						}
					}
					else if (gui->event.key.keysym.sym == SDLK_c && gui->control_key_active)
					{
						SDL_SetClipboardText(gui->selected_window->text);
					}
					else if (gui->event.key.keysym.sym == SDLK_x && gui->control_key_active)
					{
						gui->selected_window->text_length = 0;
						gui->selected_window->text[0] = 0;
						gui->selected_window->paint_data.update_texture = 1;
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

int rea_draw_window(rea_gui_t* gui, rea_window_t* window)
{
	//SDL_Rect rectangle = { window->paint_data.base_x + window->x, window->paint_data.base_y + window->y, window->w, window->h };
	//SDL_SetRenderDrawColor(gui->renderer_handle, (window->background_color >> 0) & 0xFF, (window->background_color >> 8) & 0xFF, (window->background_color >> 16) & 0xFF, 0xFF);
	//return SDL_RenderFillRect(gui->renderer_handle, &rectangle);

	if (window->paint_data.texture_handle)
	{
		SDL_Rect rectangle = { window->paint_data.base_x + window->x, window->paint_data.base_y + window->y, window->paint_data.w, window->paint_data.h };
		//int set_color_error = SDL_SetRenderDrawColor(gui->renderer_handle, 0xFF, 0xFF, 0xFF, 0xFF);
		//assert(!set_color_error);
		int copy_error = SDL_RenderCopy(gui->renderer_handle, window->paint_data.texture_handle, 0, &rectangle);
		assert(!copy_error);
	}

	return 0;
}

void rea_update_windows_z_coordinas(rea_gui_t* gui)
{
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		int z = (gui->window_table[i].style_flags & REA_WINDOW_IS_TOP_LEVEL) ? 0x1000 : 0;
		uint32_t parent_id = gui->window_table[i].parent_id;
		size_t parent_index = (size_t)~0;
		do
		{
			parent_index = (size_t)~0;
			for (size_t j = 0; parent_id && j != gui->window_count; ++j)
				if (gui->window_table[j].id == parent_id)
				{
					parent_index = j;
					parent_id = gui->window_table[j].parent_id;
					++z;
				}
		} while (parent_index != (size_t)~0);
		gui->window_table[i].paint_data.z = z;
	}
}

void rea_sort_windows_by_z_coordinate(rea_gui_t* gui)
{
	// this sort is slow but, it doas not matter coz gui->window_count is small and this table is probably sorted anyway
	
	if (gui->window_count)
	{
		rea_window_t tmp;
		for (size_t i = 1; i != gui->window_count;)
			if (gui->window_table[i - 1].paint_data.z > gui->window_table[i].paint_data.z)
			{
				memcpy(&tmp, gui->window_table + i - 1, sizeof(rea_window_t));
				memcpy(gui->window_table + i - 1, gui->window_table + i, sizeof(rea_window_t));
				memcpy(gui->window_table + i, &tmp, sizeof(rea_window_t));
				if (i)
					--i;
			}
			else
				++i;
	}
}

void rea_update_windows_parent_x_and_y_coordinas(rea_gui_t* gui, int base_x, int base_y)
{
	for (size_t i = 0; i != gui->window_count; ++i)
	{
		size_t parent_index = (size_t)~0;
		for (size_t j = 0; parent_index == (size_t)~0 && j != i; ++j)
			if (gui->window_table[j].id == gui->window_table[i].parent_id)
				parent_index = j;
		if (parent_index != (size_t)~0)
		{
			gui->window_table[i].paint_data.base_x = base_x + gui->window_table[parent_index].paint_data.base_x + gui->window_table[parent_index].x;
			gui->window_table[i].paint_data.base_y = base_y + gui->window_table[parent_index].paint_data.base_y + gui->window_table[parent_index].y;
		}
		else
		{
			gui->window_table[i].paint_data.base_x = base_x;
			gui->window_table[i].paint_data.base_y = base_y;
		}
	}
}

int rea_draw_gui(rea_gui_t* gui)
{
	rea_update_windows_z_coordinas(gui);
	rea_sort_windows_by_z_coordinate(gui);
	rea_update_windows_parent_x_and_y_coordinas(gui, 0, 0);
	gui->main_window = rea_get_main_window(gui);

	for (size_t i = 0; i != gui->window_count; ++i)
		if (gui->window_table[i].paint_data.update_texture)
			rea_create_window_texture(gui, gui->window_table + i);

	//rea_window_t* debug_window_table[16];
	//for (size_t i = 0; i != 16; ++i)
	//	debug_window_table[i] = gui->window_table + i;

	SDL_SetRenderDrawColor(gui->renderer_handle, 0, 0, 0, 0xFF);
	SDL_RenderClear(gui->renderer_handle);
	for (size_t i = 0; i != gui->window_count; ++i)
		if (gui->window_table + 1 != gui->selected_window)
		{
			int error = rea_draw_window(gui, gui->window_table + i);
			if (error)
				return error;
		}
	if (gui->selected_window)
		rea_draw_window(gui, gui->selected_window);
	SDL_RenderPresent(gui->renderer_handle);

	return 0;
}

void rea_draw_dropdown_icon_bitmap(int w, int h, size_t stride, uint32_t background_color, uint32_t icon_color, uint32_t* image)
{
	assert(!(stride % sizeof(uint32_t)));
	stride /= sizeof(uint32_t);
	int icon_w;
	if (w > h)
	{
		if (h > 2)
			icon_w = ((h - 2) / 2) + ((h - 2) / 4);
		else
			icon_w = h;
	}
	else
	{
		if (w > 2)
			icon_w = ((w - 2) / 2) + ((w - 2) / 4);
		else
			icon_w = w;
	}
	int horizontal_skip = (w / 2) - (icon_w / 2);
	if (icon_w && !(icon_w & 1))
	{
		icon_w--;
		horizontal_skip++;
	}
	int icon_h = (icon_w / 2) + 1;
	int vertical_skip = (h / 2) - (icon_h / 2);

	for (int y = 0; y != vertical_skip; ++y)
		for (int x = 0; x != w; ++x)
			image[y * stride + x] = background_color;
	for (int y = vertical_skip; y != (vertical_skip + icon_h); ++y)
	{
		for (int x = 0; x != horizontal_skip + (y - vertical_skip); ++x)
			image[y * stride + x] = background_color;
		for (int x = horizontal_skip + (y - vertical_skip); x != (horizontal_skip + (y - vertical_skip) + (icon_w - (2 * (y - vertical_skip)))); ++x)
			image[y * stride + x] = icon_color;
		for (int x = horizontal_skip + (y - vertical_skip) + (icon_w - (2 * (y - vertical_skip))); x != w; ++x)
			image[y * stride + x] = background_color;
	}
	for (int y = (vertical_skip + icon_h); y != h; ++y)
		for (int x = 0; x != w; ++x)
			image[y * stride + x] = background_color;
}