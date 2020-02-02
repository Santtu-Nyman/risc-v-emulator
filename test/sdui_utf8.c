/*
	sdui_utf8.c - v0.4.0 (2020-02-02) - public domain
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

#include "sdui_utf8.h"

int sdui_utf8_to_unicode(size_t utf8_string_size, const char* utf8_string, size_t unicode_buffer_length, size_t* unicode_character_count, uint32_t* unicode_buffer)
{
	int invalid_byte_sequence_found = 0;
	size_t character_count = 0;
	for (size_t character_offset = 0; character_offset != utf8_string_size;)
	{
		int invalid_data = 0;
		size_t character_size;
		uint32_t character_code;
		uint32_t utf8_data;
		int high_set_bit_count;
		size_t character_size_limit = utf8_string_size - character_offset;
		if (character_size_limit > 3)
			utf8_data =
			(uint32_t)*(((const uint8_t*)utf8_string) + character_offset) |
			((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + 1) << 8) |
			((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + 2) << 16) |
			((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + 3) << 24);
		else
		{
			utf8_data = 0;
			for (size_t byte_index = 0; byte_index != character_size_limit; ++byte_index)
				utf8_data |= ((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + byte_index) << (byte_index << 3));
		}
		high_set_bit_count = 0;
		while (high_set_bit_count != 5 && (utf8_data & (uint32_t)(0x80 >> high_set_bit_count)))
			++high_set_bit_count;
		switch (high_set_bit_count)
		{
			case 0:
			{
				character_size = 1;
				character_code = utf8_data & 0x7F;
				break;
			}
			case 2:
			{
				if (((utf8_data >> 14) & 0x03) == 0x02)
				{
					character_size = 2;
					character_code = ((utf8_data & 0x1F) << 6) | ((utf8_data >> 8) & 0x3F);
				}
				else
					invalid_data = 1;
				break;
			}
			case 3:
			{
				if (((utf8_data >> 14) & 0x03) == 0x02 && ((utf8_data >> 22) & 0x03) == 0x02)
				{
					character_size = 3;
					character_code = ((utf8_data & 0x0F) << 12) | (((utf8_data >> 8) & 0x3F) << 6) | ((utf8_data >> 16) & 0x3F);
				}
				else
					invalid_data = 1;
				break;
			}
			case 4:
			{
				if (((utf8_data >> 14) & 0x03) == 0x02 && ((utf8_data >> 22) & 0x03) == 0x02 && ((utf8_data >> 30) & 0x03) == 0x02)
				{
					character_size = 4;
					character_code = ((utf8_data & 0x0F) << 18) | (((utf8_data >> 8) & 0x3F) << 12) | (((utf8_data >> 16) & 0x3F) << 6) | ((utf8_data >> 24) & 0x3F);
				}
				else
					invalid_data = 1;
				break;
			}
			default:
			{
				invalid_data = 1;
				break;
			}
		}
		if (invalid_data)
		{
			character_size = 1;
			while (character_size != character_size_limit && invalid_data)
			{
				utf8_data = (uint32_t)*(((const uint8_t*)utf8_string) + character_offset + character_size);
				high_set_bit_count = 0;
				while (high_set_bit_count != 5 && (utf8_data & (uint32_t)(0x80 >> high_set_bit_count)))
					++high_set_bit_count;
				switch (high_set_bit_count)
				{
					case 0:
					{
						invalid_data = 0;
						break;
					}
					case 2:
					{
						if (character_size_limit > (character_size + 1) &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 1) >> 6) == 0x02)
							invalid_data = 0;
						break;
					}
					case 3:
					{
						if (character_size_limit > (character_size + 2) &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 1) >> 6) == 0x02 &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 2) >> 6) == 0x02)
							invalid_data = 0;
						break;
					}
					case 5:
					{
						if (character_size_limit > (character_size + 2) &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 1) >> 6) == 0x02 &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 2) >> 6) == 0x02 &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 3) >> 6) == 0x02)
							invalid_data = 0;
						break;
					}
					default:
						break;
				}
				if (invalid_data)
					++character_size;
			}
			invalid_byte_sequence_found = 1;
			character_code = 0xFFFD;
		}
		if (character_count < unicode_buffer_length)
			unicode_buffer[character_count] = character_code;
		++character_count;
		character_offset += character_size;
	}
	*unicode_character_count = character_count;
	if (character_count > unicode_buffer_length)
		return ENOBUFS;
	else if (invalid_byte_sequence_found)
		return EILSEQ;
	else
		return 0;
}

int sdui_unicode_to_utf8(size_t unicode_string_size, const uint32_t* unicode_string, size_t utf8_buffer_length, size_t* utf8_byte_count, char* utf8_buffer)
{
	int invalid_character_found = 0;
	size_t byte_count = 0;
	for (size_t character_index = 0; character_index != unicode_string_size; ++character_index)
	{
		uint32_t character = unicode_string[character_index];
		if (character < 0x80)
		{
			if (byte_count + 1 <= utf8_buffer_length)
				*(uint8_t*)(utf8_buffer + byte_count) = (uint8_t)character;
			++byte_count;
		}
		else if (character < 0x800)
		{
			if (byte_count + 2 <= utf8_buffer_length)
			{
				*((uint8_t*)utf8_buffer + byte_count + 0) = 0xC0 | (uint8_t)(character >> 6);
				*((uint8_t*)utf8_buffer + byte_count + 1) = 0x80 | (uint8_t)(character & 0x3F);
			}
			byte_count += 2;
		}
		else if (character < 0x10000)
		{
			if (byte_count + 3 <= utf8_buffer_length)
			{
				*((uint8_t*)utf8_buffer + byte_count + 0) = 0xE0 | (uint8_t)(character >> 12);
				*((uint8_t*)utf8_buffer + byte_count + 1) = 0x80 | (uint8_t)((character >> 6) & 0x3F);
				*((uint8_t*)utf8_buffer + byte_count + 2) = 0x80 | (uint8_t)(character & 0x3F);
			}
			byte_count += 3;
		}
		else if (character < 0x200000)
		{
			if (byte_count + 4 <= utf8_buffer_length)
			{
				*((uint8_t*)utf8_buffer + byte_count + 0) = 0xF0 | (uint8_t)(character >> 18);
				*((uint8_t*)utf8_buffer + byte_count + 1) = 0x80 | (uint8_t)((character >> 12) & 0x3F);
				*((uint8_t*)utf8_buffer + byte_count + 2) = 0x80 | (uint8_t)((character >> 6) & 0x3F);
				*((uint8_t*)utf8_buffer + byte_count + 3) = 0x80 | (uint8_t)(character & 0x3F);
			}
			byte_count += 4;
		}
		else
		{
			if (byte_count + 3 <= utf8_buffer_length)
			{
				*((uint8_t*)utf8_buffer + byte_count + 0) = 0xEF;
				*((uint8_t*)utf8_buffer + byte_count + 1) = 0xBF;
				*((uint8_t*)utf8_buffer + byte_count + 2) = 0xBD;
			}
			byte_count += 3;
			invalid_character_found = 1;
		}
	}
	*utf8_byte_count = byte_count;
	if (byte_count > utf8_buffer_length)
		return ENOBUFS;
	else if (invalid_character_found)
		return EILSEQ;
	else
		return 0;
}