/*
	sdui_font.c - v0.4.0 (2020-02-02) - public domain
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

#include "sdui_font.h"
#include "sdui_base.h"
#include <assert.h>
#include <stdio.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define SDUI_FONT_HEADER_ALIGNMENT 16
#define SDUI_FONT_HEADER_SIZE ((sizeof(stbtt_fontinfo) + (SDUI_FONT_HEADER_ALIGNMENT - 1)) & ~(SDUI_FONT_HEADER_ALIGNMENT - 1))
#define SDUI_FONT_GLYPH_BUFFER_SIZE (((SDUI_MAXIMUM_FONT_GLYPH_LENGTH * SDUI_MAXIMUM_FONT_GLYPH_LENGTH) + (SDUI_FONT_HEADER_ALIGNMENT - 1)) & ~(SDUI_FONT_HEADER_ALIGNMENT - 1))

int sdui_load_truetype_font(const char* truetype_file_name, stbtt_fontinfo** font_buffer)
{
#ifndef _MSC_VER
	int error = 0;
	FILE* handle = fopen(truetype_file_name, "rb");
	if (!handle)
	{
		error = errno;
		return error;
	}
#else
	FILE* handle;
	int error = fopen_s(&handle, truetype_file_name, "rb");
	if (error)
		return error;
	__assume(handle);
#endif
	if (fseek(handle, 0, SEEK_END))
	{
		error = ferror(handle);
		fclose(handle);
		return error;
	}
	size_t file_size = (size_t)ftell(handle);
	if (file_size == (size_t)-1)
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
	stbtt_fontinfo* buffer = (stbtt_fontinfo*)malloc(SDUI_FONT_HEADER_SIZE + SDUI_FONT_GLYPH_BUFFER_SIZE + ((file_size + (SDUI_FONT_HEADER_ALIGNMENT - 1)) & ~(SDUI_FONT_HEADER_ALIGNMENT - 1)));
	void* file_buffer = (void*)((uintptr_t)buffer + SDUI_FONT_HEADER_SIZE + SDUI_FONT_GLYPH_BUFFER_SIZE);
	if (!buffer)
	{
		error = errno;
		fclose(handle);
		return error;
	}
	for (size_t index = 0; index != file_size;)
	{
		size_t read = fread((void*)((uintptr_t)file_buffer + index), 1, file_size - index, handle);
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

int sdui_calculate_string_rectangle(stbtt_fontinfo* font, float font_height, size_t string_length, const uint32_t* string, SDL_Rect* string_rectangle)
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

int sdui_draw_string(int width, int height, uint32_t* pixels, float x, float y, stbtt_fontinfo* font, float font_height, size_t string_length, const uint32_t* string, uint32_t color)
{
	if (width < 1 || height < 1 || font_height < 0.0f)
		return EINVAL;

	uint32_t color_b = ((color >> 8) & 0xFF);
	uint32_t color_g = ((color >> 16) & 0xFF);
	uint32_t color_r = ((color >> 24) & 0xFF);

	uint8_t* glyph_bitmap = (uint8_t*)((uintptr_t)font + SDUI_FONT_HEADER_SIZE);
	float string_x_offset = x;
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
			if (w > SDUI_MAXIMUM_FONT_GLYPH_LENGTH && h > SDUI_MAXIMUM_FONT_GLYPH_LENGTH)
				return ENOMEM;
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

						uint32_t b = ((((destination >> 8) & 0xFF) * (0xFF - glyph_intensity)) + (color_b * glyph_intensity)) / 0xFF;
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
	return 0;
}

int sdui_draw_string_with_select(int width, int height, uint32_t* pixels, float x, float y, stbtt_fontinfo* font, float font_height, size_t string_length, size_t select_offset, size_t select_length, const uint32_t* string, uint32_t color, uint32_t select_color)
{
	if (width < 1 || height < 1 || font_height < 0.0f)
		return EINVAL;

	if (select_offset > string_length)
	{
		select_offset = 0;
		select_length = 0;
	}
	else if (select_offset + select_length > string_length)
		select_length = string_length - select_offset;

	uint32_t color_b = ((color >> 8) & 0xFF);
	uint32_t color_g = ((color >> 16) & 0xFF);
	uint32_t color_r = ((color >> 24) & 0xFF);

	uint32_t select_color_b = ((select_color >> 8) & 0xFF);
	uint32_t select_color_g = ((select_color >> 16) & 0xFF);
	uint32_t select_color_r = ((select_color >> 24) & 0xFF);

	float string_x_offset = x;
	float string_y_offset = y;
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
			stbtt_GetCodepointHMetrics(font, (int)string[i], &advance, &lsb);
			float fp_w = (scale * (float)advance) + ((i + 1 != string_length) ? (scale * (float)stbtt_GetCodepointKernAdvance(font, (int)string[i], (int)string[i + 1])) : 0);
			if ((i >= select_offset) && (i < select_offset + select_length))
			{
				int fill_x = (int)x;
				int fill_y = (int)y;
				int fill_w = (int)fp_w ? (int)fp_w + 1 : 0;
				int fill_h = (int)font_height;
				if (fill_x < 0)
				{
					fill_w += fill_x;
					fill_x = 0;
				}
				if (fill_w < 0)
					fill_w = 0;
				else if (fill_x + fill_w > width)
					fill_w = width - fill_x;
				if (fill_y < 0)
				{
					fill_h += fill_y;
					fill_y = 0;
				}
				if (fill_h < 0)
					fill_h = 0;
				else if (fill_y + fill_h > height)
					fill_h = height - fill_y;
				for (int fill_yl = fill_y, fill_yh = fill_y + fill_h; fill_yl != fill_yh; ++fill_yl)
				{
					uint32_t* fill_row = (uint32_t*)((uintptr_t)pixels + (size_t)fill_yl * ((size_t)width * 4));
					for (int fill_xl = fill_x, fill_xh = fill_x + fill_w; fill_xl != fill_xh; ++fill_xl)
						fill_row[fill_xl] = color;
				}
			}
			x += fp_w;
		}
		else
		{
			x = string_x_offset;
			y += font_height;
		}
	}


	uint8_t* glyph_bitmap = (uint8_t*)((uintptr_t)font + SDUI_FONT_HEADER_SIZE);
	x = string_x_offset;
	y = string_y_offset;
	for (size_t i = 0; i != string_length; ++i)
	{
		if (string[i] != (uint32_t)'\n')
		{
			uint32_t glyph_b;
			uint32_t glyph_g;
			uint32_t glyph_r;
			if ((i >= select_offset) && (i < select_offset + select_length))
			{
				glyph_b = select_color_b;
				glyph_g = select_color_g;
				glyph_r = select_color_r;
			}
			else
			{
				glyph_b = color_b;
				glyph_g = color_g;
				glyph_r = color_r;
			}

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
			if (w > SDUI_MAXIMUM_FONT_GLYPH_LENGTH && h > SDUI_MAXIMUM_FONT_GLYPH_LENGTH)
				return ENOMEM;
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
						int glyph_x = blend_x - blit_x;
						uint32_t glyph_intensity = (uint32_t)glyph_bitmap[glyph_y * w + glyph_x];
						uint32_t destination = row[blend_x];
						uint32_t b = ((((destination >> 8) & 0xFF) * (0xFF - glyph_intensity)) + (glyph_b * glyph_intensity)) / 0xFF;
						uint32_t g = ((((destination >> 16) & 0xFF) * (0xFF - glyph_intensity)) + (glyph_g * glyph_intensity)) / 0xFF;
						uint32_t r = ((((destination >> 24) & 0xFF) * (0xFF - glyph_intensity)) + (glyph_r * glyph_intensity)) / 0xFF;
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
	return 0;
}

int sdui_select_from_string(stbtt_fontinfo* font, float font_height, size_t string_length, const uint32_t* string, const SDL_Rect* select, size_t* select_offset, size_t* select_length)
{
	if (!string_length)
	{
		*select_offset = (size_t)~0;
		*select_length = 0;
		return 0;
	}

	if (font_height < 0.0f)
		return EINVAL;

	size_t offset = (size_t)~0;
	size_t length = 0;

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

			float fp_w = (scale * (float)advance) + ((i + 1 != string_length) ? (scale * (float)stbtt_GetCodepointKernAdvance(font, (int)string[i], (int)string[i + 1])) : 0);
			SDL_Rect fill = { (int)draw_x, (int)draw_y, (int)fp_w ? (int)fp_w + 1 : 0, (int)font_height };
			if (sdui_are_rectengle_overlapped(select, &fill))
			{
				if (offset != (size_t)~0)
				{
					if (offset + length - 1 < i)
						length = i - offset + 1;
				}
				else
				{
					offset = i;
					length = 1;
				}
			}
			draw_x += fp_w;

			/*
			int right;
			int bottom;
			int top;
			stbtt_GetCodepointBitmapBoxSubpixel(font, (int)string[i], scale, scale, x_shift, 0, &left, &bottom, &right, &top);
			SDL_Rect blit = { (int)draw_x + left, (int)draw_y + bottom + baseline, right - left, top - bottom };

			if (sdui_are_rectengle_overlapped(select, &blit))
			{
				if (offset != (size_t)~0)
				{
					if (offset + length - 1 < i)
						length = i - offset + 1;
				}
				else
				{
					offset = i;
					length = 1;
				}
			}

			draw_x += (scale * (float)advance);
			if (i + 1 != string_length)
				draw_x += scale * (float)stbtt_GetCodepointKernAdvance(font, (int)string[i], (int)string[i + 1]);
			*/
		}
		else
		{
			draw_x = 0.0f;
			draw_y += font_height;
		}
	}

	if (offset == (size_t)~0)
	{
		*select_offset = (size_t)~0;
		*select_length = 0;
	}
	else
	{
		*select_offset = offset;
		*select_length = length;
	}
	return 0;
}
