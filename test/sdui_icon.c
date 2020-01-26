/*
	sdui_icon.c - v0.2 (2020-01-26) - public domain
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

#include "sdui_icon.h"
#include <assert.h>

void sdui_draw_dropdown_icon_bitmap(int w, int h, size_t stride, uint32_t background_color, uint32_t icon_color, uint32_t* image)
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

void sdui_draw_minus_icon_bitmap(int w, int h, size_t stride, uint32_t background_color, uint32_t icon_color, uint32_t* image)
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
	int icon_h = icon_w / 4;
	int h_skip = (w / 2) - (icon_w / 2);
	int v_skip = (h / 2) - (icon_h / 2);

	for (int y = 0; y != v_skip; ++y)
		for (int x = 0; x != w; ++x)
			image[y * stride + x] = background_color;
	for (int y = v_skip; y != (v_skip + icon_h); ++y)
	{
		for (int x = 0; x != h_skip; ++x)
			image[y * stride + x] = background_color;
		for (int x = h_skip; x != h_skip + icon_w; ++x)
			image[y * stride + x] = icon_color;
		for (int x = h_skip + icon_w; x != w; ++x)
			image[y * stride + x] = background_color;
	}
	for (int y = v_skip + icon_h; y != h; ++y)
		for (int x = 0; x != w; ++x)
			image[y * stride + x] = background_color;
}

void sdui_draw_plus_icon_bitmap(int w, int h, size_t stride, uint32_t background_color, uint32_t icon_color, uint32_t* image)
{
	assert(!(stride % sizeof(uint32_t)));
	stride /= sizeof(uint32_t);

	int icon_l;
	if (w > h)
	{
		if (h > 2)
			icon_l = ((h - 2) / 2) + ((h - 2) / 4);
		else
			icon_l = h;
	}
	else
	{
		if (w > 2)
			icon_l = ((w - 2) / 2) + ((w - 2) / 4);
		else
			icon_l = w;
	}
	int h_skip = (w / 2) - (icon_l / 2);
	int v_skip = (h / 2) - (icon_l / 2);

	int icon_t = icon_l / 4;
	int yl = v_skip + (icon_l / 2) - (icon_t / 2);
	int yh = yl + icon_t;
	int xl = h_skip + (icon_l / 2) - (icon_t / 2);
	int xh = xl + icon_t;

	for (int y = 0; y != v_skip; ++y)
		for (int x = 0; x != w; ++x)
			image[y * stride + x] = background_color;
	for (int y = v_skip; y != (v_skip + icon_l); ++y)
	{
		if (y >= yl && y < yh)
		{
			for (int x = 0; x != h_skip; ++x)
				image[y * stride + x] = background_color;
			for (int x = h_skip; x != h_skip + icon_l; ++x)
				image[y * stride + x] = icon_color;
			for (int x = h_skip + icon_l; x != w; ++x)
				image[y * stride + x] = background_color;
		}
		else
		{
			for (int x = 0; x != xl; ++x)
				image[y * stride + x] = background_color;
			for (int x = xl; x != xh; ++x)
				image[y * stride + x] = icon_color;
			for (int x = xh; x != w; ++x)
				image[y * stride + x] = background_color;
		}
	}
	for (int y = v_skip + icon_l; y != h; ++y)
		for (int x = 0; x != w; ++x)
			image[y * stride + x] = background_color;
}