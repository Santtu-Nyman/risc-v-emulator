/*
	sdui_font.h - v0.3 (2020-01-26) - public domain
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

#ifndef SDUI_FONT_H
#define SDUI_FONT_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <SDL.h>
#include "stb_truetype.h"

#define SDUI_MAXIMUM_FONT_GLYPH_LENGTH 0x200

int sdui_load_truetype_font(const char* truetype_file_name, stbtt_fontinfo** font_buffer);
	
int sdui_calculate_string_rectangle(stbtt_fontinfo* font, float font_height, size_t string_length, const char* string, SDL_Rect* string_rectangle);
	
int sdui_draw_string(int width, int height, uint32_t* pixels, float x, float y, stbtt_fontinfo* font, float font_height, size_t string_length, const char* string, uint32_t color);
	
int sdui_draw_string_with_select(int width, int height, uint32_t* pixels, float x, float y, stbtt_fontinfo* font, float font_height, size_t string_length, size_t select_offset, size_t select_length, const char* string, uint32_t color, uint32_t select_color);

int sdui_select_from_string(stbtt_fontinfo* font, float font_height, size_t string_length, const char* string, const SDL_Rect* select, size_t* select_offset, size_t* select_length);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SDUI_FONT_H