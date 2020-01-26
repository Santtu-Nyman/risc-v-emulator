/*
	sdui_base.c - v0.3 (2020-01-26) - public domain
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

#include "sdui_base.h"
#include <assert.h>

void sdui_fill_rectengle(int w, int h, size_t stride, uint32_t color, uint32_t* image)
{
	assert(w > -1 && h > -1);
	size_t row_size = (size_t)w * sizeof(uint32_t);
	size_t row_skip = stride - row_size;
	for (uint32_t* write = image, *image_end = (uint32_t*)((uintptr_t)image + ((size_t)h * stride)); write != image_end; write = (uint32_t*)((uintptr_t)write + row_skip))
		for (uint32_t* row_end = (uint32_t*)((uintptr_t)write + row_size); write != row_end; ++write)
			*write = color;
}

int sdui_are_rectengle_overlapped(const SDL_Rect* a, const SDL_Rect* b)
{
	return (a->w && a->h && b->w && b->h) &&
		((a->x >= b->x && a->x < (b->x + b->w)) || ((a->x + a->w - 1) >= b->x && (a->x + a->w - 1) < (b->x + b->w))) &&
		((a->y >= b->y && a->y < (b->y + b->h)) || ((a->y + a->h - 1) >= b->y && (a->y + a->h - 1) < (b->y + b->h)));
}

void sdui_copy(void* destination, const void* source, size_t size)
{
	for (const void* source_end = (const void*)((uintptr_t)source + size); source != source_end; source = (const void*)((uintptr_t)source + 1), destination = (void*)((uintptr_t)destination + 1))
		*(uint8_t*)destination = *(const uint8_t*)source;
}

size_t sdui_string_size(const char* string)
{
	const char* read_string = string;
	while (*read_string)
		++read_string;
	return (size_t)((uintptr_t)read_string - (uintptr_t)string);
}

size_t sdui_string_line_size(const char* string)
{
	const char* read_string = string;
	while (*read_string && *read_string != '\n' && *read_string != '\r')
		++read_string;
	return (size_t)((uintptr_t)read_string - (uintptr_t)string);
}

size_t sdui_get_string_line_offset(const char* string, size_t line_number)
{
	const char* read_string = string;
	for (size_t line_index = 0; line_index != line_number; ++line_index)
	{
		read_string += sdui_string_line_size(read_string);
		if (!*read_string)
			return (size_t)~0;
		++read_string;
	}
	return (size_t)((uintptr_t)read_string - (uintptr_t)string);
}

size_t sdui_get_string_line_count(const char* string)
{
	size_t line_count = 1;
	while (*string)
	{
		if (*string == '\n' || *string == '\r')
			++line_count;
		++string;
	}
	return line_count;
}

int sdui_string_compare(const char* a, const char* b)
{
	char a_tmp = *a++;
	char b_tmp = *b++;
	while (a_tmp != 0)
	{
		if (a_tmp != b_tmp)
			return 0;
		a_tmp = *a++;
		b_tmp = *b++;
	}
	return !b_tmp;
}

size_t sdui_print_hex(uint32_t value, char* buffer)
{
	static const char hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for (int i = 0; i != 8; ++i)
		buffer[i] = hex_table[(value >> ((7 - i) << 2)) & 0xF];
	return 8;
}

size_t sdui_print_hex_digits(int digit_count, uint32_t value, char* buffer)
{
	static const char hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	int fill = 0;
	while (digit_count > 8)
		buffer[fill++] = hex_table[0];

	for (int b = 8 - digit_count, i = b; i != 8; ++i)
		buffer[fill + i - b] = hex_table[(value >> ((7 - i) << 2)) & 0xF];

	return (size_t)digit_count;
}

size_t sdui_print_unsigned(uint32_t value, char* buffer)
{
	size_t size = 0;
	do
	{
		for (char* move = buffer + size++; move != buffer; --move)
			*move = *(move - 1);
		*buffer = '0' + (char)(value % 10);
		value /= 10;
	} while (value);
	return size;// max size is 10
}

size_t sdui_print_signed(int32_t value, char* buffer)
{
	int is_negative = value < 0;
	if (is_negative)
		*buffer = '-';
	return (size_t)is_negative + sdui_print_unsigned((uint32_t)(is_negative ? -value : value), buffer + is_negative);// max size is 11
}

uint32_t sdui_inverse_color(uint32_t color)
{
	uint32_t a = ((color >> 0) & 0xFF);
	uint32_t b = 0xFF - ((color >> 8) & 0xFF);
	uint32_t g = 0xFF - ((color >> 16) & 0xFF);
	uint32_t r = 0xFF - ((color >> 24) & 0xFF);
	return (a << 0) | (b << 8) | (g << 16) | (r << 24);
}