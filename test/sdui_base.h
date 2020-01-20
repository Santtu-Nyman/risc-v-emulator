/*
	sdui_base.h - v0.1 (2020-01-20) - public domain
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

#ifndef SDUI_BASE_H
#define SDUI_BASE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <SDL.h>

void sdui_copy(void* destination, const void* source, size_t size);

size_t sdui_string_size(const char* string);

size_t sdui_string_line_size(const char* string);

size_t sdui_get_string_line_offset(const char* string, size_t line_number);

size_t sdui_get_string_line_count(const char* string);

int sdui_string_compare(const char* a, const char* b);

size_t sdui_print_hex(uint32_t value, char* buffer);

size_t sdui_print_hex_digits(int digit_count, uint32_t value, char* buffer);

size_t sdui_print_unsigned(uint32_t value, char* buffer);

size_t sdui_print_signed(int32_t value, char* buffer);

void sdui_fill_rectengle(int w, int h, size_t stride, uint32_t color, uint32_t* image);

int sdui_are_rectengle_overlapped(const SDL_Rect* a, const SDL_Rect* b);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SDUI_BASE_H